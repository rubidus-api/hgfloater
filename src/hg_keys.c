/* Window - function - keys.
 *
 * The table below is the whole list of rebindable functions. Everything else in
 * this file is machinery around it: text to chord and back, the settings file,
 * the lookup a window procedure calls, and the two Win32 shapes a binding has
 * to be poured into (RegisterHotKey for the system context, an accelerator
 * table for the widgets).
 *
 * Chords are stored as a virtual key plus modifier bits of our own rather than
 * as MOD_* or FVIRTKEY flags: those two spellings disagree with each other, and
 * both describe a Win32 call rather than the chord a person pressed. The two
 * conversions live at the bottom of this file and nowhere else. */
#include "hg_keys.h"
#include "hg_globals.h"
#include "hg_utils.h"

typedef struct HgKeyActionRow {
    int context;
    const WCHAR *name;
    const WCHAR *label;
    const WCHAR *about;
    const WCHAR *defaults; /* comma separated; L"" means no key by default */
    UINT cmd_id;           /* widget context: the WM_COMMAND it stands for */
    BOOL doc_safe;         /* widget context: also live while typing in a note */
} HgKeyActionRow;

/* clang-format off */
static const HgKeyActionRow hg_key_actions[] = {
    /* -- the one Windows itself carries, so it works with any program in front */
    {HG_KEYCTX_SYSTEM, L"show-taskbox", L"Show or hide the taskbox",
     L"works from inside any program", L"Win+Alt+Space", 0, FALSE},

    /* -- the floater and the taskbox share these, as an accelerator table */
    {HG_KEYCTX_WIDGET, L"quit", L"Quit hgfloater", L"from any of its windows",
     L"Ctrl+Q, Alt+F4", HG_IDM_CLOSE_APP, TRUE},
    {HG_KEYCTX_WIDGET, L"about", L"About", L"the README, in a window", L"F1", HG_IDM_ABOUT, TRUE},
    {HG_KEYCTX_WIDGET, L"reset", L"Reset position, size and opacity",
     L"widgets only, so these are safe to press while typing",
     L"Ctrl+R, Ctrl+Shift+R, F5, Ctrl+0", HG_IDM_RESET_ALL, FALSE},
    {HG_KEYCTX_WIDGET, L"font-up", L"Larger icons and text", L"the widget font, one step up",
     L"Ctrl+Plus, Ctrl+NumPlus", HG_IDM_FONT_UP, FALSE},
    {HG_KEYCTX_WIDGET, L"font-down", L"Smaller icons and text", L"the widget font, one step down",
     L"Ctrl+Minus, Ctrl+NumMinus", HG_IDM_FONT_DOWN, FALSE},

    /* -- the floater */
    {HG_KEYCTX_FLOATER, L"open-taskbox", L"Open the taskbox", L"the same thing a click does",
     L"T", 0, FALSE},
    {HG_KEYCTX_FLOATER, L"command-box", L"Command box", L"the console that drives windows by name",
     L"C, Ctrl+E", 0, FALSE},
    {HG_KEYCTX_FLOATER, L"notes", L"Note list", L"", L"Ctrl+N", 0, FALSE},
    {HG_KEYCTX_FLOATER, L"clipboard", L"Clipboard history", L"", L"Ctrl+L", 0, FALSE},
    {HG_KEYCTX_FLOATER, L"menu", L"The floater's menu", L"the same menu the O button opens",
     L"F2", 0, FALSE},

    /* -- the taskbox */
    {HG_KEYCTX_TASKBOX, L"command-box", L"Command box", L"", L"C, Ctrl+E", 0, FALSE},
    {HG_KEYCTX_TASKBOX, L"notes", L"Note list", L"", L"N, Ctrl+N", 0, FALSE},
    {HG_KEYCTX_TASKBOX, L"clipboard", L"Clipboard history", L"", L"Ctrl+L", 0, FALSE},
    {HG_KEYCTX_TASKBOX, L"hide", L"Fold back into the floater",
     L"and re-read the shortcuts folder", L"Esc", 0, FALSE},
};
/* clang-format on */

static HgChord s_bindings[HG_ARRAYSIZE(hg_key_actions)][HG_KEY_MAX_BINDINGS];
static int s_binding_count[HG_ARRAYSIZE(hg_key_actions)];
static BOOL s_loaded = FALSE;

static HACCEL s_accel_widget = NULL;
static HACCEL s_accel_doc = NULL;

/* ------------------------------------------------------------------ contexts */

typedef struct HgKeyContextRow {
    int context;
    const WCHAR *name;
    const WCHAR *summary;
} HgKeyContextRow;

static const HgKeyContextRow hg_key_contexts[] = {
    {HG_KEYCTX_SYSTEM, L"system", L"registered with Windows: works from inside any program"},
    {HG_KEYCTX_WIDGET, L"widget", L"the floater and the taskbox, whichever has the keyboard"},
    {HG_KEYCTX_FLOATER, L"floater", L"the small clock widget"},
    {HG_KEYCTX_TASKBOX, L"taskbox", L"the dashboard"},
};

const WCHAR *hg_key_context_name(int context)
{
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_key_contexts); ++i) {
        if (hg_key_contexts[i].context == context)
            return hg_key_contexts[i].name;
    }
    return L"?";
}

const WCHAR *hg_key_context_summary(int context)
{
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_key_contexts); ++i) {
        if (hg_key_contexts[i].context == context)
            return hg_key_contexts[i].summary;
    }
    return L"";
}

int hg_key_context_find(const WCHAR *name)
{
    if (!name || !*name)
        return 0;

    size_t len = wcslen(name);
    int match = 0;
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_key_contexts); ++i) {
        if (_wcsnicmp(hg_key_contexts[i].name, name, len) != 0)
            continue;
        if (wcslen(hg_key_contexts[i].name) == len)
            return hg_key_contexts[i].context;
        if (match)
            return 0; /* ambiguous: another letter is cheaper than a wrong guess */
        match = hg_key_contexts[i].context;
    }
    return match;
}

/* ------------------------------------------------------------- key name table */

typedef struct HgKeyNameRow {
    const WCHAR *name;
    UINT vk;
} HgKeyNameRow;

/* The first spelling of a key is the one printed back; the rest are accepted.
 * A name is worth having for every key that is not a letter or a digit, because
 * "Ctrl+0xBB" is not something anyone should have to read or type. */
static const HgKeyNameRow hg_key_names[] = {
    {L"Space", VK_SPACE},      {L"Enter", VK_RETURN},   {L"Return", VK_RETURN},
    {L"Esc", VK_ESCAPE},       {L"Escape", VK_ESCAPE},  {L"Tab", VK_TAB},
    {L"Backspace", VK_BACK},   {L"Insert", VK_INSERT},  {L"Ins", VK_INSERT},
    {L"Delete", VK_DELETE},    {L"Del", VK_DELETE},     {L"Home", VK_HOME},
    {L"End", VK_END},          {L"PageUp", VK_PRIOR},   {L"PageDown", VK_NEXT},
    {L"Up", VK_UP},            {L"Down", VK_DOWN},      {L"Left", VK_LEFT},
    {L"Right", VK_RIGHT},      {L"Plus", VK_OEM_PLUS},  {L"Minus", VK_OEM_MINUS},
    {L"NumPlus", VK_ADD},      {L"NumMinus", VK_SUBTRACT}, {L"Comma", VK_OEM_COMMA},
    {L"Period", VK_OEM_PERIOD}, {L"Pause", VK_PAUSE},   {L"PrintScreen", VK_SNAPSHOT},
};

static BOOL key_name_to_vk(const WCHAR *word, UINT *out_vk)
{
    if (!word || !*word)
        return FALSE;

    /* A single letter or digit is itself: 'T' is VK 'T'. */
    if (word[1] == L'\0') {
        WCHAR c = (WCHAR)towupper(word[0]);
        if ((c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9')) {
            *out_vk = (UINT)c;
            return TRUE;
        }
    }

    if ((word[0] == L'F' || word[0] == L'f') && word[1] >= L'1' && word[1] <= L'9') {
        int n = 0;
        for (const WCHAR *p = word + 1; *p; ++p) {
            if (*p < L'0' || *p > L'9')
                return FALSE;
            n = n * 10 + (int)(*p - L'0');
            if (n > 24)
                return FALSE;
        }
        *out_vk = (UINT)(VK_F1 + n - 1);
        return TRUE;
    }

    for (size_t i = 0; i < HG_ARRAYSIZE(hg_key_names); ++i) {
        if (_wcsicmp(hg_key_names[i].name, word) == 0) {
            *out_vk = hg_key_names[i].vk;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL key_vk_to_name(UINT vk, WCHAR *out, size_t out_cch)
{
    if (!out || out_cch == 0)
        return FALSE;

    if ((vk >= L'A' && vk <= L'Z') || (vk >= L'0' && vk <= L'9')) {
        WCHAR text[2] = {(WCHAR)vk, L'\0'};
        return SUCCEEDED(StringCchCopyW(out, out_cch, text));
    }
    if (vk >= VK_F1 && vk <= VK_F24)
        return SUCCEEDED(StringCchPrintfW(out, out_cch, L"F%u", vk - VK_F1 + 1u));

    for (size_t i = 0; i < HG_ARRAYSIZE(hg_key_names); ++i) {
        if (hg_key_names[i].vk == vk)
            return SUCCEEDED(StringCchCopyW(out, out_cch, hg_key_names[i].name));
    }
    return SUCCEEDED(StringCchPrintfW(out, out_cch, L"0x%02X", vk));
}

BOOL hg_key_parse_chord(const WCHAR *text, HgChord *out)
{
    if (!text || !out)
        return FALSE;

    WCHAR buffer[64];
    if (FAILED(StringCchCopyW(buffer, HG_ARRAYSIZE(buffer), text)))
        return FALSE;

    HgChord chord = {0, 0};
    WCHAR *p = buffer;

    /* Trim: a chord typed with spaces around it is the same chord. */
    while (*p == L' ' || *p == L'\t')
        ++p;
    size_t len = wcslen(p);
    while (len > 0 && (p[len - 1] == L' ' || p[len - 1] == L'\t'))
        p[--len] = L'\0';
    if (!*p)
        return FALSE;

    for (;;) {
        WCHAR *plus = wcschr(p, L'+');
        /* A lone '+' is the key itself, not a separator: "Ctrl++" is Ctrl+Plus
         * spelled the way a hand types it. */
        if (plus == p && p[1] == L'\0')
            plus = NULL;
        if (!plus)
            break;

        *plus = L'\0';
        if (_wcsicmp(p, L"ctrl") == 0 || _wcsicmp(p, L"control") == 0)
            chord.mods |= HG_KMOD_CTRL;
        else if (_wcsicmp(p, L"alt") == 0)
            chord.mods |= HG_KMOD_ALT;
        else if (_wcsicmp(p, L"shift") == 0)
            chord.mods |= HG_KMOD_SHIFT;
        else if (_wcsicmp(p, L"win") == 0 || _wcsicmp(p, L"windows") == 0)
            chord.mods |= HG_KMOD_WIN;
        else
            return FALSE;
        p = plus + 1;
        while (*p == L' ')
            ++p;
    }

    if (wcscmp(p, L"+") == 0) {
        chord.vk = VK_OEM_PLUS;
    } else if (!key_name_to_vk(p, &chord.vk)) {
        return FALSE;
    }

    *out = chord;
    return TRUE;
}

BOOL hg_key_chord_text(HgChord chord, WCHAR *out, size_t out_cch)
{
    if (!out || out_cch == 0)
        return FALSE;

    WCHAR key[16];
    if (!key_vk_to_name(chord.vk, key, HG_ARRAYSIZE(key)))
        return FALSE;

    /* Ctrl, Alt, Shift, Win - in the order Windows itself prints them, so a
     * chord read here matches the one printed in any other program's menu. */
    return SUCCEEDED(StringCchPrintfW(out, out_cch, L"%ls%ls%ls%ls%ls",
                                      (chord.mods & HG_KMOD_CTRL) ? L"Ctrl+" : L"",
                                      (chord.mods & HG_KMOD_WIN) ? L"Win+" : L"",
                                      (chord.mods & HG_KMOD_ALT) ? L"Alt+" : L"",
                                      (chord.mods & HG_KMOD_SHIFT) ? L"Shift+" : L"", key));
}

/* --------------------------------------------------------------- the table */

int hg_key_action_count(void)
{
    return (int)HG_ARRAYSIZE(hg_key_actions);
}

BOOL hg_key_action_info(int action, HgKeyActionInfo *out)
{
    if (!out || action < 1 || action > hg_key_action_count())
        return FALSE;
    const HgKeyActionRow *row = &hg_key_actions[action - 1];
    out->context = row->context;
    out->name = row->name;
    out->label = row->label;
    out->about = row->about;
    return TRUE;
}

int hg_key_action_find(int context, const WCHAR *name)
{
    if (!name || !*name)
        return 0;

    size_t len = wcslen(name);
    int match = 0;
    for (int i = 0; i < hg_key_action_count(); ++i) {
        const HgKeyActionRow *row = &hg_key_actions[i];
        if (context && row->context != context)
            continue;
        if (_wcsnicmp(row->name, name, len) != 0)
            continue;
        if (wcslen(row->name) == len && context)
            return i + 1; /* exact beats prefix, once a context has been named */
        if (match)
            return 0;
        match = i + 1;
    }
    return match;
}

/* ------------------------------------------------------------ the settings file */

static void keys_section_name(int context, WCHAR *out, size_t out_cch)
{
    StringCchPrintfW(out, out_cch, L"keys.%ls", hg_key_context_name(context));
}

static void keys_write_action(int action)
{
    const HgKeyActionRow *row = &hg_key_actions[action - 1];
    WCHAR section[32];
    keys_section_name(row->context, section, HG_ARRAYSIZE(section));

    WCHAR text[256];
    if (!hg_key_bindings_text(action, text, HG_ARRAYSIZE(text)))
        return;
    /* An action with no keys is written as an empty value rather than left out:
     * a missing line means "whatever the program's default is", and that is not
     * what someone who has just cleared a binding meant. */
    if (s_binding_count[action - 1] == 0)
        text[0] = L'\0';

    WritePrivateProfileStringW(section, row->name, text, hg_g_config_path);
}

static void keys_parse_list(int action, const WCHAR *list)
{
    s_binding_count[action - 1] = 0;
    if (!list)
        return;

    WCHAR buffer[256];
    if (FAILED(StringCchCopyW(buffer, HG_ARRAYSIZE(buffer), list)))
        return;

    WCHAR *p = buffer;
    while (*p) {
        WCHAR *comma = wcschr(p, L',');
        if (comma)
            *comma = L'\0';

        HgChord chord;
        if (hg_key_parse_chord(p, &chord) && s_binding_count[action - 1] < HG_KEY_MAX_BINDINGS) {
            BOOL duplicate = FALSE;
            for (int i = 0; i < s_binding_count[action - 1]; ++i) {
                if (s_bindings[action - 1][i].vk == chord.vk && s_bindings[action - 1][i].mods == chord.mods)
                    duplicate = TRUE;
            }
            if (!duplicate)
                s_bindings[action - 1][s_binding_count[action - 1]++] = chord;
        }

        if (!comma)
            break;
        p = comma + 1;
    }
}

void hg_keys_load(void)
{
    s_loaded = TRUE;

    for (int action = 1; action <= hg_key_action_count(); ++action) {
        const HgKeyActionRow *row = &hg_key_actions[action - 1];
        WCHAR section[32];
        keys_section_name(row->context, section, HG_ARRAYSIZE(section));

        /* The sentinel separates "no line in the file" from "a line that is
         * deliberately empty" - the first means the default, the second means
         * no key at all, and an empty string cannot say which. */
        WCHAR value[256];
        GetPrivateProfileStringW(section, row->name, L"\x1", value, (DWORD)HG_ARRAYSIZE(value),
                                 hg_g_config_path);
        keys_parse_list(action, (value[0] == L'\x1' && value[1] == L'\0') ? row->defaults : value);
    }

    /* One migration: the global hotkey used to live in [hotkeys] as a pair of
     * numbers. Anyone who changed it there keeps their chord. */
    int show = hg_key_action_find(HG_KEYCTX_SYSTEM, L"show-taskbox");
    if (show) {
        WCHAR probe[64];
        WCHAR section[32];
        keys_section_name(HG_KEYCTX_SYSTEM, section, HG_ARRAYSIZE(section));
        GetPrivateProfileStringW(section, L"show-taskbox", L"", probe, (DWORD)HG_ARRAYSIZE(probe),
                                 hg_g_config_path);
        UINT legacy_key = GetPrivateProfileIntW(L"hotkeys", L"global_focus_key", 0, hg_g_config_path);
        if (!probe[0] && legacy_key != 0 && legacy_key <= 0xFF) {
            UINT legacy_mods = GetPrivateProfileIntW(L"hotkeys", L"global_focus_modifiers", 0, hg_g_config_path);
            HgChord chord = {legacy_key, 0};
            if (legacy_mods & MOD_CONTROL)
                chord.mods |= HG_KMOD_CTRL;
            if (legacy_mods & MOD_ALT)
                chord.mods |= HG_KMOD_ALT;
            if (legacy_mods & MOD_SHIFT)
                chord.mods |= HG_KMOD_SHIFT;
            if (legacy_mods & MOD_WIN)
                chord.mods |= HG_KMOD_WIN;
            s_bindings[show - 1][0] = chord;
            s_binding_count[show - 1] = 1;
            keys_write_action(show);
        }
    }
}

static void keys_ensure_loaded(void)
{
    if (!s_loaded)
        hg_keys_load();
}

/* ---------------------------------------------------------------- bindings */

int hg_key_binding_count(int action)
{
    if (action < 1 || action > hg_key_action_count())
        return 0;
    keys_ensure_loaded();
    return s_binding_count[action - 1];
}

BOOL hg_key_binding_chord(int action, int index, HgChord *out)
{
    if (!out || index < 0 || index >= hg_key_binding_count(action))
        return FALSE;
    *out = s_bindings[action - 1][index];
    return TRUE;
}

BOOL hg_key_bindings_text(int action, WCHAR *out, size_t out_cch)
{
    if (!out || out_cch == 0)
        return FALSE;

    int count = hg_key_binding_count(action);
    if (count == 0)
        return SUCCEEDED(StringCchCopyW(out, out_cch, L"-"));

    out[0] = L'\0';
    for (int i = 0; i < count; ++i) {
        WCHAR text[48];
        if (!hg_key_chord_text(s_bindings[action - 1][i], text, HG_ARRAYSIZE(text)))
            continue;
        if (i > 0)
            StringCchCatW(out, out_cch, L", ");
        StringCchCatW(out, out_cch, text);
    }
    return TRUE;
}

BOOL hg_key_add(int action, const WCHAR *chord_text, int *out_conflict)
{
    if (out_conflict)
        *out_conflict = 0;
    if (action < 1 || action > hg_key_action_count())
        return FALSE;
    keys_ensure_loaded();

    HgChord chord;
    if (!hg_key_parse_chord(chord_text, &chord))
        return FALSE;

    const HgKeyActionRow *row = &hg_key_actions[action - 1];

    /* The Windows key is the system context's alone. Everywhere else the chord
     * never arrives: the shell takes it before any window sees a key. */
    if ((chord.mods & HG_KMOD_WIN) && row->context != HG_KEYCTX_SYSTEM)
        return FALSE;

    for (int other = 1; other <= hg_key_action_count(); ++other) {
        if (hg_key_actions[other - 1].context != row->context)
            continue;
        for (int i = 0; i < s_binding_count[other - 1]; ++i) {
            if (s_bindings[other - 1][i].vk != chord.vk || s_bindings[other - 1][i].mods != chord.mods)
                continue;
            if (other == action)
                return TRUE; /* already there: asking twice is not an error */
            if (out_conflict)
                *out_conflict = other;
            return FALSE;
        }
    }

    if (s_binding_count[action - 1] >= HG_KEY_MAX_BINDINGS)
        return FALSE;

    s_bindings[action - 1][s_binding_count[action - 1]++] = chord;
    keys_write_action(action);
    hg_keys_apply();
    return TRUE;
}

BOOL hg_key_remove(int action, const WCHAR *chord_text)
{
    if (action < 1 || action > hg_key_action_count())
        return FALSE;
    keys_ensure_loaded();

    HgChord chord;
    if (!hg_key_parse_chord(chord_text, &chord))
        return FALSE;

    for (int i = 0; i < s_binding_count[action - 1]; ++i) {
        if (s_bindings[action - 1][i].vk != chord.vk || s_bindings[action - 1][i].mods != chord.mods)
            continue;
        for (int j = i; j + 1 < s_binding_count[action - 1]; ++j)
            s_bindings[action - 1][j] = s_bindings[action - 1][j + 1];
        --s_binding_count[action - 1];
        keys_write_action(action);
        hg_keys_apply();
        return TRUE;
    }
    return FALSE;
}

void hg_key_clear(int action)
{
    if (action < 1 || action > hg_key_action_count())
        return;
    keys_ensure_loaded();
    s_binding_count[action - 1] = 0;
    keys_write_action(action);
    hg_keys_apply();
}

void hg_key_reset(int action)
{
    if (action < 1 || action > hg_key_action_count())
        return;
    keys_ensure_loaded();
    keys_parse_list(action, hg_key_actions[action - 1].defaults);

    /* Removed rather than written: a line that is not there is what "the
     * default" looks like in the file, and it stays right if the default ever
     * changes. */
    const HgKeyActionRow *row = &hg_key_actions[action - 1];
    WCHAR section[32];
    keys_section_name(row->context, section, HG_ARRAYSIZE(section));
    WritePrivateProfileStringW(section, row->name, NULL, hg_g_config_path);
    hg_keys_apply();
}

void hg_key_reset_all(void)
{
    for (int action = 1; action <= hg_key_action_count(); ++action)
        hg_key_reset(action);
}

/* ---------------------------------------------------------------- dispatch */

int hg_key_lookup(int context, UINT vk, UINT mods)
{
    keys_ensure_loaded();
    for (int action = 1; action <= hg_key_action_count(); ++action) {
        if (hg_key_actions[action - 1].context != context)
            continue;
        for (int i = 0; i < s_binding_count[action - 1]; ++i) {
            if (s_bindings[action - 1][i].vk == vk && s_bindings[action - 1][i].mods == mods)
                return action;
        }
    }
    return 0;
}

UINT hg_key_current_mods(void)
{
    UINT mods = 0;
    if (GetKeyState(VK_CONTROL) < 0)
        mods |= HG_KMOD_CTRL;
    if (GetKeyState(VK_MENU) < 0)
        mods |= HG_KMOD_ALT;
    if (GetKeyState(VK_SHIFT) < 0)
        mods |= HG_KMOD_SHIFT;
    if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0)
        mods |= HG_KMOD_WIN;
    return mods;
}

int hg_key_lookup_now(int context, UINT vk)
{
    return hg_key_lookup(context, vk, hg_key_current_mods());
}

/* ------------------------------------------------------------------- Win32 */

static UINT keys_to_mod_flags(UINT mods)
{
    UINT out = 0;
    if (mods & HG_KMOD_CTRL)
        out |= MOD_CONTROL;
    if (mods & HG_KMOD_ALT)
        out |= MOD_ALT;
    if (mods & HG_KMOD_SHIFT)
        out |= MOD_SHIFT;
    if (mods & HG_KMOD_WIN)
        out |= MOD_WIN;
    return out;
}

static BYTE keys_to_accel_flags(UINT mods)
{
    BYTE out = FVIRTKEY;
    if (mods & HG_KMOD_CTRL)
        out |= FCONTROL;
    if (mods & HG_KMOD_ALT)
        out |= FALT;
    if (mods & HG_KMOD_SHIFT)
        out |= FSHIFT;
    return out;
}

static void keys_build_accel_tables(void)
{
    ACCEL widget[HG_KEY_MAX_BINDINGS * 8];
    ACCEL doc[HG_KEY_MAX_BINDINGS * 8];
    int widget_count = 0;
    int doc_count = 0;

    for (int action = 1; action <= hg_key_action_count(); ++action) {
        const HgKeyActionRow *row = &hg_key_actions[action - 1];
        if (row->context != HG_KEYCTX_WIDGET || row->cmd_id == 0)
            continue;
        for (int i = 0; i < s_binding_count[action - 1]; ++i) {
            HgChord chord = s_bindings[action - 1][i];
            if (chord.mods & HG_KMOD_WIN)
                continue; /* an accelerator table cannot express it */
            ACCEL entry;
            entry.fVirt = keys_to_accel_flags(chord.mods);
            entry.key = (WORD)chord.vk;
            entry.cmd = (WORD)row->cmd_id;
            if (widget_count < (int)HG_ARRAYSIZE(widget))
                widget[widget_count++] = entry;
            if (row->doc_safe && doc_count < (int)HG_ARRAYSIZE(doc))
                doc[doc_count++] = entry;
        }
    }

    HACCEL new_widget = widget_count ? CreateAcceleratorTableW(widget, widget_count) : NULL;
    HACCEL new_doc = doc_count ? CreateAcceleratorTableW(doc, doc_count) : NULL;

    if (s_accel_widget)
        DestroyAcceleratorTable(s_accel_widget);
    if (s_accel_doc)
        DestroyAcceleratorTable(s_accel_doc);
    s_accel_widget = new_widget;
    s_accel_doc = new_doc;
}

void hg_keys_shutdown(void)
{
    if (s_accel_widget) {
        DestroyAcceleratorTable(s_accel_widget);
        s_accel_widget = NULL;
    }
    if (s_accel_doc) {
        DestroyAcceleratorTable(s_accel_doc);
        s_accel_doc = NULL;
    }
}

HACCEL hg_keys_accel_table(BOOL document_window)
{
    keys_ensure_loaded();
    if (!s_accel_widget && !s_accel_doc)
        keys_build_accel_tables();
    return document_window ? s_accel_doc : s_accel_widget;
}

void hg_keys_apply(void)
{
    keys_ensure_loaded();
    keys_build_accel_tables();

    /* The system chords, one RegisterHotKey id each. Ids are the binding's
     * place in the list plus one, which is what the floater's WM_HOTKEY reads:
     * every one of them means the same thing, so it does not have to care
     * which chord arrived. */
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        for (int id = 1; id <= HG_KEY_MAX_BINDINGS; ++id)
            UnregisterHotKey(hg_g_floater_wnd, id);
        hg_g_hotkey_registered = FALSE;

        int show = hg_key_action_find(HG_KEYCTX_SYSTEM, L"show-taskbox");
        if (show) {
            for (int i = 0; i < s_binding_count[show - 1]; ++i) {
                HgChord chord = s_bindings[show - 1][i];
                if (RegisterHotKey(hg_g_floater_wnd, i + 1, keys_to_mod_flags(chord.mods) | MOD_NOREPEAT,
                                   chord.vk))
                    hg_g_hotkey_registered = TRUE;
            }
        }
    }
}
