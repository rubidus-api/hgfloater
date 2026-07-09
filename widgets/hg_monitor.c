#include "hg_monitor.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

static void close_monitor_window(int idx)
{
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return;
    }

    hg_g_monitors[idx].active = FALSE;
    if (hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        DestroyWindow(hg_g_monitors[idx].hwnd);
    }
    hg_g_monitors[idx].hwnd = NULL;
}

/* Persist preview geometry under the display device name so a re-enumeration
 * that reorders monitors keeps geometry attached to the right display. */
static void monitor_geometry_key(int idx, const WCHAR *suffix, WCHAR *out, size_t out_count)
{
    WCHAR sanitized[64];
    size_t si = 0;
    const WCHAR *name = hg_g_monitors[idx].name;

    for (size_t i = 0; name[i] && si + 1 < HG_ARRAYSIZE(sanitized); ++i) {
        if (iswalnum((wint_t)name[i])) {
            sanitized[si++] = name[i];
        }
    }
    sanitized[si] = L'\0';
    if (si == 0) {
        StringCchPrintfW(sanitized, HG_ARRAYSIZE(sanitized), L"monitor%d", idx + 1);
    }
    StringCchPrintfW(out, out_count, L"%ls_%ls", sanitized, suffix);
}

static BOOL open_monitor_window(int idx)
{
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return FALSE;
    }

    if (hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        ShowWindow(hg_g_monitors[idx].hwnd, SW_SHOWNORMAL);
        SetForegroundWindow(hg_g_monitors[idx].hwnd);
        return TRUE;
    }

    hg_g_monitors[idx].active = TRUE;

    int x, y, w, h;
    RECT m_rc = hg_g_monitors[idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;

    int def_w = 640;
    int def_h = (mw > 0) ? (def_w * mh / mw) : 480;

    WCHAR key_x[96], key_y[96], key_w[96], key_h[96];
    WCHAR legacy_key[64];
    monitor_geometry_key(idx, L"x", key_x, HG_ARRAYSIZE(key_x));
    monitor_geometry_key(idx, L"y", key_y, HG_ARRAYSIZE(key_y));
    monitor_geometry_key(idx, L"w", key_w, HG_ARRAYSIZE(key_w));
    monitor_geometry_key(idx, L"h", key_h, HG_ARRAYSIZE(key_h));

    /* Old index-based keys serve as the default so existing settings migrate. */
    StringCchPrintfW(legacy_key, HG_ARRAYSIZE(legacy_key), L"monitor%d_x", idx + 1);
    int legacy_x = (int)GetPrivateProfileIntW(L"monitor", legacy_key, 100, hg_g_config_path);
    StringCchPrintfW(legacy_key, HG_ARRAYSIZE(legacy_key), L"monitor%d_y", idx + 1);
    int legacy_y = (int)GetPrivateProfileIntW(L"monitor", legacy_key, 100, hg_g_config_path);
    StringCchPrintfW(legacy_key, HG_ARRAYSIZE(legacy_key), L"monitor%d_w", idx + 1);
    int legacy_w = (int)GetPrivateProfileIntW(L"monitor", legacy_key, def_w, hg_g_config_path);
    StringCchPrintfW(legacy_key, HG_ARRAYSIZE(legacy_key), L"monitor%d_h", idx + 1);
    int legacy_h = (int)GetPrivateProfileIntW(L"monitor", legacy_key, def_h, hg_g_config_path);

    x = (int)GetPrivateProfileIntW(L"monitor", key_x, legacy_x, hg_g_config_path);
    y = (int)GetPrivateProfileIntW(L"monitor", key_y, legacy_y, hg_g_config_path);
    w = (int)GetPrivateProfileIntW(L"monitor", key_w, legacy_w, hg_g_config_path);
    h = (int)GetPrivateProfileIntW(L"monitor", key_h, legacy_h, hg_g_config_path);

    HWND mwnd =
        CreateWindowExW(WS_EX_TOPMOST | WS_EX_NOACTIVATE, HG_CLASS_MONITOR, hg_g_monitors[idx].name,
                        WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN, x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (mwnd) {
        apply_dwm_attributes(mwnd);
        hg_g_monitors[idx].hwnd = mwnd;

        if (hg_g_tooltip_wnd) {
            TOOLINFOW ti = {0};
            ti.cbSize = TOOLINFO_V1_SIZE;
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = mwnd;
            ti.uId = (UINT_PTR)mwnd;
            ti.lpszText = hg_g_monitors[idx].name;
            SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        }
        return TRUE;
    }

    hg_g_monitors[idx].active = FALSE;
    return FALSE;
}

void toggle_monitor_window(int idx)
{
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return;
    }

    if (hg_g_monitors[idx].active && hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        close_monitor_window(idx);
    } else {
        open_monitor_window(idx);
    }
}

static LRESULT CALLBACK monitor_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR uid,
                                                   DWORD_PTR dw_ref)
{
    if (hg_readonly_edit_common(hwnd, msg, w_param)) {
        return 0;
    }
    switch (msg) {
    case WM_LBUTTONDOWN:
        SendMessageW(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
        return 0;
    case WM_RBUTTONDOWN:
        PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

static BOOL monitor_pt_to_screen_pt(HWND hwnd, int monitor_idx, POINT client_pt, POINT *out_screen_pt, BOOL allow_out)
{
    if (monitor_idx < 0 || monitor_idx >= hg_g_monitor_count)
        return FALSE;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int border = SC(HG_BORDER_THICKNESS);
    int edit_height = 0;
    HWND edit_wnd = GetDlgItem(hwnd, 104);
    if (edit_wnd) {
        RECT erc;
        GetWindowRect(edit_wnd, &erc);
        edit_height = erc.bottom - erc.top;
    }
    int preview_top = border + edit_height;
    int preview_left = border;
    int preview_right = rc.right - border;
    int preview_bottom = rc.bottom - border;

    if (!allow_out && (client_pt.x < preview_left || client_pt.x > preview_right || client_pt.y < preview_top ||
                       client_pt.y > preview_bottom)) {
        return FALSE;
    }

    int pw = preview_right - preview_left;
    int ph = preview_bottom - preview_top;
    if (pw <= 0 || ph <= 0)
        return FALSE;

    RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;

    double rx = (double)(client_pt.x - preview_left) / pw;
    double ry = (double)(client_pt.y - preview_top) / ph;

    out_screen_pt->x = m_rc.left + (int)(rx * mw);
    out_screen_pt->y = m_rc.top + (int)(ry * mh);
    return TRUE;
}

static BOOL screen_pt_to_monitor_pt(HWND hwnd, int monitor_idx, POINT screen_pt, POINT *out_client_pt)
{
    if (monitor_idx < 0 || monitor_idx >= hg_g_monitor_count)
        return FALSE;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int border = SC(HG_BORDER_THICKNESS);
    int edit_height = 0;
    HWND edit_wnd = GetDlgItem(hwnd, 104);
    if (edit_wnd) {
        RECT erc;
        GetWindowRect(edit_wnd, &erc);
        edit_height = erc.bottom - erc.top;
    }
    int preview_top = border + edit_height;
    int preview_left = border;
    int preview_right = rc.right - border;
    int preview_bottom = rc.bottom - border;

    int pw = preview_right - preview_left;
    int ph = preview_bottom - preview_top;
    if (pw <= 0 || ph <= 0)
        return FALSE;

    RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;
    if (mw <= 0 || mh <= 0)
        return FALSE;

    double rx = (double)(screen_pt.x - m_rc.left) / mw;
    double ry = (double)(screen_pt.y - m_rc.top) / mh;

    if (rx < 0.0 || rx > 1.0 || ry < 0.0 || ry > 1.0)
        return FALSE;

    out_client_pt->x = preview_left + (int)(rx * pw);
    out_client_pt->y = preview_top + (int)(ry * ph);
    return TRUE;
}

LRESULT CALLBACK monitor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    int monitor_idx = -1;
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hwnd == hwnd) {
            monitor_idx = i;
            break;
        }
    }

    switch (msg) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_CREATE: {
        LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)l_param;
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (!hg_g_main_font) {
            hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                         DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
            if (!hg_g_main_font)
                hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        HWND edit_wnd =
            CreateWindowExW(0, L"EDIT", pcs->lpszName, WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL | ES_READONLY,
                            0, 0, 0, 0, hwnd, (HMENU)104, GetModuleHandle(NULL), NULL);
        if (edit_wnd) {
            SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
            SetWindowSubclass(edit_wnd, monitor_edit_subclass_proc, 1, 0);
            disable_window_ime(edit_wnd);
        }
        SetTimer(hwnd, HG_TIMER_MONITOR_REFRESH, 100, NULL);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DPICHANGED:
        /* Scale ownership stays with the floater/taskbox pair; just take the size. */
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        return 0;
    case WM_TIMER: {
        if (w_param == HG_TIMER_MONITOR_REFRESH) {
            if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_SIZE: {
        HWND edit_wnd = GetDlgItem(hwnd, 104);
        if (!hg_g_main_font) {
            hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                         DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
            if (!hg_g_main_font)
                hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        if (edit_wnd && hg_g_main_font) {
            int border = SC(HG_BORDER_THICKNESS);
            int w = (int)LOWORD(l_param) - border * 2;

            int edit_height = hg_measure_edit_height(edit_wnd, hg_g_main_font);

            MoveWindow(edit_wnd, border, border, w, edit_height, TRUE);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        return hg_on_ctlcolor_edit((HDC)w_param);
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        if (rc.right > 0 && rc.bottom > 0) {
            HgPaintBuffer paint_buffer;
            if (!hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &paint_buffer)) {
                EndPaint(hwnd, &ps);
                return 0;
            }
            HDC mem_dc = paint_buffer.dc;

            int border = SC(HG_BORDER_THICKNESS);
            int edit_height = 0;
            HWND edit_wnd = GetDlgItem(hwnd, 104);
            if (edit_wnd) {
                RECT erc;
                GetWindowRect(edit_wnd, &erc);
                edit_height = erc.bottom - erc.top;
            }

            if (monitor_idx >= 0 && hg_g_monitors[monitor_idx].active) {
                RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
                int mw = m_rc.right - m_rc.left;
                int mh = m_rc.bottom - m_rc.top;

                HDC hdc_screen = GetDC(NULL);
                if (hdc_screen) {
                    SetStretchBltMode(mem_dc, HALFTONE);
                    /* Draw preview area below the edit box + border */
                    int preview_top = border + edit_height;
                    StretchBlt(mem_dc, border, preview_top, rc.right - 2 * border, rc.bottom - preview_top - border,
                               hdc_screen, m_rc.left, m_rc.top, mw, mh, SRCCOPY);
                    ReleaseDC(NULL, hdc_screen);
                }

                /* Draw cursor if needed */
                POINT cursor_pt;
                if (GetCursorPos(&cursor_pt)) {
                    POINT client_pt;
                    if (screen_pt_to_monitor_pt(hwnd, monitor_idx, cursor_pt, &client_pt)) {
                        COLORREF fill_color = RGB(255, 0, 0); /* red by default */
                        if (GetAsyncKeyState(VK_MBUTTON) < 0)
                            fill_color = RGB(255, 255, 0); /* yellow */
                        else if (GetAsyncKeyState(VK_RBUTTON) < 0)
                            fill_color = RGB(0, 0, 255); /* blue */
                        else if (GetAsyncKeyState(VK_LBUTTON) < 0)
                            fill_color = RGB(0, 255, 0); /* green */

                        int len = SC(8);
                        /* Draw black crosshair background for contrast */
                        HPEN hPenBg = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
                        HPEN restore_pen = NULL;
                        if (hPenBg) {
                            restore_pen = (HPEN)SelectObject(mem_dc, hPenBg);
                            MoveToEx(mem_dc, client_pt.x - len, client_pt.y, NULL);
                            LineTo(mem_dc, client_pt.x + len + 1, client_pt.y);
                            MoveToEx(mem_dc, client_pt.x, client_pt.y - len, NULL);
                            LineTo(mem_dc, client_pt.x, client_pt.y + len + 1);
                        }

                        /* Draw colored crosshair core */
                        HPEN hPenFg = CreatePen(PS_SOLID, 1, fill_color);
                        if (hPenFg) {
                            HPEN old_pen = (HPEN)SelectObject(mem_dc, hPenFg);
                            if (!restore_pen)
                                restore_pen = old_pen;
                            MoveToEx(mem_dc, client_pt.x - len, client_pt.y, NULL);
                            LineTo(mem_dc, client_pt.x + len + 1, client_pt.y);
                            MoveToEx(mem_dc, client_pt.x, client_pt.y - len, NULL);
                            LineTo(mem_dc, client_pt.x, client_pt.y + len + 1);
                        }

                        if (restore_pen)
                            SelectObject(mem_dc, restore_pen);
                        if (hPenBg)
                            DeleteObject(hPenBg);
                        if (hPenFg)
                            DeleteObject(hPenFg);
                    }
                }
            }

            /* Draw border like taskbox/floater */
            HWND fg = GetForegroundWindow();
            BOOL is_focused = (fg == hwnd || IsChild(hwnd, fg));
            COLORREF border_color = is_focused ? HG_COLOR_BORDER_SELECTED : HG_COLOR_BG_TOOLBAR;
            HBRUSH border_brush = CreateSolidBrush(border_color);
            if (border_brush) {
                RECT rc_top = {0, 0, rc.right, border};
                RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
                RECT rc_left = {0, border, border, rc.bottom - border};
                RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};
                RECT rc_edit_bg = {0, border, rc.right, border + edit_height};

                FillRect(mem_dc, &rc_top, border_brush);
                FillRect(mem_dc, &rc_bottom, border_brush);
                FillRect(mem_dc, &rc_left, border_brush);
                FillRect(mem_dc, &rc_right, border_brush);
                FillRect(mem_dc, &rc_edit_bg, border_brush);
                DeleteObject(border_brush);
            }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);

            hg_paint_buffer_end(&paint_buffer);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
        HMONITOR my_phys_mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor_idx >= 0 && my_phys_mon == hg_g_monitors[monitor_idx].hMonitor) {
            return 0;
        }

        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        POINT screen_pt;
        if (monitor_pt_to_screen_pt(hwnd, monitor_idx, pt, &screen_pt, FALSE)) {
            INPUT inputs[2] = {0};

            inputs[0].type = INPUT_MOUSE;
            if (msg == WM_LBUTTONDOWN)
                inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            else if (msg == WM_RBUTTONDOWN)
                inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            else if (msg == WM_MBUTTONDOWN)
                inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

            SetCursorPos(screen_pt.x, screen_pt.y);

            inputs[1].type = INPUT_MOUSE;
            if (msg == WM_LBUTTONDOWN)
                inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            else if (msg == WM_RBUTTONDOWN)
                inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            else if (msg == WM_MBUTTONDOWN)
                inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;

            SendInput(2, inputs, sizeof(INPUT));
        }
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        HMONITOR my_phys_mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor_idx >= 0 && my_phys_mon == hg_g_monitors[monitor_idx].hMonitor) {
            return 0;
        }

        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        POINT screen_pt;
        if (monitor_pt_to_screen_pt(hwnd, monitor_idx, pt, &screen_pt, FALSE)) {
            INPUT inputs[1] = {0};
            inputs[0].type = INPUT_MOUSE;
            if (msg == WM_LBUTTONUP)
                inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            else if (msg == WM_RBUTTONUP)
                inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            else if (msg == WM_MBUTTONUP)
                inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

            SetCursorPos(screen_pt.x, screen_pt.y);
            SendInput(1, inputs, sizeof(INPUT));
        }
        return 0;
    }
    case WM_CLOSE: {
        if (monitor_idx >= 0) {
            close_monitor_window(monitor_idx);
        } else {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_NCHITTEST: {
        POINT pt;
        pt.x = GET_X_LPARAM(l_param);
        pt.y = GET_Y_LPARAM(l_param);
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int border = SC(8);
        if (pt.x < rc.left + border && pt.y < rc.top + border)
            return HTTOPLEFT;
        if (pt.x >= rc.right - border && pt.y < rc.top + border)
            return HTTOPRIGHT;
        if (pt.x < rc.left + border && pt.y >= rc.bottom - border)
            return HTBOTTOMLEFT;
        if (pt.x >= rc.right - border && pt.y >= rc.bottom - border)
            return HTBOTTOMRIGHT;
        if (pt.x < rc.left + border)
            return HTLEFT;
        if (pt.x >= rc.right - border)
            return HTRIGHT;
        if (pt.y < rc.top + border)
            return HTTOP;
        if (pt.y >= rc.bottom - border)
            return HTBOTTOM;

        return HTCLIENT;
    }
    case WM_SIZING: {
        /* Force Aspect Ratio */
        if (monitor_idx >= 0) {
            RECT *prc = (RECT *)l_param;
            RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
            int mw = m_rc.right - m_rc.left;
            int mh = m_rc.bottom - m_rc.top;
            if (mh > 0 && mw > 0) {
                int w = prc->right - prc->left;
                int h = prc->bottom - prc->top;

                /* Depending on sizing edge, adjust width or height */
                switch (w_param) {
                case WMSZ_LEFT:
                case WMSZ_RIGHT:
                    prc->bottom = prc->top + (w * mh) / mw;
                    break;
                case WMSZ_TOP:
                case WMSZ_BOTTOM:
                    prc->right = prc->left + (h * mw) / mh;
                    break;
                case WMSZ_TOPLEFT:
                case WMSZ_TOPRIGHT:
                case WMSZ_BOTTOMLEFT:
                case WMSZ_BOTTOMRIGHT:
                    /* just adjust bottom to keep ratio with width */
                    prc->bottom = prc->top + (w * mh) / mw;
                    break;
                }
            }
            return TRUE;
        }
        break;
    }
    case WM_EXITSIZEMOVE: {
        if (monitor_idx >= 0) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            WCHAR key_x[96], key_y[96], key_w[96], key_h[96];
            monitor_geometry_key(monitor_idx, L"x", key_x, HG_ARRAYSIZE(key_x));
            monitor_geometry_key(monitor_idx, L"y", key_y, HG_ARRAYSIZE(key_y));
            monitor_geometry_key(monitor_idx, L"w", key_w, HG_ARRAYSIZE(key_w));
            monitor_geometry_key(monitor_idx, L"h", key_h, HG_ARRAYSIZE(key_h));

            WCHAR buf[32];
            hellgates_wsprintf(buf, 32, L"%d", rc.left);
            WritePrivateProfileStringW(L"monitor", key_x, buf, hg_g_config_path);
            hellgates_wsprintf(buf, 32, L"%d", rc.top);
            WritePrivateProfileStringW(L"monitor", key_y, buf, hg_g_config_path);
            hellgates_wsprintf(buf, 32, L"%d", rc.right - rc.left);
            WritePrivateProfileStringW(L"monitor", key_w, buf, hg_g_config_path);
            hellgates_wsprintf(buf, 32, L"%d", rc.bottom - rc.top);
            WritePrivateProfileStringW(L"monitor", key_h, buf, hg_g_config_path);
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
