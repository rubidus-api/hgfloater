#ifndef HG_CAPHOOK_H
#define HG_CAPHOOK_H

#include "hg_common.h"

/* A menu on every window's maximize button.
 *
 * Right-click the maximize button of any window and hgfloater offers the same
 * move-and-resize entries the task icons offer. Design and costs are in
 * docs/RFC-2026-07-caption-button-menu.md.
 *
 * This is the only thing hgfloater does outside its own windows, so it is the
 * only thing that needs a system-wide hook. The hook goes in when the setting
 * is on and comes out when it is off or when the program exits - there is no
 * behaviour left behind for hgfloater not to be running. */

BOOL hg_caphook_enabled(void);
void hg_caphook_set_enabled(BOOL enabled);

/* Installs or removes the hook to match the setting. Safe to call repeatedly. */
void hg_caphook_apply(void);
void hg_caphook_shutdown(void);

/* Posted to the taskbox when a right-click landed on a maximize button;
 * w_param carries the target window. */
#define HG_MSG_CAPTION_MENU (WM_APP + 40)
void hg_caphook_show_menu(HWND owner, HWND target);

#endif /* HG_CAPHOOK_H */
