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
double hg_window_scale(HWND hwnd);
double hg_point_scale(POINT pt);
void refresh_theme_surfaces(HWND hwnd);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
void update_monitor_enum(void);
HRESULT hellgates_wsprintf(LPWSTR dest, size_t dest_size, LPCWSTR format, ...);
void normalize_path_for_api(const WCHAR *input, WCHAR *output, size_t output_size);
void init_paths(void);
BOOL is_alt_tab_window(HWND hwnd);
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
void draw_outlined_text(HDC hdc, const WCHAR *text, int len, RECT *rc, UINT format, COLORREF text_color, COLORREF outline_color);
int hg_measure_edit_height(HWND edit_wnd, HFONT font, double scale);
LRESULT hg_on_ctlcolor_edit(HDC hdc);
BOOL hg_step_alpha_value(BYTE *alpha, int delta);
void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect);
typedef enum HgToolbarTextMode {
    HG_TOOLBAR_TEXT_FOCUS = 0,
    HG_TOOLBAR_TEXT_TOOLTIP = 1
} HgToolbarTextMode;
typedef enum HgToolbarClickRole {
    HG_TOOLBAR_CLICK_NONE = 0,
    HG_TOOLBAR_CLICK_HIDE_TASKBOX,
    HG_TOOLBAR_CLICK_TOGGLE_DESKTOP,
    HG_TOOLBAR_CLICK_OPEN_MENU,
    HG_TOOLBAR_CLICK_SHOW_COMMANDBOX,
    HG_TOOLBAR_CLICK_TOGGLE_MUTE,
    HG_TOOLBAR_CLICK_FLOATER_ADJUST
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

#endif /* HG_UTILS_H */
