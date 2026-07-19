#include "hg_utils.h"
#include <knownfolders.h>

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
    hg_g_color_scheme_dark = hg_g_custom_palette; /* configurable via [colors] */

    /* GetSysColor values do not follow app dark mode, so they form the light scheme. */
    hg_g_color_scheme_light = (color_scheme_t){
        .bg = GetSysColor(COLOR_WINDOW),
        .border = GetSysColor(COLOR_WINDOWFRAME),
        .text = GetSysColor(COLOR_WINDOWTEXT),
        .flash = GetSysColor(COLOR_HOTLIGHT),
        .selected = GetSysColor(COLOR_HIGHLIGHT),
    };
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

    if (!hg_g_is_high_contrast && hg_g_has_system_accent_color) {
        hg_g_color_scheme_light.flash = hg_g_system_accent_color;
    }

    if (hg_g_is_high_contrast) {
        hg_g_color_scheme_selected = (color_scheme_t){
            .bg = GetSysColor(COLOR_BTNFACE),
            .border = GetSysColor(COLOR_WINDOWFRAME),
            .text = GetSysColor(COLOR_BTNTEXT),
            .flash = GetSysColor(COLOR_HOTLIGHT),
            .selected = GetSysColor(COLOR_HIGHLIGHT),
        };
    } else if (hg_g_is_dark_mode) {
        /* Deliberate contrast inversion (see SPEC): the widgets must stand out
         * against the system theme, so dark systems get the light scheme and
         * light systems get the dark custom palette. */
        hg_g_color_scheme_selected = hg_g_color_scheme_light;
    } else {
        hg_g_color_scheme_selected = hg_g_color_scheme_dark;
    }
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
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &hg_g_color_scheme_selected.border,
                          sizeof(hg_g_color_scheme_selected.border));
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

/* Small color-keyed cache so paint paths stop creating and destroying dozens
 * of solid brushes per frame. Entries are never selected across messages, so
 * evicting the oldest entry when the cache is full is safe. */
#define HG_BRUSH_CACHE_CAPACITY 16
static struct {
    COLORREF color;
    HBRUSH brush;
} s_solid_brush_cache[HG_BRUSH_CACHE_CAPACITY];
static int s_solid_brush_cache_count = 0;

HBRUSH hg_cached_solid_brush(COLORREF color)
{
    for (int i = 0; i < s_solid_brush_cache_count; ++i) {
        if (s_solid_brush_cache[i].color == color) {
            return s_solid_brush_cache[i].brush;
        }
    }

    HBRUSH brush = CreateSolidBrush(color);
    if (!brush) {
        return NULL;
    }

    if (s_solid_brush_cache_count == HG_BRUSH_CACHE_CAPACITY) {
        DeleteObject(s_solid_brush_cache[0].brush);
        for (int i = 1; i < HG_BRUSH_CACHE_CAPACITY; ++i) {
            s_solid_brush_cache[i - 1] = s_solid_brush_cache[i];
        }
        s_solid_brush_cache_count--;
    }
    s_solid_brush_cache[s_solid_brush_cache_count].color = color;
    s_solid_brush_cache[s_solid_brush_cache_count].brush = brush;
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

/* M click (as opposed to M drag): nudge the pair to the first cardinal slot -
 * north, then west, then south, then east - that clears the area they occupy
 * right now, so whatever sits underneath becomes readable without a manual
 * drag. The step is the smallest one that stops the overlap: the pair lands
 * flush against the area it just vacated rather than at the screen edge. The
 * floater rides along to the same corner a move-drag would leave it at. Returns
 * FALSE when no slot clears the current area, leaving the pair put. */
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
    if (hg_calc_relocation(target, occupied, work, &slot) == HG_RELOCATE_NONE)
        return FALSE;

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
LRESULT hg_on_ctlcolor_edit(HDC hdc)
{
    SetTextColor(hdc, hg_g_color_scheme_selected.text);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, hg_g_color_scheme_selected.bg);
    if (!hg_g_edit_bg_brush)
        hg_g_edit_bg_brush = CreateSolidBrush(hg_g_color_scheme_selected.bg);
    return hg_g_edit_bg_brush ? (LRESULT)hg_g_edit_bg_brush : (LRESULT)GetStockObject(BLACK_BRUSH);
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

void refresh_theme_surfaces(HWND hwnd)
{
    update_theme_colors();

    apply_dwm_attributes(hwnd);
    if (hg_g_floater_wnd && hg_g_floater_wnd != hwnd) {
        apply_dwm_attributes(hg_g_floater_wnd);
    }
    if (hg_g_about_wnd && hg_g_about_wnd != hwnd) {
        apply_dwm_attributes(hg_g_about_wnd);
    }

    hg_flush_solid_brush_cache();
    release_brush_handle(&hg_g_main_bg_brush);
    hg_g_main_bg_brush = CreateSolidBrush(HG_CLICKABLE_BG);
    /* Every class registered with the shared brush must be re-stamped, or the class
     * keeps a deleted handle; lazily created windows heal their class on creation. */
    hg_apply_class_background(hwnd);
    hg_apply_class_background(hg_g_floater_wnd);
    hg_apply_class_background(hg_g_commandbox_wnd);

    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        hg_apply_class_background(hg_g_about_wnd);
        InvalidateRect(hg_g_about_wnd, NULL, TRUE);
        HWND edit_wnd = GetDlgItem(hg_g_about_wnd, 100);
        if (edit_wnd) {
            InvalidateRect(edit_wnd, NULL, TRUE);
        }
    }

    release_brush_handle(&hg_g_edit_bg_brush);

    if (hg_g_tooltip_wnd && IsWindow(hg_g_tooltip_wnd)) {
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
    }



    if (hwnd) {
        InvalidateRect(hwnd, NULL, TRUE);
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
    WCHAR label;
    const WCHAR *focus_text;
    const WCHAR *tooltip_text;
    HgToolbarBuiltinValueKind value_kind;
    HgToolbarClickRole click_role;
    HgToolbarDragRole drag_role;
} HgToolbarBuiltinDescriptor;

static const HgToolbarBuiltinDescriptor hg_toolbar_builtin_descriptors[] = {
    {HG_TOOL_ICON_RESIZE, L'R', L"Drag to Resize Window", L"Drag to Resize Window", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_RESIZE_TASKBOX},
    {HG_TOOL_ICON_MOVE, L'M', L"Click to Move Aside, Drag to Move Window",
     L"Click to Move Aside, Drag to Move Window", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_RELOCATE_AWAY,
     HG_TOOLBAR_DRAG_MOVE_TASKBOX},
    {HG_TOOL_ICON_CLOSE, L'X', L"Exit hgfloater", L"Exit hgfloater", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_EXIT_APP, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_DESKTOP, L'D', L"Show Desktop", L"Show Desktop", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_TOGGLE_DESKTOP, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_MENU, L'O', L"Options", L"Options", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_OPEN_MENU,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_COMMAND, L'C', L"Command Box", L"Command Box", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_SHOW_COMMANDBOX, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_ALPHA, L'A', NULL, NULL, HG_TOOLBAR_VALUE_ALPHA, HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_BRIGHTNESS, L'B', NULL, NULL, HG_TOOLBAR_VALUE_BRIGHTNESS, HG_TOOLBAR_CLICK_NONE,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_VOLUME, L'V', NULL, NULL, HG_TOOLBAR_VALUE_VOLUME, HG_TOOLBAR_CLICK_TOGGLE_MUTE,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_FLOATER, L'F', L"Floater (Ctrl+Wheel size, Alt+Wheel alpha; click to return)",
     L"Floater (Ctrl+Wheel size, Alt+Wheel alpha; click to return)", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_FLOATER_ADJUST, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_PIN, L'P', L"Pin the Taskbox Open", L"Pin the Taskbox Open", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_TOGGLE_PIN, HG_TOOLBAR_DRAG_NONE},
};

enum {
    HG_TOOLBAR_BUILTIN_DESCRIPTOR_COUNT_CHECK =
        1 / ((HG_ARRAYSIZE(hg_toolbar_builtin_descriptors) == HG_NUM_BASIC_ICONS) ? 1 : 0)
};

static const HgToolbarBuiltinDescriptor *hg_toolbar_builtin_descriptor(int index)
{
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_toolbar_builtin_descriptors); ++i) {
        if (hg_toolbar_builtin_descriptors[i].index == index)
            return &hg_toolbar_builtin_descriptors[i];
    }
    return NULL;
}

WCHAR hg_toolbar_builtin_label(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->label : L'?';
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

void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect)
{
    if (width <= 0 || !out_rect) {
        if (out_rect)
            SetRectEmpty(out_rect);
        return;
    }

    int cols = get_items_per_row(width, icon_size);
    if (cols <= 0)
        cols = 1;
    int row_height = icon_size + SC(10);

    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

    int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (min_required_rows <= 0)
        min_required_rows = 1;

    int visible_rows = (height - SC(20) + SC(10)) / row_height;
    if (visible_rows <= 0)
        visible_rows = 1;

    int rows = (visible_rows > min_required_rows) ? visible_rows : min_required_rows;

    int total_cells = rows * cols;

    int cell_index;
    if (item_type == 0) {
        cell_index = item_index;
    } else {
        cell_index = total_cells - 1 - item_index;
    }

    int row = cell_index / cols;
    int col = cell_index % cols;

    out_rect->left = SC(10) + col * (icon_size + SC(15));
    out_rect->top = SC(10) + row * row_height;
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
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
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

    for (int i = 0; i < total_shortcuts; i++) {
        RECT item_rc;
        get_toolbar_item_rect(1, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        const WCHAR *tooltip_text = hg_toolbar_builtin_tooltip_text(i);
        if (tooltip_text) {
            ti_tool.lpszText = (LPWSTR)tooltip_text;
        } else if (hg_toolbar_builtin_has_value(i)) {
            static WCHAR value_tips[HG_NUM_BASIC_ICONS][64];
            if (hg_toolbar_builtin_value_text(i, HG_TOOLBAR_TEXT_TOOLTIP, value_tips[i],
                                              HG_ARRAYSIZE(value_tips[i]))) {
                ti_tool.lpszText = value_tips[i];
            }
        } else {
            ti_tool.lpszText = hg_g_shortcuts[i - HG_NUM_BASIC_ICONS].name;
        }

        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    SendMessageW(hg_g_tooltip_wnd, TTM_ACTIVATE, TRUE, 0);
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
