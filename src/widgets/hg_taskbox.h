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
/* Keyboard mode: the arrows are driving, so the pointer is not.
 *
 * The taskbox answers the mouse continuously - a pointer resting on an icon
 * opens that icon's list, a pointer that has wandered off closes the box and
 * eventually collapses the whole taskbox. All of that is right when the mouse
 * is what the reader is using, and all of it is wrong the moment they reach for
 * the arrow keys: the pointer is then wherever it was left, pointing at
 * whatever happens to be under it, and it would keep closing the list the
 * keyboard just opened.
 *
 * So a key press says "the keyboard is driving" and the pointer is ignored -
 * not moved, not warped, just not listened to - until the mouse actually moves
 * again, which is the reader saying the opposite. Movement means movement: a
 * WM_MOUSEMOVE arriving because a window slid under a still pointer does not
 * count, which is why this compares positions rather than counting messages. */
void hg_keyboard_mode_begin(void);
/* Called from the hover tick: leaves keyboard mode if the pointer has moved
 * since it began. */
void hg_keyboard_mode_check_pointer(void);

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

/* Two kinds of row, and they are the same two the Set list has: something to
 * run, and something to turn.
 *
 * Scale and brightness used to be a rung each - six rows to offer one display
 * six scaling percentages, five more for its backlight, and a reader with three
 * monitors got thirty-three rows of ladder. They are one row each now, holding
 * the reading, and the wheel or the arrows walk the ladder behind it. The rungs
 * are still exactly the values Windows and the monitor will accept: this moves
 * along the same list, it does not invent values between them. */
enum {
    HG_MENU_ROW_COMMAND = 0,
    HG_MENU_ROW_VALUE
};

typedef struct HgMenuRow {
    WCHAR label[HG_MENU_ROW_MAX]; /* the whole path, joined */
    UINT id;                      /* the WM_COMMAND it stands for; 0 = nothing to run */
    BOOL enabled;
    BOOL checked;

    int kind;
    /* Value rows only: the ladder this row steps along. The command for rung n
     * is base_id + n, which is the same command the row used to be, so what
     * applies the change is unchanged and there is one place that knows how. */
    UINT base_id;
    const int *steps; /* the percentages, in order */
    int step_count;
    int step_index; /* where it is now; -1 when the display would not say */
    int step_min;   /* the rungs this display actually allows */
    int step_max;
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
