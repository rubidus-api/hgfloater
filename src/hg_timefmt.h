#ifndef HG_TIMEFMT_H
#define HG_TIMEFMT_H

#include <stddef.h> /* wchar_t */

/* The floater's clock and date, rendered from a format a person typed.
 *
 * The formats are written the way strftime's are, because that is the notation
 * people already know, but nothing here calls strftime: the C library's version
 * reads the process locale, answers an unknown code differently on every
 * runtime, and cannot say where in its output the hour ended up. This one has a
 * closed list of codes, refuses everything else before it writes a character,
 * and reports where the hours and the minutes landed - the floater needs that to
 * know which glyphs the pointer is resting on.
 *
 * No Win32 dependency, so the host-side test compiles the unit directly (see
 * test/test_timefmt.c).
 *
 * The codes:
 *   %H  hour, 00-23          %I  hour, 01-12         %p  AM or PM
 *   %M  minute, 00-59        %S  second, 00-59
 *   %Y  year, four digits    %y  year, two digits
 *   %m  month, 01-12         %b  Jan ... Dec         %B  January ... December
 *   %d  day, 01-31           %e  day, space padded   %j  day of the year, 001-366
 *   %a  Sun ... Sat          %A  Sunday ... Saturday
 *   %%  a percent sign
 * A minus between the percent and a numeric code drops the padding: %-d is 6
 * where %d is 06. It is refused on the codes that are words. */

/* The longest format accepted, in characters, not counting the terminator. */
#define HG_TIMEFMT_MAX_FORMAT 32
/* A buffer of this many characters holds anything an accepted format can
 * render, terminator included: validation refuses a format whose longest
 * possible output would not fit, so rendering never has to truncate. */
#define HG_TIMEFMT_OUT_CCH 64

typedef enum HgTimefmtError {
    HG_TIMEFMT_OK = 0,
    HG_TIMEFMT_EMPTY,    /* nothing typed */
    HG_TIMEFMT_TOO_LONG, /* more than HG_TIMEFMT_MAX_FORMAT characters */
    HG_TIMEFMT_DANGLING, /* a % (or %-) with nothing after it */
    HG_TIMEFMT_UNKNOWN,  /* a code that is not on the list, or a minus on a word */
    HG_TIMEFMT_CONTROL,  /* a control character: a tab, a line break */
    HG_TIMEFMT_BLANK,    /* nothing but spaces: there would be nothing to see */
    HG_TIMEFMT_OUTPUT    /* could render longer than HG_TIMEFMT_OUT_CCH - 1 */
} HgTimefmtError;

typedef struct HgTimeParts {
    int year;   /* 2026 */
    int month;  /* 1-12 */
    int day;    /* 1-31 */
    int wday;   /* 0 = Sunday */
    int hour;   /* 0-23 */
    int minute; /* 0-59 */
    int second; /* 0-59 */
} HgTimeParts;

/* Where the first hour code and the first minute code were written, as
 * [start, end) character offsets into the output. Both -1 when the format has
 * no such code. */
typedef struct HgTimeSpans {
    int hour_start;
    int hour_end;
    int minute_start;
    int minute_end;
} HgTimeSpans;

/* Checks a format without rendering it. err_pos (may be NULL) receives the
 * offset of the character at fault - the % of a bad code - or 0 when the fault
 * is the format as a whole. Reads at most HG_TIMEFMT_MAX_FORMAT + 1 characters,
 * so it is safe on text that is not known to be short. */
HgTimefmtError hg_timefmt_validate(const wchar_t *format, int *err_pos);

/* Renders `format` for `parts` into `out`. Returns the number of characters
 * written, not counting the terminator, or -1 when the format is not valid or
 * the result does not fit in `cch`; `out` is then an empty string (when there
 * is room for one). Out-of-range parts are clamped rather than trusted. `spans`
 * may be NULL. */
int hg_timefmt_format(wchar_t *out, int cch, const wchar_t *format, const HgTimeParts *parts, HgTimeSpans *spans);

/* A format as it is kept in the settings file, and back.
 *
 * The file is plain text in the machine's code page unless it happens to start
 * with a byte-order mark, and the profile API trims spaces and strips a pair of
 * quotes from what it reads. A format is free text, so it is stored in a form
 * none of that can change: a backslash is written \\, and every character
 * outside printable ASCII is written \uXXXX. The caller wraps the result in
 * double quotes, which keeps leading and trailing spaces.
 *
 * Both return the number of characters written, or -1 - with `out` empty - when
 * the result does not fit or, reading back, when the text holds an escape that
 * is not one of those two. HG_TIMEFMT_STORED_CCH holds any accepted format. */
#define HG_TIMEFMT_STORED_CCH (HG_TIMEFMT_MAX_FORMAT * 6 + 1)
int hg_timefmt_escape(wchar_t *out, int cch, const wchar_t *format);
int hg_timefmt_unescape(wchar_t *out, int cch, const wchar_t *stored);

/* One line saying what is wrong, for the settings window. */
const wchar_t *hg_timefmt_error_text(HgTimefmtError error);

#endif /* HG_TIMEFMT_H */
