#ifndef HG_TASKBOX_H
#define HG_TASKBOX_H

#include "../hg_common.h"

/* Taskbox Switcher Widget Interface */
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass, DWORD_PTR dw_ref_data);
void refresh_window_list(BOOL force);
void update_layout(HWND hwnd);
void update_size(int delta);
void update_edit_font_size(int delta);
void update_taskbox_alpha(int delta);
void set_taskbox_opacity_pct(int pct);
void hide_taskbox(HWND hwnd);
void activate_toolbar_item(int index);
void activate_taskbar_item(int index);
void update_focus_message(int override_type, int override_index);
int get_item_at_pt(POINT pt, int width, int height, int icon_size, int *out_type, int *out_index);
BOOL get_explorer_path(HWND target_hwnd, WCHAR *out_path, int max_len);

#endif /* HG_TASKBOX_H */
