#ifndef HG_SETTINGS_H
#define HG_SETTINGS_H

#include "../hg_common.h"

/* The settings window: one place that shows what can be changed and changes it.
 *
 * It owns no state of its own. Every row is a view of a table that already
 * exists - hg_options.c for the switches, hg_values.c for the numbers,
 * hg_keys.c for window/function/keys - so what it writes is what the menu, the
 * command box and the program itself are reading. Nothing here waits for an OK
 * button: a change lands in the settings file and in the running program at the
 * moment it is made, which is the same promise every other control in this
 * program keeps. */

void hg_settings_toggle_window(void);
LRESULT CALLBACK settings_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_SETTINGS_H */
