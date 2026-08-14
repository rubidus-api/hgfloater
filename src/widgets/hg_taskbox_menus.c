/* Taskbox menu construction and command dispatch. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"
#include "../hg_caphook.h"
#include "../hg_options.h"

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

/* Windows scaling percentages for one display, with the current value checked
 * and anything outside the range that monitor allows greyed out. */
static HMENU taskbox_create_scale_submenu(int monitor_index)
{
    HgDisplayScale info;
    if (!hg_query_display_scale(hg_g_monitors[monitor_index].hMonitor, &info) || !info.valid)
        return NULL;

    HMENU scale_menu = CreatePopupMenu();
    if (!scale_menu)
        return NULL;

    UINT base = (UINT)HG_IDM_SCALE_BASE + (UINT)monitor_index * HG_SCALE_OPTION_COUNT;
    for (int i = 0; i < HG_SCALE_OPTION_COUNT; ++i) {
        int percent = hg_display_scale_options[i];
        UINT flags = MF_STRING;
        if (percent == info.current_percent)
            flags |= MF_CHECKED;
        if (percent < info.min_percent || percent > info.max_percent)
            flags |= MF_GRAYED;

        WCHAR text[16];
        hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d%%", percent);
        AppendMenuW(scale_menu, flags, (UINT_PTR)(base + (UINT)i), text);
    }
    return scale_menu;
}

/* Backlight in quarter steps. The tick follows the cached DDC/CI reading rounded
 * to the nearest step, so a monitor sitting at 60% shows 50% ticked rather than
 * nothing at all; an unread monitor shows no tick. */
static HMENU taskbox_create_brightness_submenu(int monitor_index)
{
    /* Only once the display has actually been probed and refused everything;
     * one not yet asked is offered normally. */
    if (hg_monitor_brightness_unavailable(hg_g_monitors[monitor_index].hMonitor))
        return NULL;

    HMENU brightness_menu = CreatePopupMenu();
    if (!brightness_menu)
        return NULL;

    int current = hg_g_monitors[monitor_index].brightness;
    int nearest = (current >= 0) ? ((current + 12) / 25) * 25 : -1;
    UINT base = (UINT)HG_IDM_BRIGHTNESS_BASE + (UINT)monitor_index * HG_BRIGHTNESS_OPTION_COUNT;

    for (int i = 0; i < HG_BRIGHTNESS_OPTION_COUNT; ++i) {
        int percent = hg_brightness_options[i];
        UINT flags = MF_STRING;
        if (percent == nearest)
            flags |= MF_CHECKED;

        WCHAR text[16];
        hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d%%", percent);
        AppendMenuW(brightness_menu, flags, (UINT_PTR)(base + (UINT)i), text);
    }
    return brightness_menu;
}

/* Everything this app can do to one display, gathered under that display's own
 * name: its preview window, its scaling, and its backlight. */
static HMENU taskbox_create_display_submenu(int monitor_index)
{
    HMENU display_menu = CreatePopupMenu();
    if (!display_menu)
        return NULL;

    UINT preview_flags = MF_STRING;
    if (hg_g_monitors[monitor_index].active)
        preview_flags |= MF_CHECKED;
    AppendMenuW(display_menu, preview_flags, (UINT_PTR)(HG_IDM_MONITOR_BASE + (UINT)monitor_index),
                L"Preview Window");
    AppendMenuW(display_menu, MF_SEPARATOR, 0, NULL);

    HMENU scale_menu = taskbox_create_scale_submenu(monitor_index);
    if (scale_menu) {
        if (!AppendMenuW(display_menu, MF_POPUP, (UINT_PTR)scale_menu, L"Scale")) {
            DestroyMenu(scale_menu);
        }
    } else {
        /* Kept visible but dead: a missing entry reads as a bug, a greyed one
         * says the driver would not answer. */
        AppendMenuW(display_menu, MF_STRING | MF_GRAYED, 0, L"Scale (unavailable)");
    }

    HMENU brightness_menu = taskbox_create_brightness_submenu(monitor_index);
    if (brightness_menu) {
        if (!AppendMenuW(display_menu, MF_POPUP, (UINT_PTR)brightness_menu, L"Brightness")) {
            DestroyMenu(brightness_menu);
        }
    } else {
        /* Same bargain as Scale: kept visible but dead, because a missing entry
         * reads as a bug and a greyed one says the display would not answer. */
        AppendMenuW(display_menu, MF_STRING | MF_GRAYED, 0, L"Brightness (unavailable)");
    }
    return display_menu;
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

/* One row per option, checked from its current value. An option that cannot be
 * switched in this build is greyed and says why rather than vanishing: a
 * feature that disappears leaves the reader wondering whether they imagined
 * it, while a greyed line that reads "(off in this build)" leaves them
 * informed, in the one place they would look. */
static HMENU taskbox_create_options_submenu(void)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return NULL;

    int count = hg_option_count();
    for (int i = 1; i <= count; ++i) {
        HgOptionInfo info;
        if (!hg_option_info(i, &info))
            continue;

        WCHAR text[160];
        if (info.available) {
            StringCchCopyW(text, HG_ARRAYSIZE(text), info.label);
        } else {
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%ls  (%ls)", info.label,
                             info.unavailable_note ? info.unavailable_note : L"unavailable");
        }

        UINT flags = MF_STRING;
        if (!info.available)
            flags |= MF_GRAYED;
        else if (hg_option_get(i))
            flags |= MF_CHECKED;

        AppendMenuW(menu, flags, (UINT_PTR)(HG_IDM_OPTION_BASE + (UINT)i), text);
    }

    return menu;
}

HMENU taskbox_create_main_popup_menu(void)
{
    update_audio_device_list();

    HMENU h_menu = CreatePopupMenu();
    if (!h_menu)
        return NULL;

    AppendMenuW(h_menu, MF_STRING, HG_IDM_OPEN_SHORTCUTS, L"Open Shortcuts Folder");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_EDIT_CONFIG, L"Edit Configuration");

    /* Every switch in one place. They used to sit loose at the top of this
     * menu, three of them, each with its own command id - and the fourth would
     * have made four. One submenu built from the options table means a new
     * toggle appears here by existing. */
    HMENU options_menu = taskbox_create_options_submenu();
    if (options_menu) {
        if (!AppendMenuW(h_menu, MF_POPUP, (UINT_PTR)options_menu, L"Options")) {
            DestroyMenu(options_menu);
        }
    }
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

    /* One entry per display, named the way its owner would name it, off the
     * enumeration WM_DISPLAYCHANGE keeps current. */
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        HMENU display_menu = taskbox_create_display_submenu(i);
        if (!display_menu)
            continue;

        const WCHAR *label = hg_g_monitors[i].label[0] ? hg_g_monitors[i].label : hg_g_monitors[i].name;
        if (!AppendMenuW(h_menu, MF_POPUP, (UINT_PTR)display_menu, label)) {
            DestroyMenu(display_menu);
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

static HMENU taskbox_create_task_context_menu(BOOL is_tab)
{
    HMENU h_menu = CreatePopupMenu();
    if (!h_menu)
        return NULL;

    /* Windows: Focus only (remove Run) */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESTORE, L"Focus (&F)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_MOVETO_0_0, L"Move to (0, 0) (&0)");
    /* On a tab this closes the tab, so it must not say Window. Saying the wrong
     * one is how a right-click threw away eleven other tabs. */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_CLOSE, is_tab ? L"Close Tab (&X)" : L"Close Window (&X)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    /* Sizes and their order come from the shared preset table, so this menu and
     * the command box's 'list resize' can never disagree; only the access keys
     * and the group separators live here. */
    static const WCHAR *const accel_keys[HG_RESIZE_PRESET_COUNT] = {L"A", L"S", L"D", L"Q", L"W", L"E",
                                                                    L"R", L"1", L"2", L"3", L"4"};
    for (int i = 0; i < HG_RESIZE_PRESET_COUNT; ++i) {
        if (i == 3 || i == 7) {
            AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
        }
        WCHAR text[48];
        hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%ls (&%ls)", hg_resize_presets[i].name, accel_keys[i]);
        AppendMenuW(h_menu, MF_STRING, (UINT_PTR)(HG_IDM_TASK_RESIZE_4_3_1 + (UINT)i), text);
    }
    return h_menu;
}

static BOOL taskbox_task_menu_size_for_command(int cmd, int *out_cx, int *out_cy)
{
    if (!out_cx || !out_cy)
        return FALSE;

    /* The resize command ids are consecutive, so the offset from the first one
     * is the index into the preset table the menu was built from. */
    *out_cx = 0;
    *out_cy = 0;
    int index = cmd - HG_IDM_TASK_RESIZE_4_3_1;
    if (index < 0 || index >= HG_RESIZE_PRESET_COUNT)
        return FALSE;

    *out_cx = hg_resize_presets[index].cx;
    *out_cy = hg_resize_presets[index].cy;
    return TRUE;
}

static void taskbox_dispatch_task_menu_command(int cmd, HWND target, BOOL is_tab, int tab_index)
{
    if (cmd == 0 || !target || !IsWindow(target))
        return;

    if (cmd == HG_IDM_TASK_RESTORE) {
        if (is_tab) {
            hg_tabs_activate(target, tab_index);
            return;
        }
        if (IsIconic(target))
            ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    } else if (cmd == HG_IDM_TASK_CLOSE) {
        if (is_tab) {
            /* Nothing at all when the tab cannot be closed. The only other
             * thing this code could do is close the window, which is what the
             * reader was not asking for and cannot be undone. */
            if (!hg_tabs_close(target, tab_index))
                append_message(L"That tab has no close button of its own; nothing was closed");
            return;
        }
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
    BOOL is_tab = hg_g_window_items[cur_index].is_tab;
    int tab_index = hg_g_window_items[cur_index].tab_index;

    HMENU h_menu = taskbox_create_task_context_menu(is_tab);
    if (!h_menu)
        return;

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 0, cur_index, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetMenuDefaultItem(h_menu, HG_IDM_TASK_RESTORE, FALSE);
    int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD, screen_pt.x, screen_pt.y, hwnd);
    taskbox_dispatch_task_menu_command(cmd, target, is_tab, tab_index);
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

