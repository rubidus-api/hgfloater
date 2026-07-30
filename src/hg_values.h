#ifndef HG_VALUES_H
#define HG_VALUES_H

#include "hg_common.h"

/* The tunable numbers, in one list, so `show value` and `write value` do not
 * each have to know where every setting lives.
 *
 * Everything here is an int in the unit a person would say out loud - percent
 * for brightness and opacity, points for a font, a count for a maximum - rather
 * than the representation the subsystem happens to use internally. A command
 * that made you type 191 to mean 75% opacity would not be worth having. */

typedef struct HgValueInfo {
    const WCHAR *name; /* one word, no spaces: what `write value` accepts */
    const WCHAR *unit; /* "%", "pt", "" */
    int min;
    int max;
    const WCHAR *about; /* one line, for the listing */
} HgValueInfo;

int hg_value_count(void);
BOOL hg_value_info(int number, HgValueInfo *out);
BOOL hg_value_get(int number, int *out);

/* Applies the value. out_persisted says whether it also reached a config file:
 * screen brightness and volume are the machine's state rather than ours, so
 * they take effect and are not saved anywhere. */
BOOL hg_value_set(int number, int value, BOOL *out_persisted);

/* The number for a name, or 0. Case-insensitive; a leading unique prefix is
 * enough, so `w v bright 60` works. */
int hg_value_find(const WCHAR *name);

#endif /* HG_VALUES_H */
