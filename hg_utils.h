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
void update_audio_device_list(void);
BOOL set_default_audio_device(const WCHAR *device_id);
void init_color_scheme(void);
void update_theme_colors(void);
void apply_dwm_attributes(HWND hwnd);
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
int compare_shortcuts(const void *a, const void *b);
void load_shortcuts(void);
void append_message(const WCHAR *msg);
void draw_outlined_text(HDC hdc, const WCHAR *text, int len, RECT *rc, UINT format, COLORREF text_color, COLORREF outline_color);
int get_items_per_row(int width, int icon_size);
void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect);
WCHAR hg_toolbar_builtin_label(int index);
const WCHAR *hg_toolbar_builtin_focus_text(int index);
const WCHAR *hg_toolbar_builtin_tooltip_text(int index);
void update_toolbar_tooltips(HWND hwnd);
BOOL CALLBACK minimize_restore_enum_proc(HWND hwnd, LPARAM l_param);
void move_window_by_offset(HWND hwnd, int dx, int dy);
void resize_window_by_offset(HWND hwnd, int dw, int dh);
BOOL should_refresh_theme_on_setting_change(LPARAM l_param);
void disable_window_ime(HWND hwnd);
BOOL readonly_edit_handle_ime_messages(HWND hwnd, UINT msg, WPARAM w_param);

#endif /* HG_UTILS_H */
