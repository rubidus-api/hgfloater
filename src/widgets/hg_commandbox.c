#include "hg_commandbox.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "../hg_command.h"

void commandbox_execute(void);

static int commandbox_line_height(HWND hwnd, double scale)
{
    int line_h = SCW(scale, 16);
    if (!hg_g_commandbox_font)
        return line_h;

    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return line_h;

    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_commandbox_font);
    TEXTMETRIC tm = {0};
    if (GetTextMetrics(hdc, &tm)) {
        line_h = tm.tmHeight + tm.tmExternalLeading;
    }
    if (old_font)
        SelectObject(hdc, old_font);
    ReleaseDC(hwnd, hdc);
    return line_h;
}

/* ------------------------------------------------------------------- modes
 *
 * Scrolling the transcript and walking the history both want the arrow keys,
 * and so does the caret in a multi-line input box. Modifier combinations were
 * the first answer and the wrong one: they took Shift+arrow away from text
 * selection, which is a key every other text box on the machine has.
 *
 * So the arrows stay the caret's, and a mode is something you enter on purpose.
 * Ctrl+S for scrolling, Ctrl+H for the history, and while one is on the arrows
 * mean that instead. The caption says which, because a window that has silently
 * changed what its keys do is a window that looks broken. */
typedef enum HgCommandBoxMode {
    HG_CB_MODE_NONE = 0,
    HG_CB_MODE_SCROLL,
    HG_CB_MODE_HISTORY
} HgCommandBoxMode;

static HgCommandBoxMode s_cb_mode = HG_CB_MODE_NONE;

static void commandbox_set_mode(HgCommandBoxMode mode)
{
    if (s_cb_mode == mode)
        return;
    s_cb_mode = mode;

    if (!hg_g_commandbox_wnd || !IsWindow(hg_g_commandbox_wnd))
        return;
    switch (mode) {
    case HG_CB_MODE_SCROLL:
        SetWindowTextW(hg_g_commandbox_wnd, L"Command Box - SCROLL: up/down a line, left/right a page, Esc to leave");
        break;
    case HG_CB_MODE_HISTORY:
        SetWindowTextW(hg_g_commandbox_wnd, L"Command Box - HISTORY: up/down through it, Enter to run, Esc to leave");
        break;
    default:
        SetWindowTextW(hg_g_commandbox_wnd, L"Command Box");
        break;
    }
}

static void commandbox_scroll(int bar)
{
    if (hg_g_commandbox_out_wnd && IsWindow(hg_g_commandbox_out_wnd))
        SendMessageW(hg_g_commandbox_out_wnd, WM_VSCROLL, (WPARAM)bar, 0);
}

static BOOL commandbox_recall(int direction)
{
    const WCHAR *line = hg_command_history_step(direction);
    if (!line)
        return FALSE;
    SetWindowTextW(hg_g_commandbox_in_wnd, line);
    int len = GetWindowTextLengthW(hg_g_commandbox_in_wnd);
    SendMessageW(hg_g_commandbox_in_wnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    return TRUE;
}

/* TRUE when the key was the mode's. Anything the mode has no use for drops the
 * mode and is then handled normally, so typing never lands in a hole. */
static BOOL commandbox_mode_key(WPARAM key)
{
    if (s_cb_mode == HG_CB_MODE_SCROLL) {
        switch (key) {
        case VK_UP: commandbox_scroll(SB_LINEUP); return TRUE;
        case VK_DOWN: commandbox_scroll(SB_LINEDOWN); return TRUE;
        case VK_LEFT: commandbox_scroll(SB_PAGEUP); return TRUE;
        case VK_RIGHT: commandbox_scroll(SB_PAGEDOWN); return TRUE;
        default: break;
        }
        commandbox_set_mode(HG_CB_MODE_NONE);
        return FALSE;
    }

    if (s_cb_mode == HG_CB_MODE_HISTORY) {
        switch (key) {
        case VK_UP: commandbox_recall(1); return TRUE;   /* older */
        case VK_DOWN: commandbox_recall(-1); return TRUE; /* newer */
        default: break;
        }
        /* Enter runs what the history put in the box, so leaving the mode and
         * letting it through is exactly right. */
        commandbox_set_mode(HG_CB_MODE_NONE);
        return FALSE;
    }
    return FALSE;
}

/* Esc leaves a mode if one is on, and otherwise puts the box away and hands the
 * keyboard to the taskbox - which is where the reader was before they opened
 * this, and the only other place the keys mean anything. */
static void commandbox_escape(HWND hwnd)
{
    if (s_cb_mode != HG_CB_MODE_NONE) {
        commandbox_set_mode(HG_CB_MODE_NONE);
        return;
    }

    ShowWindow(hwnd, SW_HIDE);

    if (!hg_g_taskbox_wnd || !IsWindow(hg_g_taskbox_wnd))
        return;

    if (!IsWindowVisible(hg_g_taskbox_wnd)) {
        /* The same expansion the floater does, so the taskbox lands where it
         * would have if it had been opened from there. */
        hg_expand_taskbox_from_floater(hg_g_floater_wnd, hg_g_taskbox_wnd);
        refresh_window_list(FALSE);
        ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
        if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd))
            ShowWindow(hg_g_floater_wnd, SW_HIDE);
    }
    hg_force_foreground(hg_g_taskbox_wnd);
    if (hg_g_toolbar_wnd)
        SetFocus(hg_g_toolbar_wnd);
}

LRESULT CALLBACK commandbox_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                               UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;
    HWND parent = GetParent(hwnd);
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
        BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);
        BOOL is_shift = (GetKeyState(VK_SHIFT) < 0);

        if (is_ctrl && !is_alt && (w_param == 'S' || w_param == 'H')) {
            commandbox_set_mode((w_param == 'S') ? HG_CB_MODE_SCROLL : HG_CB_MODE_HISTORY);
            return 0;
        }

        if (s_cb_mode != HG_CB_MODE_NONE && !is_ctrl && !is_alt) {
            if (w_param == VK_ESCAPE) {
                commandbox_set_mode(HG_CB_MODE_NONE);
                return 0;
            }
            if (commandbox_mode_key(w_param))
                return 0;
        }

        /* Enter runs it; Shift+Enter is the newline, which is what makes a
         * multi-line box worth having. */
        if (w_param == VK_RETURN && !is_shift && !is_alt && !is_ctrl) {
            commandbox_execute();
            return 0;
        }

        if (w_param == VK_SPACE && is_ctrl) {
            SetFocus(hg_g_commandbox_in_wnd);
            int len = GetWindowTextLengthW(hg_g_commandbox_in_wnd);
            SendMessageW(hg_g_commandbox_in_wnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            return 0;
        }
        if ((is_ctrl || is_alt) && (w_param == VK_LEFT || w_param == VK_RIGHT || w_param == VK_UP || w_param == VK_DOWN)) {
            SendMessageW(parent, msg, w_param, l_param);
            return 0;
        }
        if (w_param == VK_ESCAPE) {
            commandbox_escape(parent);
            return 0;
        }
    }
    else if (msg == WM_MOUSEWHEEL) {
        /* Ctrl resizes the text and Alt changes the opacity, both the window's
         * business. A plain wheel is the control's, and forwarding it too left
         * the transcript unable to scroll. */
        if ((GET_KEYSTATE_WPARAM(w_param) & MK_CONTROL) || (GetKeyState(VK_MENU) < 0)) {
            SendMessageW(parent, msg, w_param, l_param);
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}


void load_commandbox_font()
{
    load_commandbox_font_config();

    release_font_handle(&hg_g_commandbox_font, FALSE);

    hg_g_commandbox_font = CreateFontW(
        hg_g_commandbox_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, hg_g_commandbox_font_name
    );
}

void commandbox_print(const WCHAR *text)
{
    if (!hg_g_commandbox_out_wnd || !IsWindow(hg_g_commandbox_out_wnd) || !text)
        return;

    int out_len = GetWindowTextLengthW(hg_g_commandbox_out_wnd);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SETSEL, (WPARAM)out_len, (LPARAM)out_len);
    if (out_len > 0) {
        SendMessageW(hg_g_commandbox_out_wnd, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    }
    SendMessageW(hg_g_commandbox_out_wnd, EM_REPLACESEL, FALSE, (LPARAM)text);

    int new_out_len = GetWindowTextLengthW(hg_g_commandbox_out_wnd);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SETSEL, (WPARAM)new_out_len, (LPARAM)new_out_len);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SCROLLCARET, 0, 0);
}

void commandbox_execute()
{
    if (!hg_g_commandbox_in_wnd || !hg_g_commandbox_out_wnd)
        return;

    int in_len = GetWindowTextLengthW(hg_g_commandbox_in_wnd);
    if (in_len <= 0)
        return;

    size_t in_len_cch = (size_t)in_len;
    WCHAR *in_buf = (WCHAR *)malloc(sizeof(WCHAR) * (in_len_cch + 1u));
    if (!in_buf)
        return;
    GetWindowTextW(hg_g_commandbox_in_wnd, in_buf, in_len + 1);

    /* The input field is multi-line, so a paste can carry several commands: each
     * line is echoed behind a prompt and then run, in the order given. */
    BOOL moved_focus = FALSE;
    WCHAR *cursor = in_buf;
    while (*cursor) {
        WCHAR *end = cursor;
        while (*end && *end != L'\r' && *end != L'\n')
            ++end;
        WCHAR saved = *end;
        *end = L'\0';

        if (*cursor) {
            WCHAR echo[HG_MAX_STR];
            StringCchPrintfW(echo, HG_ARRAYSIZE(echo), L"> %ls", cursor);
            commandbox_print(echo);
            /* Recorded before it runs, so a command that fails is still
             * something you can bring back and correct. */
            hg_command_history_add(cursor);
            if (hg_command_execute(cursor))
                moved_focus = TRUE;
        }

        if (saved == L'\0')
            break;
        *end = saved;
        while (*end == L'\r' || *end == L'\n')
            ++end;
        cursor = end;
    }

    SetWindowTextW(hg_g_commandbox_in_wnd, L"");
    hg_command_history_reset();
    /* Taking the keyboard back is right after a command that only printed, and
     * wrong after one that opened a window for the reader to work in. */
    if (!moved_focus)
        SetFocus(hg_g_commandbox_in_wnd);
    free(in_buf);
}

/* The window exists to be typed into, so the keyboard goes to the input box
 * rather than to the frame: opening it and having the first keystroke land
 * nowhere is the whole complaint this answers. */
void commandbox_focus_input(void)
{
    if (!hg_g_commandbox_in_wnd || !IsWindow(hg_g_commandbox_in_wnd))
        return;
    SetFocus(hg_g_commandbox_in_wnd);
    int len = GetWindowTextLengthW(hg_g_commandbox_in_wnd);
    SendMessageW(hg_g_commandbox_in_wnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
}

/* Recover a position saved off-screen (for example from an old monitor
 * layout), record where the window actually is in the taskbox log, and take
 * focus. Shared by the first-show and re-show paths. */
static void commandbox_after_show(void)
{
    ensure_window_visible(hg_g_commandbox_wnd, L"commandbox");
    RECT rc;
    if (GetWindowRect(hg_g_commandbox_wnd, &rc)) {
        WCHAR msg[96];
        hellgates_wsprintf(msg, HG_ARRAYSIZE(msg), L"CommandBox: shown at (%d,%d) %dx%d", rc.left, rc.top,
                           rc.right - rc.left, rc.bottom - rc.top);
        append_message(msg);
    }
    hg_force_foreground(hg_g_commandbox_wnd);
    commandbox_focus_input();
}

void show_commandbox_window()
{
    if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd)) {
        if (IsWindowVisible(hg_g_commandbox_wnd)) {
            ShowWindow(hg_g_commandbox_wnd, SW_HIDE);
        } else {
            ShowWindow(hg_g_commandbox_wnd, SW_SHOWNORMAL);
            commandbox_after_show();
        }
        return;
    }

    int x, y, w, h;
    load_config(L"commandbox", &x, &y, &w, &h, 100, 100, SC(400), SC(300));

    /* Scale for the monitor the box will appear on, not the floater's. */
    POINT cb_origin = {x, y};
    double ws = hg_point_scale(cb_origin);
    if (w < SCW(ws, 200)) w = SCW(ws, 200);

    int border = SCW(ws, 8);
    int btn_h = SCW(ws, 26);
    load_commandbox_font();
    int line_h = commandbox_line_height(NULL, ws);
    int min_out_h = line_h * 3 + SCW(ws, 6);
    int min_in_h = line_h * 1 + SCW(ws, 6);
    int min_cy = border * 4 + min_out_h + min_in_h + btn_h;
    if (h < min_cy) {
        h = min_cy;
    }

    RECT rc_win = {0, 0, w, h};
    AdjustWindowRectEx(&rc_win, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, FALSE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
    int win_w = rc_win.right - rc_win.left;
    int win_h = rc_win.bottom - rc_win.top;
    int required_win_h = rc_win.bottom - rc_win.top;
    if (win_h < required_win_h) {
        win_h = required_win_h;
    }

    hg_g_commandbox_wnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST, HG_CLASS_COMMANDBOX, L"Command Box",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
        x, y, win_w, win_h, hg_g_taskbox_wnd, NULL, GetModuleHandle(NULL), NULL
    );

    if (hg_g_commandbox_wnd) {
        hg_apply_class_background(hg_g_commandbox_wnd);
        SetLayeredWindowAttributes(hg_g_commandbox_wnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
        
        hg_g_commandbox_out_wnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", NULL,
            /* No ES_AUTOHSCROLL: on a multiline edit that is the switch that
             * turns word wrap off, and a transcript of window titles and help
             * text is worth reading without a horizontal scrollbar. */
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0, hg_g_commandbox_wnd, (HMENU)101, GetModuleHandle(NULL), NULL
        );

        hg_g_commandbox_in_wnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            0, 0, 0, 0, hg_g_commandbox_wnd, (HMENU)102, GetModuleHandle(NULL), NULL
        );

        hg_g_commandbox_btn_wnd = CreateWindowExW(
            0, L"BUTTON", L"Execute (Enter)",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hg_g_commandbox_wnd, (HMENU)103, GetModuleHandle(NULL), NULL
        );

        SetWindowSubclass(hg_g_commandbox_out_wnd, commandbox_edit_subclass_proc, 1, 0);
        SetWindowSubclass(hg_g_commandbox_in_wnd, commandbox_edit_subclass_proc, 2, 0);

        SendMessageW(hg_g_commandbox_out_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
        SendMessageW(hg_g_commandbox_in_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
        SendMessageW(hg_g_commandbox_btn_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);

        RECT rc_client;
        GetClientRect(hg_g_commandbox_wnd, &rc_client);
        SendMessageW(hg_g_commandbox_wnd, WM_SIZE, 0, MAKELPARAM(rc_client.right, rc_client.bottom));

        /* What the box opens on. A blank transcript and a cursor is a prompt
         * you have to have been told about; the keys are the thing a first-time
         * reader needs and the thing a returning one forgets. */
        hg_command_print_key_help();

        ShowWindow(hg_g_commandbox_wnd, SW_SHOWNORMAL);
        commandbox_after_show();
    } else {
        MessageBoxW(NULL, L"Failed to create the Command Box window.", L"hgfloater", MB_ICONERROR);
    }
}

LRESULT CALLBACK commandbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_DPICHANGED:
        /* Scale ownership stays with the floater/taskbox pair; just take the size. */
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        return 0;

    case WM_ACTIVATE:
        /* Clicking the window anywhere, or alt-tabbing back to it, puts the
         * keyboard where typing belongs. */
        if (LOWORD(w_param) != WA_INACTIVE)
            commandbox_focus_input();
        return 0;

    case WM_COMMAND:
        if (LOWORD(w_param) == 103) {
            commandbox_execute();
            return 0;
        }
        break;


    case WM_SIZE: {
        int cx = LOWORD(l_param);
        int cy = HIWORD(l_param);
        
        double ws = hg_window_scale(hwnd);
        int border = SCW(ws, 8);
        int btn_h = SCW(ws, 26);
        int cx_inner = cx - border * 2;

        int line_h = commandbox_line_height(hwnd, ws);

        int min_out_h = line_h * 3 + SCW(ws, 6);
        int min_in_h = line_h * 1 + SCW(ws, 6);

        int rem_h = cy - (border * 4 + btn_h);
        int out_h = (rem_h * 7) / 10;
        int in_h = rem_h - out_h;

        if (out_h < min_out_h) {
            out_h = min_out_h;
            in_h = rem_h - out_h;
        }
        if (in_h < min_in_h) {
            in_h = min_in_h;
            out_h = rem_h - in_h;
        }

        if (hg_g_commandbox_out_wnd)
            MoveWindow(hg_g_commandbox_out_wnd, border, border, cx_inner, out_h, TRUE);
        if (hg_g_commandbox_in_wnd)
            MoveWindow(hg_g_commandbox_in_wnd, border, border * 2 + out_h, cx_inner, in_h, TRUE);
        if (hg_g_commandbox_btn_wnd)
            MoveWindow(hg_g_commandbox_btn_wnd, border, border * 3 + out_h + in_h, cx_inner, btn_h, TRUE);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO *)l_param;
        
        double ws = hg_window_scale(hwnd);
        int border = SCW(ws, 8);
        int btn_h = SCW(ws, 26);
        int line_h = commandbox_line_height(hwnd, ws);

        int min_out_h = line_h * 3 + SCW(ws, 6);
        int min_in_h = line_h * 1 + SCW(ws, 6);
        int min_cy = border * 4 + min_out_h + min_in_h + btn_h;

        RECT rc_win = {0, 0, SCW(ws, 200), min_cy};
        AdjustWindowRectEx(&rc_win, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, FALSE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);

        mmi->ptMinTrackSize.x = rc_win.right - rc_win.left;
        mmi->ptMinTrackSize.y = rc_win.bottom - rc_win.top;
        return 0;
    }

    case WM_EXITSIZEMOVE: {
        /* Persist once per drag; saving on WM_MOVE hit the INI file on every pixel. */
        if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            RECT rc;
            if (GetWindowRect(hwnd, &rc)) {
                save_commandbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            }
        }
        return 0;
    }

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
        BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
        BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);
        if (w_param == VK_SPACE && is_ctrl) {
            SetFocus(hg_g_commandbox_in_wnd);
            int len = GetWindowTextLengthW(hg_g_commandbox_in_wnd);
            SendMessageW(hg_g_commandbox_in_wnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            return 0;
        }
        if (is_alt) {
            int dx = 0, dy = 0;
            int move_step = SCW(hg_window_scale(hwnd), 20);
            if (w_param == VK_LEFT)
                dx = -move_step;
            else if (w_param == VK_RIGHT)
                dx = move_step;
            else if (w_param == VK_UP)
                dy = -move_step;
            else if (w_param == VK_DOWN)
                dy = move_step;
            if (dx || dy) {
                move_window_by_offset(hwnd, dx, dy);
                /* Keyboard moves bypass WM_EXITSIZEMOVE, so persist here. */
                RECT rc;
                if (GetWindowRect(hwnd, &rc)) {
                    save_commandbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
                }
                return 0;
            }
        }
        if (is_ctrl) {
            int dw = 0, dh = 0;
            int resize_step = SCW(hg_window_scale(hwnd), 20);
            if (w_param == VK_LEFT)
                dw = -resize_step;
            else if (w_param == VK_RIGHT)
                dw = resize_step;
            else if (w_param == VK_UP)
                dh = -resize_step;
            else if (w_param == VK_DOWN)
                dh = resize_step;
            if (dw || dh) {
                resize_window_by_offset(hwnd, dw, dh);
                return 0;
            }
        }
        if (is_ctrl && !is_alt && (w_param == 'S' || w_param == 'H')) {
            commandbox_set_mode((w_param == 'S') ? HG_CB_MODE_SCROLL : HG_CB_MODE_HISTORY);
            commandbox_focus_input();
            return 0;
        }
        if (s_cb_mode != HG_CB_MODE_NONE && !is_ctrl && !is_alt && w_param != VK_ESCAPE) {
            if (commandbox_mode_key(w_param))
                return 0;
        }
        if (w_param == VK_ESCAPE) {
            commandbox_escape(hwnd);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        short delta = (short)HIWORD(w_param);
        if (LOWORD(w_param) & MK_CONTROL) {
            double ws = hg_window_scale(hwnd);
            int size = (int)(ABS(hg_g_commandbox_font_size) / (ws > 0 ? ws : 1.0) + 0.5);
            size += (delta > 0 ? 1 : -1);
            if (size < 8) size = 8;
            if (size > 72) size = 72;
            hg_g_commandbox_font_size = -SCW(ws, size);
            save_commandbox_font_config();
            
            load_commandbox_font();
            if (hg_g_commandbox_out_wnd)
                SendMessageW(hg_g_commandbox_out_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
            if (hg_g_commandbox_in_wnd)
                SendMessageW(hg_g_commandbox_in_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
            if (hg_g_commandbox_btn_wnd)
                SendMessageW(hg_g_commandbox_btn_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);

            RECT rc;
            GetClientRect(hwnd, &rc);
            SendMessageW(hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
            return 0;
        }
        if (GetKeyState(VK_MENU) < 0) {
            if (!hg_step_alpha_value(&hg_g_commandbox_alpha, delta))
                return 0;
            SetLayeredWindowAttributes(hwnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
            save_commandbox_alpha_config();
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_CLOSE:
        commandbox_set_mode(HG_CB_MODE_NONE);
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
