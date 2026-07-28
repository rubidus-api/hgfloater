/* The command box's language.
 *
 * Every command that names a window uses the number `list` printed beside it,
 * and that number is an index into the same window list the toolbar draws. The
 * list is only re-read by the commands that print numbers: if `go` refreshed
 * first, the window list could reorder between the `list` the reader saw and
 * the number they typed, and they would switch to the wrong window. */
#include "hg_command.h"
#include "hg_utils.h"
#include "hg_globals.h"
#include "widgets/hg_note.h"

#define HG_CMD_MAX_ARGS 8

static void cmd_printf(const WCHAR *format, ...)
{
    WCHAR buffer[HG_MAX_STR];
    va_list args;
    va_start(args, format);
    HRESULT hr = StringCchVPrintfW(buffer, HG_ARRAYSIZE(buffer), format, args);
    va_end(args);
    if (SUCCEEDED(hr) || hr == STRSAFE_E_INSUFFICIENT_BUFFER)
        commandbox_print(buffer);
}

/* ------------------------------------------------------------------ parsing */

static BOOL cmd_is_space(WCHAR c)
{
    return c == L' ' || c == L'\t';
}

/* Splits into at most max_args tokens. The line is copied first, so the caller's
 * text is untouched and the tail of the line stays available for arguments that
 * are allowed to contain spaces. */
static int cmd_tokenize(const WCHAR *line, WCHAR *store, size_t store_cch, WCHAR *argv[], int max_args)
{
    if (FAILED(StringCchCopyW(store, store_cch, line)))
        return 0;

    int argc = 0;
    WCHAR *p = store;
    while (*p && argc < max_args) {
        while (*p && cmd_is_space(*p))
            ++p;
        if (!*p)
            break;
        argv[argc++] = p;
        while (*p && !cmd_is_space(*p))
            ++p;
        if (*p)
            *p++ = L'\0';
    }
    return argc;
}

/* Everything after the first `count` tokens, leading blanks removed, so a needle
 * may contain spaces. */
static const WCHAR *cmd_tail(const WCHAR *line, int count)
{
    const WCHAR *p = line;
    for (int i = 0; i < count; ++i) {
        while (*p && cmd_is_space(*p))
            ++p;
        while (*p && !cmd_is_space(*p))
            ++p;
    }
    while (*p && cmd_is_space(*p))
        ++p;
    return p;
}

static BOOL cmd_parse_int(const WCHAR *text, int *out)
{
    if (!text || !*text)
        return FALSE;

    int sign = 1;
    const WCHAR *p = text;
    if (*p == L'-') {
        sign = -1;
        ++p;
    } else if (*p == L'+') {
        ++p;
    }
    if (!*p)
        return FALSE;

    int value = 0;
    for (; *p; ++p) {
        if (*p < L'0' || *p > L'9')
            return FALSE;
        value = value * 10 + (int)(*p - L'0');
    }
    *out = value * sign;
    return TRUE;
}

static BOOL cmd_word_is(const WCHAR *word, const WCHAR *full, const WCHAR *shorthand)
{
    return (full && lstrcmpiW(word, full) == 0) || (shorthand && lstrcmpiW(word, shorthand) == 0);
}

/* Case-insensitive substring search: the reader types what they remember of a
 * title, not its exact case. */
static BOOL cmd_title_contains(const WCHAR *title, const WCHAR *needle)
{
    return StrStrIW(title, needle) != NULL;
}

/* ----------------------------------------------------------------- lookups */

/* Printing numbers is the moment to make sure they are current. Icons are left
 * alone (that is what the force flag rebuilds); only the membership and order
 * of the list matter here. The toolbar is redrawn so it agrees with what was
 * just printed. */
static void cmd_refresh_windows(void)
{
    refresh_window_list(FALSE);
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd))
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
}

static HWND cmd_window_by_number(int number)
{
    if (number < 1 || number > hg_g_window_count)
        return NULL;
    HWND target = hg_g_window_items[number - 1].hwnd;
    return IsWindow(target) ? target : NULL;
}

/* Monitors answer to the number their menu entry shows, which is the number
 * Windows itself puts on the display; the array position is the fallback for
 * the odd setup where the device name carries no number. */
static const MonitorInfo *cmd_monitor_by_number(int number)
{
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_monitor_display_number(hg_g_monitors[i].name) == number)
            return &hg_g_monitors[i];
    }
    if (number >= 1 && number <= hg_g_monitor_count)
        return &hg_g_monitors[number - 1];
    return NULL;
}

/* ---------------------------------------------------------------- commands */

static void cmd_help(void)
{
    static const WCHAR *lines[] = {
        L"help, h                     this list",
        L"list, l                      list windows, with the number each command uses",
        L"list windows, l w            the same list",
        L"list resize, l r             the resize presets",
        L"list shortcut, l s           the shortcut icons",
        L"go N                         focus window N",
        L"resize N P, r N P            resize window N to preset P",
        L"move N X Y, m N X Y          move window N to X, Y on the display it is on",
        L"move N X Y M                 move window N to X, Y on display M",
        L"search windows T, s w T      windows whose title contains T",
        L"note                         open the note list",
    };
    for (size_t i = 0; i < HG_ARRAYSIZE(lines); ++i)
        commandbox_print(lines[i]);
}

static void cmd_list_windows(void)
{
    cmd_refresh_windows();

    if (hg_g_window_count <= 0) {
        commandbox_print(L"no windows");
        return;
    }
    for (int i = 0; i < hg_g_window_count; ++i) {
        cmd_printf(L"%3d  %ls", i + 1, hg_g_window_items[i].title);
    }
}

static void cmd_list_resize(void)
{
    for (int i = 0; i < HG_RESIZE_PRESET_COUNT; ++i) {
        cmd_printf(L"%3d  %ls", i + 1, hg_resize_presets[i].name);
    }
}

static void cmd_list_shortcuts(void)
{
    if (hg_g_shortcut_count <= 0) {
        commandbox_print(L"no shortcuts");
        return;
    }
    for (int i = 0; i < hg_g_shortcut_count; ++i) {
        cmd_printf(L"%3d  %ls", i + 1, hg_g_shortcuts[i].name);
    }
}

static void cmd_list(int argc, WCHAR *argv[])
{
    if (argc < 2) {
        cmd_list_windows();
        return;
    }
    if (cmd_word_is(argv[1], L"windows", L"w")) {
        cmd_list_windows();
    } else if (cmd_word_is(argv[1], L"resize", L"r")) {
        cmd_list_resize();
    } else if (cmd_word_is(argv[1], L"shortcut", L"s") || cmd_word_is(argv[1], L"shortcuts", NULL)) {
        cmd_list_shortcuts();
    } else {
        cmd_printf(L"list: unknown kind '%ls' (windows, resize, shortcut)", argv[1]);
    }
}

static void cmd_go(int argc, WCHAR *argv[])
{
    int number = 0;
    if (argc < 2 || !cmd_parse_int(argv[1], &number)) {
        commandbox_print(L"go: needs a window number, as in 'go 1'");
        return;
    }

    HWND target = cmd_window_by_number(number);
    if (!target) {
        cmd_printf(L"go: no window %d", number);
        return;
    }

    if (IsIconic(target))
        ShowWindow(target, SW_RESTORE);
    SetForegroundWindow(target);
    cmd_printf(L"go %d: %ls", number, hg_g_window_items[number - 1].title);
}

static void cmd_resize(int argc, WCHAR *argv[])
{
    int number = 0, preset = 0;
    if (argc < 3 || !cmd_parse_int(argv[1], &number) || !cmd_parse_int(argv[2], &preset)) {
        commandbox_print(L"resize: needs a window and a preset, as in 'resize 1 1'");
        return;
    }

    HWND target = cmd_window_by_number(number);
    if (!target) {
        cmd_printf(L"resize: no window %d", number);
        return;
    }
    if (preset < 1 || preset > HG_RESIZE_PRESET_COUNT) {
        cmd_printf(L"resize: no preset %d (see 'list resize')", preset);
        return;
    }

    const HgResizePreset *spec = &hg_resize_presets[preset - 1];
    SetWindowPos(target, NULL, 0, 0, spec->cx, spec->cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    cmd_printf(L"resize %d: %ls", number, spec->name);
}

static void cmd_move(int argc, WCHAR *argv[])
{
    int number = 0, x = 0, y = 0;
    if (argc < 4 || !cmd_parse_int(argv[1], &number) || !cmd_parse_int(argv[2], &x) || !cmd_parse_int(argv[3], &y)) {
        commandbox_print(L"move: needs a window and a position, as in 'move 1 100 100'");
        return;
    }

    HWND target = cmd_window_by_number(number);
    if (!target) {
        cmd_printf(L"move: no window %d", number);
        return;
    }

    /* X and Y are read against a display's own top-left corner, not the virtual
     * desktop's, so the same numbers mean the same place on every screen. */
    RECT origin = {0, 0, 0, 0};
    int display_number = 0;
    if (argc >= 5) {
        int wanted = 0;
        if (!cmd_parse_int(argv[4], &wanted)) {
            cmd_printf(L"move: '%ls' is not a display number", argv[4]);
            return;
        }
        const MonitorInfo *monitor = cmd_monitor_by_number(wanted);
        if (!monitor) {
            cmd_printf(L"move: no display %d", wanted);
            return;
        }
        origin = monitor->rcMonitor;
        display_number = wanted;
    } else {
        MONITORINFO mi;
        SecureZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(MonitorFromWindow(target, MONITOR_DEFAULTTONEAREST), &mi))
            origin = mi.rcMonitor;
    }

    SetWindowPos(target, NULL, origin.left + x, origin.top + y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (display_number > 0) {
        cmd_printf(L"move %d: %d, %d on display %d", number, x, y, display_number);
    } else {
        cmd_printf(L"move %d: %d, %d", number, x, y);
    }
}

static void cmd_search(int argc, WCHAR *argv[], const WCHAR *line)
{
    if (argc < 3 || !cmd_word_is(argv[1], L"windows", L"w")) {
        commandbox_print(L"search: only windows so far, as in 'search windows notepad'");
        return;
    }

    const WCHAR *needle = cmd_tail(line, 2);
    if (!needle || !*needle) {
        commandbox_print(L"search: needs something to look for");
        return;
    }

    cmd_refresh_windows();

    int found = 0;
    for (int i = 0; i < hg_g_window_count; ++i) {
        if (!cmd_title_contains(hg_g_window_items[i].title, needle))
            continue;
        /* The number printed is the window's place in the full list, not the
         * position among the matches, so it can be handed straight to `go`. */
        cmd_printf(L"%3d  %ls", i + 1, hg_g_window_items[i].title);
        ++found;
    }
    if (found == 0)
        cmd_printf(L"no window matches '%ls'", needle);
}

void hg_command_execute(const WCHAR *line)
{
    if (!line)
        return;
    while (*line && cmd_is_space(*line))
        ++line;
    if (!*line)
        return;

    WCHAR store[HG_MAX_STR];
    WCHAR *argv[HG_CMD_MAX_ARGS];
    int argc = cmd_tokenize(line, store, HG_ARRAYSIZE(store), argv, HG_CMD_MAX_ARGS);
    if (argc <= 0)
        return;

    if (cmd_word_is(argv[0], L"help", L"h")) {
        cmd_help();
    } else if (cmd_word_is(argv[0], L"list", L"l")) {
        cmd_list(argc, argv);
    } else if (cmd_word_is(argv[0], L"go", NULL)) {
        cmd_go(argc, argv);
    } else if (cmd_word_is(argv[0], L"resize", L"r")) {
        cmd_resize(argc, argv);
    } else if (cmd_word_is(argv[0], L"move", L"m")) {
        cmd_move(argc, argv);
    } else if (cmd_word_is(argv[0], L"search", L"s")) {
        cmd_search(argc, argv, line);
    } else if (cmd_word_is(argv[0], L"note", L"n")) {
        show_note_list_window();
        commandbox_print(L"note: opened the note list");
    } else {
        cmd_printf(L"unknown command '%ls' - type help", argv[0]);
    }
}
