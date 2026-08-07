#ifndef HG_COMMAND_H
#define HG_COMMAND_H

#include "hg_common.h"

/* The command box's language. One line in, printed lines out; the caller owns
 * the echo of the line itself.
 *
 * Returns TRUE when the command deliberately handed the foreground to another
 * window, so the command box knows not to take the keyboard straight back. */
BOOL hg_command_execute(const WCHAR *line);

/* Append one line to the command box transcript. */
void commandbox_print(const WCHAR *text);
void commandbox_clear(void);

/* The key reference: `help key`, and what the window prints when it opens. */
void hg_command_print_key_help(void);

/* The command history Shift+Left and Shift+Right walk. Nothing is written to
 * disk; only the cap is a setting. */
void hg_command_history_add(const WCHAR *line);
void hg_command_history_reset(void);
/* direction > 0 for older, < 0 for newer. NULL at the end; L"" past the newest. */
const WCHAR *hg_command_history_step(int direction);
/* Oldest first, so display numbers start at 1 and stay put. */
int hg_command_history_count(void);
const WCHAR *hg_command_history_at(int oldest_index);
int hg_command_history_max(void);
void hg_command_set_history_max(int value);

#endif /* HG_COMMAND_H */
