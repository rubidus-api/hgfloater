#include "hg_utils.h"
#include "hg_keys.h" /* the tooltips say which chord reaches a button */
#include "hg_recbak.h" /* BakREC is a button only while a recorder is in */
#include "widgets/hg_tabbox.h"
#include "widgets/hg_avbox.h"
#include <knownfolders.h>

static const HgUiPalette s_pal_inanis_light = {
    RGB(0xF8, 0xF6, 0xFB), /* bg: whisper */
    RGB(0xEE, 0xEA, 0xF4), /* bg2: magnolia */
    RGB(0xE0, 0xD8, 0xEC), /* bg3: snuff */
    RGB(0x22, 0x1E, 0x2C), /* fg: bastille */
    RGB(0x59, 0x50, 0x66), /* muted: smoky */
    RGB(0xCE, 0xC5, 0xDB), /* line: blue_haze */
    RGB(0x93, 0x7F, 0xB0), /* line_strong: purple_mountains_majesty */
    RGB(0x58, 0x45, 0x76), /* accent: victoria */
    RGB(0xFF, 0xFF, 0xFF), /* accent_fg: white */
    RGB(0x96, 0x53, 0x0F), /* accent2: hawaiian_tan */
    RGB(0xF2, 0xB0, 0x4A), /* honey: casablanca */
    RGB(0x2B, 0x1D, 0x00), /* honey_fg: brown_tumbleweed */
    RGB(0x85, 0x54, 0x00), /* honey_text: rusty_nail */
    RGB(0xFB, 0xEB, 0xC6), /* honey_soft: oasis */
    RGB(0xD0, 0xC5, 0xE0), /* select: prelude */
    RGB(0x9B, 0x1B, 0x30), /* danger: paprika */
    RGB(0x1D, 0x6E, 0x37), /* ok: fun_green */
    RGB(0x85, 0x54, 0x00)  /* warn: honey_text */
};

static const HgUiPalette s_pal_inanis_dark = {
    RGB(0x1B, 0x18, 0x22), /* bg: vulcan */
    RGB(0x23, 0x1F, 0x2C), /* bg2: baltic_sea */
    RGB(0x33, 0x2C, 0x40), /* bg3: blackcurrant */
    RGB(0xE6, 0xE1, 0xEE), /* fg: selago */
    RGB(0xAA, 0xA1, 0xB8), /* muted: amethyst_smoke */
    RGB(0x3E, 0x36, 0x50), /* line: martinique */
    RGB(0x72, 0x63, 0x93), /* line_strong: deluge */
    RGB(0xC4, 0xB3, 0xE6), /* accent: perfume */
    RGB(0x1B, 0x18, 0x22), /* accent_fg: vulcan */
    RGB(0xD4, 0x88, 0x3B), /* accent2: brandy_punch */
    RGB(0xF2, 0xB0, 0x4A), /* honey: casablanca */
    RGB(0x1B, 0x18, 0x22), /* honey_fg: vulcan */
    RGB(0xF2, 0xB0, 0x4A), /* honey_text: casablanca */
    RGB(0x3B, 0x31, 0x1E), /* honey_soft: birch */
    RGB(0x4A, 0x40, 0x63), /* select: mulled_wine */
    RGB(0xF0, 0x7A, 0x8C), /* danger: froly */
    RGB(0x7B, 0xCB, 0x95), /* ok: de_york */
    RGB(0xF2, 0xB0, 0x4A)  /* warn: casablanca */
};

static const HgUiPalette s_pal_plain_light = {
    RGB(0xFF, 0xFF, 0xFF), /* bg */
    RGB(0xD9, 0xD9, 0xD9), /* bg2 */
    RGB(0xBF, 0xBF, 0xBF), /* bg3 */
    RGB(0x00, 0x00, 0x00), /* fg */
    RGB(0x59, 0x59, 0x59), /* muted */
    RGB(0xA6, 0xA6, 0xA6), /* line */
    RGB(0x00, 0x00, 0x00), /* line_strong */
    RGB(0x00, 0x00, 0x00), /* accent */
    RGB(0xFF, 0xFF, 0xFF), /* accent_fg */
    RGB(0x00, 0x00, 0x00), /* accent2 */
    RGB(0xA6, 0xA6, 0xA6), /* honey */
    RGB(0x00, 0x00, 0x00), /* honey_fg */
    RGB(0x00, 0x00, 0x00), /* honey_text */
    RGB(0xD9, 0xD9, 0xD9), /* honey_soft */
    RGB(0xA6, 0xA6, 0xA6), /* select */
    RGB(0x00, 0x00, 0x00), /* danger */
    RGB(0x00, 0x00, 0x00), /* ok */
    RGB(0x00, 0x00, 0x00)  /* warn */
};

static const HgUiPalette s_pal_plain_dark = {
    RGB(0x00, 0x00, 0x00), /* bg */
    RGB(0x26, 0x26, 0x26), /* bg2 */
    RGB(0x40, 0x40, 0x40), /* bg3 */
    RGB(0xFF, 0xFF, 0xFF), /* fg */
    RGB(0xA6, 0xA6, 0xA6), /* muted */
    RGB(0x59, 0x59, 0x59), /* line */
    RGB(0xFF, 0xFF, 0xFF), /* line_strong */
    RGB(0xFF, 0xFF, 0xFF), /* accent */
    RGB(0x00, 0x00, 0x00), /* accent_fg */
    RGB(0xFF, 0xFF, 0xFF), /* accent2 */
    RGB(0x59, 0x59, 0x59), /* honey */
    RGB(0xFF, 0xFF, 0xFF), /* honey_fg */
    RGB(0xFF, 0xFF, 0xFF), /* honey_text */
    RGB(0x26, 0x26, 0x26), /* honey_soft */
    RGB(0x59, 0x59, 0x59), /* select */
    RGB(0xFF, 0xFF, 0xFF), /* danger */
    RGB(0xFF, 0xFF, 0xFF), /* ok */
    RGB(0xFF, 0xFF, 0xFF)  /* warn */
};

static HgUiPalette s_widget_pal;
static HgUiPalette s_doc_pal;
static BOOL s_pal_initialized = FALSE;

static void build_high_contrast_palette(HgUiPalette *out, BOOL is_widget)
{
    COLORREF bg = GetSysColor(is_widget ? COLOR_BTNFACE : COLOR_WINDOW);
    COLORREF fg = GetSysColor(is_widget ? COLOR_BTNTEXT : COLOR_WINDOWTEXT);
    COLORREF edge = GetSysColor(COLOR_WINDOWFRAME);
    COLORREF hi = GetSysColor(COLOR_HIGHLIGHT);
    COLORREF hifg = GetSysColor(COLOR_HIGHLIGHTTEXT);
    *out = (HgUiPalette){
        .bg = bg,
        .bg2 = bg,
        .bg3 = GetSysColor(COLOR_3DFACE),
        .fg = fg,
        .muted = GetSysColor(COLOR_GRAYTEXT),
        .line = edge,
        .line_strong = fg,
        .accent = hi,
        .accent_fg = hifg,
        .accent2 = hi,
        .honey = hi,
        .honey_fg = hifg,
        .honey_text = fg,
        .honey_soft = bg,
        .select = hi,
        .danger = fg,
        .ok = fg,
        .warn = fg,
    };
}

static void refresh_runtime_palettes(void)
{
    if (hg_g_is_high_contrast) {
        build_high_contrast_palette(&s_widget_pal, TRUE);
        build_high_contrast_palette(&s_doc_pal, FALSE);
        s_pal_initialized = TRUE;
        return;
    }
    BOOL plain = (hg_g_color_scheme_kind == HG_SCHEME_PLAIN);
    if (hg_g_is_dark_mode) {
        s_widget_pal = plain ? s_pal_plain_light : s_pal_inanis_light;
        s_doc_pal = plain ? s_pal_plain_dark : s_pal_inanis_dark;
    } else {
        s_widget_pal = plain ? s_pal_plain_dark : s_pal_inanis_dark;
        s_doc_pal = plain ? s_pal_plain_light : s_pal_inanis_light;
        if (!plain) {
            /* Respect manual [colors] scheme_* overrides on dark widget scheme. */
            if (hg_g_custom_palette.bg != HG_THEME_CUSTOM_BG)
                s_widget_pal.bg = hg_g_custom_palette.bg;
            if (hg_g_custom_palette.border != HG_THEME_CUSTOM_BORDER)
                s_widget_pal.line_strong = hg_g_custom_palette.border;
            if (hg_g_custom_palette.text != HG_THEME_CUSTOM_TEXT)
                s_widget_pal.fg = hg_g_custom_palette.text;
            if (hg_g_custom_palette.flash != HG_THEME_CUSTOM_FLASH)
                s_widget_pal.honey = hg_g_custom_palette.flash;
            if (hg_g_custom_palette.selected != HG_THEME_CUSTOM_SELECTED)
                s_widget_pal.bg3 = hg_g_custom_palette.selected;
        }
    }
    if (hg_g_color_focus_bg != HG_COLOR_FOCUS_BG_DEFAULT)
        s_widget_pal.accent2 = hg_g_color_focus_bg;
    s_pal_initialized = TRUE;
}

static void refresh_system_accent_color(void)
{
    DWORD accent_color = 0;
    BOOL opaque_blend = FALSE;

    if (SUCCEEDED(DwmGetColorizationColor(&accent_color, &opaque_blend))) {
        hg_g_system_accent_color = (COLORREF)(accent_color & 0x00FFFFFFu);
        hg_g_has_system_accent_color = TRUE;
    } else {
        hg_g_system_accent_color = GetSysColor(COLOR_HOTLIGHT);
        hg_g_has_system_accent_color = FALSE;
    }
}

void init_color_scheme(void)
{
    BOOL plain = (hg_g_color_scheme_kind == HG_SCHEME_PLAIN);
    const HgUiPalette *dk = plain ? &s_pal_plain_dark : &s_pal_inanis_dark;
    const HgUiPalette *lt = plain ? &s_pal_plain_light : &s_pal_inanis_light;

    hg_g_color_scheme_dark = plain ? (color_scheme_t){dk->bg, dk->line_strong, dk->fg, dk->honey, dk->bg3}
                                   : hg_g_custom_palette;
    hg_g_color_scheme_light = (color_scheme_t){lt->bg, lt->line_strong, lt->fg, lt->honey, lt->bg3};
}

void update_theme_colors()
{
    HIGHCONTRASTW hc = {0};
    hc.cbSize = sizeof(HIGHCONTRASTW);
    BOOL is_hc = FALSE;
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &hc, 0)) {
        if (hc.dwFlags & HCF_HIGHCONTRASTON)
            is_hc = TRUE;
    }

    hg_g_is_high_contrast = is_hc;
    refresh_system_accent_color();

    hg_g_is_dark_mode = 0;
    DWORD use_light_theme = 1;
    DWORD cbData = sizeof(use_light_theme);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&use_light_theme, &cbData);
        RegCloseKey(hKey);
    }
    if (!hg_g_is_high_contrast && use_light_theme == 0)
        hg_g_is_dark_mode = 1;

    init_color_scheme();
    refresh_runtime_palettes();

    hg_g_color_scheme_selected = (color_scheme_t){
        .bg = s_widget_pal.bg,
        .border = s_widget_pal.line_strong,
        .text = s_widget_pal.fg,
        .flash = s_widget_pal.honey,
        .selected = s_widget_pal.bg3,
    };
}

const HgUiPalette *hg_ui_widget_palette(void)
{
    if (!s_pal_initialized)
        refresh_runtime_palettes();
    return &s_widget_pal;
}

const HgUiPalette *hg_ui_doc_palette(void)
{
    if (!s_pal_initialized)
        refresh_runtime_palettes();
    return &s_doc_pal;
}

COLORREF hg_ui_focus_color(void)
{
    return hg_ui_widget_palette()->accent2;
}

COLORREF hg_ui_doc_focus_color(void)
{
    return hg_ui_doc_palette()->accent2;
}

COLORREF hg_ui_select_color(void)
{
    return hg_ui_widget_palette()->select;
}

COLORREF hg_ui_doc_select_color(void)
{
    return hg_ui_doc_palette()->select;
}

COLORREF hg_ui_doc_line_strong(void)
{
    return hg_ui_doc_palette()->line_strong;
}

COLORREF hg_ui_honey(void)
{
    return hg_ui_widget_palette()->honey;
}

/* The box's em: the status line's font height, which already carries the
 * DPI scale and follows Ctrl+wheel. */
static int hg_box_em(void)
{
    int em = ABS(hg_g_edit_font_size);
    return em > 0 ? em : SC(16);
}

/* The band round a widget box and the thickness of its corner brackets, one
 * number (owner, 2026-11-13: brackets thicker and in proportion to the
 * text): 0.3em, never under the old 5 px - which is what the default 16 px
 * font gives, so the default layout is the one it always was. */
int hg_box_band(void)
{
    int t = (hg_box_em() * 3 + 5) / 10;
    int floor_px = SC(HG_BORDER_THICKNESS);
    return t > floor_px ? t : floor_px;
}

static void paint_brackets_sized(HDC dc, const RECT *rc, COLORREF color, int t, int arm)
{
    HBRUSH brush = hg_cached_solid_brush(color);
    if (!dc || !rc || !brush)
        return;
    int w = rc->right - rc->left, h = rc->bottom - rc->top;
    if (arm > w / 2)
        arm = w / 2;
    if (arm > h / 2)
        arm = h / 2;
    if (t > arm)
        t = arm;
    if (t <= 0)
        return;
    RECT r[8] = {
        {rc->left, rc->top, rc->left + arm, rc->top + t},           {rc->left, rc->top, rc->left + t, rc->top + arm},
        {rc->right - arm, rc->top, rc->right, rc->top + t},         {rc->right - t, rc->top, rc->right, rc->top + arm},
        {rc->left, rc->bottom - t, rc->left + arm, rc->bottom},     {rc->left, rc->bottom - arm, rc->left + t, rc->bottom},
        {rc->right - arm, rc->bottom - t, rc->right, rc->bottom},   {rc->right - t, rc->bottom - arm, rc->right, rc->bottom},
    };
    for (int i = 0; i < 8; ++i)
        FillRect(dc, &r[i], brush);
}

/* A button's brackets: three thick (scaled, never under 3), arms of 8. */
void hg_paint_brackets(HDC dc, const RECT *rc, COLORREF color)
{
    int t = SC(3);
    if (t < 3)
        t = 3;
    paint_brackets_sized(dc, rc, color, t, SC(8));
}

/* norma_ui's box (N-D6) as the owner put it (2026-11-13): a filled rectangle
 * with a bracket on each corner and no line between them. The caller has
 * already filled the box; the band the mouse sizes by is that same fill.
 * The brackets are as thick as the band (hg_box_band) so no child window
 * covers part of them, and their arms 1.5em long. */
void hg_paint_box_frame(HDC dc, const RECT *rc, int thickness, BOOL focused)
{
    if (!dc || !rc || thickness <= 0)
        return;
    int w = rc->right - rc->left;
    int h = rc->bottom - rc->top;
    int arm = (hg_box_em() * 3 + 1) / 2;
    int cap = (w < h ? w : h) / 3; /* D-U4=A: prevent opposing brackets meeting on compact widgets */
    if (cap > 0 && arm > cap)
        arm = cap;
    paint_brackets_sized(dc, rc, focused ? hg_ui_focus_color() : HG_COLOR_BG_TOOLBAR, thickness, arm);
}

BOOL should_refresh_theme_on_setting_change(LPARAM l_param)
{
    const WCHAR *setting = (const WCHAR *)l_param;

    if (!setting || !*setting) {
        return TRUE;
    }

    return lstrcmpiW(setting, L"ImmersiveColorSet") == 0 || lstrcmpiW(setting, L"AppsUseLightTheme") == 0 ||
           lstrcmpiW(setting, L"WindowsThemeElement") == 0 || lstrcmpiW(setting, L"HighContrast") == 0;
}

void apply_dwm_attributes(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;
    BOOL use_immersive_dark_mode = hg_g_is_dark_mode && !hg_g_is_high_contrast;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_immersive_dark_mode,
                          sizeof(use_immersive_dark_mode));
    /* No window border (owner, 2026-11-13): the corner brackets are the
     * widget's whole edge, so Windows 11 draws no line of its own round it. */
    COLORREF no_border = HG_DWMWA_COLOR_NONE;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &no_border, sizeof(no_border));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &hg_g_color_scheme_selected.bg,
                          sizeof(hg_g_color_scheme_selected.bg));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &hg_g_color_scheme_selected.text,
                          sizeof(hg_g_color_scheme_selected.text));

    int corner_pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));
}

void hg_apply_class_background(HWND hwnd)
{
    if (hwnd && IsWindow(hwnd) && hg_g_main_bg_brush) {
        SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hg_g_main_bg_brush);
    }
}

/* LRU GDI brush & pen caches so paint paths stay allocation-free and handle-safe. */
#define HG_BRUSH_CACHE_CAPACITY 32
static struct {
    COLORREF color;
    HBRUSH brush;
} s_solid_brush_cache[HG_BRUSH_CACHE_CAPACITY];
static int s_solid_brush_cache_count = 0;

HBRUSH hg_cached_solid_brush(COLORREF color)
{
    for (int i = 0; i < s_solid_brush_cache_count; ++i) {
        if (s_solid_brush_cache[i].color == color) {
            HBRUSH found = s_solid_brush_cache[i].brush;
            for (int j = i; j > 0; --j) {
                s_solid_brush_cache[j] = s_solid_brush_cache[j - 1];
            }
            s_solid_brush_cache[0].color = color;
            s_solid_brush_cache[0].brush = found;
            return found;
        }
    }

    HBRUSH brush = CreateSolidBrush(color);
    if (!brush) {
        return NULL;
    }

    if (s_solid_brush_cache_count == HG_BRUSH_CACHE_CAPACITY) {
        DeleteObject(s_solid_brush_cache[HG_BRUSH_CACHE_CAPACITY - 1].brush);
        s_solid_brush_cache_count--;
    }
    for (int i = s_solid_brush_cache_count; i > 0; --i) {
        s_solid_brush_cache[i] = s_solid_brush_cache[i - 1];
    }
    s_solid_brush_cache[0].color = color;
    s_solid_brush_cache[0].brush = brush;
    s_solid_brush_cache_count++;
    return brush;
}

void hg_flush_solid_brush_cache(void)
{
    for (int i = 0; i < s_solid_brush_cache_count; ++i) {
        DeleteObject(s_solid_brush_cache[i].brush);
        s_solid_brush_cache[i].brush = NULL;
    }
    s_solid_brush_cache_count = 0;
}

#define HG_PEN_CACHE_CAPACITY 16
static struct {
    int width;
    COLORREF color;
    HPEN pen;
} s_pen_cache[HG_PEN_CACHE_CAPACITY];
static int s_pen_cache_count = 0;

HPEN hg_cached_pen(int width, COLORREF color)
{
    if (width < 1)
        width = 1;
    for (int i = 0; i < s_pen_cache_count; ++i) {
        if (s_pen_cache[i].width == width && s_pen_cache[i].color == color) {
            HPEN found = s_pen_cache[i].pen;
            for (int j = i; j > 0; --j) {
                s_pen_cache[j] = s_pen_cache[j - 1];
            }
            s_pen_cache[0].width = width;
            s_pen_cache[0].color = color;
            s_pen_cache[0].pen = found;
            return found;
        }
    }

    HPEN pen = CreatePen(PS_SOLID, width, color);
    if (!pen)
        return NULL;

    if (s_pen_cache_count == HG_PEN_CACHE_CAPACITY) {
        DeleteObject(s_pen_cache[HG_PEN_CACHE_CAPACITY - 1].pen);
        s_pen_cache_count--;
    }
    for (int i = s_pen_cache_count; i > 0; --i) {
        s_pen_cache[i] = s_pen_cache[i - 1];
    }
    s_pen_cache[0].width = width;
    s_pen_cache[0].color = color;
    s_pen_cache[0].pen = pen;
    s_pen_cache_count++;
    return pen;
}

void hg_flush_pen_cache(void)
{
    for (int i = 0; i < s_pen_cache_count; ++i) {
        DeleteObject(s_pen_cache[i].pen);
        s_pen_cache[i].pen = NULL;
    }
    s_pen_cache_count = 0;
}

void hg_update_scale_from_dpi(UINT dpi)
{
    if (dpi > 0) {
        hg_g_scale_factor = (double)dpi / 96.0;
    }
}

void hg_apply_dpi_suggested_rect(HWND hwnd, LPARAM l_param)
{
    const RECT *rc = (const RECT *)l_param;
    if (hwnd && IsWindow(hwnd) && rc) {
        SetWindowPos(hwnd, NULL, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

/* SetForegroundWindow is refused when this process no longer owns the
 * foreground (for example after an auto-collapse that deliberately left focus
 * with another application). For user-initiated shows, briefly attaching to
 * the foreground thread's input queue restores the right to take focus. */
void hg_force_foreground(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;

    if (SetForegroundWindow(hwnd) && GetForegroundWindow() == hwnd)
        return;

    HWND fg_wnd = GetForegroundWindow();
    DWORD our_tid = GetCurrentThreadId();
    DWORD fg_tid = fg_wnd ? GetWindowThreadProcessId(fg_wnd, NULL) : 0;

    if (fg_tid && fg_tid != our_tid && AttachThreadInput(our_tid, fg_tid, TRUE)) {
        SetForegroundWindow(hwnd);
        AttachThreadInput(our_tid, fg_tid, FALSE);
    }
}

/* Expand the taskbox from the floater: centered on it, but pushed fully inside
 * the work area of the monitor the floater sits on, so a floater parked at a
 * screen edge does not produce a clipped taskbox. The floater's position is
 * remembered first so hiding can restore it instead of re-centering. */
void hg_expand_taskbox_from_floater(HWND floater_wnd, HWND taskbox_wnd)
{
    RECT f_rc, t_rc;

    if (!floater_wnd || !IsWindow(floater_wnd) || !taskbox_wnd || !IsWindow(taskbox_wnd))
        return;
    if (!GetWindowRect(floater_wnd, &f_rc) || !GetWindowRect(taskbox_wnd, &t_rc))
        return;

    hg_g_floater_home_rect = f_rc;
    hg_g_floater_home_valid = TRUE;

    int tw = t_rc.right - t_rc.left;
    int th = t_rc.bottom - t_rc.top;
    int cx = f_rc.left + (f_rc.right - f_rc.left) / 2;
    int cy = f_rc.top + (f_rc.bottom - f_rc.top) / 2;
    int x = cx - tw / 2;
    int y = cy - th / 2;

    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(MonitorFromWindow(floater_wnd, MONITOR_DEFAULTTONEAREST), &mi)) {
        const RECT *work = &mi.rcWork;
        if (x + tw > work->right)
            x = work->right - tw;
        if (y + th > work->bottom)
            y = work->bottom - th;
        if (x < work->left)
            x = work->left;
        if (y < work->top)
            y = work->top;
    }

    SetWindowPos(taskbox_wnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    save_taskbox_geometry_config(x, y, tw, th);

    /* Collapsing measures the taskbox against this rect, so any move made while
     * the taskbox is open carries the floater along with it. */
    hg_g_taskbox_expand_rect.left = x;
    hg_g_taskbox_expand_rect.top = y;
    hg_g_taskbox_expand_rect.right = x + tw;
    hg_g_taskbox_expand_rect.bottom = y + th;
}

/* M click (as opposed to M drag): nudge the pair clear of the area it occupies
 * right now, so whatever sits underneath becomes readable without a manual drag.
 * The step is the smallest one that stops the overlap, and it keeps the heading
 * of the previous click - turning counter-clockwise (north, west, south, east)
 * only when the current heading runs out of room - so repeated clicks walk the
 * pair around the screen instead of bouncing between two spots. The floater
 * follows on collapse. Returns FALSE when no direction has room. */
BOOL hg_relocate_taskbox_away(HWND taskbox_wnd)
{
    RECT t_rc;

    if (!taskbox_wnd || !IsWindow(taskbox_wnd) || !GetWindowRect(taskbox_wnd, &t_rc))
        return FALSE;

    HgBox target = {t_rc.left, t_rc.top, t_rc.right, t_rc.bottom};
    HgBox occupied = target;

    /* The floater's own spot counts as occupied even while it is hidden: that is
     * where collapsing puts it back, so a slot overlapping it is not clear. */
    RECT f_rc;
    SetRectEmpty(&f_rc);
    if (hg_g_floater_home_valid) {
        f_rc = hg_g_floater_home_rect;
    } else if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        if (!GetWindowRect(hg_g_floater_wnd, &f_rc))
            SetRectEmpty(&f_rc);
    }
    if (!IsRectEmpty(&f_rc)) {
        if (f_rc.left < occupied.left)
            occupied.left = f_rc.left;
        if (f_rc.top < occupied.top)
            occupied.top = f_rc.top;
        if (f_rc.right > occupied.right)
            occupied.right = f_rc.right;
        if (f_rc.bottom > occupied.bottom)
            occupied.bottom = f_rc.bottom;
    }

    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(MonitorFromWindow(taskbox_wnd, MONITOR_DEFAULTTONEAREST), &mi))
        return FALSE;

    HgBox work = {mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom};
    HgBox slot;
    HgRelocateDirection taken = hg_calc_relocation(target, occupied, work, hg_g_relocate_direction, &slot);
    if (taken == HG_RELOCATE_NONE)
        return FALSE;

    /* Remember the heading so the next click carries on the same way. */
    hg_g_relocate_direction = taken;

    int tw = slot.right - slot.left;
    int th = slot.bottom - slot.top;
    SetWindowPos(taskbox_wnd, NULL, slot.left, slot.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    save_taskbox_geometry_config(slot.left, slot.top, tw, th);

    /* The floater is not repositioned here: collapsing applies the taskbox's
     * total travel to it, which covers this move and every drag or nudge. */
    return TRUE;
}

/* Shared +/-15 alpha wheel step with the runtime clamp; TRUE when changed. */
BOOL hg_step_alpha_value(BYTE *alpha, int delta)
{
    int new_alpha;

    if (!alpha)
        return FALSE;
    new_alpha = hg_clamp_alpha((int)*alpha + (delta > 0 ? 15 : -15));
    if (*alpha == (BYTE)new_alpha)
        return FALSE;
    *alpha = (BYTE)new_alpha;
    return TRUE;
}

/* Shared WM_CTLCOLOR* handling for the scheme-colored edit controls. */
/* Our own windows that are documents rather than widgets: a page you read or
 * type in, as opposed to a control you press. They take the document colours,
 * and they keep their own keys - Ctrl+X is Cut there, not a command of ours.
 * The monitor previews are widgets despite being separate windows. */
BOOL hg_is_document_window(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return FALSE;

    WCHAR class_name[64];
    if (GetClassNameW(hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0)
        return FALSE;

    return lstrcmpiW(class_name, HG_CLASS_NOTE_EDIT) == 0 || lstrcmpiW(class_name, HG_CLASS_NOTE_LIST) == 0 ||
           lstrcmpiW(class_name, HG_CLASS_CLIP) == 0 || lstrcmpiW(class_name, HG_CLASS_COMMANDBOX) == 0 ||
           lstrcmpiW(class_name, HG_CLASS_SETTINGS) == 0 || lstrcmpiW(class_name, HG_CLASS_ABOUT) == 0;
}

/* --------------------------------------------------- pages of their own (R1.4)
 *
 * The reader's colours for the note page, the clipboard and the command box,
 * and the opacity of the first two. Kept here, beside the theme they override,
 * so that every place that asks for a document colour gets one answer. */
typedef struct HgPageColors {
    BOOL set[2]; /* HG_DOC_COLOR_TEXT, HG_DOC_COLOR_BG */
    COLORREF color[2];
} HgPageColors;

static HgPageColors s_page_colors[HG_DOC_PAGE_COUNT];
static BYTE s_page_alpha[HG_DOC_PAGE_COUNT] = {255, 255, 255, 255};

/* The config keys: [colors] <prefix>_text / <prefix>_bg, [<section>] alpha. */
static const WCHAR *const s_page_key_prefix[HG_DOC_PAGE_COUNT] = {NULL, L"note", L"clip", L"commandbox"};
static const WCHAR *const s_page_alpha_section[HG_DOC_PAGE_COUNT] = {NULL, L"note", L"clipboard", NULL};
static const WCHAR *const s_page_names[HG_DOC_PAGE_COUNT] = {L"Other", L"Note", L"Clipboard", L"Command Box"};

static BOOL hg_page_valid(int page)
{
    return page > HG_DOC_PAGE_OTHER && page < HG_DOC_PAGE_COUNT;
}

const WCHAR *hg_document_page_name(int page)
{
    return (page >= 0 && page < HG_DOC_PAGE_COUNT) ? s_page_names[page] : L"";
}

static void hg_page_color_key(int page, int which, WCHAR *key, size_t cch)
{
    hellgates_wsprintf(key, cch, L"%ls_%ls", s_page_key_prefix[page], which == HG_DOC_COLOR_BG ? L"bg" : L"text");
}

/* RRGGBB, as every other colour in [colors] is written. Anything else -
 * nothing, "theme", a typo - leaves the page on the theme. */
static BOOL hg_parse_rrggbb(const WCHAR *text, COLORREF *out)
{
    unsigned value = 0;
    int digits = 0;
    for (; text[digits]; ++digits) {
        WCHAR c = text[digits];
        unsigned d;
        if (c >= L'0' && c <= L'9')
            d = (unsigned)(c - L'0');
        else if (c >= L'A' && c <= L'F')
            d = (unsigned)(c - L'A' + 10);
        else if (c >= L'a' && c <= L'f')
            d = (unsigned)(c - L'a' + 10);
        else
            return FALSE;
        if (digits >= 6)
            return FALSE;
        value = (value << 4) | d;
    }
    if (digits != 6)
        return FALSE;
    *out = RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
    return TRUE;
}

void hg_document_pages_load(void)
{
    for (int page = HG_DOC_PAGE_OTHER + 1; page < HG_DOC_PAGE_COUNT; ++page) {
        for (int which = 0; which < 2; ++which) {
            WCHAR key[32];
            WCHAR text[16] = {0};
            hg_page_color_key(page, which, key, HG_ARRAYSIZE(key));
            GetPrivateProfileStringW(L"colors", key, L"", text, (DWORD)HG_ARRAYSIZE(text), hg_g_config_path);
            s_page_colors[page].set[which] = hg_parse_rrggbb(text, &s_page_colors[page].color[which]);
        }
        if (s_page_alpha_section[page]) {
            int alpha = (int)GetPrivateProfileIntW(s_page_alpha_section[page], L"alpha", 255, hg_g_config_path);
            s_page_alpha[page] = (BYTE)hg_clamp_alpha(alpha);
        }
    }
}

BOOL hg_document_page_color(int page, int which, COLORREF *out)
{
    if (!hg_page_valid(page) || which < 0 || which > 1 || !s_page_colors[page].set[which])
        return FALSE;
    if (out)
        *out = s_page_colors[page].color[which];
    return TRUE;
}

static void hg_page_windows_apply(int page, BOOL colors, BOOL alpha);

void hg_document_set_page_color(int page, int which, BOOL set, COLORREF color)
{
    if (!hg_page_valid(page) || which < 0 || which > 1)
        return;
    s_page_colors[page].set[which] = set;
    s_page_colors[page].color[which] = color & 0x00FFFFFFu;

    WCHAR key[32];
    WCHAR text[16];
    hg_page_color_key(page, which, key, HG_ARRAYSIZE(key));
    if (set)
        hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%02X%02X%02X", GetRValue(color), GetGValue(color),
                           GetBValue(color));
    else
        StringCchCopyW(text, HG_ARRAYSIZE(text), L"theme");
    WritePrivateProfileStringW(L"colors", key, text, hg_g_config_path);
    hg_flush_solid_brush_cache();
    hg_page_windows_apply(page, TRUE, FALSE);
}

BYTE hg_document_page_alpha(int page)
{
    if (page == HG_DOC_PAGE_COMMANDBOX)
        return hg_g_commandbox_alpha;
    return hg_page_valid(page) ? s_page_alpha[page] : 255;
}

void hg_document_set_page_alpha(int page, BYTE alpha)
{
    if (!hg_page_valid(page) || !s_page_alpha_section[page])
        return;
    s_page_alpha[page] = (BYTE)hg_clamp_alpha(alpha);

    WCHAR text[16];
    hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%u", (UINT)s_page_alpha[page]);
    WritePrivateProfileStringW(s_page_alpha_section[page], L"alpha", text, hg_g_config_path);
    hg_page_windows_apply(page, FALSE, TRUE);
}

void hg_document_apply_page_alpha(HWND hwnd, int page)
{
    if (!hwnd || !IsWindow(hwnd) || !hg_page_valid(page) || !s_page_alpha_section[page])
        return;
    /* Fully opaque, the window is not layered at all: a layered RichEdit or
     * list box costs a redirection surface for nothing a reader can see. */
    BYTE alpha = s_page_alpha[page];
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (alpha >= 255) {
        if (ex & WS_EX_LAYERED) {
            /* A style changed this way is cached until the frame is told, and
             * a shown window can stay composited as layered - or unpainted -
             * until something repaints all of it. */
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex & ~(LONG_PTR)WS_EX_LAYERED);
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
        }
        return;
    }
    if (!(ex & WS_EX_LAYERED))
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

static int hg_page_of_class(const WCHAR *class_name)
{
    if (lstrcmpiW(class_name, HG_CLASS_NOTE_EDIT) == 0 || lstrcmpiW(class_name, HG_CLASS_NOTE_LIST) == 0)
        return HG_DOC_PAGE_NOTE;
    if (lstrcmpiW(class_name, HG_CLASS_CLIP) == 0)
        return HG_DOC_PAGE_CLIP;
    if (lstrcmpiW(class_name, HG_CLASS_COMMANDBOX) == 0)
        return HG_DOC_PAGE_COMMANDBOX;
    return HG_DOC_PAGE_OTHER;
}

typedef struct HgPageApply {
    int page;
    BOOL colors;
    BOOL alpha;
} HgPageApply;

static BOOL CALLBACK hg_page_apply_proc(HWND hwnd, LPARAM l_param)
{
    const HgPageApply *apply = (const HgPageApply *)l_param;
    WCHAR class_name[64];
    if (GetClassNameW(hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0 ||
        hg_page_of_class(class_name) != apply->page)
        return TRUE;
    if (apply->alpha)
        hg_document_apply_page_alpha(hwnd, apply->page);
    if (apply->colors) {
        /* Each window already knows how to take a theme change - the caption,
         * and for a note the RichEdit's colours, which it does not ask for -
         * so a changed colour is handed to it as one. */
        SendMessageW(hwnd, WM_SETTINGCHANGE, 0, (LPARAM)L"ImmersiveColorSet");
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    }
    return TRUE;
}

static void hg_page_windows_apply(int page, BOOL colors, BOOL alpha)
{
    HgPageApply apply = {page, colors, alpha};
    EnumThreadWindows(GetCurrentThreadId(), hg_page_apply_proc, (LPARAM)&apply);
}

/* What the theme says a page of text should look like, with no inversion. High
 * contrast is the one case that is not ours to decide: the user has named the
 * exact colors, and a fixed pair would override the setting that exists to be
 * obeyed. */
HgDocumentColors hg_document_colors(void)
{
    return hg_document_colors_for(HG_DOC_PAGE_OTHER);
}

HgDocumentColors hg_document_colors_for(int page)
{
    const HgUiPalette *doc_pal = hg_ui_doc_palette();
    HgDocumentColors doc = {doc_pal->bg, doc_pal->fg};
    if (!hg_g_is_high_contrast && hg_page_valid(page)) {
        if (s_page_colors[page].set[HG_DOC_COLOR_BG])
            doc.bg = s_page_colors[page].color[HG_DOC_COLOR_BG];
        if (s_page_colors[page].set[HG_DOC_COLOR_TEXT])
            doc.text = s_page_colors[page].color[HG_DOC_COLOR_TEXT];
    }
    return doc;
}

/* A small input field, one step off the page so that it still reads as a place
 * to type once its outline is gone. High contrast keeps the system pair: there
 * the field is told apart by the caret and the focus rectangle the user chose. */
HgDocumentColors hg_document_field_colors(void)
{
    return hg_document_field_colors_for(HG_DOC_PAGE_OTHER);
}

/* One channel a twelfth of the way from the page towards its text. */
static BYTE hg_field_mix(BYTE bg, BYTE text)
{
    return (BYTE)(((int)bg * 11 + (int)text) / 12);
}

HgDocumentColors hg_document_field_colors_for(int page)
{
    HgDocumentColors field = hg_document_colors_for(page);
    if (hg_g_is_high_contrast)
        return field;
    if (hg_page_valid(page) && s_page_colors[page].set[HG_DOC_COLOR_BG]) {
        /* A background of the reader's own: the field is a shade of it rather
         * than the theme's white or near-black, which could be anything
         * against it. */
        field.bg = RGB(hg_field_mix(GetRValue(field.bg), GetRValue(field.text)),
                       hg_field_mix(GetGValue(field.bg), GetGValue(field.text)),
                       hg_field_mix(GetBValue(field.bg), GetBValue(field.text)));
        return field;
    }
    field.bg = hg_ui_doc_palette()->bg2;
    return field;
}

/* The document counterpart of hg_on_ctlcolor_edit. The brush comes from the
 * cache rather than a global, so a theme change cannot leave a control holding
 * a deleted handle: the cache is flushed and the next paint asks again. */
static LRESULT hg_on_ctlcolor_pair(HDC hdc, HgDocumentColors colors)
{
    SetTextColor(hdc, colors.text);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, colors.bg);
    HBRUSH brush = hg_cached_solid_brush(colors.bg);
    return brush ? (LRESULT)brush : (LRESULT)GetStockObject(BLACK_BRUSH);
}

LRESULT hg_on_ctlcolor_document(HDC hdc)
{
    return hg_on_ctlcolor_pair(hdc, hg_document_colors());
}

LRESULT hg_on_ctlcolor_field(HDC hdc)
{
    return hg_on_ctlcolor_pair(hdc, hg_document_field_colors());
}

LRESULT hg_on_ctlcolor_document_for(HDC hdc, int page)
{
    return hg_on_ctlcolor_pair(hdc, hg_document_colors_for(page));
}

LRESULT hg_on_ctlcolor_field_for(HDC hdc, int page)
{
    return hg_on_ctlcolor_pair(hdc, hg_document_field_colors_for(page));
}

/* A document window's own client area, behind and around its controls. Painted
 * here rather than through the class brush because the class brush is the
 * widget colour shared with the floater, and because a brush the class holds
 * is a handle that a theme change can delete underneath it. */
void hg_document_paint_background(HWND hwnd, HDC hdc)
{
    hg_document_paint_background_for(hwnd, hdc, HG_DOC_PAGE_OTHER);
}

void hg_document_paint_background_for(HWND hwnd, HDC hdc, int page)
{
    if (!hwnd || !hdc)
        return;

    HBRUSH brush = hg_cached_solid_brush(hg_document_colors_for(page).bg);
    if (!brush)
        return;

    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, brush);
}

/* A document window's title bar belongs to the page, not to the widgets. Left
 * on the widget scheme it inverts against its own client area - a light grey
 * note under a black caption - which reads as two windows stuck together. */
void hg_apply_dwm_attributes_document(HWND hwnd)
{
    hg_apply_dwm_attributes_document_for(hwnd, HG_DOC_PAGE_OTHER);
}

void hg_apply_dwm_attributes_document_for(HWND hwnd, int page)
{
    if (!hwnd || !IsWindow(hwnd))
        return;

    HgDocumentColors doc = hg_document_colors_for(page);
    BOOL use_immersive_dark_mode = hg_g_is_dark_mode && !hg_g_is_high_contrast;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_immersive_dark_mode,
                          sizeof(use_immersive_dark_mode));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &doc.bg, sizeof(doc.bg));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &doc.text, sizeof(doc.text));
    /* The frame keeps the widget border: it is the one edge that has to be
     * findable against whatever is behind the window, and the caption color
     * cannot serve as its own outline. */
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &hg_g_color_scheme_selected.border,
                          sizeof(hg_g_color_scheme_selected.border));

    int corner_pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));
}

LRESULT hg_on_ctlcolor_edit(HDC hdc)
{
    SetTextColor(hdc, hg_g_color_scheme_selected.text);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, hg_g_color_scheme_selected.bg);
    HBRUSH hbr = hg_cached_solid_brush(hg_g_color_scheme_selected.bg);
    return hbr ? (LRESULT)hbr : (LRESULT)GetStockObject(BLACK_BRUSH);
}

/* Shared single-line edit height measurement (font metrics plus padding). */
int hg_measure_edit_height(HWND edit_wnd, HFONT font, double scale)
{
    int edit_height = SCW(scale, 20);

    if (!edit_wnd || !font)
        return edit_height;

    HDC hdc = GetDC(edit_wnd);
    if (hdc) {
        HFONT old_font = (HFONT)SelectObject(hdc, font);
        TEXTMETRIC tm = {0};
        if (GetTextMetrics(hdc, &tm)) {
            edit_height = (tm.tmHeight + tm.tmExternalLeading) + SCW(scale, 6);
        }
        if (old_font)
            SelectObject(hdc, old_font);
        ReleaseDC(edit_wnd, hdc);
    }
    return edit_height;
}

void hg_theme_settle_later(void)
{
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd))
        SetTimer(hg_g_floater_wnd, HG_TIMER_THEME_SETTLE, 400, NULL);
}

void refresh_theme_surfaces(HWND hwnd)
{
    update_theme_colors();

    apply_dwm_attributes(hwnd);
    if (hg_g_floater_wnd && hg_g_floater_wnd != hwnd) {
        apply_dwm_attributes(hg_g_floater_wnd);
    }
    /* Document windows get the document caption, not the widget one. Doing it
     * here as well as in their own WM_SETTINGCHANGE is what makes the order the
     * broadcast reaches them in stop mattering: whichever runs last, both write
     * the same answer. */
    if (hg_g_about_wnd && hg_g_about_wnd != hwnd) {
        hg_apply_dwm_attributes_document(hg_g_about_wnd);
    }
    if (hg_g_commandbox_wnd && hg_g_commandbox_wnd != hwnd) {
        hg_apply_dwm_attributes_document_for(hg_g_commandbox_wnd, HG_DOC_PAGE_COMMANDBOX);
    }

    hg_flush_solid_brush_cache();
    hg_flush_pen_cache();
    release_brush_handle(&hg_g_main_bg_brush);
    hg_g_main_bg_brush = CreateSolidBrush(HG_CLICKABLE_BG);
    /* Every class registered with the shared brush must be re-stamped, or the class
     * keeps a deleted handle; lazily created windows heal their class on creation. */
    hg_apply_class_background(hwnd);
    hg_apply_class_background(hg_g_floater_wnd);
    /* The command box and the About window are not re-stamped: they paint their
     * own document background, so the class brush is never asked for. */

    if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd)) {
        InvalidateRect(hg_g_commandbox_wnd, NULL, TRUE);
    }
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        InvalidateRect(hg_g_about_wnd, NULL, TRUE);
        HWND edit_wnd = GetDlgItem(hg_g_about_wnd, 100);
        if (edit_wnd) {
            InvalidateRect(edit_wnd, NULL, TRUE);
        }
    }

    if (hg_g_tooltip_wnd && IsWindow(hg_g_tooltip_wnd)) {
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
    }

    hg_tabbox_refresh_theme();
    hg_avbox_refresh_theme();



    if (hwnd) {
        /* The children too: the status line takes its colours from here, and
         * the taskbox clips its children out of its own repaint. */
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
    }
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd)) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
    }
}

HRESULT hellgates_wsprintf(LPWSTR dest, size_t dest_size, LPCWSTR format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    HRESULT hr = StringCchVPrintfW(dest, dest_size, format, arg_list);
    va_end(arg_list);
    return hr;
}

void normalize_path_for_api(const WCHAR *input, WCHAR *output, size_t output_size)
{
    if (!input || !output || output_size == 0)
        return;

    WCHAR full[HG_MAX_PATH];
    if (GetFullPathNameW(input, HG_MAX_PATH, full, NULL) == 0) {
        StringCchCopyW(output, output_size, input);
        return;
    }

    if (wcsncmp(full, L"\\\\?\\", 4) == 0) {
        StringCchCopyW(output, output_size, full);
        return;
    }

    if (iswalpha(full[0]) && full[1] == L':' && full[2] == L'\\') {
        StringCchPrintfW(output, output_size, L"\\\\?\\%ls", full);
    } else if (full[0] == L'\\' && full[1] == L'\\') {
        StringCchPrintfW(output, output_size, L"\\\\?\\UNC\\%ls", full + 2);
    } else {
        StringCchCopyW(output, output_size, full);
    }
}

void init_paths()
{
    WCHAR profile[HG_MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"USERPROFILE", profile, HG_MAX_PATH) == 0) {
        StringCchCopyW(profile, HG_MAX_PATH, L"C:\\");
    }
    hellgates_wsprintf(hg_g_base_path, HG_MAX_PATH, L"%ls\\.HellGates\\hgfloater", profile);
    hellgates_wsprintf(hg_g_shortcuts_path, HG_MAX_PATH, L"%ls\\shortcuts", hg_g_base_path);
    hellgates_wsprintf(hg_g_config_path, HG_MAX_PATH, L"%ls\\config.ini", hg_g_base_path);

    SHCreateDirectoryExW(NULL, hg_g_base_path, NULL);
    SHCreateDirectoryExW(NULL, hg_g_shortcuts_path, NULL);

    WCHAR legacy_config_path[HG_MAX_PATH];
    hellgates_wsprintf(legacy_config_path, HG_MAX_PATH, L"%ls\\config.ini.txt", hg_g_base_path);

    WCHAR norm_legacy[HG_MAX_PATH], norm_config[HG_MAX_PATH];
    normalize_path_for_api(legacy_config_path, norm_legacy, HG_MAX_PATH);
    normalize_path_for_api(hg_g_config_path, norm_config, HG_MAX_PATH);

    if (GetFileAttributesW(norm_legacy) != INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(norm_config) == INVALID_FILE_ATTRIBUTES) {
        MoveFileW(norm_legacy, norm_config);
    }
}

void release_font_handle(HFONT *font, BOOL preserve_stock)
{
    if (!font || !*font)
        return;
    if (!preserve_stock || *font != GetStockObject(DEFAULT_GUI_FONT)) {
        DeleteObject(*font);
    }
    *font = NULL;
    /* Windows reuses GDI handle values, so anything caching per-font results
     * keyed by HFONT would go stale silently. The generation ticks on every
     * release; a cache that remembers the generation it was filled under can
     * tell a live handle from a reincarnated one. */
    hg_g_font_generation++;
}

void release_brush_handle(HBRUSH *brush)
{
    if (!brush || !*brush)
        return;
    DeleteObject(*brush);
    *brush = NULL;
}

void release_bstr(BSTR *value)
{
    if (!value || !*value)
        return;
    SysFreeString(*value);
    *value = NULL;
}

BOOL hg_paint_buffer_begin(HDC target_dc, int width, int height, HgPaintBuffer *buffer)
{
    if (!buffer)
        return FALSE;
    ZeroMemory(buffer, sizeof(*buffer));
    if (!target_dc || width <= 0 || height <= 0)
        return FALSE;

    buffer->dc = CreateCompatibleDC(target_dc);
    if (!buffer->dc)
        return FALSE;

    buffer->bitmap = CreateCompatibleBitmap(target_dc, width, height);
    if (!buffer->bitmap) {
        hg_paint_buffer_end(buffer);
        return FALSE;
    }

    buffer->old_bitmap = (HBITMAP)SelectObject(buffer->dc, buffer->bitmap);
    if (!buffer->old_bitmap) {
        hg_paint_buffer_end(buffer);
        return FALSE;
    }

    return TRUE;
}

void hg_paint_buffer_end(HgPaintBuffer *buffer)
{
    if (!buffer)
        return;
    if (buffer->dc && buffer->old_bitmap)
        SelectObject(buffer->dc, buffer->old_bitmap);
    if (buffer->bitmap)
        DeleteObject(buffer->bitmap);
    if (buffer->dc)
        DeleteDC(buffer->dc);
    ZeroMemory(buffer, sizeof(*buffer));
}

/* The status line shows one message at a time; the clock takes the line back
 * once the message has sat there for HG_STATUS_CLOCK_IDLE_MS. */
void append_message(const WCHAR *msg)
{
    if (!hg_g_edit_msg_wnd || !msg)
        return;

    hg_g_status_is_welcome = FALSE;
    SetWindowTextW(hg_g_edit_msg_wnd, msg);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, 0, 0);
    hg_g_edit_msg_tick = GetTickCount64();
}

/* Called once a second: replaces an idle status line with the current time and
 * keeps it fresh, writing only when the displayed minute actually changes. */
void hg_update_status_clock(void)
{
    if (!hg_g_edit_msg_wnd || !IsWindow(hg_g_edit_msg_wnd))
        return;
    if (GetTickCount64() - hg_g_edit_msg_tick < HG_STATUS_CLOCK_IDLE_MS)
        return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    WCHAR clock_text[32];
    if (hg_calc_format_clock(clock_text, (int)HG_ARRAYSIZE(clock_text), st.wYear, st.wMonth, st.wDay, st.wDayOfWeek,
                             st.wHour, st.wMinute) <= 0)
        return;

    WCHAR current[32];
    GetWindowTextW(hg_g_edit_msg_wnd, current, (int)HG_ARRAYSIZE(current));
    if (wcscmp(current, clock_text) == 0)
        return;

    /* Not append_message: showing the clock must not restart the idle timer. */
    hg_g_status_is_welcome = FALSE;
    SetWindowTextW(hg_g_edit_msg_wnd, clock_text);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, 0, 0);
}

void draw_outlined_text(HDC hdc, const WCHAR *text, int len, RECT *rc, UINT format, COLORREF text_color,
                        COLORREF outline_color)
{
    if (!hdc || !text || !rc || len <= 0)
        return;
    int old_bk_mode = SetBkMode(hdc, TRANSPARENT);
    COLORREF old_text_color = GetTextColor(hdc);

    SetTextColor(hdc, outline_color);
    RECT temp_rc;
    int offset = SC(1);
    if (offset < 1)
        offset = 1;

    int dx[] = {-offset, 0, offset, -offset, offset, -offset, 0, offset};
    int dy[] = {-offset, -offset, -offset, 0, 0, offset, offset, offset};

    for (int i = 0; i < 8; i++) {
        temp_rc = *rc;
        OffsetRect(&temp_rc, dx[i], dy[i]);
        DrawTextW(hdc, text, len, &temp_rc, format);
    }

    SetTextColor(hdc, text_color);
    DrawTextW(hdc, text, len, rc, format);

    SetTextColor(hdc, old_text_color);
    SetBkMode(hdc, old_bk_mode);
}

typedef enum HgToolbarBuiltinValueKind {
    HG_TOOLBAR_VALUE_NONE = 0,
    HG_TOOLBAR_VALUE_ALPHA,
    HG_TOOLBAR_VALUE_BRIGHTNESS,
    HG_TOOLBAR_VALUE_VOLUME
} HgToolbarBuiltinValueKind;

typedef struct HgToolbarBuiltinDescriptor {
    int index;
    /* Three letters where one used to do. A single capital is only legible
     * when the reader already knows the program; "Vol" and "Dir" are legible
     * the first time, and the button is wide enough to say them if the text is
     * fitted to the button rather than the button to the text. */
    const WCHAR *label;
    const WCHAR *focus_text;
    const WCHAR *tooltip_text;
    HgToolbarBuiltinValueKind value_kind;
    HgToolbarClickRole click_role;
    HgToolbarDragRole drag_role;
} HgToolbarBuiltinDescriptor;

const HgResizePreset hg_resize_presets[HG_RESIZE_PRESET_COUNT] = {
    {L"640x480", 640, 480},   {L"800x600", 800, 600},   {L"1024x768", 1024, 768}, {L"1280x960", 1280, 960},
    {L"640x360", 640, 360},   {L"800x480", 800, 480},   {L"960x540", 960, 540},   {L"1280x720", 1280, 720},
    {L"360x640", 360, 640},   {L"480x800", 480, 800},   {L"540x960", 540, 960},   {L"720x1280", 720, 1280},
};

static const HgToolbarBuiltinDescriptor hg_toolbar_builtin_descriptors[] = {
    {HG_TOOL_ICON_RESIZE, L"R", L"Drag to Resize Window", L"Drag to Resize Window", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_RESIZE_TASKBOX},
    {HG_TOOL_ICON_MOVE, L"M", L"Click to Move Aside, Drag to Move Window",
     L"Click to Move Aside, Drag to Move Window", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_RELOCATE_AWAY,
     HG_TOOLBAR_DRAG_MOVE_TASKBOX},
    {HG_TOOL_ICON_CLOSE, L"X", L"Exit hgfloater", L"Exit hgfloater", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_EXIT_APP, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_DESKTOP, L"D", L"Show Desktop", L"Show Desktop", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_TOGGLE_DESKTOP, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_COMMAND, L"Cmd", L"Command Box", L"Command Box", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_SHOW_COMMANDBOX, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_NOTE, L"Note", L"Notes", L"Notes", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_SHOW_NOTES,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_CLIP, L"Clip", L"Clipboard History", L"Clipboard History", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_SHOW_CLIPBOARD, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_SETTINGS, L"Set", L"Opacity, pin, switches, settings",
     L"Opacity, pin, switches, settings", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_OPEN_CONTROLS,
     HG_TOOLBAR_DRAG_NONE},
    /* A/V: what is left in this list is the machine's sound and picture - the
     * output device, how the screens are arranged, each display's scaling and
     * brightness - so the button says so. It was "Opt" while the list also held
     * the shortcuts folder, the lock and About; those are rows of Set now. */
    {HG_TOOL_ICON_MENU, L"A/V", L"Audio and Video: output device, screens, scaling, brightness",
     L"Audio and Video: output device, screens, scaling, brightness", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_OPEN_MENU, HG_TOOLBAR_DRAG_NONE},

    {HG_TOOL_ICON_DIR, L"Dir", L"Folders (hover or click for the list)",
     L"Folders (hover or click for the list)", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_OPEN_DIRS,
     HG_TOOLBAR_DRAG_NONE},

    /* The three that carry a reading, on the row. Their colour is the value,
     * and each of the three gestures a button has answers with a different
     * depth: the wheel changes the reading, the click does the one thing you
     * would want without choosing, and the right button opens the choice.
     *
     * Vol: wheel = volume, click = mute, right = which output device.
     * Mon: wheel = brightness, click = this screen's scaling, right = how the
     * screens are arranged.
     * Alp: wheel = the taskbox's opacity; nothing to choose between. */
    {HG_TOOL_ICON_VOLUME, L"Vol", L"System volume - wheel to set, click to mute, right-click for the device",
     L"System volume - wheel to set, click to mute, right-click for the device", HG_TOOLBAR_VALUE_VOLUME,
     HG_TOOLBAR_CLICK_TOGGLE_MUTE, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_MONITOR, L"Mon", L"Screen brightness - wheel to set, click for scaling, right-click for the arrangement and resolution",
     L"Screen brightness - wheel to set, click for scaling, right-click for the arrangement and resolution",
     HG_TOOLBAR_VALUE_BRIGHTNESS, HG_TOOLBAR_CLICK_OPEN_SCALE_MENU, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_ALPHA, L"Alp", L"Taskbox opacity - wheel to set", L"Taskbox opacity - wheel to set",
     HG_TOOLBAR_VALUE_ALPHA, HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_NONE},

    /* Only while a recorder is plugged in (hg_recbak). Six letters, so the
     * label splitter stacks it as Bak over REC; while the move runs the word
     * is the percent done instead (hg_toolbar_builtin_label). */
    {HG_TOOL_ICON_RECBAK, L"BakREC", L"Move the recorder's recordings to a folder",
     L"Move the recorder's recordings to a folder", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_RECORDER_BACKUP,
     HG_TOOLBAR_DRAG_NONE},

    /* Off the row, in the Set box. It keeps its descriptor because the box
     * draws its row from it: one table still says what every button is called
     * and what it does. */
    {HG_TOOL_ICON_PIN, L"Pin", L"Pin the Taskbox Open", L"Pin the Taskbox Open", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_TOGGLE_PIN, HG_TOOLBAR_DRAG_NONE},
};

/* Eleven on the grid (BakREC only while a recorder is in), plus four off-grid
 * actions reachable from Set or keybindings (Pin, Move Aside, Resize, Exit).
 * Stated rather than derived, so adding a descriptor without deciding where it
 * belongs does not compile. */
enum {
    HG_TOOLBAR_BUILTIN_DESCRIPTOR_COUNT_CHECK =
        1 / ((HG_ARRAYSIZE(hg_toolbar_builtin_descriptors) == HG_NUM_BASIC_ICONS + 4) ? 1 : 0)
};

static const HgToolbarBuiltinDescriptor *hg_toolbar_builtin_descriptor(int index)
{
    /* Without a recorder BakREC is not a button, and its index is the first
     * shortcut's id: nothing may answer for it as a button then. */
    if (index == HG_TOOL_ICON_RECBAK && !hg_recbak_present())
        return NULL;
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_toolbar_builtin_descriptors); ++i) {
        if (hg_toolbar_builtin_descriptors[i].index == index)
            return &hg_toolbar_builtin_descriptors[i];
    }
    return NULL;
}

const WCHAR *hg_toolbar_builtin_label(int index)
{
    if (index == HG_TOOL_ICON_RECBAK && hg_recbak_busy_label())
        return hg_recbak_busy_label();
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->label : L"?";
}

const WCHAR *hg_toolbar_builtin_focus_text(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->focus_text : NULL;
}

const WCHAR *hg_toolbar_builtin_tooltip_text(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->tooltip_text : NULL;
}

/* Which function in the key table a button stands for.
 *
 * A button and a chord that do the same thing are one function seen twice, and
 * the tooltip is where a reader finds that out. Only the buttons that have a
 * key are here; the rest are reached by pointing at them, which is what the
 * tooltip already says. */
typedef struct HgToolbarKeyRow {
    int icon;
    int context;
    const WCHAR *action;
} HgToolbarKeyRow;

static const HgToolbarKeyRow hg_toolbar_key_rows[] = {
    {HG_TOOL_ICON_CLOSE, HG_KEYCTX_WIDGET, L"quit"},
    {HG_TOOL_ICON_COMMAND, HG_KEYCTX_TASKBOX, L"commandbox"},
    {HG_TOOL_ICON_NOTE, HG_KEYCTX_TASKBOX, L"notes"},
    {HG_TOOL_ICON_CLIP, HG_KEYCTX_TASKBOX, L"clipboard"},
    {HG_TOOL_ICON_SETTINGS, HG_KEYCTX_TASKBOX, L"settingslist"},
    {HG_TOOL_ICON_MENU, HG_KEYCTX_TASKBOX, L"optionsmenu"},
};

BOOL hg_toolbar_builtin_tooltip_with_keys(int index, WCHAR *buffer, size_t buffer_cch)
{
    if (!buffer || buffer_cch == 0)
        return FALSE;

    const WCHAR *tooltip = hg_toolbar_builtin_tooltip_text(index);
    if (!tooltip)
        return FALSE;

    for (size_t i = 0; i < HG_ARRAYSIZE(hg_toolbar_key_rows); ++i) {
        if (hg_toolbar_key_rows[i].icon != index)
            continue;

        int action = hg_key_action_find(hg_toolbar_key_rows[i].context, hg_toolbar_key_rows[i].action);
        if (!action)
            break;

        WCHAR chords[128];
        if (!hg_key_bindings_text(action, chords, HG_ARRAYSIZE(chords)))
            break;
        /* A function with every chord cleared answers "-", which is right for a
         * listing and wrong in a tooltip: there is nothing to announce. */
        if (chords[0] == L'-' && chords[1] == L'\0')
            break;

        hellgates_wsprintf(buffer, buffer_cch, L"%ls  (%ls)", tooltip, chords);
        return TRUE;
    }

    return FALSE;
}

/* The first chord, not all of them: a corner has room for one, and the first is
 * the one the table lists first - the primary spelling. */
BOOL hg_toolbar_builtin_badge_text(int index, WCHAR *buffer, size_t buffer_cch)
{
    if (!buffer || buffer_cch == 0)
        return FALSE;

    for (size_t i = 0; i < HG_ARRAYSIZE(hg_toolbar_key_rows); ++i) {
        if (hg_toolbar_key_rows[i].icon != index)
            continue;

        int action = hg_key_action_find(hg_toolbar_key_rows[i].context, hg_toolbar_key_rows[i].action);
        if (!action)
            break;

        HgChord chord;
        if (!hg_key_binding_chord(action, 0, &chord))
            break;
        return hg_key_chord_badge(chord, buffer, buffer_cch);
    }

    return FALSE;
}

BOOL hg_toolbar_builtin_tooltip_full(int index, WCHAR *buffer, size_t buffer_cch)
{
    /* The whole tooltip for one button: the reading first, then what the three
     * gestures do.
     *
     * Both halves had a caller and neither had both. A button whose colour is
     * the reading still owes the reader the number - the colour says "loud",
     * not "70%" - and a button with three different gestures owes them the
     * list, because nothing on the face of it says the right button does
     * anything at all. Composing them here is what stops the two call sites
     * from each picking a different half. */
    if (!buffer || buffer_cch == 0)
        return FALSE;

    WCHAR value[64];
    BOOL has_value = hg_toolbar_builtin_value_text(index, HG_TOOLBAR_TEXT_TOOLTIP, value, HG_ARRAYSIZE(value));

    WCHAR hint[192];
    BOOL has_hint = hg_toolbar_builtin_tooltip_with_keys(index, hint, HG_ARRAYSIZE(hint));
    if (!has_hint) {
        const WCHAR *plain = hg_toolbar_builtin_tooltip_text(index);
        if (plain && SUCCEEDED(StringCchCopyW(hint, HG_ARRAYSIZE(hint), plain)))
            has_hint = TRUE;
    }

    if (has_value && has_hint)
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"%ls\n%ls", value, hint));
    if (has_value)
        return SUCCEEDED(StringCchCopyW(buffer, buffer_cch, value));
    if (has_hint)
        return SUCCEEDED(StringCchCopyW(buffer, buffer_cch, hint));
    return FALSE;
}

BOOL hg_toolbar_builtin_has_value(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc && desc->value_kind != HG_TOOLBAR_VALUE_NONE;
}

BOOL hg_toolbar_builtin_value_text(int index, HgToolbarTextMode mode, WCHAR *buffer, size_t buffer_cch)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    if (!desc || desc->value_kind == HG_TOOLBAR_VALUE_NONE || !buffer || buffer_cch == 0)
        return FALSE;

    switch (desc->value_kind) {
    case HG_TOOLBAR_VALUE_ALPHA: {
        int pct = (hg_g_taskbox_alpha * 100 + 127) / 255;
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"Alpha: %d%%", pct));
    }
    case HG_TOOLBAR_VALUE_BRIGHTNESS:
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"Brightness: %d%%", get_system_brightness()));
    case HG_TOOLBAR_VALUE_VOLUME: {
        const WCHAR *label = (mode == HG_TOOLBAR_TEXT_TOOLTIP) ? L"Vol" : L"System Volume";
        if (get_system_mute()) {
            return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"%ls: %d%% (Muted)", label, get_system_volume()));
        }
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"%ls: %d%%", label, get_system_volume()));
    }
    case HG_TOOLBAR_VALUE_NONE:
    default:
        return FALSE;
    }
}

HgToolbarClickRole hg_toolbar_builtin_click_role(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->click_role : HG_TOOLBAR_CLICK_NONE;
}

HgToolbarDragRole hg_toolbar_builtin_drag_role(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->drag_role : HG_TOOLBAR_DRAG_NONE;
}

int hg_toolbar_grid_shortcuts(void)
{
    return hg_g_shortcut_count;
}

int hg_toolbar_grid_buttons(void)
{
    return hg_recbak_present() ? HG_NUM_BASIC_ICONS : HG_NUM_BASIC_ICONS - 1;
}

/* Every button is in the grid; the strip above it is the status line alone. */
#define HG_TOOLBAR_GRID_BUTTONS hg_toolbar_grid_buttons()

/* Grouped, each kind has rows of its own and the buttons break into their two
 * lines. Otherwise (the default since 2026-11-08, owner's request) nothing
 * breaks: the tasks from the top-left, the shortcuts and the buttons as one run
 * ending in the bottom-right corner, a gap between, and a separator cell
 * between each pair of neighbouring kinds. */
#define HG_TOOLBAR_GRID_LINE                                                                                      \
    (hg_g_taskbox_group_rows ? HG_GRID_BUTTON_BREAK + (HG_TOOLBAR_GRID_BUTTONS - (HG_NUM_BASIC_ICONS - 1)) : 0)
#define HG_TOOLBAR_GRID_MODE (hg_g_taskbox_group_rows ? HG_GRID_GROUPED : HG_GRID_SEPARATED)

int hg_toolbar_grid_item_count(void)
{
    int count = hg_g_window_count + hg_toolbar_grid_shortcuts() + HG_TOOLBAR_GRID_BUTTONS;
    return (count > 0) ? count : 1;
}

int hg_toolbar_rows_for_cols(int cols)
{
    return hg_calc_grid_rows(cols, hg_g_window_count, hg_toolbar_grid_shortcuts(), HG_TOOLBAR_GRID_BUTTONS,
                             HG_TOOLBAR_GRID_LINE, HG_TOOLBAR_GRID_MODE);
}

int hg_toolbar_cols_for_rows(int target_rows)
{
    return hg_calc_grid_cols_for_rows(target_rows, hg_g_window_count, hg_toolbar_grid_shortcuts(),
                                      HG_TOOLBAR_GRID_BUTTONS, HG_TOOLBAR_GRID_LINE, HG_TOOLBAR_GRID_MODE);
}

int hg_toolbar_header_height(int icon_size)
{
    /* Measuring the status line takes a DC, and every rect on the toolbar asks
     * for this, several times a paint. The answer only changes with the font,
     * and every font change ticks hg_g_font_generation. */
    static HFONT cached_font = NULL;
    static unsigned cached_generation = 0;
    static double cached_scale = 0.0;
    static int cached_edit_h = 0;
    if (cached_edit_h <= 0 || cached_font != hg_g_main_font || cached_generation != hg_g_font_generation ||
        cached_scale != hg_g_scale_factor) {
        cached_edit_h = hg_measure_edit_height(hg_g_edit_msg_wnd, hg_g_main_font, hg_g_scale_factor);
        cached_font = hg_g_main_font;
        cached_generation = hg_g_font_generation;
        cached_scale = hg_g_scale_factor;
    }
    (void)icon_size;
    return cached_edit_h;
}

int hg_toolbar_height_for_rows(int rows, int icon_size)
{
    if (rows < 1)
        rows = 1;
    return hg_toolbar_header_height(icon_size) + SC(10) + rows * (icon_size + SC(10));
}

void hg_taskbox_invalidate(BOOL erase)
{
    if (!hg_g_taskbox_wnd || !IsWindow(hg_g_taskbox_wnd))
        return;
    /* RDW_FRAME for the status line, whose flat edge is non-client paint. */
    RedrawWindow(hg_g_taskbox_wnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | (erase ? RDW_ERASE : 0));
}

void hg_toolbar_grid(int width, int height, int icon_size, HgGrid *out)
{
    int cols = get_items_per_row(width, icon_size);
    int row_height = icon_size + SC(10);
    int grid_height = height - hg_toolbar_header_height(icon_size);
    int visible_rows = (row_height > 0) ? (grid_height - SC(20) + SC(10)) / row_height : 1;
    if (visible_rows <= 0)
        visible_rows = 1;

    hg_calc_grid(cols, visible_rows, hg_g_window_count, hg_toolbar_grid_shortcuts(), HG_TOOLBAR_GRID_BUTTONS,
                 HG_TOOLBAR_GRID_LINE, 0, HG_TOOLBAR_GRID_MODE, out);
}

/* item_type 0 is a task icon. item_type 1 is a function button, or - past the
 * last of those, HG_SHORTCUT_BUTTON_ID - a shortcut. An index nothing answers
 * to gets an empty rect. */
void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect)
{
    if (!out_rect)
        return;
    SetRectEmpty(out_rect);
    if (width <= 0)
        return;

    HgGrid grid;
    hg_toolbar_grid(width, height, icon_size, &grid);
    get_toolbar_cell_rect(&grid, hg_calc_grid_cell(&grid, item_type, item_index), icon_size, out_rect);
}

void get_toolbar_cell_rect(const HgGrid *grid, int cell_index, int icon_size, RECT *out_rect)
{
    if (!out_rect)
        return;
    SetRectEmpty(out_rect);
    if (!grid || grid->cols <= 0 || cell_index < 0 || cell_index >= grid->cells)
        return;

    int row = cell_index / grid->cols;
    int col = cell_index % grid->cols;

    out_rect->left = SC(10) + col * (icon_size + SC(15));
    out_rect->top = hg_toolbar_header_height(icon_size) + SC(10) + row * (icon_size + SC(10));
    out_rect->right = out_rect->left + icon_size;
    out_rect->bottom = out_rect->top + icon_size;
}

void update_toolbar_tooltips(HWND hwnd)
{
    if (!hg_g_tooltip_wnd || !hwnd)
        return;
    static int last_total_count = 0;

    for (int i = 0; i < last_total_count; i++) {
        TOOLINFOW ti = {0};
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.hwnd = hwnd;
        ti.uId = (UINT_PTR)i;
        SendMessageW(hg_g_tooltip_wnd, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }
    last_total_count = 0;

    RECT client_rc;
    GetClientRect(hwnd, &client_rc);
    if (client_rc.right <= 0 || client_rc.bottom <= 0)
        return;

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16))
        icon_size = SC(16);

    int total_tasks = hg_g_window_count;
    int total_buttons = hg_toolbar_grid_buttons();
    int id_counter = 0;

    for (int i = 0; i < total_tasks; i++) {
        RECT item_rc;
        get_toolbar_item_rect(0, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        ti_tool.lpszText = hg_g_window_items[i].title;
        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    for (int i = 0; i < total_buttons; i++) {
        RECT item_rc;
        get_toolbar_item_rect(1, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        /* The tooltip control keeps the pointer rather than a copy, so the
         * text has to outlive this call: one buffer per button, filled fresh
         * every time the tooltips are rebuilt. */
        static WCHAR key_tips[HG_NUM_BASIC_ICONS][256];
        if (i >= 0 && i < HG_NUM_BASIC_ICONS &&
            hg_toolbar_builtin_tooltip_full(i, key_tips[i], HG_ARRAYSIZE(key_tips[i]))) {
            ti_tool.lpszText = key_tips[i];
        } else {
            const WCHAR *tooltip_text = hg_toolbar_builtin_tooltip_text(i);
            if (tooltip_text)
                ti_tool.lpszText = (LPWSTR)tooltip_text;
        }

        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    /* The shortcuts: the name of
     * the program, which the array keeps for as long as the shortcut exists. */
    for (int i = 0; i < hg_toolbar_grid_shortcuts(); i++) {
        RECT item_rc;
        get_toolbar_item_rect(1, HG_SHORTCUT_BUTTON_ID(i), client_rc.right, client_rc.bottom, icon_size, &item_rc);
        if (IsRectEmpty(&item_rc))
            continue;

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        ti_tool.lpszText = hg_g_shortcuts[i].name;
        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    /* Not while a box is open - see hg_g_toolbar_tips_suppressed. This runs on
     * every wheel notch, so re-activating unconditionally would undo that. */
    SendMessageW(hg_g_tooltip_wnd, TTM_ACTIVATE, hg_g_toolbar_tips_suppressed ? FALSE : TRUE, 0);
    last_total_count = id_counter;
}

BOOL CALLBACK minimize_restore_enum_proc(HWND hwnd, LPARAM l_param)
{
    BOOL is_minimize = (BOOL)l_param;
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd)
        return TRUE;
    if (!IsWindowVisible(hwnd))
        return TRUE;

    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (owner == NULL || (ex_style & WS_EX_APPWINDOW)) {
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
            if (cloaked != 0)
                return TRUE;
        }

        if (is_minimize) {
            if (!IsIconic(hwnd))
                ShowWindowAsync(hwnd, SW_MINIMIZE);
        } else {
            if (IsIconic(hwnd))
                ShowWindowAsync(hwnd, SW_RESTORE);
        }
    }
    return TRUE;
}

void move_window_by_offset(HWND hwnd, int dx, int dy)
{
    if (!IsWindow(hwnd))
        return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc))
        return;
    SetWindowPos(hwnd, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after movement */
    if (hwnd == hg_g_floater_wnd)
        save_floater_geometry_config(rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
    else if (hwnd == hg_g_taskbox_wnd)
        save_taskbox_geometry_config(rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);

}

void resize_window_by_offset(HWND hwnd, int dw, int dh)
{
    if (!IsWindow(hwnd))
        return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc))
        return;



    int new_w = (rc.right - rc.left) + dw;
    int new_h = (rc.bottom - rc.top) + dh;
    if (new_w < SC(HG_MIN_WINDOW_WIDTH))
        new_w = SC(HG_MIN_WINDOW_WIDTH);
    if (new_h < SC(HG_MIN_WINDOW_HEIGHT))
        new_h = SC(HG_MIN_WINDOW_HEIGHT);
    SetWindowPos(hwnd, NULL, 0, 0, new_w, new_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after resizing */
    if (hwnd == hg_g_floater_wnd)
        save_floater_geometry_config(rc.left, rc.top, new_w, new_h);
    else if (hwnd == hg_g_taskbox_wnd)
        save_taskbox_geometry_config(rc.left, rc.top, new_w, new_h);
}

void disable_window_ime(HWND hwnd)
{
    if (!hwnd) {
        return;
    }

    ImmAssociateContextEx(hwnd, NULL, IACE_DEFAULT);

    HIMC himc = ImmGetContext(hwnd);
    if (!himc) {
        return;
    }

    ImmSetOpenStatus(himc, FALSE);
    ImmReleaseContext(hwnd, himc);
}

BOOL readonly_edit_handle_ime_messages(HWND hwnd, UINT msg, WPARAM w_param)
{
    switch (msg) {
    case WM_IME_SETCONTEXT:
        if (w_param) {
            disable_window_ime(hwnd);
            return TRUE;
        }
        break;
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
        disable_window_ime(hwnd);
        return TRUE;
    }

    return FALSE;
}

/* Common read-only edit subclass handling shared by the taskbox, monitor, and
 * about edits: focus-time IME disable plus IME message suppression. Returns
 * TRUE when the message was fully handled. */
BOOL hg_readonly_edit_common(HWND hwnd, UINT msg, WPARAM w_param)
{
    if (msg == WM_SETFOCUS) {
        disable_window_ime(hwnd);
        return FALSE; /* default focus processing still applies */
    }
    return readonly_edit_handle_ime_messages(hwnd, msg, w_param);
}

WORD *hg_dlg_put_text(WORD *p, const WCHAR *text)
{
    do {
        *p++ = (WORD)*text;
    } while (*text++);
    return p;
}

WORD *hg_dlg_put_item(WORD *p, DWORD style, short x, short y, short cx, short cy, WORD id, WORD atom,
                      const WCHAR *text)
{
    p = (WORD *)(((ULONG_PTR)p + 3) & ~(ULONG_PTR)3); /* items are DWORD aligned */
    DLGITEMTEMPLATE *item = (DLGITEMTEMPLATE *)p;
    item->style = style | WS_CHILD | WS_VISIBLE;
    item->dwExtendedStyle = 0;
    item->x = x;
    item->y = y;
    item->cx = cx;
    item->cy = cy;
    item->id = id;
    p = (WORD *)(item + 1);
    *p++ = 0xFFFF;
    *p++ = atom;
    p = hg_dlg_put_text(p, text);
    *p++ = 0; /* no creation data */
    return p;
}
