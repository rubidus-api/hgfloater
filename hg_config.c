#include "hg_config.h"
#include "hg_utils.h"

void load_config(const WCHAR *section, int *x, int *y, int *w, int *h, int def_x, int def_y, int def_w, int def_h)
{
    WCHAR buf[32] = {0};
    WCHAR def_buf[32] = {0};
    WCHAR *endptr;
    hellgates_wsprintf(def_buf, 32, L"%d", def_x);
    GetPrivateProfileStringW(section, L"x", def_buf, buf, 32, hg_g_config_path);
    *x = (int)wcstol(buf, &endptr, 10);
    if (endptr == buf)
        *x = def_x;

    hellgates_wsprintf(def_buf, 32, L"%d", def_y);
    GetPrivateProfileStringW(section, L"y", def_buf, buf, 32, hg_g_config_path);
    *y = (int)wcstol(buf, &endptr, 10);
    if (endptr == buf)
        *y = def_y;

    UINT uw = GetPrivateProfileIntW(section, L"w", def_w, hg_g_config_path);
    *w = (uw == 0 || uw > 30000) ? def_w : (int)uw;

    UINT uh = GetPrivateProfileIntW(section, L"h", def_h, hg_g_config_path);
    *h = (uh == 0 || uh > 30000) ? def_h : (int)uh;

    save_config(section, *x, *y, *w, *h);
}

void save_config(const WCHAR *section, int x, int y, int w, int h)
{
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", x);
    WritePrivateProfileStringW(section, L"x", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", y);
    WritePrivateProfileStringW(section, L"y", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", w);
    WritePrivateProfileStringW(section, L"w", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", h);
    WritePrivateProfileStringW(section, L"h", buf, hg_g_config_path);
}

void load_floater_font_config()
{
    UINT fs = GetPrivateProfileIntW(L"floater", L"font_size", 28, hg_g_config_path);
    if (fs < HG_FLOATER_MIN_FONT_SIZE)
        fs = HG_FLOATER_MIN_FONT_SIZE;
    if (fs > HG_FLOATER_MAX_FONT_SIZE)
        fs = HG_FLOATER_MAX_FONT_SIZE;
    hg_g_floater_font_size = (int)fs;

    save_floater_font_config();
}

void save_floater_font_config()
{
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", hg_g_floater_font_size);
    WritePrivateProfileStringW(L"floater", L"font_size", buf, hg_g_config_path);
}

void load_taskbox_font_config()
{
    UINT fs = GetPrivateProfileIntW(L"taskbox", L"font_size", 16, hg_g_config_path);
    if (fs < 12)
        fs = 12;
    if (fs > 128)
        fs = 128;
    hg_g_edit_font_size = -SC((int)fs);

    UINT is = GetPrivateProfileIntW(L"taskbox", L"icon_size", 22, hg_g_config_path);
    if (is < 16)
        is = 16;
    if (is > 128)
        is = 128;
    hg_g_current_font_size = -SC((int)is);

    save_taskbox_font_config();
}

void save_taskbox_font_config()
{
    WCHAR buf[32];
    int unscaled = (int)(ABS(hg_g_edit_font_size) / (hg_g_scale_factor > 0 ? hg_g_scale_factor : 1.0) + 0.5);
    hellgates_wsprintf(buf, 32, L"%d", unscaled);
    WritePrivateProfileStringW(L"taskbox", L"font_size", buf, hg_g_config_path);

    int unscaled_icon = (int)(ABS(hg_g_current_font_size) / (hg_g_scale_factor > 0 ? hg_g_scale_factor : 1.0) + 0.5);
    hellgates_wsprintf(buf, 32, L"%d", unscaled_icon);
    WritePrivateProfileStringW(L"taskbox", L"icon_size", buf, hg_g_config_path);
}

void save_hotkey_config()
{
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%u", hg_g_hotkey_modifiers);
    WritePrivateProfileStringW(L"hotkeys", L"global_focus_modifiers", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%u", hg_g_hotkey_key);
    WritePrivateProfileStringW(L"hotkeys", L"global_focus_key", buf, hg_g_config_path);
}

int get_alpha_config(const WCHAR *section, int def_alpha)
{
    int a = (int)GetPrivateProfileIntW(section, L"alpha", def_alpha, hg_g_config_path);
    if (a < 128)
        a = 128;
    if (a > 255)
        a = 255;

    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", a);
    WritePrivateProfileStringW(section, L"alpha", buf, hg_g_config_path);

    return a;
}

void load_font_name_config()
{
    GetPrivateProfileStringW(L"etc", L"font_name", L"Segoe UI", hg_g_font_name, 64, hg_g_config_path);
    if (wcslen(hg_g_font_name) == 0) {
        wcscpy(hg_g_font_name, L"Segoe UI");
    }
    WritePrivateProfileStringW(L"etc", L"font_name", hg_g_font_name, hg_g_config_path);
}

void load_hotkey_config()
{
    BOOL needs_save = FALSE;

    hg_g_hotkey_modifiers = GetPrivateProfileIntW(L"hotkeys", L"global_focus_modifiers", 0, hg_g_config_path);
    if (hg_g_hotkey_modifiers == 0) {
        hg_g_hotkey_modifiers = MOD_WIN | MOD_ALT;
        needs_save = TRUE;
    }

    hg_g_hotkey_key = GetPrivateProfileIntW(L"hotkeys", L"global_focus_key", 0, hg_g_config_path);
    if (hg_g_hotkey_key == 0) {
        hg_g_hotkey_key = VK_SPACE;
        needs_save = TRUE;
    }

    if (needs_save) {
        save_hotkey_config();
    }
}

void save_alpha_config()
{
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%u", (UINT)hg_g_floater_alpha);
    WritePrivateProfileStringW(L"floater", L"alpha", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%u", (UINT)hg_g_taskbox_alpha);
    WritePrivateProfileStringW(L"taskbox", L"alpha", buf, hg_g_config_path);
}

void hg_config_reset_all(HWND hwnd)
{
    (void)hwnd;
    hg_g_floater_alpha = 204;
    hg_g_taskbox_alpha = 204;
    hg_g_floater_font_size = 28;
    int target_icon_size = SC(22);
    hg_g_current_font_size = -target_icon_size;
    hg_g_edit_font_size = -SC(16);
    wcscpy(hg_g_font_name, L"Segoe UI");
    hg_g_hotkey_modifiers = MOD_WIN | MOD_ALT;
    hg_g_hotkey_key = VK_SPACE;

    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        if (hg_g_hotkey_registered) {
            UnregisterHotKey(hg_g_floater_wnd, 1);
            hg_g_hotkey_registered = FALSE;
        }
        hg_g_hotkey_registered =
            RegisterHotKey(hg_g_floater_wnd, 1, hg_g_hotkey_modifiers | MOD_NOREPEAT, hg_g_hotkey_key);
    }

    save_alpha_config();
    save_floater_font_config();
    save_taskbox_font_config();
    save_hotkey_config();
    WritePrivateProfileStringW(L"etc", L"font_name", hg_g_font_name, hg_g_config_path);
    save_config(L"floater", 100, 100, SC(80), SC(55));
    save_config(L"taskbox", 200, 200, SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT));

    hg_g_commandbox_alpha = 204;
    hg_g_commandbox_font_size = -SC(16);
    wcscpy(hg_g_commandbox_font_name, L"Consolas");
    WritePrivateProfileStringW(L"commandbox", L"font_name", hg_g_commandbox_font_name, hg_g_config_path);
    WCHAR buf_fs[32], buf_al[32];
    hellgates_wsprintf(buf_fs, 32, L"16");
    WritePrivateProfileStringW(L"commandbox", L"font_size", buf_fs, hg_g_config_path);
    hellgates_wsprintf(buf_al, 32, L"204");
    WritePrivateProfileStringW(L"commandbox", L"alpha", buf_al, hg_g_config_path);
    save_config(L"commandbox", 100, 100, SC(400), SC(300));

    if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd)) {
        SetLayeredWindowAttributes(hg_g_commandbox_wnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
        SetWindowPos(hg_g_commandbox_wnd, NULL, 100, 100, SC(400), SC(300), SWP_NOZORDER | SWP_NOACTIVATE);
        load_commandbox_font();
        if (hg_g_commandbox_out_wnd)
            SendMessageW(hg_g_commandbox_out_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
        if (hg_g_commandbox_in_wnd)
            SendMessageW(hg_g_commandbox_in_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
        if (hg_g_commandbox_btn_wnd)
            SendMessageW(hg_g_commandbox_btn_wnd, WM_SETFONT, (WPARAM)hg_g_commandbox_font, TRUE);
        RECT rc_client;
        GetClientRect(hg_g_commandbox_wnd, &rc_client);
        SendMessageW(hg_g_commandbox_wnd, WM_SIZE, 0, MAKELPARAM(rc_client.right, rc_client.bottom));
    }

    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
        if (hg_g_floater_time_font) {
            DeleteObject(hg_g_floater_time_font);
            hg_g_floater_time_font = NULL;
        }
        if (hg_g_floater_date_font) {
            DeleteObject(hg_g_floater_date_font);
            hg_g_floater_date_font = NULL;
        }
        SetWindowPos(hg_g_floater_wnd, NULL, 100, 100, SC(80), SC(55), SWP_NOZORDER | SWP_NOACTIVATE);
        update_floater_layout(hg_g_floater_wnd);
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);

        if (hg_g_toolbar_btn_font) {
            DeleteObject(hg_g_toolbar_btn_font);
            hg_g_toolbar_btn_font = NULL;
        }
        if (hg_g_main_font) {
            DeleteObject(hg_g_main_font);
            hg_g_main_font = NULL;
        }

        SetWindowPos(hg_g_taskbox_wnd, NULL, 200, 200, SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT),
                     SWP_NOZORDER | SWP_NOACTIVATE);
        update_layout(hg_g_taskbox_wnd);

        if (hg_g_edit_msg_wnd) {
            hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                         DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
            if (!hg_g_main_font)
                hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        }

        if (hg_g_tooltip_wnd && hg_g_main_font) {
            SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        }

        RECT rc = {0};
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        if (tb_width <= 0)
            tb_width = 1;
        int cols = get_items_per_row(tb_width, target_icon_size);
        if (cols <= 0)
            cols = 1;
        int exact_tb_width = (cols - 1) * (target_icon_size + SC(15)) + target_icon_size + SC(20);
        int new_w = exact_tb_width + border * 2;

        if ((rc.right - rc.left) != new_w) {
            SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            GetWindowRect(hg_g_taskbox_wnd, &rc);
            save_config(L"taskbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        }

        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }

    update_monitor_enum();
    int mx = SC(300), my = SC(300);
    for (int i = 0; i < hg_g_monitor_count; i++) {
        if (hg_g_monitors[i].active && hg_g_monitors[i].hwnd) {
            int m_w = SC(640);
            int mmw = hg_g_monitors[i].rcMonitor.right - hg_g_monitors[i].rcMonitor.left;
            int mmh = hg_g_monitors[i].rcMonitor.bottom - hg_g_monitors[i].rcMonitor.top;
            int m_h = (mmw > 0) ? (m_w * mmh / mmw) : SC(480);

            SetWindowPos(hg_g_monitors[i].hwnd, NULL, mx, my, m_w, m_h, SWP_NOZORDER | SWP_NOACTIVATE);

            WCHAR key_x[64], key_y[64], key_w[64], key_h[64], key_name[64];
            StringCchPrintfW(key_x, 64, L"monitor%d_x", i + 1);
            StringCchPrintfW(key_y, 64, L"monitor%d_y", i + 1);
            StringCchPrintfW(key_w, 64, L"monitor%d_w", i + 1);
            StringCchPrintfW(key_h, 64, L"monitor%d_h", i + 1);
            StringCchPrintfW(key_name, 64, L"monitor%d_name", i + 1);
            WritePrivateProfileStringW(L"monitor", key_name, hg_g_monitors[i].name, hg_g_config_path);

            WCHAR buf[32];
            hellgates_wsprintf(buf, 32, L"%d", mx);
            WritePrivateProfileStringW(L"monitor", key_x, buf, hg_g_config_path);
            hellgates_wsprintf(buf, 32, L"%d", my);
            WritePrivateProfileStringW(L"monitor", key_y, buf, hg_g_config_path);
            WritePrivateProfileStringW(L"monitor", key_w, L"640", hg_g_config_path);
            hellgates_wsprintf(buf, 32, L"%d", m_h);
            WritePrivateProfileStringW(L"monitor", key_h, buf, hg_g_config_path);

            mx += SC(50);
            my += SC(50);
        }
    }
}
