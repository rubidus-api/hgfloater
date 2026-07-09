/* Taskbox menu construction and command dispatch. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

static HMENU taskbox_create_audio_submenu(void)
{
    HMENU audio_menu = CreatePopupMenu();
    if (!audio_menu)
        return NULL;

    for (int i = 0; i < hg_g_audio_device_count; i++) {
        UINT flags = MF_STRING;
        if (hg_g_audio_devices[i].is_default)
            flags |= MF_CHECKED;
        AppendMenuW(audio_menu, flags, (UINT_PTR)(HG_IDM_AUDIO_DEVICE_BASE + (UINT)i), hg_g_audio_devices[i].name);
    }
    if (hg_g_audio_device_count > 0) {
        AppendMenuW(audio_menu, MF_SEPARATOR, 0, NULL);
    }

    UINT mute_flags = MF_STRING;
    if (get_system_mute()) {
        mute_flags |= MF_CHECKED;
    }
    AppendMenuW(audio_menu, mute_flags, HG_IDM_MUTE, L"Mute");
    return audio_menu;
}

static HMENU taskbox_create_monitor_submenu(void)
{
    HMENU monitor_menu = CreatePopupMenu();
    if (!monitor_menu)
        return NULL;

    for (int i = 0; i < hg_g_monitor_count; i++) {
        UINT flags = MF_STRING;
        if (hg_g_monitors[i].active)
            flags |= MF_CHECKED;
        AppendMenuW(monitor_menu, flags, (UINT_PTR)(HG_IDM_MONITOR_BASE + (UINT)i), hg_g_monitors[i].name);
    }
    return monitor_menu;
}

int taskbox_track_owned_popup_menu(HMENU h_menu, UINT flags, int x, int y, HWND owner)
{
    int cmd = 0;
    if (!h_menu)
        return 0;

    hg_g_menu_active = TRUE;
    cmd = TrackPopupMenuEx(h_menu, flags, x, y, owner, NULL);
    hg_g_menu_active = FALSE;
    DestroyMenu(h_menu);
    return cmd;
}

HMENU taskbox_create_main_popup_menu(void)
{
    update_audio_device_list();

    HMENU h_menu = CreatePopupMenu();
    if (!h_menu)
        return NULL;

    AppendMenuW(h_menu, MF_STRING, HG_IDM_OPEN_SHORTCUTS, L"Open Shortcuts Folder");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_EDIT_CONFIG, L"Edit Configuration");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_ABOUT, L"About...");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_RESET_ALL, L"Reset Settings");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    HMENU audio_menu = taskbox_create_audio_submenu();
    if (audio_menu) {
        if (!AppendMenuW(h_menu, MF_POPUP, (UINT_PTR)audio_menu, L"Select Audio Device")) {
            DestroyMenu(audio_menu);
        }
    }

    HMENU monitor_menu = taskbox_create_monitor_submenu();
    if (monitor_menu) {
        if (!AppendMenuW(h_menu, MF_POPUP, (UINT_PTR)monitor_menu, L"Arrange Monitors")) {
            DestroyMenu(monitor_menu);
        }
    }

    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_POWER_OFF, L"Lock Screen (Power Off)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_CLOSE_APP, L"Exit");
    return h_menu;
}

void taskbox_dispatch_main_menu_command(UINT cmd)
{
    if (cmd != 0) {
        PostMessageW(hg_g_floater_wnd, WM_COMMAND, (WPARAM)cmd, 0);
    }
}

static BOOL toolbar_controller_get_context_menu_point(HWND hwnd, int cur_type, int cur_index, int icon_size,
                                                      LPARAM l_param, POINT *screen_pt)
{
    if (!screen_pt)
        return FALSE;

    if (l_param == 0) {
        RECT rc;
        RECT rc_item;
        GetClientRect(hwnd, &rc);
        get_toolbar_item_rect(cur_type, cur_index, rc.right, rc.bottom, icon_size, &rc_item);
        screen_pt->x = rc_item.left;
        screen_pt->y = rc_item.top;
        ClientToScreen(hwnd, screen_pt);
    } else {
        GetCursorPos(screen_pt);
    }

    return TRUE;
}

static HMENU taskbox_create_task_context_menu(void)
{
    HMENU h_menu = CreatePopupMenu();
    if (!h_menu)
        return NULL;

    /* Windows: Focus only (remove Run) */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESTORE, L"Focus (&F)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_MOVETO_0_0, L"Move to (0, 0) (&0)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_CLOSE, L"Close Window (&X)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_1, L"640x480 (&A)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_2, L"800x600 (&S)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_3, L"1280x960 (&D)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_1, L"640x360 (&Q)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_2, L"800x480 (&W)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_3, L"960x540 (&E)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_4, L"1280x720 (&R)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_1, L"360x640 (&1)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_2, L"480x800 (&2)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_3, L"540x960 (&3)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_4, L"720x1280 (&4)");
    return h_menu;
}

static BOOL taskbox_task_menu_size_for_command(int cmd, int *out_cx, int *out_cy)
{
    if (!out_cx || !out_cy)
        return FALSE;

    *out_cx = 0;
    *out_cy = 0;
    switch (cmd) {
    case HG_IDM_TASK_RESIZE_4_3_1:
        *out_cx = 640;
        *out_cy = 480;
        break;
    case HG_IDM_TASK_RESIZE_4_3_2:
        *out_cx = 800;
        *out_cy = 600;
        break;
    case HG_IDM_TASK_RESIZE_4_3_3:
        *out_cx = 1280;
        *out_cy = 960;
        break;
    case HG_IDM_TASK_RESIZE_16_9_1:
        *out_cx = 640;
        *out_cy = 360;
        break;
    case HG_IDM_TASK_RESIZE_16_9_2:
        *out_cx = 800;
        *out_cy = 480;
        break;
    case HG_IDM_TASK_RESIZE_16_9_3:
        *out_cx = 960;
        *out_cy = 540;
        break;
    case HG_IDM_TASK_RESIZE_16_9_4:
        *out_cx = 1280;
        *out_cy = 720;
        break;
    case HG_IDM_TASK_RESIZE_9_16_1:
        *out_cx = 360;
        *out_cy = 640;
        break;
    case HG_IDM_TASK_RESIZE_9_16_2:
        *out_cx = 480;
        *out_cy = 800;
        break;
    case HG_IDM_TASK_RESIZE_9_16_3:
        *out_cx = 540;
        *out_cy = 960;
        break;
    case HG_IDM_TASK_RESIZE_9_16_4:
        *out_cx = 720;
        *out_cy = 1280;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void taskbox_dispatch_task_menu_command(int cmd, HWND target)
{
    if (cmd == 0 || !target || !IsWindow(target))
        return;

    if (cmd == HG_IDM_TASK_RESTORE) {
        if (IsIconic(target))
            ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    } else if (cmd == HG_IDM_TASK_CLOSE) {
        PostMessageW(target, WM_CLOSE, 0, 0);
    } else if (cmd == HG_IDM_TASK_MOVETO_0_0) {
        SetWindowPos(target, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        int cx = 0, cy = 0;
        if (taskbox_task_menu_size_for_command(cmd, &cx, &cy)) {
            SetWindowPos(target, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

void toolbar_controller_show_task_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param)
{
    if (cur_index < 0 || cur_index >= hg_g_window_count)
        return;

    /* The refresh timer can reorder the item list while the menu is modal, so act on
     * the window handle captured now, not on the index. */
    HWND target = hg_g_window_items[cur_index].hwnd;

    HMENU h_menu = taskbox_create_task_context_menu();
    if (!h_menu)
        return;

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 0, cur_index, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetMenuDefaultItem(h_menu, HG_IDM_TASK_RESTORE, FALSE);
    int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD, screen_pt.x, screen_pt.y, hwnd);
    taskbox_dispatch_task_menu_command(cmd, target);
}

static HMENU taskbox_create_shortcut_context_menu(int cur_index)
{
    HMENU h_menu = CreatePopupMenu();
    if (!h_menu)
        return NULL;

    /* Shortcuts: Run only (remove Focus) */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_SHORTCUT_RUN, L"Run (&R)");
    if (cur_index >= HG_NUM_BASIC_ICONS) {
        AppendMenuW(h_menu, MF_STRING, HG_IDM_SHORTCUT_OPEN_DIR, L"Open File Location (&O)");
    }
    return h_menu;
}

static void taskbox_dispatch_shortcut_menu_command(UINT cmd, int cur_index)
{
    if (cmd == HG_IDM_SHORTCUT_RUN) {
        activate_toolbar_item(cur_index);
    } else if (cmd == HG_IDM_SHORTCUT_OPEN_DIR) {
        int s_idx = cur_index - HG_NUM_BASIC_ICONS;
        if (s_idx >= 0 && s_idx < hg_g_shortcut_count) {
            PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(hg_g_shortcuts[s_idx].path);
            if (pidl) {
                SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
                ILFree(pidl);
            }
        }
    }
}

void toolbar_controller_show_shortcut_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param)
{
    HMENU h_menu = taskbox_create_shortcut_context_menu(cur_index);
    if (!h_menu)
        return;

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 1, cur_index, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetMenuDefaultItem(h_menu, HG_IDM_SHORTCUT_RUN, FALSE);
    int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD, screen_pt.x, screen_pt.y, hwnd);
    taskbox_dispatch_shortcut_menu_command((UINT)cmd, cur_index);
}

BOOL taskbox_handle_audio_menu_command(UINT cmd)
{
    if (cmd >= HG_IDM_AUDIO_DEVICE_BASE && cmd < HG_IDM_AUDIO_DEVICE_BASE + HG_MAX_AUDIO_DEVICES) {
        int idx = (int)(cmd - HG_IDM_AUDIO_DEVICE_BASE);
        if (idx >= 0 && idx < hg_g_audio_device_count) {
            if (set_default_audio_device(hg_g_audio_devices[idx].id)) {
                update_audio_device_list();
            }
            update_toolbar_tooltips(hg_g_toolbar_wnd);
            update_focus_message(1, HG_TOOL_ICON_VOLUME);
            if (hg_g_toolbar_wnd) {
                InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
            }
        }
        return TRUE;
    }

    if (cmd == HG_IDM_MUTE) {
        set_system_mute(!get_system_mute());
        update_toolbar_tooltips(hg_g_toolbar_wnd);
        update_focus_message(1, HG_TOOL_ICON_VOLUME);
        if (hg_g_toolbar_wnd) {
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
        }
        return TRUE;
    }

    return FALSE;
}

void toolbar_controller_show_audio_context_menu(HWND hwnd, int icon_size, LPARAM l_param)
{
    update_audio_device_list();

    HMENU h_menu = taskbox_create_audio_submenu();
    if (!h_menu)
        return;

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 1, HG_TOOL_ICON_VOLUME, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetForegroundWindow(hwnd);
    int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_pt.x, screen_pt.y, hwnd);
    if (cmd != 0) {
        taskbox_handle_audio_menu_command((UINT)cmd);
    }
}

