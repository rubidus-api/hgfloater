#include "hg_commandbox.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

void commandbox_execute(void);

LRESULT CALLBACK commandbox_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                               UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;
    HWND parent = GetParent(hwnd);
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
        BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);
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
            SendMessageW(parent, msg, w_param, l_param);
            return 0;
        }
        if (w_param == VK_RETURN && is_ctrl) {
            commandbox_execute();
            return 0;
        }
    }
    else if (msg == WM_MOUSEWHEEL) {
        SendMessageW(parent, msg, w_param, l_param);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}


void load_commandbox_font()
{
    GetPrivateProfileStringW(L"commandbox", L"font_name", L"", hg_g_commandbox_font_name, LF_FACESIZE, hg_g_config_path);
    int size = (int)GetPrivateProfileIntW(L"commandbox", L"font_size", 16, hg_g_config_path);
    hg_g_commandbox_font_size = -SC(size);

    if (hg_g_commandbox_font) {
        DeleteObject(hg_g_commandbox_font);
        hg_g_commandbox_font = NULL;
    }

    if (hg_g_commandbox_font_name[0] == L'\0') {
        StringCchCopyW(hg_g_commandbox_font_name, LF_FACESIZE, L"Consolas");
    }

    hg_g_commandbox_font = CreateFontW(
        hg_g_commandbox_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, hg_g_commandbox_font_name
    );
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

    int out_len = GetWindowTextLengthW(hg_g_commandbox_out_wnd);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SETSEL, (WPARAM)out_len, (LPARAM)out_len);
    if (out_len > 0) {
        SendMessageW(hg_g_commandbox_out_wnd, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    }
    SendMessageW(hg_g_commandbox_out_wnd, EM_REPLACESEL, FALSE, (LPARAM)in_buf);
    
    int new_out_len = GetWindowTextLengthW(hg_g_commandbox_out_wnd);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SETSEL, (WPARAM)new_out_len, (LPARAM)new_out_len);
    SendMessageW(hg_g_commandbox_out_wnd, EM_SCROLLCARET, 0, 0);

    SetWindowTextW(hg_g_commandbox_in_wnd, L"");
    SetFocus(hg_g_commandbox_in_wnd);
    free(in_buf);
}

void show_commandbox_window()
{
    if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd)) {
        if (IsWindowVisible(hg_g_commandbox_wnd)) {
            ShowWindow(hg_g_commandbox_wnd, SW_HIDE);
        } else {
            ShowWindow(hg_g_commandbox_wnd, SW_SHOWNORMAL);
            SetForegroundWindow(hg_g_commandbox_wnd);
        }
        return;
    }

    int x, y, w, h;
    load_config(L"commandbox", &x, &y, &w, &h, 100, 100, SC(400), SC(300));
    if (w < SC(200)) w = SC(200);

    int border = SC(8);
    int btn_h = SC(26);
    int line_h = SC(16);
    load_commandbox_font();
    if (hg_g_commandbox_font) {
        HWND dummy = CreateWindowExW(0, L"STATIC", NULL, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (dummy) {
            HDC hdc = GetDC(dummy);
            HFONT old_font = SelectObject(hdc, hg_g_commandbox_font);
            TEXTMETRIC tm;
            if (GetTextMetrics(hdc, &tm)) {
                line_h = tm.tmHeight + tm.tmExternalLeading;
            }
            SelectObject(hdc, old_font);
            ReleaseDC(dummy, hdc);
            DestroyWindow(dummy);
        }
    }
    int min_out_h = line_h * 3 + SC(6);
    int min_in_h = line_h * 1 + SC(6);
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
        SetLayeredWindowAttributes(hg_g_commandbox_wnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
        
        hg_g_commandbox_out_wnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY | WS_VSCROLL,
            0, 0, 0, 0, hg_g_commandbox_wnd, (HMENU)101, GetModuleHandle(NULL), NULL
        );

        hg_g_commandbox_in_wnd = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL,
            0, 0, 0, 0, hg_g_commandbox_wnd, (HMENU)102, GetModuleHandle(NULL), NULL
        );

        hg_g_commandbox_btn_wnd = CreateWindowExW(
            0, L"BUTTON", L"Execute (Ctrl+Enter)",
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

        ShowWindow(hg_g_commandbox_wnd, SW_SHOWNORMAL);
        SetForegroundWindow(hg_g_commandbox_wnd);
    }
}

LRESULT CALLBACK commandbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE:
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
        
        int border = SC(8);
        int btn_h = SC(26);
        int cx_inner = cx - border * 2;

        int line_h = SC(16);
        if (hg_g_commandbox_font) {
            HDC hdc = GetDC(hwnd);
            HFONT old_font = SelectObject(hdc, hg_g_commandbox_font);
            TEXTMETRIC tm;
            if (GetTextMetrics(hdc, &tm)) {
                line_h = tm.tmHeight + tm.tmExternalLeading;
            }
            SelectObject(hdc, old_font);
            ReleaseDC(hwnd, hdc);
        }

        int min_out_h = line_h * 3 + SC(6);
        int min_in_h = line_h * 1 + SC(6);

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
        
        int border = SC(8);
        int btn_h = SC(26);
        int line_h = SC(16);
        if (hg_g_commandbox_font) {
            HDC hdc = GetDC(hwnd);
            HFONT old_font = SelectObject(hdc, hg_g_commandbox_font);
            TEXTMETRIC tm;
            if (GetTextMetrics(hdc, &tm)) {
                line_h = tm.tmHeight + tm.tmExternalLeading;
            }
            SelectObject(hdc, old_font);
            ReleaseDC(hwnd, hdc);
        }

        int min_out_h = line_h * 3 + SC(6);
        int min_in_h = line_h * 1 + SC(6);
        int min_cy = border * 4 + min_out_h + min_in_h + btn_h;

        RECT rc_win = {0, 0, SC(200), min_cy};
        AdjustWindowRectEx(&rc_win, WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, FALSE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);

        mmi->ptMinTrackSize.x = rc_win.right - rc_win.left;
        mmi->ptMinTrackSize.y = rc_win.bottom - rc_win.top;
        return 0;
    }

    case WM_MOVE:
    case WM_WINDOWPOSCHANGED: {
        if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            RECT rc;
            if (GetWindowRect(hwnd, &rc)) {
                save_commandbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            }
        }
        break;
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
            int move_step = SC(20);
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
                return 0;
            }
        }
        if (is_ctrl) {
            int dw = 0, dh = 0;
            int resize_step = SC(20);
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
        if (w_param == VK_ESCAPE) {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        short delta = (short)HIWORD(w_param);
        if (LOWORD(w_param) & MK_CONTROL) {
            int size = (int)(ABS(hg_g_commandbox_font_size) / (hg_g_scale_factor > 0 ? hg_g_scale_factor : 1.0) + 0.5);
            size += (delta > 0 ? 1 : -1);
            if (size < 8) size = 8;
            if (size > 72) size = 72;
            
            WCHAR buf[32];
            hellgates_wsprintf(buf, 32, L"%d", size);
            WritePrivateProfileStringW(L"commandbox", L"font_size", buf, hg_g_config_path);
            
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
            int new_alpha = (int)hg_g_commandbox_alpha + (delta > 0 ? 15 : -15);
            if (new_alpha > HG_MAX_ALPHA) new_alpha = HG_MAX_ALPHA;
            if (new_alpha < HG_MIN_ALPHA) new_alpha = HG_MIN_ALPHA;
            if (hg_g_commandbox_alpha == (BYTE)new_alpha)
                return 0;
            hg_g_commandbox_alpha = (BYTE)new_alpha;
            SetLayeredWindowAttributes(hwnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
            save_commandbox_alpha_config();
            return 0;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc_static = (HDC)w_param;
        SetTextColor(hdc_static, hg_g_color_scheme_selected.text);
        SetBkMode(hdc_static, OPAQUE);
        SetBkColor(hdc_static, hg_g_color_scheme_selected.bg);
        if (!hg_g_edit_bg_brush)
            hg_g_edit_bg_brush = CreateSolidBrush(hg_g_color_scheme_selected.bg);
        return hg_g_edit_bg_brush ? (LRESULT)hg_g_edit_bg_brush : (LRESULT)GetStockObject(BLACK_BRUSH);
    }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
