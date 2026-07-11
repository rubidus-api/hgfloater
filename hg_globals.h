#ifndef HG_GLOBALS_H
#define HG_GLOBALS_H

/* Global state variables declared extern */
extern color_scheme_t hg_g_custom_palette; /* configurable via [colors] */
extern COLORREF hg_g_color_focus_bg;
extern COLORREF hg_g_color_stat_cpu;
extern COLORREF hg_g_color_stat_mem;
extern COLORREF hg_g_color_stat_bat;
extern COLORREF hg_g_color_value_alpha_lo;
extern COLORREF hg_g_color_value_alpha_hi;
extern COLORREF hg_g_color_value_bright_lo;
extern COLORREF hg_g_color_value_bright_hi;
extern COLORREF hg_g_color_value_vol_lo;
extern COLORREF hg_g_color_value_vol_hi;
extern color_scheme_t hg_g_color_scheme_dark;
extern color_scheme_t hg_g_color_scheme_light;
extern color_scheme_t hg_g_color_scheme_selected;
extern int hg_g_is_dark_mode;
extern BOOL hg_g_is_high_contrast;
extern COLORREF hg_g_system_accent_color;
extern BOOL hg_g_has_system_accent_color;

extern double hg_g_scale_factor;

extern HWND hg_g_toolbar_wnd;
extern HWND hg_g_edit_msg_wnd;
extern HWND hg_g_tooltip_wnd;
extern HWND hg_g_taskbox_wnd;
extern HWND hg_g_commandbox_wnd;
extern HWND hg_g_commandbox_out_wnd;
extern HWND hg_g_commandbox_in_wnd;
extern HWND hg_g_commandbox_btn_wnd;
extern HFONT hg_g_commandbox_font;
extern int hg_g_commandbox_font_size;
extern BYTE hg_g_commandbox_alpha;
extern WCHAR hg_g_commandbox_font_name[LF_FACESIZE];
extern HMENU hg_g_h_audio_submenu;

extern UINT hg_g_shellhook_msg;
extern HWND hg_g_floater_wnd;
extern HWND hg_g_about_wnd;
extern HWND hg_g_prev_active_hwnd;
extern HFONT hg_g_main_font;
extern HFONT hg_g_floater_time_font;
extern HFONT hg_g_floater_date_font;
extern HFONT hg_g_toolbar_btn_font;
extern HBRUSH hg_g_main_bg_brush;
extern HBRUSH hg_g_edit_bg_brush;
extern HBRUSH hg_g_hbr_highlight;
extern int hg_g_floater_font_size;
extern int hg_g_current_font_size;
extern int hg_g_edit_font_size;
extern BYTE hg_g_floater_alpha;
extern BYTE hg_g_taskbox_alpha;
extern int hg_g_floater_highlight_ticks;
extern int hg_g_taskbox_highlight_ticks;
extern UINT hg_g_hotkey_modifiers;
extern UINT hg_g_hotkey_key;
extern BOOL hg_g_hotkey_registered;
extern WCHAR hg_g_pending_command_line[HG_MAX_PATH];
extern BOOL hg_g_has_pending_command_line;

extern WCHAR hg_g_base_path[HG_MAX_PATH];
extern WCHAR hg_g_shortcuts_path[HG_MAX_PATH];
extern WCHAR hg_g_config_path[HG_MAX_PATH];
extern WCHAR hg_g_font_name[64];

extern ShortcutItem hg_g_shortcuts[HG_MAX_SHORTCUTS];
extern int hg_g_shortcut_count;

extern AudioDevice hg_g_audio_devices[HG_MAX_AUDIO_DEVICES];
extern int hg_g_audio_device_count;

extern WindowItem hg_g_window_items[HG_MAX_WINDOW_ITEMS];
extern WindowItem hg_g_new_items[HG_MAX_WINDOW_ITEMS];
extern int hg_g_window_count;

extern BOOL hg_g_in_sizemove;

/* Pointer/menu interaction states */
extern POINT hg_g_last_mouse_pos;
extern BOOL hg_g_hover_check_armed;
extern BOOL hg_g_floater_show_stats; /* [floater] show_stats: battery/CPU/memory line */
extern BOOL hg_g_floater_adjust_mode; /* F button: floater shown for size/alpha tuning (no hover-expand) */
extern BOOL hg_g_menu_active;
extern POINT hg_g_drag_start_pt;

extern MonitorInfo hg_g_monitors[HG_MAX_MONITORS];
extern int hg_g_monitor_count;

#endif /* HG_GLOBALS_H */
