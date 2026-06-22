#include "hg_common.h"

color_scheme_t hg_g_color_scheme_dark = {0};
color_scheme_t hg_g_color_scheme_light = {0};
color_scheme_t hg_g_color_scheme_selected = {0};
int hg_g_is_dark_mode = 1;
BOOL hg_g_is_high_contrast = FALSE;
COLORREF hg_g_system_accent_color = 0;
BOOL hg_g_has_system_accent_color = FALSE;

double hg_g_scale_factor = 1.0;

HWND hg_g_toolbar_wnd = NULL;
HWND hg_g_edit_msg_wnd = NULL;
HWND hg_g_tooltip_wnd = NULL;
HWND hg_g_taskbox_wnd = NULL;
HWND hg_g_commandbox_wnd = NULL;
HWND hg_g_commandbox_out_wnd = NULL;
HWND hg_g_commandbox_in_wnd = NULL;
HWND hg_g_commandbox_btn_wnd = NULL;
HFONT hg_g_commandbox_font = NULL;
int hg_g_commandbox_font_size = -16;
BYTE hg_g_commandbox_alpha = 204;
WCHAR hg_g_commandbox_font_name[LF_FACESIZE] = {0};
HMENU hg_g_h_audio_submenu = NULL;

UINT hg_g_shellhook_msg = 0;
HWND hg_g_floater_wnd = NULL;
HWND hg_g_about_wnd = NULL;
HWND hg_g_prev_active_hwnd = NULL;
HFONT hg_g_main_font = NULL;
HFONT hg_g_floater_time_font = NULL;
HFONT hg_g_floater_date_font = NULL;
HFONT hg_g_toolbar_btn_font = NULL;
HBRUSH hg_g_main_bg_brush = NULL;
HBRUSH hg_g_edit_bg_brush = NULL;
HBRUSH hg_g_hbr_highlight = NULL;
int hg_g_floater_font_size = 28;
int hg_g_current_font_size = -22;
int hg_g_edit_font_size = -16;
BYTE hg_g_floater_alpha = 204;
BYTE hg_g_taskbox_alpha = 204;
int hg_g_floater_highlight_ticks = 0;
int hg_g_taskbox_highlight_ticks = 0;
int hg_g_focus_area = 0;
int hg_g_toolbar_focus_index = 0;
UINT hg_g_hotkey_modifiers = MOD_WIN | MOD_ALT;
UINT hg_g_hotkey_key = VK_SPACE;
BOOL hg_g_hotkey_registered = FALSE;
WCHAR hg_g_pending_command_line[HG_MAX_PATH] = {0};
BOOL hg_g_has_pending_command_line = FALSE;

WCHAR hg_g_base_path[HG_MAX_PATH] = {0};
WCHAR hg_g_shortcuts_path[HG_MAX_PATH] = {0};
WCHAR hg_g_config_path[HG_MAX_PATH] = {0};
WCHAR hg_g_font_name[64] = L"Segoe UI";

ShortcutItem hg_g_shortcuts[HG_MAX_SHORTCUTS] = {0};
int hg_g_shortcut_count = 0;

AudioDevice hg_g_audio_devices[HG_MAX_AUDIO_DEVICES] = {0};
int hg_g_audio_device_count = 0;

WindowItem hg_g_window_items[HG_MAX_WINDOW_ITEMS] = {0};
WindowItem hg_g_new_items[HG_MAX_WINDOW_ITEMS] = {0};
int hg_g_window_count = 0;

BOOL hg_g_in_sizemove = FALSE;
RECT hg_g_drag_start_rect = {0};

/* Drag & Drop states */
POINT hg_g_last_mouse_pos = {-1, -1};
BOOL hg_g_hover_check_armed = TRUE;
BOOL hg_g_menu_active = FALSE;
BOOL hg_g_is_dragging = FALSE;
int hg_g_drag_source_index = -1;
POINT hg_g_drag_start_pt = {0, 0};
POINT hg_g_drag_current_pt = {0, 0};
int hg_g_drag_target_index = -1;

MonitorInfo hg_g_monitors[HG_MAX_MONITORS] = {0};
int hg_g_monitor_count = 0;
