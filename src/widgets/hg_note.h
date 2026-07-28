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
LRESULT CALLBACK note_list_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK note_edit_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_NOTE_H */
