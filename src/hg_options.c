/* The on/off settings, in one list.
 *
 * Each row knows how to read itself, how to apply itself, and where it is
 * written. The Options submenu, `show option`, `write option` and the settings
 * window are all loops over this table rather than a branch per setting. */
#include "hg_options.h"
#include "hg_globals.h"
#include "hg_utils.h"
#include "hg_tabs.h"
#include "hg_caphook.h"
#include "widgets/hg_floater.h"
#include "widgets/hg_hilite.h"
#include "widgets/hg_tabbox.h"

/* The ini key stays [floater] show_stats: the label and the command-line name
 * are what a reader sees, and renaming the file key would only lose the setting
 * of anyone who already had it. */
enum {
    HG_OPTION_HOVER_OPEN = 1,
    HG_OPTION_WINDOW_OUTLINE,
    HG_OPTION_TAB_BOX,
    HG_OPTION_TABS,
    HG_OPTION_CAPTION_MENU,
    HG_OPTION_STARTUP,
    HG_OPTION_STAT_BARS
};

#if HG_TEMP_DISABLE_FLAGGED_FEATURES
#define HG_OPTION_FLAGGED_AVAILABLE FALSE
#define HG_OPTION_FLAGGED_NOTE L"off in this build"
#else
#define HG_OPTION_FLAGGED_AVAILABLE TRUE
#define HG_OPTION_FLAGGED_NOTE NULL
#endif

static const HgOptionInfo hg_options[] = {
    {L"hover-open", L"Open the Taskbox on Hover",
     L"the pointer resting on the floater opens the taskbox, as it did before v0.13.0", TRUE, NULL},
    {L"window-outline", L"Outline the Window Under the Pointer",
     L"pointing at a task icon draws a frame around the window it stands for", TRUE, NULL},
    {L"tab-box", L"Show a Window's Tabs on Hover",
     L"pointing at a tabbed window's icon opens the list of its tabs beside it", TRUE, NULL},
    {L"tabs", L"Show Tabs as Task Icons", L"a tabbed window's tabs each get their own icon", TRUE, NULL},
    {L"caption-menu", L"Menu on Maximize Button", L"right-click any window's maximize button for a menu",
     HG_OPTION_FLAGGED_AVAILABLE, HG_OPTION_FLAGGED_NOTE},
    {L"startup", L"Start with Windows", L"one value under the per-user Run key", HG_OPTION_FLAGGED_AVAILABLE,
     HG_OPTION_FLAGGED_NOTE},
    {L"statbars", L"Stat Bars on the Floater", L"battery, CPU, memory and temperature, behind the clock",
     TRUE, NULL},
};

int hg_option_count(void)
{
    return (int)HG_ARRAYSIZE(hg_options);
}

BOOL hg_option_info(int number, HgOptionInfo *out)
{
    if (!out || number < 1 || number > hg_option_count())
        return FALSE;
    *out = hg_options[number - 1];
    return TRUE;
}

BOOL hg_option_get(int number)
{
    switch (number) {
    case HG_OPTION_HOVER_OPEN:
        return hg_g_taskbox_open_on_hover;
    case HG_OPTION_WINDOW_OUTLINE:
        return hg_g_window_outline;
    case HG_OPTION_TAB_BOX:
        return hg_g_tabbox_on_hover;
    case HG_OPTION_TABS:
        return hg_tabs_enabled();
    case HG_OPTION_CAPTION_MENU:
        return hg_caphook_enabled();
    case HG_OPTION_STARTUP:
        return hg_startup_is_enabled();
    case HG_OPTION_STAT_BARS:
        return hg_g_floater_show_stats;
    default:
        return FALSE;
    }
}

BOOL hg_option_set(int number, BOOL value, const WCHAR **out_message)
{
    HgOptionInfo info;
    if (!hg_option_info(number, &info))
        return FALSE;

    value = value ? TRUE : FALSE;

    if (!info.available) {
        if (out_message)
            *out_message = L"That option is off in this build.";
        return FALSE;
    }

    switch (number) {
    case HG_OPTION_HOVER_OPEN:
        hg_g_taskbox_open_on_hover = value;
        WritePrivateProfileStringW(L"taskbox", L"open_on_hover", value ? L"1" : L"0", hg_g_config_path);
        if (out_message)
            *out_message = value ? L"Taskbox: opens when the pointer rests on the floater"
                                 : L"Taskbox: opens on a click, not on hover";
        break;
    case HG_OPTION_WINDOW_OUTLINE:
        hg_g_window_outline = value;
        WritePrivateProfileStringW(L"taskbox", L"window_outline", value ? L"1" : L"0", hg_g_config_path);
        if (!value)
            hg_hilite_hide(); /* one already on screen would otherwise stay */
        if (out_message)
            *out_message = value ? L"Task icons: outline the window they stand for"
                                 : L"Task icons: no outline";
        break;
    case HG_OPTION_TAB_BOX:
        hg_g_tabbox_on_hover = value;
        WritePrivateProfileStringW(L"taskbox", L"tab_box", value ? L"1" : L"0", hg_g_config_path);
        if (!value)
            hg_tabbox_close();
        if (out_message)
            *out_message = value ? L"Tabs: listed beside the icon on hover" : L"Tabs: no hover list";
        break;
    case HG_OPTION_TABS:
        hg_tabs_set_enabled(value);
        /* Forced, so the list is rebuilt from scratch rather than waiting for
         * the tab pass to come round on its own clock. */
        refresh_window_list(TRUE);
        if (out_message)
            *out_message = value ? L"Tabs: shown as their own icons" : L"Tabs: off, one icon per window";
        break;
    case HG_OPTION_CAPTION_MENU:
        hg_caphook_set_enabled(value);
        if (out_message)
            *out_message = value ? L"Maximize button: right-click it on any window"
                                 : L"Maximize button: left as Windows has it";
        break;
    case HG_OPTION_STARTUP:
        if (!hg_startup_set_enabled(value)) {
            if (out_message)
                *out_message = L"Start with Windows: the registry refused the change";
            return FALSE;
        }
        if (out_message)
            *out_message = value ? L"Start with Windows: on" : L"Start with Windows: off";
        break;
    case HG_OPTION_STAT_BARS:
        hg_g_floater_show_stats = value;
        WritePrivateProfileStringW(L"floater", L"show_stats", value ? L"1" : L"0", hg_g_config_path);
        if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
            /* The bars are part of the layout, not just paint: dropping them
             * changes how wide the floater wants to be. */
            update_floater_layout(hg_g_floater_wnd);
            InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        }
        if (out_message)
            *out_message = value ? L"Floater: stat bars shown" : L"Floater: stat bars hidden";
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

/* A unique leading prefix is enough. Ambiguity returns 0 rather than guessing:
 * picking one of two settings the reader might have meant is worse than making
 * them type another letter. */
int hg_option_find(const WCHAR *name)
{
    if (!name || !*name)
        return 0;

    size_t len = wcslen(name);
    int match = 0;
    for (int i = 0; i < hg_option_count(); ++i) {
        if (_wcsnicmp(hg_options[i].name, name, len) != 0)
            continue;
        if (wcslen(hg_options[i].name) == len)
            return i + 1; /* exact beats prefix */
        if (match)
            return 0;
        match = i + 1;
    }
    return match;
}

BOOL hg_option_parse_value(int number, const WCHAR *text, BOOL *out)
{
    if (!text || !*text || !out)
        return FALSE;

    if (_wcsicmp(text, L"on") == 0 || _wcsicmp(text, L"1") == 0 || _wcsicmp(text, L"yes") == 0 ||
        _wcsicmp(text, L"true") == 0) {
        *out = TRUE;
        return TRUE;
    }
    if (_wcsicmp(text, L"off") == 0 || _wcsicmp(text, L"0") == 0 || _wcsicmp(text, L"no") == 0 ||
        _wcsicmp(text, L"false") == 0) {
        *out = FALSE;
        return TRUE;
    }
    if (_wcsicmp(text, L"toggle") == 0 || _wcsicmp(text, L"t") == 0) {
        *out = hg_option_get(number) ? FALSE : TRUE;
        return TRUE;
    }
    return FALSE;
}

void hg_options_load(void)
{
    hg_g_taskbox_open_on_hover =
        (GetPrivateProfileIntW(L"taskbox", L"open_on_hover", 0, hg_g_config_path) != 0);
    hg_g_window_outline = (GetPrivateProfileIntW(L"taskbox", L"window_outline", 1, hg_g_config_path) != 0);
    hg_g_tabbox_on_hover = (GetPrivateProfileIntW(L"taskbox", L"tab_box", 1, hg_g_config_path) != 0);
}
