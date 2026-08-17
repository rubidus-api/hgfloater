/* The command box's language.
 *
 * Every command that names a window uses the number `list` printed beside it,
 * and that number is an index into the same window list the toolbar draws. The
 * list is only re-read by the commands that print numbers: if `go` refreshed
 * first, the window list could reorder between the `list` the reader saw and
 * the number they typed, and they would switch to the wrong window. */
#include <limits.h>

#include "hg_command.h"
#include "hg_utils.h"
#include "hg_globals.h"
#include "hg_tabs.h"
#include "widgets/hg_note.h"
#include "widgets/hg_clip.h"
#include "widgets/hg_monitor.h"
#include "widgets/hg_settings.h"
#include "hg_values.h"
#include "hg_options.h"
#include "hg_keys.h"

#define HG_CMD_MAX_ARGS 8

/* ------------------------------------------------------------- history
 *
 * Shift+Left and Shift+Right walk this. The cap is a setting because a line
 * you typed is the cheapest thing in the program to keep and the most annoying
 * thing to retype, but an unbounded list of everything anyone ever ran is a
 * different promise than a scratchpad, so it has a number and the number is
 * visible. Nothing here is written to disk. */
#define HG_CMD_HISTORY_CAP 256
#define HG_CMD_HISTORY_DEFAULT 64

static WCHAR *s_history[HG_CMD_HISTORY_CAP]; /* newest first */
static int s_history_count = 0;
static int s_history_max = 0;     /* 0 until the config is read */
static int s_history_cursor = -1; /* -1 when not walking the list */

static int cmd_history_clamp(int value)
{
    if (value < 1)
        return 1;
    if (value > HG_CMD_HISTORY_CAP)
        return HG_CMD_HISTORY_CAP;
    return value;
}

static void cmd_history_trim(void)
{
    while (s_history_count > s_history_max) {
        free(s_history[s_history_count - 1]);
        s_history[s_history_count - 1] = NULL;
        --s_history_count;
    }
}

int hg_command_history_max(void)
{
    if (s_history_max == 0) {
        s_history_max = cmd_history_clamp(
            (int)GetPrivateProfileIntW(L"commandbox", L"history_max", HG_CMD_HISTORY_DEFAULT, hg_g_config_path));
    }
    return s_history_max;
}

void hg_command_set_history_max(int value)
{
    (void)hg_command_history_max(); /* make sure the file has been read first */
    s_history_max = cmd_history_clamp(value);

    WCHAR text[16];
    hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d", s_history_max);
    WritePrivateProfileStringW(L"commandbox", L"history_max", text, hg_g_config_path);

    /* Same asymmetry as the clipboard: lowering it takes effect now, because
     * "at most this many" would otherwise be false the moment it was set. */
    cmd_history_trim();
}

void hg_command_history_add(const WCHAR *line)
{
    if (!line || !*line)
        return;
    (void)hg_command_history_max();

    /* Running the same line twice in a row is one entry, not two: a history
     * whose first three rows are the same command is a history you have to
     * scroll past to reach anything. */
    if (s_history_count > 0 && s_history[0] && wcscmp(s_history[0], line) == 0) {
        s_history_cursor = -1;
        return;
    }

    size_t cch = wcslen(line);
    WCHAR *copy = (WCHAR *)malloc(sizeof(WCHAR) * (cch + 1u));
    if (!copy)
        return;
    memcpy(copy, line, sizeof(WCHAR) * (cch + 1u));

    if (s_history_count >= s_history_max) {
        free(s_history[s_history_max - 1]);
        s_history[s_history_max - 1] = NULL;
        s_history_count = s_history_max - 1;
    }
    if (s_history_count > 0)
        memmove(&s_history[1], &s_history[0], sizeof(s_history[0]) * (size_t)s_history_count);
    s_history[0] = copy;
    ++s_history_count;
    s_history_cursor = -1;
}

void hg_command_history_reset(void)
{
    s_history_cursor = -1;
}

/* The whole history, oldest first, for the command box's history list. Index 0
 * is the oldest entry so the on-screen numbers can start at 1 and never move
 * as new commands arrive. */
int hg_command_history_count(void)
{
    return s_history_count;
}

const WCHAR *hg_command_history_at(int oldest_index)
{
    if (oldest_index < 0 || oldest_index >= s_history_count)
        return NULL;
    return s_history[s_history_count - 1 - oldest_index]; /* storage is newest first */
}

/* direction > 0 walks towards older lines. Returns NULL at either end, and an
 * empty string when walking back past the newest, which is how the input box
 * gets cleared rather than stuck on the first entry. */
const WCHAR *hg_command_history_step(int direction)
{
    if (s_history_count <= 0)
        return NULL;

    if (direction > 0) {
        if (s_history_cursor + 1 >= s_history_count)
            return NULL;
        ++s_history_cursor;
    } else {
        if (s_history_cursor < 0)
            return NULL;
        --s_history_cursor;
        if (s_history_cursor < 0)
            return L"";
    }
    return s_history[s_history_cursor];
}

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
        int digit = (int)(*p - L'0');
        /* Reject before the multiply can pass INT_MAX: overflow of a signed
         * int is undefined, and a wrapped value could name a real window. */
        if (value > (INT_MAX - digit) / 10)
            return FALSE;
        value = value * 10 + digit;
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
/* Windows' own display number where the system gives us one, so the number in
 * `show monitor` is the number on the Settings page and the number `move`
 * takes. Position in our list only as a fallback, and only when nothing is
 * labelled - mixing the two would give two monitors the same number. */
static int cmd_monitor_index_by_number(int number)
{
    BOOL any_numbered = FALSE;
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        int labelled = hg_monitor_display_number(hg_g_monitors[i].name);
        if (labelled > 0)
            any_numbered = TRUE;
        if (labelled == number)
            return i;
    }
    if (!any_numbered && number >= 1 && number <= hg_g_monitor_count)
        return number - 1;
    return -1;
}

static int cmd_monitor_number_of(int index)
{
    int labelled = hg_monitor_display_number(hg_g_monitors[index].name);
    return (labelled > 0) ? labelled : index + 1;
}

static const MonitorInfo *cmd_monitor_by_number(int number)
{
    int index = cmd_monitor_index_by_number(number);
    return (index >= 0) ? &hg_g_monitors[index] : NULL;
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
    L"help [command|key]  (h)",
    L"",
    L"  With no command, lists them all. With one, explains that one",
    L"  and shows what it looks like in use. 'help key' prints the keys",
    L"  this window answers to - the same list it opens on.",
    L"",
    L"Examples:",
    L"  help                everything, in one line each",
    L"  h move              just move, in detail",
    L"  h k                 the keys, not the commands",
};

/* The keys, by where you press them.
 *
 * One list of every key in the program would be the wrong shape: the same key
 * means different things in the taskbox and in a note, and a reader is always
 * standing in one of those places rather than in all of them. So the keys are
 * grouped by the window they belong to, `help key` prints the lot with an index
 * at the top, and `help key <topic>` prints one.
 *
 * This is also what the command box shows when it opens, which is why the
 * commandbox topic comes with a paragraph rather than a bare table: a box with
 * a blinking cursor and no other clue is a box you have to be told about
 * somewhere else. */
static const WCHAR *const cmd_key_global[] = {
    L"Everywhere",
    L"  Win+Alt+Space      show or hide the taskbox, from any program",
    L"  F1                 About",
    L"  Ctrl+Q             quit hgfloater, from any of its windows",
    L"  Alt+F4             quit, from the floater or the taskbox; from a",
    L"                     note, the clipboard or the command box it closes",
    L"                     that window instead",
    L"  Ctrl+R  Ctrl+0     reset position, size and opacity (widgets only,",
    L"  F5  Ctrl+Shift+R   so these are safe to press while typing)",
};

static const WCHAR *const cmd_key_floater[] = {
    L"The floater - the small clock widget",
    L"  click it           open the taskbox   T           the same",
    L"  C   Ctrl+E         the command box",
    L"  Ctrl+N             the note list      Ctrl+L      the clipboard",
    L"  Alt+arrows/WASD    move it            Alt+drag    the same",
    L"  Ctrl+ + / -        font size          Ctrl+wheel  the same",
    L"  Alt+ + / -         opacity            Alt+wheel   the same",
};

static const WCHAR *const cmd_key_taskbox[] = {
    L"The taskbox - the dashboard the floater opens into",
    L"  arrows / WASD      move the focus between icons",
    L"  Space              activate it: the window comes forward and the",
    L"                     dashboard folds back into the floater",
    L"  Enter  F2          the focused icon's menu",
    L"  Shift+0-9, A-Z     straight to the icon with that label",
    L"  C   Ctrl+E         the command box",
    L"  N   Ctrl+N         the note list      Ctrl+L      the clipboard",
    L"  Esc                hide it, and re-read the shortcuts folder",
    L"  Ctrl+arrows/WASD   how many columns and rows the grid has",
    L"  Alt+arrows/WASD    move the window    Ctrl+ + / -  icon size",
    L"",
    L"  On an icon with tabs, a box of its tabs opens beside it:",
    L"  1-9  0             straight to that tab, and to the last one",
    L"  Tab                step into the box - the frame appears, and the",
    L"                     rest of the keys are the box's from then on",
    L"  up/down  Home/End  move the selection (inside the box)",
    L"  a-z  A-Z           the labels past the ninth row (inside the box:",
    L"                     outside it those letters move the grid)",
    L"  Enter  Space       switch to the selected tab",
    L"  Esc                close the box, keyboard stays in the taskbox",
};

static const WCHAR *const cmd_key_commandbox[] = {
    L"The command box - this window",
    L"  Enter              run what is typed",
    L"  Shift+Enter        new line - several commands, run in order",
    L"  Ctrl+S             scroll mode: up/down a line, left/right a page",
    L"  Ctrl+H             history mode: the list, numbered from the oldest;",
    L"                     up/down choose, Enter puts one in the input box",
    L"  Esc                leave the mode, or close and go back to the taskbox",
    L"  Ctrl+W             close this window, leaving the taskbox alone",
    L"  Ctrl+Space         jump to the input box",
    L"  Ctrl+Wheel         text size          Alt+Wheel   opacity",
    L"  Alt+arrows         move this window   Ctrl+arrows resize it",
    L"  Ctrl+X/C/V         cut, copy, paste   Ctrl+Z/Y    undo, redo",
    L"",
    L"  The arrows stay the caret's until a mode is on, so selecting and",
    L"  editing text works the way it does everywhere else. A mode says so",
    L"  twice: in the title bar, and as a coloured frame around the pane",
    L"  the arrows now belong to - the transcript, or the input box.",
    L"",
    L"  The input box is always three lines tall and scrolls past that.",
};

static const WCHAR *const cmd_key_note[] = {
    L"Notes - the list, and each editor window",
    L"  In the list:",
    L"  Enter              open the selected note; on +Add Note, make one",
    L"  Insert             make a note from any row",
    L"  K                  archive the selected note, or restore it",
    L"  Delete             delete it (to the Recycle Bin)",
    L"  Esc  Ctrl+W        close the list",
    L"",
    L"  In an editor:",
    L"  Ctrl+X/C/V         cut, copy, paste",
    L"  Ctrl+Z  Ctrl+Y     undo and redo, a hundred levels deep",
    L"  Ctrl+A             select all         right-click  the full menu",
    L"  Ctrl+Wheel         text size, shared by the list and every editor",
    L"  Esc  Ctrl+W        close it - what you typed is saved first",
};

static const WCHAR *const cmd_key_clipboard[] = {
    L"The clipboard history",
    L"  Ctrl+L             open it, from the floater or the taskbox",
    L"  Enter  click       make that clip the current one; the window stays",
    L"  right-click        that clip's menu: copy, delete, delete all",
    L"  Del                delete the selected clip, selection stays put",
    L"  Esc  Ctrl+W        close it",
    L"  Ctrl+Wheel         text size          Alt+Wheel   opacity",
    L"  Alt+arrows         move this window   Ctrl+arrows resize it",
    L"",
    L"  The search box at the top filters the list; the number box at the",
    L"  top right is how many clips to keep. Capture runs whether or not",
    L"  this window is open.",
};

typedef struct HgKeyTopic {
    const WCHAR *name;
    const WCHAR *summary;
    const WCHAR *const *lines;
    size_t line_count;
} HgKeyTopic;

static const HgKeyTopic cmd_key_topics[] = {
    {L"global", L"the keys that work from anywhere", cmd_key_global, HG_ARRAYSIZE(cmd_key_global)},
    {L"floater", L"the small clock widget", cmd_key_floater, HG_ARRAYSIZE(cmd_key_floater)},
    {L"taskbox", L"the dashboard, its grid and its tab boxes", cmd_key_taskbox, HG_ARRAYSIZE(cmd_key_taskbox)},
    {L"commandbox", L"this window", cmd_key_commandbox, HG_ARRAYSIZE(cmd_key_commandbox)},
    {L"note", L"the note list and the note editors", cmd_key_note, HG_ARRAYSIZE(cmd_key_note)},
    {L"clipboard", L"the clipboard history", cmd_key_clipboard, HG_ARRAYSIZE(cmd_key_clipboard)},
};

static void cmd_key_print_topic(const HgKeyTopic *topic)
{
    for (size_t i = 0; i < topic->line_count; ++i)
        commandbox_print(topic->lines[i]);
}

static void cmd_key_print_index(void)
{
    commandbox_print(L"Keys, by where you press them.  'h k <topic>' for one:");
    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_key_topics); ++i)
        cmd_printf(L"  %-12ls %ls", cmd_key_topics[i].name, cmd_key_topics[i].summary);
    commandbox_print(L"");
    commandbox_print(L"  As many letters as it takes to be the only one: 'h k f',");
    commandbox_print(L"  'h k g', 'h k n', 'h k t' - and 'cl' or 'co' for the two");
    commandbox_print(L"  that both start with c.");
}

/* Enough letters to be the only match. An exact name wins outright, so a topic
 * whose name is a prefix of another would still be reachable by typing it in
 * full - none is today, and this costs one comparison to keep true. */
static const HgKeyTopic *cmd_key_topic_lookup(const WCHAR *word, int *out_matches)
{
    if (out_matches)
        *out_matches = 0;
    if (!word || !*word)
        return NULL;

    size_t len = wcslen(word);
    const HgKeyTopic *found = NULL;
    int matches = 0;

    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_key_topics); ++i) {
        const WCHAR *name = cmd_key_topics[i].name;
        if (lstrcmpiW(word, name) == 0) {
            if (out_matches)
                *out_matches = 1;
            return &cmd_key_topics[i];
        }
        if (wcslen(name) >= len && CompareStringOrdinal(word, (int)len, name, (int)len, TRUE) == CSTR_EQUAL) {
            ++matches;
            found = &cmd_key_topics[i];
        }
    }

    if (out_matches)
        *out_matches = matches;
    return (matches == 1) ? found : NULL;
}

/* No topic: the index, then every topic under it. The index alone would make a
 * reader type a second command to see anything, and this window scrolls. */
void hg_command_print_key_help(void)
{
    cmd_key_print_index();
    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_key_topics); ++i) {
        commandbox_print(L"");
        cmd_key_print_topic(&cmd_key_topics[i]);
    }
    commandbox_print(L"");
    commandbox_print(L"Type 'help' for the commands, or 'h <command>' for one in detail.");
}

static void cmd_key_help(const WCHAR *topic_word)
{
    if (!topic_word) {
        hg_command_print_key_help();
        return;
    }

    int matches = 0;
    const HgKeyTopic *topic = cmd_key_topic_lookup(topic_word, &matches);
    if (topic) {
        cmd_key_print_topic(topic);
        return;
    }

    if (matches > 1) {
        cmd_printf(L"help key: '%ls' fits more than one topic - type another letter:", topic_word);
        size_t len = wcslen(topic_word);
        for (size_t i = 0; i < HG_ARRAYSIZE(cmd_key_topics); ++i) {
            const WCHAR *name = cmd_key_topics[i].name;
            if (wcslen(name) >= len && CompareStringOrdinal(topic_word, (int)len, name, (int)len, TRUE) == CSTR_EQUAL)
                cmd_printf(L"  %-12ls %ls", name, cmd_key_topics[i].summary);
        }
        return;
    }

    cmd_printf(L"help key: no topic called '%ls'.", topic_word);
    commandbox_print(L"");
    cmd_key_print_index();
}

static const WCHAR *const cmd_help_show[] = {
    L"show [kind [n]]     (s)",
    L"",
    L"  Prints a numbered list. Without a kind, shows windows.",
    L"  The number beside a window is the one go, resize, and move take,",
    L"  so this is where those commands get their arguments.",
    L"  Kinds: windows (w), resize (r), shortcut (c), note (n),",
    L"         monitor (m), sensors (s), tabs (t), value (v),",
    L"         option (o), key (k),",
    L"         theme, tabsinfo (no shorthand - both diagnostics).",
    L"",
    L"Examples:",
    L"  show                every window, numbered",
    L"  s w                 the same list",
    L"  s w class           with each window's class, for tab_classes",
    L"  s r                 the resize presets, numbered for 'resize'",
    L"  s c                 the shortcut icons",
    L"  s n                 every note, numbered for the 'note' command",
    L"  s m                 every display, numbered, with its size and place",
    L"  s m 1               turn display 1's preview window on - again to close",
    L"  s s                 every temperature sensor, numbered",
    L"  s s 2               just sensor 2, with its unit",
    L"  s t                 every tab of every tabbed window, numbered",
    L"  s v                 the settable values and what they are now",
    L"  s o                 the on/off options and what they are now",
    L"  s k                 every window, function and key; s k floater for one",
    L"  show theme          the colours, opacity and handles the floater is",
    L"                      painted from - run it while it looks wrong",
    L"  show tabsinfo       what the tab reader itself is doing, and costing",
    L"",
    L"  'show sensors' is a diagnostic. The floater shows one CPU",
    L"  temperature, chosen from whatever the firmware exposes; this",
    L"  prints all of them so a reading that looks wrong can be checked",
    L"  against its neighbours. A zone that never moves is firmware",
    L"  filler, and is why the chosen one may not be the hottest.",
};

static const WCHAR *const cmd_help_go[] = {
    L"go <window>",
    L"go tab <n>          (g t <n>)",
    L"",
    L"  Brings a window to the front, restoring it first if it is",
    L"  minimised. <window> is the number 'list' printed beside it.",
    L"",
    L"  'go tab' takes a number from 'show tabs' instead - that list",
    L"  runs across every tabbed window, so it reaches a tab without",
    L"  your having to know which window is holding it.",
    L"",
    L"Examples:",
    L"  list                read the numbers",
    L"  go 3                switch to the third window",
    L"  s t                 read the tab numbers",
    L"  g t 4               switch to the fourth tab in that list",
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

static const WCHAR *const cmd_help_find[] = {
    L"find windows <text>               (f w)",
    L"find note <text>                  (f n)",
    L"",
    L"  Lists what contains <text>, ignoring case. Windows are matched on",
    L"  their title; notes on their title and their body both, because a",
    L"  note is looked for by what is in it as often as by its name.",
    L"",
    L"  The numbers printed are the ones 'show' and 'show note' would",
    L"  give, not 1, 2, 3 among the matches, so a result can be handed",
    L"  straight to 'go' or 'note'.",
    L"  Everything after the kind is the text, spaces included.",
    L"",
    L"Examples:",
    L"  find windows notepad",
    L"  f w visual studio   the space is part of what is searched for",
    L"  go 7                using a number the search printed",
    L"  f n groceries       notes mentioning groceries anywhere",
    L"  note 4              opening one the search printed",
};

static const WCHAR *const cmd_help_note[] = {
    L"note [new|<note> [action]]        (n)",
    L"",
    L"  With nothing after it, opens the note list - the same window the",
    L"  N toolbar button opens.",
    L"",
    L"  'note new' makes one and opens it, the same as +Add Note.",
    L"",
    L"  <note> is the number 'show note' prints. That numbering does not",
    L"  follow the window's sorting, so a number stays the note it was.",
    L"",
    L"  Actions: archive (a), restore (r), delete (d). Without one, the",
    L"  note opens in its own editor. An archived note opens read-only;",
    L"  restore it first to write in it again.",
    L"",
    L"  delete is not undoable from here. The file goes to the Recycle",
    L"  Bin, which is where it can be got back from. It also renumbers",
    L"  the notes after it, so run 'show note' again before typing",
    L"  another number.",
    L"",
    L"Examples:",
    L"  note                the list window",
    L"  n n                 write a new note",
    L"  s n                 every note, with its number",
    L"  note 3              open note 3",
    L"  n 3 a               file note 3 away, read-only from then on",
    L"  n 3 r               put it back among the active notes",
    L"  n 3 d               delete note 3",
};

static const WCHAR *const cmd_help_config[] = {
    L"config              (c)",
    L"",
    L"  Opens config.ini in Notepad. Everything the program remembers",
    L"  between runs is in that one file, in plain text.",
    L"",
    L"  hgfloater writes the file as settings change, so a value you",
    L"  edit by hand can be overwritten by the running program. Change",
    L"  it, save, and restart if the setting has a live control.",
    L"",
    L"Examples:",
    L"  config",
};

static const WCHAR *const cmd_help_clipboard[] = {
    L"clipboard [<n>]     (b)",
    L"",
    L"  With nothing after it, prints the clipboard history, newest",
    L"  first, numbered. With a number, makes that entry the current",
    L"  clipboard: everything above it moves down one, so the list ends",
    L"  up in the order it would have been in had you copied that text",
    L"  again. Nothing is lost.",
    L"",
    L"  The same list the L toolbar button shows. It is text only, and",
    L"  it is never written to disk.",
    L"",
    L"Examples:",
    L"  clipboard           the history, numbered",
    L"  b                   the same",
    L"  b 3                 make entry 3 the current clipboard",
};

static const WCHAR *const cmd_help_write[] = {
    L"write value <what> <number>            (w v)",
    L"write option <what> <on|off|toggle>    (w o)",
    L"",
    L"  Sets one of the values 'show value' lists. <what> is either the",
    L"  number beside it or its name, and a unique leading piece of the",
    L"  name is enough.",
    L"",
    L"  A value that lives in a settings file is written there straight",
    L"  away, so it survives a restart without waiting for anything.",
    L"  Brightness and volume are the machine's state rather than ours",
    L"  and are not saved anywhere; the line printed says which it was.",
    L"",
    L"  Out-of-range numbers are clamped rather than refused, and the",
    L"  line printed shows what the value actually became.",
    L"",
    L"Examples:",
    L"  s v                 what can be set, and what it is now",
    L"  write value 1 60    by number",
    L"  w v bright 60       by name, shortened",
    L"  w v clip-max 32     keep 32 clipboard entries from now on",
    L"",
    L"  'write option' is the same idea for the switches - the ones the",
    L"  O button lists under Options - by number or by name, set to on,",
    L"  off, or toggle. The change reaches the settings file and the",
    L"  running program at once, so nothing waits for a restart.",
    L"",
    L"  s o                 the options and what they are now",
    L"  w o hoveropen on    open the taskbox on hover again",
    L"  w o tabs toggle     turn tabs-as-icons the other way",
};

static const WCHAR *const cmd_help_settings[] = {
    L"settings            (set)",
    L"",
    L"  Opens the settings window: every option, every value and every",
    L"  key, in one list. It is a view of the same tables this command",
    L"  box writes through, so a change made in either shows up in the",
    L"  other without anything being saved or applied.",
    L"",
    L"  'config' is the other half of the same subject and opens the file",
    L"  itself in Notepad, including the handful of keys that have no",
    L"  control anywhere.",
};

static const WCHAR *const cmd_help_bind[] = {
    L"bind <window> <function> <key|default>",
    L"unbind <window> <function> [key]",
    L"",
    L"  Keys are kept as window, function, and the chords that reach it -",
    L"  none, one, or up to four. The window comes first because the same",
    L"  function name lives in several of them: 'notes' is one row in the",
    L"  floater and another in the taskbox.",
    L"",
    L"  Windows: system (works from inside any program), widget (the",
    L"  floater and the taskbox together), floater, taskbox.",
    L"",
    L"  A chord is written the way it is read: Ctrl+N, Alt+F4, Ctrl+Shift+R,",
    L"  F2, Esc, Space, Plus, NumMinus. Win+ belongs to the system window",
    L"  alone - the shell takes it before any other window sees it, and the",
    L"  bare arrows and WASD belong to no function: they are how a window is",
    L"  walked. Held with Ctrl or Alt they bind like anything else.",
    L"",
    L"  'unbind <window> <function>' with no chord takes every key away and",
    L"  leaves the function reachable by button and menu. 'bind ... default'",
    L"  puts the built-in keys back.",
    L"",
    L"Examples:",
    L"  show key            every window, function and chord",
    L"  s k floater         just the floater's",
    L"  bind floater notes Ctrl+Shift+N",
    L"  unbind taskbox clipboard Ctrl+L",
    L"  bind system showtaskbox Win+Alt+Z",
    L"  bind floater notes default",
};

static const WCHAR *const cmd_help_clear[] = {
    L"clear               (cls)",
    L"",
    L"  Empties the transcript above. The command history is a",
    L"  different thing and is left alone - Ctrl+H still has every",
    L"  line you have run.",
};

static const HgCommandHelp cmd_help_table[] = {
    {L"help", L"h", L"this list, one command in detail, or the keys", cmd_help_help, HG_ARRAYSIZE(cmd_help_help)},
    {L"show", L"s", L"windows, presets, shortcuts, notes, displays, values", cmd_help_show,
     HG_ARRAYSIZE(cmd_help_show)},
    {L"find", L"f", L"windows or notes containing some text", cmd_help_find, HG_ARRAYSIZE(cmd_help_find)},
    {L"go", NULL, L"focus a window by its number", cmd_help_go, HG_ARRAYSIZE(cmd_help_go)},
    {L"resize", L"r", L"resize a window to a preset", cmd_help_resize, HG_ARRAYSIZE(cmd_help_resize)},
    {L"move", L"m", L"move a window, optionally to another display", cmd_help_move, HG_ARRAYSIZE(cmd_help_move)},
    {L"note", L"n", L"the note list, a new note, or one by number", cmd_help_note, HG_ARRAYSIZE(cmd_help_note)},
    {L"clipboard", L"b", L"the clipboard history, or take one entry", cmd_help_clipboard, HG_ARRAYSIZE(cmd_help_clipboard)},
    {L"write", L"w", L"set a value or an option, as 'show value' and 'show option' list them",
     cmd_help_write, HG_ARRAYSIZE(cmd_help_write)},
    {L"config", L"c", L"open config.ini in Notepad", cmd_help_config, HG_ARRAYSIZE(cmd_help_config)},
    {L"settings", L"set", L"the settings window: options, values and keys", cmd_help_settings,
     HG_ARRAYSIZE(cmd_help_settings)},
    {L"bind", NULL, L"give a function a key, in one window", cmd_help_bind, HG_ARRAYSIZE(cmd_help_bind)},
    {L"unbind", NULL, L"take a key away, or every key of one function", cmd_help_bind,
     HG_ARRAYSIZE(cmd_help_bind)},
    {L"clear", L"cls", L"empty the transcript above", cmd_help_clear, HG_ARRAYSIZE(cmd_help_clear)},
};

static const HgCommandHelp *cmd_help_lookup(const WCHAR *word)
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
        const HgCommandHelp *entry = cmd_help_lookup(argv[1]);
        if (!entry) {
            if (cmd_word_is(argv[1], L"key", L"k") || cmd_word_is(argv[1], L"keys", NULL)) {
                cmd_key_help((argc >= 3) ? argv[2] : NULL);
                return;
            }
            cmd_printf(L"help: no command called '%ls' - type help, or 'h k' for the keys", argv[1]);
            return;
        }
        for (size_t i = 0; i < entry->detail_count; ++i)
            commandbox_print(entry->detail[i]);
        return;
    }

    for (size_t i = 0; i < HG_ARRAYSIZE(cmd_help_table); ++i) {
        const HgCommandHelp *entry = &cmd_help_table[i];
        cmd_printf(L"%-10ls %-4ls %ls", entry->name, entry->shorthand ? entry->shorthand : L"", entry->summary);
    }
    /* 'key' is not a command, so it has no row above - but it is the thing a
     * reader looks for first and finding it must not require guessing. */
    cmd_printf(L"%-10ls %-4ls %ls", L"help key", L"h k", L"every key, grouped by where you press it");
    commandbox_print(L"");
    commandbox_print(L"'help <command>' explains one of them, with examples.");
}

/* Every tab of every tabbed window, in one numbered list.
 *
 * The numbers are this list's own and run across windows, because that is what
 * `go tab <n>` takes: the hover box is for the tabs of the window under the
 * pointer, and this is for finding a tab when you do not know which window it
 * is in. The table below is remembered so `go tab` means the same thing the
 * reader just read - the same contract `show windows` has with `go`. */
#define HG_CMD_TAB_LIST_MAX 128
static struct {
    HWND hwnd;
    int tab_index;
} s_tab_list[HG_CMD_TAB_LIST_MAX];
static int s_tab_list_count = 0;

static void cmd_list_tabs(void)
{
    cmd_refresh_windows();
    s_tab_list_count = 0;

    if (!hg_tabs_enabled()) {
        commandbox_print(L"tabs are off - turn on Show Tabs as Task Icons, or [taskbox] show_tabs=1");
        return;
    }

    /* Ask for everything first, then read: the worker staggers the windows
     * and the answers land while this prints, so a first run may show a
     * window's tabs as pending and a second run has them. */
    HWND ask[HG_TABS_WORKER_WINDOWS];
    int ask_count = 0;
    for (int i = 0; i < hg_g_window_count && ask_count < HG_TABS_WORKER_WINDOWS; ++i) {
        if (hg_tabs_window_may_have_tabs(hg_g_window_items[i].hwnd))
            ask[ask_count++] = hg_g_window_items[i].hwnd;
    }
    if (ask_count > 0)
        hg_tabs_request(ask, ask_count);

    static WCHAR titles[HG_TABS_MAX_PER_WINDOW][HG_MAX_STR];
    int windows_seen = 0;
    for (int i = 0; i < hg_g_window_count; ++i) {
        HWND hwnd = hg_g_window_items[i].hwnd;
        if (!hg_tabs_window_may_have_tabs(hwnd))
            continue;
        ++windows_seen;

        int count = hg_tabs_take_result(hwnd, titles, HG_TABS_MAX_PER_WINDOW, NULL);
        cmd_printf(L"[%ls]", hg_g_window_items[i].title);
        if (count < 0) {
            commandbox_print(L"     (reading - run it again in a moment)");
            continue;
        }
        if (count == 0) {
            commandbox_print(L"     (no tabs)");
            continue;
        }
        for (int t = 0; t < count && s_tab_list_count < HG_CMD_TAB_LIST_MAX; ++t) {
            s_tab_list[s_tab_list_count].hwnd = hwnd;
            s_tab_list[s_tab_list_count].tab_index = t;
            ++s_tab_list_count;
            cmd_printf(L"%3d  %ls", s_tab_list_count, titles[t]);
        }
    }

    if (windows_seen == 0)
        commandbox_print(L"no windows that can have tabs are open");
    else if (s_tab_list_count > 0)
        commandbox_print(L"'go tab <n>' (g t <n>) switches to one");
}

/* With `class`, the window class beside each title. That is the name
 * `[taskbox] tab_classes` takes, and without this there is no way to find it
 * short of a separate tool. */
static void cmd_list_windows(BOOL with_class)
{
    cmd_refresh_windows();

    if (hg_g_window_count <= 0) {
        commandbox_print(L"no windows");
        return;
    }
    for (int i = 0; i < hg_g_window_count; ++i) {
        if (with_class) {
            WCHAR class_name[64];
            if (GetClassNameW(hg_g_window_items[i].hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0)
                StringCchCopyW(class_name, HG_ARRAYSIZE(class_name), L"?");
            cmd_printf(L"%3d  %-28ls %ls", i + 1, class_name, hg_g_window_items[i].title);
        } else {
            cmd_printf(L"%3d  %ls", i + 1, hg_g_window_items[i].title);
        }
    }
    if (with_class)
        commandbox_print(L"     add one to [taskbox] tab_classes in config.ini to look for its tabs");
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
/* The sensors, numbered so one of them can be asked for on its own. The two
 * readings the floater actually draws come last and keep their own numbers,
 * because "which of these is on the bar" is the question this command exists to
 * answer. */
static int cmd_sensor_collect(HgThermalZone *zones, int max, int *out_zone_count)
{
    int count = hg_thermal_enumerate(zones, max);
    if (out_zone_count)
        *out_zone_count = count;
    /* zones, then TMP, then GPU */
    return count + 2;
}

static void cmd_sensor_line(int number, const HgThermalZone *zones, int zone_count, WCHAR *out, size_t out_cch)
{
    if (number <= zone_count) {
        const HgThermalZone *z = &zones[number - 1];
        hellgates_wsprintf(out, out_cch, L"%-7ls %ls: %d C", z->from_counter ? L"counter" : L"wmi", z->name,
                           z->celsius);
        return;
    }

    if (number == zone_count + 1) {
        int chosen = 0;
        if (hg_thermal_zone_celsius(&chosen))
            hellgates_wsprintf(out, out_cch, L"shown  TMP (CPU thermal zone): %d C", chosen);
        else
            StringCchCopyW(out, out_cch, L"shown  TMP: nothing - the row is hidden");
        return;
    }

    int gpu = 0;
    if (hg_get_gpu_temperature(&gpu))
        hellgates_wsprintf(out, out_cch, L"shown  GPU (adapter sensor): %d C", gpu);
    else
        StringCchCopyW(out, out_cch, L"shown  GPU: nothing - no adapter reports a sensor");
}

static void cmd_show_sensors(int argc, WCHAR *argv[])
{
    HgThermalZone zones[HG_THERMAL_MAX_ZONES];
    int zone_count = 0;
    int total = cmd_sensor_collect(zones, (int)HG_ARRAYSIZE(zones), &zone_count);

    if (argc >= 3) {
        int wanted = 0;
        if (!cmd_parse_int(argv[2], &wanted)) {
            cmd_printf(L"show sensors: '%ls' is not a sensor number", argv[2]);
            return;
        }
        if (wanted < 1 || wanted > total) {
            cmd_printf(L"show sensors: no sensor %d (there are %d)", wanted, total);
            return;
        }
        WCHAR line[HG_MAX_STR];
        cmd_sensor_line(wanted, zones, zone_count, line, HG_ARRAYSIZE(line));
        cmd_printf(L"%3d  %ls", wanted, line);
        return;
    }

    if (zone_count <= 0)
        commandbox_print(L"no thermal zones - this machine's firmware exposes none");
    for (int i = 1; i <= total; ++i) {
        WCHAR line[HG_MAX_STR];
        cmd_sensor_line(i, zones, zone_count, line, HG_ARRAYSIZE(line));
        cmd_printf(L"%3d  %ls", i, line);
    }
}

/* The displays, and the preview window each one can have. With a number this
 * toggles that preview rather than printing anything, which is the same switch
 * the display's own submenu in the O menu holds - saying it twice in two places
 * would be two things to keep in step. */
static void cmd_show_monitors(int argc, WCHAR *argv[])
{
    if (hg_g_monitor_count <= 0) {
        commandbox_print(L"no displays reported");
        return;
    }

    if (argc >= 3) {
        int wanted = 0;
        if (!cmd_parse_int(argv[2], &wanted)) {
            cmd_printf(L"show monitor: '%ls' is not a display number", argv[2]);
            return;
        }
        int index = cmd_monitor_index_by_number(wanted);
        if (index < 0) {
            cmd_printf(L"show monitor: no display %d", wanted);
            return;
        }

        toggle_monitor_window(index);

        WCHAR described[HG_MAX_STR];
        hg_describe_monitor(hg_g_monitors[index].name, described, HG_ARRAYSIZE(described));
        cmd_printf(L"display %d preview %ls: %ls", wanted, hg_g_monitors[index].active ? L"on" : L"off", described);
        return;
    }

    for (int i = 0; i < hg_g_monitor_count; ++i) {
        WCHAR described[HG_MAX_STR];
        hg_describe_monitor(hg_g_monitors[i].name, described, HG_ARRAYSIZE(described));
        const RECT *rc = &hg_g_monitors[i].rcMonitor;
        cmd_printf(L"%3d  %-8ls %4dx%-4d at %5d,%-5d  %ls", cmd_monitor_number_of(i),
                   hg_g_monitors[i].active ? L"preview" : L"", rc->right - rc->left, rc->bottom - rc->top, rc->left,
                   rc->top, described);
    }
    commandbox_print(L"     's m <n>' turns that display's preview window on, or off again");
}

/* Every number that can be written, what it is now, and what it may be. */
static void cmd_show_values(void)
{
    int count = hg_value_count();
    for (int i = 1; i <= count; ++i) {
        HgValueInfo info;
        int value = 0;
        if (!hg_value_info(i, &info) || !hg_value_get(i, &value))
            continue;
        cmd_printf(L"%3d  %-17ls %4d%-2ls  (%d-%d%ls)  %ls", i, info.name, value, info.unit, info.min, info.max,
                   info.unit, info.about);
    }
    commandbox_print(L"     set one with 'write value <number|name> <value>'");
}

/* Window, then function, then the chords - the three levels the settings file
 * and the settings window use, printed in the same order so the listing and the
 * file read as one thing. */
static void cmd_show_keys(const WCHAR *context_word)
{
    int only = context_word ? hg_key_context_find(context_word) : 0;
    if (context_word && !only) {
        cmd_printf(L"show key: no window called '%ls'", context_word);
        return;
    }

    int last_context = 0;
    for (int action = 1; action <= hg_key_action_count(); ++action) {
        HgKeyActionInfo info;
        if (!hg_key_action_info(action, &info))
            continue;
        if (only && info.context != only)
            continue;

        if (info.context != last_context) {
            last_context = info.context;
            commandbox_print(L"");
            cmd_printf(L"[%ls] %ls", hg_key_context_name(info.context), hg_key_context_summary(info.context));
        }

        WCHAR keys[256];
        hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
        cmd_printf(L"%3d  %-14ls %-26ls %ls", action, info.name, keys, info.label);
    }

    commandbox_print(L"");
    commandbox_print(L"     bind <window> <function> <key>, unbind to take one away");
}

/* What the floater is actually painted from, at this instant.
 *
 * The widget going faint is a report about colour, opacity, or a paint that
 * did not happen, and those three look identical from across the room. This
 * prints all of them so the answer is read rather than guessed: run it while
 * the floater looks wrong. */
static void cmd_show_theme(void)
{
    cmd_printf(L"theme       dark=%ls  high-contrast=%ls", hg_g_is_dark_mode ? L"yes" : L"no",
               hg_g_is_high_contrast ? L"yes" : L"no");
    cmd_printf(L"            the widgets invert the system theme on purpose: a dark system");
    cmd_printf(L"            paints them from the light scheme, and a light system from the");
    cmd_printf(L"            custom palette in [colors].");

    const color_scheme_t *s = &hg_g_color_scheme_selected;
    cmd_printf(L"in use      bg=#%02X%02X%02X  text=#%02X%02X%02X  border=#%02X%02X%02X", GetRValue(s->bg),
               GetGValue(s->bg), GetBValue(s->bg), GetRValue(s->text), GetGValue(s->text), GetBValue(s->text),
               GetRValue(s->border), GetGValue(s->border), GetBValue(s->border));
    cmd_printf(L"light       bg=#%02X%02X%02X   (GetSysColor COLOR_WINDOW)", GetRValue(hg_g_color_scheme_light.bg),
               GetGValue(hg_g_color_scheme_light.bg), GetBValue(hg_g_color_scheme_light.bg));
    cmd_printf(L"custom      bg=#%02X%02X%02X   ([colors] in config.ini)", GetRValue(hg_g_color_scheme_dark.bg),
               GetGValue(hg_g_color_scheme_dark.bg), GetBValue(hg_g_color_scheme_dark.bg));

    /* The opacity the program believes in, and the one the window is actually
     * wearing - they are different things, and only the second is what the eye
     * sees. */
    cmd_printf(L"alpha       floater=%d (%d%%)  taskbox=%d  commandbox=%d", (int)hg_g_floater_alpha,
               ((int)hg_g_floater_alpha * 100 + 127) / 255, (int)hg_g_taskbox_alpha,
               (int)hg_g_commandbox_alpha);

    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        COLORREF key = 0;
        BYTE actual = 0;
        DWORD flags = 0;
        if (GetLayeredWindowAttributes(hg_g_floater_wnd, &key, &actual, &flags)) {
            cmd_printf(L"            the floater window reports alpha=%d flags=0x%X%ls", (int)actual,
                       (unsigned)flags,
                       (actual == hg_g_floater_alpha) ? L"" : L"   <- does not match the setting");
        } else {
            cmd_printf(L"            the floater window reports no layered attributes at all");
        }

        RECT rc = {0, 0, 0, 0};
        GetWindowRect(hg_g_floater_wnd, &rc);
        cmd_printf(L"floater     %ldx%ld at %ld,%ld  visible=%ls", rc.right - rc.left, rc.bottom - rc.top,
                   rc.left, rc.top, IsWindowVisible(hg_g_floater_wnd) ? L"yes" : L"no");
    }

    /* A paint that quietly gave up is usually a handle it could not get. */
    cmd_printf(L"gdi         %u objects in this process (the per-process ceiling is 10000)",
               (unsigned)GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS));
    cmd_printf(L"fonts       clock=%ls date=%ls", hg_g_floater_time_font ? L"ok" : L"MISSING",
               hg_g_floater_date_font ? L"ok" : L"MISSING");
}

static void cmd_show_options(void)
{
    int count = hg_option_count();
    for (int i = 1; i <= count; ++i) {
        HgOptionInfo info;
        if (!hg_option_info(i, &info))
            continue;
        const WCHAR *state = !info.available
                                 ? (info.unavailable_note ? info.unavailable_note : L"unavailable")
                                 : (hg_option_get(i) ? L"on" : L"off");
        cmd_printf(L"%3d  %-15ls %-18ls %ls", i, info.name, state, info.about);
    }
    commandbox_print(L"     set one with 'write option <number|name> <on|off|toggle>'");
}

static void cmd_show(int argc, WCHAR *argv[])
{
    if (argc < 2) {
        cmd_list_windows(FALSE);
        return;
    }
    if (cmd_word_is(argv[1], L"windows", L"w")) {
        cmd_list_windows(argc >= 3 && cmd_word_is(argv[2], L"class", L"c"));
    } else if (cmd_word_is(argv[1], L"resize", L"r")) {
        cmd_list_resize();
    } else if (cmd_word_is(argv[1], L"shortcut", L"c") || cmd_word_is(argv[1], L"shortcuts", NULL)) {
        cmd_list_shortcuts();
    } else if (cmd_word_is(argv[1], L"note", L"n") || cmd_word_is(argv[1], L"notes", NULL)) {
        cmd_list_notes();
    } else if (cmd_word_is(argv[1], L"sensors", L"s") || cmd_word_is(argv[1], L"sensor", NULL) ||
               cmd_word_is(argv[1], L"temp", NULL)) {
        cmd_show_sensors(argc, argv);
    } else if (cmd_word_is(argv[1], L"monitor", L"m") || cmd_word_is(argv[1], L"monitors", NULL) ||
               cmd_word_is(argv[1], L"display", NULL)) {
        cmd_show_monitors(argc, argv);
    } else if (cmd_word_is(argv[1], L"value", L"v") || cmd_word_is(argv[1], L"values", NULL)) {
        cmd_show_values();
    } else if (cmd_word_is(argv[1], L"option", L"o") || cmd_word_is(argv[1], L"options", NULL)) {
        cmd_show_options();
    } else if (cmd_word_is(argv[1], L"key", L"k") || cmd_word_is(argv[1], L"keys", NULL)) {
        cmd_show_keys((argc >= 3) ? argv[2] : NULL);
    } else if (cmd_word_is(argv[1], L"theme", NULL)) {
        cmd_show_theme();
    } else if (cmd_word_is(argv[1], L"tabsinfo", NULL)) {
        /* The tab reader's own numbers: per window, whether the scoped read
         * or a full discovery answered and what it cost; in total, what was
         * queued, dropped and slow. A diagnostic, so no shorthand - it is not
         * something anyone reaches for in a hurry. */
        hg_tabs_report(commandbox_print);
    } else if (cmd_word_is(argv[1], L"tabs", L"t") || cmd_word_is(argv[1], L"tab", NULL)) {
        cmd_list_tabs();
    } else {
        cmd_printf(
            L"show: unknown kind '%ls' (windows, resize, shortcut, note, monitor, sensors, tabs, value, "
            L"option, key, theme)",
                   argv[1]);
    }
}

/* ------------------------------------------------------- write, config, b */

static void cmd_write_option(int argc, WCHAR *argv[])
{
    if (argc < 3) {
        commandbox_print(L"write option <number|name> <on|off|toggle> - 'show option' lists them:");
        cmd_show_options();
        return;
    }

    int number = 0;
    if (!cmd_parse_int(argv[2], &number))
        number = hg_option_find(argv[2]);

    HgOptionInfo info;
    if (!hg_option_info(number, &info)) {
        cmd_printf(L"write option: no option called '%ls' (see 'show option')", argv[2]);
        return;
    }

    /* No value given means toggle. It is what the menu item does, and the
     * shortest thing to type is the thing most often wanted. */
    BOOL wanted = hg_option_get(number) ? FALSE : TRUE;
    if (argc >= 4 && !hg_option_parse_value(number, argv[3], &wanted)) {
        cmd_printf(L"write option: '%ls' is not on, off or toggle", argv[3]);
        return;
    }

    const WCHAR *message = NULL;
    if (!hg_option_set(number, wanted, &message)) {
        cmd_printf(L"write option: %ls could not be set%ls%ls", info.name, message ? L" - " : L"",
                   message ? message : L"");
        return;
    }

    cmd_printf(L"%ls  %ls", info.name, hg_option_get(number) ? L"on" : L"off");
    if (message)
        commandbox_print(message);
}

static void cmd_write(int argc, WCHAR *argv[])
{
    if (argc >= 2 && (cmd_word_is(argv[1], L"option", L"o") || cmd_word_is(argv[1], L"options", NULL))) {
        cmd_write_option(argc, argv);
        return;
    }

    if (argc < 2 || !(cmd_word_is(argv[1], L"value", L"v") || cmd_word_is(argv[1], L"values", NULL))) {
        commandbox_print(L"write: 'write value' or 'write option', as in 'w v brightness 60' or 'w o tabs on'");
        return;
    }
    if (argc < 4) {
        commandbox_print(L"write value <number|name> <value> - 'show value' lists them:");
        cmd_show_values();
        return;
    }

    int number = 0;
    if (!cmd_parse_int(argv[2], &number))
        number = hg_value_find(argv[2]);

    HgValueInfo info;
    if (!hg_value_info(number, &info)) {
        cmd_printf(L"write value: no value called '%ls' (see 'show value')", argv[2]);
        return;
    }

    int wanted = 0;
    if (!cmd_parse_int(argv[3], &wanted)) {
        cmd_printf(L"write value: '%ls' is not a number", argv[3]);
        return;
    }

    BOOL persisted = FALSE;
    if (!hg_value_set(number, wanted, &persisted)) {
        cmd_printf(L"write value: %ls could not be set", info.name);
        return;
    }

    /* Read it back rather than echoing what was asked for: it may have been
     * clamped, and a monitor may not have taken the value it was given. */
    int now = 0;
    hg_value_get(number, &now);
    if (persisted)
        cmd_printf(L"%ls = %d%ls, saved", info.name, now, info.unit);
    else
        cmd_printf(L"%ls = %d%ls (not saved - the machine keeps this one)", info.name, now, info.unit);
}

static void cmd_config(void)
{
    /* notepad rather than ShellExecute on the .ini itself: .ini is not always
     * associated with an editor, and on some machines opening it launches
     * something that is not one. */
    /* Quoted: the path runs through the user's profile directory, and a user
     * name with a space in it would otherwise reach Notepad as two arguments. */
    WCHAR argument[HG_MAX_PATH + 4];
    hellgates_wsprintf(argument, HG_ARRAYSIZE(argument), L"\"%ls\"", hg_g_config_path);
    if ((INT_PTR)ShellExecuteW(NULL, L"open", L"notepad.exe", argument, NULL, SW_SHOWNORMAL) <= 32) {
        commandbox_print(L"config: could not open Notepad");
        return;
    }
    cmd_printf(L"config: %ls", hg_g_config_path);
}

static BOOL cmd_clipboard(int argc, WCHAR *argv[])
{
    int count = hg_clip_count();

    if (argc >= 2) {
        int number = 0;
        if (!cmd_parse_int(argv[1], &number)) {
            cmd_printf(L"clipboard: '%ls' is not an entry number", argv[1]);
            return FALSE;
        }
        WCHAR row[HG_CLIP_ROW_CCH_PUBLIC];
        if (!hg_clip_row(number, row, HG_ARRAYSIZE(row))) {
            cmd_printf(L"clipboard: no entry %d (there %ls %d)", number, (count == 1) ? L"is" : L"are", count);
            return FALSE;
        }
        hg_clip_take(number);
        cmd_printf(L"clipboard 1: %ls", row);
        return FALSE;
    }

    if (count <= 0) {
        commandbox_print(L"clipboard: nothing copied yet this session");
        return FALSE;
    }
    for (int i = 1; i <= count; ++i) {
        WCHAR row[HG_CLIP_ROW_CCH_PUBLIC];
        if (hg_clip_row(i, row, HG_ARRAYSIZE(row)))
            cmd_printf(L"%3d  %ls", i, row);
    }
    cmd_printf(L"     keeping at most %d; 'b <n>' makes one of them current", hg_clip_max());
    return FALSE;
}

/* TRUE when a window really was brought forward, so the caller can leave the
 * keyboard where this command just put it. */
static BOOL cmd_go(int argc, WCHAR *argv[])
{
    int number = 0;

    /* `go tab <n>` takes the number from `show tabs`, which spans windows -
     * so this raises that tab's window and selects the tab in it. */
    if (argc >= 2 && (cmd_word_is(argv[1], L"tab", L"t") || cmd_word_is(argv[1], L"tabs", NULL))) {
        if (argc < 3 || !cmd_parse_int(argv[2], &number)) {
            commandbox_print(L"go tab: needs a number from 'show tabs', as in 'go tab 1'");
            return FALSE;
        }
        if (number < 1 || number > s_tab_list_count) {
            cmd_printf(L"go tab: no tab %d - run 'show tabs' first", number);
            return FALSE;
        }
        HWND hwnd = s_tab_list[number - 1].hwnd;
        if (!IsWindow(hwnd)) {
            commandbox_print(L"go tab: that window has gone - run 'show tabs' again");
            return FALSE;
        }
        if (!hg_tabs_activate(hwnd, s_tab_list[number - 1].tab_index)) {
            cmd_printf(L"go tab %d: the window came forward, but the tab could not be selected", number);
            return TRUE;
        }
        cmd_printf(L"go tab %d", number);
        return TRUE;
    }

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

/* The numbers printed are the ones `show note` gives, not 1, 2, 3 among the
 * matches, so a result can be handed straight back to `note`. */
static void cmd_find_notes(const WCHAR *needle)
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

static void cmd_find(int argc, WCHAR *argv[], const WCHAR *line)
{
    BOOL windows = (argc >= 2) && cmd_word_is(argv[1], L"windows", L"w");
    BOOL notes = (argc >= 2) && (cmd_word_is(argv[1], L"note", L"n") || cmd_word_is(argv[1], L"notes", NULL));
    if (argc < 3 || (!windows && !notes)) {
        commandbox_print(L"find: windows or note, as in 'find windows notepad' or 'f n groceries'");
        return;
    }

    const WCHAR *needle = cmd_tail(line, 2);
    if (!needle || !*needle) {
        commandbox_print(L"find: needs something to look for");
        return;
    }

    if (notes) {
        cmd_find_notes(needle);
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

    if (cmd_word_is(argv[1], L"new", L"n")) {
        if (!hg_note_command_new()) {
            commandbox_print(L"note: could not make a note");
            return FALSE;
        }
        commandbox_print(L"note: a new note is open");
        return TRUE;
    }

    int number = 0;
    if (!cmd_parse_int(argv[1], &number)) {
        cmd_printf(L"note: '%ls' is not a note number - see 'show note'", argv[1]);
        return FALSE;
    }

    HgNoteBrief brief;
    if (!hg_note_command_brief(number, &brief)) {
        cmd_printf(L"note: no note %d (see 'show note')", number);
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

/* bind <window> <function> <chord>, and its opposite. The window comes first
 * because the same function name lives in several of them - "notes" is a
 * different row in the floater and in the taskbox, and a command that guessed
 * between them would rebind the wrong one half the time. */
static void cmd_bind(int argc, WCHAR *argv[], BOOL removing)
{
    const WCHAR *verb = removing ? L"unbind" : L"bind";

    if (argc < 3) {
        cmd_printf(L"%ls <window> <function> [key] - 'show key' lists them:", verb);
        cmd_show_keys(NULL);
        return;
    }

    int context = hg_key_context_find(argv[1]);
    if (!context) {
        cmd_printf(L"%ls: no window called '%ls' (system, widget, floater, taskbox)", verb, argv[1]);
        return;
    }

    int action = hg_key_action_find(context, argv[2]);
    HgKeyActionInfo info;
    if (!action || !hg_key_action_info(action, &info)) {
        cmd_printf(L"%ls: the %ls has no function called '%ls' (see 'show key %ls')", verb,
                   hg_key_context_name(context), argv[2], hg_key_context_name(context));
        return;
    }

    WCHAR keys[256];

    /* No chord given: bind says what is bound, unbind takes them all away.
     * Clearing every key is a thing worth being able to say - a function with
     * a button and a menu entry does not have to own a chord as well. */
    if (argc < 4) {
        if (!removing) {
            hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
            cmd_printf(L"%ls %ls  %ls", hg_key_context_name(context), info.name, keys);
            return;
        }
        hg_key_clear(action);
        cmd_printf(L"%ls %ls  no key", hg_key_context_name(context), info.name);
        return;
    }

    if (cmd_word_is(argv[3], L"default", NULL)) {
        hg_key_reset(action);
        hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
        cmd_printf(L"%ls %ls  %ls", hg_key_context_name(context), info.name, keys);
        return;
    }

    HgChord chord;
    if (removing) {
        if (!hg_key_remove(action, argv[3])) {
            cmd_printf(L"unbind: %ls is not one of %ls's keys", argv[3], info.name);
            return;
        }
    } else {
        int conflict = 0;
        if (!hg_key_add(action, argv[3], &conflict)) {
            HgKeyActionInfo other;
            if (conflict && hg_key_action_info(conflict, &other)) {
                cmd_printf(L"bind: %ls already runs '%ls' in the %ls - unbind it first", argv[3], other.name,
                           hg_key_context_name(context));
            } else if (hg_key_parse_chord(argv[3], &chord) && hg_key_is_navigation(chord)) {
                cmd_printf(L"bind: %ls moves the selection - the arrows and WASD are how a window is walked, "
                           L"so they take no binding of their own",
                           argv[3]);
            } else if (hg_key_binding_count(action) >= HG_KEY_MAX_BINDINGS) {
                cmd_printf(L"bind: %ls already has %d keys, which is the most one function takes", info.name,
                           HG_KEY_MAX_BINDINGS);
            } else {
                cmd_printf(L"bind: '%ls' is not a key - try Ctrl+N, Alt+F4, F2, Esc, Space", argv[3]);
            }
            return;
        }
    }

    hg_key_bindings_text(action, keys, HG_ARRAYSIZE(keys));
    cmd_printf(L"%ls %ls  %ls", hg_key_context_name(context), info.name, keys);
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
    } else if (cmd_word_is(argv[0], L"show", L"s")) {
        cmd_show(argc, argv);
    } else if (cmd_word_is(argv[0], L"find", L"f")) {
        cmd_find(argc, argv, line);
    } else if (cmd_word_is(argv[0], L"go", NULL)) {
        moved_focus = cmd_go(argc, argv);
    } else if (cmd_word_is(argv[0], L"resize", L"r")) {
        cmd_resize(argc, argv);
    } else if (cmd_word_is(argv[0], L"move", L"m")) {
        cmd_move(argc, argv);
    } else if (cmd_word_is(argv[0], L"note", L"n")) {
        moved_focus = cmd_note(argc, argv);
    } else if (cmd_word_is(argv[0], L"clipboard", L"b")) {
        moved_focus = cmd_clipboard(argc, argv);
    } else if (cmd_word_is(argv[0], L"write", L"w")) {
        cmd_write(argc, argv);
    } else if (cmd_word_is(argv[0], L"settings", L"set")) {
        hg_settings_toggle_window();
        commandbox_print(L"settings: the window is open");
    } else if (cmd_word_is(argv[0], L"bind", NULL)) {
        cmd_bind(argc, argv, FALSE);
    } else if (cmd_word_is(argv[0], L"unbind", NULL)) {
        cmd_bind(argc, argv, TRUE);
    } else if (cmd_word_is(argv[0], L"config", L"c")) {
        cmd_config();
    } else if (cmd_word_is(argv[0], L"clear", L"cls")) {
        /* The transcript only. The history is what you typed and outlives
         * what it printed; `write value history-max 0` is how you drop that. */
        commandbox_clear();
    } else {
        cmd_printf(L"unknown command '%ls' - type help", argv[0]);
    }

    return moved_focus;
}
