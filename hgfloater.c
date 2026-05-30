#include "hg_common.h"
#include "hg_utils.h"
#include "hg_config.h"

/* =========================================================================
 * 창 클래스 등록 기능 (Window Class Registration)
 * ========================================================================= */
void show_about_window(void);

static BOOL register_app_window_class(HINSTANCE instance, const WindowClassSpec *spec, HICON icon_large,
                                      HICON icon_small)
{
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = spec->wnd_proc;
    wc.hInstance = instance;
    wc.lpszClassName = spec->class_name;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = spec->background;
    wc.hIcon = icon_large;
    wc.hIconSm = icon_small;

    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxW(NULL, spec->fail_message, L"hgfloater", MB_ICONERROR);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL register_app_window_classes(HINSTANCE instance, HICON icon_large, HICON icon_small)
{
    const WindowClassSpec specs[] = {
        {HG_CLASS_FLOATER_WIDGET, floater_proc, hg_g_main_bg_brush, L"Failed to register floater class."},
        {HG_CLASS_ABOUT, about_proc, hg_g_main_bg_brush, L"Failed to register about class."},
        {HG_CLASS_TASKBOX, window_proc, hg_g_main_bg_brush, L"Failed to register taskbox class."},
        {HG_CLASS_MONITOR, monitor_wnd_proc, (HBRUSH)GetStockObject(BLACK_BRUSH), L"Failed to register monitor class."},
        {HG_CLASS_COMMANDBOX, commandbox_proc, hg_g_main_bg_brush, L"Failed to register commandbox class."},
    };

    for (int i = 0; i < (int)HG_ARRAYSIZE(specs); ++i) {
        if (!register_app_window_class(instance, &specs[i], icon_large, icon_small)) {
            return FALSE;
        }
    }
    return TRUE;
}

static void unregister_app_window_classes(HINSTANCE instance)
{
    const WCHAR *classes[] = {
        HG_CLASS_FLOATER_WIDGET, HG_CLASS_ABOUT, HG_CLASS_TASKBOX, HG_CLASS_MONITOR, HG_CLASS_COMMANDBOX,
    };

    for (int i = 0; i < (int)HG_ARRAYSIZE(classes); ++i) {
        if (!UnregisterClassW(classes[i], instance)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_DOES_NOT_EXIST) {
                /* Ignore cleanup noise; class teardown happens at process exit anyway. */
            }
        }
    }
}

static void store_pending_command_line(LPCWSTR cmd_line)
{
    if (!cmd_line || !cmd_line[0]) {
        return;
    }

    if (FAILED(StringCchCopyW(hg_g_pending_command_line, HG_ARRAYSIZE(hg_g_pending_command_line), cmd_line))) {
        hg_g_pending_command_line[0] = L'\0';
        hg_g_has_pending_command_line = FALSE;
        return;
    }

    hg_g_has_pending_command_line = TRUE;
}

static BOOL cli_arg_equals(LPCWSTR arg, LPCWSTR long_name, LPCWSTR short_name)
{
    return arg && ((long_name && lstrcmpiW(arg, long_name) == 0) || (short_name && lstrcmpiW(arg, short_name) == 0));
}

static HgCliAction parse_cli_action(LPCWSTR cmd_line)
{
    int argc = 0;
    LPWSTR *argv = NULL;
    HgCliAction action = HG_CLI_ACTION_DEFAULT;

    if (!cmd_line || !cmd_line[0]) {
        return HG_CLI_ACTION_DEFAULT;
    }

    argv = CommandLineToArgvW(cmd_line, &argc);
    if (!argv) {
        return HG_CLI_ACTION_DEFAULT;
    }

    for (int i = 0; i < argc; ++i) {
        LPCWSTR arg = argv[i];
        if (cli_arg_equals(arg, L"--show", L"/show")) {
            action = HG_CLI_ACTION_SHOW;
            break;
        }
        if (cli_arg_equals(arg, L"--hide", L"/hide")) {
            action = HG_CLI_ACTION_HIDE;
            break;
        }
        if (cli_arg_equals(arg, L"--toggle", L"/toggle")) {
            action = HG_CLI_ACTION_TOGGLE;
            break;
        }
        if (cli_arg_equals(arg, L"--about", L"/about")) {
            action = HG_CLI_ACTION_ABOUT;
            break;
        }
        if (cli_arg_equals(arg, L"--exit", L"/exit") || cli_arg_equals(arg, L"--quit", L"/quit")) {
            action = HG_CLI_ACTION_EXIT;
            break;
        }
        if (cli_arg_equals(arg, L"--help", L"/help") || cli_arg_equals(arg, L"-h", L"/?")) {
            action = HG_CLI_ACTION_HELP;
            break;
        }
    }

    LocalFree(argv);
    return action;
}

static void show_taskbox_from_cli(void)
{
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        SendMessageW(hg_g_floater_wnd, WM_HOTKEY, 1, 0);
    }
}

static BOOL dispatch_cli_action(HgCliAction action)
{
    switch (action) {
    case HG_CLI_ACTION_SHOW:
        show_taskbox_from_cli();
        return TRUE;
    case HG_CLI_ACTION_HIDE:
        if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
            hide_taskbox(hg_g_taskbox_wnd);
        }
        return TRUE;
    case HG_CLI_ACTION_TOGGLE:
        if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
            hide_taskbox(hg_g_taskbox_wnd);
        } else {
            show_taskbox_from_cli();
        }
        return TRUE;
    case HG_CLI_ACTION_ABOUT:
        show_about_window();
        return TRUE;
    case HG_CLI_ACTION_EXIT:
        if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
            PostMessageW(hg_g_floater_wnd, WM_COMMAND, HG_IDM_CLOSE_APP, 0);
        }
        return TRUE;
    case HG_CLI_ACTION_HELP:
        MessageBoxW(NULL,
                    L"hgfloater command line options:\r\n\r\n"
                    L"  --show     Show the taskbox\r\n"
                    L"  --hide     Hide the taskbox\r\n"
                    L"  --toggle   Toggle the taskbox\r\n"
                    L"  --about    Show the About window\r\n"
                    L"  --exit     Exit the running instance\r\n"
                    L"  --help     Show this help",
                    L"hgfloater", MB_OK | MB_ICONINFORMATION);
        return TRUE;
    case HG_CLI_ACTION_DEFAULT:
    default:
        return FALSE;
    }
}

static BOOL dispatch_pending_command_line(void)
{
    HgCliAction action;

    if (!hg_g_has_pending_command_line || !hg_g_pending_command_line[0]) {
        return FALSE;
    }

    action = parse_cli_action(hg_g_pending_command_line);
    hg_g_pending_command_line[0] = L'\0';
    hg_g_has_pending_command_line = FALSE;
    return dispatch_cli_action(action);
}

static BOOL forward_command_line_to_instance(HWND existing_wnd, LPCWSTR cmd_line)
{
    COPYDATASTRUCT cds;

    if (!existing_wnd || !IsWindow(existing_wnd) || !cmd_line || !cmd_line[0]) {
        return FALSE;
    }

    cds.dwData = HG_COPYDATA_COMMAND_LINE;
    cds.cbData = (DWORD)((wcslen(cmd_line) + 1) * sizeof(WCHAR));
    cds.lpData = (PVOID)cmd_line;
    return SendMessageW(existing_wnd, WM_COPYDATA, 0, (LPARAM)&cds) != 0;
}

LRESULT handle_copydata_command_line(const COPYDATASTRUCT *cds)
{
    if (!cds || cds->dwData != HG_COPYDATA_COMMAND_LINE || !cds->lpData || cds->cbData < sizeof(WCHAR)) {
        return FALSE;
    }

    if (FAILED(
            StringCchCopyW(hg_g_pending_command_line, HG_ARRAYSIZE(hg_g_pending_command_line), (LPCWSTR)cds->lpData))) {
        hg_g_pending_command_line[0] = L'\0';
        hg_g_has_pending_command_line = FALSE;
        return FALSE;
    }

    hg_g_has_pending_command_line = TRUE;
    return dispatch_pending_command_line();
}

static void request_existing_instance_activation(LPCWSTR cmd_line)
{
    HWND existing_wnd = FindWindowW(HG_CLASS_FLOATER_WIDGET, NULL);
    HgCliAction action = parse_cli_action(cmd_line);
    if (!existing_wnd) {
        return;
    }

    if (cmd_line && cmd_line[0]) {
        forward_command_line_to_instance(existing_wnd, cmd_line);
    }

    if (action != HG_CLI_ACTION_DEFAULT) {
        return;
    }

    if (IsIconic(existing_wnd)) {
        ShowWindow(existing_wnd, SW_RESTORE);
    }

    SetForegroundWindow(existing_wnd);
    PostMessageW(existing_wnd, WM_HOTKEY, 1, 0);
}











/* 창이 화면을 벗어났는지 확인하고 0,0으로 복구 */
void ensure_window_visible(HWND hwnd, const WCHAR *section)
{
    if (!IsWindow(hwnd) || !section)
        return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc))
        return;

    HMONITOR monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor_handle, &mi)) {
        /* 창의 일부라도 작업 영역(Work Area)을 벗어나면 0,0으로 이동 */
        if (rc.left < mi.rcWork.left || rc.top < mi.rcWork.top || rc.right > mi.rcWork.right ||
            rc.bottom > mi.rcWork.bottom) {
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_config(section, 0, 0, rc.right - rc.left, rc.bottom - rc.top);
        }
    }
}










/* 증분 업데이트: 순서 고정 및 스마트 리프레시 */

/* taskbar_proc removed */








int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int cmd_show)
{
    HRESULT co_hr;
    BOOL com_initialized = FALSE;
    HANDLE mutex = NULL;
    HICON icon_large = NULL;
    HICON icon_small = NULL;
    HACCEL accel_table = NULL;
    int exit_code = 0;

    (void)prev_instance;
    (void)cmd_line;
    (void)cmd_show;

    /* DPI awareness must be set before UI/DC related work. */
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    co_hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    com_initialized = SUCCEEDED(co_hr);

    /* 단일 인스턴스 실행 보장 (Mutex 사용) */
    mutex = CreateMutexW(NULL, TRUE, HG_SINGLE_INSTANCE_MUTEX_NAME);
    if (!mutex) {
        exit_code = 1;
        goto cleanup_finish;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        request_existing_instance_activation(cmd_line);
        goto cleanup_finish;
    }

    init_paths();
    init_color_scheme();
    update_theme_colors();

    /* DPI 스케일 계산 */
    HDC screen_dc = GetDC(NULL);
    if (screen_dc) {
        int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
        ReleaseDC(NULL, screen_dc);
        if (dpi_x > 0)
            hg_g_scale_factor = dpi_x / 96.0;
    }

    /* 초기 값들에 DPI 배율 적용 */

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    /* Extract icons first to share across classes */
    icon_large = (HICON)LoadImageW(instance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                                   GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    icon_small = (HICON)LoadImageW(instance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    if (!hg_g_main_bg_brush)
        hg_g_main_bg_brush = CreateSolidBrush(HG_CLICKABLE_BG);
    if (!register_app_window_classes(instance, icon_large, icon_small)) {
        exit_code = 1;
        goto cleanup_finish;
    }

    store_pending_command_line(cmd_line);

    int fx, fy, fw, fh, tx, ty, tw, th;
    load_config(L"floater", &fx, &fy, &fw, &fh, 100, 100, SC(80), SC(55));
    load_config(L"taskbox", &tx, &ty, &tw, &th, 200, 200, SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT));
    hg_g_floater_alpha = (BYTE)get_alpha_config(L"floater", 204);
    hg_g_taskbox_alpha = (BYTE)get_alpha_config(L"taskbox", 204);
    hg_g_commandbox_alpha = (BYTE)get_alpha_config(L"commandbox", 204);
    load_font_name_config();
    load_hotkey_config();
    load_floater_font_config();
    load_taskbox_font_config();

    hg_g_floater_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_APPWINDOW, HG_CLASS_FLOATER_WIDGET,
                                       L"floater", WS_POPUP | WS_VISIBLE, fx, fy, fw, fh, NULL, NULL, instance, NULL);
    if (!hg_g_floater_wnd) {
        MessageBoxW(NULL, L"Failed to create floater window.", L"hgfloater", MB_ICONERROR);
        exit_code = 1;
        goto cleanup_finish;
    }

    hg_g_hotkey_registered = RegisterHotKey(hg_g_floater_wnd, 1, hg_g_hotkey_modifiers | MOD_NOREPEAT, hg_g_hotkey_key);
    if (!hg_g_hotkey_registered) {
        MessageBoxW(NULL, L"Global hotkey registration failed. Another program may be using the assigned hotkey.",
                    L"hgfloater", MB_ICONWARNING);
    }

    hg_g_taskbox_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, HG_CLASS_TASKBOX, L"taskbox",
                                       WS_POPUP, tx, ty, tw, th, NULL, NULL, instance, NULL);
    if (!hg_g_taskbox_wnd) {
        MessageBoxW(NULL, L"Failed to create taskbox window.", L"hgfloater", MB_ICONERROR);
        exit_code = 1;
        goto cleanup_finish;
    }

    update_monitor_enum();
    dispatch_pending_command_line();

    ACCEL accel[] = {{FCONTROL | FVIRTKEY, 'Q', HG_IDM_CLOSE_APP},
                     {FCONTROL | FVIRTKEY, 'X', HG_IDM_CLOSE_APP},
                     {FALT | FVIRTKEY, VK_F4, HG_IDM_CLOSE_APP},
                     {FVIRTKEY, VK_F1, HG_IDM_ABOUT},
                     {FCONTROL | FSHIFT | FVIRTKEY, 'R', HG_IDM_RESET_ALL},
                     {FCONTROL | FVIRTKEY, 'R', HG_IDM_RESET_ALL},
                     {FVIRTKEY, VK_F5, HG_IDM_RESET_ALL},
                     {FCONTROL | FVIRTKEY, '0', HG_IDM_RESET_ALL},
                     {FCONTROL | FVIRTKEY, VK_OEM_PLUS, HG_IDM_FONT_UP},
                     {FCONTROL | FVIRTKEY, VK_OEM_MINUS, HG_IDM_FONT_DOWN},
                     {FCONTROL | FVIRTKEY, VK_ADD, HG_IDM_FONT_UP},
                     {FCONTROL | FVIRTKEY, VK_SUBTRACT, HG_IDM_FONT_DOWN}};
    accel_table = CreateAcceleratorTableW(accel, HG_ARRAYSIZE(accel));

    MSG msg_struct;
    BOOL bRet;
    while ((bRet = GetMessageW(&msg_struct, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
            exit_code = 1;
            break;
        }



        BOOL accel_handled = FALSE;
        if (accel_table && hg_g_taskbox_wnd && TranslateAcceleratorW(hg_g_taskbox_wnd, accel_table, &msg_struct)) {
            accel_handled = TRUE;
        }
        if (!accel_handled && accel_table && hg_g_floater_wnd &&
            TranslateAcceleratorW(hg_g_floater_wnd, accel_table, &msg_struct)) {
            accel_handled = TRUE;
        }

        if (!accel_handled) {
            TranslateMessage(&msg_struct);
            DispatchMessageW(&msg_struct);
        }
    }

    save_alpha_config();

cleanup_finish:
    if (accel_table) {
        DestroyAcceleratorTable(accel_table);
        accel_table = NULL;
    }

    if (hg_g_hotkey_registered && hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        UnregisterHotKey(hg_g_floater_wnd, 1);
        hg_g_hotkey_registered = FALSE;
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        DestroyWindow(hg_g_taskbox_wnd);
        hg_g_taskbox_wnd = NULL;
    }

    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        DestroyWindow(hg_g_about_wnd);
        hg_g_about_wnd = NULL;
    }
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        DestroyWindow(hg_g_floater_wnd);
        hg_g_floater_wnd = NULL;
    }
    if (hg_g_tooltip_wnd && IsWindow(hg_g_tooltip_wnd)) {
        DestroyWindow(hg_g_tooltip_wnd);
        hg_g_tooltip_wnd = NULL;
    }
    if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd)) {
        DestroyWindow(hg_g_commandbox_wnd);
        hg_g_commandbox_wnd = NULL;
    }

    unregister_app_window_classes(instance);

    if (mutex) {
        CloseHandle(mutex);
        mutex = NULL;
    }

    /* Consolidated cleanup for all global GDI resources */
    if (hg_g_main_font && hg_g_main_font != GetStockObject(DEFAULT_GUI_FONT)) {
        DeleteObject(hg_g_main_font);
        hg_g_main_font = NULL;
    }
    if (hg_g_floater_time_font) {
        DeleteObject(hg_g_floater_time_font);
        hg_g_floater_time_font = NULL;
    }
    if (hg_g_floater_date_font) {
        DeleteObject(hg_g_floater_date_font);
        hg_g_floater_date_font = NULL;
    }
    if (hg_g_toolbar_btn_font) {
        DeleteObject(hg_g_toolbar_btn_font);
        hg_g_toolbar_btn_font = NULL;
    }
    if (hg_g_main_bg_brush) {
        DeleteObject(hg_g_main_bg_brush);
        hg_g_main_bg_brush = NULL;
    }
    if (hg_g_edit_bg_brush) {
        DeleteObject(hg_g_edit_bg_brush);
        hg_g_edit_bg_brush = NULL;
    }
    if (hg_g_hbr_highlight) {
        DeleteObject(hg_g_hbr_highlight);
        hg_g_hbr_highlight = NULL;
    }
    if (hg_g_commandbox_font) {
        DeleteObject(hg_g_commandbox_font);
        hg_g_commandbox_font = NULL;
    }

    /* Clean up all shortcut and window item icons safely */
    for (int i = 0; i < hg_g_shortcut_count; i++) {
        if (hg_g_shortcuts[i].icon) {
            DestroyIcon(hg_g_shortcuts[i].icon);
            hg_g_shortcuts[i].icon = NULL;
        }
    }
    for (int i = 0; i < hg_g_window_count; i++) {
        if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
            DestroyIcon(hg_g_window_items[i].icon);
            hg_g_window_items[i].icon = NULL;
        }
    }

    if (icon_large)
        DestroyIcon(icon_large);
    if (icon_small)
        DestroyIcon(icon_small);
    if (com_initialized)
        CoUninitialize();
    return exit_code;
}
