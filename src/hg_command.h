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

#endif /* HG_COMMAND_H */
