#ifndef HG_CLIP_H
#define HG_CLIP_H

#include "../hg_common.h"

/* Clipboard history: what has been copied, newest first, behind the L button.
 *
 * Capture runs whether or not the window is open - a history that only records
 * while you are looking at it is not a history - so a message-only window holds
 * the clipboard listener for the life of the process.
 *
 * Nothing is written to disk except the maximum. A clipboard history on disk is
 * a file of every password and recovery code that passed through the clipboard,
 * and a floating clock widget should not be the program that owns that file.
 * The cost is stated rather than hidden: restarting empties the history. */

void hg_clip_init(void);
void hg_clip_shutdown(void);

/* The L button: opens the window, or closes it if it is already open. */
void hg_clip_toggle_window(void);

LRESULT CALLBACK clip_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_CLIP_H */
