#include "hg_about.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

static void close_about_window(HWND hwnd)
{
    if (hg_g_about_wnd == hwnd) {
        hg_g_about_wnd = NULL;
    }
    if (hwnd && IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
}


void show_about_window(void)
{
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        ShowWindow(hg_g_about_wnd, SW_SHOWNORMAL);
        SetForegroundWindow(hg_g_about_wnd);
        return;
    }

    hg_g_about_wnd = CreateWindowExW(0, HG_CLASS_ABOUT, L"about hgfloater", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                     CW_USEDEFAULT, SC(400), SC(300), NULL, NULL, GetModuleHandle(NULL), NULL);
    if (hg_g_about_wnd) {
        ShowWindow(hg_g_about_wnd, SW_SHOW);
    }
}

static LRESULT CALLBACK about_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                                 UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (msg == WM_SETFOCUS) {
        disable_window_ime(hwnd);
    }
    if (readonly_edit_handle_ime_messages(hwnd, msg, w_param)) {
        return 0;
    }
    if (msg == WM_KEYDOWN && w_param == VK_ESCAPE) {
        PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK about_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE: {
        apply_dwm_attributes(hwnd);
        HWND edit_wnd =
            CreateWindowExW(0, L"EDIT", HG_ABOUT_TEXT_W,
                            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                            SC(10), SC(10), SC(364), SC(240), hwnd, (HMENU)100, GetModuleHandle(NULL), NULL);
        if (edit_wnd) {
            SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
            SetWindowSubclass(edit_wnd, about_edit_subclass_proc, 1, 0);
            disable_window_ime(edit_wnd);
        }
        return 0;
    }
    case WM_SIZE: {
        HWND edit_wnd = GetDlgItem(hwnd, 100);
        if (edit_wnd) {
            int w = (int)LOWORD(l_param) - SC(20);
            int h = (int)HIWORD(l_param) - SC(20);
            if (w < 0)
                w = 0;
            if (h < 0)
                h = 0;
            MoveWindow(edit_wnd, SC(10), SC(10), w, h, TRUE);
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)w_param;
        SetTextColor(hdc, hg_g_color_scheme_selected.text);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, hg_g_color_scheme_selected.bg);
        return (LRESULT)GetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND);
    }
    case WM_KEYDOWN: {
        if (w_param == VK_ESCAPE) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        return 0;
    }
    case WM_CLOSE: {
        close_about_window(hwnd);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

