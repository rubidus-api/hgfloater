/* The tunable numbers, in one list.
 *
 * Each entry knows how to read itself, how to apply itself, and whether
 * applying it also writes a config file. Keeping that in one table is what lets
 * `show value` and `write value` stay four lines each instead of growing a
 * branch per setting, and it is also the only place that has to change when a
 * new tunable appears. */
#include "hg_values.h"
#include "hg_utils.h"
#include "hg_globals.h"
#include "hg_config.h"
#include "hg_command.h"
#include "widgets/hg_taskbox.h"
#include "widgets/hg_note.h"
#include "widgets/hg_clip.h"
#include "widgets/hg_floater.h"

/* Opacity is stored as a byte because that is what SetLayeredWindowAttributes
 * takes; it is shown as percent because that is what the wheel and the menu
 * have always called it. The two conversions live here and nowhere else. */
static int alpha_to_percent(BYTE alpha)
{
    return ((int)alpha * 100 + 127) / 255;
}

static BYTE percent_to_alpha(int percent)
{
    int alpha = (percent * 255 + 50) / 100;
    if (alpha < HG_MIN_ALPHA)
        alpha = HG_MIN_ALPHA;
    if (alpha > HG_MAX_ALPHA)
        alpha = HG_MAX_ALPHA;
    return (BYTE)alpha;
}

enum {
    HG_VALUE_BRIGHTNESS = 1,
    HG_VALUE_VOLUME,
    HG_VALUE_FLOATER_ALPHA,
    HG_VALUE_TASKBOX_ALPHA,
    HG_VALUE_COMMANDBOX_ALPHA,
    HG_VALUE_NOTE_FONT,
    HG_VALUE_CLIP_FONT,
    HG_VALUE_UI_FONT,
    HG_VALUE_ICON_SIZE,
    HG_VALUE_FLOATER_FONT,
    HG_VALUE_CLIP_MAX,
    HG_VALUE_HISTORY_MAX,
    HG_VALUE_COUNT_PLUS_ONE
};

static const HgValueInfo hg_values[] = {
    {L"brightness", L"%", 0, 100, L"screen brightness, on the display under the pointer"},
    {L"volume", L"%", 0, 100, L"system output volume"},
    {L"floater-alpha", L"%", 30, 100, L"floater opacity"},
    {L"taskbox-alpha", L"%", 30, 100, L"taskbox opacity"},
    {L"commandbox-alpha", L"%", 30, 100, L"command box opacity"},
    {L"note-font", L"pt", 8, 72, L"note text size, shared by the list and every editor"},
    {L"clip-font", L"pt", 8, 72, L"clipboard text size"},
    {L"ui-font", L"pt", 8, 72, L"interface text size - buttons, lists, the status line"},
    {L"icon-size", L"pt", 12, 96, L"how big a task icon is drawn"},
    {L"floater-font", L"pt", 8, 200, L"the floater's clock"},
    {L"clip-max", L"", 1, 64, L"how many clipboard entries to keep"},
    {L"history-max", L"", 1, 256, L"how many command lines to remember"},
};

int hg_value_count(void)
{
    return (int)HG_ARRAYSIZE(hg_values);
}

BOOL hg_value_info(int number, HgValueInfo *out)
{
    if (!out || number < 1 || number > hg_value_count())
        return FALSE;
    *out = hg_values[number - 1];
    return TRUE;
}

BOOL hg_value_get(int number, int *out)
{
    if (!out)
        return FALSE;
    switch (number) {
    case HG_VALUE_BRIGHTNESS:
        *out = get_system_brightness();
        return TRUE;
    case HG_VALUE_VOLUME:
        *out = get_system_volume();
        return TRUE;
    case HG_VALUE_FLOATER_ALPHA:
        *out = alpha_to_percent(hg_g_floater_alpha);
        return TRUE;
    case HG_VALUE_TASKBOX_ALPHA:
        *out = alpha_to_percent(hg_g_taskbox_alpha);
        return TRUE;
    case HG_VALUE_COMMANDBOX_ALPHA:
        *out = alpha_to_percent(hg_g_commandbox_alpha);
        return TRUE;
    case HG_VALUE_NOTE_FONT:
        *out = hg_note_font_size();
        return TRUE;
    case HG_VALUE_CLIP_FONT:
        *out = hg_clip_font_size();
        return TRUE;
    case HG_VALUE_UI_FONT:
        *out = hg_ui_font_point_size();
        return TRUE;
    case HG_VALUE_ICON_SIZE:
        *out = hg_taskbox_icon_point_size();
        return TRUE;
    case HG_VALUE_FLOATER_FONT:
        *out = hg_g_floater_font_size;
        return TRUE;
    case HG_VALUE_CLIP_MAX:
        *out = hg_clip_max();
        return TRUE;
    case HG_VALUE_HISTORY_MAX:
        *out = hg_command_history_max();
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL hg_value_set(int number, int value, BOOL *out_persisted)
{
    HgValueInfo info;
    if (!hg_value_info(number, &info))
        return FALSE;
    if (value < info.min)
        value = info.min;
    if (value > info.max)
        value = info.max;

    BOOL persisted = TRUE;
    switch (number) {
    case HG_VALUE_BRIGHTNESS:
        /* The machine's state, not ours: it is not in any file of ours to save
         * to, and the monitor keeps it. */
        set_system_brightness(value);
        persisted = FALSE;
        break;
    case HG_VALUE_VOLUME:
        set_system_volume(value);
        persisted = FALSE;
        break;
    case HG_VALUE_FLOATER_ALPHA:
        hg_g_floater_alpha = percent_to_alpha(value);
        if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd))
            SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
        save_alpha_config();
        break;
    case HG_VALUE_TASKBOX_ALPHA:
        set_taskbox_opacity_pct(value); /* applies and saves */
        break;
    case HG_VALUE_COMMANDBOX_ALPHA:
        hg_g_commandbox_alpha = percent_to_alpha(value);
        if (hg_g_commandbox_wnd && IsWindow(hg_g_commandbox_wnd))
            SetLayeredWindowAttributes(hg_g_commandbox_wnd, 0, hg_g_commandbox_alpha, LWA_ALPHA);
        save_commandbox_alpha_config();
        break;
    case HG_VALUE_NOTE_FONT:
        hg_note_set_font_size(value); /* note.ini */
        break;
    case HG_VALUE_CLIP_FONT:
        hg_clip_set_font_size(value); /* config.ini */
        break;
    case HG_VALUE_UI_FONT:
        hg_set_ui_font_point_size(value); /* config.ini, and every window redrawn */
        break;
    case HG_VALUE_ICON_SIZE:
        hg_set_taskbox_icon_point_size(value); /* config.ini */
        break;
    case HG_VALUE_FLOATER_FONT:
        hg_set_floater_font_size(value); /* config.ini */
        break;
    case HG_VALUE_CLIP_MAX:
        hg_clip_set_max(value); /* config.ini */
        break;
    case HG_VALUE_HISTORY_MAX:
        hg_command_set_history_max(value); /* config.ini */
        break;
    default:
        return FALSE;
    }

    if (out_persisted)
        *out_persisted = persisted;
    return TRUE;
}

/* A unique leading prefix is enough. Ambiguity returns 0 rather than guessing:
 * picking one of two settings the reader might have meant is worse than making
 * them type another letter. */
int hg_value_find(const WCHAR *name)
{
    if (!name || !*name)
        return 0;

    size_t len = wcslen(name);
    int match = 0;
    for (int i = 0; i < hg_value_count(); ++i) {
        if (_wcsnicmp(hg_values[i].name, name, len) != 0)
            continue;
        if (wcslen(hg_values[i].name) == len)
            return i + 1; /* exact beats prefix */
        if (match)
            return 0;
        match = i + 1;
    }
    return match;
}
