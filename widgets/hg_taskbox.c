#include "hg_taskbox.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "hg_taskbox_internal.h"

void update_size(int delta)
{
    int old_size = ABS(hg_g_current_font_size);
    if (old_size < SC(16))
        old_size = SC(16);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(16))
        new_size = SC(16);
    if (new_size > SC(128))
        new_size = SC(128);
    if (new_size == old_size)
        return;
    hg_g_current_font_size = -new_size;
    save_taskbox_font_config();
    release_font_handle(&hg_g_toolbar_btn_font, FALSE);

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        RECT rc;
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        if (tb_width <= 0)
            tb_width = 1;

        int cols = get_items_per_row(tb_width, old_size);
        if (cols <= 0)
            cols = 1;

        /* Calculate exact width required for the SAME number of columns but NEW icon size */
        int exact_tb_width = hg_snap_width_for_cols(cols, new_size);
        int new_w = exact_tb_width + border * 2;

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        load_shortcuts();
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }
    refresh_window_list(TRUE);
}

void update_edit_font_size(int delta)
{
    int old_size = ABS(hg_g_edit_font_size);
    if (old_size < SC(12))
        old_size = SC(12);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(12))
        new_size = SC(12);
    if (new_size > SC(128))
        new_size = SC(128);
    if (new_size == old_size)
        return;
    hg_g_edit_font_size = -new_size;
    save_taskbox_font_config();

    release_font_handle(&hg_g_main_font, TRUE);

    hg_g_main_font =
        CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!hg_g_main_font)
        hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if (hg_g_edit_msg_wnd) {
        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        InvalidateRect(hg_g_edit_msg_wnd, NULL, TRUE);
    }

    if (hg_g_tooltip_wnd) {
        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
    }

    /* The about edit also uses hg_g_main_font; without this it keeps the destroyed handle. */
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        HWND about_edit = GetDlgItem(hg_g_about_wnd, 100);
        if (about_edit) {
            SendMessageW(about_edit, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        }
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }

    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hwnd && IsWindow(hg_g_monitors[i].hwnd)) {
            HWND edit_wnd = GetDlgItem(hg_g_monitors[i].hwnd, 104);
            if (edit_wnd) {
                SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                /* Trigger WM_SIZE to recalculate edit box height */
                RECT rc;
                GetClientRect(hg_g_monitors[i].hwnd, &rc);
                SendMessageW(hg_g_monitors[i].hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
                InvalidateRect(hg_g_monitors[i].hwnd, NULL, TRUE);
            }
        }
    }
}

void update_taskbox_alpha(int delta)
{
    if (!hg_step_alpha_value(&hg_g_taskbox_alpha, delta))
        return;
    if (hg_g_taskbox_wnd) {
        SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    }
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    save_alpha_config();
}

void set_taskbox_opacity_pct(int pct)
{
    if (pct < 30)
        pct = 30;
    if (pct > 100)
        pct = 100;
    int alpha = (pct * 255 + 50) / 100;
    if (alpha < HG_MIN_ALPHA)
        alpha = HG_MIN_ALPHA;
    if (alpha > HG_MAX_ALPHA)
        alpha = HG_MAX_ALPHA;
    hg_g_taskbox_alpha = (BYTE)alpha;
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    }
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    save_alpha_config();
}

int taskbox_toolbar_icon_size(void)
{
    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16))
        icon_size = SC(16);
    return icon_size;
}

typedef struct HgTaskboxLayoutState {
    RECT sizemove_start_rect;
} HgTaskboxLayoutState;

HgTaskboxFocusState hg_taskbox_focus = {0, 0};

/* Consecutive 100 ms hover-check ticks with the cursor outside the taskbox. */
static int s_hover_outside_ticks = 0;

void hide_taskbox(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    /* The hover-check timer must die on every hide path, not only its own collapse. */
    KillTimer(hwnd, HG_TIMER_HOVER_CHECK);
    s_hover_outside_ticks = 0;
    if (hg_g_floater_wnd) {
        RECT f_rc;
        GetWindowRect(hg_g_floater_wnd, &f_rc);
        int fw = f_rc.right - f_rc.left;
        int fh = f_rc.bottom - f_rc.top;

        /* Return to where the floater sat before it expanded, rather than to the
         * center of a taskbox that may have been pushed away from a screen edge -
         * but follow the taskbox if it was moved (dragged, nudged with Alt+arrows,
         * resized, or sent aside by the M button) while it was open. */
        RECT t_rc;
        GetWindowRect(hwnd, &t_rc);
        int fx, fy;
        if (hg_g_floater_home_valid) {
            MONITORINFO mi;
            SecureZeroMemory(&mi, sizeof(mi));
            mi.cbSize = sizeof(mi);
            RECT work = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
            if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
                work = mi.rcWork;

            HgBox home = {hg_g_floater_home_rect.left, hg_g_floater_home_rect.top,
                          hg_g_floater_home_rect.left + fw, hg_g_floater_home_rect.top + fh};
            HgBox from = {hg_g_taskbox_expand_rect.left, hg_g_taskbox_expand_rect.top,
                          hg_g_taskbox_expand_rect.right, hg_g_taskbox_expand_rect.bottom};
            HgBox to = {t_rc.left, t_rc.top, t_rc.right, t_rc.bottom};
            HgBox work_box = {work.left, work.top, work.right, work.bottom};
            HgBox moved = hg_calc_follow_move(home, from, to, work_box);
            fx = moved.left;
            fy = moved.top;
        } else {
            fx = t_rc.left + (t_rc.right - t_rc.left) / 2 - fw / 2;
            fy = t_rc.top + (t_rc.bottom - t_rc.top) / 2 - fh / 2;
        }

        SetWindowPos(hg_g_floater_wnd, HWND_TOPMOST, fx, fy, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        ShowWindow(hg_g_floater_wnd, SW_SHOW);
        /* Take foreground only while this process still owns it; the auto-collapse
         * path must not steal focus from the application the user switched to. */
        HWND fg_wnd = GetForegroundWindow();
        DWORD fg_pid = 0;
        if (fg_wnd)
            GetWindowThreadProcessId(fg_wnd, &fg_pid);
        if (!fg_wnd || fg_pid == GetCurrentProcessId()) {
            SetForegroundWindow(hg_g_floater_wnd);
        }
        save_floater_geometry_config(fx, fy, fw, fh);
        hg_g_floater_home_valid = FALSE;
    }
    ShowWindow(hwnd, SW_HIDE);
    load_shortcuts_if_changed();
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
}

void activate_toolbar_item(int index)
{
    if (index < 0)
        return;
    switch (hg_toolbar_builtin_click_role(index)) {
    case HG_TOOLBAR_CLICK_NONE:
        break;
    case HG_TOOLBAR_CLICK_EXIT_APP:
        if (hg_g_floater_wnd)
            PostMessageW(hg_g_floater_wnd, WM_COMMAND, HG_IDM_CLOSE_APP, 0);
        break;
    case HG_TOOLBAR_CLICK_TOGGLE_DESKTOP: {
        static BOOL is_desktop_shown = FALSE;
        is_desktop_shown = !is_desktop_shown;
        EnumWindows(minimize_restore_enum_proc, (LPARAM)is_desktop_shown);
        break;
    }
    case HG_TOOLBAR_CLICK_OPEN_MENU: {
        HMENU h_menu = taskbox_create_main_popup_menu();
        if (!h_menu)
            break;

        POINT pt;
        GetCursorPos(&pt);
        int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hg_g_taskbox_wnd);
        taskbox_dispatch_main_menu_command((UINT)cmd);
        break;
    }
    case HG_TOOLBAR_CLICK_SHOW_COMMANDBOX:
        show_commandbox_window();
        break;
    case HG_TOOLBAR_CLICK_TOGGLE_MUTE:
        set_system_mute(!get_system_mute());
        update_toolbar_tooltips(hg_g_toolbar_wnd);
        update_focus_message(1, HG_TOOL_ICON_VOLUME);
        if (hg_g_toolbar_wnd) {
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
        }
        break;
    case HG_TOOLBAR_CLICK_RELOCATE_AWAY:
        /* A click on M (no drag) sends the pair to the first free cardinal slot. */
        hg_relocate_taskbox_away(hg_g_taskbox_wnd);
        break;
    case HG_TOOLBAR_CLICK_TOGGLE_PIN:
        /* Pinned, the taskbox ignores the hover-out collapse; every explicit
         * close (X, Esc, the hotkey, a floater click) still works. */
        hg_g_taskbox_pinned = !hg_g_taskbox_pinned;
        append_message(hg_g_taskbox_pinned ? L"Taskbox pinned open." : L"Taskbox unpinned.");
        update_toolbar_tooltips(hg_g_toolbar_wnd);
        if (hg_g_toolbar_wnd)
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
        break;
    case HG_TOOLBAR_CLICK_FLOATER_ADJUST:
        /* Collapse to the floater for size/alpha tuning: hover-expand is suppressed
         * while in this mode (Ctrl+Wheel resizes, Alt+Wheel changes opacity); a
         * click on the floater returns to the taskbox. */
        hg_g_floater_adjust_mode = TRUE;
        hide_taskbox(hg_g_taskbox_wnd);
        break;
    default:
        break;
    }

    if (index >= HG_NUM_BASIC_ICONS) {
        int s_idx = index - HG_NUM_BASIC_ICONS;
        if (s_idx >= 0 && s_idx < hg_g_shortcut_count) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts[s_idx].path, NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

void activate_taskbar_item(int index)
{
    if (index < 0 || index >= hg_g_window_count)
        return;
    HWND target = hg_g_window_items[index].hwnd;
    if (IsWindow(target)) {
        if (IsIconic(target))
            ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    }
}

void update_focus_message(int override_type, int override_index)
{
    if (override_type == -1 && override_index == -1) {
        return;
    }

    int type = (override_type == -2) ? hg_taskbox_focus.area : override_type;
    int index = (override_index == -2) ? hg_taskbox_focus.index : override_index;

    if (type == 0) {
        if (index >= 0 && index < hg_g_window_count) {
            append_message(hg_g_window_items[index].title);
        }
    } else if (type == 1) {
        int total_items = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        if (index >= 0 && index < total_items) {
            const WCHAR *focus_text = hg_toolbar_builtin_focus_text(index);
            if (focus_text) {
                append_message(focus_text);
            } else if (hg_toolbar_builtin_has_value(index)) {
                static WCHAR value_str[64];
                if (hg_toolbar_builtin_value_text(index, HG_TOOLBAR_TEXT_FOCUS, value_str, HG_ARRAYSIZE(value_str))) {
                    append_message(value_str);
                }
            } else {
                append_message(hg_g_shortcuts[index - HG_NUM_BASIC_ICONS].name);
            }
        }
    }
}

void reset_taskbox_focus(void)
{
    hg_taskbox_focus.area = 0;
    hg_taskbox_focus.index = 0;
    update_focus_message(-2, -2);
}

/* Shared by interactive resize and WM_EXITSIZEMOVE: pick the column count that
 * best fits the requested window height. */
int taskbox_cols_from_height(int window_height, int icon_size, int border, int total_items)
{
    int edit_height = hg_measure_edit_height(hg_g_edit_msg_wnd, hg_g_main_font, hg_g_scale_factor);
    int row_height = icon_size + SC(10);
    int available_toolbar_h = window_height - (border * 2 + edit_height);
    int target_rows = (available_toolbar_h - SC(10) + row_height / 2) / row_height;
    if (target_rows < 1)
        target_rows = 1;
    if (target_rows > total_items)
        target_rows = total_items;
    return (total_items + target_rows - 1) / target_rows;
}

LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass,
                                    DWORD_PTR dw_ref_data)
{
    if (hg_readonly_edit_common(hwnd, msg, w_param)) {
        return 0;
    }
    switch (msg) {
    case WM_LBUTTONDOWN: {
        ReleaseCapture();
        SendMessageW(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
        return 0;
    }
    case WM_KEYDOWN: {
        if (w_param == VK_F2) {
            if (hg_g_floater_wnd)
                PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        if (LOWORD(w_param) & MK_CONTROL) {
            short delta = (short)HIWORD(w_param);
            update_edit_font_size(delta > 0 ? 1 : -1);
            return 0;
        } else if (GetKeyState(VK_MENU) < 0) {
            short delta = (short)HIWORD(w_param);
            update_taskbox_alpha(delta > 0 ? 1 : -1);
            return 0;
        }
        break;
    }
    case WM_CONTEXTMENU: {
        HMENU h_menu = CreatePopupMenu();
        if (!h_menu)
            return 0;
        AppendMenuW(h_menu, MF_STRING, HG_IDM_EDIT_COPYALL, L"Copy Status Line (&A)");

        POINT pt;
        if (l_param == (LPARAM)-1) { /* Keyboard shortcut */
            RECT rc;
            GetWindowRect(hwnd, &rc);
            pt.x = rc.left + 5;
            pt.y = rc.top + 5;
        } else {
            pt.x = GET_X_LPARAM(l_param);
            pt.y = GET_Y_LPARAM(l_param);
        }

        int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y,
                                                 hwnd);

        if (cmd == HG_IDM_EDIT_COPYALL) {
            SendMessageW(hwnd, EM_SETSEL, 0, (LPARAM)-1);
            SendMessageW(hwnd, WM_COPY, 0, 0);
            SendMessageW(hwnd, EM_SETSEL, (WPARAM)-1, 0);
        }

        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

void update_layout(HWND hwnd)
{
    if (!hwnd || !hg_g_edit_msg_wnd || !hg_g_toolbar_wnd || !hg_g_main_font)
        return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right;
    int border = SC(HG_BORDER_THICKNESS);

    int edit_height = hg_measure_edit_height(hg_g_edit_msg_wnd, hg_g_main_font, hg_g_scale_factor);

    int tb_width = width - (border * 2);
    if (tb_width <= 0)
        tb_width = 1;

    /* Calculate necessary height for the taskbox based on columns and rows */
    int icon_size = taskbox_toolbar_icon_size();
    int cols = get_items_per_row(tb_width, icon_size);
    if (cols <= 0)
        cols = 1;
    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    int rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (rows <= 0)
        rows = 1;

    int row_height = icon_size + SC(10);

    RECT win_rc;
    GetWindowRect(hwnd, &win_rc);
    int current_height = win_rc.bottom - win_rc.top;
    int current_width = win_rc.right - win_rc.left;

    int req_toolbar_height = SC(10) + rows * row_height;
    int required_total_height = border * 2 + edit_height + req_toolbar_height;

    int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
    int required_total_width = border * 2 + exact_tb_width;

    /* If the current window size is different from required, resize it */
    if (!hg_g_in_sizemove) {
        if (current_height != required_total_height || current_width != required_total_width) {
            SetWindowPos(hwnd, NULL, 0, 0, required_total_width, required_total_height,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return; /* SetWindowPos will trigger WM_SIZE, calling update_layout again */
        }
    }

    MoveWindow(hg_g_edit_msg_wnd, border, border, exact_tb_width, edit_height, TRUE);
    MoveWindow(hg_g_toolbar_wnd, border, border + edit_height, exact_tb_width, req_toolbar_height, TRUE);
    update_toolbar_tooltips(hg_g_toolbar_wnd);
}

static LRESULT taskbox_controller_on_create(HWND hwnd)
{
    /* 툴바 클래스는 다른 클래스들과 함께 공유 등록 테이블에서 등록됨 */

    /* UI용 일반 폰트 생성 (아이콘 크기와 분리) */
    hg_g_main_font =
        CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!hg_g_main_font)
        hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    hg_g_toolbar_wnd = CreateWindowExW(0, HG_CLASS_TOOLBAR, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                                       (HMENU)HG_IDC_TOOLBAR, GetModuleHandle(NULL), NULL);

    hg_g_edit_msg_wnd =
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 0, 0, 0, 0, hwnd,
                        (HMENU)HG_IDC_EDIT_MSG, GetModuleHandle(NULL), NULL);
    if (hg_g_edit_msg_wnd) {
        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        SetWindowTextW(hg_g_edit_msg_wnd,
                       L"X: Exit | O: Options | Ctrl+Arrow/Wheel: Grid/Size | Alt+Arrow/Wheel: Move/Alpha");
        hg_g_edit_msg_tick = GetTickCount64();
        SetWindowSubclass(hg_g_edit_msg_wnd, edit_subclass_proc, 0, 0);
        disable_window_ime(hg_g_edit_msg_wnd);
    }

    /* 툴팁 생성: 메인 윈도우를 소유자로 지정하되 TOPMOST 유지 */
    hg_g_tooltip_wnd =
        CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, GetModuleHandle(NULL), NULL);

    if (hg_g_tooltip_wnd) {
        SendMessageW(hg_g_tooltip_wnd, TTM_SETMAXTIPWIDTH, 0, SC(1000));
        /* 툴팁 폰트도 DPI 배율이 적용된 폰트로 설정 */
        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        /* 즉시 표시되도록 설정 (약간의 지연 시간 부여) */
        SendMessageW(hg_g_tooltip_wnd, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
        /* 시스템 테마를 해제하여 설정한 폰트와 컬러가 적용되게 함 */
        SendMessageW(hg_g_tooltip_wnd, CCM_SETWINDOWTHEME, 0, (LPARAM)L"");
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
    }

    /* taskbar_wnd creation removed */

    load_shortcuts();
    refresh_window_list(TRUE);
    update_toolbar_tooltips(hg_g_toolbar_wnd);

    /* 창 테두리 및 모서리 등 DWM 속성 설정 */
    apply_dwm_attributes(hwnd);

    SetLayeredWindowAttributes(hwnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    SetTimer(hwnd, HG_TIMER_TASKBOX_REFRESH, 1000, NULL); /* 1초마다 시계 및 목록 갱신 */
    return 0;
}

static LRESULT taskbox_controller_on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    /* 하이라이트 효과 (깜빡임) */
    COLORREF bg_color = HG_CLICKABLE_BG;
    if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
        bg_color = HG_COLOR_BG_FLASH;
    }
    HBRUSH hbr_bg = hg_cached_solid_brush(bg_color);
    if (hbr_bg) {
        FillRect(hdc, &rc, hbr_bg);
    }

    /* 외곽선 그리기 */
    int border = SC(HG_BORDER_THICKNESS);
    HWND fg = GetForegroundWindow();
    BOOL is_focused = (fg == hwnd || IsChild(hwnd, fg));
    COLORREF border_color = is_focused ? HG_COLOR_BORDER_SELECTED : HG_COLOR_BG_TOOLBAR;
    HBRUSH hbr_border = hg_cached_solid_brush(border_color);
    if (hbr_border) {
        /* 상하좌우 사각형으로 채우기 */
        RECT rc_top = {0, 0, rc.right, border};
        RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
        RECT rc_left = {0, border, border, rc.bottom - border};
        RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};

        FillRect(hdc, &rc_top, hbr_border);
        FillRect(hdc, &rc_bottom, hbr_border);
        FillRect(hdc, &rc_left, hbr_border);
        FillRect(hdc, &rc_right, hbr_border);
    }
    EndPaint(hwnd, &ps);
    return 0;
}
static LRESULT taskbox_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    int dx = 0, dy = 0;
    int move_step = SC(20);
    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    /* Do not infer Alt from WM_SYSKEYDOWN: when the window is active without
     * keyboard focus, plain keys also arrive as WM_SYSKEYDOWN, and treating
     * them as Alt turned arrow navigation into window moves. */
    BOOL is_alt = (GetKeyState(VK_MENU) < 0);

    /* Alt + 방향키/wasd: 현재 창 이동 (태스크 박스) */
    if (is_alt) {
        if (w_param == VK_LEFT || w_param == 'A')
            dx = -move_step;
        else if (w_param == VK_RIGHT || w_param == 'D')
            dx = move_step;
        else if (w_param == VK_UP || w_param == 'W')
            dy = -move_step;
        else if (w_param == VK_DOWN || w_param == 'S')
            dy = move_step;

        if (dx != 0 || dy != 0) {
            move_window_by_offset(hwnd, dx, dy);
            /* 이동 후에도 최소한 일부는 화면에 보이도록 보호 */
            ensure_window_visible(hwnd, L"taskbox");
            hg_g_hover_check_armed = FALSE;
            GetCursorPos(&hg_g_last_mouse_pos);
            return 0;
        }

        /* Alt + +/-: 투명도 조절 */
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_taskbox_alpha(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_taskbox_alpha(-1);
            return 0;
        }
    }

    /* Ctrl + +/-: 텍스트 글꼴 크기 조절, Ctrl + 방향키/wasd: 창 크기/그리드 조절 */
    if (is_ctrl) {
        int icon_size = taskbox_toolbar_icon_size();

        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_edit_font_size(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_edit_font_size(-1);
            return 0;
        } else if (w_param == VK_LEFT || w_param == 'A') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);

            if (tb_width > exact_tb_width + SC(5)) {
                /* Right padding exists, snap to current cols */
            } else if (cols > 1) {
                cols--;
                exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
            }

            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        } else if (w_param == VK_RIGHT || w_param == 'D') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            cols++;
            int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        } else if (w_param == VK_UP || w_param == 'W') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
            int total_items = total_tasks + total_shortcuts;
            if (total_items <= 0)
                total_items = 1;

            int current_rows = (total_items + cols - 1) / cols;
            if (current_rows > 1) {
                while (cols < total_items) {
                    cols++;
                    int new_rows = (total_items + cols - 1) / cols;
                    if (new_rows < current_rows)
                        break;
                }
                int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
                int new_w = exact_tb_width + border * 2;
                SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        } else if (w_param == VK_DOWN || w_param == 'S') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
            int total_items = total_tasks + total_shortcuts;
            if (total_items <= 0)
                total_items = 1;

            int current_rows = (total_items + cols - 1) / cols;
            while (cols > 1) {
                cols--;
                int new_rows = (total_items + cols - 1) / cols;
                if (new_rows > current_rows)
                    break;
            }

            int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
    }

    if (w_param == 'C' && !is_ctrl && !is_alt) {
        show_commandbox_window();
        return 0;
    }

    /* Esc: 창 닫기 */
    if (w_param == VK_ESCAPE) {
        hide_taskbox(hwnd);
        return 0;
    }
    if (msg == WM_SYSKEYDOWN)
        return DefWindowProcW(hwnd, msg, w_param, l_param);

    /* 탐색 및 선택 */
    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

    if (total_tasks > 0 || total_shortcuts > 0) {
        int icon_size = taskbox_toolbar_icon_size();

        RECT rc_toolbar;
        GetClientRect(hg_g_toolbar_wnd, &rc_toolbar);

        int cols = get_items_per_row(rc_toolbar.right, icon_size);
        int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
        if (min_required_rows <= 0)
            min_required_rows = 1;

        int visible_rows = (rc_toolbar.bottom - SC(20) + SC(10)) / (icon_size + SC(10));
        if (visible_rows <= 0)
            visible_rows = 1;

        int rows = (visible_rows > min_required_rows) ? visible_rows : min_required_rows;
        int total_cells = rows * cols;

        int current_cell = -1;
        if (hg_taskbox_focus.area == 0) {
            current_cell = hg_taskbox_focus.index;
        } else {
            current_cell = total_cells - 1 - hg_taskbox_focus.index;
        }

        if (current_cell < 0)
            current_cell = 0;

        int r = current_cell / cols;
        int c = current_cell % cols;
        BOOL changed = FALSE;

        if (w_param == VK_LEFT || w_param == 'A') {
            c--;
            changed = TRUE;
        } else if (w_param == VK_RIGHT || w_param == 'D') {
            c++;
            changed = TRUE;
        } else if (w_param == VK_UP || w_param == 'W') {
            r--;
            changed = TRUE;
        } else if (w_param == VK_DOWN || w_param == 'S') {
            r++;
            changed = TRUE;
        } else if (w_param == VK_F2) {
            if (hg_g_floater_wnd)
                PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
        } else if (w_param == VK_SPACE) {
            if (hg_taskbox_focus.area == 0)
                activate_taskbar_item(hg_taskbox_focus.index);
            else
                activate_toolbar_item(hg_taskbox_focus.index);
        } else if (w_param == VK_RETURN) {
            RECT rc_item;
            get_toolbar_item_rect(hg_taskbox_focus.area, hg_taskbox_focus.index, rc_toolbar.right, rc_toolbar.bottom,
                                  icon_size, &rc_item);
            PostMessageW(hg_g_toolbar_wnd, WM_RBUTTONUP, 0, 0); // Send 0 for l_param to indicate keyboard trigger
        }

        if (changed) {
            if (c < 0) {
                c = cols - 1;
                r--;
            }
            if (c >= cols) {
                c = 0;
                r++;
            }
            if (r < 0)
                r = 0;
            if (r >= rows)
                r = rows - 1;

            int new_cell = r * cols + c;

            // 빈 공간이라면 가장 가까운 유효한 셀로 이동 (이 경우는 Task의 마지막이나 Shortcut의 첫번째가 될 것)
            if (new_cell >= total_tasks && new_cell < total_cells - total_shortcuts) {
                if (new_cell > current_cell) {
                    new_cell = total_cells - total_shortcuts;
                } else {
                    new_cell = total_tasks - 1;
                }
            }
            if (new_cell < 0)
                new_cell = 0;
            if (new_cell >= total_cells)
                new_cell = total_cells - 1;

            // 새로운 cell의 정보를 다시 item_type/index로 매핑
            if (new_cell < total_tasks) {
                hg_taskbox_focus.area = 0;
                hg_taskbox_focus.index = new_cell;
            } else if (new_cell >= total_cells - total_shortcuts) {
                hg_taskbox_focus.area = 1;
                hg_taskbox_focus.index = total_cells - 1 - new_cell;
            }
            update_focus_message(-2, -2);
        }

        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    return 0;
}

static BOOL taskbox_forward_floater_command(UINT cmd, WPARAM w_param, LPARAM l_param)
{
    if (cmd != HG_IDM_ABOUT && cmd != HG_IDM_RESET_ALL)
        return FALSE;

    if (hg_g_floater_wnd)
        SendMessage(hg_g_floater_wnd, WM_COMMAND, w_param, l_param);
    return TRUE;
}

static BOOL taskbox_dispatch_font_command(UINT cmd)
{
    if (cmd == HG_IDM_FONT_UP) {
        update_edit_font_size(1);
        return TRUE;
    }
    if (cmd == HG_IDM_FONT_DOWN) {
        update_edit_font_size(-1);
        return TRUE;
    }
    return FALSE;
}

static LRESULT taskbox_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    UINT cmd = LOWORD(w_param);
    BOOL handled = taskbox_forward_floater_command(cmd, w_param, l_param) || taskbox_dispatch_font_command(cmd);

    if (taskbox_handle_audio_menu_command(cmd)) {
        return 0;
    }

    if (!handled && cmd == HG_IDM_CLOSE_APP) {
        if (hg_g_floater_wnd) {
            DestroyWindow(hg_g_floater_wnd);
        }
    }
    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}
static LRESULT taskbox_controller_on_destroy(HWND hwnd)
{
    hg_g_taskbox_highlight_ticks = 0;
    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
    KillTimer(hwnd, HG_TIMER_TASKBOX_REFRESH);
    KillTimer(hwnd, HG_TIMER_HOVER_CHECK);

    /* GDI 전역 리소스(font, brush) 및 전역 툴팁은 wWinMain의 cleanup_finish에서 안전하게 일괄 해제하므로 여기서
     * 삭제하지 않음 */

    for (int i = 0; i < hg_g_shortcut_count; i++) {
        release_shortcut_item_icon(&hg_g_shortcuts[i]);
    }
    for (int i = 0; i < hg_g_window_count; i++) {
        release_window_item_icon(&hg_g_window_items[i]);
    }
    /* 전역 종료는 floater_proc에서 처리하므로 주석 처리 */
    /* PostQuitMessage(0); */
    return 0;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static HgTaskboxLayoutState layout_state = {{0, 0, 0, 0}};

    switch (msg) {
    /* WM_DISPLAYCHANGE is handled once, by the floater, for the whole process. */
    case WM_DPICHANGED: {
        /* The floater/taskbox pair is co-located, so it owns the process scale. */
        hg_update_scale_from_dpi(LOWORD(w_param));
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        release_font_handle(&hg_g_toolbar_btn_font, FALSE);
        update_layout(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
            release_font_handle(&hg_g_floater_time_font, FALSE);
            release_font_handle(&hg_g_floater_date_font, FALSE);
            update_floater_layout(hg_g_floater_wnd);
            InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_ACTIVATE: {
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (!should_refresh_theme_on_setting_change(l_param)) {
            break;
        }
        /* fallthrough */
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
    case WM_DWMCOLORIZATIONCOLORCHANGED: {
        refresh_theme_surfaces(hwnd);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO *)l_param;
        mmi->ptMinTrackSize.x = SC(HG_MIN_WINDOW_WIDTH);
        mmi->ptMinTrackSize.y = SC(HG_MIN_WINDOW_HEIGHT);
        return 0;
    }
    case WM_CREATE:
        return taskbox_controller_on_create(hwnd);
    case WM_MOUSEWHEEL: {
        if (GetKeyState(VK_MENU) < 0) {
            /* Alt 키가 눌린 상태에서만 투명도 조절 실행 */
            short delta = (short)HIWORD(w_param);
            update_taskbox_alpha(delta > 0 ? 1 : -1);
        } else {
            /* Alt 키가 없으면 태스크바 창으로 메시지를 전달하여 창 전체에서 스크롤 가능하게 함 */
            /* Ctrl+휠(크기조절) 메시지도 태스크바 창에서 처리되도록 전달됨 */
            SendMessageW(hg_g_toolbar_wnd, WM_MOUSEWHEEL, w_param, l_param);
        }
        return 0;
    }
    case WM_NOTIFY: {
        /* ListView 관련 통지 제거됨 */
        break;
    }
    case WM_ENTERSIZEMOVE: {
        hg_g_in_sizemove = TRUE;
        GetWindowRect(hwnd, &layout_state.sizemove_start_rect);
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
    case WM_EXITSIZEMOVE: {
        hg_g_in_sizemove = FALSE;
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);

        int dw = (rc.right - rc.left) -
                 (layout_state.sizemove_start_rect.right - layout_state.sizemove_start_rect.left);
        int dh = (rc.bottom - rc.top) -
                 (layout_state.sizemove_start_rect.bottom - layout_state.sizemove_start_rect.top);

        int icon_size = taskbox_toolbar_icon_size();
        int border = SC(HG_BORDER_THICKNESS);

        int total_tasks = hg_g_window_count;
        int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        int total_items = total_tasks + total_shortcuts;
        if (total_items <= 0)
            total_items = 1;

        int cols;
        if (ABS(dh) > ABS(dw)) {
            cols = taskbox_cols_from_height(rc.bottom - rc.top, icon_size, border, total_items);
        } else {
            int tb_width = (rc.right - rc.left) - border * 2;
            cols = get_items_per_row(tb_width, icon_size);
        }
        if (cols <= 0)
            cols = 1;

        int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
        int new_w = exact_tb_width + border * 2;

        SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        update_layout(hwnd); /* Will trigger snap exact height */
        GetWindowRect(hwnd, &rc);
        save_taskbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        if (hg_g_floater_wnd) {
            SetWindowPos(hg_g_floater_wnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        }
        return 0;
    }
    case WM_TIMER:
        if (w_param == HG_TIMER_TASKBOX_REFRESH) {
            if (IsWindowVisible(hwnd) && !hg_g_menu_active) {
                /* Idle status line falls back to the clock, which then reads
                 * the same every second until the minute rolls over. */
                hg_update_status_clock();
                refresh_window_list(FALSE);
                /* Keep the DDC/CI brightness cache warm off the paint path. */
                static int brightness_refresh_ticks = 0;
                if (++brightness_refresh_ticks >= 5) {
                    brightness_refresh_ticks = 0;
                    hg_refresh_brightness_cache();
                }
            }
        } else if (w_param == HG_TIMER_HIGHLIGHT) {
            hg_g_taskbox_highlight_ticks--;
            if (hg_g_taskbox_highlight_ticks <= 0) {
                KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (w_param == HG_TIMER_HOVER_CHECK) {
            if (IsWindowVisible(hwnd) && !hg_g_menu_active && GetCapture() == NULL && !hg_g_in_sizemove) {
                POINT pt;
                GetCursorPos(&pt);
                RECT rc;
                GetWindowRect(hwnd, &rc);
                if (!hg_g_hover_check_armed) {
                    if (PtInRect(&rc, pt)) {
                        hg_g_hover_check_armed = TRUE;
                    } else if (hg_g_last_mouse_pos.x != -1 && (pt.x != hg_g_last_mouse_pos.x || pt.y != hg_g_last_mouse_pos.y)) {
                        hg_g_hover_check_armed = TRUE;
                    }
                }
                
                /* Collapse to the floater only after the cursor has stayed outside
                 * for 0.5s (re-checked every 100ms tick); coming back inside resets
                 * the count so a brief exit doesn't collapse the taskbox. */
                if (hg_g_hover_check_armed && !hg_g_taskbox_pinned) {
                    if (!PtInRect(&rc, pt)) {
                        s_hover_outside_ticks++;
                        if (s_hover_outside_ticks >= 5) {
                            hide_taskbox(hwnd);
                        }
                    } else {
                        s_hover_outside_ticks = 0;
                    }
                } else {
                    s_hover_outside_ticks = 0;
                }
            }
        }
        return 0;
    case WM_PAINT:
        return taskbox_controller_on_paint(hwnd);
    case WM_NCHITTEST: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(hwnd, &pt);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);

        if (pt.x < border && pt.y < border)
            return HTTOPLEFT;
        if (pt.x >= rc.right - border && pt.y < border)
            return HTTOPRIGHT;
        if (pt.x < border && pt.y >= rc.bottom - border)
            return HTBOTTOMLEFT;
        if (pt.x >= rc.right - border && pt.y >= rc.bottom - border)
            return HTBOTTOMRIGHT;
        if (pt.y < border)
            return HTTOP;
        if (pt.y >= rc.bottom - border)
            return HTBOTTOM;
        if (pt.x < border)
            return HTLEFT;
        if (pt.x >= rc.right - border)
            return HTRIGHT;

        return HTCAPTION; /* 그 외 모든 클라이언트 영역은 드래그 가능하게 */
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        return taskbox_controller_on_keydown(hwnd, msg, w_param, l_param);
    case WM_COMMAND:
        return taskbox_controller_on_command(hwnd, w_param, l_param);
    case WM_SIZE: {
        update_layout(hwnd);
        InvalidateRect(hwnd, NULL, TRUE); /* 외곽선 다시 그리기 */
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        /* fallthrough */
    case WM_CTLCOLORLISTBOX: {
        HDC hdc_static = (HDC)w_param;

        /* 하이라이트 중에는 깜빡임 배경 유지 (브러시는 캐싱하여 GDI 누수 방지) */
        if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
            SetTextColor(hdc_static, hg_g_color_scheme_selected.text);
            SetBkMode(hdc_static, OPAQUE);
            SetBkColor(hdc_static, HG_COLOR_BG_FLASH);
            if (!hg_g_hbr_highlight) {
                hg_g_hbr_highlight = CreateSolidBrush(HG_COLOR_BG_FLASH);
            }
            return hg_g_hbr_highlight ? (LRESULT)hg_g_hbr_highlight : (LRESULT)GetStockObject(BLACK_BRUSH);
        }

        return hg_on_ctlcolor_edit(hdc_static);
    }
    case WM_DESTROY:
        return taskbox_controller_on_destroy(hwnd);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
