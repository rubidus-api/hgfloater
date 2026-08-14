/* The settings window.
 *
 * A list of rows over three tables that already exist: the switches
 * (hg_options.c), the numbers (hg_values.c), and window/function/keys
 * (hg_keys.c). The window holds no copy of any of them - it reads them when it
 * draws and writes through them when a key is pressed - so it cannot drift out
 * of step with the O menu or the command box, and there is nothing to apply or
 * cancel.
 *
 * The rows are a flat list rather than a tree control. The three levels of a
 * key binding are still visible - the window is a heading, the function is a
 * row, its chords are the row's right-hand column - and a flat list is what
 * lets one pair of arrow keys walk every setting in the program without ever
 * expanding anything. */
#include "hg_settings.h"
#include "../hg_utils.h"
#include "../hg_globals.h"
#include "../hg_options.h"
#include "../hg_values.h"
#include "../hg_keys.h"

#define HG_SETTINGS_LIST_ID 100
#define HG_SETTINGS_STATUS_ID 101

#define HG_SETTINGS_FONT_MIN 8
#define HG_SETTINGS_FONT_MAX 72
#define HG_SETTINGS_FONT_DEFAULT 15

enum {
    HG_ROW_HEADING = 1,
    HG_ROW_OPTION,
    HG_ROW_VALUE,
    HG_ROW_KEY,  /* a function */
    HG_ROW_CHORD /* one of that function's chords */
};

typedef struct HgSettingsRow {
    int kind;
    int number; /* option number, value number, or key action */
    int index;  /* HG_ROW_CHORD: which of the action's chords */
} HgSettingsRow;

#define HG_SETTINGS_MAX_ROWS 256

static HWND s_settings_wnd = NULL;
static HFONT s_settings_font = NULL;
static int s_settings_font_pt = HG_SETTINGS_FONT_DEFAULT;
static BYTE s_settings_alpha = 255;
static BOOL s_settings_view_loaded = FALSE;

static HgSettingsRow s_rows[HG_SETTINGS_MAX_ROWS];
static int s_row_count = 0;

/* The action waiting for a chord, or 0. While one is waiting the list answers
 * no keys of its own: whatever is pressed next is the binding, which is the
 * only way to bind a key that the list itself uses. */
static int s_capture_action = 0;

/* Set while the chord being pressed is meant to replace one that is already
 * there. The old chord is dropped only once the new one is in, so pressing Esc
 * halfway through leaves the binding exactly as it was. */
static BOOL s_capture_replaces = FALSE;
static HgChord s_capture_replaced = {0, 0};

static void settings_fill(HWND hwnd);

BOOL hg_settings_capturing(void)
{
    return s_capture_action != 0;
}

/* ------------------------------------------------------------------ view */

static void settings_write_int(const WCHAR *key, int value)
{
    WCHAR text[16];
    hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d", value);
    WritePrivateProfileStringW(L"settings", key, text, hg_g_config_path);
}

static void settings_load_view(void)
{
    if (s_settings_view_loaded)
        return;
    s_settings_view_loaded = TRUE;

    int pt = (int)GetPrivateProfileIntW(L"settings", L"font_size", HG_SETTINGS_FONT_DEFAULT, hg_g_config_path);
    if (pt < HG_SETTINGS_FONT_MIN)
        pt = HG_SETTINGS_FONT_MIN;
    if (pt > HG_SETTINGS_FONT_MAX)
        pt = HG_SETTINGS_FONT_MAX;
    s_settings_font_pt = pt;

    int alpha = (int)GetPrivateProfileIntW(L"settings", L"alpha", 255, hg_g_config_path);
    if (alpha < 32)
        alpha = 32;
    if (alpha > 255)
        alpha = 255;
    s_settings_alpha = (BYTE)alpha;
}

/* Fixed pitch, unlike the other document windows: every row here is two or
 * three columns, and columns that do not line up are columns nobody reads. */
static void settings_apply_font(HWND hwnd)
{
    settings_load_view();
    double ws = hg_window_scale(hwnd);
    HFONT font = CreateFontW(SCW(ws, s_settings_font_pt), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, hg_g_commandbox_font_name);
    if (!font)
        return;

    const int ids[] = {HG_SETTINGS_LIST_ID, HG_SETTINGS_STATUS_ID};
    for (int i = 0; i < (int)HG_ARRAYSIZE(ids); ++i) {
        HWND child = GetDlgItem(hwnd, ids[i]);
        if (child)
            SendMessageW(child, WM_SETFONT, (WPARAM)font, TRUE);
    }
    if (s_settings_font)
        DeleteObject(s_settings_font);
    s_settings_font = font;
}

static void settings_layout(HWND hwnd, int width, int height)
{
    double ws = hg_window_scale(hwnd);
    int pad = SCW(ws, 8);
    int status_h = SCW(ws, s_settings_font_pt) + SCW(ws, 10);

    HWND status = GetDlgItem(hwnd, HG_SETTINGS_STATUS_ID);
    HWND list = GetDlgItem(hwnd, HG_SETTINGS_LIST_ID);

    int list_h = height - status_h - pad * 3;
    if (list_h < 0)
        list_h = 0;
    int inner_w = (width - pad * 2 > 0) ? width - pad * 2 : 0;

    if (list)
        MoveWindow(list, pad, pad, inner_w, list_h, TRUE);
    if (status)
        MoveWindow(status, pad, pad * 2 + list_h, inner_w, status_h, TRUE);
}

static void settings_say(HWND hwnd, const WCHAR *text)
{
    HWND status = GetDlgItem(hwnd, HG_SETTINGS_STATUS_ID);
    if (status)
        SetWindowTextW(status, text ? text : L"");
}

/* ------------------------------------------------------------------ rows */

static void settings_add_row_at(HWND list, int kind, int number, int index, const WCHAR *text)
{
    if (s_row_count >= HG_SETTINGS_MAX_ROWS)
        return;
    s_rows[s_row_count].kind = kind;
    s_rows[s_row_count].number = number;
    s_rows[s_row_count].index = index;
    SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)text);
    ++s_row_count;
}

static void settings_add_row(HWND list, int kind, int number, const WCHAR *text)
{
    settings_add_row_at(list, kind, number, 0, text);
}

static void settings_fill(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_SETTINGS_LIST_ID);
    if (!list)
        return;

    int selected = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    int top = (int)SendMessageW(list, LB_GETTOPINDEX, 0, 0);

    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    s_row_count = 0;

    WCHAR line[256];

    settings_add_row(list, HG_ROW_HEADING, 0, L"OPTIONS");
    for (int i = 1; i <= hg_option_count(); ++i) {
        HgOptionInfo info;
        if (!hg_option_info(i, &info))
            continue;
        const WCHAR *state = !info.available
                                 ? (info.unavailable_note ? info.unavailable_note : L"unavailable")
                                 : (hg_option_get(i) ? L"on" : L"off");
        StringCchPrintfW(line, HG_ARRAYSIZE(line), L"   %-34ls %ls", info.label, state);
        settings_add_row(list, HG_ROW_OPTION, i, line);
    }

    settings_add_row(list, HG_ROW_HEADING, 0, L"");
    settings_add_row(list, HG_ROW_HEADING, 0, L"VALUES");
    for (int i = 1; i <= hg_value_count(); ++i) {
        HgValueInfo info;
        int value = 0;
        if (!hg_value_info(i, &info) || !hg_value_get(i, &value))
            continue;
        StringCchPrintfW(line, HG_ARRAYSIZE(line), L"   %-34ls %d%ls   (%d-%d)", info.name, value, info.unit,
                         info.min, info.max);
        settings_add_row(list, HG_ROW_VALUE, i, line);
    }

    /* Window, function, keys - the headings are the windows, so the three
     * levels read down the page in the order they are spoken. */
    int last_context = 0;
    for (int action = 1; action <= hg_key_action_count(); ++action) {
        HgKeyActionInfo info;
        if (!hg_key_action_info(action, &info))
            continue;

        if (info.context != last_context) {
            last_context = info.context;
            settings_add_row(list, HG_ROW_HEADING, 0, L"");
            StringCchPrintfW(line, HG_ARRAYSIZE(line), L"KEYS - %ls (%ls)", hg_key_context_name(info.context),
                             hg_key_context_summary(info.context));
            settings_add_row(list, HG_ROW_HEADING, 0, line);
        }

        /* The function is one row and each of its chords is another, indented
         * under it. A single row listing "Ctrl+N, Ctrl+Shift+N" could only ever
         * offer to delete the last one, which is not the one anybody means: a
         * chord you can point at is a chord you can remove. */
        int count = hg_key_binding_count(action);
        StringCchPrintfW(line, HG_ARRAYSIZE(line), L"   %-34ls %ls", info.label,
                         (s_capture_action == action && !s_capture_replaces)
                             ? L"[press a key, Esc to stop]"
                             : (count ? L"" : L"(no key)"));
        settings_add_row(list, HG_ROW_KEY, action, line);

        for (int i = 0; i < count; ++i) {
            HgChord chord;
            WCHAR text[64];
            if (!hg_key_binding_chord(action, i, &chord) || !hg_key_chord_text(chord, text, HG_ARRAYSIZE(text)))
                continue;
            if (s_capture_action == action && s_capture_replaces && s_capture_replaced.vk == chord.vk &&
                s_capture_replaced.mods == chord.mods)
                StringCchCopyW(text, HG_ARRAYSIZE(text), L"[press the key that replaces it, Esc to stop]");
            StringCchPrintfW(line, HG_ARRAYSIZE(line), L"        %ls", text);
            settings_add_row_at(list, HG_ROW_CHORD, action, i, line);
        }
    }

    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, NULL, TRUE);

    if (selected >= 0 && selected < s_row_count)
        SendMessageW(list, LB_SETCURSEL, (WPARAM)selected, 0);
    if (top >= 0)
        SendMessageW(list, LB_SETTOPINDEX, (WPARAM)top, 0);
}

static BOOL settings_selected_row(HWND hwnd, HgSettingsRow *out)
{
    HWND list = GetDlgItem(hwnd, HG_SETTINGS_LIST_ID);
    if (!list || !out)
        return FALSE;
    int row = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    if (row < 0 || row >= s_row_count)
        return FALSE;
    *out = s_rows[row];
    return TRUE;
}

/* The line under the list says what the row under the cursor answers to. A
 * window whose keys differ per row has to say which ones, where the eye already
 * is, or they are keys nobody finds. */
static void settings_describe(HWND hwnd)
{
    HgSettingsRow row;
    if (s_capture_action) {
        settings_say(hwnd, L"Press the chord to add.  Esc cancels.");
        return;
    }
    if (!settings_selected_row(hwnd, &row)) {
        settings_say(hwnd, L"Up/Down to walk the list.");
        return;
    }

    switch (row.kind) {
    case HG_ROW_OPTION:
        settings_say(hwnd, L"Enter or Space switches it.");
        break;
    case HG_ROW_VALUE:
        settings_say(hwnd, L"Left/Right changes it - by 5 for a percentage, by 1 otherwise.");
        break;
    case HG_ROW_KEY:
        settings_say(hwnd, L"Enter adds a key.  Del takes them all away, R restores the default.");
        break;
    case HG_ROW_CHORD:
        settings_say(hwnd, L"Del removes this key.  Enter replaces it with the next one you press.");
        break;
    default:
        settings_say(hwnd, L"Up/Down to walk the list.");
        break;
    }
}

/* ------------------------------------------------------------- the actions */

static void settings_toggle_option(HWND hwnd, int number)
{
    const WCHAR *message = NULL;
    if (!hg_option_set(number, hg_option_get(number) ? FALSE : TRUE, &message)) {
        settings_say(hwnd, message ? message : L"That option cannot be changed here.");
        return;
    }
    settings_fill(hwnd);
    if (message)
        settings_say(hwnd, message);
}

static void settings_step_value(HWND hwnd, int number, int direction)
{
    HgValueInfo info;
    int value = 0;
    if (!hg_value_info(number, &info) || !hg_value_get(number, &value))
        return;

    /* Percentages move by five. A opacity that took twenty presses to cross the
     * range would be a number nobody sets from the keyboard. */
    int step = (info.unit && wcscmp(info.unit, L"%") == 0) ? 5 : 1;
    hg_value_set(number, value + direction * step, NULL);
    settings_fill(hwnd);

    int now = 0;
    hg_value_get(number, &now);
    WCHAR text[128];
    StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%ls  %d%ls", info.name, now, info.unit);
    settings_say(hwnd, text);
}

static void settings_capture_chord(HWND hwnd, UINT vk)
{
    int action = s_capture_action;
    HgKeyActionInfo info;
    if (!action || !hg_key_action_info(action, &info)) {
        s_capture_action = 0;
        return;
    }

    /* A modifier on its own is half a chord: keep waiting rather than binding
     * "Ctrl" to anything. */
    if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT || vk == VK_LWIN || vk == VK_RWIN)
        return;

    if (vk == VK_ESCAPE) {
        s_capture_action = 0;
        s_capture_replaces = FALSE;
        settings_fill(hwnd);
        settings_say(hwnd, L"Nothing changed.");
        return;
    }

    HgChord chord = {vk, hg_key_current_mods()};
    WCHAR text[64];
    if (!hg_key_chord_text(chord, text, HG_ARRAYSIZE(text))) {
        s_capture_action = 0;
        s_capture_replaces = FALSE;
        settings_fill(hwnd);
        settings_say(hwnd, L"That key has no name this program can write down.");
        return;
    }

    s_capture_action = 0;
    BOOL replacing = s_capture_replaces;
    HgChord replaced = s_capture_replaced;
    s_capture_replaces = FALSE;

    int conflict = 0;
    WCHAR message[220];
    if (hg_key_add(action, text, &conflict)) {
        /* The old chord goes only now, with the new one already in place: a
         * replacement that failed halfway would otherwise leave the function
         * with one key fewer than it started with. */
        if (replacing) {
            WCHAR old_text[64];
            if (hg_key_chord_text(replaced, old_text, HG_ARRAYSIZE(old_text)))
                hg_key_remove(action, old_text);
        }
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls now runs %ls.", text, info.label);
    } else if (replacing && chord.vk == replaced.vk && chord.mods == replaced.mods) {
        /* Replacing a chord with itself is not a conflict, it is a no-op. */
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls left as it was.", text);
    } else if (conflict) {
        HgKeyActionInfo other;
        if (hg_key_action_info(conflict, &other))
            StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls already runs %ls in this window.", text,
                             other.label);
        else
            StringCchCopyW(message, HG_ARRAYSIZE(message), L"That chord is already taken here.");
    } else if (hg_key_binding_count(action) >= HG_KEY_MAX_BINDINGS) {
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls already has %d keys, which is the most.",
                         info.label, HG_KEY_MAX_BINDINGS);
    } else if (chord.mods & HG_KMOD_WIN) {
        StringCchCopyW(message, HG_ARRAYSIZE(message),
                       L"Win+ belongs to the system window alone - the shell takes it first.");
    } else {
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls could not be added.", text);
    }

    settings_fill(hwnd);
    settings_say(hwnd, message);
}

/* The function row: add a key, take them all away, or go back to the default. */
static void settings_key_row_command(HWND hwnd, int action, UINT vk)
{
    HgKeyActionInfo info;
    if (!hg_key_action_info(action, &info))
        return;

    WCHAR message[220];
    WCHAR keys[160];

    if (vk == VK_RETURN) {
        s_capture_action = action;
        s_capture_replaces = FALSE;
        settings_fill(hwnd);
        settings_describe(hwnd);
        return;
    }

    if (vk == VK_DELETE) {
        if (hg_key_binding_count(action) == 0) {
            settings_say(hwnd, L"That function has no key to take away.");
            return;
        }
        hg_key_clear(action);
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls has no key now.", info.label);
        settings_fill(hwnd);
        settings_say(hwnd, message);
        return;
    }

    if (vk == 'R') {
        hg_key_reset(action);
        hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls back to its default: %ls", info.label, keys);
        settings_fill(hwnd);
        settings_say(hwnd, message);
    }
}

/* A chord row: this one, by name. */
static void settings_chord_row_command(HWND hwnd, int action, int index, UINT vk)
{
    HgKeyActionInfo info;
    HgChord chord;
    WCHAR text[64];
    if (!hg_key_action_info(action, &info) || !hg_key_binding_chord(action, index, &chord) ||
        !hg_key_chord_text(chord, text, HG_ARRAYSIZE(text)))
        return;

    WCHAR message[220];

    if (vk == VK_DELETE) {
        hg_key_remove(action, text);
        WCHAR keys[160];
        hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
        StringCchPrintfW(message, HG_ARRAYSIZE(message), L"%ls dropped.  %ls now: %ls", text, info.label, keys);
        settings_fill(hwnd);
        settings_say(hwnd, message);
        return;
    }

    if (vk == VK_RETURN) {
        s_capture_action = action;
        s_capture_replaces = TRUE;
        s_capture_replaced = chord;
        settings_fill(hwnd);
        settings_describe(hwnd);
    }
}

/* Every key the list answers, in one place, because the list is a child control
 * and the keys would otherwise never reach the window. */
static BOOL settings_list_key(HWND parent, UINT vk)
{
    BOOL ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL alt = (GetKeyState(VK_MENU) < 0);

    if (s_capture_action) {
        settings_capture_chord(parent, vk);
        return TRUE;
    }


    if (alt)
        return FALSE; /* Alt+arrows move the window; the list wants none of it */

    if (vk == VK_ESCAPE || (ctrl && vk == 'W')) {
        PostMessageW(parent, WM_CLOSE, 0, 0);
        return TRUE;
    }

    HgSettingsRow row;
    if (!settings_selected_row(parent, &row))
        return FALSE;

    switch (row.kind) {
    case HG_ROW_OPTION:
        if (vk == VK_RETURN || vk == VK_SPACE) {
            settings_toggle_option(parent, row.number);
            return TRUE;
        }
        break;
    case HG_ROW_VALUE:
        if (vk == VK_LEFT || vk == VK_RIGHT) {
            settings_step_value(parent, row.number, (vk == VK_RIGHT) ? 1 : -1);
            return TRUE;
        }
        break;
    case HG_ROW_KEY:
        if (vk == VK_RETURN || vk == VK_DELETE || vk == 'R') {
            settings_key_row_command(parent, row.number, vk);
            return TRUE;
        }
        break;
    case HG_ROW_CHORD:
        if (vk == VK_RETURN || vk == VK_DELETE) {
            settings_chord_row_command(parent, row.number, row.index, vk);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/* ------------------------------------------------------------ the window */

/* Text size, opacity, move and resize: the four every document window here
 * answers, in the shape hg_clip.c and the command box use. */
static BOOL settings_adjust_message(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (!wnd)
        return FALSE;

    if (msg == WM_MOUSEWHEEL) {
        BOOL ctrl = (LOWORD(w_param) & MK_CONTROL) || (GetKeyState(VK_CONTROL) < 0);
        BOOL alt = (GetKeyState(VK_MENU) < 0);
        int delta = ((short)HIWORD(w_param) > 0) ? 1 : -1;

        if (alt) {
            if (hg_step_alpha_value(&s_settings_alpha, delta)) {
                SetLayeredWindowAttributes(wnd, 0, s_settings_alpha, LWA_ALPHA);
                settings_write_int(L"alpha", (int)s_settings_alpha);
            }
            return TRUE;
        }
        if (ctrl) {
            int pt = s_settings_font_pt + delta;
            if (pt < HG_SETTINGS_FONT_MIN)
                pt = HG_SETTINGS_FONT_MIN;
            if (pt > HG_SETTINGS_FONT_MAX)
                pt = HG_SETTINGS_FONT_MAX;
            if (pt != s_settings_font_pt) {
                s_settings_font_pt = pt;
                settings_write_int(L"font_size", pt);
                settings_apply_font(wnd);
                RECT rc;
                GetClientRect(wnd, &rc);
                settings_layout(wnd, rc.right, rc.bottom);
            }
            return TRUE;
        }
        return FALSE;
    }

    if (msg != WM_KEYDOWN && msg != WM_SYSKEYDOWN)
        return FALSE;

    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);
    if (!is_alt)
        return FALSE; /* Ctrl+arrows belong to the rows here, not to the frame */
    (void)is_ctrl;

    int step = SCW(hg_window_scale(wnd), 20);
    int dx = 0, dy = 0;
    if (w_param == VK_LEFT)
        dx = -step;
    else if (w_param == VK_RIGHT)
        dx = step;
    else if (w_param == VK_UP)
        dy = -step;
    else if (w_param == VK_DOWN)
        dy = step;

    if (dx || dy) {
        move_window_by_offset(wnd, dx, dy);
        return TRUE;
    }
    (void)l_param;
    return FALSE;
}

static LRESULT CALLBACK settings_list_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                                    UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;

    HWND parent = GetParent(hwnd);

    /* While a chord is being captured this control answers nothing else - not
     * the wheel, not Alt+arrows - because anything it did answer would be a
     * chord that cannot be bound. */
    if (!s_capture_action && settings_adjust_message(parent, msg, w_param, l_param))
        return 0;

    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        if (settings_list_key(parent, (UINT)w_param))
            return 0;
    }

    /* A listbox jumps to the row whose text starts with the letter typed. Here
     * the letters are commands ('R' restores a default) and the rows all start
     * with spaces, so the search would do nothing but eat them. */
    if (msg == WM_CHAR)
        return 0;

    LRESULT result = DefSubclassProc(hwnd, msg, w_param, l_param);
    if (msg == WM_KEYDOWN || msg == WM_LBUTTONUP)
        settings_describe(parent);
    return result;
}

LRESULT CALLBACK settings_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE: {
        hg_apply_dwm_attributes_document(hwnd);
        HINSTANCE instance = GetModuleHandleW(NULL);

        CreateWindowExW(0, L"LISTBOX", NULL,
                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_HASSTRINGS, 0, 0, 0,
                        0, hwnd, (HMENU)HG_SETTINGS_LIST_ID, instance, NULL);
        CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd,
                        (HMENU)HG_SETTINGS_STATUS_ID, instance, NULL);

        HWND list = GetDlgItem(hwnd, HG_SETTINGS_LIST_ID);
        if (list)
            SetWindowSubclass(list, settings_list_subclass_proc, 1, 0);

        settings_apply_font(hwnd);
        settings_fill(hwnd);
        if (list)
            SendMessageW(list, LB_SETCURSEL, 1, 0); /* the first row that is not a heading */
        settings_describe(hwnd);
        return 0;
    }

    case WM_SIZE:
        settings_layout(hwnd, LOWORD(l_param), HIWORD(l_param));
        return 0;

    case WM_SETFOCUS: {
        HWND list = GetDlgItem(hwnd, HG_SETTINGS_LIST_ID);
        if (list)
            SetFocus(list);
        return 0;
    }

    case WM_ACTIVATE:
        /* Whatever changed while this window was away - a wheel over the
         * toolbar, a menu item, a command line - is what it should be showing.
         * It is a view, so it re-reads rather than remembers. */
        if (LOWORD(w_param) != WA_INACTIVE)
            settings_fill(hwnd);
        return 0;

    case WM_MOUSEWHEEL:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (settings_adjust_message(hwnd, msg, w_param, l_param))
            return 0;
        if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && settings_list_key(hwnd, (UINT)w_param))
            return 0;
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        return hg_on_ctlcolor_document((HDC)w_param);

    case WM_DPICHANGED:
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        settings_apply_font(hwnd);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        s_capture_action = 0;
        s_capture_replaces = FALSE;
        s_settings_wnd = NULL;
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

void hg_settings_toggle_window(void)
{
    if (s_settings_wnd && IsWindow(s_settings_wnd)) {
        if (IsWindowVisible(s_settings_wnd) && !IsIconic(s_settings_wnd) &&
            GetForegroundWindow() == s_settings_wnd) {
            ShowWindow(s_settings_wnd, SW_HIDE);
            return;
        }
        settings_fill(s_settings_wnd);
        ShowWindow(s_settings_wnd, IsIconic(s_settings_wnd) ? SW_RESTORE : SW_SHOWNORMAL);
        hg_force_foreground(s_settings_wnd);
        return;
    }

    POINT pt = {0, 0};
    GetCursorPos(&pt);
    double ws = hg_point_scale(pt);

    s_settings_wnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_LAYERED, HG_CLASS_SETTINGS, L"Settings",
                                     WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                                         WS_THICKFRAME | WS_CLIPCHILDREN,
                                     pt.x, pt.y, SCW(ws, 620), SCW(ws, 520), NULL, NULL,
                                     GetModuleHandleW(NULL), NULL);
    if (!s_settings_wnd)
        return;

    settings_load_view();
    SetLayeredWindowAttributes(s_settings_wnd, 0, s_settings_alpha, LWA_ALPHA);
    ensure_window_visible(s_settings_wnd, NULL);
    ShowWindow(s_settings_wnd, SW_SHOWNORMAL);
    hg_force_foreground(s_settings_wnd);
}
