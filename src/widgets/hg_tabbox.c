/* The tab sub-box. See hg_tabbox.h and docs/RFC-2026-07-tabs-as-task-icons.md
 * section D8 for why the tabs left the grid.
 *
 * Three properties shape the code:
 *
 * - It never takes the focus. WS_EX_NOACTIVATE, and the keys arrive because
 *   the toolbar offers them here before using them itself. A hover box that
 *   stole the keyboard would make hovering a destructive act.
 * - It draws from the cache and asks in the background. Opening posts one
 *   request for one window; the rows appear immediately from whatever was
 *   known, and HG_MSG_TABS_READY brings the fresh answer through
 *   hg_tabbox_refresh. Nothing is ever waited for.
 * - It is one box. Opening it for another icon re-targets the same window
 *   rather than stacking a second one. */
#include "hg_tabbox.h"
#include "../hg_utils.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"
#include "hg_taskbox.h"
#include "../hg_options.h"
#include "../hg_keys.h"

/* Whether the keyboard is *in* the box.
 *
 * An open box and a focused box are different states, because the letters
 * that label its rows are also the taskbox's own keys - WASD moves the grid,
 * C opens the command box, N the notes. A box that grabbed those the moment
 * it appeared would break navigation every time the focus passed a browser.
 *
 * So: an open box takes the digits (which the grid does not use) and Tab.
 * Tab enters it, and a focused box takes everything - arrows, letters, Enter,
 * Space. Escape leaves, closing the box, which is where the reader started. */
static BOOL s_focused = FALSE;
static HWND s_wnd = NULL;
static HWND s_target = NULL;
static RECT s_anchor = {0, 0, 0, 0};

/* Rows enough for the longest of the four lists. That used to be the folders,
 * capped by the shortcuts folder; it is the options menu now, which grows by
 * about a dozen rows per display once its submenus are flattened out. */
#define HG_BOX_MAX_ROWS 128
static WCHAR s_titles[HG_BOX_MAX_ROWS][HG_MAX_STR];
static int s_count = 0;
static int s_selected = 0;
static int s_mode = HG_BOX_TABS;
static BOOL s_open = FALSE;

/* Shown for a row whose text does not fit, whether the pointer found it or the
 * arrows did. The box is already as wide as it is allowed to be by then, so the
 * choice is between a tooltip and a title nobody can read. Tracked rather than
 * hovered, because the keyboard has no pointer to hover with. */
static HWND s_tip = NULL;
static int s_tip_row = -1;

/* The control list, in one table.
 *
 * Two kinds of row: the buttons that left the toolbar, and the switches that
 * were a submenu of the O menu. They are the same kind of thing to a reader -
 * something to set, right here - and having them in two places meant knowing
 * which of the two a given setting had been filed under. A row is a kind and an
 * id; everything else about it is asked of the table that owns that kind.
 *
 * A row's label is the whole name rather than three letters: this list is text,
 * and text has room. The value rows say what turns them, because a wheel over a
 * row is not a thing anyone guesses. */
enum {
    HG_ROW_BUTTON = 0, /* hg_toolbar_builtin_* by icon id */
    HG_ROW_OPTION      /* hg_option_* by option number */
};

typedef struct HgControlRow {
    int kind;
    int id;
    const WCHAR *label; /* NULL: ask the table that owns the id */
} HgControlRow;

static HgControlRow s_control_rows[4 + 16];
static int s_control_row_count = 0;

/* The options menu, as rows. Built from the menu itself every time the list is
 * pulled, because half of it - the audio devices, the displays, what is ticked -
 * is a picture of the machine at that moment. */
static HgMenuRow s_menu_rows[HG_BOX_MAX_ROWS];
static int s_menu_row_count = 0;

static void tabbox_add_control_row(HgControlRow row)
{
    if (s_control_row_count < (int)HG_ARRAYSIZE(s_control_rows))
        s_control_rows[s_control_row_count++] = row;
}

/* The list, in the order a reader works down it: the things with a number
 * first, then the things with a state, and last the ones that cannot be touched
 * in this build.
 *
 * Grouped by what a row *is* rather than by where it used to live. What cannot
 * be switched sits at the bottom, because a row nobody can use should not stand
 * between two that can. The options menu is not here at all any more - it is
 * the Opt button, on the row, since a menu reached by opening a list and
 * picking its first line was a menu behind a door. */
static void tabbox_build_control_rows(void)
{
    s_control_row_count = 0;

    /* The three the wheel turns - and, once the row is under the arrows, the
     * left and right keys. */
    const HgControlRow values[] = {
        {HG_ROW_BUTTON, HG_TOOL_ICON_VOLUME, L"Volume (ScrollWheel)"},
        {HG_ROW_BUTTON, HG_TOOL_ICON_BRIGHTNESS, L"Brightness (ScrollWheel)"},
        {HG_ROW_BUTTON, HG_TOOL_ICON_ALPHA, L"Alpha (ScrollWheel)"},
    };
    for (size_t i = 0; i < HG_ARRAYSIZE(values); ++i)
        tabbox_add_control_row(values[i]);

    /* The ones that are on or off, starting with the pin - it is a button
     * rather than a setting in the file, but to a reader it is the same
     * question with the same answer. */
    HgControlRow pin = {HG_ROW_BUTTON, HG_TOOL_ICON_PIN, L"Pin"};
    tabbox_add_control_row(pin);

    for (int i = 1; i <= hg_option_count(); ++i) {
        HgOptionInfo info;
        if (!hg_option_info(i, &info) || !info.available)
            continue;
        HgControlRow row = {HG_ROW_OPTION, i, NULL};
        tabbox_add_control_row(row);
    }

    /* And last, the ones this build cannot switch. They are still listed: a
     * feature that vanishes leaves the reader wondering whether they imagined
     * it, while a row that says why does not. */
    for (int i = 1; i <= hg_option_count(); ++i) {
        HgOptionInfo info;
        if (!hg_option_info(i, &info) || info.available)
            continue;
        HgControlRow row = {HG_ROW_OPTION, i, NULL};
        tabbox_add_control_row(row);
    }
}

/* Labels run 1-9, then a-z, then A-Z: sixty-one of them for a cap of
 * twenty-four, so the alphabet never runs out and the digits - the easiest
 * keys - go to the tabs most likely to be wanted. */
static WCHAR tabbox_label_for(int index)
{
    if (index < 0)
        return 0;
    if (index < 9)
        return (WCHAR)(L'1' + index);
    index -= 9;
    if (index < 26)
        return (WCHAR)(L'a' + index);
    index -= 26;
    if (index < 26)
        return (WCHAR)(L'A' + index);
    return 0;
}

/* The row a key names, or -1. Zero is the last row: it sits beside the 9 on
 * every keyboard and "the end" is the one position a reader can always name
 * without counting. */
static int tabbox_index_for_key(WPARAM key)
{
    if (s_count <= 0)
        return -1;
    if (key == L'0')
        return s_count - 1;
    if (key >= L'1' && key <= L'9')
        return (int)(key - L'1');
    if (key >= L'A' && key <= L'Z') {
        /* Windows delivers letters as uppercase virtual keys; the case the
         * reader sees on the row depends on the shift state at the time. */
        int upper = 9 + 26 + (int)(key - L'A');
        int lower = 9 + (int)(key - L'A');
        return (GetKeyState(VK_SHIFT) < 0) ? upper : lower;
    }
    return -1;
}

static int tabbox_row_height(void)
{
    double ws = hg_window_scale(s_wnd ? s_wnd : hg_g_taskbox_wnd);
    int size = ABS(hg_g_edit_font_size);
    if (size < SC(12))
        size = SC(12);
    (void)ws;
    return size + SC(8);
}

/* How wide the rows would like to be, measured rather than assumed.
 *
 * A fixed width cut titles that had room to spare on a wide screen and left
 * white space on short ones. The box asks its own rows instead, and takes what
 * they need up to a ceiling - past that a list stops being a list and becomes a
 * wall of text, and the rows that are still too long say so through the
 * tooltip. */
static int tabbox_wanted_width(double ws)
{
    int fallback = SCW(ws, 320);
    if (!s_wnd || s_count <= 0)
        return fallback;

    HDC dc = GetDC(s_wnd);
    if (!dc)
        return fallback;

    HFONT font = hg_g_main_font ? hg_g_main_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT old = (HFONT)SelectObject(dc, font);

    int widest = 0;
    for (int i = 0; i < s_count; ++i) {
        WCHAR line[HG_MAX_STR + 8];
        WCHAR label = tabbox_label_for(i);
        if (label)
            hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"  %lc: %ls", label, s_titles[i]);
        else
            hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"  %ls", s_titles[i]);

        SIZE sz = {0, 0};
        if (GetTextExtentPoint32W(dc, line, lstrlenW(line), &sz) && sz.cx > widest)
            widest = sz.cx;
    }

    SelectObject(dc, old);
    ReleaseDC(s_wnd, dc);

    int want = widest + SCW(ws, 24); /* the padding either side, and room for the marker */
    if (want < fallback)
        want = fallback;
    int ceiling = SCW(ws, 760);
    if (want > ceiling)
        want = ceiling;
    return want;
}

static void tabbox_layout(void)
{
    if (!s_wnd)
        return;

    double ws = hg_window_scale(s_wnd);
    int row_h = tabbox_row_height();
    int pad = SCW(ws, 6);
    int rows = (s_count > 0) ? s_count : 1;
    int height = rows * row_h + pad * 2;
    int width = tabbox_wanted_width(ws);

    /* Above or below the icon, flush against it, never beside it: the icon is
     * what the reader is pointing at and what they will point at next, so the
     * one thing this box must not cover is that. Four placements - the box
     * running right or left from the icon, sitting under it or over it - and
     * the first that fits the work area whole is the one used.
     *
     * Running "right" means the box's left edge starts at the icon's left, so
     * the icon's whole width lands inside the box's width and the two read as
     * attached. Running "left" mirrors it from the icon's right edge. */
    HMONITOR monitor = MonitorFromRect(&s_anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    RECT work = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (GetMonitorInfoW(monitor, &mi))
        work = mi.rcWork;

    int x_right = s_anchor.left;          /* box runs right from the icon's left edge */
    int x_left = s_anchor.right - width;  /* box runs left from the icon's right edge */
    BOOL fits_right = (x_right >= work.left) && (x_right + width <= work.right);
    BOOL fits_left = (x_left >= work.left) && (x_left + width <= work.right);

    int x;
    if (fits_right)
        x = x_right;
    else if (fits_left)
        x = x_left;
    else {
        /* Wider than the display it is on. Nothing to choose between, so keep
         * it on screen; the icon stays uncovered either way, because what
         * decides that is above-or-below and not this. */
        x = work.left;
        if (width > work.right - work.left)
            width = work.right - work.left;
    }

    int y_below = s_anchor.bottom;         /* flush under the icon */
    int y_above = s_anchor.top - height;   /* flush over it */
    BOOL fits_below = (y_below + height <= work.bottom) && (y_below >= work.top);
    BOOL fits_above = (y_above >= work.top) && (y_above + height <= work.bottom);

    int y;
    if (fits_below)
        y = y_below;
    else if (fits_above)
        y = y_above;
    else {
        /* Too tall for either side. Take the roomier one and give up the rows
         * that do not fit rather than any part of the icon: a clamped y would
         * slide the box back over the thing the reader is pointing at. */
        int room_below = work.bottom - s_anchor.bottom;
        int room_above = s_anchor.top - work.top;
        if (room_below >= room_above) {
            y = s_anchor.bottom;
            height = (room_below > 0) ? room_below : 0;
        } else {
            height = (room_above > 0) ? room_above : 0;
            y = s_anchor.top - height;
        }
    }

    SetWindowPos(s_wnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(s_wnd, NULL, TRUE);
}

/* Fill the rows for whichever list is up.
 *
 * Three sources, one row model: a string per row, a count, and a selection.
 * Everything past this function - the placement, the painting, the keys, the
 * clicking - is written against that and does not know which list it has. */
static void tabbox_pull(void)
{
    if (s_mode == HG_BOX_TABS) {
        if (!s_target)
            return;
        HgTabsAnswer answer;
        int fresh = hg_tabs_take_result(s_target, s_titles, HG_TABS_MAX_PER_WINDOW, &answer);
        if (fresh >= 0 && !answer.failed)
            s_count = fresh;
    } else if (s_mode == HG_BOX_DIRS) {
        s_count = 0;
        for (int i = 0; i < hg_g_folder_count && s_count < HG_BOX_MAX_ROWS; ++i) {
            StringCchCopyW(s_titles[s_count], HG_MAX_STR, hg_g_folders[i].name);
            ++s_count;
        }
    } else if (s_mode == HG_BOX_MENU) {
        /* Built fresh: the audio devices, the displays and every tick in the
         * menu are a picture of the machine right now, and a list assembled
         * once would be a picture of the machine when the box first opened. */
        s_count = 0;
        s_menu_row_count = 0;
        HMENU menu = taskbox_create_main_popup_menu();
        if (menu) {
            s_menu_row_count = hg_menu_flatten(menu, s_menu_rows, HG_BOX_MAX_ROWS);
            DestroyMenu(menu);
        }
        for (int i = 0; i < s_menu_row_count; ++i) {
            /* The tick keeps its column so the ticked rows line up with the
             * ones that are not, rather than shifting two characters left. */
            hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%ls%ls",
                               s_menu_rows[i].checked ? L"\u2713 " : L"   ", s_menu_rows[i].label);
            ++s_count;
        }
    } else {
        s_count = 0;
        tabbox_build_control_rows();
        s_count = 0;
        for (int i = 0; i < s_control_row_count && s_count < HG_BOX_MAX_ROWS; ++i) {
            const HgControlRow *row = &s_control_rows[i];

            if (row->kind == HG_ROW_OPTION) {
                HgOptionInfo info;
                if (!hg_option_info(row->id, &info))
                    continue;
                const WCHAR *state = !info.available
                                         ? (info.unavailable_note ? info.unavailable_note : L"unavailable")
                                         : (hg_option_get(row->id) ? L"on" : L"off");
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-30ls %ls", info.label, state);
                ++s_count;
                continue;
            }

            /* Name, then the reading: the reading is the reason to open the box
             * at all, and the row is what the wheel is spinning. */
            int pct = 0;
            if (hg_toolbar_value_percent(row->id, &pct)) {
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-30ls %3d%%%ls", row->label, pct,
                                   (row->id == HG_TOOL_ICON_VOLUME && get_system_mute()) ? L"  (muted)" : L"");
            } else if (row->id == HG_TOOL_ICON_PIN) {
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-30ls %ls", row->label,
                                   hg_g_taskbox_pinned ? L"on" : L"off");
            } else {
                StringCchCopyW(s_titles[s_count], HG_MAX_STR, row->label);
            }
            ++s_count;
        }
    }

    if (s_selected >= s_count)
        s_selected = (s_count > 0) ? s_count - 1 : 0;
}

/* What a control row answers to, asked in one place.
 *
 * A row with a number is turned; a row with a state is switched. Every key this
 * box gives a control row follows from that distinction, and so does what the
 * tooltip tells the reader - which matters, because "the arrows change this
 * one and not that one" is not a rule anyone guesses from looking. */
static BOOL tabbox_row_is_value(int index)
{
    if (s_mode != HG_BOX_CONTROLS || index < 0 || index >= s_control_row_count)
        return FALSE;
    const HgControlRow *row = &s_control_rows[index];
    if (row->kind != HG_ROW_BUTTON)
        return FALSE;
    int pct = 0;
    return hg_toolbar_value_percent(row->id, &pct);
}

/* The line the tooltip adds: what this row does when a key arrives. Shown for
 * every control row, fitting or not, because it is the answer to a question the
 * row cannot answer by being read. */
static const WCHAR *tabbox_row_hint(int index)
{
    if (s_mode == HG_BOX_MENU) {
        if (index < 0 || index >= s_menu_row_count)
            return NULL;
        if (!s_menu_rows[index].id || !s_menu_rows[index].enabled)
            return L"This one is not available right now.";
        return L"Space or Enter: run it";
    }

    if (s_mode != HG_BOX_CONTROLS || index < 0 || index >= s_control_row_count)
        return NULL;

    const HgControlRow *row = &s_control_rows[index];
    if (row->kind == HG_ROW_OPTION) {
        HgOptionInfo info;
        if (hg_option_info(row->id, &info) && !info.available)
            return L"Off in this build - nothing to switch.";
        return L"Space or Enter: switch it on or off";
    }

    if (tabbox_row_is_value(index))
        return L"Left / Right: less / more   (the wheel does the same)";
    return L"Space or Enter: switch it on or off";
}

/* Left and right on a row with a number. Same call as the wheel, so one step of
 * the arrows and one notch of the wheel move a value by the same amount and
 * there is only one place that decides how big a step is. */
static void tabbox_tip_show(int index);

static BOOL tabbox_adjust_row(int index, int direction)
{
    if (!tabbox_row_is_value(index))
        return FALSE;
    if (!hg_toolbar_value_wheel(s_control_rows[index].id, (short)(direction * WHEEL_DELTA)))
        return FALSE;

    s_selected = index;
    tabbox_pull();
    if (s_wnd)
        InvalidateRect(s_wnd, NULL, TRUE);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    tabbox_tip_show(index); /* the reading just changed under it */
    return TRUE;
}

/* The full text of a row, as it is drawn: the label and the title. */
static void tabbox_row_text(int index, WCHAR *out, size_t out_cch)
{
    if (index < 0 || index >= s_count) {
        StringCchCopyW(out, out_cch, L"");
        return;
    }
    WCHAR label = tabbox_label_for(index);
    if (label)
        hellgates_wsprintf(out, out_cch, L"%lc: %ls", label, s_titles[index]);
    else
        StringCchCopyW(out, out_cch, s_titles[index]);
}

/* TRUE when that row is drawn shorter than it is. */
static BOOL tabbox_row_is_clipped(int index, WCHAR *text, size_t text_cch)
{
    if (!s_wnd || index < 0 || index >= s_count)
        return FALSE;

    tabbox_row_text(index, text, text_cch);

    HDC dc = GetDC(s_wnd);
    if (!dc)
        return FALSE;
    HFONT font = hg_g_main_font ? hg_g_main_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT old = (HFONT)SelectObject(dc, font);
    SIZE sz = {0, 0};
    GetTextExtentPoint32W(dc, text, lstrlenW(text), &sz);
    SelectObject(dc, old);
    ReleaseDC(s_wnd, dc);

    RECT rc;
    GetClientRect(s_wnd, &rc);
    double ws = hg_window_scale(s_wnd);
    int room = rc.right - SCW(ws, 6) * 2 - SCW(ws, 20); /* the padding, and the marker column */
    return sz.cx > room;
}

/* The tooltip that says what a clipped row says. */
static void tabbox_tip_hide(void)
{
    if (!s_tip)
        return;
    TOOLINFOW ti;
    SecureZeroMemory(&ti, sizeof(ti));
    ti.cbSize = TOOLINFO_V1_SIZE;
    ti.hwnd = s_wnd;
    ti.uId = 1;
    SendMessageW(s_tip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
    s_tip_row = -1;
}

static void tabbox_tip_show(int index)
{
    if (!s_wnd || index < 0 || index >= s_count) {
        tabbox_tip_hide();
        return;
    }

    /* Two reasons for a tip, and they compose: a row too long to be read whole,
     * and a row whose keys are worth saying out loud. A control row gets the
     * second whether or not it needs the first. */
    WCHAR full[HG_MAX_STR + 8];
    BOOL clipped = tabbox_row_is_clipped(index, full, HG_ARRAYSIZE(full));
    const WCHAR *hint = tabbox_row_hint(index);
    if (!clipped && !hint) {
        /* A row that fits and does nothing surprising says everything it has to
         * say already. */
        tabbox_tip_hide();
        return;
    }

    WCHAR text[HG_MAX_STR + 128];
    if (clipped && hint)
        hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%ls\r\n%ls", full, hint);
    else if (clipped)
        StringCchCopyW(text, HG_ARRAYSIZE(text), full);
    else
        StringCchCopyW(text, HG_ARRAYSIZE(text), hint);

    if (!s_tip) {
        s_tip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                0, 0, 0, 0, s_wnd, NULL, GetModuleHandleW(NULL), NULL);
        if (!s_tip)
            return;

        TOOLINFOW ti;
        SecureZeroMemory(&ti, sizeof(ti));
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
        ti.hwnd = s_wnd;
        ti.uId = 1;
        ti.hinst = GetModuleHandleW(NULL);
        ti.lpszText = L"";
        SendMessageW(s_tip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        /* Wide enough to wrap rather than run off the display. */
        SendMessageW(s_tip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)SCW(hg_window_scale(s_wnd), 700));
    }

    TOOLINFOW ti;
    SecureZeroMemory(&ti, sizeof(ti));
    ti.cbSize = TOOLINFO_V1_SIZE;
    ti.hwnd = s_wnd;
    ti.uId = 1;
    ti.lpszText = text;
    SendMessageW(s_tip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);

    /* Beside the row it belongs to, not under the pointer: the keyboard has no
     * pointer, and a tip that jumps to the mouse while the arrows are moving
     * would be pointing at the wrong row. */
    double ws = hg_window_scale(s_wnd);
    int pad = SCW(ws, 6);
    int row_h = tabbox_row_height();
    POINT pt = {pad + SCW(ws, 12), pad + index * row_h + row_h};
    ClientToScreen(s_wnd, &pt);
    SendMessageW(s_tip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x, pt.y));
    SendMessageW(s_tip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
    s_tip_row = index;
}

/* The window, made once and reused by all three lists. */
static BOOL tabbox_ensure_window(void)
{
    if (s_wnd)
        return TRUE;
    s_wnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, HG_CLASS_TABBOX, L"Tabs",
                            WS_POPUP | WS_BORDER, 0, 0, 0, 0, hg_g_taskbox_wnd, NULL, GetModuleHandle(NULL),
                            NULL);
    return s_wnd != NULL;
}

void hg_tabbox_open(HWND target, const RECT *anchor_screen_rc)
{
    /* Switched off means switched off wherever the ask came from - the mouse in
     * the toolbar, the keyboard focus in the grid - so the gate is here rather
     * than at each call. */
    if (!hg_g_tabbox_on_hover) {
        hg_tabbox_close();
        return;
    }

    if (!target || !IsWindow(target) || !anchor_screen_rc)
        return;
    if (!hg_tabs_enabled() || !hg_tabs_window_may_have_tabs(target))
        return;

    if (!tabbox_ensure_window())
        return;

    if (s_mode != HG_BOX_TABS) {
        s_mode = HG_BOX_TABS;
        s_target = NULL;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE;
    }
    s_open = TRUE;

    if (s_target != target) {
        s_target = target;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE; /* a new box is never entered until Tab says so */
    }
    s_anchor = *anchor_screen_rc;

    /* Draw what is known now, ask for the rest. This one request is the only
     * enumeration this feature performs - there is no cadence behind it. */
    tabbox_pull();
    hg_tabs_request(&target, 1);
    tabbox_layout();
}

/* The two lists that belong to a button rather than to a window. Neither asks
 * anything of the tab reader, and neither is gated by the tabs option: that
 * switch is about hovering a task icon, which is not what these are. */
static void tabbox_open_list(int mode, const RECT *anchor_screen_rc)
{
    if (!anchor_screen_rc || !tabbox_ensure_window())
        return;

    if (s_mode != mode) {
        s_mode = mode;
        s_target = NULL;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE;
    }
    s_open = TRUE;
    s_anchor = *anchor_screen_rc;
    tabbox_pull();
    tabbox_layout();
}

void hg_tabbox_open_dirs(const RECT *anchor_screen_rc)
{
    tabbox_open_list(HG_BOX_DIRS, anchor_screen_rc);
}

void hg_tabbox_open_controls(const RECT *anchor_screen_rc)
{
    tabbox_open_list(HG_BOX_CONTROLS, anchor_screen_rc);
}

void hg_tabbox_open_menu(const RECT *anchor_screen_rc)
{
    tabbox_open_list(HG_BOX_MENU, anchor_screen_rc);
}

void hg_tabbox_enter(void)
{
    if (!hg_tabbox_is_open())
        return;
    s_focused = TRUE;
    if (s_count > 0 && (s_selected < 0 || s_selected >= s_count))
        s_selected = 0;
    InvalidateRect(s_wnd, NULL, TRUE);
    tabbox_tip_show(s_selected);
}

int hg_tabbox_mode(void)
{
    return hg_tabbox_is_open() ? s_mode : HG_BOX_TABS;
}

void hg_tabbox_close(void)
{
    if (!s_wnd)
        return;
    tabbox_tip_hide();
    ShowWindow(s_wnd, SW_HIDE);
    s_target = NULL;
    s_count = 0;
    s_selected = 0;
    s_focused = FALSE;
    s_open = FALSE;
    s_mode = HG_BOX_TABS;
}

BOOL hg_tabbox_is_open(void)
{
    return s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd) && s_open;
}

HWND hg_tabbox_target(void)
{
    return (hg_tabbox_is_open() && s_mode == HG_BOX_TABS) ? s_target : NULL;
}

HWND hg_tabbox_window(void)
{
    return s_wnd;
}

void hg_tabbox_refresh(void)
{
    if (!hg_tabbox_is_open() || s_mode != HG_BOX_TABS)
        return;
    int before = s_count;
    tabbox_pull();
    if (s_count != before)
        tabbox_layout();
    else
        InvalidateRect(s_wnd, NULL, TRUE);
}

/* What a row does when it is picked.
 *
 * A tab and a folder are both destinations, so the box and the taskbox both get
 * out of the way - staying open over the thing you just went to is clutter. A
 * control is not a destination: the box stays, because the reason to open it is
 * usually to move a value and then look at what moved. */
static void tabbox_activate(int index)
{
    if (index < 0 || index >= s_count)
        return;

    if (s_mode == HG_BOX_TABS) {
        HWND target = s_target;
        if (!target)
            return;
        hg_tabbox_close();
        hide_taskbox(hg_g_taskbox_wnd);
        hg_tabs_activate(target, index);
        return;
    }

    if (s_mode == HG_BOX_DIRS) {
        if (index >= hg_g_folder_count)
            return;
        WCHAR path[HG_MAX_PATH];
        StringCchCopyW(path, HG_ARRAYSIZE(path), hg_g_folders[index].target[0] ? hg_g_folders[index].target
                                                                              : hg_g_folders[index].path);
        hg_tabbox_close();
        hide_taskbox(hg_g_taskbox_wnd);
        ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
        return;
    }

    if (s_mode == HG_BOX_MENU) {
        if (index >= s_menu_row_count)
            return;
        const HgMenuRow *row = &s_menu_rows[index];
        /* A row the menu itself greys out stays dead here. Saying so by doing
         * nothing is what the menu does, and a box that ran a disabled command
         * would be a different program wearing the same list. */
        if (!row->id || !row->enabled)
            return;

        UINT cmd = row->id;
        hg_tabbox_close();
        taskbox_dispatch_main_menu_command(cmd);
        return;
    }

    if (index >= s_control_row_count)
        return;
    const HgControlRow *row = &s_control_rows[index];

    if (row->kind == HG_ROW_OPTION) {
        const WCHAR *message = NULL;
        hg_option_set(row->id, hg_option_get(row->id) ? FALSE : TRUE, &message);
        if (message)
            append_message(message);
        tabbox_pull();
        if (s_wnd)
            InvalidateRect(s_wnd, NULL, TRUE);
        return;
    }

    /* The same call the buttons made when they were on the row. Mute and the
     * pin did not change by moving in here, and writing them out a second time
     * is how two copies of one behaviour start to differ. */
    activate_toolbar_item(row->id);

    tabbox_pull();
    if (s_wnd)
        InvalidateRect(s_wnd, NULL, TRUE);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
}

BOOL hg_tabbox_pointer_over(void)
{
    if (!hg_tabbox_is_open())
        return FALSE;
    POINT pt;
    RECT box;
    if (!GetCursorPos(&pt) || !GetWindowRect(s_wnd, &box))
        return FALSE;
    return PtInRect(&box, pt);
}

BOOL hg_tabbox_handle_wheel(short delta)
{
    if (!hg_tabbox_is_open() || s_mode != HG_BOX_CONTROLS || !hg_tabbox_pointer_over())
        return FALSE;

    POINT pt;
    if (!GetCursorPos(&pt))
        return FALSE;
    ScreenToClient(s_wnd, &pt);

    double ws = hg_window_scale(s_wnd);
    int pad = SCW(ws, 6);
    int row_h = tabbox_row_height();
    int row = (row_h > 0 && pt.y >= pad) ? (pt.y - pad) / row_h : -1;
    if (row < 0 || row >= s_count || row >= s_control_row_count)
        return FALSE;
    if (s_control_rows[row].kind != HG_ROW_BUTTON)
        return FALSE;

    if (!hg_toolbar_value_wheel(s_control_rows[row].id, delta))
        return FALSE;

    s_selected = row;
    tabbox_pull();
    InvalidateRect(s_wnd, NULL, TRUE);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    return TRUE;
}

BOOL hg_tabbox_handle_key(WPARAM key)
{
    if (!hg_tabbox_is_open())
        return FALSE;

    if (!s_focused) {
        /* Open, not entered: the digits, Tab, and the two keys that point at
         * the box.
         *
         * Up and Down step into it rather than to the icon above or below,
         * because the box *is* what is above or below: it is hanging off this
         * icon, filling that space, and stepping over it to the next row would
         * mean walking past the thing the reader just opened. Left and Right
         * still walk the grid, so nothing is unreachable - and Esc comes back
         * out to the icon. */
        if (key == VK_TAB || key == VK_UP || key == VK_DOWN) {
            s_focused = TRUE;
            if (s_count > 0) {
                /* Down enters at the top and Up at the bottom: the selection
                 * lands where the key was already pointing. */
                s_selected = (key == VK_UP) ? s_count - 1 : 0;
            }
            InvalidateRect(s_wnd, NULL, TRUE);
            tabbox_tip_show(s_selected);
            return TRUE;
        }
        if (key >= L'0' && key <= L'9') {
            int index = tabbox_index_for_key(key);
            if (index >= 0 && index < s_count) {
                tabbox_activate(index);
                return TRUE;
            }
        }
        return FALSE;
    }

    switch (key) {
    case VK_TAB:
        return TRUE; /* already in */
    case VK_ESCAPE:
        /* Out of the box, back to what it belongs to. The keyboard is already
         * the taskbox's, which is where the reader was before entering. */
        hg_tabbox_close();
        return TRUE;
    case VK_LEFT:
    case VK_RIGHT:
        /* On a row that holds a number, left and right are less and more. It is
         * the one place in this box where they are not navigation, and it is
         * worth the exception: a value the wheel can turn but the keyboard
         * cannot is a value only half the readers can reach. The tooltip on
         * such a row says so, because nothing about the row itself would.
         *
         * Everywhere else they are what they are in the rest of the taskbox:
         * the icon to the left, the icon to the right. A list is a column and
         * has nothing of its own to do with them, so the box closes and the key
         * goes on to the grid - which moves the focus, and the next icon opens
         * its own box if it has one. */
        if (tabbox_adjust_row(s_selected, (key == VK_RIGHT) ? 1 : -1))
            return TRUE;
        hg_tabbox_close();
        return FALSE;
    case VK_UP:
        if (s_selected > 0)
            --s_selected;
        InvalidateRect(s_wnd, NULL, TRUE);
        tabbox_tip_show(s_selected);
        return TRUE;
    case VK_DOWN:
        if (s_selected + 1 < s_count)
            ++s_selected;
        InvalidateRect(s_wnd, NULL, TRUE);
        tabbox_tip_show(s_selected);
        return TRUE;
    case VK_HOME:
        s_selected = 0;
        InvalidateRect(s_wnd, NULL, TRUE);
        tabbox_tip_show(s_selected);
        return TRUE;
    case VK_END:
        s_selected = (s_count > 0) ? s_count - 1 : 0;
        InvalidateRect(s_wnd, NULL, TRUE);
        tabbox_tip_show(s_selected);
        return TRUE;
    case VK_RETURN:
    case VK_SPACE:
        tabbox_activate(s_selected);
        return TRUE;
    default:
        break;
    }

    int index = tabbox_index_for_key(key);
    if (index >= 0 && index < s_count) {
        /* A label key goes straight there - the point of labelling rows. */
        tabbox_activate(index);
        return TRUE;
    }
    return FALSE;
}

void hg_tabbox_pointer_moved(void)
{
    if (!hg_tabbox_is_open())
        return;

    POINT pt;
    if (!GetCursorPos(&pt))
        return;

    RECT box;
    GetWindowRect(s_wnd, &box);
    if (PtInRect(&box, pt) || PtInRect(&s_anchor, pt))
        return;

    hg_tabbox_close();
}

static int tabbox_row_at(POINT client_pt)
{
    double ws = hg_window_scale(s_wnd);
    int pad = SCW(ws, 6);
    int row_h = tabbox_row_height();
    if (row_h <= 0 || client_pt.y < pad)
        return -1;
    int index = (client_pt.y - pad) / row_h;
    return (index >= 0 && index < s_count) ? index : -1;
}

LRESULT CALLBACK tabbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_MOUSEACTIVATE:
        /* Clicking a row must not move the focus away from the taskbox. */
        return MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HgPaintBuffer buffer;
        HDC dc = hdc;
        BOOL buffered = hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &buffer);
        if (buffered)
            dc = buffer.dc;

        HBRUSH bg = CreateSolidBrush(hg_g_color_scheme_selected.bg);
        if (bg) {
            FillRect(dc, &rc, bg);
            DeleteObject(bg);
        }

        HFONT font = hg_g_main_font ? hg_g_main_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT old_font = (HFONT)SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);

        double ws = hg_window_scale(hwnd);
        int pad = SCW(ws, 6);
        int row_h = tabbox_row_height();

        if (s_count <= 0) {
            SetTextColor(dc, hg_g_color_scheme_selected.text);
            RECT row = {pad, pad, rc.right - pad, pad + row_h};
            const WCHAR *empty = (s_mode == HG_BOX_TABS)
                                     ? L"(reading tabs...)"
                                     : (s_mode == HG_BOX_DIRS)
                                           ? L"(no folder shortcuts - put one in the shortcuts folder)"
                                           : (s_mode == HG_BOX_MENU) ? L"(the menu is empty)"
                                                                     : L"(nothing to show)";
            DrawTextW(dc, empty, -1, &row, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }

        /* An entered box says so with a frame, because the keys it answers
         * change the moment it is entered. */
        if (s_focused) {
            HBRUSH frame = CreateSolidBrush(hg_g_has_system_accent_color ? hg_g_system_accent_color
                                                                        : GetSysColor(COLOR_HIGHLIGHT));
            if (frame) {
                RECT edge = rc;
                for (int i = 0; i < SCW(ws, 2); ++i) {
                    FrameRect(dc, &edge, frame);
                    InflateRect(&edge, -1, -1);
                }
                DeleteObject(frame);
            }
        }

        for (int i = 0; i < s_count; ++i) {
            RECT row = {pad, pad + i * row_h, rc.right - pad, pad + (i + 1) * row_h};
            BOOL selected = (i == s_selected && s_focused);
            if (selected) {
                /* Filled, and the text inverted over it. A row the arrows are
                 * standing on has to be findable at a glance, or the arrows are
                 * moving something the reader cannot see. */
                HBRUSH sel = CreateSolidBrush(hg_g_has_system_accent_color ? hg_g_system_accent_color
                                                                           : GetSysColor(COLOR_HIGHLIGHT));
                if (sel) {
                    FillRect(dc, &row, sel);
                    DeleteObject(sel);
                }
            }
            SetTextColor(dc, selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : hg_g_color_scheme_selected.text);

            WCHAR line[HG_MAX_STR + 8];
            WCHAR label = tabbox_label_for(i);
            if (label)
                hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"%ls%lc: %ls", selected ? L"\u25b8 " : L"  ", label,
                                   s_titles[i]);
            else
                hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"%ls%ls", selected ? L"\u25b8 " : L"  ",
                                   s_titles[i]);

            RECT text_rc = row;
            text_rc.left += SCW(ws, 4);
            DrawTextW(dc, line, -1, &text_rc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        }

        SelectObject(dc, old_font);
        if (buffered) {
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, buffer.dc, 0, 0, SRCCOPY);
            hg_paint_buffer_end(&buffer);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0 && row != s_selected) {
            s_selected = row;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (row != s_tip_row)
            tabbox_tip_show(row);

        /* So the tip goes when the pointer does, rather than hanging over a box
         * the pointer has left. */
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        tabbox_tip_hide();
        return 0;

    case WM_LBUTTONUP: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0)
            tabbox_activate(row);
        return 0;
    }

    case WM_RBUTTONUP: {
        /* Right-click closes that tab, and the box stays: closing several in a
         * row is the reason anyone right-clicks a tab list. */
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0 && s_target && s_mode == HG_BOX_TABS) {
            hg_tabs_close(s_target, row);
            hg_tabs_request(&s_target, 1); /* the list just changed */
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
        /* Reached when the pointer is over the box and something gave it the
         * message; the toolbar forwards the ones that arrive there instead. */
        if (hg_tabbox_handle_wheel((short)HIWORD(w_param)))
            return 0;
        break;

    case WM_DESTROY:
        if (s_wnd == hwnd) {
            s_wnd = NULL;
            s_target = NULL;
            s_count = 0;
            s_open = FALSE;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
