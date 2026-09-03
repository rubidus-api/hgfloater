/* Taskbox menu construction and command dispatch. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"
#include "../hg_caphook.h"
#include "../hg_options.h"

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

/* The options list, written straight out as rows.
 *
 * There is no Win32 menu behind this any more. There used to be: the options
 * were a menu with submenus, and when the Opt button started showing them in
 * the box the menu was still built, walked back into rows, and thrown away -
 * a tree assembled only to be flattened. The box is the only reader now, so the
 * rows are the thing that gets built.
 *
 * Every row carries the whole path it would have had - "Display 1 > Scale >
 * 125%" - because a list is not a tree and a row has to say where it belongs
 * without a parent above it to lean on.
 *
 * Assembling this costs something real: the audio endpoints are enumerated
 * through COM and every display is asked for its scaling. That is why nothing
 * builds it on a hover - see taskbox_button_box_opens_on_arrival. */
static int menu_add_row(HgMenuRow *rows, int max_rows, int count, const WCHAR *label, UINT id, BOOL enabled,
                        BOOL checked)
{
    if (count >= max_rows)
        return count;
    SecureZeroMemory(&rows[count], sizeof(rows[count]));
    StringCchCopyW(rows[count].label, HG_MENU_ROW_MAX, label);
    rows[count].id = id;
    rows[count].enabled = enabled;
    rows[count].checked = checked;
    rows[count].kind = HG_MENU_ROW_COMMAND;
    return count + 1;
}

/* A row that holds a reading and steps along a ladder. current is the value the
 * display reports; the nearest rung at or below it is where the row starts, so
 * a monitor sitting at 60% shows the 50% rung rather than nothing. */
static int menu_add_value_row(HgMenuRow *rows, int max_rows, int count, const WCHAR *label, UINT base_id,
                              const int *steps, int step_count, int current, int min_percent, int max_percent,
                              BOOL defer)
{
    if (count >= max_rows)
        return count;

    SecureZeroMemory(&rows[count], sizeof(rows[count]));
    StringCchCopyW(rows[count].label, HG_MENU_ROW_MAX, label);
    rows[count].kind = HG_MENU_ROW_VALUE;
    rows[count].enabled = TRUE;
    rows[count].base_id = base_id;
    rows[count].steps = steps;
    rows[count].step_count = step_count;
    rows[count].step_index = -1;
    rows[count].step_min = 0;
    rows[count].step_max = step_count - 1;
    rows[count].defer = defer;

    for (int i = 0; i < step_count; ++i) {
        if (steps[i] < min_percent)
            rows[count].step_min = i + 1;
        if (steps[i] <= max_percent)
            rows[count].step_max = i;
        if (current >= 0 && steps[i] <= current)
            rows[count].step_index = i;
    }
    if (rows[count].step_min > rows[count].step_max) /* nothing this display allows */
        rows[count].step_min = rows[count].step_max;

    /* Nothing is owed yet: the row is showing what the display is on. */
    rows[count].applied_index = rows[count].step_index;

    return count + 1;
}

/* One display's rows: its preview window, its scaling ladder and its backlight,
 * each under that display's own name. */
static int menu_add_display_rows(HgMenuRow *rows, int max_rows, int count, int monitor_index)
{
    const WCHAR *display = hg_g_monitors[monitor_index].label[0] ? hg_g_monitors[monitor_index].label
                                                                 : hg_g_monitors[monitor_index].name;
    WCHAR label[HG_MENU_ROW_MAX];

    hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"%ls > Preview Window", display);
    count = menu_add_row(rows, max_rows, count, label, (UINT)(HG_IDM_MONITOR_BASE + (UINT)monitor_index), TRUE,
                         hg_g_monitors[monitor_index].active);

    HgDisplayScale scale;
    if (hg_query_display_scale(hg_g_monitors[monitor_index].hMonitor, &scale) && scale.valid) {
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"%ls > Scale", display);
        /* Deferred: every rung of this ladder relays out every window on the
         * display, so it is sent once the reader has stopped walking it. */
        count = menu_add_value_row(rows, max_rows, count, label,
                                   (UINT)HG_IDM_SCALE_BASE + (UINT)monitor_index * HG_SCALE_OPTION_COUNT,
                                   hg_display_scale_options, HG_SCALE_OPTION_COUNT, scale.current_percent,
                                   scale.min_percent, scale.max_percent, TRUE);
    } else {
        /* Kept visible but dead: a missing entry reads as a bug, a dead one
         * says the driver would not answer. */
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"%ls > Scale (unavailable)", display);
        count = menu_add_row(rows, max_rows, count, label, 0, FALSE, FALSE);
    }

    if (!hg_monitor_brightness_unavailable(hg_g_monitors[monitor_index].hMonitor)) {
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"%ls > Brightness", display);
        /* Immediate: a backlight step is a message to the monitor and seeing it
         * is the whole point of stepping. */
        count = menu_add_value_row(rows, max_rows, count, label,
                                   (UINT)HG_IDM_BRIGHTNESS_BASE +
                                       (UINT)monitor_index * HG_BRIGHTNESS_OPTION_COUNT,
                                   hg_brightness_options, HG_BRIGHTNESS_OPTION_COUNT,
                                   hg_g_monitors[monitor_index].brightness, 0, 100, FALSE);
    } else {
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"%ls > Brightness (unavailable)", display);
        count = menu_add_row(rows, max_rows, count, label, 0, FALSE, FALSE);
    }

    return count;
}

int hg_menu_build_rows(HgMenuRow *rows, int max_rows)
{
    if (!rows || max_rows <= 0)
        return 0;

    /* Opening the list is the natural refresh point for a changed default
     * device, the same as opening the menu was. */
    update_audio_device_list();

    /* What this list holds, and what it does not.
     *
     * Nothing here is a setting. The switches went to the Set button's list,
     * and so did the three doors that lead to settings - the settings window,
     * the config file, the reset - because a reader looking for what to change
     * should find one list, not two. What is left is the machine: the folder,
     * the audio devices, the displays, and the two ways to stop.
     *
     * About and Exit are last, in that order. They are the end of the list in
     * the sense that matters - you are leaving - and About sits above Exit
     * because the two are one keystroke apart and only one of them is
     * reversible. */
    int count = 0;
    count = menu_add_row(rows, max_rows, count, L"Open Shortcuts Folder", HG_IDM_OPEN_SHORTCUTS, TRUE, FALSE);

    WCHAR label[HG_MENU_ROW_MAX];
    for (int i = 0; i < hg_g_audio_device_count; ++i) {
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"Select Audio Device > %ls", hg_g_audio_devices[i].name);
        count = menu_add_row(rows, max_rows, count, label, (UINT)(HG_IDM_AUDIO_DEVICE_BASE + (UINT)i), TRUE,
                             hg_g_audio_devices[i].is_default);
    }
    count = menu_add_row(rows, max_rows, count, L"Select Audio Device > Mute", HG_IDM_MUTE, TRUE,
                         get_system_mute());

    /* How the screens are arranged comes before what any one of them is set
     * to - it decides which of them there are to set.
     *
     * Always listed, even with one display attached. Hiding these rows when
     * only one screen is in use would be a one-way door: "PC screen only"
     * leaves exactly one display, and the rows that could bring the other one
     * back would be the rows that had just disappeared. */
    int current_topology = hg_display_topology_current();
    for (int i = 0; i < HG_TOPOLOGY_COUNT; ++i) {
        hellgates_wsprintf(label, HG_ARRAYSIZE(label), L"Screens > %ls", hg_display_topology_label(i));
        count = menu_add_row(rows, max_rows, count, label, (UINT)(HG_IDM_TOPOLOGY_BASE + (UINT)i), TRUE,
                             i == current_topology);
    }

    /* One group per display, named the way its owner would name it, off the
     * enumeration WM_DISPLAYCHANGE keeps current. */
    for (int i = 0; i < hg_g_monitor_count; ++i)
        count = menu_add_display_rows(rows, max_rows, count, i);

    count = menu_add_row(rows, max_rows, count, L"Lock Screen (Power Off)", HG_IDM_POWER_OFF, TRUE, FALSE);
    count = menu_add_row(rows, max_rows, count, L"About...", HG_IDM_ABOUT, TRUE, FALSE);
    count = menu_add_row(rows, max_rows, count, L"Exit", HG_IDM_CLOSE_APP, TRUE, FALSE);
    return count;
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
    /* 1024x768 takes G rather than the D that would follow A, S: D has meant
     * 1280x960 since there were three of these, and a hand that has learned it
     * should not be re-taught for the sake of an alphabet. */
    static const WCHAR *const accel_keys[HG_RESIZE_PRESET_COUNT] = {L"A", L"S", L"G", L"D", L"Q", L"W",
                                                                    L"E", L"R", L"1", L"2", L"3", L"4"};
    for (int i = 0; i < HG_RESIZE_PRESET_COUNT; ++i) {
        if (i == 4 || i == 8) {
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
