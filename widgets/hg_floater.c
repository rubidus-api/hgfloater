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
        if (hg_g_floater_time_font) {
            DeleteObject(hg_g_floater_time_font);
            hg_g_floater_time_font = NULL;
        }
        if (hg_g_floater_date_font) {
            DeleteObject(hg_g_floater_date_font);
            hg_g_floater_date_font = NULL;
        }
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
    hg_g_floater_alpha = (BYTE)new_alpha;
    SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
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
    SetTimer(hwnd, 1, 1000, NULL);
    return 0;
}

static LRESULT floater_controller_on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP mem_bm = NULL, old_bm = NULL;

        if (mem_dc) {
            mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            if (mem_bm) {
                old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
                if (!old_bm) {
                    DeleteObject(mem_bm);
                    DeleteDC(mem_dc);
                    EndPaint(hwnd, &ps);
                    return 0;
                }

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
                SelectObject(mem_dc, old_bm);
                DeleteObject(mem_bm);
            }
            DeleteDC(mem_dc);
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

static LRESULT floater_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    if (LOWORD(w_param) == HG_IDM_CLOSE_APP) {
        DestroyWindow(hwnd);
    } else if (LOWORD(w_param) >= HG_IDM_MONITOR_BASE && LOWORD(w_param) < HG_IDM_MONITOR_BASE + HG_MAX_MONITORS) {
        int idx = LOWORD(w_param) - HG_IDM_MONITOR_BASE;
        if (idx >= 0 && idx < hg_g_monitor_count) {
            toggle_monitor_window(idx);
        }
    } else if (LOWORD(w_param) >= HG_IDM_AUDIO_DEVICE_BASE &&
               LOWORD(w_param) < HG_IDM_AUDIO_DEVICE_BASE + HG_MAX_AUDIO_DEVICES) {
        int idx = LOWORD(w_param) - HG_IDM_AUDIO_DEVICE_BASE;
        if (idx >= 0 && idx < hg_g_audio_device_count) {
            if (set_default_audio_device(hg_g_audio_devices[idx].id)) {
                update_audio_device_list();
            }
        }
    } else if (LOWORD(w_param) == HG_IDM_MUTE) {
        set_system_mute(!get_system_mute());
        update_toolbar_tooltips(hg_g_toolbar_wnd);
        if (hg_g_toolbar_wnd) {
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
        }
    } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_0) {
        set_system_volume(0);
    } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_25) {
        set_system_volume(25);
    } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_50) {
        set_system_volume(50);
    } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_75) {
        set_system_volume(75);
    } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_100) {
        set_system_volume(100);
    } else if (LOWORD(w_param) == HG_IDM_OPEN_SHORTCUTS) {
        ShellExecuteW(NULL, L"open", hg_g_shortcuts_path, NULL, NULL, SW_SHOWNORMAL);
    } else if (LOWORD(w_param) == HG_IDM_EDIT_CONFIG) {
        ShellExecuteW(NULL, L"open", L"notepad.exe", hg_g_config_path, NULL, SW_SHOWNORMAL);
    } else if (LOWORD(w_param) == HG_IDM_ABOUT) {
        show_about_window();
    } else if (LOWORD(w_param) == HG_IDM_RESET_ALL) {
        hg_config_reset_all(hwnd);
    } else if (LOWORD(w_param) == HG_IDM_FONT_UP) {
        update_floater_font_size(1);
    } else if (LOWORD(w_param) == HG_IDM_FONT_DOWN) {
        update_floater_font_size(-1);
    } else if (LOWORD(w_param) == HG_IDM_POWER_OFF) {
        HWND h_shell = FindWindowW(L"Shell_TrayWnd", NULL);
        if (h_shell)
            PostMessageW(h_shell, WM_COMMAND, 506, 0);
    }

    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}

static LRESULT floater_controller_on_destroy(HWND hwnd)
{
    DeregisterShellHookWindow(hwnd);
    KillTimer(hwnd, 1);
    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        DestroyWindow(hg_g_about_wnd);
        hg_g_about_wnd = NULL;
    }
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        DestroyWindow(hg_g_taskbox_wnd);
        hg_g_taskbox_wnd = NULL;
    }
    if (hg_g_hotkey_registered) {
        UnregisterHotKey(hwnd, 1);
        hg_g_hotkey_registered = FALSE;
    }
    if (hg_g_floater_time_font) {
        DeleteObject(hg_g_floater_time_font);
        hg_g_floater_time_font = NULL;
    }
    if (hg_g_floater_date_font) {
        DeleteObject(hg_g_floater_date_font);
        hg_g_floater_date_font = NULL;
    }
    hg_g_floater_wnd = NULL;
    PostQuitMessage(0);
    return 0;
}

LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static int initial_floater_font_size = 0;
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
                int delta = (pt.x - hg_g_drag_start_pt.x) / 5;
                int new_size = initial_floater_font_size + delta;
                if (new_size < HG_FLOATER_MIN_FONT_SIZE)
                    new_size = HG_FLOATER_MIN_FONT_SIZE;
                if (new_size > HG_FLOATER_MAX_FONT_SIZE)
                    new_size = HG_FLOATER_MAX_FONT_SIZE;
                if (hg_g_floater_font_size != new_size) {
                    hg_g_floater_font_size = new_size;
                    if (hg_g_floater_time_font) {
                        DeleteObject(hg_g_floater_time_font);
                        hg_g_floater_time_font = NULL;
                    }
                    if (hg_g_floater_date_font) {
                        DeleteObject(hg_g_floater_date_font);
                        hg_g_floater_date_font = NULL;
                    }
                    update_floater_layout(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    save_floater_font_config();
                }
            }
        } else {
            // Hover logic
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd) && !IsWindowVisible(hg_g_taskbox_wnd)) {
                RECT rc;
                GetWindowRect(hwnd, &rc);
                ShowWindow(hwnd, SW_HIDE);
                SetWindowPos(hg_g_taskbox_wnd, HWND_TOPMOST, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
                // Make it appear instantly, refresh without forcing icon reload
                refresh_window_list(FALSE);
                ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                SetForegroundWindow(hg_g_taskbox_wnd);
                hg_g_hover_check_armed = TRUE;
                SetTimer(hg_g_taskbox_wnd, HG_TIMER_HOVER_CHECK, 100, NULL);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hwnd) {
            ReleaseCapture();
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
                if (IsWindowVisible(hg_g_taskbox_wnd)) {
                    hide_taskbox(hg_g_taskbox_wnd);
                } else {
                    PostMessageW(hwnd, WM_HOTKEY, 1, 0);
                }
            }
        }
        return 0;
    }
    case WM_RBUTTONUP: {
        update_audio_device_list();
        POINT pt;
        GetCursorPos(&pt);
        HMENU menu = CreatePopupMenu();
        if (menu) {
            AppendMenuW(menu, MF_STRING, HG_IDM_OPEN_SHORTCUTS, L"Open Shortcuts Folder");
            AppendMenuW(menu, MF_STRING, HG_IDM_EDIT_CONFIG, L"Edit Configuration");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, HG_IDM_ABOUT, L"About...");
            AppendMenuW(menu, MF_STRING, HG_IDM_RESET_ALL, L"Reset Settings");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);

            /* Audio Endpoint Devices List sub-menu */
            HMENU audio_menu = CreatePopupMenu();
            if (audio_menu) {
                for (int i = 0; i < hg_g_audio_device_count; i++) {
                    UINT flags = MF_STRING;
                    if (hg_g_audio_devices[i].is_default)
                        flags |= MF_CHECKED;
                    AppendMenuW(audio_menu, flags, (UINT_PTR)(HG_IDM_AUDIO_DEVICE_BASE + (UINT)i),
                                hg_g_audio_devices[i].name);
                }
                if (hg_g_audio_device_count > 0) {
                    AppendMenuW(audio_menu, MF_SEPARATOR, 0, NULL);
                }
                UINT mute_flags = MF_STRING;
                if (get_system_mute()) {
                    mute_flags |= MF_CHECKED;
                }
                AppendMenuW(audio_menu, mute_flags, HG_IDM_MUTE, L"Mute");

                AppendMenuW(menu, MF_POPUP, (UINT_PTR)audio_menu, L"Select Audio Device");
            }

            /* Physical Monitors layout options */
            HMENU monitor_menu = CreatePopupMenu();
            if (monitor_menu) {
                for (int i = 0; i < hg_g_monitor_count; i++) {
                    UINT flags = MF_STRING;
                    if (hg_g_monitors[i].active)
                        flags |= MF_CHECKED;
                    AppendMenuW(monitor_menu, flags, (UINT_PTR)(HG_IDM_MONITOR_BASE + (UINT)i), hg_g_monitors[i].name);
                }
                AppendMenuW(menu, MF_POPUP, (UINT_PTR)monitor_menu, L"Arrange Monitors");
            }

            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, HG_IDM_POWER_OFF, L"Lock Screen (Power Off)");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, HG_IDM_CLOSE_APP, L"Exit");

            SetForegroundWindow(hwnd);
            hg_g_menu_active = TRUE;
            TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
            hg_g_menu_active = FALSE;
            DestroyMenu(menu);
        }
        return 0;
    }
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
        save_config(L"floater", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }
    case WM_TIMER: {
        if (w_param == 1) {
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
                    refresh_window_list(FALSE);
                    ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                } else {
                    refresh_window_list(FALSE);
                }
                SetWindowPos(hg_g_taskbox_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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

                hg_g_focus_area = 0;
                SetFocus(hg_g_toolbar_wnd);
                if (hg_g_window_count > 0) {
                    hg_g_toolbar_focus_index = 0;
                }
                update_focus_message(-2, -2);

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
                        if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
                            DestroyIcon(hg_g_window_items[i].icon);
                        }
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
