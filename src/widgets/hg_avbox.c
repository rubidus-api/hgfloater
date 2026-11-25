#include "hg_avbox.h"
#include "hg_tabbox.h"
#include "hg_monitor.h"
#include "hg_taskbox_internal.h"
#include "../hg_common.h"
#include "../hg_globals.h"
#include "../hg_utils.h"

#define HG_AV_MAX_NAV 12
#define HG_AV_MAX_ITEMS 32
#define HG_AV_SLIDER_STEP_VOL 5
#define HG_AV_SLIDER_STEP_BRIGHT 10

enum {
    HG_AV_ITEM_SLIDER = 0,
    HG_AV_ITEM_TOGGLE,
    HG_AV_ITEM_CARD,
    HG_AV_ITEM_TILE
};

enum {
    HG_AV_CMD_NONE = 0,
    HG_AV_CMD_VOL_SLIDER,
    HG_AV_CMD_BRIGHT_SLIDER,
    HG_AV_CMD_MUTE,
    HG_AV_CMD_PREVIEW,
    HG_AV_CMD_AUDIO_DEV,
    HG_AV_CMD_TOPOLOGY,
    HG_AV_CMD_SCALE,
    HG_AV_CMD_RES
};

typedef struct HgAvNav {
    int section;
    int monitor_idx;
    const WCHAR *group_header;
    WCHAR label[HG_MAX_STR];
    WCHAR sub[32];
    RECT rc;
} HgAvNav;

typedef struct HgAvItem {
    int kind;
    int cmd;
    int arg;
    int value;
    BOOL enabled;
    BOOL checked;
    WCHAR group_header[64];
    WCHAR label[HG_MAX_STR];
    WCHAR right_text[48];
    RECT rc;
    RECT track_rc;
} HgAvItem;

static HWND s_wnd = NULL;
static BOOL s_open = FALSE;
static HgAvNav s_nav[HG_AV_MAX_NAV];
static int s_nav_count = 0;
static int s_nav_sel = 0;
static int s_nav_hover = -1;

static HgAvItem s_items[HG_AV_MAX_ITEMS];
static int s_item_count = 0;
static int s_item_sel = 0;
static int s_item_hover = -1;
static BOOL s_focus_detail = FALSE;
static int s_drag_slider_idx = -1;
static BOOL s_close_hover = FALSE;

static int s_scroll_y = 0;
static int s_content_h = 0;
static RECT s_header_rc = {0, 0, 0, 0};
static RECT s_close_rc = {0, 0, 0, 0};
static RECT s_rail_rc = {0, 0, 0, 0};
static RECT s_detail_rc = {0, 0, 0, 0};
static RECT s_footer_rc = {0, 0, 0, 0};

static WCHAR s_status[160] = L"";
static HFONT s_font_title = NULL;
static HFONT s_font_body = NULL;
static HFONT s_font_small = NULL;
static double s_font_scale = 0.0;
static unsigned s_font_gen = 0;

static void avbox_release_fonts(void)
{
    release_font_handle(&s_font_title, FALSE);
    release_font_handle(&s_font_body, FALSE);
    release_font_handle(&s_font_small, FALSE);
    s_font_scale = 0.0;
}

static void avbox_ensure_fonts(HWND hwnd)
{
    double ws = hg_window_scale(hwnd);
    if (s_font_body && s_font_title && s_font_small && s_font_scale == ws && s_font_gen == hg_g_font_generation)
        return;

    avbox_release_fonts();
    int h_title = SCW(ws, 16);
    int h_body = SCW(ws, 14);
    int h_small = SCW(ws, 12);
    if (h_title < 12)
        h_title = 12;
    if (h_body < 11)
        h_body = 11;
    if (h_small < 10)
        h_small = 10;

    s_font_title = CreateFontW(-h_title, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    s_font_body = CreateFontW(-h_body, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    s_font_small = CreateFontW(-h_small, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    s_font_scale = ws;
    s_font_gen = hg_g_font_generation;
}

static void avbox_add_item(int kind, int cmd, int arg, int value, BOOL enabled, BOOL checked,
                           const WCHAR *group_header, const WCHAR *label, const WCHAR *right_text)
{
    if (s_item_count >= HG_AV_MAX_ITEMS)
        return;
    HgAvItem *it = &s_items[s_item_count++];
    ZeroMemory(it, sizeof(*it));
    it->kind = kind;
    it->cmd = cmd;
    it->arg = arg;
    it->value = value;
    it->enabled = enabled;
    it->checked = checked;
    if (group_header && group_header[0])
        StringCchCopyW(it->group_header, HG_ARRAYSIZE(it->group_header), group_header);
    if (label && label[0])
        StringCchCopyW(it->label, HG_ARRAYSIZE(it->label), label);
    if (right_text && right_text[0])
        StringCchCopyW(it->right_text, HG_ARRAYSIZE(it->right_text), right_text);
}

static void avbox_rebuild_nav(void)
{
    s_nav_count = 0;
    {
        HgAvNav *n = &s_nav[s_nav_count++];
        ZeroMemory(n, sizeof(*n));
        n->section = HG_AVBOX_SECTION_AUDIO;
        n->monitor_idx = -1;
        n->group_header = L"AUDIO";
        StringCchCopyW(n->label, HG_ARRAYSIZE(n->label), L"Output & Volume");
        if (get_system_mute())
            StringCchCopyW(n->sub, HG_ARRAYSIZE(n->sub), L"Muted");
        else
            hellgates_wsprintf(n->sub, HG_ARRAYSIZE(n->sub), L"%d%%", get_system_volume());
    }
    {
        HgAvNav *n = &s_nav[s_nav_count++];
        ZeroMemory(n, sizeof(*n));
        n->section = HG_AVBOX_SECTION_TOPOLOGY;
        n->monitor_idx = -1;
        n->group_header = L"DISPLAYS";
        StringCchCopyW(n->label, HG_ARRAYSIZE(n->label), L"Screen Topology");
        int cur = hg_display_topology_current();
        if (cur >= 0 && cur < HG_TOPOLOGY_COUNT)
            StringCchCopyW(n->sub, HG_ARRAYSIZE(n->sub), hg_display_topology_label(cur));
    }
    for (int i = 0; i < hg_g_monitor_count && s_nav_count < HG_AV_MAX_NAV; ++i) {
        HgAvNav *n = &s_nav[s_nav_count++];
        ZeroMemory(n, sizeof(*n));
        n->section = HG_AVBOX_SECTION_DISPLAY;
        n->monitor_idx = i;
        n->group_header = NULL;
        const WCHAR *name = hg_g_monitors[i].label[0] ? hg_g_monitors[i].label : hg_g_monitors[i].name;
        hellgates_wsprintf(n->label, HG_ARRAYSIZE(n->label), L"%d. %ls", i + 1, name);
        if (hg_g_monitors[i].rcMonitor.left == 0 && hg_g_monitors[i].rcMonitor.top == 0)
            StringCchCopyW(n->sub, HG_ARRAYSIZE(n->sub), L"Primary");
    }
    if (s_nav_sel < 0 || s_nav_sel >= s_nav_count)
        s_nav_sel = 0;
}

static void avbox_rebuild_detail(void)
{
    s_item_count = 0;
    if (s_nav_sel < 0 || s_nav_sel >= s_nav_count)
        return;

    const HgAvNav *nav = &s_nav[s_nav_sel];
    if (nav->section == HG_AVBOX_SECTION_AUDIO) {
        int vol = get_system_volume();
        BOOL muted = get_system_mute() != 0;
        WCHAR vtxt[32];
        hellgates_wsprintf(vtxt, HG_ARRAYSIZE(vtxt), L"%d%%%ls", vol, muted ? L" (Muted)" : L"");
        avbox_add_item(HG_AV_ITEM_SLIDER, HG_AV_CMD_VOL_SLIDER, 0, vol, TRUE, !muted, L"OUTPUT & VOLUME",
                       L"Master Volume", vtxt);
        avbox_add_item(HG_AV_ITEM_TOGGLE, HG_AV_CMD_MUTE, 0, muted ? 1 : 0, TRUE, muted, NULL,
                       muted ? L"Sound Muted  (Click to Unmute)" : L"Mute Output", muted ? L"MUTED" : L"Active");

        if (hg_g_audio_device_count <= 0) {
            avbox_add_item(HG_AV_ITEM_CARD, HG_AV_CMD_NONE, 0, 0, FALSE, FALSE, L"OUTPUT DEVICES",
                           L"(No active output device)", L"--");
        } else {
            for (int i = 0; i < hg_g_audio_device_count; ++i) {
                avbox_add_item(HG_AV_ITEM_CARD, HG_AV_CMD_AUDIO_DEV, i, 0, TRUE, hg_g_audio_devices[i].is_default,
                               (i == 0) ? L"OUTPUT DEVICES" : NULL, hg_g_audio_devices[i].name,
                               hg_g_audio_devices[i].is_default ? L"Default" : L"");
            }
        }
    } else if (nav->section == HG_AVBOX_SECTION_TOPOLOGY) {
        int cur = hg_display_topology_current();
        for (int i = 0; i < HG_TOPOLOGY_COUNT; ++i) {
            avbox_add_item(HG_AV_ITEM_TILE, HG_AV_CMD_TOPOLOGY, i, 0, TRUE, (i == cur),
                           (i == 0) ? L"SCREEN ARRANGEMENT (WIN+P)" : NULL, hg_display_topology_label(i),
                           (i == cur) ? L"Active" : L"");
        }
    } else {
        int m = nav->monitor_idx;
        if (m < 0 || m >= hg_g_monitor_count)
            return;

        WCHAR hdr[64];
        const WCHAR *name = hg_g_monitors[m].label[0] ? hg_g_monitors[m].label : hg_g_monitors[m].name;
        hellgates_wsprintf(hdr, HG_ARRAYSIZE(hdr), L"DISPLAY %d: %ls", m + 1, name);

        avbox_add_item(HG_AV_ITEM_TOGGLE, HG_AV_CMD_PREVIEW, m, hg_g_monitors[m].active ? 1 : 0, TRUE,
                       hg_g_monitors[m].active, hdr, L"Preview Window", hg_g_monitors[m].active ? L"ON" : L"OFF");

        int bright = 0;
        BOOL bright_ok = !hg_monitor_brightness_unavailable(hg_g_monitors[m].hMonitor) &&
                         hg_query_monitor_brightness(hg_g_monitors[m].hMonitor, &bright);
        WCHAR btxt[32];
        if (bright_ok)
            hellgates_wsprintf(btxt, HG_ARRAYSIZE(btxt), L"%d%%", bright);
        else
            StringCchCopyW(btxt, HG_ARRAYSIZE(btxt), L"Unavailable");
        avbox_add_item(HG_AV_ITEM_SLIDER, HG_AV_CMD_BRIGHT_SLIDER, m, bright_ok ? bright : 0, bright_ok, bright_ok,
                       NULL, L"Brightness", btxt);

        HgDisplayScale scale;
        BOOL scale_valid = hg_query_display_scale(hg_g_monitors[m].hMonitor, &scale) && scale.valid;
        for (int i = 0; i < HG_SCALE_OPTION_COUNT; ++i) {
            int pct = hg_display_scale_options[i];
            BOOL ok = scale_valid && pct >= scale.min_percent && pct <= scale.max_percent;
            WCHAR stxt[16];
            hellgates_wsprintf(stxt, HG_ARRAYSIZE(stxt), L"%d%%", pct);
            avbox_add_item(HG_AV_ITEM_TILE, HG_AV_CMD_SCALE, (m << 8) | i, pct, ok,
                           scale_valid && scale.current_percent == pct, (i == 0) ? L"SCALE" : NULL, stxt, NULL);
        }

        int cw = 0, ch = 0;
        BOOL has_res = hg_display_resolution_current(hg_g_monitors[m].name, &cw, &ch);
        for (int i = 0; i < HG_RESOLUTION_OPTION_COUNT; ++i) {
            int rw = hg_display_resolution_options[i][0];
            int rh = hg_display_resolution_options[i][1];
            WCHAR rtxt[24];
            hellgates_wsprintf(rtxt, HG_ARRAYSIZE(rtxt), L"%dx%d", rw, rh);
            avbox_add_item(HG_AV_ITEM_TILE, HG_AV_CMD_RES, (m << 8) | i, 0, TRUE, (has_res && cw == rw && ch == rh),
                           (i == 0) ? L"RESOLUTION" : NULL, rtxt, NULL);
        }
    }

    if (s_item_sel < 0 || s_item_sel >= s_item_count)
        s_item_sel = 0;
}

static void avbox_layout_panes(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    double ws = hg_window_scale(hwnd);
    int band = hg_box_band();
    int hdr_h = SCW(ws, 36);
    int ft_h = SCW(ws, 26);
    int rail_w = SCW(ws, 168);
    if (rail_w > (rc.right - band * 2) / 2)
        rail_w = (rc.right - band * 2) / 2;

    s_header_rc = (RECT){band, band, rc.right - band, band + hdr_h};
    int close_w = SCW(ws, 74);
    int close_pad = SCW(ws, 6);
    s_close_rc = (RECT){s_header_rc.right - close_pad - close_w, s_header_rc.top + close_pad,
                        s_header_rc.right - close_pad, s_header_rc.bottom - close_pad};

    s_footer_rc = (RECT){band, rc.bottom - band - ft_h, rc.right - band, rc.bottom - band};
    s_rail_rc = (RECT){band, s_header_rc.bottom, band + rail_w, s_footer_rc.top};
    s_detail_rc = (RECT){s_rail_rc.right, s_header_rc.bottom, rc.right - band, s_footer_rc.top};

    /* Layout left nav rail. */
    int pad = SCW(ws, 8);
    int nav_y = s_rail_rc.top + pad;
    int hdr_gap = SCW(ws, 18);
    int nav_h = SCW(ws, 36);
    for (int i = 0; i < s_nav_count; ++i) {
        if (s_nav[i].group_header) {
            if (i > 0)
                nav_y += SCW(ws, 8);
            nav_y += hdr_gap;
        }
        s_nav[i].rc = (RECT){s_rail_rc.left + pad / 2, nav_y, s_rail_rc.right - pad / 2, nav_y + nav_h};
        nav_y += nav_h + SCW(ws, 4);
    }

    /* Layout detail items with group headers and multi-column grids. */
    int d_pad = SCW(ws, 12);
    int d_x = s_detail_rc.left + d_pad;
    int d_w = (s_detail_rc.right - s_detail_rc.left) - d_pad * 2;
    if (d_w < SCW(ws, 120))
        d_w = SCW(ws, 120);
    int y = s_detail_rc.top + SCW(ws, 10);
    int tile_gap = SCW(ws, 6);
    int idx = 0;

    while (idx < s_item_count) {
        HgAvItem *it = &s_items[idx];
        if (it->group_header[0]) {
            if (idx > 0)
                y += SCW(ws, 10);
            y += SCW(ws, 20);
        }

        if (it->kind == HG_AV_ITEM_TILE) {
            int cols = (it->cmd == HG_AV_CMD_TOPOLOGY) ? 2 : 3;
            int run_start = idx;
            int run_end = idx + 1;
            while (run_end < s_item_count && s_items[run_end].kind == HG_AV_ITEM_TILE &&
                   s_items[run_end].cmd == it->cmd && !s_items[run_end].group_header[0]) {
                ++run_end;
            }
            int cell_w = (d_w - (cols - 1) * tile_gap) / cols;
            int cell_h = SCW(ws, (it->cmd == HG_AV_CMD_TOPOLOGY) ? 38 : 34);
            for (int k = run_start; k < run_end; ++k) {
                int rel = k - run_start;
                int row = rel / cols;
                int col = rel % cols;
                int tx = d_x + col * (cell_w + tile_gap);
                int ty = y + row * (cell_h + tile_gap);
                s_items[k].rc = (RECT){tx, ty, tx + cell_w, ty + cell_h};
            }
            int rows = (run_end - run_start + cols - 1) / cols;
            y += rows * cell_h + (rows - 1) * tile_gap + SCW(ws, 6);
            idx = run_end;
            continue;
        }

        if (it->kind == HG_AV_ITEM_SLIDER) {
            int h = SCW(ws, 52);
            it->rc = (RECT){d_x, y, d_x + d_w, y + h};
            int bar_pad = SCW(ws, 10);
            int bar_h = SCW(ws, 12);
            it->track_rc = (RECT){it->rc.left + bar_pad, it->rc.bottom - SCW(ws, 10) - bar_h, it->rc.right - bar_pad,
                                  it->rc.bottom - SCW(ws, 10)};
            y += h + SCW(ws, 8);
        } else {
            int h = SCW(ws, 34);
            it->rc = (RECT){d_x, y, d_x + d_w, y + h};
            y += h + SCW(ws, 6);
        }
        ++idx;
    }

    s_content_h = y - (s_detail_rc.top + SCW(ws, 10));
    int view_h = s_detail_rc.bottom - s_detail_rc.top - SCW(ws, 10);
    int max_scroll = s_content_h - view_h;
    if (max_scroll < 0)
        max_scroll = 0;
    if (s_scroll_y > max_scroll)
        s_scroll_y = max_scroll;
    if (s_scroll_y < 0)
        s_scroll_y = 0;
}

static void avbox_ensure_item_visible(int idx)
{
    if (idx < 0 || idx >= s_item_count)
        return;
    RECT r = s_items[idx].rc;
    OffsetRect(&r, 0, -s_scroll_y);
    double ws = hg_window_scale(s_wnd);
    int top_limit = s_detail_rc.top + SCW(ws, 6);
    int bot_limit = s_detail_rc.bottom - SCW(ws, 6);
    if (r.top < top_limit)
        s_scroll_y -= (top_limit - r.top);
    else if (r.bottom > bot_limit)
        s_scroll_y += (r.bottom - bot_limit);
    if (s_scroll_y < 0)
        s_scroll_y = 0;
}

static void avbox_apply_slider(int item_idx, int pct)
{
    if (item_idx < 0 || item_idx >= s_item_count)
        return;
    HgAvItem *it = &s_items[item_idx];
    if (!it->enabled)
        return;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;

    if (it->cmd == HG_AV_CMD_VOL_SLIDER) {
        if (get_system_mute())
            set_system_mute(FALSE);
        set_system_volume(pct);
        int actual = get_system_volume();
        it->value = actual;
        hellgates_wsprintf(it->right_text, HG_ARRAYSIZE(it->right_text), L"%d%%", actual);
        hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Volume: %d%%", actual);
        hg_toolbar_value_announce(HG_TOOL_ICON_VOLUME);
        avbox_rebuild_nav();
    } else if (it->cmd == HG_AV_CMD_BRIGHT_SLIDER) {
        int m = it->arg;
        if (m >= 0 && m < hg_g_monitor_count && hg_g_monitors[m].hMonitor) {
            hg_set_monitor_brightness(hg_g_monitors[m].hMonitor, pct);
            it->value = pct;
            hellgates_wsprintf(it->right_text, HG_ARRAYSIZE(it->right_text), L"%d%%", pct);
            hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Brightness: %d%%", pct);
            update_toolbar_tooltips(hg_g_toolbar_wnd);
        }
    }
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd))
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    if (s_wnd && IsWindow(s_wnd))
        InvalidateRect(s_wnd, NULL, FALSE);
}

static void avbox_activate_item(int idx)
{
    if (idx < 0 || idx >= s_item_count)
        return;
    const HgAvItem *it = &s_items[idx];
    if (!it->enabled)
        return;

    switch (it->cmd) {
    case HG_AV_CMD_MUTE: {
        BOOL next = !get_system_mute();
        set_system_mute(next);
        hg_toolbar_value_announce(HG_TOOL_ICON_VOLUME);
        StringCchCopyW(s_status, HG_ARRAYSIZE(s_status), next ? L"Audio: Muted" : L"Audio: Unmuted");
        break;
    }
    case HG_AV_CMD_AUDIO_DEV: {
        int dev_idx = it->arg;
        if (dev_idx >= 0 && dev_idx < hg_g_audio_device_count) {
            if (set_default_audio_device(hg_g_audio_devices[dev_idx].id)) {
                update_audio_device_list();
                hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Default output: %ls",
                                   hg_g_audio_devices[dev_idx].name);
                hg_toolbar_value_announce(HG_TOOL_ICON_VOLUME);
            }
        }
        break;
    }
    case HG_AV_CMD_TOPOLOGY: {
        int which = it->arg;
        if (hg_display_topology_set(which)) {
            hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Screens: %ls", hg_display_topology_label(which));
        } else {
            hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Screens: %ls is not available",
                               hg_display_topology_label(which));
        }
        append_message(s_status);
        break;
    }
    case HG_AV_CMD_PREVIEW: {
        int m = it->arg;
        if (m >= 0 && m < hg_g_monitor_count) {
            toggle_monitor_window(m);
            hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Preview %d: %ls", m + 1,
                               hg_g_monitors[m].active ? L"ON" : L"OFF");
        }
        break;
    }
    case HG_AV_CMD_SCALE: {
        int m = (it->arg >> 8) & 0xFF;
        int step = it->arg & 0xFF;
        if (m >= 0 && m < hg_g_monitor_count && step >= 0 && step < HG_SCALE_OPTION_COUNT &&
            hg_g_monitors[m].hMonitor) {
            int pct = hg_display_scale_options[step];
            hg_set_display_scale(hg_g_monitors[m].hMonitor, pct);
            hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Scale: %d%%", pct);
        }
        break;
    }
    case HG_AV_CMD_RES: {
        int m = (it->arg >> 8) & 0xFF;
        int step = it->arg & 0xFF;
        if (m >= 0 && m < hg_g_monitor_count && step >= 0 && step < HG_RESOLUTION_OPTION_COUNT) {
            int rw = hg_display_resolution_options[step][0];
            int rh = hg_display_resolution_options[step][1];
            LONG rc_disp = hg_display_resolution_set(hg_g_monitors[m].name, rw, rh);
            if (rc_disp == DISP_CHANGE_SUCCESSFUL) {
                hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Resolution: %d x %d", rw, rh);
            } else {
                const WCHAR *why = (rc_disp == DISP_CHANGE_BADMODE) ? L"not offered by display"
                                   : (rc_disp == DISP_CHANGE_RESTART) ? L"requires restart"
                                                                      : L"refused";
                hellgates_wsprintf(s_status, HG_ARRAYSIZE(s_status), L"Resolution %d x %d: %ls", rw, rh, why);
            }
            append_message(s_status);
        }
        break;
    }
    default:
        return;
    }

    avbox_rebuild_nav();
    avbox_rebuild_detail();
    avbox_layout_panes(s_wnd);
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd))
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    if (s_wnd && IsWindow(s_wnd))
        InvalidateRect(s_wnd, NULL, FALSE);
}

static void avbox_position_window(HWND hwnd)
{
    double ws = hg_window_scale(hg_g_taskbox_wnd ? hg_g_taskbox_wnd : hwnd);
    int want_w = SCW(ws, 560);
    int want_h = SCW(ws, 420);

    RECT anchor = {0, 0, 0, 0};
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd) && IsWindowVisible(hg_g_taskbox_wnd))
        GetWindowRect(hg_g_taskbox_wnd, &anchor);
    else if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd))
        GetWindowRect(hg_g_floater_wnd, &anchor);
    else
        GetCursorPos((POINT *)&anchor);

    HMONITOR mon = MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    RECT work = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (GetMonitorInfoW(mon, &mi))
        work = mi.rcWork;

    int max_w = work.right - work.left - SCW(ws, 16);
    int max_h = work.bottom - work.top - SCW(ws, 16);
    if (want_w > max_w && max_w > 240)
        want_w = max_w;
    if (want_h > max_h && max_h > 200)
        want_h = max_h;

    int x = (anchor.left + anchor.right - want_w) / 2;
    int y = anchor.top - want_h - SCW(ws, 6);
    if (y < work.top)
        y = anchor.bottom + SCW(ws, 6);
    if (x < work.left)
        x = work.left;
    if (x + want_w > work.right)
        x = work.right - want_w;
    if (y + want_h > work.bottom)
        y = work.bottom - want_h;
    if (y < work.top)
        y = work.top;

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, want_w, want_h, SWP_SHOWWINDOW);
}

static BOOL avbox_ensure_window(void)
{
    if (s_wnd && IsWindow(s_wnd))
        return TRUE;
    s_wnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, HG_CLASS_AVBOX, L"A/V Control Center",
                            WS_POPUP | WS_CLIPCHILDREN, 0, 0, 560, 420, hg_g_taskbox_wnd, NULL,
                            GetModuleHandleW(NULL), NULL);
    if (s_wnd)
        apply_dwm_attributes(s_wnd);
    return s_wnd != NULL;
}

static int avbox_find_taskbox_monitor(void)
{
    HWND ref = (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) ? hg_g_taskbox_wnd : hg_g_floater_wnd;
    if (!ref || !IsWindow(ref))
        return 0;
    HMONITOR hmon = MonitorFromWindow(ref, MONITOR_DEFAULTTONEAREST);
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hMonitor == hmon)
            return i;
    }
    return 0;
}

void hg_avbox_open(int section, int monitor_index)
{
    if (!avbox_ensure_window())
        return;

    hg_tabbox_close();
    update_audio_device_list();
    hg_refresh_brightness_cache();
    avbox_rebuild_nav();

    int target_sel = 0;
    if (section == HG_AVBOX_SECTION_TOPOLOGY) {
        target_sel = (s_nav_count > 1) ? 1 : 0;
    } else if (section == HG_AVBOX_SECTION_DISPLAY) {
        int m = (monitor_index >= 0 && monitor_index < hg_g_monitor_count) ? monitor_index : avbox_find_taskbox_monitor();
        for (int i = 0; i < s_nav_count; ++i) {
            if (s_nav[i].section == HG_AVBOX_SECTION_DISPLAY && s_nav[i].monitor_idx == m) {
                target_sel = i;
                break;
            }
        }
    }
    s_nav_sel = target_sel;
    s_item_sel = 0;
    s_scroll_y = 0;
    s_focus_detail = TRUE;
    s_status[0] = L'\0';

    avbox_rebuild_detail();
    avbox_position_window(s_wnd);
    avbox_layout_panes(s_wnd);
    s_open = TRUE;
    hg_force_foreground(s_wnd);
    SetFocus(s_wnd);
    InvalidateRect(s_wnd, NULL, TRUE);
}

void hg_avbox_toggle(int section, int monitor_index)
{
    if (hg_avbox_is_open()) {
        hg_avbox_close();
        return;
    }
    hg_avbox_open(section, monitor_index);
}

void hg_avbox_close(void)
{
    BOOL was_open = s_open || (s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd));
    BOOL had_fg = (s_wnd && GetForegroundWindow() == s_wnd);
    s_drag_slider_idx = -1;
    if (s_wnd && GetCapture() == s_wnd)
        ReleaseCapture();
    s_open = FALSE;
    if (s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd))
        ShowWindow(s_wnd, SW_HIDE);
    if (was_open && had_fg && hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd) && IsWindowVisible(hg_g_taskbox_wnd)) {
        hg_force_foreground(hg_g_taskbox_wnd);
        if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd))
            SetFocus(hg_g_toolbar_wnd);
    }
}

BOOL hg_avbox_is_open(void)
{
    return s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd) && s_open;
}

HWND hg_avbox_window(void)
{
    return s_wnd;
}

void hg_avbox_refresh_theme(void)
{
    avbox_release_fonts();
    if (s_wnd && IsWindow(s_wnd)) {
        apply_dwm_attributes(s_wnd);
        InvalidateRect(s_wnd, NULL, TRUE);
    }
}

static void avbox_paint_header(HDC dc, HWND hwnd, const HgUiPalette *pal)
{
    double ws = hg_window_scale(hwnd);
    HBRUSH hbr_hdr = hg_cached_solid_brush(pal->bg2);
    if (hbr_hdr)
        FillRect(dc, &s_header_rc, hbr_hdr);

    RECT rule = {s_header_rc.left, s_header_rc.bottom - 1, s_header_rc.right, s_header_rc.bottom};
    HBRUSH hbr_rule = hg_cached_solid_brush(pal->line);
    if (hbr_rule)
        FillRect(dc, &rule, hbr_rule);

    RECT title_rc = s_header_rc;
    title_rc.left += SCW(ws, 14);
    title_rc.right = s_close_rc.left - SCW(ws, 8);
    SelectObject(dc, s_font_title);
    SetTextColor(dc, pal->fg);
    DrawTextW(dc, L"Audio / Video Control", -1, &title_rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);

    HBRUSH hbr_close = hg_cached_solid_brush(s_close_hover ? pal->bg3 : pal->bg);
    if (hbr_close)
        FillRect(dc, &s_close_rc, hbr_close);
    HBRUSH hbr_close_line = hg_cached_solid_brush(pal->line);
    if (hbr_close_line)
        FrameRect(dc, &s_close_rc, hbr_close_line);
    SelectObject(dc, s_font_small);
    SetTextColor(dc, pal->fg);
    DrawTextW(dc, L"Esc  Close", -1, &s_close_rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
}

static void avbox_paint_rail(HDC dc, HWND hwnd, const HgUiPalette *pal)
{
    double ws = hg_window_scale(hwnd);
    HBRUSH hbr_rail = hg_cached_solid_brush(pal->bg2);
    if (hbr_rail)
        FillRect(dc, &s_rail_rc, hbr_rail);

    RECT div = {s_rail_rc.right - 1, s_rail_rc.top, s_rail_rc.right, s_rail_rc.bottom};
    HBRUSH hbr_line = hg_cached_solid_brush(pal->line);
    if (hbr_line)
        FillRect(dc, &div, hbr_line);

    int bar_w = SCW(ws, 3);
    if (bar_w < 2)
        bar_w = 2;

    for (int i = 0; i < s_nav_count; ++i) {
        const HgAvNav *n = &s_nav[i];
        if (n->group_header) {
            RECT hrc = {n->rc.left + SCW(ws, 8), n->rc.top - SCW(ws, 16), n->rc.right, n->rc.top - SCW(ws, 2)};
            SelectObject(dc, s_font_small);
            SetTextColor(dc, pal->muted);
            DrawTextW(dc, n->group_header, -1, &hrc, DT_SINGLELINE | DT_BOTTOM | DT_LEFT | DT_NOPREFIX);
        }

        BOOL active = (i == s_nav_sel);
        BOOL hovered = (i == s_nav_hover);
        COLORREF fill = active ? pal->select : (hovered ? pal->bg3 : pal->bg2);
        HBRUSH hbr = hg_cached_solid_brush(fill);
        if (hbr)
            FillRect(dc, &n->rc, hbr);
        if (active) {
            RECT bar = {n->rc.left, n->rc.top, n->rc.left + bar_w, n->rc.bottom};
            HBRUSH acc = hg_cached_solid_brush(pal->accent);
            if (acc)
                FillRect(dc, &bar, acc);
        }
        if (active && !s_focus_detail) {
            HBRUSH ring = hg_cached_solid_brush(pal->accent2);
            if (ring)
                FrameRect(dc, &n->rc, ring);
        }

        RECT trc = {n->rc.left + bar_w + SCW(ws, 8), n->rc.top + SCW(ws, 2), n->rc.right - SCW(ws, 6),
                    n->rc.bottom - (n->sub[0] ? SCW(ws, 14) : SCW(ws, 2))};
        SelectObject(dc, s_font_body);
        SetTextColor(dc, pal->fg);
        DrawTextW(dc, n->label, -1, &trc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

        if (n->sub[0]) {
            RECT src = {trc.left, trc.bottom - SCW(ws, 2), trc.right, n->rc.bottom - SCW(ws, 2)};
            SelectObject(dc, s_font_small);
            SetTextColor(dc, (active && !s_focus_detail) ? pal->fg : pal->muted);
            DrawTextW(dc, n->sub, -1, &src, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
    }
}

static void avbox_paint_detail(HDC dc, HWND hwnd, const HgUiPalette *pal)
{
    double ws = hg_window_scale(hwnd);
    int saved = SaveDC(dc);
    IntersectClipRect(dc, s_detail_rc.left, s_detail_rc.top, s_detail_rc.right, s_detail_rc.bottom);

    int bar_w = SCW(ws, 3);
    if (bar_w < 2)
        bar_w = 2;

    for (int i = 0; i < s_item_count; ++i) {
        const HgAvItem *it = &s_items[i];
        RECT r = it->rc;
        OffsetRect(&r, 0, -s_scroll_y);

        if (it->group_header[0]) {
            RECT hrc = {r.left, r.top - SCW(ws, 18), s_detail_rc.right - SCW(ws, 12), r.top - SCW(ws, 3)};
            SelectObject(dc, s_font_small);
            SetTextColor(dc, pal->muted);
            DrawTextW(dc, it->group_header, -1, &hrc, DT_SINGLELINE | DT_BOTTOM | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);
        }

        if (r.bottom < s_detail_rc.top || r.top > s_detail_rc.bottom)
            continue;

        BOOL focused = (s_focus_detail && i == s_item_sel);
        BOOL hovered = (i == s_item_hover);
        BOOL is_active_tile = (it->kind == HG_AV_ITEM_CARD || it->kind == HG_AV_ITEM_TILE) && it->checked;
        COLORREF fill = is_active_tile ? pal->select : (hovered ? pal->bg3 : pal->bg2);
        HBRUSH hbr_fill = hg_cached_solid_brush(fill);
        if (hbr_fill)
            FillRect(dc, &r, hbr_fill);

        if (is_active_tile || (it->kind == HG_AV_ITEM_TOGGLE && it->checked)) {
            COLORREF ind = (it->cmd == HG_AV_CMD_MUTE && it->checked) ? pal->danger : pal->accent;
            RECT bar = {r.left, r.top, r.left + bar_w, r.bottom};
            HBRUSH hbr_bar = hg_cached_solid_brush(ind);
            if (hbr_bar)
                FillRect(dc, &bar, hbr_bar);
        }

        if (focused) {
            HBRUSH ring = hg_cached_solid_brush(pal->accent2);
            if (ring)
                FrameRect(dc, &r, ring);
        }

        if (it->kind == HG_AV_ITEM_SLIDER) {
            RECT top_rc = {r.left + SCW(ws, 10), r.top + SCW(ws, 4), r.right - SCW(ws, 10),
                           r.top + SCW(ws, 24)};
            SelectObject(dc, s_font_body);
            SetTextColor(dc, it->enabled ? pal->fg : pal->muted);
            DrawTextW(dc, it->label, -1, &top_rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
            DrawTextW(dc, it->right_text, -1, &top_rc, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);

            RECT trk = it->track_rc;
            OffsetRect(&trk, 0, -s_scroll_y);
            HBRUSH hbr_trk = hg_cached_solid_brush(pal->bg);
            if (hbr_trk)
                FillRect(dc, &trk, hbr_trk);
            HBRUSH hbr_tl = hg_cached_solid_brush(pal->line);
            if (hbr_tl)
                FrameRect(dc, &trk, hbr_tl);

            if (it->enabled && it->value > 0) {
                RECT fill_rc = trk;
                fill_rc.right = trk.left + ((trk.right - trk.left) * it->value) / 100;
                HBRUSH hbr_ac = hg_cached_solid_brush(pal->accent);
                if (hbr_ac && fill_rc.right > fill_rc.left)
                    FillRect(dc, &fill_rc, hbr_ac);
            }
            if (it->enabled) {
                int thumb_x = trk.left + ((trk.right - trk.left) * it->value) / 100;
                int tw = SCW(ws, 4);
                RECT thumb = {thumb_x - tw, trk.top - SCW(ws, 2), thumb_x + tw, trk.bottom + SCW(ws, 2)};
                HBRUSH hbr_th = hg_cached_solid_brush(pal->fg);
                if (hbr_th)
                    FillRect(dc, &thumb, hbr_th);
            }
        } else if (it->kind == HG_AV_ITEM_TILE) {
            SelectObject(dc, s_font_body);
            SetTextColor(dc, it->enabled ? pal->fg : pal->muted);
            DrawTextW(dc, it->label, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        } else {
            RECT lrc = {r.left + bar_w + SCW(ws, 8), r.top, r.right - SCW(ws, 72), r.bottom};
            SelectObject(dc, s_font_body);
            SetTextColor(dc, it->enabled ? pal->fg : pal->muted);
            DrawTextW(dc, it->label, -1, &lrc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);

            if (it->right_text[0]) {
                RECT rrc = {r.right - SCW(ws, 70), r.top, r.right - SCW(ws, 10), r.bottom};
                SelectObject(dc, s_font_small);
                COLORREF rcol = pal->muted;
                if (it->cmd == HG_AV_CMD_MUTE && it->checked)
                    rcol = pal->danger;
                else if (it->checked)
                    rcol = pal->accent;
                SetTextColor(dc, rcol);
                DrawTextW(dc, it->right_text, -1, &rrc, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
            }
        }
    }

    if (saved)
        RestoreDC(dc, saved);
}

static void avbox_paint_footer(HDC dc, HWND hwnd, const HgUiPalette *pal)
{
    double ws = hg_window_scale(hwnd);
    HBRUSH hbr_ft = hg_cached_solid_brush(pal->bg2);
    if (hbr_ft)
        FillRect(dc, &s_footer_rc, hbr_ft);
    RECT rule = {s_footer_rc.left, s_footer_rc.top, s_footer_rc.right, s_footer_rc.top + 1};
    HBRUSH hbr_rule = hg_cached_solid_brush(pal->line);
    if (hbr_rule)
        FillRect(dc, &rule, hbr_rule);

    RECT trc = s_footer_rc;
    trc.left += SCW(ws, 12);
    trc.right -= SCW(ws, 12);
    SelectObject(dc, s_font_small);
    SetTextColor(dc, pal->fg);
    const WCHAR *msg = s_status[0] ? s_status : L"Tab: pane  |  Arrows: navigate/adjust  |  Enter: apply  |  Esc: close";
    DrawTextW(dc, msg, -1, &trc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX);
}

static int avbox_nav_at(POINT pt)
{
    for (int i = 0; i < s_nav_count; ++i) {
        if (PtInRect(&s_nav[i].rc, pt))
            return i;
    }
    return -1;
}

static int avbox_item_at(POINT pt)
{
    if (!PtInRect(&s_detail_rc, pt))
        return -1;
    for (int i = 0; i < s_item_count; ++i) {
        RECT r = s_items[i].rc;
        OffsetRect(&r, 0, -s_scroll_y);
        if (PtInRect(&r, pt))
            return i;
    }
    return -1;
}

static int avbox_slider_pct_from_x(int item_idx, int x)
{
    RECT trk = s_items[item_idx].track_rc;
    int w = trk.right - trk.left;
    if (w <= 0)
        return 0;
    int pct = ((x - trk.left) * 100 + w / 2) / w;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    return pct;
}

static BOOL avbox_handle_keydown(WPARAM key)
{
    switch (key) {
    case VK_ESCAPE:
        hg_avbox_close();
        return TRUE;
    case VK_TAB:
        s_focus_detail = !s_focus_detail;
        InvalidateRect(s_wnd, NULL, FALSE);
        return TRUE;
    case VK_UP:
    case 'W':
        if (!s_focus_detail) {
            if (s_nav_sel > 0) {
                --s_nav_sel;
                s_item_sel = 0;
                s_scroll_y = 0;
                avbox_rebuild_detail();
                avbox_layout_panes(s_wnd);
            }
        } else if (s_item_count > 0) {
            int next = s_item_sel - 1;
            if (next >= 0) {
                s_item_sel = next;
                avbox_ensure_item_visible(s_item_sel);
            }
        }
        InvalidateRect(s_wnd, NULL, FALSE);
        return TRUE;
    case VK_DOWN:
    case 'S':
        if (!s_focus_detail) {
            if (s_nav_sel + 1 < s_nav_count) {
                ++s_nav_sel;
                s_item_sel = 0;
                s_scroll_y = 0;
                avbox_rebuild_detail();
                avbox_layout_panes(s_wnd);
            }
        } else if (s_item_count > 0) {
            int next = s_item_sel + 1;
            if (next < s_item_count) {
                s_item_sel = next;
                avbox_ensure_item_visible(s_item_sel);
            }
        }
        InvalidateRect(s_wnd, NULL, FALSE);
        return TRUE;
    case VK_LEFT:
    case 'A':
    case VK_PRIOR:
    case VK_NEXT:
    case VK_RIGHT:
    case 'D':
        if (!s_focus_detail) {
            if (key == VK_RIGHT || key == 'D') {
                s_focus_detail = TRUE;
                InvalidateRect(s_wnd, NULL, FALSE);
            }
            return TRUE;
        }
        if (s_item_sel >= 0 && s_item_sel < s_item_count) {
            const HgAvItem *it = &s_items[s_item_sel];
            if (it->kind == HG_AV_ITEM_SLIDER) {
                int step = (it->cmd == HG_AV_CMD_BRIGHT_SLIDER) ? HG_AV_SLIDER_STEP_BRIGHT : HG_AV_SLIDER_STEP_VOL;
                int dir = (key == VK_RIGHT || key == 'D' || key == VK_PRIOR) ? 1 : -1;
                avbox_apply_slider(s_item_sel, it->value + dir * step);
                return TRUE;
            }
            if (key == VK_LEFT || key == 'A') {
                if (it->kind == HG_AV_ITEM_TILE && s_item_sel > 0 && s_items[s_item_sel - 1].kind == HG_AV_ITEM_TILE) {
                    --s_item_sel;
                } else {
                    s_focus_detail = FALSE;
                }
                avbox_ensure_item_visible(s_item_sel);
                InvalidateRect(s_wnd, NULL, FALSE);
                return TRUE;
            }
            if (key == VK_RIGHT || key == 'D') {
                if (it->kind == HG_AV_ITEM_TILE && s_item_sel + 1 < s_item_count &&
                    s_items[s_item_sel + 1].kind == HG_AV_ITEM_TILE) {
                    ++s_item_sel;
                    avbox_ensure_item_visible(s_item_sel);
                    InvalidateRect(s_wnd, NULL, FALSE);
                }
                return TRUE;
            }
        }
        return TRUE;
    case VK_RETURN:
    case VK_SPACE:
        if (!s_focus_detail) {
            s_focus_detail = TRUE;
            InvalidateRect(s_wnd, NULL, FALSE);
        } else if (s_item_sel >= 0 && s_item_sel < s_item_count) {
            avbox_activate_item(s_item_sel);
        }
        return TRUE;
    default:
        return FALSE;
    }
}

LRESULT CALLBACK avbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        avbox_layout_panes(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        if (rc.right > 0 && rc.bottom > 0) {
            avbox_ensure_fonts(hwnd);
            HgPaintBuffer buffer;
            BOOL buffered = hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &buffer);
            HDC dc = buffered ? buffer.dc : hdc;

            const HgUiPalette *pal = hg_ui_widget_palette();
            HBRUSH hbr_bg = hg_cached_solid_brush(pal->bg);
            if (hbr_bg)
                FillRect(dc, &rc, hbr_bg);

            SetBkMode(dc, TRANSPARENT);
            HFONT old_font = (HFONT)SelectObject(dc, s_font_body);
            avbox_paint_header(dc, hwnd, pal);
            avbox_paint_rail(dc, hwnd, pal);
            avbox_paint_detail(dc, hwnd, pal);
            avbox_paint_footer(dc, hwnd, pal);
            SelectObject(dc, old_font);

            hg_paint_box_frame(dc, &rc, hg_box_band(), TRUE);

            if (buffered) {
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, buffer.dc, 0, 0, SRCCOPY);
                hg_paint_buffer_end(&buffer);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if ((w_param & MK_LBUTTON) && s_drag_slider_idx >= 0 && GetCapture() == hwnd) {
            avbox_apply_slider(s_drag_slider_idx, avbox_slider_pct_from_x(s_drag_slider_idx, pt.x));
            return 0;
        }
        int nav_h = avbox_nav_at(pt);
        int item_h = avbox_item_at(pt);
        BOOL close_h = PtInRect(&s_close_rc, pt);
        if (nav_h != s_nav_hover || item_h != s_item_hover || close_h != s_close_hover) {
            s_nav_hover = nav_h;
            s_item_hover = item_h;
            s_close_hover = close_h;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        if (s_nav_hover != -1 || s_item_hover != -1 || s_close_hover) {
            s_nav_hover = -1;
            s_item_hover = -1;
            s_close_hover = FALSE;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        SetFocus(hwnd);
        if (PtInRect(&s_close_rc, pt)) {
            hg_avbox_close();
            return 0;
        }
        int nav = avbox_nav_at(pt);
        if (nav >= 0) {
            s_nav_sel = nav;
            s_focus_detail = FALSE;
            s_item_sel = 0;
            s_scroll_y = 0;
            avbox_rebuild_detail();
            avbox_layout_panes(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        int item = avbox_item_at(pt);
        if (item >= 0) {
            s_item_sel = item;
            s_focus_detail = TRUE;
            if (s_items[item].kind == HG_AV_ITEM_SLIDER && s_items[item].enabled) {
                s_drag_slider_idx = item;
                SetCapture(hwnd);
                avbox_apply_slider(item, avbox_slider_pct_from_x(item, pt.x));
            } else {
                avbox_activate_item(item);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
        if (s_drag_slider_idx >= 0) {
            s_drag_slider_idx = -1;
            if (GetCapture() == hwnd)
                ReleaseCapture();
        }
        return 0;

    case WM_MOUSEWHEEL: {
        short delta = (short)HIWORD(w_param);
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        ScreenToClient(hwnd, &pt);
        int item = avbox_item_at(pt);
        if (item >= 0 && s_items[item].kind == HG_AV_ITEM_SLIDER && s_items[item].enabled) {
            int step = (s_items[item].cmd == HG_AV_CMD_BRIGHT_SLIDER) ? HG_AV_SLIDER_STEP_BRIGHT : HG_AV_SLIDER_STEP_VOL;
            avbox_apply_slider(item, s_items[item].value + ((delta > 0) ? step : -step));
            return 0;
        }
        double ws = hg_window_scale(hwnd);
        int view_h = s_detail_rc.bottom - s_detail_rc.top - SCW(ws, 10);
        int max_scroll = s_content_h - view_h;
        if (max_scroll > 0) {
            s_scroll_y -= (delta > 0) ? SCW(ws, 36) : -SCW(ws, 36);
            if (s_scroll_y < 0)
                s_scroll_y = 0;
            if (s_scroll_y > max_scroll)
                s_scroll_y = max_scroll;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (avbox_handle_keydown(w_param))
            return 0;
        break;

    case WM_DESTROY:
        avbox_release_fonts();
        if (s_wnd == hwnd) {
            s_wnd = NULL;
            s_open = FALSE;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
