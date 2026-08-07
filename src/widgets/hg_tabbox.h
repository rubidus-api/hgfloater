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

/* Open (or re-target) the box for a window, anchored to that icon's rect in
 * screen coordinates. Safe to call repeatedly with the same window. */
void hg_tabbox_open(HWND target, const RECT *anchor_screen_rc);

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
