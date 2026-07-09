#include "hg_taskbox.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

void update_size(int delta)
{
    int old_size = ABS(hg_g_current_font_size);
    if (old_size < SC(16))
        old_size = SC(16);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(16))
        new_size = SC(16);
    if (new_size > SC(128))
        new_size = SC(128);
    if (new_size == old_size)
        return;
    hg_g_current_font_size = -new_size;
    save_taskbox_font_config();
    release_font_handle(&hg_g_toolbar_btn_font, FALSE);

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        RECT rc;
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        if (tb_width <= 0)
            tb_width = 1;

        int cols = get_items_per_row(tb_width, old_size);
        if (cols <= 0)
            cols = 1;

        /* Calculate exact width required for the SAME number of columns but NEW icon size */
        int exact_tb_width = (cols - 1) * (new_size + SC(15)) + new_size + SC(20);
        int new_w = exact_tb_width + border * 2;

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        load_shortcuts();
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }
    refresh_window_list(TRUE);
}

void update_edit_font_size(int delta)
{
    int old_size = ABS(hg_g_edit_font_size);
    if (old_size < SC(12))
        old_size = SC(12);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(12))
        new_size = SC(12);
    if (new_size > SC(128))
        new_size = SC(128);
    if (new_size == old_size)
        return;
    hg_g_edit_font_size = -new_size;
    save_taskbox_font_config();

    release_font_handle(&hg_g_main_font, TRUE);

    hg_g_main_font =
        CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!hg_g_main_font)
        hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if (hg_g_edit_msg_wnd) {
        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        InvalidateRect(hg_g_edit_msg_wnd, NULL, TRUE);
    }

    if (hg_g_tooltip_wnd) {
        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
    }

    /* The about edit also uses hg_g_main_font; without this it keeps the destroyed handle. */
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        HWND about_edit = GetDlgItem(hg_g_about_wnd, 100);
        if (about_edit) {
            SendMessageW(about_edit, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        }
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }

    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hwnd && IsWindow(hg_g_monitors[i].hwnd)) {
            HWND edit_wnd = GetDlgItem(hg_g_monitors[i].hwnd, 104);
            if (edit_wnd) {
                SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                /* Trigger WM_SIZE to recalculate edit box height */
                RECT rc;
                GetClientRect(hg_g_monitors[i].hwnd, &rc);
                SendMessageW(hg_g_monitors[i].hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
                InvalidateRect(hg_g_monitors[i].hwnd, NULL, TRUE);
            }
        }
    }
}

void update_taskbox_alpha(int delta)
{
    int new_alpha = (int)hg_g_taskbox_alpha + (delta > 0 ? 15 : -15);
    if (new_alpha > HG_MAX_ALPHA)
        new_alpha = HG_MAX_ALPHA;
    if (new_alpha < HG_MIN_ALPHA)
        new_alpha = HG_MIN_ALPHA;
    if (hg_g_taskbox_alpha == (BYTE)new_alpha)
        return;
    hg_g_taskbox_alpha = (BYTE)new_alpha;
    if (hg_g_taskbox_wnd) {
        SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    }
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    save_alpha_config();
}

void set_taskbox_opacity_pct(int pct)
{
    if (pct < 30)
        pct = 30;
    if (pct > 100)
        pct = 100;
    int alpha = (pct * 255 + 50) / 100;
    if (alpha < HG_MIN_ALPHA)
        alpha = HG_MIN_ALPHA;
    if (alpha > HG_MAX_ALPHA)
        alpha = HG_MAX_ALPHA;
    hg_g_taskbox_alpha = (BYTE)alpha;
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    }
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    save_alpha_config();
}

static int toolbar_clamp_percent(int pct)
{
    if (pct < 0)
        return 0;
    if (pct > 100)
        return 100;
    return pct;
}

static BYTE toolbar_scale_color_channel(int min_value, int max_value, int pct)
{
    int clamped = toolbar_clamp_percent(pct);
    int value = min_value + ((max_value - min_value) * clamped + 50) / 100;
    if (value < 0)
        value = 0;
    if (value > 255)
        value = 255;
    return (BYTE)value;
}

static COLORREF toolbar_invert_color(COLORREF color)
{
    return RGB(255 - GetRValue(color), 255 - GetGValue(color), 255 - GetBValue(color));
}

static int toolbar_taskbox_alpha_percent(void)
{
    return toolbar_clamp_percent((hg_g_taskbox_alpha * 100 + 127) / 255);
}

static int taskbox_toolbar_icon_size(void)
{
    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16))
        icon_size = SC(16);
    return icon_size;
}

typedef struct HgTaskboxDragState {
    BOOL is_dragging;
    int source_index;
    POINT start_pt;
    POINT current_pt;
    int target_index;
} HgTaskboxDragState;

typedef struct HgTaskboxFocusState {
    int area;
    int index;
} HgTaskboxFocusState;

typedef struct HgTaskboxLayoutState {
    RECT sizemove_start_rect;
} HgTaskboxLayoutState;

static HgTaskboxFocusState hg_taskbox_focus = {0, 0};

static COLORREF toolbar_basic_icon_bg_color(int index, COLORREF base_color)
{
    if (index == HG_TOOL_ICON_ALPHA) {
        int pct = toolbar_taskbox_alpha_percent();
        return RGB(toolbar_scale_color_channel(24, 255, pct), toolbar_scale_color_channel(0, 56, pct),
                   toolbar_scale_color_channel(0, 56, pct));
    }
    if (index == HG_TOOL_ICON_BRIGHTNESS) {
        int pct = get_system_brightness();
        return RGB(toolbar_scale_color_channel(0, 72, pct), toolbar_scale_color_channel(24, 255, pct),
                   toolbar_scale_color_channel(0, 72, pct));
    }
    if (index == HG_TOOL_ICON_VOLUME) {
        int pct = get_system_volume();
        return RGB(toolbar_scale_color_channel(0, 72, pct), toolbar_scale_color_channel(0, 120, pct),
                   toolbar_scale_color_channel(24, 255, pct));
    }
    return toolbar_invert_color(base_color);
}

static void toolbar_draw_muted_border(HDC hdc, const RECT *rc)
{
    if (!hdc || !rc)
        return;

    HBRUSH hbr = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
    if (!hbr)
        return;

    RECT border_rc = *rc;
    int thickness = SC(3);
    if (thickness < 2)
        thickness = 2;
    for (int i = 0; i < thickness; ++i) {
        FrameRect(hdc, &border_rc, hbr);
        InflateRect(&border_rc, -1, -1);
        if (border_rc.left >= border_rc.right || border_rc.top >= border_rc.bottom)
            break;
    }

    DeleteObject(hbr);
}

/* Consecutive 100 ms hover-check ticks with the cursor outside the taskbox. */
static int s_hover_outside_ticks = 0;

void hide_taskbox(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    /* The hover-check timer must die on every hide path, not only its own collapse. */
    KillTimer(hwnd, HG_TIMER_HOVER_CHECK);
    s_hover_outside_ticks = 0;
    if (hg_g_floater_wnd) {
        RECT t_rc, f_rc;
        GetWindowRect(hwnd, &t_rc);
        GetWindowRect(hg_g_floater_wnd, &f_rc);
        int cx = t_rc.left + (t_rc.right - t_rc.left) / 2;
        int cy = t_rc.top + (t_rc.bottom - t_rc.top) / 2;
        int fw = f_rc.right - f_rc.left;
        int fh = f_rc.bottom - f_rc.top;
        SetWindowPos(hg_g_floater_wnd, HWND_TOPMOST, cx - fw / 2, cy - fh / 2, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        ShowWindow(hg_g_floater_wnd, SW_SHOW);
        /* Take foreground only while this process still owns it; the auto-collapse
         * path must not steal focus from the application the user switched to. */
        HWND fg_wnd = GetForegroundWindow();
        DWORD fg_pid = 0;
        if (fg_wnd)
            GetWindowThreadProcessId(fg_wnd, &fg_pid);
        if (!fg_wnd || fg_pid == GetCurrentProcessId()) {
            SetForegroundWindow(hg_g_floater_wnd);
        }
        save_floater_geometry_config(cx - fw / 2, cy - fh / 2, fw, fh);
    }
    ShowWindow(hwnd, SW_HIDE);
    load_shortcuts_if_changed();
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
}

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

static int taskbox_track_owned_popup_menu(HMENU h_menu, UINT flags, int x, int y, HWND owner)
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

static HMENU taskbox_create_main_popup_menu(void)
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

static void taskbox_dispatch_main_menu_command(UINT cmd)
{
    if (cmd != 0) {
        PostMessageW(hg_g_floater_wnd, WM_COMMAND, (WPARAM)cmd, 0);
    }
}

void activate_toolbar_item(int index)
{
    if (index < 0)
        return;
    switch (hg_toolbar_builtin_click_role(index)) {
    case HG_TOOLBAR_CLICK_NONE:
        break;
    case HG_TOOLBAR_CLICK_HIDE_TASKBOX:
        hide_taskbox(hg_g_taskbox_wnd);
        break;
    case HG_TOOLBAR_CLICK_TOGGLE_DESKTOP: {
        static BOOL is_desktop_shown = FALSE;
        is_desktop_shown = !is_desktop_shown;
        EnumWindows(minimize_restore_enum_proc, (LPARAM)is_desktop_shown);
        break;
    }
    case HG_TOOLBAR_CLICK_OPEN_MENU: {
        HMENU h_menu = taskbox_create_main_popup_menu();
        if (!h_menu)
            break;

        POINT pt;
        GetCursorPos(&pt);
        int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hg_g_taskbox_wnd);
        taskbox_dispatch_main_menu_command((UINT)cmd);
        break;
    }
    case HG_TOOLBAR_CLICK_SHOW_COMMANDBOX:
        show_commandbox_window();
        break;
    case HG_TOOLBAR_CLICK_TOGGLE_MUTE:
        set_system_mute(!get_system_mute());
        update_toolbar_tooltips(hg_g_toolbar_wnd);
        update_focus_message(1, HG_TOOL_ICON_VOLUME);
        if (hg_g_toolbar_wnd) {
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
        }
        break;
    case HG_TOOLBAR_CLICK_FLOATER_ADJUST:
        /* Collapse to the floater for size/alpha tuning: hover-expand is suppressed
         * while in this mode (Ctrl+Wheel resizes, Alt+Wheel changes opacity); a
         * click on the floater returns to the taskbox. */
        hg_g_floater_adjust_mode = TRUE;
        hide_taskbox(hg_g_taskbox_wnd);
        break;
    default:
        break;
    }

    if (index >= HG_NUM_BASIC_ICONS) {
        int s_idx = index - HG_NUM_BASIC_ICONS;
        if (s_idx >= 0 && s_idx < hg_g_shortcut_count) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts[s_idx].path, NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

void activate_taskbar_item(int index)
{
    if (index < 0 || index >= hg_g_window_count)
        return;
    HWND target = hg_g_window_items[index].hwnd;
    if (IsWindow(target)) {
        if (IsIconic(target))
            ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    }
}

void update_focus_message(int override_type, int override_index)
{
    if (override_type == -1 && override_index == -1) {
        return;
    }

    int type = (override_type == -2) ? hg_taskbox_focus.area : override_type;
    int index = (override_index == -2) ? hg_taskbox_focus.index : override_index;

    if (type == 0) {
        if (index >= 0 && index < hg_g_window_count) {
            append_message(hg_g_window_items[index].title);
        }
    } else if (type == 1) {
        int total_items = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        if (index >= 0 && index < total_items) {
            const WCHAR *focus_text = hg_toolbar_builtin_focus_text(index);
            if (focus_text) {
                append_message(focus_text);
            } else if (hg_toolbar_builtin_has_value(index)) {
                static WCHAR value_str[64];
                if (hg_toolbar_builtin_value_text(index, HG_TOOLBAR_TEXT_FOCUS, value_str, HG_ARRAYSIZE(value_str))) {
                    append_message(value_str);
                }
            } else {
                append_message(hg_g_shortcuts[index - HG_NUM_BASIC_ICONS].name);
            }
        }
    }
}

void reset_taskbox_focus(void)
{
    hg_taskbox_focus.area = 0;
    hg_taskbox_focus.index = 0;
    update_focus_message(-2, -2);
}

int get_item_at_pt(POINT pt, int width, int height, int icon_size, int *out_type, int *out_index)
{
    for (int i = 0; i < hg_g_window_count; i++) {
        RECT rc_item, rc_btn;
        get_toolbar_item_rect(0, i, width, height, icon_size, &rc_item);
        rc_btn = rc_item;
        InflateRect(&rc_btn, SC(4), SC(4));
        if (PtInRect(&rc_btn, pt)) {
            if (out_type)
                *out_type = 0;
            if (out_index)
                *out_index = i;
            return 1;
        }
    }
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    for (int i = 0; i < total_shortcuts; i++) {
        RECT rc_item, rc_btn;
        get_toolbar_item_rect(1, i, width, height, icon_size, &rc_item);
        rc_btn = rc_item;
        InflateRect(&rc_btn, SC(4), SC(4));
        if (PtInRect(&rc_btn, pt)) {
            if (out_type)
                *out_type = 1;
            if (out_index)
                *out_index = i;
            return 1;
        }
    }
    return 0;
}

static LRESULT toolbar_controller_on_paint(HWND hwnd, int hovered_type, int hovered_index, int pressed_type,
                                           int pressed_index, const HgTaskboxDragState *drag_state,
                                           int *cached_icon_size)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HgPaintBuffer paint_buffer;
        if (hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &paint_buffer)) {
            HDC mem_dc = paint_buffer.dc;

                COLORREF bg_color = HG_COLOR_BG_TOOLBAR;
                if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
                    bg_color = HG_COLOR_BG_FLASH;
                }
                HBRUSH hbr_bg = CreateSolidBrush(bg_color);
                if (hbr_bg) {
                    FillRect(mem_dc, &rc, hbr_bg);
                    DeleteObject(hbr_bg);
                }

                int icon_size = taskbox_toolbar_icon_size();

                if (icon_size != *cached_icon_size || !hg_g_toolbar_btn_font) {
                    release_font_handle(&hg_g_toolbar_btn_font, FALSE);
                    hg_g_toolbar_btn_font = CreateFontW(icon_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
                    *cached_icon_size = icon_size;
                }

                int visual_order[HG_MAX_WINDOW_ITEMS];
                for (int i = 0; i < hg_g_window_count; i++)
                    visual_order[i] = i;

                if (drag_state->is_dragging && drag_state->source_index != -1) {
                    int src = drag_state->source_index;
                    int dst = drag_state->target_index;
                    if (dst != -1 && dst != src) {
                        int temp = visual_order[src];
                        if (src < dst) {
                            for (int i = src; i < dst; i++)
                                visual_order[i] = visual_order[i + 1];
                        } else {
                            for (int i = src; i > dst; i--)
                                visual_order[i] = visual_order[i - 1];
                        }
                        visual_order[dst] = temp;
                    }
                }

                // Draw tasks (type 0)
                for (int pos = 0; pos < hg_g_window_count; pos++) {
                    int r_idx = visual_order[pos];
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(0, pos, rc.right, rc.bottom, icon_size, &rc_item);
                    rc_btn = rc_item;
                    InflateRect(&rc_btn, SC(4), SC(4));

                    if (drag_state->is_dragging && drag_state->source_index != -1 &&
                        r_idx == drag_state->source_index) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FrameRect(mem_dc, &rc_btn, hbr);
                            DeleteObject(hbr);
                        }
                        continue;
                    }

                    if (pressed_type == 0 && pressed_index == r_idx && !drag_state->is_dragging) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FillRect(mem_dc, &rc_btn, hbr);
                            DeleteObject(hbr);
                        }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 0 && hovered_index == pos && !drag_state->is_dragging) ||
                               (hg_taskbox_focus.area == 0 && hg_taskbox_focus.index == r_idx)) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FillRect(mem_dc, &rc_btn, hbr);
                            DeleteObject(hbr);
                        }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_taskbox_focus.area == 0 && hg_taskbox_focus.index == r_idx) {
                            HBRUSH hbr_focus = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
                            if (hbr_focus) {
                                FrameRect(mem_dc, &rc_btn, hbr_focus);
                                DeleteObject(hbr_focus);
                            }
                        }
                    }
                    if (hg_g_window_items[r_idx].icon) {
                        DrawIconEx(mem_dc, rc_item.left, rc_item.top, hg_g_window_items[r_idx].icon, icon_size,
                                   icon_size, 0, NULL, DI_NORMAL);
                    }

                    /* Draw active status indicator (elegant pill/dot under the active task's icon) */
                    if (hg_g_window_items[r_idx].hwnd == hg_g_prev_active_hwnd) {
                        int dot_w = SC(6);
                        if (dot_w < 4)
                            dot_w = 4;
                        int dot_h = SC(3);
                        if (dot_h < 2)
                            dot_h = 2;
                        int dot_x = rc_item.left + (icon_size - dot_w) / 2;
                        int dot_y = rc_item.bottom + SC(1);
                        RECT dot_rc = {dot_x, dot_y, dot_x + dot_w, dot_y + dot_h};
                        HBRUSH hbr_dot = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
                        if (hbr_dot) {
                            FillRect(mem_dc, &dot_rc, hbr_dot);
                            DeleteObject(hbr_dot);
                        }
                    }
                }

                // Draw hg_g_shortcuts (type 1)
                int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
                for (int i = 0; i < total_shortcuts; i++) {
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(1, i, rc.right, rc.bottom, icon_size, &rc_item);
                    rc_btn = rc_item;
                    InflateRect(&rc_btn, SC(4), SC(4));

                    COLORREF button_bg = (i < HG_NUM_BASIC_ICONS) ? toolbar_basic_icon_bg_color(i, bg_color)
                                                                  : toolbar_invert_color(bg_color);
                    BOOL keep_value_bg = (i == HG_TOOL_ICON_ALPHA || i == HG_TOOL_ICON_BRIGHTNESS ||
                                          i == HG_TOOL_ICON_VOLUME);
                    HBRUSH hbr_opp = CreateSolidBrush(button_bg);
                    if (hbr_opp) {
                        FillRect(mem_dc, &rc_btn, hbr_opp);
                        DeleteObject(hbr_opp);
                    }

                    if (pressed_type == 1 && pressed_index == i) {
                        if (!keep_value_bg) {
                            HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                            if (hbr) {
                                FillRect(mem_dc, &rc_btn, hbr);
                                DeleteObject(hbr);
                            }
                        }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 1 && hovered_index == i) ||
                               (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == i)) {
                        if (!keep_value_bg) {
                            HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                            if (hbr) {
                                FillRect(mem_dc, &rc_btn, hbr);
                                DeleteObject(hbr);
                            }
                        }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == i) {
                            HBRUSH hbr_focus = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
                            if (hbr_focus) {
                                FrameRect(mem_dc, &rc_btn, hbr_focus);
                                DeleteObject(hbr_focus);
                            }
                        }
                    }

                    if (i == HG_TOOL_ICON_VOLUME && get_system_mute()) {
                        toolbar_draw_muted_border(mem_dc, &rc_btn);
                    }

                    if (i >= HG_NUM_BASIC_ICONS) {
                        int s_idx = i - HG_NUM_BASIC_ICONS;
                        if (hg_g_shortcuts[s_idx].icon) {
                            DrawIconEx(mem_dc, rc_item.left, rc_item.top, hg_g_shortcuts[s_idx].icon, icon_size,
                                       icon_size, 0, NULL, DI_NORMAL);
                        }
                    } else if (hg_g_toolbar_btn_font) {
                        SetTextColor(mem_dc, HG_COLOR_TEXT_DEFAULT);
                        SetBkMode(mem_dc, TRANSPARENT);
                        HFONT old_font = (HFONT)SelectObject(mem_dc, hg_g_toolbar_btn_font);
                        WCHAR btn_text[2] = {hg_toolbar_builtin_label(i), 0};
                        draw_outlined_text(mem_dc, btn_text, 1, &rc_item, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                                           HG_COLOR_TEXT_DEFAULT, HG_COLOR_BG_DEFAULT);
                        SelectObject(mem_dc, old_font);
                    }
                }

                if (drag_state->is_dragging && drag_state->source_index != -1 &&
                    drag_state->source_index < hg_g_window_count) {
                    int cx = drag_state->current_pt.x - icon_size / 2;
                    int cy = drag_state->current_pt.y - icon_size / 2;
                    RECT drag_rc = {cx - SC(4), cy - SC(4), cx + icon_size + SC(4), cy + icon_size + SC(4)};
                    HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                    if (hbr) {
                        FillRect(mem_dc, &drag_rc, hbr);
                        DeleteObject(hbr);
                    }
                    DrawEdge(mem_dc, &drag_rc, BDR_RAISEDINNER, BF_RECT);
                    if (hg_g_window_items[drag_state->source_index].icon) {
                        DrawIconEx(mem_dc, cx, cy, hg_g_window_items[drag_state->source_index].icon, icon_size, icon_size,
                                   0, NULL, DI_NORMAL);
                    }
                }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
            hg_paint_buffer_end(&paint_buffer);
        }
    }
    EndPaint(hwnd, &ps);
    return 0;
}

static LRESULT toolbar_controller_on_mouse_move(HWND hwnd, ToolbarControllerState *state,
                                                HgTaskboxDragState *drag_state, LPARAM l_param)
{
    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
    RECT rc;
    GetClientRect(hwnd, &rc);
    int icon_size = taskbox_toolbar_icon_size();

    if (state->is_resizing && GetCapture() == hwnd) {
        POINT cur_mouse;
        GetCursorPos(&cur_mouse);
        int dw = cur_mouse.x - state->start_mouse.x;
        int dh = cur_mouse.y - state->start_mouse.y;
        int border = SC(HG_BORDER_THICKNESS);

        int total_tasks = hg_g_window_count;
        int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        int total_items = total_tasks + total_shortcuts;
        if (total_items <= 0)
            total_items = 1;

        int cols = 1;
        if (ABS(dh) > ABS(dw)) {
            int new_height = (state->start_rect.bottom - state->start_rect.top) + dh;
            if (new_height < SC(HG_MIN_WINDOW_HEIGHT))
                new_height = SC(HG_MIN_WINDOW_HEIGHT);

            int edit_height = SC(20);
            if (hg_g_edit_msg_wnd && hg_g_main_font) {
                HDC hdc = GetDC(hg_g_edit_msg_wnd);
                if (hdc) {
                    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                    TEXTMETRIC tm = {0};
                    GetTextMetrics(hdc, &tm);
                    edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
                    SelectObject(hdc, old_font);
                    ReleaseDC(hg_g_edit_msg_wnd, hdc);
                }
            }
            int row_height = icon_size + SC(10);
            int available_toolbar_h = new_height - (border * 2 + edit_height);
            int target_rows = (available_toolbar_h - SC(10) + row_height / 2) / row_height;
            if (target_rows < 1)
                target_rows = 1;
            if (target_rows > total_items)
                target_rows = total_items;
            cols = (total_items + target_rows - 1) / target_rows;
        } else {
            int new_width = (state->start_rect.right - state->start_rect.left) + dw;
            if (new_width < SC(HG_MIN_WINDOW_WIDTH))
                new_width = SC(HG_MIN_WINDOW_WIDTH);
            int tb_width = new_width - (border * 2);
            if (tb_width <= 0)
                tb_width = 1;
            cols = get_items_per_row(tb_width, icon_size);
        }
        if (cols <= 0)
            cols = 1;

        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
        int req_w = exact_tb_width + border * 2;

        int rows = (total_items + cols - 1) / cols;
        if (rows <= 0)
            rows = 1;
        int row_height = icon_size + SC(10);
        int req_toolbar_height = SC(10) + rows * row_height;

        int edit_height = SC(20);
        if (hg_g_edit_msg_wnd && hg_g_main_font) {
            HDC hdc = GetDC(hg_g_edit_msg_wnd);
            if (hdc) {
                HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                TEXTMETRIC tm = {0};
                GetTextMetrics(hdc, &tm);
                edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
                SelectObject(hdc, old_font);
                ReleaseDC(hg_g_edit_msg_wnd, hdc);
            }
        }
        int req_h = border * 2 + edit_height + req_toolbar_height;

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, req_w, req_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    if (state->is_moving_taskbox && GetCapture() == hwnd) {
        POINT cur_mouse;
        GetCursorPos(&cur_mouse);
        int dx = cur_mouse.x - state->start_mouse.x;
        int dy = cur_mouse.y - state->start_mouse.y;
        if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
            SetWindowPos(hg_g_taskbox_wnd, NULL, state->start_rect.left + dx, state->start_rect.top + dy, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }

    int cur_type = -1, cur_index = -1;
    get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index);

    if (drag_state->source_index != -1 && !drag_state->is_dragging && GetCapture() == hwnd &&
        state->pressed_type == 0) {
        if (ABS(pt.x - drag_state->start_pt.x) > GetSystemMetrics(SM_CXDRAG) ||
            ABS(pt.y - drag_state->start_pt.y) > GetSystemMetrics(SM_CYDRAG)) {
            drag_state->is_dragging = TRUE;
        }
    }

    if (drag_state->is_dragging && drag_state->source_index != -1) {
        drag_state->current_pt = pt;
        if (cur_type == 0 && cur_index != -1) {
            drag_state->target_index = cur_index;
        } else if (cur_type == -1 || cur_type == 1) {
            drag_state->target_index = -1;
        }
        InvalidateRect(hwnd, NULL, FALSE);
    } else if (cur_type != state->hovered_type || cur_index != state->hovered_index) {
        state->hovered_type = cur_type;
        state->hovered_index = cur_index;
        update_focus_message(state->hovered_type, state->hovered_index);
        InvalidateRect(hwnd, NULL, FALSE);
    }

    TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
    TrackMouseEvent(&tme);
    return 0;
}

static LRESULT toolbar_controller_on_lbutton_up(HWND hwnd, ToolbarControllerState *state,
                                                HgTaskboxDragState *drag_state, LPARAM l_param)
{
    if (state->is_resizing) {
        state->is_resizing = FALSE;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        /* 크기 조절 완료 시 불필요한 우측 여백을 제거하고 정확하게 스냅되도록 함 */
        RECT rc;
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        int icon_size = taskbox_toolbar_icon_size();
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        int cols = get_items_per_row(tb_width, icon_size);
        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
        int new_w = exact_tb_width + border * 2;
        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        GetWindowRect(hg_g_taskbox_wnd, &rc);
        save_taskbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        if (hg_g_floater_wnd) {
            SetWindowPos(hg_g_floater_wnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        }
    } else if (state->is_moving_taskbox) {
        state->is_moving_taskbox = FALSE;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        RECT rc;
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        save_taskbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        if (hg_g_floater_wnd) {
            SetWindowPos(hg_g_floater_wnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        }
    } else if (drag_state->source_index != -1) {
        BOOL was_dragging = drag_state->is_dragging;
        int final_source = drag_state->source_index;
        int final_target = drag_state->target_index;
        drag_state->source_index = -1;
        drag_state->target_index = -1;
        state->pressed_type = -1;
        state->pressed_index = -1;
        drag_state->is_dragging = FALSE;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        if (!was_dragging) {
            POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            RECT rc;
            GetClientRect(hwnd, &rc);
            int icon_size = taskbox_toolbar_icon_size();
            int cur_type = -1, cur_index = -1;
            if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
                if (cur_type == 0 && cur_index == final_source) {
                    activate_taskbar_item(cur_index);
                }
            }
        } else {
            if (final_target != -1 && final_target != final_source) {
                WindowItem temp = hg_g_window_items[final_source];
                if (final_source < final_target) {
                    for (int i = final_source; i < final_target; i++) {
                        hg_g_window_items[i] = hg_g_window_items[i + 1];
                    }
                } else {
                    for (int i = final_source; i > final_target; i--) {
                        hg_g_window_items[i] = hg_g_window_items[i - 1];
                    }
                }
                hg_g_window_items[final_target] = temp;
                hg_taskbox_focus.index = final_target;
                update_toolbar_tooltips(hwnd);
            }
        }
    } else if (state->pressed_type == 1 && state->pressed_index != -1) {
        int cur_index = state->pressed_index;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        activate_toolbar_item(cur_index);
    } else {
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
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

static void toolbar_controller_show_task_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param)
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

static void toolbar_controller_show_shortcut_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param)
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

static BOOL taskbox_handle_audio_menu_command(UINT cmd)
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

static void toolbar_controller_show_audio_context_menu(HWND hwnd, int icon_size, LPARAM l_param)
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

static LRESULT toolbar_controller_on_lbutton_down(HWND hwnd, ToolbarControllerState *state,
                                                  HgTaskboxDragState *drag_state, LPARAM l_param)
{
    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = taskbox_toolbar_icon_size();

    int cur_type = -1, cur_index = -1;
    if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
        state->hovered_type = cur_type;
        state->hovered_index = cur_index;
        hg_taskbox_focus.area = cur_type;
        hg_taskbox_focus.index = cur_index;
        state->pressed_type = cur_type;
        state->pressed_index = cur_index;

        HgToolbarDragRole drag_role = (cur_type == 1) ? hg_toolbar_builtin_drag_role(cur_index) : HG_TOOLBAR_DRAG_NONE;
        if (drag_role == HG_TOOLBAR_DRAG_RESIZE_TASKBOX) {
            state->is_resizing = TRUE;
            GetCursorPos(&state->start_mouse);
            GetWindowRect(hg_g_taskbox_wnd, &state->start_rect);
        } else if (drag_role == HG_TOOLBAR_DRAG_MOVE_TASKBOX) {
            state->is_moving_taskbox = TRUE;
            GetCursorPos(&state->start_mouse);
            GetWindowRect(hg_g_taskbox_wnd, &state->start_rect);
        } else if (cur_type == 0) {
            drag_state->is_dragging = FALSE;
            drag_state->source_index = cur_index;
            drag_state->start_pt = pt;
            drag_state->current_pt = pt;
            drag_state->target_index = -1;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        SetCapture(hwnd);
    }
    return 0;
}

static LRESULT toolbar_controller_on_rbutton_up(HWND hwnd, LPARAM l_param)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = taskbox_toolbar_icon_size();

    int cur_type = -1, cur_index = -1;
    if (l_param == 0) {
        cur_type = hg_taskbox_focus.area;
        cur_index = hg_taskbox_focus.index;
    } else {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index);
    }

    if (cur_type == 0 && cur_index != -1) {
        toolbar_controller_show_task_context_menu(hwnd, cur_index, icon_size, l_param);
    } else if (cur_type == 1 && cur_index != -1) {
        if (cur_index == HG_TOOL_ICON_VOLUME) {
            toolbar_controller_show_audio_context_menu(hwnd, icon_size, l_param);
        } else {
            toolbar_controller_show_shortcut_context_menu(hwnd, cur_index, icon_size, l_param);
        }
    }

    return 0;
}

static LRESULT toolbar_controller_on_mbutton_up(HWND hwnd, LPARAM l_param)
{
    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = taskbox_toolbar_icon_size();

    int cur_type = -1, cur_index = -1;
    if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
        if (cur_type == 0) {
            HWND target = hg_g_window_items[cur_index].hwnd;
            if (IsWindow(target)) {
                PostMessageW(target, WM_CLOSE, 0, 0);
            }
        }
    }
    return 0;
}

static LRESULT toolbar_controller_on_mouse_leave(HWND hwnd, ToolbarControllerState *state,
                                                 HgTaskboxDragState *drag_state)
{
    state->hovered_type = -1;
    state->hovered_index = -1;
    if (!state->is_resizing && !state->is_moving_taskbox && !drag_state->is_dragging) {
        state->pressed_type = -1;
        state->pressed_index = -1;
        /* A pending (not yet started) reorder drag must not survive the pointer
         * leaving, or the next unrelated button-up reuses its source index. */
        drag_state->source_index = -1;
        drag_state->target_index = -1;
        if (GetCapture() == hwnd) {
            ReleaseCapture();
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
    return 0;
}

static int toolbar_value_current_percent(int index)
{
    if (index == HG_TOOL_ICON_ALPHA)
        return toolbar_taskbox_alpha_percent();
    if (index == HG_TOOL_ICON_BRIGHTNESS)
        return get_system_brightness();
    if (index == HG_TOOL_ICON_VOLUME)
        return get_system_volume();
    return 0;
}

static int toolbar_value_min_percent(int index)
{
    if (index == HG_TOOL_ICON_ALPHA)
        return (HG_MIN_ALPHA * 100 + 127) / 255;
    return 0;
}

static int toolbar_value_next_percent(int index, int current, short delta)
{
    int next = current;
    if (delta > 0) {
        if (current % 5 == 0) {
            next = current + 5;
        } else {
            next = ((current / 5) + 1) * 5;
        }
        if (next > 100)
            next = 100;
    } else {
        if (current % 5 == 0) {
            next = current - 5;
        } else {
            next = (current / 5) * 5;
        }
        int min_value = toolbar_value_min_percent(index);
        if (next < min_value)
            next = min_value;
    }
    return next;
}

static void toolbar_value_apply_percent(int index, int value)
{
    if (index == HG_TOOL_ICON_ALPHA) {
        set_taskbox_opacity_pct(value);
    } else if (index == HG_TOOL_ICON_BRIGHTNESS) {
        set_system_brightness(value);
    } else if (index == HG_TOOL_ICON_VOLUME) {
        set_system_volume(value);
    }
}

static void toolbar_update_value_tooltip(HWND hwnd, int index)
{
    if (!hg_g_tooltip_wnd)
        return;

    TOOLINFOW ti = {0};
    ti.cbSize = TOOLINFO_V1_SIZE;
    ti.hwnd = hwnd;
    ti.uId = (UINT_PTR)(hg_g_window_count + index);
    static WCHAR value_tips[HG_NUM_BASIC_ICONS][64];
    if (index >= 0 && index < HG_NUM_BASIC_ICONS &&
        hg_toolbar_builtin_value_text(index, HG_TOOLBAR_TEXT_TOOLTIP, value_tips[index],
                                      HG_ARRAYSIZE(value_tips[index]))) {
        ti.lpszText = value_tips[index];
        SendMessageW(hg_g_tooltip_wnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    }
}

static LRESULT toolbar_controller_on_mouse_wheel(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    if (LOWORD(w_param) & MK_CONTROL) {
        short delta = (short)HIWORD(w_param);
        update_size(delta > 0 ? 1 : -1);
        return 0;
    }
    if (GetKeyState(VK_MENU) < 0) {
        return SendMessageW(GetParent(hwnd), WM_MOUSEWHEEL, w_param, l_param);
    }

    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
    ScreenToClient(hwnd, &pt);

    RECT rc_client;
    GetClientRect(hwnd, &rc_client);

    int icon_size = taskbox_toolbar_icon_size();

    for (int i = 0; i < HG_NUM_BASIC_ICONS; ++i) {
        if (!hg_toolbar_builtin_has_value(i))
            continue;

        RECT rc_icon;
        get_toolbar_item_rect(1, i, rc_client.right, rc_client.bottom, icon_size, &rc_icon);
        InflateRect(&rc_icon, SC(4), SC(4));
        if (PtInRect(&rc_icon, pt)) {
            short delta = (short)HIWORD(w_param);
            int cur_value = toolbar_value_current_percent(i);
            int new_value = toolbar_value_next_percent(i, cur_value, delta);
            toolbar_value_apply_percent(i, new_value);

            update_toolbar_tooltips(hwnd);
            toolbar_update_value_tooltip(hwnd, i);
            update_focus_message(1, i);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    }

    return 0;
}

LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static ToolbarControllerState state = {-1, -1, -1, -1, 0, FALSE, FALSE, {0, 0}, {0, 0, 0, 0}};
    static HgTaskboxDragState drag_state = {FALSE, -1, {0, 0}, {0, 0}, -1};

    switch (msg) {
    case WM_NCHITTEST: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(hwnd, &pt);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int icon_size = taskbox_toolbar_icon_size();
        if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, NULL, NULL))
            return HTCLIENT;
        return HTTRANSPARENT;
    }
    case WM_SIZE: {
        update_toolbar_tooltips(hwnd);
        return 0;
    }
    case WM_PAINT:
        return toolbar_controller_on_paint(hwnd, state.hovered_type, state.hovered_index, state.pressed_type,
                                           state.pressed_index, &drag_state, &state.cached_icon_size);
    case WM_KEYDOWN: {
        SendMessage(GetParent(hwnd), WM_KEYDOWN, w_param, l_param);
        return 0;
    }
    case WM_LBUTTONDOWN:
        return toolbar_controller_on_lbutton_down(hwnd, &state, &drag_state, l_param);
    case WM_MOUSEMOVE:
        return toolbar_controller_on_mouse_move(hwnd, &state, &drag_state, l_param);
    case WM_LBUTTONUP:
        return toolbar_controller_on_lbutton_up(hwnd, &state, &drag_state, l_param);
    case WM_RBUTTONUP:
        return toolbar_controller_on_rbutton_up(hwnd, l_param);
    case WM_MBUTTONUP:
        return toolbar_controller_on_mbutton_up(hwnd, l_param);
    case WM_MOUSELEAVE:
        return toolbar_controller_on_mouse_leave(hwnd, &state, &drag_state);
    case WM_MOUSEWHEEL:
        return toolbar_controller_on_mouse_wheel(hwnd, w_param, l_param);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

BOOL get_explorer_path(HWND target_hwnd, WCHAR *out_path, int max_len)
{
    if (!out_path || max_len <= 0)
        return FALSE;
    out_path[0] = L'\0';
    IShellWindows *psw = NULL;
    if (FAILED(CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, (void **)&psw)))
        return FALSE;

    long count = 0;
    if (FAILED(psw->lpVtbl->get_Count(psw, &count))) {
        HG_RELEASE_COM(psw);
        return FALSE;
    }

    BOOL found = FALSE;
    for (long i = 0; i < count; i++) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = i;
        IDispatch *pdisp = NULL;
        if (SUCCEEDED(psw->lpVtbl->Item(psw, v, &pdisp)) && pdisp) {
            IWebBrowser2 *pwb = NULL;
            if (SUCCEEDED(pdisp->lpVtbl->QueryInterface(pdisp, &IID_IWebBrowser2, (void **)&pwb)) && pwb) {
                /* Try reading hw as long or INT_PTR depending on headers. SHANDLE_PTR is standard */
                LONG_PTR hw = 0;
                if (SUCCEEDED(pwb->lpVtbl->get_HWND(pwb, &hw)) && (HWND)hw == target_hwnd) {
                    BSTR url = NULL;
                    if (SUCCEEDED(pwb->lpVtbl->get_LocationURL(pwb, &url)) && url) {
                        DWORD pc_len = (DWORD)max_len;
                        if (SUCCEEDED(PathCreateFromUrlW(url, out_path, &pc_len, 0))) {
                            found = TRUE;
                        } else {
                            /* Might be search-ms: or other specialized protocol; fallback */
                            BSTR title = NULL;
                            if (SUCCEEDED(pwb->lpVtbl->get_LocationName(pwb, &title)) && title) {
                                lstrcpynW(out_path, title, max_len);
                                release_bstr(&title);
                            } else {
                                lstrcpynW(out_path, url, max_len);
                            }
                            found = TRUE;
                        }
                        release_bstr(&url);
                    }
                }
                HG_RELEASE_COM(pwb);
            }
            HG_RELEASE_COM(pdisp);
        }
        if (found)
            break;
    }
    HG_RELEASE_COM(psw);
    return found;
}

void refresh_window_list(BOOL force)
{
    int new_count = 0;
    ZeroMemory(hg_g_new_items, sizeof(hg_g_new_items));

    /* 1단계: 기존 창 유효성 체크 및 아이콘 재사용 */
    for (int i = 0; i < hg_g_window_count; i++) {
        if (new_count >= HG_MAX_WINDOW_ITEMS)
            break;

        if (IsWindow(hg_g_window_items[i].hwnd) && is_alt_tab_window(hg_g_window_items[i].hwnd)) {
            hg_g_new_items[new_count] = (WindowItem){0};
            hg_g_new_items[new_count].hwnd = hg_g_window_items[i].hwnd;
            if (force) {
                release_window_item_icon(&hg_g_window_items[i]);
                hg_g_new_items[new_count].icon = get_window_icon(hg_g_window_items[i].hwnd, ABS(hg_g_current_font_size),
                                                                 &hg_g_new_items[new_count].own_icon);
            } else {
                hg_g_new_items[new_count].icon = hg_g_window_items[i].icon;
                hg_g_new_items[new_count].own_icon = hg_g_window_items[i].own_icon;
                hg_g_window_items[i].icon = NULL;
                hg_g_window_items[i].own_icon = FALSE;
            }

            hg_g_new_items[new_count].exists = TRUE;
            StringCchCopyW(hg_g_new_items[new_count].process_name, HG_MAX_STR, hg_g_window_items[i].process_name);
            hg_g_new_items[new_count].process_id = hg_g_window_items[i].process_id;

            if (GetWindowTextW(hg_g_new_items[new_count].hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                hg_g_new_items[new_count].title[0] = L'\0';
            }

            if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                WCHAR path[HG_MAX_PATH];
                if (get_explorer_path(hg_g_new_items[new_count].hwnd, path, HG_MAX_PATH)) {
                    StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                }
            }

            new_count++;
        } else {
            release_window_item_icon(&hg_g_window_items[i]);
        }
    }

    /* 2단계: 새로 나타난 창 추가 */
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        if (is_alt_tab_window(hwnd)) {
            BOOL exists = FALSE;
            for (int i = 0; i < new_count; i++) {
                if (hg_g_new_items[i].hwnd == hwnd) {
                    exists = TRUE;
                    break;
                }
            }

            if (!exists) {
                if (new_count >= HG_MAX_WINDOW_ITEMS)
                    break;

                hg_g_new_items[new_count] = (WindowItem){0};
                hg_g_new_items[new_count].hwnd = hwnd;
                hg_g_new_items[new_count].icon =
                    get_window_icon(hwnd, ABS(hg_g_current_font_size), &hg_g_new_items[new_count].own_icon);
                hg_g_new_items[new_count].exists = TRUE;
                get_process_name_by_hwnd(hwnd, hg_g_new_items[new_count].process_name, HG_MAX_STR,
                                         &hg_g_new_items[new_count].process_id);

                if (GetWindowTextW(hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                    hg_g_new_items[new_count].title[0] = L'\0';
                }

                if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                    WCHAR path[HG_MAX_PATH];
                    if (get_explorer_path(hwnd, path, HG_MAX_PATH)) {
                        StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                    }
                }

                new_count++;
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    BOOL changed = force || (new_count != hg_g_window_count);
    if (!changed) {
        for (int i = 0; i < hg_g_window_count; i++) {
            if (hg_g_new_items[i].hwnd != hg_g_window_items[i].hwnd ||
                lstrcmpW(hg_g_new_items[i].title, hg_g_window_items[i].title) != 0) {
                changed = TRUE;
                break;
            }
        }
    }

    hg_g_window_count = new_count;
    for (int i = 0; i < hg_g_window_count; i++) {
        hg_g_window_items[i] = hg_g_new_items[i];
    }

    if (changed && hg_g_taskbox_wnd) {
        update_layout(hg_g_taskbox_wnd);
        if (hg_g_toolbar_wnd)
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
}

LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass,
                                    DWORD_PTR dw_ref_data)
{
    switch (msg) {
    case WM_SETFOCUS: {
        disable_window_ime(hwnd);
        break;
    }
    case WM_IME_SETCONTEXT:
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
        if (readonly_edit_handle_ime_messages(hwnd, msg, w_param)) {
            return 0;
        }
        break;
    case WM_LBUTTONDOWN: {
        ReleaseCapture();
        SendMessageW(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
        return 0;
    }
    case WM_KEYDOWN: {
        if (w_param == VK_F2) {
            if (hg_g_floater_wnd)
                PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        if (LOWORD(w_param) & MK_CONTROL) {
            short delta = (short)HIWORD(w_param);
            update_edit_font_size(delta > 0 ? 1 : -1);
            return 0;
        } else if (GetKeyState(VK_MENU) < 0) {
            short delta = (short)HIWORD(w_param);
            update_taskbox_alpha(delta > 0 ? 1 : -1);
            return 0;
        }
        break;
    }
    case WM_CONTEXTMENU: {
        HMENU h_menu = CreatePopupMenu();
        if (!h_menu)
            return 0;
        AppendMenuW(h_menu, MF_STRING, HG_IDM_EDIT_COPYALL, L"Copy All (&A)");

        POINT pt;
        if (l_param == (LPARAM)-1) { /* Keyboard shortcut */
            RECT rc;
            GetWindowRect(hwnd, &rc);
            pt.x = rc.left + 5;
            pt.y = rc.top + 5;
        } else {
            pt.x = GET_X_LPARAM(l_param);
            pt.y = GET_Y_LPARAM(l_param);
        }

        int cmd = taskbox_track_owned_popup_menu(h_menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y,
                                                 hwnd);

        if (cmd == HG_IDM_EDIT_COPYALL) {
            SendMessageW(hwnd, EM_SETSEL, 0, (LPARAM)-1);
            SendMessageW(hwnd, WM_COPY, 0, 0);
            SendMessageW(hwnd, EM_SETSEL, (WPARAM)-1, 0);
        }

        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

void update_layout(HWND hwnd)
{
    if (!hwnd || !hg_g_edit_msg_wnd || !hg_g_toolbar_wnd || !hg_g_main_font)
        return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right;
    int border = SC(HG_BORDER_THICKNESS);

    /* Get edit control height */
    HDC hdc = GetDC(hg_g_edit_msg_wnd);
    if (!hdc)
        return;
    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
    TEXTMETRIC tm = {0};
    GetTextMetrics(hdc, &tm);
    int edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
    SelectObject(hdc, old_font);
    ReleaseDC(hg_g_edit_msg_wnd, hdc);

    int tb_width = width - (border * 2);
    if (tb_width <= 0)
        tb_width = 1;

    /* Calculate necessary height for the taskbox based on columns and rows */
    int icon_size = taskbox_toolbar_icon_size();
    int cols = get_items_per_row(tb_width, icon_size);
    if (cols <= 0)
        cols = 1;
    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    int rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (rows <= 0)
        rows = 1;

    int row_height = icon_size + SC(10);

    RECT win_rc;
    GetWindowRect(hwnd, &win_rc);
    int current_height = win_rc.bottom - win_rc.top;
    int current_width = win_rc.right - win_rc.left;

    int req_toolbar_height = SC(10) + rows * row_height;
    int required_total_height = border * 2 + edit_height + req_toolbar_height;

    int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
    int required_total_width = border * 2 + exact_tb_width;

    /* If the current window size is different from required, resize it */
    if (!hg_g_in_sizemove) {
        if (current_height != required_total_height || current_width != required_total_width) {
            SetWindowPos(hwnd, NULL, 0, 0, required_total_width, required_total_height,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return; /* SetWindowPos will trigger WM_SIZE, calling update_layout again */
        }
    }

    MoveWindow(hg_g_edit_msg_wnd, border, border, exact_tb_width, edit_height, TRUE);
    MoveWindow(hg_g_toolbar_wnd, border, border + edit_height, exact_tb_width, req_toolbar_height, TRUE);
    update_toolbar_tooltips(hg_g_toolbar_wnd);
}

static LRESULT taskbox_controller_on_create(HWND hwnd)
{
    /* 툴바 클래스 등록 */
    WNDCLASSW twc = {0};
    twc.style = CS_HREDRAW | CS_VREDRAW;
    twc.lpfnWndProc = toolbar_proc;
    twc.hInstance = GetModuleHandle(NULL);
    twc.lpszClassName = L"hgtoolbar_class";
    twc.hCursor = LoadCursor(NULL, IDC_HAND);
    RegisterClassW(&twc);

    /* UI용 일반 폰트 생성 (아이콘 크기와 분리) */
    hg_g_main_font =
        CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!hg_g_main_font)
        hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    hg_g_toolbar_wnd = CreateWindowExW(0, L"hgtoolbar_class", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                                       (HMENU)HG_IDC_TOOLBAR, GetModuleHandle(NULL), NULL);

    hg_g_edit_msg_wnd =
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY, 0, 0, 0,
                        0, hwnd, (HMENU)HG_IDC_EDIT_MSG, GetModuleHandle(NULL), NULL);
    if (hg_g_edit_msg_wnd) {
        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        SetWindowTextW(hg_g_edit_msg_wnd,
                       L"RClick: Settings | Ctrl+Arrow/Wheel: Grid/Size | Alt+Arrow/Wheel: Move/Alpha");
        SetWindowSubclass(hg_g_edit_msg_wnd, edit_subclass_proc, 0, 0);
        disable_window_ime(hg_g_edit_msg_wnd);
    }

    /* 툴팁 생성: 메인 윈도우를 소유자로 지정하되 TOPMOST 유지 */
    hg_g_tooltip_wnd =
        CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, GetModuleHandle(NULL), NULL);

    if (hg_g_tooltip_wnd) {
        SendMessageW(hg_g_tooltip_wnd, TTM_SETMAXTIPWIDTH, 0, SC(1000));
        /* 툴팁 폰트도 DPI 배율이 적용된 폰트로 설정 */
        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        /* 즉시 표시되도록 설정 (약간의 지연 시간 부여) */
        SendMessageW(hg_g_tooltip_wnd, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
        /* 시스템 테마를 해제하여 설정한 폰트와 컬러가 적용되게 함 */
        SendMessageW(hg_g_tooltip_wnd, CCM_SETWINDOWTHEME, 0, (LPARAM)L"");
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
    }

    /* taskbar_wnd creation removed */

    load_shortcuts();
    refresh_window_list(TRUE);
    update_toolbar_tooltips(hg_g_toolbar_wnd);

    /* 창 테두리 및 모서리 등 DWM 속성 설정 */
    apply_dwm_attributes(hwnd);

    SetLayeredWindowAttributes(hwnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
    SetTimer(hwnd, HG_TIMER_TASKBOX_REFRESH, 1000, NULL); /* 1초마다 시계 및 목록 갱신 */
    return 0;
}

static LRESULT taskbox_controller_on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    /* 하이라이트 효과 (깜빡임) */
    COLORREF bg_color = HG_CLICKABLE_BG;
    if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
        bg_color = HG_COLOR_BG_FLASH;
    }
    HBRUSH hbr_bg = CreateSolidBrush(bg_color);
    if (hbr_bg) {
        FillRect(hdc, &rc, hbr_bg);
        DeleteObject(hbr_bg);
    }

    /* 외곽선 그리기 */
    int border = SC(HG_BORDER_THICKNESS);
    HWND fg = GetForegroundWindow();
    BOOL is_focused = (fg == hwnd || IsChild(hwnd, fg));
    COLORREF border_color = is_focused ? HG_COLOR_BORDER_SELECTED : HG_COLOR_BG_TOOLBAR;
    HBRUSH hbr_border = CreateSolidBrush(border_color);
    if (hbr_border) {
        /* 상하좌우 사각형으로 채우기 */
        RECT rc_top = {0, 0, rc.right, border};
        RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
        RECT rc_left = {0, border, border, rc.bottom - border};
        RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};

        FillRect(hdc, &rc_top, hbr_border);
        FillRect(hdc, &rc_bottom, hbr_border);
        FillRect(hdc, &rc_left, hbr_border);
        FillRect(hdc, &rc_right, hbr_border);

        DeleteObject(hbr_border);
    }
    EndPaint(hwnd, &ps);
    return 0;
}
static LRESULT taskbox_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    int dx = 0, dy = 0;
    int move_step = SC(20);
    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);

    /* Alt + 방향키/wasd: 현재 창 이동 (태스크 박스) */
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
            /* 이동 후에도 최소한 일부는 화면에 보이도록 보호 */
            ensure_window_visible(hwnd, L"taskbox");
            hg_g_hover_check_armed = FALSE;
            GetCursorPos(&hg_g_last_mouse_pos);
            return 0;
        }

        /* Alt + +/-: 투명도 조절 */
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_taskbox_alpha(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_taskbox_alpha(-1);
            return 0;
        }
    }

    /* Ctrl + +/-: 텍스트 글꼴 크기 조절, Ctrl + 방향키/wasd: 창 크기/그리드 조절 */
    if (is_ctrl) {
        int icon_size = taskbox_toolbar_icon_size();

        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_edit_font_size(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_edit_font_size(-1);
            return 0;
        } else if (w_param == VK_LEFT || w_param == 'A') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);

            if (tb_width > exact_tb_width + SC(5)) {
                /* Right padding exists, snap to current cols */
            } else if (cols > 1) {
                cols--;
                exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
            }

            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        } else if (w_param == VK_RIGHT || w_param == 'D') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            cols++;
            int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        } else if (w_param == VK_UP || w_param == 'W') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
            int total_items = total_tasks + total_shortcuts;
            if (total_items <= 0)
                total_items = 1;

            int current_rows = (total_items + cols - 1) / cols;
            if (current_rows > 1) {
                while (cols < total_items) {
                    cols++;
                    int new_rows = (total_items + cols - 1) / cols;
                    if (new_rows < current_rows)
                        break;
                }
                int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
                int new_w = exact_tb_width + border * 2;
                SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        } else if (w_param == VK_DOWN || w_param == 'S') {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            int tb_width = (rc.right - rc.left) - border * 2;
            int cols = get_items_per_row(tb_width, icon_size);
            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
            int total_items = total_tasks + total_shortcuts;
            if (total_items <= 0)
                total_items = 1;

            int current_rows = (total_items + cols - 1) / cols;
            while (cols > 1) {
                cols--;
                int new_rows = (total_items + cols - 1) / cols;
                if (new_rows > current_rows)
                    break;
            }

            int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
            int new_w = exact_tb_width + border * 2;
            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
    }

    if (w_param == 'C' && !is_ctrl && !is_alt) {
        show_commandbox_window();
        return 0;
    }

    /* Esc: 창 닫기 */
    if (w_param == VK_ESCAPE) {
        hide_taskbox(hwnd);
        return 0;
    }
    if (msg == WM_SYSKEYDOWN)
        return DefWindowProcW(hwnd, msg, w_param, l_param);

    /* 탐색 및 선택 */
    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

    if (total_tasks > 0 || total_shortcuts > 0) {
        int icon_size = taskbox_toolbar_icon_size();

        RECT rc_toolbar;
        GetClientRect(hg_g_toolbar_wnd, &rc_toolbar);

        int cols = get_items_per_row(rc_toolbar.right, icon_size);
        int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
        if (min_required_rows <= 0)
            min_required_rows = 1;

        int visible_rows = (rc_toolbar.bottom - SC(20) + SC(10)) / (icon_size + SC(10));
        if (visible_rows <= 0)
            visible_rows = 1;

        int rows = (visible_rows > min_required_rows) ? visible_rows : min_required_rows;
        int total_cells = rows * cols;

        int current_cell = -1;
        if (hg_taskbox_focus.area == 0) {
            current_cell = hg_taskbox_focus.index;
        } else {
            current_cell = total_cells - 1 - hg_taskbox_focus.index;
        }

        if (current_cell < 0)
            current_cell = 0;

        int r = current_cell / cols;
        int c = current_cell % cols;
        BOOL changed = FALSE;

        if (w_param == VK_LEFT || w_param == 'A') {
            c--;
            changed = TRUE;
        } else if (w_param == VK_RIGHT || w_param == 'D') {
            c++;
            changed = TRUE;
        } else if (w_param == VK_UP || w_param == 'W') {
            r--;
            changed = TRUE;
        } else if (w_param == VK_DOWN || w_param == 'S') {
            r++;
            changed = TRUE;
        } else if (w_param == VK_F2) {
            if (hg_g_floater_wnd)
                PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
        } else if (w_param == VK_SPACE) {
            if (hg_taskbox_focus.area == 0)
                activate_taskbar_item(hg_taskbox_focus.index);
            else
                activate_toolbar_item(hg_taskbox_focus.index);
        } else if (w_param == VK_RETURN) {
            RECT rc_item;
            get_toolbar_item_rect(hg_taskbox_focus.area, hg_taskbox_focus.index, rc_toolbar.right, rc_toolbar.bottom,
                                  icon_size, &rc_item);
            PostMessageW(hg_g_toolbar_wnd, WM_RBUTTONUP, 0, 0); // Send 0 for l_param to indicate keyboard trigger
        }

        if (changed) {
            if (c < 0) {
                c = cols - 1;
                r--;
            }
            if (c >= cols) {
                c = 0;
                r++;
            }
            if (r < 0)
                r = 0;
            if (r >= rows)
                r = rows - 1;

            int new_cell = r * cols + c;

            // 빈 공간이라면 가장 가까운 유효한 셀로 이동 (이 경우는 Task의 마지막이나 Shortcut의 첫번째가 될 것)
            if (new_cell >= total_tasks && new_cell < total_cells - total_shortcuts) {
                if (new_cell > current_cell) {
                    new_cell = total_cells - total_shortcuts;
                } else {
                    new_cell = total_tasks - 1;
                }
            }
            if (new_cell < 0)
                new_cell = 0;
            if (new_cell >= total_cells)
                new_cell = total_cells - 1;

            // 새로운 cell의 정보를 다시 item_type/index로 매핑
            if (new_cell < total_tasks) {
                hg_taskbox_focus.area = 0;
                hg_taskbox_focus.index = new_cell;
            } else if (new_cell >= total_cells - total_shortcuts) {
                hg_taskbox_focus.area = 1;
                hg_taskbox_focus.index = total_cells - 1 - new_cell;
            }
            update_focus_message(-2, -2);
        }

        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
    return 0;
}

static BOOL taskbox_forward_floater_command(UINT cmd, WPARAM w_param, LPARAM l_param)
{
    if (cmd != HG_IDM_ABOUT && cmd != HG_IDM_RESET_ALL)
        return FALSE;

    if (hg_g_floater_wnd)
        SendMessage(hg_g_floater_wnd, WM_COMMAND, w_param, l_param);
    return TRUE;
}

static BOOL taskbox_dispatch_font_command(UINT cmd)
{
    if (cmd == HG_IDM_FONT_UP) {
        update_edit_font_size(1);
        return TRUE;
    }
    if (cmd == HG_IDM_FONT_DOWN) {
        update_edit_font_size(-1);
        return TRUE;
    }
    return FALSE;
}

static LRESULT taskbox_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    UINT cmd = LOWORD(w_param);
    BOOL handled = taskbox_forward_floater_command(cmd, w_param, l_param) || taskbox_dispatch_font_command(cmd);

    if (taskbox_handle_audio_menu_command(cmd)) {
        return 0;
    }

    if (!handled && cmd == HG_IDM_CLOSE_APP) {
        if (hg_g_floater_wnd) {
            DestroyWindow(hg_g_floater_wnd);
        }
    }
    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}
static LRESULT taskbox_controller_on_destroy(HWND hwnd)
{
    hg_g_taskbox_highlight_ticks = 0;
    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
    KillTimer(hwnd, HG_TIMER_TASKBOX_REFRESH);
    KillTimer(hwnd, HG_TIMER_HOVER_CHECK);

    /* GDI 전역 리소스(font, brush) 및 전역 툴팁은 wWinMain의 cleanup_finish에서 안전하게 일괄 해제하므로 여기서
     * 삭제하지 않음 */

    for (int i = 0; i < hg_g_shortcut_count; i++) {
        release_shortcut_item_icon(&hg_g_shortcuts[i]);
    }
    for (int i = 0; i < hg_g_window_count; i++) {
        release_window_item_icon(&hg_g_window_items[i]);
    }
    /* 전역 종료는 floater_proc에서 처리하므로 주석 처리 */
    /* PostQuitMessage(0); */
    return 0;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static HgTaskboxLayoutState layout_state = {{0, 0, 0, 0}};

    switch (msg) {
    /* WM_DISPLAYCHANGE is handled once, by the floater, for the whole process. */
    case WM_ACTIVATE: {
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_SETTINGCHANGE:
        if (!should_refresh_theme_on_setting_change(l_param)) {
            break;
        }
        /* fallthrough */
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
    case WM_DWMCOLORIZATIONCOLORCHANGED: {
        refresh_theme_surfaces(hwnd);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO *)l_param;
        mmi->ptMinTrackSize.x = SC(HG_MIN_WINDOW_WIDTH);
        mmi->ptMinTrackSize.y = SC(HG_MIN_WINDOW_HEIGHT);
        return 0;
    }
    case WM_CREATE:
        return taskbox_controller_on_create(hwnd);
    case WM_MOUSEWHEEL: {
        if (GetKeyState(VK_MENU) < 0) {
            /* Alt 키가 눌린 상태에서만 투명도 조절 실행 */
            short delta = (short)HIWORD(w_param);
            update_taskbox_alpha(delta > 0 ? 1 : -1);
        } else {
            /* Alt 키가 없으면 태스크바 창으로 메시지를 전달하여 창 전체에서 스크롤 가능하게 함 */
            /* Ctrl+휠(크기조절) 메시지도 태스크바 창에서 처리되도록 전달됨 */
            SendMessageW(hg_g_toolbar_wnd, WM_MOUSEWHEEL, w_param, l_param);
        }
        return 0;
    }
    case WM_NOTIFY: {
        /* ListView 관련 통지 제거됨 */
        break;
    }
    case WM_ENTERSIZEMOVE: {
        hg_g_in_sizemove = TRUE;
        GetWindowRect(hwnd, &layout_state.sizemove_start_rect);
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
    case WM_EXITSIZEMOVE: {
        hg_g_in_sizemove = FALSE;
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);

        int dw = (rc.right - rc.left) -
                 (layout_state.sizemove_start_rect.right - layout_state.sizemove_start_rect.left);
        int dh = (rc.bottom - rc.top) -
                 (layout_state.sizemove_start_rect.bottom - layout_state.sizemove_start_rect.top);

        int icon_size = taskbox_toolbar_icon_size();
        int border = SC(HG_BORDER_THICKNESS);

        int total_tasks = hg_g_window_count;
        int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        int total_items = total_tasks + total_shortcuts;
        if (total_items <= 0)
            total_items = 1;

        int cols;
        if (ABS(dh) > ABS(dw)) {
            int edit_height = SC(20);
            if (hg_g_edit_msg_wnd && hg_g_main_font) {
                HDC hdc = GetDC(hg_g_edit_msg_wnd);
                if (hdc) {
                    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                    TEXTMETRIC tm = {0};
                    if (GetTextMetrics(hdc, &tm)) {
                        edit_height = (tm.tmHeight + tm.tmExternalLeading) + SC(6);
                    }
                    if (old_font)
                        SelectObject(hdc, old_font);
                    ReleaseDC(hg_g_edit_msg_wnd, hdc);
                }
            }

            int row_height = icon_size + SC(10);
            int current_h = rc.bottom - rc.top;
            int available_toolbar_h = current_h - (border * 2 + edit_height);
            int target_rows = (available_toolbar_h - SC(10) + row_height / 2) / row_height;
            if (target_rows < 1)
                target_rows = 1;
            if (target_rows > total_items)
                target_rows = total_items;
            cols = (total_items + target_rows - 1) / target_rows;
        } else {
            int tb_width = (rc.right - rc.left) - border * 2;
            cols = get_items_per_row(tb_width, icon_size);
        }
        if (cols <= 0)
            cols = 1;

        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
        int new_w = exact_tb_width + border * 2;

        SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        update_layout(hwnd); /* Will trigger snap exact height */
        GetWindowRect(hwnd, &rc);
        save_taskbox_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        if (hg_g_floater_wnd) {
            SetWindowPos(hg_g_floater_wnd, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        }
        return 0;
    }
    case WM_TIMER:
        if (w_param == HG_TIMER_TASKBOX_REFRESH) {
            if (IsWindowVisible(hwnd) && !hg_g_menu_active) {
                refresh_window_list(FALSE);
            }
        } else if (w_param == HG_TIMER_HIGHLIGHT) {
            hg_g_taskbox_highlight_ticks--;
            if (hg_g_taskbox_highlight_ticks <= 0) {
                KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (w_param == HG_TIMER_HOVER_CHECK) {
            if (IsWindowVisible(hwnd) && !hg_g_menu_active && GetCapture() == NULL && !hg_g_in_sizemove) {
                POINT pt;
                GetCursorPos(&pt);
                RECT rc;
                GetWindowRect(hwnd, &rc);
                if (!hg_g_hover_check_armed) {
                    if (PtInRect(&rc, pt)) {
                        hg_g_hover_check_armed = TRUE;
                    } else if (hg_g_last_mouse_pos.x != -1 && (pt.x != hg_g_last_mouse_pos.x || pt.y != hg_g_last_mouse_pos.y)) {
                        hg_g_hover_check_armed = TRUE;
                    }
                }
                
                /* Collapse to the floater only after the cursor has stayed outside
                 * for 0.5s (re-checked every 100ms tick); coming back inside resets
                 * the count so a brief exit doesn't collapse the taskbox. */
                if (hg_g_hover_check_armed) {
                    if (!PtInRect(&rc, pt)) {
                        s_hover_outside_ticks++;
                        if (s_hover_outside_ticks >= 5) {
                            hide_taskbox(hwnd);
                        }
                    } else {
                        s_hover_outside_ticks = 0;
                    }
                } else {
                    s_hover_outside_ticks = 0;
                }
            }
        }
        return 0;
    case WM_PAINT:
        return taskbox_controller_on_paint(hwnd);
    case WM_NCHITTEST: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(hwnd, &pt);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);

        if (pt.x < border && pt.y < border)
            return HTTOPLEFT;
        if (pt.x >= rc.right - border && pt.y < border)
            return HTTOPRIGHT;
        if (pt.x < border && pt.y >= rc.bottom - border)
            return HTBOTTOMLEFT;
        if (pt.x >= rc.right - border && pt.y >= rc.bottom - border)
            return HTBOTTOMRIGHT;
        if (pt.y < border)
            return HTTOP;
        if (pt.y >= rc.bottom - border)
            return HTBOTTOM;
        if (pt.x < border)
            return HTLEFT;
        if (pt.x >= rc.right - border)
            return HTRIGHT;

        return HTCAPTION; /* 그 외 모든 클라이언트 영역은 드래그 가능하게 */
    }
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        return taskbox_controller_on_keydown(hwnd, msg, w_param, l_param);
    case WM_COMMAND:
        return taskbox_controller_on_command(hwnd, w_param, l_param);
    case WM_SIZE: {
        update_layout(hwnd);
        InvalidateRect(hwnd, NULL, TRUE); /* 외곽선 다시 그리기 */
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        /* fallthrough */
    case WM_CTLCOLORLISTBOX: {
        HDC hdc_static = (HDC)w_param;
        SetTextColor(hdc_static, hg_g_color_scheme_selected.text);
        /* 투명 배경 모드 시 글자가 겹쳐 그려지는 문제 방지를 위해 배경을 칠함 */
        SetBkMode(hdc_static, OPAQUE);

        COLORREF bg_color = hg_g_color_scheme_selected.bg;
        if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
            bg_color = HG_COLOR_BG_FLASH;
        }
        SetBkColor(hdc_static, bg_color);

        /* 하이라이트 중일 때는 브러시를 캐싱하여 반환 (GDI 누수 방지) */
        if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
            if (!hg_g_hbr_highlight) {
                hg_g_hbr_highlight = CreateSolidBrush(HG_COLOR_BG_FLASH);
            }
            return hg_g_hbr_highlight ? (LRESULT)hg_g_hbr_highlight : (LRESULT)GetStockObject(BLACK_BRUSH);
        }

        if (!hg_g_edit_bg_brush)
            hg_g_edit_bg_brush = CreateSolidBrush(hg_g_color_scheme_selected.bg);
        return hg_g_edit_bg_brush ? (LRESULT)hg_g_edit_bg_brush : (LRESULT)GetStockObject(BLACK_BRUSH);
    }
    case WM_DESTROY:
        return taskbox_controller_on_destroy(hwnd);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
