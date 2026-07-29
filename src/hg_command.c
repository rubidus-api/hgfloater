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
/* Returns the token count, or -1 when the line does not fit the store: the
 * caller has to tell the reader, because a command that vanishes without a word
 * reads as one that ran and found nothing. */
static int cmd_tokenize(const WCHAR *line, WCHAR *store, size_t store_cch, WCHAR *argv[], int max_args)
{
    if (FAILED(StringCchCopyW(store, store_cch, line)))
        return -1;

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
 * Windows itself puts on the display. The array position is a fallback only for
 * the odd setup where no device name carries a number at all: Windows leaves
 * gaps after a monitor is unplugged, and answering 'display 2' with whichever
 * monitor happens to sit second in the array would move a window to a display
 * the reader did not name. */
static const MonitorInfo *cmd_monitor_by_number(int number)
{
    BOOL any_numbered = FALSE;
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        int labelled = hg_monitor_display_number(hg_g_monitors[i].name);
        if (labelled > 0)
            any_numbered = TRUE;
        if (labelled == number)
            return &hg_g_monitors[i];
    }
    if (!any_numbered && number >= 1 && number <= hg_g_monitor_count)
        return &hg_g_monitors[number - 1];
    return NULL;
}

/* ---------------------------------------------------------------- commands */

/* One entry per command: the summary the overview prints, and the lines
 * `help <command>` prints. Keeping both here means the overview cannot list a
 * command the detailed help has never heard of. */
typedef struct HgCommandHelp {
    const WCHAR *name;
    const WCHAR *shorthand;
    const WCHAR *summary;
    const WCHAR *const *detail;
    size_t detail_count;
} HgCommandHelp;

static const WCHAR *const cmd_help_help[] = {
    L"help [command]      (h)",
    L"",
    L"  With no command, lists them all. With one, explains that one",
    L"  and shows what it looks like in use.",
    L"",
    L"Examples:",
    L"  help                everything, in one line each",
    L"  h move              just move, in detail",
};

static const WCHAR *const cmd_help_list[] = {
    L"list [kind]         (l)",
    L"",
    L"  Prints a numbered list. Without a kind, lists windows.",
    L"  The number beside a window is the one go, resize, and move take,",
    L"  so this is where those commands get their arguments.",
    L"  Kinds: windows (w), resize (r), shortcut (s), note (n), sensors (t).",
    L"",
    L"Examples:",
    L"  list                every window, numbered",
    L"  l w                 the same list",
    L"  l r                 the resize presets, numbered for 'resize'",
    L"  l s                 the shortcut icons",
    L"  l n                 every note, numbered for the 'note' command",
    L"  l t                 every temperature sensor, and which one is shown",
    L"",
    L"  'list sensors' is a diagnostic. The floater shows one CPU",
    L"  temperature, chosen from whatever the firmware exposes; this",
    L"  prints all of them so a reading that looks wrong can be checked",
    L"  against its neighbours. A zone that never moves is firmware",
    L"  filler, and is why the chosen one may not be the hottest.",
};

static const WCHAR *const cmd_help_go[] = {
    L"go <window>",
    L"",
    L"  Brings a window to the front, restoring it first if it is",
    L"  minimised. <window> is the number 'list' printed beside it.",
    L"",
    L"Examples:",
    L"  list                read the numbers",
    L"  go 3                switch to the third window",
};

static const WCHAR *const cmd_help_resize[] = {
    L"resize <window> <preset>          (r)",
    L"",
    L"  Resizes a window to one of the fixed sizes. 'list resize'",
    L"  numbers them; the same set the task context menu offers.",
    L"  The window keeps its position; only its size changes.",
    L"",
    L"Examples:",
    L"  l r                 see which preset is which number",
    L"  resize 1 1          window 1 to 640x480",
    L"  r 2 7               window 2 to 1280x720",
};

static const WCHAR *const cmd_help_move[] = {
    L"move <window> <x> <y> [display]   (m)",
    L"",
    L"  Moves a window. X and Y are measured from a display's own",
    L"  top-left corner rather than the virtual desktop's, so the same",
    L"  pair means the same place on every screen.",
    L"  Without a display, the one the window is already on.",
    L"  The display number is the one the options menu shows beside",
    L"  that monitor's name.",
    L"  The window keeps its size; only its position changes.",
    L"",
    L"Examples:",
    L"  move 1 100 100      100,100 on the display window 1 is on",
    L"  m 1 0 0             flush into that display's top-left corner",
    L"  m 1 0 0 2           the same corner, but on display 2",
};

static const WCHAR *const cmd_help_search[] = {
    L"search windows <text>             (s w)",
    L"search note <text>                (s n)",
    L"",
    L"  Lists what contains <text>, ignoring case. Windows are matched on",
    L"  their title; notes on their title and their body both, because a",
    L"  note is looked for by what is in it as often as by its name.",
    L"",
    L"  The numbers printed are the ones 'list' and 'list note' would",
    L"  give, not 1, 2, 3 among the matches, so a result can be handed",
    L"  straight to 'go' or 'note'.",
    L"  Everything after the kind is the text, spaces included.",
    L"",
    L"Examples:",
    L"  search windows notepad",
    L"  s w visual studio   the space is part of what is searched for",
    L"  go 7                using a number the search printed",
    L"  s n groceries       notes mentioning groceries anywhere",
    L"  note 4              opening one the search printed",
};

static const WCHAR *const cmd_help_note[] = {
    L"note [<note> [action]]            (n)",
    L"",
    L"  With nothing after it, opens the note list - the same window the",
    L"  N toolbar button opens.",
    L"",
    L"  <note> is the number 'list note' prints. That numbering does not",
    L"  follow the window's sorting, so a number stays the note it was.",
    L"",
    L"  Actions: archive (a), restore (r), delete (d). Without one, the",
    L"  note opens in its own editor. An archived note opens read-only;",
    L"  restore it first to write in it again.",
    L"",
    L"  delete is not undoable from here. The file goes to the Recycle",
    L"  Bin, which is where it can be got back from. It also renumbers",
    L"  the notes after it, so run 'list note' again before typing",
    L"  another number.",
    L"",
    L"Examples:",
    L"  note                the list window",
    L"  l n                 every note, with its number",
    L"  note 3              open note 3",
    L"  n 3 a               file note 3 away, read-only from then on",
    L"  n 3 r               put it back among the active notes",
    L"  n 3 d               delete note 3",
};

static const HgCommandHelp cmd_help_table[] = {
    {L"help", L"h", L"this list, or one command in detail", cmd_help_help, HG_ARRAYSIZE(cmd_help_help)},
    {L"list", L"l", L"windows, resize presets, shortcuts, notes, or sensors", cmd_help_list, HG_ARRAYSIZE(cmd_help_list)},
    {L"go", NULL, L"focus a window by its number", cmd_help_go, HG_ARRAYSIZE(cmd_help_go)},
    {L"resize", L"r", L"resize a window to a preset", cmd_help_resize, HG_ARRAYSIZE(cmd_help_resize)},
    {L"move", L"m", L"move a window, optionally to another display", cmd_help_move, HG_ARRAYSIZE(cmd_help_move)},
    {L"search", L"s", L"find windows or notes by their text", cmd_help_search, HG_ARRAYSIZE(cmd_help_search)},
    {L"note", L"n", L"the note list, or one note by number", cmd_help_note, HG_ARRAYSIZE(cmd_help_note)},
};

static const HgCommandHelp *cmd_help_find(const WCHAR *word)
{
    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_help_table); ++i) {
        if (cmd_word_is(word, cmd_help_table[i].name, cmd_help_table[i].shorthand))
            return &cmd_help_table[i];
    }
    return NULL;
}

static void cmd_help(int argc, WCHAR *argv[])
{
    if (argc >= 2) {
        const HgCommandHelp *entry = cmd_help_find(argv[1]);
        if (!entry) {
            cmd_printf(L"help: no command called '%ls' - type help for the list", argv[1]);
            return;
        }
        for (size_t i = 0; i < entry->detail_count; ++i)
            commandbox_print(entry->detail[i]);
        return;
    }

    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_help_table); ++i) {
        const HgCommandHelp *entry = &cmd_help_table[i];
        cmd_printf(L"%-8ls %-4ls %ls", entry->name, entry->shorthand ? entry->shorthand : L"", entry->summary);
    }
    commandbox_print(L"");
    commandbox_print(L"'help <command>' explains one of them, with examples.");
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

/* The numbering here is the one `note` and `search note` take, and it is not
 * the order the note window shows: that window sorts however the reader left
 * it, and a number that moves when a sort changes is a number nobody can
 * write down. */
static void cmd_list_notes(void)
{
    int count = hg_note_command_count();
    if (count <= 0) {
        commandbox_print(L"no notes yet - 'note' opens the list, where +Add Note makes one");
        return;
    }
    for (int i = 1; i <= count; ++i) {
        HgNoteBrief brief;
        if (hg_note_command_brief(i, &brief))
            cmd_printf(L"%3d  %-9ls %ls", i, brief.archived ? L"archived" : L"", brief.title);
    }
}

/* The diagnostic behind the TMP row. One number is shown; this is every
 * number it was chosen from, so a suspicious reading can be judged. */
static void cmd_list_sensors(void)
{
    HgThermalZone zones[HG_THERMAL_MAX_ZONES];
    int count = hg_thermal_enumerate(zones, (int)HG_ARRAYSIZE(zones));
    if (count <= 0) {
        commandbox_print(L"no thermal zones - this machine's firmware exposes none");
    } else {
        for (int i = 0; i < count; ++i) {
            cmd_printf(L"%3d  %-7ls %3d C  %ls", i + 1, zones[i].from_counter ? L"counter" : L"wmi",
                       zones[i].celsius, zones[i].name);
        }
    }

    int chosen = 0;
    if (hg_thermal_zone_celsius(&chosen))
        cmd_printf(L"     shown as TMP: %d C", chosen);
    else
        commandbox_print(L"     shown as TMP: nothing - the row is hidden");

    int gpu = 0;
    if (hg_get_gpu_temperature(&gpu))
        cmd_printf(L"     shown as GPU: %d C", gpu);
    else
        commandbox_print(L"     shown as GPU: nothing - no adapter reports a sensor");
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
    } else if (cmd_word_is(argv[1], L"note", L"n") || cmd_word_is(argv[1], L"notes", NULL)) {
        cmd_list_notes();
    } else if (cmd_word_is(argv[1], L"sensors", L"t") || cmd_word_is(argv[1], L"sensor", NULL) ||
               cmd_word_is(argv[1], L"temp", NULL)) {
        cmd_list_sensors();
    } else {
        cmd_printf(L"list: unknown kind '%ls' (windows, resize, shortcut, note, sensors)", argv[1]);
    }
}

/* TRUE when a window really was brought forward, so the caller can leave the
 * keyboard where this command just put it. */
static BOOL cmd_go(int argc, WCHAR *argv[])
{
    int number = 0;
    if (argc < 2 || !cmd_parse_int(argv[1], &number)) {
        commandbox_print(L"go: needs a window number, as in 'go 1'");
        return FALSE;
    }

    HWND target = cmd_window_by_number(number);
    if (!target) {
        cmd_printf(L"go: no window %d", number);
        return FALSE;
    }

    if (IsIconic(target))
        ShowWindow(target, SW_RESTORE);
    SetForegroundWindow(target);
    cmd_printf(L"go %d: %ls", number, hg_g_window_items[number - 1].title);
    return TRUE;
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

/* The numbers printed are the ones `list note` gives, not 1, 2, 3 among the
 * matches, so a result can be handed straight back to `note`. */
static void cmd_search_notes(const WCHAR *needle)
{
    int count = hg_note_command_count();
    int found = 0;
    for (int i = 1; i <= count; ++i) {
        if (!hg_note_command_matches(i, needle))
            continue;
        HgNoteBrief brief;
        if (hg_note_command_brief(i, &brief)) {
            cmd_printf(L"%3d  %-9ls %ls", i, brief.archived ? L"archived" : L"", brief.title);
            ++found;
        }
    }
    if (found == 0)
        cmd_printf(L"no note matches '%ls'", needle);
}

static void cmd_search(int argc, WCHAR *argv[], const WCHAR *line)
{
    BOOL windows = (argc >= 2) && cmd_word_is(argv[1], L"windows", L"w");
    BOOL notes = (argc >= 2) && (cmd_word_is(argv[1], L"note", L"n") || cmd_word_is(argv[1], L"notes", NULL));
    if (argc < 3 || (!windows && !notes)) {
        commandbox_print(L"search: windows or note, as in 'search windows notepad' or 's n groceries'");
        return;
    }

    const WCHAR *needle = cmd_tail(line, 2);
    if (!needle || !*needle) {
        commandbox_print(L"search: needs something to look for");
        return;
    }

    if (notes) {
        cmd_search_notes(needle);
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

/* Returns TRUE when a note window was put in front, which is what stops the
 * command box from pulling the keyboard straight back off it. */
static BOOL cmd_note(int argc, WCHAR *argv[])
{
    if (argc < 2) {
        show_note_list_window();
        commandbox_print(L"note: opened the note list");
        return TRUE;
    }

    int number = 0;
    if (!cmd_parse_int(argv[1], &number)) {
        cmd_printf(L"note: '%ls' is not a note number - see 'list note'", argv[1]);
        return FALSE;
    }

    HgNoteBrief brief;
    if (!hg_note_command_brief(number, &brief)) {
        cmd_printf(L"note: no note %d (see 'list note')", number);
        return FALSE;
    }

    if (argc < 3) {
        if (!hg_note_command_open(number))
            return FALSE;
        if (brief.archived)
            cmd_printf(L"note %d: %ls - archived, so read only", number, brief.title);
        else
            cmd_printf(L"note %d: %ls", number, brief.title);
        return TRUE;
    }

    if (cmd_word_is(argv[2], L"archive", L"a") || cmd_word_is(argv[2], L"restore", L"r")) {
        BOOL archive = cmd_word_is(argv[2], L"archive", L"a");
        BOOL changed = FALSE;
        if (!hg_note_command_set_archived(number, archive, &changed))
            return FALSE;
        if (!changed)
            cmd_printf(L"note %d: already %ls", number, archive ? L"archived" : L"active");
        else if (archive)
            cmd_printf(L"note %d archived: %ls - read only until restored", number, brief.title);
        else
            cmd_printf(L"note %d restored: %ls", number, brief.title);
        return FALSE;
    }

    if (cmd_word_is(argv[2], L"delete", L"d")) {
        /* The title is read before the note is gone, so the line that confirms
         * the deletion can still say what was deleted. */
        if (!hg_note_command_delete(number))
            return FALSE;
        cmd_printf(L"note %d deleted: %ls - it is in the Recycle Bin", number, brief.title);
        return FALSE;
    }

    cmd_printf(L"note: unknown action '%ls' (archive, restore, delete)", argv[2]);
    return FALSE;
}

BOOL hg_command_execute(const WCHAR *line)
{
    if (!line)
        return FALSE;
    while (*line && cmd_is_space(*line))
        ++line;
    if (!*line)
        return FALSE;

    WCHAR store[HG_MAX_STR];
    WCHAR *argv[HG_CMD_MAX_ARGS];
    int argc = cmd_tokenize(line, store, HG_ARRAYSIZE(store), argv, HG_CMD_MAX_ARGS);
    if (argc < 0) {
        cmd_printf(L"that line is too long - the limit is %d characters", (int)HG_ARRAYSIZE(store) - 1);
        return FALSE;
    }
    if (argc == 0)
        return FALSE;

    /* `go` and `note` put another window in front on purpose; saying so is what
     * stops the command box from pulling the keyboard straight back. */
    BOOL moved_focus = FALSE;

    if (cmd_word_is(argv[0], L"help", L"h")) {
        cmd_help(argc, argv);
    } else if (cmd_word_is(argv[0], L"list", L"l")) {
        cmd_list(argc, argv);
    } else if (cmd_word_is(argv[0], L"go", NULL)) {
        moved_focus = cmd_go(argc, argv);
    } else if (cmd_word_is(argv[0], L"resize", L"r")) {
        cmd_resize(argc, argv);
    } else if (cmd_word_is(argv[0], L"move", L"m")) {
        cmd_move(argc, argv);
    } else if (cmd_word_is(argv[0], L"search", L"s")) {
        cmd_search(argc, argv, line);
    } else if (cmd_word_is(argv[0], L"note", L"n")) {
        moved_focus = cmd_note(argc, argv);
    } else {
        cmd_printf(L"unknown command '%ls' - type help", argv[0]);
    }

    return moved_focus;
}
