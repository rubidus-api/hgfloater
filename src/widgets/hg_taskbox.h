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

/* The options menu itself, and running what one of its rows names. Public
 * because the tab box shows this menu as a list and has to build it. */
HMENU taskbox_create_main_popup_menu(void);
void taskbox_dispatch_main_menu_command(UINT cmd);

/* A menu tree as one flat list of rows.
 *
 * The Opt button shows its menu in the same box the tabs, the folders and the
 * Set controls use, and a box is a list rather than a tree: a submenu that only
 * opens when you hover its parent is a thing to discover, and there is nothing
 * to discover here - four displays' worth of brightness steps is a long list,
 * not a deep one. So the tree is walked and every leaf becomes a row, carrying
 * the path that led to it: "Display 1 > Brightness > 50%".
 *
 * Walked from the menu itself rather than listed a second time by hand, so a
 * row added to the menu appears in the box with nothing else to edit. */
#define HG_MENU_ROW_MAX 160

typedef struct HgMenuRow {
    WCHAR label[HG_MENU_ROW_MAX]; /* the whole path, joined */
    UINT id;                      /* the WM_COMMAND it stands for; 0 = nothing to run */
    BOOL enabled;
    BOOL checked;
} HgMenuRow;

/* Fills rows and answers how many. Separators are dropped - a rule that only
 * says "these two are different" needs a gap, and this list already has one in
 * the path column. */
int hg_menu_flatten(HMENU menu, HgMenuRow *rows, int max_rows);
void activate_taskbar_item(int index);
void update_focus_message(int override_type, int override_index);
void reset_taskbox_focus(void);
int get_item_at_pt(POINT pt, int width, int height, int icon_size, int *out_type, int *out_index);

#endif /* HG_TASKBOX_H */
