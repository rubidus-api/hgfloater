#include "hg_floater.h"
#include "../hg_utils.h"
#include "../hg_config.h"

/* Forward declaration for show_about_window which is in hgfloater.c (or widgets/hg_about.c when created) */
void show_about_window(void);

void update_floater_font_size(int delta)
{
    int new_size = hg_g_floater_font_size + (delta > 0 ? 2 : -2);
    if (new_size < HG_FLOATER_MIN_FONT_SIZE)
        new_size = HG_FLOATER_MIN_FONT_SIZE;
    if (new_size > HG_FLOATER_MAX_FONT_SIZE)
        new_size = HG_FLOATER_MAX_FONT_SIZE;
    if (hg_g_floater_font_size != new_size) {
        hg_g_floater_font_size = new_size;
        release_font_handle(&hg_g_floater_time_font, FALSE);
        release_font_handle(&hg_g_floater_date_font, FALSE);
        update_floater_layout(hg_g_floater_wnd);
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        save_floater_font_config();
    }
}

void update_floater_alpha(int delta)
{
    int new_alpha = (int)hg_g_floater_alpha + (delta > 0 ? 15 : -15);
    if (new_alpha > HG_MAX_ALPHA)
        new_alpha = HG_MAX_ALPHA;
    if (new_alpha < HG_MIN_ALPHA)
        new_alpha = HG_MIN_ALPHA;
    if (hg_g_floater_alpha == (BYTE)new_alpha)
        return;
    hg_g_floater_alpha = (BYTE)new_alpha;
    if (hg_g_floater_wnd) {
        SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
    }
    save_alpha_config();
}

void update_floater_layout(HWND hwnd)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR time_str[16], date_str[32];
    hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
    const WCHAR *months[] = {L"",    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                             L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
    const WCHAR *days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
    hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

    int time_size = hg_g_floater_font_size * 12 / 10;
    int date_size = hg_g_floater_font_size * 8 / 10;
    if (date_size < 10)
        date_size = 10;
    if (!hg_g_floater_time_font)
        hg_g_floater_time_font = CreateFontW(SC(time_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0,
                                             0, 0, 0, hg_g_font_name);
    if (!hg_g_floater_date_font)
        hg_g_floater_date_font = CreateFontW(SC(date_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0,
                                             0, 0, 0, hg_g_font_name);
    if (!hg_g_floater_time_font || !hg_g_floater_date_font)
        return;

    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return;

    SIZE sz_time = {0}, sz_date = {0};
    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_floater_time_font);
    GetTextExtentPoint32W(hdc, time_str, (int)lstrlenW(time_str), &sz_time);
    SelectObject(hdc, hg_g_floater_date_font);
    GetTextExtentPoint32W(hdc, date_str, (int)lstrlenW(date_str), &sz_date);
    SelectObject(hdc, old_font);

    int pen_width = SC(HG_BORDER_THICKNESS);
    int padding_x = pen_width + SC(20);
    int padding_y = pen_width + SC(10);
    int width = (sz_time.cx > sz_date.cx ? sz_time.cx : sz_date.cx) + padding_x;
    int height = sz_time.cy + sz_date.cy + padding_y;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.right - rc.left != width || rc.bottom - rc.top != height) {
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ReleaseDC(hwnd, hdc);
}

static LRESULT floater_controller_on_create(HWND hwnd)
{
    hg_g_shellhook_msg = RegisterWindowMessageW(L"SHELLHOOK");
    RegisterShellHookWindow(hwnd);

    SetLayeredWindowAttributes(hwnd, 0, hg_g_floater_alpha, LWA_ALPHA);

    apply_dwm_attributes(hwnd);

    update_floater_layout(hwnd);
    SetTimer(hwnd, HG_TIMER_FLOATER_CLOCK, 1000, NULL);
    return 0;
}

static LRESULT floater_controller_on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HgPaintBuffer paint_buffer;
        if (hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &paint_buffer)) {
            HDC mem_dc = paint_buffer.dc;

                int time_size = hg_g_floater_font_size * 12 / 10;
                int date_size = hg_g_floater_font_size * 8 / 10;
                if (date_size < 10)
                    date_size = 10;
                if (!hg_g_floater_time_font)
                    hg_g_floater_time_font = CreateFontW(SC(time_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                                         DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
                if (!hg_g_floater_date_font)
                    hg_g_floater_date_font = CreateFontW(SC(date_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                                         DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);

                if (hg_g_floater_time_font && hg_g_floater_date_font) {
                    COLORREF bg_color = HG_COLOR_BG_DEFAULT;
                    if (hg_g_floater_highlight_ticks > 0 && (hg_g_floater_highlight_ticks % 2 != 0)) {
                        bg_color = HG_COLOR_BG_FLASH;
                    }
                    HBRUSH bg_brush = CreateSolidBrush(bg_color);
                    int pen_width = SC(HG_BORDER_THICKNESS);

                    COLORREF border_color = HG_COLOR_BG_TOOLBAR;
                    HPEN border_pen = CreatePen(PS_SOLID, pen_width, border_color);

                    HBRUSH old_brush = NULL;
                    HPEN old_pen = NULL;
                    if (bg_brush)
                        old_brush = (HBRUSH)SelectObject(mem_dc, bg_brush);
                    if (border_pen)
                        old_pen = (HPEN)SelectObject(mem_dc, border_pen);

                    FillRect(mem_dc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

                    RECT draw_rc = rc;
                    InflateRect(&draw_rc, -pen_width / 2, -pen_width / 2);
                    if (bg_brush && border_pen) {
                        Rectangle(mem_dc, draw_rc.left, draw_rc.top, draw_rc.right, draw_rc.bottom);
                    }

                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    WCHAR time_str[16], date_str[32];
                    hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
                    const WCHAR *months[] = {L"",    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                             L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
                    const WCHAR *days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
                    hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

                    SetBkMode(mem_dc, TRANSPARENT);
                    SetTextColor(mem_dc, HG_COLOR_TEXT_DEFAULT);

                    SIZE sz_time = {0}, sz_date = {0};
                    HFONT old_font_in_paint = (HFONT)SelectObject(mem_dc, hg_g_floater_time_font);
                    GetTextExtentPoint32W(mem_dc, time_str, (int)lstrlenW(time_str), &sz_time);
                    SelectObject(mem_dc, hg_g_floater_date_font);
                    GetTextExtentPoint32W(mem_dc, date_str, (int)lstrlenW(date_str), &sz_date);

                    int total_text_height = sz_time.cy + sz_date.cy;
                    int start_y = (rc.bottom - rc.top - total_text_height) / 2;
                    if (start_y < 0)
                        start_y = 0;

                    RECT time_rc = rc;
                    time_rc.top = start_y;
                    time_rc.bottom = time_rc.top + sz_time.cy;

                    RECT date_rc = rc;
                    date_rc.top = time_rc.bottom;
                    date_rc.bottom = date_rc.top + sz_date.cy;

                    SelectObject(mem_dc, hg_g_floater_time_font);
                    DrawTextW(mem_dc, time_str, -1, &time_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(mem_dc, hg_g_floater_date_font);
                    DrawTextW(mem_dc, date_str, -1, &date_rc, DT_CENTER | DT_TOP | DT_SINGLELINE);

                    SelectObject(mem_dc, old_font_in_paint);

                    if (old_brush)
                        SelectObject(mem_dc, old_brush);
                    if (old_pen)
                        SelectObject(mem_dc, old_pen);
                    if (bg_brush)
                        DeleteObject(bg_brush);
                    if (border_pen)
                        DeleteObject(border_pen);
                }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
            hg_paint_buffer_end(&paint_buffer);
        }
    }

    EndPaint(hwnd, &ps);
    return 0;
}

static LRESULT floater_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    int dx = 0, dy = 0;
    int move_step = SC(20);
    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);

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
            ensure_window_visible(hwnd, L"floater");
            return 0;
        }

        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_alpha(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_alpha(-1);
            return 0;
        }
    }

    if (is_ctrl) {
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_font_size(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_font_size(-1);
            return 0;
        }
    }

    if (w_param == VK_F2) {
        SendMessageW(hwnd, WM_RBUTTONUP, 0, 0);
        return 0;
    }

    if (!is_ctrl && !is_alt) {
        if (w_param == 'C') {
            show_commandbox_window();
            return 0;
        } else if (w_param == 'T') {
            PostMessageW(hwnd, WM_HOTKEY, 1, 0);
            return 0;
        }
    }



    if (msg == WM_SYSKEYDOWN)
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

static BOOL floater_dispatch_monitor_command(UINT cmd)
{
    if (cmd < HG_IDM_MONITOR_BASE || cmd >= HG_IDM_MONITOR_BASE + HG_MAX_MONITORS)
        return FALSE;

    int idx = (int)(cmd - HG_IDM_MONITOR_BASE);
    if (idx >= 0 && idx < hg_g_monitor_count) {
        toggle_monitor_window(idx);
    }
    return TRUE;
}

static BOOL floater_dispatch_audio_device_command(UINT cmd)
{
    if (cmd < HG_IDM_AUDIO_DEVICE_BASE || cmd >= HG_IDM_AUDIO_DEVICE_BASE + HG_MAX_AUDIO_DEVICES)
        return FALSE;

    int idx = (int)(cmd - HG_IDM_AUDIO_DEVICE_BASE);
    if (idx >= 0 && idx < hg_g_audio_device_count) {
        if (set_default_audio_device(hg_g_audio_devices[idx].id)) {
            update_audio_device_list();
        }
    }
    return TRUE;
}

static BOOL floater_dispatch_volume_command(UINT cmd)
{
    int volume = -1;
    switch (cmd) {
    case HG_IDM_VOLUME_SET_0:
        volume = 0;
        break;
    case HG_IDM_VOLUME_SET_25:
        volume = 25;
        break;
    case HG_IDM_VOLUME_SET_50:
        volume = 50;
        break;
    case HG_IDM_VOLUME_SET_75:
        volume = 75;
        break;
    case HG_IDM_VOLUME_SET_100:
        volume = 100;
        break;
    default:
        return FALSE;
    }

    set_system_volume(volume);
    return TRUE;
}

static void floater_refresh_volume_ui(void)
{
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
}

static LRESULT floater_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    UINT cmd = LOWORD(w_param);

    BOOL handled = floater_dispatch_monitor_command(cmd) || floater_dispatch_audio_device_command(cmd) ||
                   floater_dispatch_volume_command(cmd);
    if (!handled) {
        if (cmd == HG_IDM_CLOSE_APP) {
            DestroyWindow(hwnd);
        } else if (cmd == HG_IDM_MUTE) {
            set_system_mute(!get_system_mute());
            floater_refresh_volume_ui();
        } else if (cmd == HG_IDM_OPEN_SHORTCUTS) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts_path, NULL, NULL, SW_SHOWNORMAL);
        } else if (cmd == HG_IDM_EDIT_CONFIG) {
            ShellExecuteW(NULL, L"open", L"notepad.exe", hg_g_config_path, NULL, SW_SHOWNORMAL);
        } else if (cmd == HG_IDM_ABOUT) {
            show_about_window();
        } else if (cmd == HG_IDM_RESET_ALL) {
            hg_config_reset_all(hwnd);
        } else if (cmd == HG_IDM_FONT_UP) {
            update_floater_font_size(1);
        } else if (cmd == HG_IDM_FONT_DOWN) {
            update_floater_font_size(-1);
        } else if (cmd == HG_IDM_POWER_OFF) {
            HWND h_shell = FindWindowW(L"Shell_TrayWnd", NULL);
            if (h_shell)
                PostMessageW(h_shell, WM_COMMAND, 506, 0);
        }
    }

    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}

static LRESULT floater_controller_on_destroy(HWND hwnd)
{
    DeregisterShellHookWindow(hwnd);
    KillTimer(hwnd, HG_TIMER_FLOATER_CLOCK);
    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        DestroyWindow(hg_g_about_wnd);
        hg_g_about_wnd = NULL;
    }
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        DestroyWindow(hg_g_taskbox_wnd);
        hg_g_taskbox_wnd = NULL;
    }
    unregister_global_hotkey(hwnd);
    release_font_handle(&hg_g_floater_time_font, FALSE);
    release_font_handle(&hg_g_floater_date_font, FALSE);
    hg_g_floater_wnd = NULL;
    PostQuitMessage(0);
    return 0;
}

LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static int initial_floater_font_size = 0;
    static BOOL font_drag_active = FALSE;
    switch (msg) {
    case WM_DISPLAYCHANGE: {
        update_monitor_enum();
        return 0;
    }
    case WM_MOUSEACTIVATE: {
        if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd))
            return MA_NOACTIVATE;
        return MA_ACTIVATE;
    }
    case WM_ACTIVATE: {
        if (LOWORD(w_param) != WA_INACTIVE) {
            if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                SetForegroundWindow(hg_g_taskbox_wnd);
            }
        }
        return 0;
    }
    case WM_CREATE:
        return floater_controller_on_create(hwnd);
    case WM_PAINT:
        return floater_controller_on_paint(hwnd);
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        return floater_controller_on_keydown(hwnd, msg, w_param, l_param);
    case WM_LBUTTONDOWN: {
        hg_g_drag_start_pt.x = GET_X_LPARAM(l_param);
        hg_g_drag_start_pt.y = GET_Y_LPARAM(l_param);
        font_drag_active = FALSE;
        if (GetKeyState(VK_CONTROL) < 0) {
            initial_floater_font_size = hg_g_floater_font_size;
        }
        SetCapture(hwnd);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (w_param & MK_LBUTTON && GetCapture() == hwnd) {
            POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            if (GetKeyState(VK_CONTROL) < 0) {
                if (pt.x != hg_g_drag_start_pt.x || pt.y != hg_g_drag_start_pt.y) {
                    font_drag_active = TRUE;
                }
                int delta = (pt.x - hg_g_drag_start_pt.x) / 5;
                int new_size = initial_floater_font_size + delta;
                if (new_size < HG_FLOATER_MIN_FONT_SIZE)
                    new_size = HG_FLOATER_MIN_FONT_SIZE;
                if (new_size > HG_FLOATER_MAX_FONT_SIZE)
                    new_size = HG_FLOATER_MAX_FONT_SIZE;
                if (hg_g_floater_font_size != new_size) {
                    hg_g_floater_font_size = new_size;
                    release_font_handle(&hg_g_floater_time_font, FALSE);
                    release_font_handle(&hg_g_floater_date_font, FALSE);
                    update_floater_layout(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    save_floater_font_config();
                }
            }
        } else if (!hg_g_floater_adjust_mode) {
            // Hover logic (suppressed in F floater-adjust mode so Ctrl/Alt+Wheel can
            // tune size/alpha without the floater expanding into the taskbox)
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd) && !IsWindowVisible(hg_g_taskbox_wnd)) {
                RECT f_rc, t_rc;
                GetWindowRect(hwnd, &f_rc);
                GetWindowRect(hg_g_taskbox_wnd, &t_rc);
                int cx = f_rc.left + (f_rc.right - f_rc.left) / 2;
                int cy = f_rc.top + (f_rc.bottom - f_rc.top) / 2;
                int tw = t_rc.right - t_rc.left;
                int th = t_rc.bottom - t_rc.top;
                SetWindowPos(hg_g_taskbox_wnd, HWND_TOPMOST, cx - tw / 2, cy - th / 2, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                // Make it appear instantly, refresh without forcing icon reload
                refresh_window_list(FALSE);
                ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);
                SetForegroundWindow(hg_g_taskbox_wnd);
                hg_g_hover_check_armed = TRUE;
                SetTimer(hg_g_taskbox_wnd, HG_TIMER_HOVER_CHECK, 100, NULL);
                save_taskbox_geometry_config(cx - tw / 2, cy - th / 2, tw, th);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hwnd) {
            ReleaseCapture();
            hg_g_floater_adjust_mode = FALSE;   /* a click leaves floater-adjust mode */
            if (font_drag_active) {
                /* Releasing a Ctrl+drag font-resize gesture must not toggle the taskbox. */
                font_drag_active = FALSE;
                return 0;
            }
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
                if (IsWindowVisible(hg_g_taskbox_wnd)) {
                    hide_taskbox(hg_g_taskbox_wnd);
                } else {
                    PostMessageW(hwnd, WM_HOTKEY, 1, 0);   /* expand back to the taskbox */
                }
            }
        }
        return 0;
    }
    /* WM_RBUTTONUP removed per user request */
    case WM_COMMAND:
        return floater_controller_on_command(hwnd, w_param, l_param);
    case WM_MOUSEWHEEL: {
        if (GetKeyState(VK_MENU) < 0) {
            short delta = (short)HIWORD(w_param);
            update_floater_alpha(delta > 0 ? 1 : -1);
        } else if (GetKeyState(VK_CONTROL) < 0) {
            short delta = (short)HIWORD(w_param);
            update_floater_font_size(delta > 0 ? 1 : -1);
        }
        return 0;
    }
    case WM_NCHITTEST: {
        return HTCLIENT;
    }
    case WM_ENTERSIZEMOVE: {
        hg_g_in_sizemove = TRUE;
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
    case WM_EXITSIZEMOVE: {
        hg_g_in_sizemove = FALSE;
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);
        save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }
    case WM_TIMER: {
        if (w_param == HG_TIMER_FLOATER_CLOCK) {
            static SYSTEMTIME last_st = {0};
            SYSTEMTIME st;
            GetLocalTime(&st);
            if (st.wMinute != last_st.wMinute || st.wHour != last_st.wHour || st.wDay != last_st.wDay ||
                st.wMonth != last_st.wMonth || st.wYear != last_st.wYear) {
                last_st = st;
                update_floater_layout(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            for (int i = 0; i < hg_g_monitor_count; i++) {
                if (hg_g_monitors[i].active && hg_g_monitors[i].hwnd) {
                    InvalidateRect(hg_g_monitors[i].hwnd, NULL, FALSE);
                }
            }
        } else if (w_param == HG_TIMER_HIGHLIGHT) {
            hg_g_floater_highlight_ticks--;
            if (hg_g_floater_highlight_ticks <= 0) {
                KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_HOTKEY: {
        if (w_param == 1) {
            HWND fg_wnd = GetForegroundWindow();
            if (fg_wnd && fg_wnd != hg_g_taskbox_wnd && fg_wnd != hg_g_floater_wnd &&
                fg_wnd != hg_g_about_wnd) {
                hg_g_prev_active_hwnd = fg_wnd;
            }

            ensure_window_visible(hg_g_floater_wnd, L"floater");
            ensure_window_visible(hg_g_taskbox_wnd, L"taskbox");

            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            hg_g_floater_highlight_ticks = HG_HIGHLIGHT_TICKS;
            if (!SetTimer(hwnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                hg_g_floater_highlight_ticks = 0;
            }
            InvalidateRect(hwnd, NULL, FALSE);

            if (hg_g_taskbox_wnd) {
                if (!IsWindowVisible(hg_g_taskbox_wnd)) {
                    RECT f_rc, t_rc;
                    GetWindowRect(hwnd, &f_rc);
                    GetWindowRect(hg_g_taskbox_wnd, &t_rc);
                    int cx = f_rc.left + (f_rc.right - f_rc.left) / 2;
                    int cy = f_rc.top + (f_rc.bottom - f_rc.top) / 2;
                    int tw = t_rc.right - t_rc.left;
                    int th = t_rc.bottom - t_rc.top;
                    SetWindowPos(hg_g_taskbox_wnd, HWND_TOPMOST, cx - tw / 2, cy - th / 2, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

                    refresh_window_list(FALSE);
                    ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                } else {
                    refresh_window_list(FALSE);
                }
                SetForegroundWindow(hg_g_taskbox_wnd);
                
                ShowWindow(hwnd, SW_HIDE); // Hide floater just like hover
                
                hg_g_hover_check_armed = FALSE;
                GetCursorPos(&hg_g_last_mouse_pos);
                SetTimer(hg_g_taskbox_wnd, HG_TIMER_HOVER_CHECK, 100, NULL);

                hg_g_taskbox_highlight_ticks = HG_HIGHLIGHT_TICKS;
                if (!SetTimer(hg_g_taskbox_wnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                    hg_g_taskbox_highlight_ticks = 0;
                }
                InvalidateRect(hg_g_taskbox_wnd, NULL, FALSE);

                SetFocus(hg_g_toolbar_wnd);
                reset_taskbox_focus();

                InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
            } else {
                SetForegroundWindow(hwnd);
            }
        }
        return 0;
    }
    case WM_COPYDATA:
        return handle_copydata_command_line((const COPYDATASTRUCT *)l_param);
    case WM_DESTROY:
        return floater_controller_on_destroy(hwnd);
    default: {
        if (hg_g_shellhook_msg && msg == hg_g_shellhook_msg) {
            if (w_param == HSHELL_REDRAW) {
                HWND target_hwnd = (HWND)l_param;
                BOOL found = FALSE;
                for (int i = 0; i < hg_g_window_count; i++) {
                    if (hg_g_window_items[i].hwnd == target_hwnd) {
                        release_window_item_icon(&hg_g_window_items[i]);
                        hg_g_window_items[i].icon =
                            get_window_icon(target_hwnd, ABS(hg_g_current_font_size), &hg_g_window_items[i].own_icon);
                        found = TRUE;
                        break;
                    }
                }
                if (found && hg_g_toolbar_wnd && hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
                }
            } else if (w_param == HSHELL_WINDOWCREATED || w_param == HSHELL_WINDOWDESTROYED ||
                       w_param == HSHELL_WINDOWREPLACED) {
                if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    refresh_window_list(FALSE);
                }
            }
            return 0;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
