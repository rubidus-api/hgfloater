/* The floater clock's settings - see hg_clock.h.
 *
 * A format is text somebody typed, or text somebody wrote into the settings
 * file by hand, and it ends up deciding what is drawn every second. So there is
 * one way in: hg_clock_set_format for a new value, clock_read_format for the
 * file, and both go through hg_timefmt_validate before the value is kept. It is
 * never passed to a printf-style function as the format argument, here or
 * anywhere else. */
#include "hg_clock.h"
#include "hg_globals.h"
#include "hg_utils.h"
#include "hg_config.h"
#include "hg_options.h"
#include "widgets/hg_floater.h"

static const WCHAR *const clock_default_formats[HG_CLOCK_FORMAT_COUNT] = {L"%H:%M", L"%a, %b %-d"};
static const WCHAR *const clock_format_keys[HG_CLOCK_FORMAT_COUNT] = {L"time_format", L"date_format"};
static const WCHAR *const clock_format_labels[HG_CLOCK_FORMAT_COUNT] = {L"Clock format", L"Date format"};

static WCHAR s_formats[HG_CLOCK_FORMAT_COUNT][HG_TIMEFMT_MAX_FORMAT + 1] = {L"%H:%M", L"%a, %b %-d"};
static HgHoverMode s_hover_region = HG_HOVER_MINUTES; /* which part, when hover is on at all */
static BOOL s_blink = FALSE;

static BOOL clock_valid_which(int which)
{
    return which >= 0 && which < HG_CLOCK_FORMAT_COUNT;
}

const WCHAR *hg_clock_format(int which)
{
    return clock_valid_which(which) ? s_formats[which] : L"";
}

const WCHAR *hg_clock_default_format(int which)
{
    return clock_valid_which(which) ? clock_default_formats[which] : L"";
}

const WCHAR *hg_clock_format_label(int which)
{
    return clock_valid_which(which) ? clock_format_labels[which] : L"";
}

/* The format as the file holds it, or FALSE when the key is missing, was cut
 * short by the read, carries a bad escape, or does not validate. */
static BOOL clock_read_format(int which, WCHAR *out, size_t out_cch)
{
    /* Room for the longest stored form and then some, so a value that fills the
     * buffer is known to be longer than anything this program wrote. */
    WCHAR stored[HG_TIMEFMT_STORED_CCH + 8];
    DWORD got = GetPrivateProfileStringW(L"floater", clock_format_keys[which], L"", stored,
                                         (DWORD)HG_ARRAYSIZE(stored), hg_g_config_path);
    if (got == 0 || got >= HG_ARRAYSIZE(stored) - 1)
        return FALSE;

    WCHAR format[HG_TIMEFMT_MAX_FORMAT + 1];
    if (hg_timefmt_unescape(format, (int)HG_ARRAYSIZE(format), stored) < 0)
        return FALSE;
    if (hg_timefmt_validate(format, NULL) != HG_TIMEFMT_OK)
        return FALSE;
    return SUCCEEDED(StringCchCopyW(out, out_cch, format));
}

static BOOL clock_write_format(int which, const WCHAR *format)
{
    WCHAR stored[HG_TIMEFMT_STORED_CCH];
    if (hg_timefmt_escape(stored, (int)HG_ARRAYSIZE(stored), format) < 0)
        return FALSE;

    /* Quoted: the profile API hands back what is between the quotes, spaces at
     * either end included, where an unquoted value would come back trimmed. */
    WCHAR quoted[HG_TIMEFMT_STORED_CCH + 2];
    if (FAILED(StringCchPrintfW(quoted, HG_ARRAYSIZE(quoted), L"\"%ls\"", stored)))
        return FALSE;
    return WritePrivateProfileStringW(L"floater", clock_format_keys[which], quoted, hg_g_config_path) != 0;
}

static void clock_changed(void)
{
    hg_floater_clock_changed();
}

int hg_clock_set_format(int which, const WCHAR *format, int *err_pos)
{
    if (err_pos)
        *err_pos = 0;
    if (!clock_valid_which(which))
        return HG_TIMEFMT_EMPTY;

    HgTimefmtError error = hg_timefmt_validate(format, err_pos);
    if (error != HG_TIMEFMT_OK)
        return (int)error;

    /* Written first and read back, so what the running program shows is what
     * the next start will show. If the file does not return the same text, the
     * old value goes back in and nothing changes. */
    WCHAR kept[HG_TIMEFMT_MAX_FORMAT + 1];
    if (!clock_write_format(which, format) || !clock_read_format(which, kept, HG_ARRAYSIZE(kept)) ||
        wcscmp(kept, format) != 0) {
        clock_write_format(which, s_formats[which]);
        return HG_CLOCK_SET_NOT_KEPT;
    }

    StringCchCopyW(s_formats[which], HG_ARRAYSIZE(s_formats[which]), format);
    clock_changed();
    return HG_CLOCK_SET_OK;
}

static void clock_parts(const SYSTEMTIME *st, HgTimeParts *parts)
{
    parts->year = st->wYear;
    parts->month = st->wMonth;
    parts->day = st->wDay;
    parts->wday = st->wDayOfWeek;
    parts->hour = st->wHour;
    parts->minute = st->wMinute;
    parts->second = st->wSecond;
}

void hg_clock_render(int which, const SYSTEMTIME *st, WCHAR *out, int cch, HgTimeSpans *spans)
{
    if (spans) {
        spans->hour_start = spans->hour_end = -1;
        spans->minute_start = spans->minute_end = -1;
    }
    if (!out || cch <= 0)
        return;
    out[0] = L'\0';
    if (!st || !clock_valid_which(which))
        return;

    HgTimeParts parts;
    clock_parts(st, &parts);
    if (hg_timefmt_format(out, cch, s_formats[which], &parts, spans) > 0)
        return;
    /* The stored format was validated on its way in, so this is not expected;
     * a clock that shows the default is still a clock. */
    hg_timefmt_format(out, cch, clock_default_formats[which], &parts, spans);
}

BOOL hg_clock_preview(const WCHAR *format, WCHAR *out, int cch)
{
    if (!out || cch <= 0)
        return FALSE;
    out[0] = L'\0';

    SYSTEMTIME st;
    GetLocalTime(&st);
    HgTimeParts parts;
    clock_parts(&st, &parts);
    return hg_timefmt_format(out, cch, format, &parts, NULL) > 0;
}

/* ------------------------------------------------------------ hover */

static const WCHAR *const clock_region_names[HG_HOVER_MODE_COUNT] = {L"", L"minutes", L"hours", L"both"};

HgHoverMode hg_clock_hover_mode(void)
{
    return hg_g_taskbox_open_on_hover ? s_hover_region : HG_HOVER_CLICK_ONLY;
}

const WCHAR *hg_clock_hover_mode_text(HgHoverMode mode)
{
    switch (mode) {
    case HG_HOVER_CLICK_ONLY:
        return L"click only";
    case HG_HOVER_MINUTES:
        return L"hover on the minutes";
    case HG_HOVER_HOURS:
        return L"hover on the hours";
    case HG_HOVER_BOTH:
        return L"hover on hours or minutes";
    default:
        return L"";
    }
}

void hg_clock_set_hover_mode(HgHoverMode mode)
{
    if (mode < HG_HOVER_CLICK_ONLY || mode >= HG_HOVER_MODE_COUNT)
        return;

    /* On or off is the existing switch - the O menu and `write option
     * hoveropen` set the same one - and the region is remembered across a turn
     * through click only. */
    int option = hg_option_find(L"hoveropen");
    if (mode != HG_HOVER_CLICK_ONLY) {
        s_hover_region = mode;
        WritePrivateProfileStringW(L"taskbox", L"hover_region", clock_region_names[mode], hg_g_config_path);
    }
    if (option)
        hg_option_set(option, mode != HG_HOVER_CLICK_ONLY, NULL);
}

/* ------------------------------------------------------------ blink */

BOOL hg_clock_blink(void)
{
    return s_blink;
}

void hg_clock_set_blink(BOOL on)
{
    s_blink = on ? TRUE : FALSE;
    WritePrivateProfileStringW(L"floater", L"blink", s_blink ? L"1" : L"0", hg_g_config_path);
    clock_changed();
}

COLORREF hg_clock_blink_color(void)
{
    return hg_g_color_clock_blink;
}

void hg_clock_set_blink_color(COLORREF color)
{
    hg_g_color_clock_blink = color & 0x00FFFFFFu;
    hg_config_save_color(L"clock_blink", hg_g_color_clock_blink);
    clock_changed();
}

void hg_clock_reset_blink_color(void)
{
    hg_clock_set_blink_color(HG_COLOR_CLOCK_BLINK_DEFAULT);
}

/* ------------------------------------------------------------ file */

void hg_clock_load_config(void)
{
    for (int which = 0; which < HG_CLOCK_FORMAT_COUNT; ++which) {
        if (!clock_read_format(which, s_formats[which], HG_ARRAYSIZE(s_formats[which])))
            StringCchCopyW(s_formats[which], HG_ARRAYSIZE(s_formats[which]), clock_default_formats[which]);
        /* Written back either way, like every other key: a file that never held
         * the key gains it, and one that held something unusable is repaired. */
        clock_write_format(which, s_formats[which]);
    }

    WCHAR region[16];
    GetPrivateProfileStringW(L"taskbox", L"hover_region", L"minutes", region, (DWORD)HG_ARRAYSIZE(region),
                             hg_g_config_path);
    s_hover_region = HG_HOVER_MINUTES;
    for (int mode = HG_HOVER_MINUTES; mode < HG_HOVER_MODE_COUNT; ++mode) {
        if (_wcsicmp(region, clock_region_names[mode]) == 0)
            s_hover_region = (HgHoverMode)mode;
    }
    WritePrivateProfileStringW(L"taskbox", L"hover_region", clock_region_names[s_hover_region], hg_g_config_path);

    s_blink = (GetPrivateProfileIntW(L"floater", L"blink", 0, hg_g_config_path) != 0);
    WritePrivateProfileStringW(L"floater", L"blink", s_blink ? L"1" : L"0", hg_g_config_path);
}

void hg_clock_reset_all(void)
{
    for (int which = 0; which < HG_CLOCK_FORMAT_COUNT; ++which) {
        StringCchCopyW(s_formats[which], HG_ARRAYSIZE(s_formats[which]), clock_default_formats[which]);
        clock_write_format(which, s_formats[which]);
    }
    s_hover_region = HG_HOVER_MINUTES;
    WritePrivateProfileStringW(L"taskbox", L"hover_region", clock_region_names[s_hover_region], hg_g_config_path);
    s_blink = FALSE;
    WritePrivateProfileStringW(L"floater", L"blink", L"0", hg_g_config_path);
    clock_changed();
}
