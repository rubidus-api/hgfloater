/* The floater's clock and date formats - see hg_timefmt.h for the notation.
 *
 * Two rules hold everywhere in this file. The format is never trusted: it is
 * validated in full before the first character is written, and the renderer
 * validates it again rather than assuming its caller did. And the output is
 * never indexed without a bound: every character goes through timefmt_put,
 * which knows how much room is left. */
#include "hg_timefmt.h"

static const wchar_t *const timefmt_days[7] = {L"Sunday",   L"Monday", L"Tuesday", L"Wednesday",
                                               L"Thursday", L"Friday", L"Saturday"};
static const wchar_t *const timefmt_months[12] = {L"January", L"February", L"March",     L"April",
                                                  L"May",     L"June",     L"July",      L"August",
                                                  L"September", L"October", L"November", L"December"};
/* The longest name in each table: Wednesday, September. */
#define HG_TIMEFMT_LONGEST_NAME 9

/* What a code is, and the most characters it can write. 0 for a character that
 * is not a code. `numeric` says whether the minus flag applies. */
static int timefmt_code_width(wchar_t code, int *numeric)
{
    int is_numeric = 1;
    int width = 0;

    switch (code) {
    case L'H':
    case L'I':
    case L'M':
    case L'S':
    case L'y':
    case L'm':
    case L'd':
    case L'e':
        width = 2;
        break;
    case L'j':
        width = 3;
        break;
    case L'Y':
        width = 4;
        break;
    case L'p':
        width = 2;
        is_numeric = 0;
        break;
    case L'a':
    case L'b':
        width = 3;
        is_numeric = 0;
        break;
    case L'A':
    case L'B':
        width = HG_TIMEFMT_LONGEST_NAME;
        is_numeric = 0;
        break;
    case L'%':
        width = 1;
        is_numeric = 0;
        break;
    default:
        break;
    }
    if (numeric)
        *numeric = is_numeric;
    return width;
}

HgTimefmtError hg_timefmt_validate(const wchar_t *format, int *err_pos)
{
    if (err_pos)
        *err_pos = 0;
    if (!format || format[0] == L'\0')
        return HG_TIMEFMT_EMPTY;

    /* The length first, and bounded: the text may be anything. */
    int length = 0;
    while (length <= HG_TIMEFMT_MAX_FORMAT && format[length] != L'\0')
        length++;
    if (length > HG_TIMEFMT_MAX_FORMAT) {
        if (err_pos)
            *err_pos = HG_TIMEFMT_MAX_FORMAT;
        return HG_TIMEFMT_TOO_LONG;
    }

    int longest = 0; /* the most characters this format can render */
    int visible = 0; /* whether any of them is something other than a space */

    for (int i = 0; i < length; ++i) {
        wchar_t ch = format[i];

        if (ch < 0x20 || (ch >= 0x7F && ch <= 0x9F) || ch == 0x2028 || ch == 0x2029) {
            if (err_pos)
                *err_pos = i;
            return HG_TIMEFMT_CONTROL;
        }
        if (ch != L'%') {
            longest++;
            if (ch != L' ')
                visible = 1;
            continue;
        }

        int at = i; /* the % itself, which is where the eye should go */
        int minus = 0;
        if (i + 1 < length && format[i + 1] == L'-') {
            minus = 1;
            i++;
        }
        if (i + 1 >= length) {
            if (err_pos)
                *err_pos = at;
            return HG_TIMEFMT_DANGLING;
        }
        i++;

        int numeric = 0;
        int width = timefmt_code_width(format[i], &numeric);
        if (width == 0 || (minus && !numeric)) {
            if (err_pos)
                *err_pos = at;
            return HG_TIMEFMT_UNKNOWN;
        }
        longest += width;
        visible = 1;
    }

    if (!visible)
        return HG_TIMEFMT_BLANK;
    if (longest > HG_TIMEFMT_OUT_CCH - 1)
        return HG_TIMEFMT_OUTPUT;
    return HG_TIMEFMT_OK;
}

/* The one place a character reaches the output. Returns 0 once there is no
 * room for it and the terminator that has to follow. */
static int timefmt_put(wchar_t *out, int cch, int *pos, wchar_t ch)
{
    if (*pos < 0 || *pos >= cch - 1)
        return 0;
    out[*pos] = ch;
    (*pos)++;
    return 1;
}

static int timefmt_put_text(wchar_t *out, int cch, int *pos, const wchar_t *text, int limit)
{
    for (int i = 0; text[i] != L'\0' && (limit < 0 || i < limit); ++i) {
        if (!timefmt_put(out, cch, pos, text[i]))
            return 0;
    }
    return 1;
}

/* A non-negative number in at least `width` characters, padded on the left with
 * `pad`; no padding at all when `pad` is 0. */
static int timefmt_put_number(wchar_t *out, int cch, int *pos, int value, int width, wchar_t pad)
{
    wchar_t digits[12];
    int count = 0;

    if (value < 0)
        value = 0;
    do {
        digits[count++] = (wchar_t)(L'0' + value % 10);
        value /= 10;
    } while (value > 0 && count < (int)(sizeof(digits) / sizeof(digits[0])));

    if (pad != 0) {
        for (int i = count; i < width; ++i) {
            if (!timefmt_put(out, cch, pos, pad))
                return 0;
        }
    }
    while (count > 0) {
        if (!timefmt_put(out, cch, pos, digits[--count]))
            return 0;
    }
    return 1;
}

static int timefmt_clamp(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static int timefmt_day_of_year(int year, int month, int day)
{
    static const int before[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    int result = before[month - 1] + day;

    if (leap && month > 2)
        result++;
    return timefmt_clamp(result, 1, 366);
}

int hg_timefmt_format(wchar_t *out, int cch, const wchar_t *format, const HgTimeParts *parts, HgTimeSpans *spans)
{
    if (spans) {
        spans->hour_start = spans->hour_end = -1;
        spans->minute_start = spans->minute_end = -1;
    }
    if (!out || cch <= 0)
        return -1;
    out[0] = L'\0';
    if (!parts || hg_timefmt_validate(format, NULL) != HG_TIMEFMT_OK)
        return -1;

    /* Every field is brought into range before it is used as a number or as an
     * index into a table of names. */
    int year = timefmt_clamp(parts->year, 0, 9999);
    int month = timefmt_clamp(parts->month, 1, 12);
    int day = timefmt_clamp(parts->day, 1, 31);
    int wday = timefmt_clamp(parts->wday, 0, 6);
    int hour = timefmt_clamp(parts->hour, 0, 23);
    int minute = timefmt_clamp(parts->minute, 0, 59);
    int second = timefmt_clamp(parts->second, 0, 59);
    int hour12 = hour % 12;
    if (hour12 == 0)
        hour12 = 12;

    HgTimeSpans found = {-1, -1, -1, -1};
    int pos = 0;
    int ok = 1;

    /* Validated above, so the format is short, terminated, and every % is
     * followed by a code on the list; the bounds below are kept anyway. */
    for (int i = 0; ok && i < HG_TIMEFMT_MAX_FORMAT && format[i] != L'\0'; ++i) {
        if (format[i] != L'%') {
            ok = timefmt_put(out, cch, &pos, format[i]);
            continue;
        }

        wchar_t zero = L'0';
        wchar_t space = L' ';
        i++;
        if (format[i] == L'-') {
            zero = 0;
            space = 0;
            i++;
        }

        int start = pos;
        switch (format[i]) {
        case L'H':
            ok = timefmt_put_number(out, cch, &pos, hour, 2, zero);
            break;
        case L'I':
            ok = timefmt_put_number(out, cch, &pos, hour12, 2, zero);
            break;
        case L'M':
            ok = timefmt_put_number(out, cch, &pos, minute, 2, zero);
            break;
        case L'S':
            ok = timefmt_put_number(out, cch, &pos, second, 2, zero);
            break;
        case L'p':
            ok = timefmt_put_text(out, cch, &pos, (hour < 12) ? L"AM" : L"PM", -1);
            break;
        case L'Y':
            ok = timefmt_put_number(out, cch, &pos, year, 4, zero);
            break;
        case L'y':
            ok = timefmt_put_number(out, cch, &pos, year % 100, 2, zero);
            break;
        case L'm':
            ok = timefmt_put_number(out, cch, &pos, month, 2, zero);
            break;
        case L'd':
            ok = timefmt_put_number(out, cch, &pos, day, 2, zero);
            break;
        case L'e':
            ok = timefmt_put_number(out, cch, &pos, day, 2, space);
            break;
        case L'j':
            ok = timefmt_put_number(out, cch, &pos, timefmt_day_of_year(year, month, day), 3, zero);
            break;
        case L'a':
            ok = timefmt_put_text(out, cch, &pos, timefmt_days[wday], 3);
            break;
        case L'A':
            ok = timefmt_put_text(out, cch, &pos, timefmt_days[wday], -1);
            break;
        case L'b':
            ok = timefmt_put_text(out, cch, &pos, timefmt_months[month - 1], 3);
            break;
        case L'B':
            ok = timefmt_put_text(out, cch, &pos, timefmt_months[month - 1], -1);
            break;
        case L'%':
            ok = timefmt_put(out, cch, &pos, L'%');
            break;
        default:
            ok = 0; /* not reachable past validation */
            break;
        }

        if (ok && (format[i] == L'H' || format[i] == L'I') && found.hour_start < 0) {
            found.hour_start = start;
            found.hour_end = pos;
        }
        if (ok && format[i] == L'M' && found.minute_start < 0) {
            found.minute_start = start;
            found.minute_end = pos;
        }
    }

    if (!ok) {
        out[0] = L'\0';
        return -1;
    }
    out[pos] = L'\0';
    if (spans)
        *spans = found;
    return pos;
}

int hg_timefmt_escape(wchar_t *out, int cch, const wchar_t *format)
{
    static const wchar_t hex[] = L"0123456789ABCDEF";

    if (!out || cch <= 0)
        return -1;
    out[0] = L'\0';
    if (!format)
        return -1;

    int pos = 0;
    int ok = 1;
    for (int i = 0; ok && format[i] != L'\0'; ++i) {
        wchar_t ch = format[i];
        long code = (long)ch; /* wider than wchar_t wherever that is 16 bits */
        if (ch == L'\\') {
            ok = timefmt_put(out, cch, &pos, L'\\') && timefmt_put(out, cch, &pos, L'\\');
        } else if (ch >= 0x20 && ch <= 0x7E) {
            ok = timefmt_put(out, cch, &pos, ch);
        } else if (code < 0 || code > 0xFFFFL) {
            ok = 0; /* not a UTF-16 unit: only where wchar_t is wider than that */
        } else {
            unsigned value = (unsigned)code;
            ok = timefmt_put(out, cch, &pos, L'\\') && timefmt_put(out, cch, &pos, L'u') &&
                 timefmt_put(out, cch, &pos, hex[(value >> 12) & 0xFu]) &&
                 timefmt_put(out, cch, &pos, hex[(value >> 8) & 0xFu]) &&
                 timefmt_put(out, cch, &pos, hex[(value >> 4) & 0xFu]) &&
                 timefmt_put(out, cch, &pos, hex[value & 0xFu]);
        }
    }

    if (!ok) {
        out[0] = L'\0';
        return -1;
    }
    out[pos] = L'\0';
    return pos;
}

static int timefmt_hex_digit(wchar_t ch)
{
    if (ch >= L'0' && ch <= L'9')
        return (int)(ch - L'0');
    if (ch >= L'A' && ch <= L'F')
        return (int)(ch - L'A') + 10;
    if (ch >= L'a' && ch <= L'f')
        return (int)(ch - L'a') + 10;
    return -1;
}

int hg_timefmt_unescape(wchar_t *out, int cch, const wchar_t *stored)
{
    if (!out || cch <= 0)
        return -1;
    out[0] = L'\0';
    if (!stored)
        return -1;

    int pos = 0;
    int ok = 1;
    for (int i = 0; ok && stored[i] != L'\0'; ++i) {
        wchar_t ch = stored[i];
        if (ch != L'\\') {
            ok = timefmt_put(out, cch, &pos, ch);
            continue;
        }

        /* Each look ahead stops at the terminator: a digit that is missing is
         * not a hex digit, so the scan never steps past the end. */
        i++;
        if (stored[i] == L'\\') {
            ok = timefmt_put(out, cch, &pos, L'\\');
        } else if (stored[i] == L'u') {
            unsigned value = 0;
            for (int k = 0; ok && k < 4; ++k) {
                int digit = timefmt_hex_digit(stored[i + 1]);
                if (digit < 0) {
                    ok = 0;
                } else {
                    value = (value << 4) | (unsigned)digit;
                    i++;
                }
            }
            if (ok)
                ok = (value != 0) && timefmt_put(out, cch, &pos, (wchar_t)value);
        } else {
            ok = 0;
        }
    }

    if (!ok) {
        out[0] = L'\0';
        return -1;
    }
    out[pos] = L'\0';
    return pos;
}

const wchar_t *hg_timefmt_error_text(HgTimefmtError error)
{
    switch (error) {
    case HG_TIMEFMT_OK:
        return L"ok";
    case HG_TIMEFMT_EMPTY:
        return L"the format is empty";
    case HG_TIMEFMT_TOO_LONG:
        return L"the format is longer than 32 characters";
    case HG_TIMEFMT_DANGLING:
        return L"a % with no code after it (write %% for a percent sign)";
    case HG_TIMEFMT_UNKNOWN:
        return L"not a code this clock knows";
    case HG_TIMEFMT_CONTROL:
        return L"a control character (a tab or a line break) cannot be shown";
    case HG_TIMEFMT_BLANK:
        return L"nothing but spaces - there would be nothing to see";
    case HG_TIMEFMT_OUTPUT:
        return L"it could render longer than 63 characters";
    default:
        return L"not a valid format";
    }
}
