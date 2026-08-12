#ifndef HG_UTILS_H
#define HG_UTILS_H

#include "hg_common.h"

/* Manual GUID definitions for Core Audio to avoid including initguid.h which clashes with uuid.lib */
extern const GUID CLSID_MMDeviceEnumerator;
extern const GUID IID_IMMDeviceEnumerator;
extern const GUID IID_IAudioEndpointVolume;

/* Utility Functions */
int get_system_brightness(void);
void set_system_brightness(int brightness);
void restore_system_gamma(void);
int get_system_volume(void);
void set_system_volume(int percent);
int get_system_mute(void);
void set_system_mute(int mute);
void hg_reset_audio_endpoint_cache(void);
void hg_refresh_brightness_cache(void);
void update_audio_device_list(void);
BOOL set_default_audio_device(const WCHAR *device_id);
void init_color_scheme(void);
void update_theme_colors(void);
void apply_dwm_attributes(HWND hwnd);
void hg_apply_class_background(HWND hwnd);
void hg_update_scale_from_dpi(UINT dpi);
void hg_apply_dpi_suggested_rect(HWND hwnd, LPARAM l_param);
void hg_force_foreground(HWND hwnd);
void hg_expand_taskbox_from_floater(HWND floater_wnd, HWND taskbox_wnd);
BOOL hg_relocate_taskbox_away(HWND taskbox_wnd);
double hg_window_scale(HWND hwnd);
double hg_point_scale(POINT pt);

/* Per-monitor Windows display-scale (DPI) control. The supported percentages a
 * monitor exposes are a contiguous window; hg_display_scale_options() reports
 * that window and the current value so a menu can check and disable entries. */
#define HG_SCALE_OPTION_COUNT 6
typedef struct HgDisplayScale {
    BOOL valid;            /* the query succeeded */
    int current_percent;   /* 0 when the current value is unknown */
    int min_percent;       /* lowest percentage this monitor allows */
    int max_percent;       /* highest percentage this monitor allows */
    WCHAR name[128];       /* friendly monitor name, or its GDI device name */
} HgDisplayScale;

/* The fixed set offered in the menu, in order. */
extern const int hg_display_scale_options[HG_SCALE_OPTION_COUNT];

BOOL hg_query_display_scale(HMONITOR monitor, HgDisplayScale *out);
BOOL hg_set_display_scale(HMONITOR monitor, int percent);

/* Per-monitor backlight. DDC/CI drives the real backlight where the display
 * answers; the rest fall back to a gamma ramp on that display's own DC, so
 * dimming one monitor no longer dims the whole desktop. */
#define HG_BRIGHTNESS_OPTION_COUNT 5
extern const int hg_brightness_options[HG_BRIGHTNESS_OPTION_COUNT];
BOOL hg_query_monitor_brightness(HMONITOR monitor, int *out_percent);
/* TRUE only once a display has been probed and answered nothing at all, so a
 * menu can grey the entry instead of accepting a click that does nothing.
 * A display not yet probed is not claimed to be unavailable. */
BOOL hg_monitor_brightness_unavailable(HMONITOR monitor);
void hg_set_monitor_brightness(HMONITOR monitor, int percent);
void hg_refresh_all_monitor_brightness(void);

/* The internal panel's real backlight, over WMI. It is not a DDC/CI device, so
 * none of the paths above reach it; see docs/RFC-2026-07-brightness-control.md.
 * Percentages here are already percentages - unlike DDC/CI, which speaks the
 * monitor's own scale. */
BOOL hg_backlight_available(void);
BOOL hg_backlight_get(int *out_percent);
BOOL hg_backlight_set(int percent);
void hg_backlight_shutdown(void); /* release the cached WMI connection before CoUninitialize */

/* An ACPI thermal zone in degrees Celsius, from the same root\WMI connection.
 * FALSE on the many machines whose firmware exposes no zone. It tracks CPU
 * temperature without being it; see docs/RFC-2026-07-temperature.md. */
BOOL hg_thermal_zone_celsius(int *out_celsius);

/* Every thermal zone the machine will admit to, from both surfaces, for the
 * command box's `list sensors`. Firmware declares zones it never updates, so
 * seeing the whole list is the only way to tell a dead one from a live one. */
#define HG_THERMAL_MAX_ZONES 16
typedef struct HgThermalZone {
    WCHAR name[64];
    int celsius;
    BOOL from_counter; /* the performance counter rather than the WMI class */
} HgThermalZone;
int hg_thermal_enumerate(HgThermalZone *out, int max);
/* TRUE when the display hangs off an internal connector (eDP, LVDS, or the
 * INTERNAL technology), which is what makes the WMI path the right one. */
BOOL hg_monitor_is_internal(const WCHAR *gdi_name);

/* Menu label for a display: its number, the monitor name the driver reports
 * from EDID, and the connector it hangs off. */
void hg_describe_monitor(const WCHAR *gdi_name, WCHAR *out, size_t out_cch);
/* The number in that label, taken from the GDI device name, so the menu and the
 * command box call the same display by the same number. */
int hg_monitor_display_number(const WCHAR *gdi_name);

/* Window sizes offered in the task context menu and by the command box, in one
 * table so the two cannot drift apart. The menu's resize command ids are
 * consecutive, so the id offset is the index here. */
typedef struct HgResizePreset {
    const WCHAR *name;
    int cx;
    int cy;
} HgResizePreset;
#define HG_RESIZE_PRESET_COUNT 11
extern const HgResizePreset hg_resize_presets[HG_RESIZE_PRESET_COUNT];
/* Start with Windows: one value under the per-user Run key, holding the quoted
 * path of the running executable. */
BOOL hg_startup_is_enabled(void);
BOOL hg_startup_set_enabled(BOOL enabled);

void refresh_theme_surfaces(HWND hwnd);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
void update_monitor_enum(void);
HRESULT hellgates_wsprintf(LPWSTR dest, size_t dest_size, LPCWSTR format, ...);
void normalize_path_for_api(const WCHAR *input, WCHAR *output, size_t output_size);
void init_paths(void);
BOOL is_alt_tab_window(HWND hwnd);

/* Task icons carry a label in their top-left corner - 0-9 then A-Z - and
 * Shift with that character activates the icon. Digits first because the first
 * ten windows are the ones reached most often and a digit is one keystroke to
 * find; 36 labels in all, after which icons simply have none. */
#define HG_TASK_BADGE_COUNT 36
WCHAR hg_task_badge_char(int index);
int hg_task_badge_index(WCHAR ch);
void get_process_name_by_hwnd(HWND hwnd, WCHAR *out_name, size_t out_size, DWORD *out_pid);
void get_process_path_by_hwnd(HWND hwnd, WCHAR *out_path, size_t out_size, DWORD *out_pid);
HICON get_window_icon(HWND hwnd, int size_px, BOOL *own_icon);
void release_window_item_icon(WindowItem *item);
void release_shortcut_item_icon(ShortcutItem *item);
void release_font_handle(HFONT *font, BOOL preserve_stock);
void release_brush_handle(HBRUSH *brush);
HBRUSH hg_cached_solid_brush(COLORREF color);
void hg_flush_solid_brush_cache(void);
void release_bstr(BSTR *value);
#define HG_RELEASE_COM(interface_ptr)         \
    do {                                      \
        if ((interface_ptr) != NULL) {        \
            (interface_ptr)->lpVtbl->Release(interface_ptr); \
            (interface_ptr) = NULL;           \
        }                                     \
    } while (0)
#define HG_HEAP_FREE(heap_ptr)                                \
    do {                                                      \
        if ((heap_ptr) != NULL) {                             \
            HeapFree(GetProcessHeap(), 0, (heap_ptr));        \
            (heap_ptr) = NULL;                                \
        }                                                     \
    } while (0)
#define HG_COTASKMEM_FREE(task_ptr)       \
    do {                                  \
        if ((task_ptr) != NULL) {         \
            CoTaskMemFree(task_ptr);      \
            (task_ptr) = NULL;            \
        }                                 \
    } while (0)
typedef struct HgPaintBuffer {
    HDC dc;
    HBITMAP bitmap;
    HBITMAP old_bitmap;
} HgPaintBuffer;
BOOL hg_paint_buffer_begin(HDC target_dc, int width, int height, HgPaintBuffer *buffer);
void hg_paint_buffer_end(HgPaintBuffer *buffer);
int compare_shortcuts(const void *a, const void *b);
void load_shortcuts(void);
void load_shortcuts_if_changed(void);
void append_message(const WCHAR *msg);
void hg_update_status_clock(void);
void draw_outlined_text(HDC hdc, const WCHAR *text, int len, RECT *rc, UINT format, COLORREF text_color, COLORREF outline_color);
int hg_measure_edit_height(HWND edit_wnd, HFONT font, double scale);
LRESULT hg_on_ctlcolor_edit(HDC hdc);
typedef struct HgDocumentColors {
    COLORREF bg;
    COLORREF text;
} HgDocumentColors;
HgDocumentColors hg_document_colors(void);
HgDocumentColors hg_document_field_colors(void);
LRESULT hg_on_ctlcolor_document(HDC hdc);
LRESULT hg_on_ctlcolor_field(HDC hdc);
void hg_document_paint_background(HWND hwnd, HDC hdc);
void hg_apply_dwm_attributes_document(HWND hwnd);
BOOL hg_step_alpha_value(BYTE *alpha, int delta);
void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect);
typedef enum HgToolbarTextMode {
    HG_TOOLBAR_TEXT_FOCUS = 0,
    HG_TOOLBAR_TEXT_TOOLTIP = 1
} HgToolbarTextMode;
typedef enum HgToolbarClickRole {
    HG_TOOLBAR_CLICK_NONE = 0,
    HG_TOOLBAR_CLICK_EXIT_APP,
    HG_TOOLBAR_CLICK_TOGGLE_DESKTOP,
    HG_TOOLBAR_CLICK_OPEN_MENU,
    HG_TOOLBAR_CLICK_SHOW_COMMANDBOX,
    HG_TOOLBAR_CLICK_TOGGLE_MUTE,
    HG_TOOLBAR_CLICK_FLOATER_ADJUST,
    HG_TOOLBAR_CLICK_RELOCATE_AWAY,
    HG_TOOLBAR_CLICK_TOGGLE_PIN,
    HG_TOOLBAR_CLICK_SHOW_NOTES,
    HG_TOOLBAR_CLICK_SHOW_CLIPBOARD
} HgToolbarClickRole;
typedef enum HgToolbarDragRole {
    HG_TOOLBAR_DRAG_NONE = 0,
    HG_TOOLBAR_DRAG_RESIZE_TASKBOX,
    HG_TOOLBAR_DRAG_MOVE_TASKBOX
} HgToolbarDragRole;
WCHAR hg_toolbar_builtin_label(int index);
const WCHAR *hg_toolbar_builtin_focus_text(int index);
const WCHAR *hg_toolbar_builtin_tooltip_text(int index);
BOOL hg_toolbar_builtin_has_value(int index);
BOOL hg_toolbar_builtin_value_text(int index, HgToolbarTextMode mode, WCHAR *buffer, size_t buffer_cch);
HgToolbarClickRole hg_toolbar_builtin_click_role(int index);
HgToolbarDragRole hg_toolbar_builtin_drag_role(int index);
void update_toolbar_tooltips(HWND hwnd);
BOOL CALLBACK minimize_restore_enum_proc(HWND hwnd, LPARAM l_param);
void move_window_by_offset(HWND hwnd, int dx, int dy);
void resize_window_by_offset(HWND hwnd, int dw, int dh);
BOOL should_refresh_theme_on_setting_change(LPARAM l_param);
void disable_window_ime(HWND hwnd);
BOOL readonly_edit_handle_ime_messages(HWND hwnd, UINT msg, WPARAM w_param);
BOOL hg_readonly_edit_common(HWND hwnd, UINT msg, WPARAM w_param);
BOOL hg_get_battery_percent(int *out_percent, BOOL *out_charging);
int hg_get_cpu_percent(void);
int hg_get_memory_percent(void);
/* The adapter's own temperature sensor, through the same WDDM thunk Task
 * Manager reads. FALSE when no adapter reports one, which is common.
 * See docs/RFC-2026-07-temperature.md. */
BOOL hg_get_gpu_temperature(int *out_celsius);

#endif /* HG_UTILS_H */
