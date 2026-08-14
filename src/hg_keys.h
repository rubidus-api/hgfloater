#ifndef HG_KEYS_H
#define HG_KEYS_H

#include "hg_common.h"

/* Window - function - keys, in one table.
 *
 * Every key this program answers used to be a literal in the window procedure
 * that answered it, which meant the list of keys existed only as prose in the
 * help text, and rebinding one meant editing C. The three levels a person
 * actually thinks in are here instead:
 *
 *   context   the window the key is pressed in (the floater, the taskbox, a
 *             note...), because the same chord means different things in
 *             different windows and always has
 *   action    the function it runs, named once
 *   bindings  the chords that reach it - none, one, or several
 *
 * Zero bindings is a supported answer, not a broken row: a function reachable
 * by button and menu does not have to own a key as well.
 *
 * The file keeps one line per action under a section per context:
 *
 *   [keys.floater]
 *   command-box = C, Ctrl+E
 *   notes       = Ctrl+N
 *   clipboard   =
 *
 * A line that is absent means "the built-in default"; a line that is present
 * and empty means "no key at all". That distinction is the only reason the
 * loader looks for a sentinel rather than an empty string. */

#define HG_KEY_MAX_BINDINGS 4

/* Modifier bits. Deliberately not MOD_* or FCONTROL: those two disagree with
 * each other, and both are about a Win32 call rather than about the chord. */
#define HG_KMOD_CTRL 0x01u
#define HG_KMOD_ALT 0x02u
#define HG_KMOD_SHIFT 0x04u
#define HG_KMOD_WIN 0x08u

enum {
    HG_KEYCTX_SYSTEM = 1, /* registered with Windows: works with any program in front */
    HG_KEYCTX_WIDGET,     /* the floater and the taskbox, whichever has the keyboard */
    HG_KEYCTX_FLOATER,
    HG_KEYCTX_TASKBOX,
    HG_KEYCTX_COMMANDBOX,
    HG_KEYCTX_NOTE,
    HG_KEYCTX_CLIPBOARD,
    HG_KEYCTX_COUNT_PLUS_ONE
};

typedef struct HgChord {
    UINT vk;
    UINT mods; /* HG_KMOD_* */
} HgChord;

typedef struct HgKeyActionInfo {
    int context;
    const WCHAR *name;  /* one word: what `bind` accepts */
    const WCHAR *label; /* what the settings window shows */
    const WCHAR *about; /* one line */
} HgKeyActionInfo;

/* ---- the table itself */
int hg_key_action_count(void);
BOOL hg_key_action_info(int action, HgKeyActionInfo *out);
/* context 0 searches every context. A unique leading prefix is enough; an
 * ambiguous one returns 0 rather than guessing. */
int hg_key_action_find(int context, const WCHAR *name);

const WCHAR *hg_key_context_name(int context);
const WCHAR *hg_key_context_summary(int context);
int hg_key_context_find(const WCHAR *name);

/* ---- bindings */
int hg_key_binding_count(int action);
BOOL hg_key_binding_chord(int action, int index, HgChord *out);
/* One chord as text ("Ctrl+Shift+F1"), or every chord of an action joined by
 * commas - "-" when it has none, so a listing never has a blank column. */
BOOL hg_key_chord_text(HgChord chord, WCHAR *out, size_t out_cch);
BOOL hg_key_bindings_text(int action, WCHAR *out, size_t out_cch);

BOOL hg_key_parse_chord(const WCHAR *text, HgChord *out);

/* The four directions, bare: the arrows and WASD. They move a selection in the
 * taskbox grid, in the settings list and in the tab box - the language of
 * "where am I" rather than a function - and a window whose navigation had been
 * bound to something else could not be walked at all. Held with Ctrl or Alt
 * they are ordinary chords and this answers FALSE. */
BOOL hg_key_is_navigation(HgChord chord);

/* Adds a chord. Fails when the text is not a chord, when the action already has
 * HG_KEY_MAX_BINDINGS of them, or when another action in the same context holds
 * it - out_conflict then names that action, because "already taken" without
 * saying by what is a dead end. */
BOOL hg_key_add(int action, const WCHAR *chord_text, int *out_conflict);
BOOL hg_key_remove(int action, const WCHAR *chord_text);
void hg_key_clear(int action);
void hg_key_reset(int action); /* back to the built-in default */
void hg_key_reset_all(void);

/* ---- dispatch */
/* The action the chord runs in that context, or 0. */
int hg_key_lookup(int context, UINT vk, UINT mods);
/* Ctrl/Alt/Shift/Win as they are right now, for a WM_KEYDOWN handler. */
UINT hg_key_current_mods(void);
/* Convenience for the handlers: looks the chord up with the modifiers Windows
 * has at this instant. */
int hg_key_lookup_now(int context, UINT vk);

/* ---- lifecycle */
void hg_keys_load(void);
/* Re-registers the system hotkeys and rebuilds the accelerator tables. Called
 * whenever a binding changes; safe before the windows exist. */
void hg_keys_apply(void);
/* The accelerator table the message loop translates, built from the widget
 * context. document_window picks the smaller set that a note or a command line
 * can live with. */
HACCEL hg_keys_accel_table(BOOL document_window);
/* Gives the accelerator tables back, at shutdown. */
void hg_keys_shutdown(void);

#endif /* HG_KEYS_H */
