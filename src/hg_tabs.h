#ifndef HG_TABS_H
#define HG_TABS_H

#include "hg_common.h"

/* Browser and Explorer tabs as task icons. Design, and the costs it is arranged
 * around, are in docs/RFC-2026-07-tabs-as-task-icons.md.
 *
 * A tab is not a window, so no amount of window enumeration finds one. The only
 * supported way to ask is UI Automation, which is a cross-process call into the
 * application's own UI thread - slow, and able to block if that thread is. So
 * this is off by default, runs only for windows whose class can have tabs, and
 * runs on its own slower clock rather than with the window list.
 *
 * Off means not one call is made and the COM object is never created. */

#define HG_TABS_MAX_PER_WINDOW 24

BOOL hg_tabs_enabled(void);
void hg_tabs_set_enabled(BOOL enabled);

/* TRUE for a window class that is known to have tabs. Cheap - a string compare,
 * no cross-process call - so it can gate everything else. */
BOOL hg_tabs_window_may_have_tabs(HWND hwnd);

/* Enumeration runs on a worker thread, because a UIA walk is a cross-process
 * call whose cost belongs to the target application - Chrome's accessibility
 * tree grows for as long as the browser runs, and a walk that took nothing at
 * startup can take a hundred milliseconds an hour later. The UI thread only
 * files a request and reads whatever answer has arrived; it never waits.
 *
 * hg_tabs_request queues a batch of windows and returns at once. When the
 * worker finishes, it posts HG_MSG_TABS_READY to the floater window; results
 * sit in a table until hg_tabs_take_result collects them, so a taskbox that is
 * hidden when the answer lands simply picks it up on its next pass. */
#define HG_TABS_WORKER_WINDOWS 16
#define HG_MSG_TABS_READY (WM_APP + 41)

/* Queue windows for enumeration. Requests MERGE into the pending set rather
 * than replacing it, so a window queued a moment ago is not silently dropped
 * by a newer, smaller batch; a full set counts the overflow instead. */
void hg_tabs_request(const HWND *hwnds, int count);

/* What an answer says beyond the titles. failed means the ask itself broke
 * (no provider, a dead walk) - the caller should keep what it was showing,
 * because "the question failed" and "there are no tabs" are different facts.
 * provider and elapsed_ms exist for the `show tabs` diagnostics and for the
 * caller's own backoff arithmetic. */
typedef struct HgTabsAnswer {
    BOOL failed;
    DWORD pid;        /* of the window the worker actually asked */
    DWORD elapsed_ms; /* what the whole ask cost, self-measured */
    WCHAR provider;   /* L'U' = UIA (MSAA was tried and retired; see the RFC) */
} HgTabsAnswer;

/* The worker's answer for this window, if a fresh one is waiting: copies the
 * titles and returns the tab count (possibly 0). -1 when nothing new has
 * arrived, in which case the caller keeps what it has. answer may be NULL. */
int hg_tabs_take_result(HWND hwnd, WCHAR titles[][HG_MAX_STR], int max, HgTabsAnswer *answer);

/* One line per known window plus a totals line, through the caller's printer -
 * the command box's `show tabs`. Numbers over prose: this is the view the
 * delivery plan's exit criteria are checked against. */
void hg_tabs_report(void (*emit)(const WCHAR *line));

/* The window-list pass found more eligible windows than the tables hold;
 * counted so the cap fails loudly in `show tabs` instead of silently. */
void hg_tabs_note_overflow(void);

/* Raise the window and switch it to that tab. The element is re-found now
 * rather than held from the enumeration: a pointer into another process goes
 * stale when the page navigates or the strip rebuilds. */
BOOL hg_tabs_activate(HWND hwnd, int tab_index);

/* Close one tab by invoking its own close button. FALSE when there is no button
 * to invoke - and the caller must then do nothing, because the alternative on
 * hand is closing the window, which throws away every other tab. */
BOOL hg_tabs_close(HWND hwnd, int tab_index);

void hg_tabs_shutdown(void);

#endif /* HG_TABS_H */
