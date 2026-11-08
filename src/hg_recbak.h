#ifndef HG_RECBAK_H
#define HG_RECBAK_H

#include <windows.h>

/* Recorder backup (R1.8): a USB voice recorder that keeps its recordings in
 * \RECORDED on its own drive (the Sanno kind) puts a
 * BakREC button on the taskbox grid while it is plugged in. The button asks
 * for a folder and moves the recordings there on a worker thread; while that
 * runs the button shows the percent done.
 *
 * Detection costs nothing while nothing changes: the drive letters are looked
 * at when Windows announces a volume arriving or leaving (WM_DEVICECHANGE,
 * which every top-level window receives unasked), and once a minute on the
 * floater's clock only GetLogicalDrives is compared - the drives are examined
 * only when that mask is different from the last one examined. */

/* Messages the worker posts to the floater. */
#define HG_MSG_RECBAK_PROGRESS (WM_APP + 60)
#define HG_MSG_RECBAK_DONE (WM_APP + 61)

/* Nonzero while a recorder is plugged in, or a move is still running. */
BOOL hg_recbak_present(void);
/* Nonzero while the move runs. */
BOOL hg_recbak_busy(void);
/* The button's word while the move runs ("42%"), else NULL. */
const WCHAR *hg_recbak_busy_label(void);

/* `force`: look at the drives now (at start, and before the folder box).
 * Otherwise this is the floater's once-a-second clock tick: it looks on the
 * tick after a WM_DEVICECHANGE volume message, and every sixtieth tick only if
 * GetLogicalDrives differs from the mask last looked at. A change of presence
 * re-lays the taskbox grid. */
void hg_recbak_poll(BOOL force);
/* WM_DEVICECHANGE on the floater. */
void hg_recbak_on_device_change(WPARAM w_param, LPARAM l_param);

/* The button: asks for the folder and starts the move; while one runs, offers
 * to stop it. */
void hg_recbak_click(HWND owner);

/* The floater's handlers for the worker's two messages. */
void hg_recbak_on_progress(void);
void hg_recbak_on_done(HWND owner);

/* Quitting: TRUE to go on. While a move runs it asks first, and on yes stops
 * the move and waits for the worker to finish the file it is on. */
BOOL hg_recbak_confirm_quit(HWND owner);

#endif /* HG_RECBAK_H */
