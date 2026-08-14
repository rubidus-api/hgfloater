#ifndef HG_OPTIONS_H
#define HG_OPTIONS_H

#include "hg_common.h"

/* The on/off settings, in one list - the counterpart of hg_values.c, which owns
 * the ones that are numbers.
 *
 * Before this table each toggle was written three times: a menu item with its
 * own command id, a branch in the floater's WM_COMMAND, and nothing at all on
 * the command line. Everything that can be switched on or off now lives here
 * once, and the O menu's Options submenu, `show option` and `write option` are
 * each a loop over it. Adding a toggle means adding a row. */

typedef struct HgOptionInfo {
    const WCHAR *name;  /* one word, no spaces: what `write option` accepts */
    const WCHAR *label; /* menu and settings-window text */
    const WCHAR *about; /* one line, for the listing */
    BOOL available;     /* FALSE: present but not switchable in this build */
    const WCHAR *unavailable_note; /* why, when !available */
} HgOptionInfo;

int hg_option_count(void);
BOOL hg_option_info(int number, HgOptionInfo *out);
BOOL hg_option_get(int number);

/* Applies the option and writes it to the settings file. Returns FALSE when the
 * option cannot be switched in this build, or when the change was refused (the
 * registry, for instance) - out_message then carries the line to show. */
BOOL hg_option_set(int number, BOOL value, const WCHAR **out_message);

/* The number for a name, or 0. Case-insensitive; a unique leading prefix is
 * enough, so `w o hover on` works. */
int hg_option_find(const WCHAR *name);

/* on/off/1/0/yes/no/true/false/toggle - toggle needs the current value, so this
 * takes the option number rather than being a free function. */
BOOL hg_option_parse_value(int number, const WCHAR *text, BOOL *out);

/* Reads every option from the settings file at startup. The subsystems that own
 * their own state (tabs, the caption hook, Start with Windows) read themselves
 * lazily and are not touched here. */
void hg_options_load(void);

#endif /* HG_OPTIONS_H */
