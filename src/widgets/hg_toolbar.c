/* Taskbox toolbar controller: painting, hit testing, mouse and wheel handling. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_glyph.h"
#include "../hg_recbak.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"
#include "hg_tabbox.h"
#include "hg_avbox.h"
#include "hg_hilite.h"

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

/* Per-channel blend between the configurable low/high gradient endpoints. */
static COLORREF toolbar_value_blend(COLORREF lo, COLORREF hi, int pct)
{
    return RGB(toolbar_scale_color_channel(GetRValue(lo), GetRValue(hi), pct),
               toolbar_scale_color_channel(GetGValue(lo), GetGValue(hi), pct),
               toolbar_scale_color_channel(GetBValue(lo), GetBValue(hi), pct));
}

static COLORREF toolbar_basic_icon_bg_color(int index, COLORREF base_color)
{
    if (index == HG_TOOL_ICON_ALPHA) {
        return toolbar_value_blend(hg_g_color_value_alpha_lo, hg_g_color_value_alpha_hi,
                                   toolbar_taskbox_alpha_percent());
    }
    if (index == HG_TOOL_ICON_MONITOR) {
        return toolbar_value_blend(hg_g_color_value_bright_lo, hg_g_color_value_bright_hi, get_system_brightness());
    }
    if (index == HG_TOOL_ICON_VOLUME) {
        /* Muted reads as the bottom of the ramp, because that is what the
         * machine is putting out. The level itself is untouched and comes back
         * on unmute; showing it while nothing can be heard would make the plate
         * say "loud" about a silent machine - and would leave the click, whose
         * whole job is mute, with nothing to show for itself but a thin
         * border. */
        int pct = get_system_mute() ? 0 : get_system_volume();
        return toolbar_value_blend(hg_g_color_value_vol_lo, hg_g_color_value_vol_hi, pct);
    }
    return toolbar_invert_color(base_color);
}

static BOOL toolbar_tile_is_custom_value(int index)
{
    if (index == HG_TOOL_ICON_ALPHA)
        return (hg_g_color_value_alpha_lo != HG_COLOR_VALUE_ALPHA_LO_DEFAULT ||
                hg_g_color_value_alpha_hi != HG_COLOR_VALUE_ALPHA_HI_DEFAULT);
    if (index == HG_TOOL_ICON_MONITOR)
        return (hg_g_color_value_bright_lo != HG_COLOR_VALUE_BRIGHT_LO_DEFAULT ||
                hg_g_color_value_bright_hi != HG_COLOR_VALUE_BRIGHT_HI_DEFAULT);
    if (index == HG_TOOL_ICON_VOLUME)
        return (hg_g_color_value_vol_lo != HG_COLOR_VALUE_VOL_LO_DEFAULT ||
                hg_g_color_value_vol_hi != HG_COLOR_VALUE_VOL_HI_DEFAULT);
    return FALSE;
}

static int toolbar_value_percent_clamped(int index)
{
    if (index == HG_TOOL_ICON_ALPHA)
        return toolbar_taskbox_alpha_percent();
    if (index == HG_TOOL_ICON_MONITOR)
        return toolbar_clamp_percent(get_system_brightness());
    if (index == HG_TOOL_ICON_VOLUME)
        return toolbar_clamp_percent(get_system_mute() ? 0 : get_system_volume());
    return 0;
}

static COLORREF toolbar_focus_color(void)
{
    return hg_ui_focus_color();
}

/* A separator cell: one crisp vertical rule down its middle in the palette
 * hairline colour. */
static void toolbar_draw_separator(HDC hdc, const RECT *rc)
{
    HBRUSH brush = hg_cached_solid_brush(hg_ui_widget_palette()->line);
    if (!brush)
        return;
    int width = SC(1);
    if (width < 1)
        width = 1;
    int h = rc->bottom - rc->top;
    int len = h * 3 / 5;
    int x = rc->left + (rc->right - rc->left - width) / 2;
    int y = rc->top + (h - len) / 2;
    RECT line = {x, y, x + width, y + len};
    FillRect(hdc, &line, brush);
}

/* Key badge at the top-left corner of an icon: painted on demand (hover,
 * keyboard focus, or modifier peek) so resting tiles stay calm (D-U1). */
static int toolbar_badge_height(int icon_size)
{
    int badge_h = icon_size / 2;
    return (badge_h < SC(10)) ? SC(10) : badge_h;
}

static void toolbar_draw_badge(HDC dc, const RECT *rc_item, const WCHAR *text, int icon_size, BOOL flashing)
{
    if (!dc || !rc_item || !text || !*text || !hg_g_toolbar_badge_font)
        return;

    const HgUiPalette *pal = hg_ui_widget_palette();
    int badge_h = toolbar_badge_height(icon_size);

    HFONT old_badge = (HFONT)SelectObject(dc, hg_g_toolbar_badge_font);

    SIZE sz = {0, 0};
    GetTextExtentPoint32W(dc, text, lstrlenW(text), &sz);
    int badge_w = sz.cx + SC(4);
    if (badge_w < badge_h)
        badge_w = badge_h;

    RECT badge_rc = {rc_item->left - SC(2), rc_item->top - SC(2), rc_item->left - SC(2) + badge_w,
                     rc_item->top - SC(2) + badge_h};

    HBRUSH hbr_badge = hg_cached_solid_brush(flashing ? pal->honey : pal->bg2);
    if (hbr_badge)
        FillRect(dc, &badge_rc, hbr_badge);
    HBRUSH hbr_edge = hg_cached_solid_brush(flashing ? pal->honey_fg : pal->line);
    if (hbr_edge)
        FrameRect(dc, &badge_rc, hbr_edge);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, flashing ? pal->honey_fg : pal->fg);
    DrawTextW(dc, text, -1, &badge_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_badge);
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
    /* The buttons, and after them the shortcuts. One that has no cell comes
     * back as an empty rect and is skipped. */
    int total_buttons = hg_toolbar_grid_buttons() + hg_toolbar_grid_shortcuts();
    for (int i = 0; i < total_buttons; i++) {
        RECT rc_item, rc_btn;
        get_toolbar_item_rect(1, i, width, height, icon_size, &rc_item);
        if (IsRectEmpty(&rc_item))
            continue;
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

/* A font for a button's word, sized so the word fits the button.
 *
 * The buttons used to carry one capital, which fits anything; three letters do
 * not, and a label clipped to "Vo" is worse than a smaller one that reads. The
 * text is measured at the button's own height and scaled down until it fits the
 * width, which keeps the fit right across DPI, icon sizes and font names
 * without any of them being a special case.
 *
 * One font is cached, keyed by the height it was made at: every button on the
 * row is the same size, so in practice this is created when the icons change
 * and never per paint. */
/* A word too long for one line, split into two.
 *
 * Up to four letters still fit a button across at a size that reads - "Note"
 * and "Clip" are words, and cutting a four-letter word in half makes two
 * syllables of nothing. Five and six are stacked instead: two short lines
 * rather than one line shrunk past legibility. The break is as near the middle
 * as the word allows, which keeps the two lines looking like one word rather
 * than a word and a remainder. */
static int toolbar_label_lines(const WCHAR *label, WCHAR *out, size_t out_cch)
{
    int len = label ? lstrlenW(label) : 0;
    if (len <= 4 || len > 6) {
        StringCchCopyW(out, out_cch, label ? label : L"");
        return 1;
    }

    int split = (len + 1) / 2; /* the longer half goes on top */
    if (FAILED(StringCchCopyNW(out, out_cch, label, (size_t)split)))
        return 1;
    StringCchCatW(out, out_cch, L"\n");
    StringCchCatW(out, out_cch, label + split);
    return 2;
}

static HFONT toolbar_label_font(HDC dc, const WCHAR *label, int box_w, int box_h, int lines)
{
    static HFONT s_font = NULL;
    static int s_height = 0;
    /* The family the cached font was built in: a different interface font has
     * to rebuild it even when the height is the same. */
    static WCHAR s_face[LF_FACESIZE] = L"";
    if (s_font && lstrcmpW(s_face, hg_g_font_name) != 0)
        release_font_handle(&s_font, FALSE);

    if (!label || !*label || box_w <= 0 || box_h <= 0)
        return NULL;

    if (lines < 1)
        lines = 1;

    int len = lstrlenW(label);
    /* Two lines share the button's height, and each line is only half the
     * glyphs wide, so both bounds change together. */
    int height = box_h / lines;
    int per_line = (lines > 1) ? (len + 1) / 2 : len;
    if (per_line > 1) {
        /* A first guess from the glyph count, so the measuring loop below
         * usually confirms rather than iterates. */
        int guess = (box_w * 2) / (per_line + 1);
        if (guess < height)
            height = guess;
    }
    if (height < SC(7))
        height = SC(7);

    for (int attempt = 0; attempt < 4; ++attempt) {
        if (height != s_height || !s_font) {
            release_font_handle(&s_font, FALSE);
            s_font = CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                 DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
            s_height = height;
            StringCchCopyW(s_face, HG_ARRAYSIZE(s_face), hg_g_font_name);
        }
        if (!s_font)
            return NULL;

        HFONT old = (HFONT)SelectObject(dc, s_font);
        SIZE sz = {0, 0};
        GetTextExtentPoint32W(dc, label, (lines > 1) ? per_line : len, &sz);
        SelectObject(dc, old);

        /* A little air on each side: a word touching the frame reads as
         * clipped even when every glyph is there. */
        int room = (box_w * 88) / 100;
        if (sz.cx <= room || height <= SC(7))
            return s_font;

        int next = (height * room) / (sz.cx > 0 ? sz.cx : 1);
        if (next >= height)
            next = height - 1;
        height = (next > SC(7)) ? next : SC(7);
    }
    return s_font;
}

/* A function button's word, fitted to the button and centred on its plate. */
static void toolbar_draw_builtin_label(HDC dc, int index, const RECT *rc_item, const RECT *rc_btn, COLORREF fg,
                                       COLORREF bg)
{
    (void)rc_item;
    const WCHAR *btn_text = hg_toolbar_builtin_label(index);
    WCHAR stacked[16];
    int lines = toolbar_label_lines(btn_text, stacked, HG_ARRAYSIZE(stacked));
    RECT text_rc = *rc_btn;
    text_rc.left = rc_btn->left + SC(2);
    text_rc.right = rc_btn->right - SC(2);
    text_rc.top = rc_btn->top + SC(2);
    text_rc.bottom = rc_btn->bottom - SC(2);
    HFONT label_font =
        toolbar_label_font(dc, btn_text, rc_btn->right - rc_btn->left, text_rc.bottom - text_rc.top, lines);
    if (!label_font)
        label_font = hg_g_toolbar_btn_font;
    if (!label_font)
        return;

    SetTextColor(dc, fg);
    SetBkMode(dc, TRANSPARENT);
    HFONT old_font = (HFONT)SelectObject(dc, label_font);
    UINT format = (lines > 1) ? (DT_CENTER | DT_VCENTER | DT_EDITCONTROL | DT_WORDBREAK)
                              : (DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    draw_outlined_text(dc, stacked, lstrlenW(stacked), &text_rc, format, fg, bg);
    SelectObject(dc, old_font);
}

/* Crisp 1px keyboard focus ring (Win8 flat). Only painted when
 * hg_g_keyboard_mode is active. */
static void toolbar_draw_focus_ring(HDC dc, const RECT *rc_btn)
{
    HBRUSH ring = hg_cached_solid_brush(toolbar_focus_color());
    if (!ring)
        return;
    FrameRect(dc, rc_btn, ring);
}

/* A shortcut on the Win8 flat grid: resting bg2 plate, bg3 hover plate,
 * accent2 1px focus ring when hg_g_keyboard_mode, and on-demand badge. */
void hg_toolbar_paint_shortcut_cell(HDC dc, int s_idx, const RECT *rc_item, int icon_size, BOOL selected,
                                    BOOL hovered)
{
    if (!dc || !rc_item || s_idx < 0 || s_idx >= hg_g_shortcut_count)
        return;

    const HgUiPalette *pal = hg_ui_widget_palette();
    BOOL flashing = (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0));
    RECT rc_btn = *rc_item;
    InflateRect(&rc_btn, SC(4), SC(4));

    COLORREF plate = flashing ? pal->honey : (hovered ? pal->bg3 : pal->bg2);
    HBRUSH hbr = hg_cached_solid_brush(plate);
    if (hbr)
        FillRect(dc, &rc_btn, hbr);

    if (selected && hg_g_keyboard_mode)
        toolbar_draw_focus_ring(dc, &rc_btn);

    if (hg_g_shortcuts[s_idx].icon)
        DrawIconEx(dc, rc_item->left, rc_item->top, hg_g_shortcuts[s_idx].icon, icon_size, icon_size, 0, NULL,
                   DI_NORMAL);

    if (hovered || (selected && hg_g_keyboard_mode) || hg_g_badge_mod_peek) {
        WCHAR badge[8];
        if (hg_shortcut_badge_text(s_idx, badge, HG_ARRAYSIZE(badge)))
            toolbar_draw_badge(dc, rc_item, badge, icon_size, flashing);
    }
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

                const HgUiPalette *pal = hg_ui_widget_palette();
                BOOL flashing = (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0));
                COLORREF bg_color = flashing ? pal->honey : pal->bg;
                HBRUSH hbr_bg = hg_cached_solid_brush(bg_color);
                if (hbr_bg) {
                    FillRect(mem_dc, &rc, hbr_bg);
                }

                int icon_size = taskbox_toolbar_icon_size();

                if (icon_size != *cached_icon_size || !hg_g_toolbar_btn_font) {
                    release_font_handle(&hg_g_toolbar_btn_font, FALSE);
                    hg_g_toolbar_btn_font = CreateFontW(icon_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
                    /* The label sits in a box half the icon, so it needs its own
                     * size; built here so it is rebuilt exactly when the icons
                     * are and never once per paint. */
                    release_font_handle(&hg_g_toolbar_badge_font, FALSE);
                    int badge_px = icon_size / 2;
                    if (badge_px < SC(10))
                        badge_px = SC(10);
                    hg_g_toolbar_badge_font =
                        CreateFontW(badge_px - SC(2), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                    DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
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

                // Draw tasks (type 0): transparent on pal->bg, hover bg3, active bottom bar, on-demand badge.
                for (int pos = 0; pos < hg_g_window_count; pos++) {
                    int r_idx = visual_order[pos];
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(0, pos, rc.right, rc.bottom, icon_size, &rc_item);
                    rc_btn = rc_item;
                    InflateRect(&rc_btn, SC(4), SC(4));

                    if (drag_state->is_dragging && drag_state->source_index != -1 &&
                        r_idx == drag_state->source_index) {
                        HBRUSH hbr = hg_cached_solid_brush(pal->line_strong);
                        if (hbr) {
                            FrameRect(mem_dc, &rc_btn, hbr);
                        }
                        continue;
                    }

                    BOOL task_pointed = ((pressed_type == 0 && pressed_index == r_idx) ||
                                         (hovered_type == 0 && hovered_index == pos)) &&
                                        !drag_state->is_dragging;
                    BOOL task_focused = (hg_taskbox_focus.area == 0 && hg_taskbox_focus.index == r_idx);
                    if (task_pointed) {
                        HBRUSH hbr = hg_cached_solid_brush(flashing ? pal->honey : pal->bg3);
                        if (hbr) {
                            FillRect(mem_dc, &rc_btn, hbr);
                        }
                    }
                    if (task_focused && hg_g_keyboard_mode)
                        toolbar_draw_focus_ring(mem_dc, &rc_btn);

                    if (hg_g_window_items[r_idx].icon) {
                        DrawIconEx(mem_dc, rc_item.left, rc_item.top, hg_g_window_items[r_idx].icon, icon_size,
                                   icon_size, 0, NULL, DI_NORMAL);
                    } else if (hg_g_toolbar_btn_font) {
                        const WCHAR *title = hg_g_window_items[r_idx].title;
                        WCHAR initial[2];
                        initial[0] = (title && title[0]) ? title[0] : L'?';
                        initial[1] = L'\0';

                        HFONT old_font = (HFONT)SelectObject(mem_dc, hg_g_toolbar_btn_font);
                        SetBkMode(mem_dc, TRANSPARENT);
                        draw_outlined_text(mem_dc, initial, 1, &rc_item, DT_CENTER | DT_VCENTER | DT_SINGLELINE,
                                           flashing ? pal->honey_fg : pal->fg, bg_color);
                        SelectObject(mem_dc, old_font);
                    }

                    if (task_pointed || (task_focused && hg_g_keyboard_mode) || hg_g_badge_mod_peek) {
                        WCHAR badge[8];
                        if (hg_task_badge_text(r_idx, badge, HG_ARRAYSIZE(badge)))
                            toolbar_draw_badge(mem_dc, &rc_item, badge, icon_size, flashing);
                    }

                    /* Win8 flat bottom indicator bar on the active task. */
                    if (hg_g_window_items[r_idx].hwnd == hg_g_prev_active_hwnd) {
                        int bar_w = SC(16);
                        int max_w = (rc_btn.right - rc_btn.left) - SC(4);
                        if (bar_w > max_w)
                            bar_w = max_w;
                        if (bar_w < SC(6))
                            bar_w = SC(6);
                        int bar_h = SC(3);
                        if (bar_h < 2)
                            bar_h = 2;
                        int bar_x = rc_btn.left + ((rc_btn.right - rc_btn.left) - bar_w) / 2;
                        int bar_y = rc_btn.bottom - bar_h;
                        RECT dot_rc = {bar_x, bar_y, bar_x + bar_w, bar_y + bar_h};
                        HBRUSH hbr_dot = hg_cached_solid_brush(flashing ? pal->honey_fg : pal->accent);
                        if (hbr_dot) {
                            FillRect(mem_dc, &dot_rc, hbr_dot);
                        }
                    }
                }

                /* The shortcuts: Win8 flat bg2 tiles (`hg_toolbar_paint_shortcut_cell`). */
                for (int s_idx = 0; s_idx < hg_toolbar_grid_shortcuts(); s_idx++) {
                    int id = HG_SHORTCUT_BUTTON_ID(s_idx);
                    RECT rc_item;
                    get_toolbar_item_rect(1, id, rc.right, rc.bottom, icon_size, &rc_item);
                    if (IsRectEmpty(&rc_item))
                        continue;
                    BOOL focused = (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == id);
                    BOOL pointed = (hovered_type == 1 && hovered_index == id) ||
                                   (pressed_type == 1 && pressed_index == id);
                    hg_toolbar_paint_shortcut_cell(mem_dc, s_idx, &rc_item, icon_size, focused, pointed);
                }

                /* Function buttons: Win8 flat bg2/bg3 tiles; Vol/Mon/Alp gauge tiles (D-U3=A);
                 * keyboard focus 1px ring only when hg_g_keyboard_mode; on-demand badges (D-U1). */
                int total_buttons = hg_toolbar_grid_buttons();
                for (int i = 0; i < total_buttons; i++) {
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(1, i, rc.right, rc.bottom, icon_size, &rc_item);
                    if (IsRectEmpty(&rc_item))
                        continue;
                    rc_btn = rc_item;
                    InflateRect(&rc_btn, SC(4), SC(4));

                    BOOL pointed = (pressed_type == 1 && pressed_index == i) ||
                                   (hovered_type == 1 && hovered_index == i);
                    BOOL focused = (hg_taskbox_focus.area == 1 && hg_taskbox_focus.index == i);
                    BOOL has_val = hg_toolbar_builtin_has_value(i);
                    BOOL muted_vol = (i == HG_TOOL_ICON_VOLUME && get_system_mute());

                    if (has_val && !flashing && toolbar_tile_is_custom_value(i)) {
                        COLORREF custom_bg = toolbar_basic_icon_bg_color(i, bg_color);
                        HBRUSH hbr_custom = hg_cached_solid_brush(custom_bg);
                        if (hbr_custom)
                            FillRect(mem_dc, &rc_btn, hbr_custom);
                    } else {
                        COLORREF tile_bg = flashing ? pal->honey : (pointed ? pal->bg3 : pal->bg2);
                        HBRUSH hbr_tile = hg_cached_solid_brush(tile_bg);
                        if (hbr_tile)
                            FillRect(mem_dc, &rc_btn, hbr_tile);

                        if (has_val && !flashing) {
                            int pct = toolbar_value_percent_clamped(i);
                            int tile_h = rc_btn.bottom - rc_btn.top;
                            if (!muted_vol && pct > 0 && tile_h > 0) {
                                int gauge_h = (tile_h * pct) / 100;
                                if (gauge_h > 0) {
                                    RECT rc_fill = {rc_btn.left, rc_btn.bottom - gauge_h, rc_btn.right, rc_btn.bottom};
                                    HBRUSH hbr_gauge = hg_cached_solid_brush(pal->select);
                                    if (hbr_gauge)
                                        FillRect(mem_dc, &rc_fill, hbr_gauge);
                                }
                            }
                            int bar_h = SC(3);
                            if (bar_h < 2)
                                bar_h = 2;
                            RECT rc_bar = {rc_btn.left, rc_btn.bottom - bar_h, rc_btn.right, rc_btn.bottom};
                            HBRUSH hbr_bar = hg_cached_solid_brush(muted_vol ? pal->danger : pal->accent);
                            if (hbr_bar)
                                FillRect(mem_dc, &rc_bar, hbr_bar);
                        }
                    }

                    if (focused && hg_g_keyboard_mode)
                        toolbar_draw_focus_ring(mem_dc, &rc_btn);

                    int glyph_id = muted_vol ? HG_GLYPH_VOLUME_MUTED : i;
                    COLORREF glyph_fg = flashing ? pal->honey_fg : (muted_vol ? pal->danger : pal->fg);
                    COLORREF tile_outline_bg = flashing ? pal->honey : (pointed ? pal->bg3 : pal->bg2);
                    if (hg_glyph_has(glyph_id) && !(i == HG_TOOL_ICON_RECBAK && hg_recbak_busy()))
                        hg_glyph_draw(mem_dc, glyph_id, &rc_item, glyph_fg, CLR_NONE);
                    else
                        toolbar_draw_builtin_label(mem_dc, i, &rc_item, &rc_btn, glyph_fg, tile_outline_bg);

                    if (pointed || (focused && hg_g_keyboard_mode) || hg_g_badge_mod_peek) {
                        WCHAR badge[8];
                        if (hg_toolbar_builtin_badge_text(i, badge, HG_ARRAYSIZE(badge)))
                            toolbar_draw_badge(mem_dc, &rc_item, badge, icon_size, flashing);
                    }
                }

                /* The separators between the kinds (separated grid only). */
                {
                    HgGrid grid;
                    hg_toolbar_grid(rc.right, rc.bottom, icon_size, &grid);
                    int separators[2] = {grid.separator_tasks, grid.separator_buttons};
                    for (int n = 0; n < 2; ++n) {
                        RECT rc_sep;
                        get_toolbar_cell_rect(&grid, separators[n], icon_size, &rc_sep);
                        if (IsRectEmpty(&rc_sep))
                            continue;
                        InflateRect(&rc_sep, SC(4), SC(4));
                        toolbar_draw_separator(mem_dc, &rc_sep);
                    }
                }

                if (drag_state->is_dragging && drag_state->source_index != -1 &&
                    drag_state->source_index < hg_g_window_count) {
                    int cx = drag_state->current_pt.x - icon_size / 2;
                    int cy = drag_state->current_pt.y - icon_size / 2;
                    RECT drag_rc = {cx - SC(4), cy - SC(4), cx + icon_size + SC(4), cy + icon_size + SC(4)};
                    HBRUSH hbr = hg_cached_solid_brush(pal->bg3);
                    if (hbr) {
                        FillRect(mem_dc, &drag_rc, hbr);
                    }
                    toolbar_draw_focus_ring(mem_dc, &drag_rc);
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
    /* A message is not a movement: WM_MOUSEMOVE also arrives when a window
     * slides under a pointer that has not stirred, and treating that as the
     * reader reaching for the mouse would undo the keyboard mid-keystroke. */
    hg_keyboard_mode_check_pointer();

    POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
    RECT rc;
    GetClientRect(hwnd, &rc);
    int icon_size = taskbox_toolbar_icon_size();

    if (state->is_resizing && GetCapture() == hwnd) {
        POINT cur_mouse;
        GetCursorPos(&cur_mouse);
        int dw = cur_mouse.x - state->start_mouse.x;
        int dh = cur_mouse.y - state->start_mouse.y;
        int border = hg_box_band();

        int total_items = hg_toolbar_grid_item_count();

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

        int rows = hg_toolbar_rows_for_cols(cols);
        int req_h = border * 2 + hg_toolbar_height_for_rows(rows, icon_size);

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, req_w, req_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    if (state->is_moving_taskbox && GetCapture() == hwnd) {
        POINT cur_mouse;
        GetCursorPos(&cur_mouse);
        int dx = cur_mouse.x - state->start_mouse.x;
        int dy = cur_mouse.y - state->start_mouse.y;
        /* The window only follows the mouse once the press passes the system drag
         * threshold; below it the press is still a click, which relocates instead. */
        if (!state->move_drag_started &&
            (ABS(dx) > GetSystemMetrics(SM_CXDRAG) || ABS(dy) > GetSystemMetrics(SM_CYDRAG))) {
            state->move_drag_started = TRUE;
        }
        if (state->move_drag_started && hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
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
    } else if (!hg_g_keyboard_mode && (cur_type != state->hovered_type || cur_index != state->hovered_index)) {
        state->hovered_type = cur_type;
        state->hovered_index = cur_index;
        if (cur_type != -1 && cur_index != -1) {
            /* Hovering moves the focus too, so the focus ring
             * follows the mouse as well as the keyboard. */
            hg_taskbox_focus.area = cur_type;
            hg_taskbox_focus.index = cur_index;
        }
        update_focus_message(state->hovered_type, state->hovered_index);
        InvalidateRect(hwnd, NULL, FALSE);

        /* A task icon whose window can have tabs opens the tab box, anchored
         * to the icon. Every other icon closes it. This is the only thing
         * that ever asks for tabs - nobody hovering means nothing asked. */
        HWND tab_target = NULL;
        if (cur_type == 0 && cur_index >= 0 && cur_index < hg_g_window_count) {
            HWND candidate = hg_g_window_items[cur_index].hwnd;
            /* Every task icon outlines its window, tabs or not - "which one is
             * that" is a question about all of them. */
            hg_hilite_show(candidate);
            if (hg_tabs_enabled() && hg_tabs_window_may_have_tabs(candidate))
                tab_target = candidate;
        } else {
            hg_hilite_hide();
        }
        /* Dir, Set and A/V carry lists of their own, opened by pointing at
         * them - the same gesture, the same box, placed the same way clear of
         * the button. */
        int list_button = -1;
        if (cur_type == 1 && taskbox_button_box_opens_on_arrival(cur_index))
            list_button = cur_index;

        if (tab_target) {
            RECT rc_item;
            get_toolbar_item_rect(0, cur_index, rc.right, rc.bottom, icon_size, &rc_item);
            MapWindowPoints(hwnd, NULL, (POINT *)&rc_item, 2);
            hg_tabbox_open(tab_target, &rc_item);
        } else if (list_button >= 0) {
            RECT rc_item;
            get_toolbar_item_rect(1, list_button, rc.right, rc.bottom, icon_size, &rc_item);
            InflateRect(&rc_item, SC(4), SC(4));
            MapWindowPoints(hwnd, NULL, (POINT *)&rc_item, 2);
            taskbox_open_box_for_button(list_button, &rc_item);
        } else if (hg_tabbox_is_open()) {
            hg_tabbox_close();
        }
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
        int border = hg_box_band();
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
        BOOL was_dragged = state->move_drag_started;
        int move_index = state->pressed_index;
        state->is_moving_taskbox = FALSE;
        state->move_drag_started = FALSE;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        if (!was_dragged) {
            /* Nothing moved, so run the button's click role instead of saving
             * the geometry the window already had. */
            if (move_index != -1)
                activate_toolbar_item(move_index);
            return 0;
        }

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
            state->move_drag_started = FALSE;
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
        /* A reading button answers the right button with the choice behind
         * the reading; everything else on the row keeps the shortcut menu it
         * had. */
        if (!hg_toolbar_builtin_context_menu(cur_index, l_param != 0))
            toolbar_controller_show_shortcut_context_menu(hwnd, cur_index, icon_size, l_param);
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
                /* The same rule the context menu follows, and it has to be
                 * stated in both places or the two disagree - which is what
                 * happened: the menu learned about tabs and this did not, so a
                 * middle click on one tab still shut the whole application.
                 * A tab closes its own tab or nothing at all; falling back to
                 * closing the window is the surprise, not the service. */
                if (hg_g_window_items[cur_index].is_tab) {
                    if (!hg_tabs_close(target, hg_g_window_items[cur_index].tab_index))
                        append_message(L"That tab has no close button of its own; nothing was closed");
                } else {
                    PostMessageW(target, WM_CLOSE, 0, 0);
                }
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
    /* The pointer is off the icons, so there is nothing being pointed at for
     * the outline to answer for. The keyboard path re-shows it if the focus is
     * still resting on an icon. */
    hg_hilite_hide();
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
    if (index == HG_TOOL_ICON_MONITOR)
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

/* 5% for volume and opacity, 10% for brightness. Brightness had its own 1% for
 * a while, on the reasoning that a monitor's real scale is known rather than
 * assumed - true, but it made the wheel a hundred notches wide to cross, and a
 * control you have to spin is worse than one that lands slightly off where you
 * wanted. It then shared the 5% (2026-10-06 to 2026-10-14) and moved to 10% on
 * the owner's request: a backlight does not show a 5% step as anything, and
 * every DDC/CI write is a round trip to the monitor. A level off the 10% grid
 * (a monitor that reports 55) snaps to it on the first notch. */
static int toolbar_value_step_percent(int index)
{
    return (index == HG_TOOL_ICON_MONITOR) ? 10 : 5;
}

static int toolbar_value_next_percent(int index, int current, short delta)
{
    int step = toolbar_value_step_percent(index);
    int next = current;
    if (delta > 0) {
        if (current % step == 0) {
            next = current + step;
        } else {
            next = ((current / step) + 1) * step;
        }
        if (next > 100)
            next = 100;
    } else {
        if (current % step == 0) {
            next = current - step;
        } else {
            next = (current / step) * step;
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
    } else if (index == HG_TOOL_ICON_MONITOR) {
        set_system_brightness(value);
    } else if (index == HG_TOOL_ICON_VOLUME) {
        /* Turning this at all means the reader wants to hear the result, so a
         * muted machine is unmuted first - in either direction. Setting the
         * level already lifted mute on the way up (that is what the audio
         * endpoint does for a non-zero level), which left the wheel behaving
         * one way up and another way down: down from a muted machine changed a
         * number nobody could hear. One rule now, and it is the audible one. */
        if (get_system_mute())
            set_system_mute(FALSE);
        set_system_volume(value);
    }
}

BOOL hg_toolbar_value_percent(int index, int *out)
{
    if (!out || !hg_toolbar_builtin_has_value(index))
        return FALSE;
    *out = toolbar_value_current_percent(index);
    return TRUE;
}

BOOL hg_toolbar_value_wheel(int index, short delta)
{
    if (!hg_toolbar_builtin_has_value(index))
        return FALSE;

    int current = toolbar_value_current_percent(index);
    int next = toolbar_value_next_percent(index, current, delta);
    if (next != current)
        toolbar_value_apply_percent(index, next);
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    return TRUE;
}

void hg_toolbar_value_announce(int index)
{
    WCHAR text[64];
    if (hg_toolbar_builtin_value_text(index, HG_TOOLBAR_TEXT_FOCUS, text, HG_ARRAYSIZE(text)))
        append_message(text);
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    hg_tabbox_invalidate();
}

BOOL hg_toolbar_builtin_context_menu(int index, BOOL at_pointer)
{
    HWND owner = hg_g_toolbar_wnd;
    if (!owner)
        return FALSE;

    /* The menus place themselves at the pointer for any non-zero l_param and at
     * the button for zero, which is the convention the row's right click and
     * its context-menu key already use. */
    LPARAM where = at_pointer ? (LPARAM)1 : 0;
    int icon_size = taskbox_toolbar_icon_size();
    if (index == HG_TOOL_ICON_VOLUME) {
        hg_avbox_open(HG_AVBOX_SECTION_AUDIO, -1);
        return TRUE;
    }
    if (index == HG_TOOL_ICON_MONITOR) {
        hg_avbox_open(HG_AVBOX_SECTION_DISPLAY, -1);
        return TRUE;
    }
    if (index == HG_TOOL_ICON_RECBAK && hg_recbak_present()) {
        toolbar_controller_show_recorder_menu(owner, index, icon_size, where);
        return TRUE;
    }
    return FALSE;
}

static void toolbar_update_value_tooltip(HWND hwnd, int index)
{
    if (!hg_g_tooltip_wnd)
        return;

    TOOLINFOW ti = {0};
    ti.cbSize = TOOLINFO_V1_SIZE;
    ti.hwnd = hwnd;
    ti.uId = (UINT_PTR)(hg_g_window_count + index);
    /* The same composer the registration uses, or spinning the wheel would
     * trade the gesture list away for the number it just changed. */
    static WCHAR value_tips[HG_NUM_BASIC_ICONS][256];
    if (index >= 0 && index < HG_NUM_BASIC_ICONS &&
        hg_toolbar_builtin_tooltip_full(index, value_tips[index], HG_ARRAYSIZE(value_tips[index]))) {
        ti.lpszText = value_tips[index];
        SendMessageW(hg_g_tooltip_wnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    }
}

static LRESULT toolbar_controller_on_mouse_wheel(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    /* The box has no focus, so its wheel messages arrive here. A notch spent
     * over a row of the box belongs to it. Vol, Mon and Alp are on the row
     * itself and are answered further down, by rect. */
    if (hg_tabbox_handle_wheel((short)HIWORD(w_param)))
        return 0;

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

            /* Every notch says where it landed, on the status line as well as
             * in the plate's colour - the same words PageUp and PageDown
             * write. The repaint is what turns the plate. */
            toolbar_update_value_tooltip(hwnd, i);
            hg_toolbar_value_announce(i);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
    }

    return 0;
}

LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static ToolbarControllerState state = {-1, -1, -1, -1, 0, FALSE, FALSE, FALSE, {0, 0}, {0, 0, 0, 0}};
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
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP: {
        SendMessage(GetParent(hwnd), msg, w_param, l_param);
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

