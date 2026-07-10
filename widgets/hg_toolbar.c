/* Taskbox toolbar controller: painting, hit testing, mouse and wheel handling. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

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

/* Focus outline: a few FrameRect rings drawn inward so the focused item stands
 * out clearly without changing the item layout. */
static void toolbar_draw_focus_frame(HDC dc, const RECT *rc)
{
    HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BORDER_SELECTED);
    if (!hbr || !rc)
        return;

    RECT ring = *rc;
    int thickness = SC(2);
    if (thickness < 2)
        thickness = 2;
    for (int i = 0; i < thickness; ++i) {
        FrameRect(dc, &ring, hbr);
        InflateRect(&ring, -1, -1);
        if (ring.left >= ring.right || ring.top >= ring.bottom)
            break;
    }
}

static void toolbar_draw_muted_border(HDC hdc, const RECT *rc)
{
    if (!hdc || !rc)
        return;

    HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BORDER_SELECTED);
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
                HBRUSH hbr_bg = hg_cached_solid_brush(bg_color);
                if (hbr_bg) {
                    FillRect(mem_dc, &rc, hbr_bg);
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
                        HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FrameRect(mem_dc, &rc_btn, hbr);
                        }
                        continue;
                    }

                    if (pressed_type == 0 && pressed_index == r_idx && !drag_state->is_dragging) {
                        HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FillRect(mem_dc, &rc_btn, hbr);
                        }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 0 && hovered_index == pos && !drag_state->is_dragging) ||
                               (hg_taskbox_focus.area == 0 && hg_taskbox_focus.index == r_idx)) {
                        HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                        if (hbr) {
                            FillRect(mem_dc, &rc_btn, hbr);
                        }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_taskbox_focus.area == 0 && hg_taskbox_focus.index == r_idx) {
                            toolbar_draw_focus_frame(mem_dc, &rc_btn);
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
                        HBRUSH hbr_dot = hg_cached_solid_brush(HG_COLOR_BORDER_SELECTED);
                        if (hbr_dot) {
                            FillRect(mem_dc, &dot_rc, hbr_dot);
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
                    HBRUSH hbr_opp = hg_cached_solid_brush(button_bg);
                    if (hbr_opp) {
                        FillRect(mem_dc, &rc_btn, hbr_opp);
                    }

                    if (pressed_type == 1 && pressed_index == i) {
                        if (!keep_value_bg) {
                            HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                            if (hbr) {
                                FillRect(mem_dc, &rc_btn, hbr);
                            }
                        }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 1 && hovered_index == i) ||
                               (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == i)) {
                        if (!keep_value_bg) {
                            HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                            if (hbr) {
                                FillRect(mem_dc, &rc_btn, hbr);
                            }
                        }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == i) {
                            toolbar_draw_focus_frame(mem_dc, &rc_btn);
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
                    HBRUSH hbr = hg_cached_solid_brush(HG_COLOR_BG_SELECTED);
                    if (hbr) {
                        FillRect(mem_dc, &drag_rc, hbr);
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
            cols = taskbox_cols_from_height(new_height, icon_size, border, total_items);
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

        int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
        int req_w = exact_tb_width + border * 2;

        int rows = (total_items + cols - 1) / cols;
        if (rows <= 0)
            rows = 1;
        int row_height = icon_size + SC(10);
        int req_toolbar_height = SC(10) + rows * row_height;

        int edit_height = hg_measure_edit_height(hg_g_edit_msg_wnd, hg_g_main_font, hg_g_scale_factor);
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
        int exact_tb_width = hg_snap_width_for_cols(cols, icon_size);
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

