#ifndef HG_CLOCK_H
#define HG_CLOCK_H

#include "hg_common.h"
#include "hg_timefmt.h"

/* What the floater's clock shows and how it answers the pointer: the two
 * formats, the half-second colour change, and which part of the clock opens the
 * taskbox on hover. The floater draws; this owns the settings, checks every
 * value on its way in - from the settings window and from the file alike - and
 * writes what it accepted. */

enum {
    HG_CLOCK_FORMAT_TIME = 0, /* the large line */
    HG_CLOCK_FORMAT_DATE = 1, /* the line under it */
    HG_CLOCK_FORMAT_COUNT
};

/* How the taskbox opens from the floater. A click opens it in every mode; the
 * three hover modes add the pointer resting on that part of the clock. */
typedef enum HgHoverMode {
    HG_HOVER_CLICK_ONLY = 0,
    HG_HOVER_MINUTES,
    HG_HOVER_HOURS,
    HG_HOVER_BOTH,
    HG_HOVER_MODE_COUNT
} HgHoverMode;

/* Reads [floater] time_format, date_format, blink and [taskbox] hover_region.
 * A value that is missing, too long, or not a valid format is replaced by its
 * default, in memory and in the file. */
void hg_clock_load_config(void);
/* Everything back to its default, written. Part of Reset All. */
void hg_clock_reset_all(void);

const WCHAR *hg_clock_format(int which);
const WCHAR *hg_clock_default_format(int which);
const WCHAR *hg_clock_format_label(int which);

/* The result of asking for a new format. */
#define HG_CLOCK_SET_OK 0
#define HG_CLOCK_SET_NOT_KEPT (-1) /* valid, but the settings file did not hand it back unchanged */
/* Validates `format`, and only then applies and saves it. Returns
 * HG_CLOCK_SET_OK, HG_CLOCK_SET_NOT_KEPT, or the HgTimefmtError (positive) with
 * the offending offset in err_pos. Anything but OK leaves the format as it was. */
int hg_clock_set_format(int which, const WCHAR *format, int *err_pos);

/* The line as it reads at `st`. Always leaves a terminated string in `out`: if
 * the stored format somehow fails, the default is rendered in its place. */
void hg_clock_render(int which, const SYSTEMTIME *st, WCHAR *out, int cch, HgTimeSpans *spans);
/* The same for a format that is not the stored one - the settings window's
 * preview. FALSE, and an empty string, when it is not valid. */
BOOL hg_clock_preview(const WCHAR *format, WCHAR *out, int cch);

HgHoverMode hg_clock_hover_mode(void);
void hg_clock_set_hover_mode(HgHoverMode mode);
const WCHAR *hg_clock_hover_mode_text(HgHoverMode mode);

BOOL hg_clock_blink(void);
void hg_clock_set_blink(BOOL on);
COLORREF hg_clock_blink_color(void);
void hg_clock_set_blink_color(COLORREF color);
void hg_clock_reset_blink_color(void);

#endif /* HG_CLOCK_H */
