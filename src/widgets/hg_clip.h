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

/* The command box's view. Numbers are 1-based positions in the history, newest
 * first, which is the same order the window shows - unlike the notes, the
 * clipboard list has only one order, so there is nothing to reconcile. */
#define HG_CLIP_ROW_CCH_PUBLIC 160
int hg_clip_count(void);
int hg_clip_max(void);
void hg_clip_set_max(int value);
/* One row of display text for entry `number`; FALSE when there is no such entry. */
BOOL hg_clip_row(int number, WCHAR *out, size_t out_cch);
/* Makes entry `number` the current clipboard, pushing the ones above it down. */
BOOL hg_clip_take(int number);

#endif /* HG_CLIP_H */
