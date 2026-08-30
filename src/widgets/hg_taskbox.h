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
/* Which of the box's lists a toolbar button carries, or -1 for a button that
 * carries none; and opening that list against the button's own rect. */
int taskbox_box_mode_for_button(int index);
void taskbox_open_box_for_button(int index, const RECT *anchor);
/* Whether merely arriving at that button - pointing at it, or landing on it
 * with the arrows - should open its list. True for the lists that are free to
 * build, false for the one that is not. */
BOOL taskbox_button_box_opens_on_arrival(int index);

/* Running what one of the options rows names. */
void taskbox_dispatch_main_menu_command(UINT cmd);

/* The options list: one flat row per thing this program can be told to do.
 *
 * There is no Win32 menu behind it. There was - the options were a menu with
 * submenus, and for one version the menu was still built, walked back into
 * rows, and thrown away every time the box opened. The box is the only reader,
 * so the rows are what gets built, and the tree that existed to be flattened is
 * gone with it.
 *
 * A submenu that only opens when you hover its parent is a thing to discover,
 * and there is nothing to discover here: four displays' worth of brightness
 * steps is a long list, not a deep one. So each row carries the path it would
 * have had - "Display 1 > Brightness > 50%" - and the reader sees all of it at
 * once. */
#define HG_MENU_ROW_MAX 160

typedef struct HgMenuRow {
    WCHAR label[HG_MENU_ROW_MAX]; /* the whole path, joined */
    UINT id;                      /* the WM_COMMAND it stands for; 0 = nothing to run */
    BOOL enabled;
    BOOL checked;
} HgMenuRow;

/* Fills rows and answers how many. Building this enumerates the audio endpoints
 * and asks every display for its scaling, so it belongs to a deliberate act -
 * see taskbox_button_box_opens_on_arrival. */
int hg_menu_build_rows(HgMenuRow *rows, int max_rows);
void activate_taskbar_item(int index);
void update_focus_message(int override_type, int override_index);
void reset_taskbox_focus(void);
int get_item_at_pt(POINT pt, int width, int height, int icon_size, int *out_type, int *out_index);

#endif /* HG_TASKBOX_H */
