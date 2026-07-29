#ifndef HG_NOTE_H
#define HG_NOTE_H

#include "../hg_common.h"

/* Notes are plain .txt files under the config directory: the first line is the
 * title, the rest is the body, so a note stays readable and editable outside
 * this app. What a text file cannot carry - the archive flag, each section's
 * sort order, and where every window was left - lives beside them in
 * note/note.ini. */

void hg_notes_load(void);
void hg_notes_flush(BOOL force);
void hg_notes_shutdown(void);

void show_note_list_window(void);

/* ------------------------------------------------- the command box's view
 *
 * The command box addresses a note by the number `list note` printed beside it.
 * That numbering is deliberately not the list window's: the window shows two
 * halves, each sorted however the reader left it, so archiving a note or
 * changing a sort would renumber everything under a number already typed. The
 * command numbering is identifier order instead - nothing in the interface can
 * reorder it, so a number written down stays the note it was until that note is
 * deleted. */
#define HG_NOTE_BRIEF_TITLE_CCH 128
typedef struct HgNoteBrief {
    WCHAR title[HG_NOTE_BRIEF_TITLE_CCH];
    BOOL archived;
} HgNoteBrief;

int hg_note_command_count(void);
BOOL hg_note_command_brief(int number, HgNoteBrief *out);
BOOL hg_note_command_matches(int number, const WCHAR *needle);
BOOL hg_note_command_open(int number);
/* Sets the archive flag; out_changed says whether it was not already there. */
BOOL hg_note_command_set_archived(int number, BOOL archived, BOOL *out_changed);
BOOL hg_note_command_delete(int number);
LRESULT CALLBACK note_list_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK note_edit_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_NOTE_H */
