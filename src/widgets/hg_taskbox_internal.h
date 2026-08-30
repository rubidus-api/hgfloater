#ifndef HG_TASKBOX_INTERNAL_H
#define HG_TASKBOX_INTERNAL_H

#include "hg_taskbox.h"

/* Shared between the taskbox translation units only (window proc, toolbar
 * controller, menus, window list); not part of the public widget interface. */

typedef struct HgTaskboxDragState {
    BOOL is_dragging;
    int source_index;
    POINT start_pt;
    POINT current_pt;
    int target_index;
} HgTaskboxDragState;

typedef struct HgTaskboxFocusState {
    int area;
    int index;
} HgTaskboxFocusState;

extern HgTaskboxFocusState hg_taskbox_focus;

/* hg_taskbox.c */
int taskbox_toolbar_icon_size(void);
int taskbox_cols_from_height(int window_height, int icon_size, int border, int total_items);

/* hg_taskbox_menus.c - the options list itself (hg_menu_build_rows) and running
 * one of its rows live in hg_taskbox.h: the box is what shows them. What is
 * left here is the icon context menus, which are still real Win32 menus. */
int taskbox_track_owned_popup_menu(HMENU h_menu, UINT flags, int x, int y, HWND owner);
BOOL taskbox_handle_audio_menu_command(UINT cmd);
void toolbar_controller_show_task_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param);
void toolbar_controller_show_shortcut_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param);

#endif /* HG_TASKBOX_INTERNAL_H */
