#ifndef HG_TABBOX_H
#define HG_TABBOX_H

#include "../hg_common.h"

/* The tab sub-box: a window's tabs, as a list, on hover.
 *
 * Tabs used to fan out into the taskbox grid, one icon each. That crowded the
 * grid, made the icons unorderable (their order was the strip's, not the
 * reader's), and cost a background enumeration cadence to keep titles fresh.
 * See docs/RFC-2026-07-tabs-as-task-icons.md, D8.
 *
 * Now a tab-capable window is one icon like any other, and hovering it opens
 * this box: one row per tab, titled and labelled, beside the icon. Enumeration
 * happens when the box opens and never otherwise - nobody looking means
 * nothing asked. */

/* The same box now serves three lists, because they are the same thing: a
 * short list that belongs to the button under the pointer, placed clear of it,
 * dismissed by leaving. Only what fills the rows and what activating one does
 * differ - the window, the placement, the keys and the painting are one copy.
 *
 *   tabs      a window's tabs, on a task icon
 *   folders   the shortcuts that point at a directory, on the Dir button
 *   controls  volume, brightness, opacity and the switches, on Set
 *   menu      the options menu, flattened to one level, on Opt
 */
enum {
    HG_BOX_TABS = 0,
    HG_BOX_DIRS,
    HG_BOX_CONTROLS,
    HG_BOX_MENU
};

/* Open (or re-target) the box for a window, anchored to that icon's rect in
 * screen coordinates. Safe to call repeatedly with the same window. */
void hg_tabbox_open(HWND target, const RECT *anchor_screen_rc);

/* The folder list, the control list and the options menu, anchored to their
 * toolbar button. */
void hg_tabbox_open_dirs(const RECT *anchor_screen_rc);
void hg_tabbox_open_controls(const RECT *anchor_screen_rc);
void hg_tabbox_open_menu(const RECT *anchor_screen_rc);

/* Which list is up, or HG_BOX_TABS when nothing is. */
int hg_tabbox_mode(void);

/* A wheel notch while the pointer is over the box. Only the control list
 * answers: it is the one whose rows are values. TRUE when it was used. */
BOOL hg_tabbox_handle_wheel(short delta);

/* TRUE while the pointer is inside the open box. */
BOOL hg_tabbox_pointer_over(void);

/* Give the box the keyboard: the rows answer the arrows, Enter picks one, and
 * the selected row is drawn as selected. Called when a button was activated by
 * a key rather than pointed at, because "I pressed Space on it" is a statement
 * about where the keyboard should be. */
void hg_tabbox_enter(void);

/* Close it if it is up. */
void hg_tabbox_close(void);

BOOL hg_tabbox_is_open(void);

/* The window the open box belongs to, or NULL. */
HWND hg_tabbox_target(void);

/* The box's own window, for hit tests by the taskbox's hover timer. */
HWND hg_tabbox_window(void);

/* Fold in a worker answer that has just arrived; called on HG_MSG_TABS_READY.
 * Does nothing when the box is closed. */
void hg_tabbox_refresh(void);

/* A key pressed while the box is up. TRUE when the box consumed it: arrows and
 * Enter and Escape, and the label keys - the box has no focus of its own, so
 * the toolbar hands its keys here first. */
BOOL hg_tabbox_handle_key(WPARAM key);

/* The pointer moved somewhere in the taskbox or the toolbar: the box closes
 * when the pointer is neither on it nor on the icon it belongs to. */
void hg_tabbox_pointer_moved(void);

LRESULT CALLBACK tabbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_TABBOX_H */
