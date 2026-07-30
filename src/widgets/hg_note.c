/* Notes: a list window, one editor window per note, and the files behind them.
 *
 * A note is a plain .txt file whose first line is the title and whose remaining
 * lines are the body, so Notepad or any other editor reads and writes them
 * without help. Everything a text file cannot carry - which section a note sits
 * in, how each section is sorted, where each editor window was left - lives in
 * note/note.ini beside them.
 *
 * Editing never touches the disk directly: the text lives in memory and a timer
 * writes out only the notes that actually changed, so holding a key down does
 * not turn into a write per character. */
#include "hg_note.h"
#include <richedit.h>
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

#define HG_MAX_NOTES 256
#define HG_NOTE_TITLE_CCH 128
#define HG_NOTE_SAVE_DELAY_MS 2000 /* how long a change rests before it is written */
#define HG_NOTE_EDIT_ID 100
#define HG_NOTE_LIST_ID 101
#define HG_NOTE_TIMER_SAVE 1
#define HG_NOTE_MSG_REFILL (WM_APP + 0) /* an editor closed; its row needs redrawing */
#define HG_NOTE_MSG_ADD (WM_APP + 1)    /* the action row was clicked */
#define HG_NOTE_MSG_DELETE_SELF (WM_APP + 2) /* an editor was told to delete its own note */

/* One text size for the list and every editor, kept unscaled: each window turns
 * it into pixels at its own display's scale, so the same note reads the same
 * physical size whichever monitor it was dragged to. */
#define HG_NOTE_FONT_MIN 8
#define HG_NOTE_FONT_MAX 72
#define HG_NOTE_FONT_DEFAULT 11

/* What a list row stands for. Rows that are not notes carry a negative marker,
 * so nothing has to reason about row numbers to know what it is looking at. */
#define HG_NOTE_ITEM_NONE (-1)
#define HG_NOTE_ITEM_ADD (-2)
#define HG_NOTE_ITEM_HEAD_ACTIVE (-3)
#define HG_NOTE_ITEM_HEAD_ARCHIVED (-4)

/* Context-menu commands, shared by the list's menu and the editor's. */
#define HG_NOTE_CMD_OPEN 1
#define HG_NOTE_CMD_ARCHIVE 2
#define HG_NOTE_CMD_DELETE 3
#define HG_NOTE_CMD_UNDO 4
#define HG_NOTE_CMD_CUT 5
#define HG_NOTE_CMD_COPY 6
#define HG_NOTE_CMD_PASTE 7
#define HG_NOTE_CMD_CLEAR 8
#define HG_NOTE_CMD_SELECT_ALL 9
#define HG_NOTE_CMD_REDO 10
#define HG_NOTE_CMD_SORT_BASE 20 /* + key * 2, + 1 when descending */

typedef enum HgNoteSortKey {
    HG_NOTE_SORT_CREATED = 0,
    HG_NOTE_SORT_MODIFIED,
    HG_NOTE_SORT_TITLE,
    HG_NOTE_SORT_KEY_COUNT
} HgNoteSortKey;

/* The two halves of the list sort independently, so each keeps its own order. */
#define HG_NOTE_SECTION_ACTIVE 0
#define HG_NOTE_SECTION_ARCHIVED 1
#define HG_NOTE_SECTION_COUNT 2

typedef struct HgNoteSort {
    HgNoteSortKey key;
    BOOL descending;
} HgNoteSort;

typedef struct HgNote {
    BOOL used;
    WCHAR id[16];
    /* The file name only. Holding a whole HG_MAX_PATH here cost 64 KiB per slot
     * and about 16 MiB of BSS across the table, paid by every user whether or
     * not a note exists; the directory is known and the name is short. */
    WCHAR file_name[64];
    SYSTEMTIME created;  /* the date from the file name, refined by note.ini */
    SYSTEMTIME modified; /* the file's own last-write time, in local time */
    WCHAR title[HG_NOTE_TITLE_CCH];
    WCHAR *text; /* the whole note, title line included; never NULL once used */
    BOOL archived;
    BOOL dirty;
    ULONGLONG changed_tick;
    HWND editor;
    HFONT editor_font; /* this editor's own handle on the shared size */
    RECT editor_rect;  /* where this note's editor was last left */
    BOOL editor_rect_valid;
} HgNote;

static HgNote s_notes[HG_MAX_NOTES];
static int s_note_count = 0;
static HWND s_note_list_wnd = NULL;
static BOOL s_notes_loaded = FALSE;
static HgNoteSort s_sort[HG_NOTE_SECTION_COUNT];
static int s_font_size = HG_NOTE_FONT_DEFAULT;
static HFONT s_list_font = NULL;

/* Defined with the list window; the editor reaches back through it whenever it
 * changes something the list is showing. */
static void note_list_refresh(void);

/* Defined with the editor; archiving a note from anywhere has to reach the
 * window that is showing it, because that window stops being writable. */
static void note_editor_apply_archived(HgNote *note);

static void note_directory(WCHAR *out, size_t out_cch)
{
    hellgates_wsprintf(out, out_cch, L"%ls\\note", hg_g_base_path);
}

/* One file holds everything about notes that is not the note text itself. */
static void note_settings_path(WCHAR *out, size_t out_cch)
{
    hellgates_wsprintf(out, out_cch, L"%ls\\note\\note.ini", hg_g_base_path);
}

static void note_file_path(const HgNote *note, WCHAR *out, size_t out_cch)
{
    WCHAR dir[HG_MAX_PATH];
    note_directory(dir, HG_MAX_PATH);
    hellgates_wsprintf(out, out_cch, L"%ls\\%ls", dir, note->file_name);
}

static void note_section_name(const HgNote *note, WCHAR *out, size_t out_cch)
{
    hellgates_wsprintf(out, out_cch, L"note-%ls", note->id);
}

static int note_ini_get_int(const WCHAR *section, const WCHAR *key, int fallback)
{
    WCHAR path[HG_MAX_PATH];
    note_settings_path(path, HG_MAX_PATH);
    return (int)GetPrivateProfileIntW(section, key, fallback, path);
}

static void note_ini_set_int(const WCHAR *section, const WCHAR *key, int value)
{
    WCHAR path[HG_MAX_PATH];
    WCHAR text[16];
    note_settings_path(path, HG_MAX_PATH);
    hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d", value);
    WritePrivateProfileStringW(section, key, text, path);
}

static void note_ini_get_str(const WCHAR *section, const WCHAR *key, const WCHAR *fallback, WCHAR *out,
                             DWORD out_cch)
{
    WCHAR path[HG_MAX_PATH];
    note_settings_path(path, HG_MAX_PATH);
    GetPrivateProfileStringW(section, key, fallback, out, out_cch, path);
}

static void note_ini_set_str(const WCHAR *section, const WCHAR *key, const WCHAR *value)
{
    WCHAR path[HG_MAX_PATH];
    note_settings_path(path, HG_MAX_PATH);
    WritePrivateProfileStringW(section, key, value, path);
}

/* ------------------------------------------------------------------- sorting */

static const WCHAR *note_sort_key_name(HgNoteSortKey key)
{
    switch (key) {
    case HG_NOTE_SORT_CREATED:
        return L"created";
    case HG_NOTE_SORT_TITLE:
        return L"title";
    case HG_NOTE_SORT_MODIFIED:
    default:
        return L"modified";
    }
}

static HgNoteSortKey note_sort_key_from_name(const WCHAR *name)
{
    if (lstrcmpiW(name, L"created") == 0)
        return HG_NOTE_SORT_CREATED;
    if (lstrcmpiW(name, L"title") == 0)
        return HG_NOTE_SORT_TITLE;
    return HG_NOTE_SORT_MODIFIED;
}

static const WCHAR *note_section_key(int section)
{
    return (section == HG_NOTE_SECTION_ARCHIVED) ? L"archived_sort" : L"active_sort";
}

static const WCHAR *note_section_desc_key(int section)
{
    return (section == HG_NOTE_SECTION_ARCHIVED) ? L"archived_desc" : L"active_desc";
}

static void note_load_settings(void)
{
    s_font_size = note_ini_get_int(L"font", L"size", HG_NOTE_FONT_DEFAULT);
    if (s_font_size < HG_NOTE_FONT_MIN)
        s_font_size = HG_NOTE_FONT_MIN;
    if (s_font_size > HG_NOTE_FONT_MAX)
        s_font_size = HG_NOTE_FONT_MAX;

    for (int section = 0; section < HG_NOTE_SECTION_COUNT; ++section) {
        WCHAR name[16];
        note_ini_get_str(L"list", note_section_key(section), L"modified", name, HG_ARRAYSIZE(name));
        s_sort[section].key = note_sort_key_from_name(name);
        /* Newest first is the useful default for dates and the harmless one for
         * titles, so both sections start descending. */
        s_sort[section].descending = (note_ini_get_int(L"list", note_section_desc_key(section), 1) != 0);
    }
}

static void note_save_sort_settings(int section)
{
    note_ini_set_str(L"list", note_section_key(section), note_sort_key_name(s_sort[section].key));
    note_ini_set_int(L"list", note_section_desc_key(section), s_sort[section].descending ? 1 : 0);
}

/* ------------------------------------------------------------- the edit host
 *
 * The stock EDIT control has a one-level undo that toggles: undo, then undo
 * again, and you are back. There is no redo to offer. RichEdit keeps a real
 * undo stack with a separate redo, so the note editor hosts one - and asks it
 * for text with the line endings named explicitly, because the notes on disk
 * are CRLF files and the control is free to hand back something else.
 *
 * If the library will not load, the editor falls back to EDIT and simply has no
 * redo, which is worth more than no editor. */

static BOOL s_richedit_tried = FALSE;
static BOOL s_richedit_ready = FALSE;

static BOOL note_richedit_available(void)
{
    if (!s_richedit_tried) {
        s_richedit_tried = TRUE;
        s_richedit_ready = (LoadLibraryW(L"Msftedit.dll") != NULL);
    }
    return s_richedit_ready;
}

static const WCHAR *note_edit_class(void)
{
    return note_richedit_available() ? MSFTEDIT_CLASS : L"EDIT";
}

/* Characters in the control, counting a line break as the two the file will
 * hold. An overestimate is fine; it only sizes a buffer. */
static int note_edit_text_length(HWND edit)
{
    if (!s_richedit_ready)
        return GetWindowTextLengthW(edit);

    GETTEXTLENGTHEX gtl;
    gtl.flags = GTL_NUMCHARS | GTL_USECRLF;
    gtl.codepage = 1200; /* UTF-16 */
    LRESULT len = SendMessageW(edit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    return (len > 0) ? (int)len : 0;
}

/* Always CRLF, whichever control is hosting the text. */
static WCHAR *note_edit_read_text(HWND edit)
{
    int len = note_edit_text_length(edit);
    if (len < 0)
        len = 0;

    WCHAR *buffer = (WCHAR *)malloc(sizeof(WCHAR) * ((size_t)len + 1u));
    if (!buffer)
        return NULL;
    buffer[0] = L'\0';

    if (!s_richedit_ready) {
        GetWindowTextW(edit, buffer, len + 1);
        return buffer;
    }

    GETTEXTEX gt;
    SecureZeroMemory(&gt, sizeof(gt));
    gt.cb = (DWORD)((size_t)(len + 1) * sizeof(WCHAR));
    gt.flags = GT_USECRLF;
    gt.codepage = 1200;
    SendMessageW(edit, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)buffer);
    return buffer;
}

/* RichEdit never sends WM_CTLCOLOREDIT, so its colours are set rather than
 * answered. Both the existing text and the default for what is typed next. */
static void note_edit_apply_theme(HWND edit)
{
    if (!edit || !s_richedit_ready)
        return;

    SendMessageW(edit, EM_SETBKGNDCOLOR, 0, (LPARAM)hg_g_color_scheme_selected.bg);

    CHARFORMAT2W cf;
    SecureZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = hg_g_color_scheme_selected.text;
    SendMessageW(edit, EM_SETCHARFORMAT, 0, (LPARAM)&cf);        /* what is typed next */
    SendMessageW(edit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);  /* what is already there */
}

/* ---------------------------------------------------------------- text size */

/* Negative height means "character height", the same bargain the rest of the
 * app strikes with CreateFontW. */
static HFONT note_make_font(HWND wnd)
{
    double ws = hg_window_scale(wnd);
    return CreateFontW(-SCW(ws, s_font_size), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                       hg_g_font_name);
}

/* The new font is handed to the control before the old one is destroyed, so the
 * control is never holding a dead handle. */
static void note_apply_font(HWND wnd, int child_id, HFONT *slot)
{
    if (!wnd || !IsWindow(wnd))
        return;

    HFONT font = note_make_font(wnd);
    if (!font)
        return;

    HWND child = GetDlgItem(wnd, child_id);
    if (child)
        SendMessageW(child, WM_SETFONT, (WPARAM)font, TRUE);

    if (*slot)
        DeleteObject(*slot);
    *slot = font;
}

/* A new size changes how much fits, not how big the window is. Both windows put
 * a single control in their client area, so re-running that layout against the
 * size the window already has is what fitting the new font means here: the list
 * re-measures its rows, the editor re-wraps, and neither window moves or
 * resizes under the reader. */
static void note_relayout(HWND wnd, int child_id)
{
    if (!wnd || !IsWindow(wnd))
        return;

    RECT rc;
    if (!GetClientRect(wnd, &rc))
        return;

    WORD w = (WORD)((rc.right > rc.left) ? (rc.right - rc.left) : 0);
    WORD h = (WORD)((rc.bottom > rc.top) ? (rc.bottom - rc.top) : 0);
    SendMessageW(wnd, WM_SIZE, (WPARAM)SIZE_RESTORED, (LPARAM)MAKELONG(w, h));

    HWND child = GetDlgItem(wnd, child_id);
    if (child)
        InvalidateRect(child, NULL, TRUE);
}

static void note_refresh_fonts(void)
{
    if (s_note_list_wnd && IsWindow(s_note_list_wnd)) {
        note_apply_font(s_note_list_wnd, HG_NOTE_LIST_ID, &s_list_font);
        note_relayout(s_note_list_wnd, HG_NOTE_LIST_ID);
        /* Row height follows the font, so how many rows fit has changed. Setting
         * the selection to where it already is scrolls it back into view rather
         * than leaving the reader looking at a different part of the list. */
        HWND list = GetDlgItem(s_note_list_wnd, HG_NOTE_LIST_ID);
        if (list) {
            int selected = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
            if (selected >= 0)
                SendMessageW(list, LB_SETCURSEL, (WPARAM)selected, 0);
        }
        InvalidateRect(s_note_list_wnd, NULL, TRUE);
    }
    for (int i = 0; i < s_note_count; ++i) {
        if (!s_notes[i].used || !s_notes[i].editor || !IsWindow(s_notes[i].editor))
            continue;
        note_apply_font(s_notes[i].editor, HG_NOTE_EDIT_ID, &s_notes[i].editor_font);
        /* A new default font can take the character colour with it. */
        note_edit_apply_theme(GetDlgItem(s_notes[i].editor, HG_NOTE_EDIT_ID));
        note_relayout(s_notes[i].editor, HG_NOTE_EDIT_ID);
        /* The text re-wrapped underneath the caret; put the caret back on
         * screen so typing continues where it looked like it would. */
        HWND edit = GetDlgItem(s_notes[i].editor, HG_NOTE_EDIT_ID);
        if (edit)
            SendMessageW(edit, EM_SCROLLCARET, 0, 0);
    }
}

/* One wheel notch. The size is stored unscaled, so a change made on a 200%
 * display lands at the same reading size on a 100% one. */
static void note_font_step(int delta)
{
    int size = s_font_size + ((delta > 0) ? 1 : -1);
    if (size < HG_NOTE_FONT_MIN)
        size = HG_NOTE_FONT_MIN;
    if (size > HG_NOTE_FONT_MAX)
        size = HG_NOTE_FONT_MAX;
    if (size == s_font_size)
        return;

    s_font_size = size;
    note_ini_set_int(L"font", L"size", s_font_size);
    note_refresh_fonts();
}

/* Ctrl and the wheel, wherever the pointer is, change the size for the list and
 * every open editor at once. Returns TRUE when the message was the app's. */
static BOOL note_handle_wheel(WPARAM w_param)
{
    if (!(GET_KEYSTATE_WPARAM(w_param) & MK_CONTROL))
        return FALSE;
    note_font_step((short)HIWORD(w_param));
    return TRUE;
}

/* qsort carries no context of its own, so the comparison reads the sort the
 * caller just installed. */
static HgNoteSort s_sort_in_use;

static int note_compare_time(const SYSTEMTIME *left, const SYSTEMTIME *right)
{
    FILETIME lft, rft;
    if (!SystemTimeToFileTime(left, &lft) || !SystemTimeToFileTime(right, &rft))
        return 0;
    return CompareFileTime(&lft, &rft);
}

static int note_compare(const void *a, const void *b)
{
    const HgNote *left = &s_notes[*(const int *)a];
    const HgNote *right = &s_notes[*(const int *)b];

    int result = 0;
    switch (s_sort_in_use.key) {
    case HG_NOTE_SORT_CREATED:
        result = note_compare_time(&left->created, &right->created);
        break;
    case HG_NOTE_SORT_TITLE:
        result = lstrcmpiW(left->title, right->title);
        break;
    case HG_NOTE_SORT_MODIFIED:
    default:
        result = note_compare_time(&left->modified, &right->modified);
        break;
    }

    /* Equal keys would otherwise shuffle between redraws, which reads as the
     * list twitching for no reason. */
    if (result == 0)
        result = note_compare_time(&left->created, &right->created);
    if (result == 0)
        result = lstrcmpiW(left->id, right->id);

    return s_sort_in_use.descending ? -result : result;
}

/* ------------------------------------------------------------ note text state */

/* The title is whatever stands on the first line; an empty note still needs
 * something to show in the list. */
static void note_refresh_title(HgNote *note)
{
    const WCHAR *text = note->text ? note->text : L"";
    size_t i = 0;
    while (text[i] && text[i] != L'\r' && text[i] != L'\n' && i + 1 < HG_NOTE_TITLE_CCH) {
        note->title[i] = text[i];
        ++i;
    }
    note->title[i] = L'\0';

    /* Trailing blanks would show as an empty row, which reads as a broken note
     * rather than an untitled one. */
    while (i > 0 && (note->title[i - 1] == L' ' || note->title[i - 1] == L'\t')) {
        note->title[--i] = L'\0';
    }
    if (note->title[0] == L'\0')
        StringCchCopyW(note->title, HG_NOTE_TITLE_CCH, L"(untitled)");
}

static void note_set_text(HgNote *note, const WCHAR *text)
{
    size_t cch = text ? wcslen(text) : 0;
    WCHAR *copy = (WCHAR *)malloc(sizeof(WCHAR) * (cch + 1u));
    if (!copy)
        return;
    if (cch > 0)
        memcpy(copy, text, sizeof(WCHAR) * cch);
    copy[cch] = L'\0';

    free(note->text);
    note->text = copy;
    note_refresh_title(note);
}

/* ---------------------------------------------------------------- file I/O */

static WCHAR *note_read_file(const WCHAR *path)
{
    WCHAR normalized[HG_MAX_PATH];
    normalize_path_for_api(path, normalized, HG_MAX_PATH);

    HANDLE file = CreateFileW(normalized, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return NULL;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size) || size.QuadPart > 4 * 1024 * 1024) {
        CloseHandle(file);
        return NULL;
    }

    DWORD bytes = (DWORD)size.QuadPart;
    char *raw = (char *)malloc(bytes + 1u);
    if (!raw) {
        CloseHandle(file);
        return NULL;
    }

    DWORD read = 0;
    BOOL ok = ReadFile(file, raw, bytes, &read, NULL);
    CloseHandle(file);
    if (!ok) {
        free(raw);
        return NULL;
    }
    raw[read] = '\0';

    /* Written as UTF-8 with a BOM; skipping the BOM also lets a note hand-saved
     * as plain ASCII come back unchanged. */
    const char *body = raw;
    DWORD body_len = read;
    if (read >= 3 && (unsigned char)raw[0] == 0xEF && (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF) {
        body += 3;
        body_len -= 3;
    }

    int cch = MultiByteToWideChar(CP_UTF8, 0, body, (int)body_len, NULL, 0);
    if (cch < 0)
        cch = 0;
    WCHAR *text = (WCHAR *)malloc(sizeof(WCHAR) * ((size_t)cch + 1u));
    if (text) {
        if (cch > 0)
            MultiByteToWideChar(CP_UTF8, 0, body, (int)body_len, text, cch);
        text[cch] = L'\0';
    }
    free(raw);
    return text;
}

static BOOL note_write_file(HgNote *note)
{
    WCHAR normalized[HG_MAX_PATH];
    WCHAR path[HG_MAX_PATH];
    note_file_path(note, path, HG_MAX_PATH);
    normalize_path_for_api(path, normalized, HG_MAX_PATH);

    const WCHAR *text = note->text ? note->text : L"";
    int cb = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    if (cb <= 0)
        return FALSE;

    char *utf8 = (char *)malloc((size_t)cb + 3u);
    if (!utf8)
        return FALSE;
    utf8[0] = (char)0xEF;
    utf8[1] = (char)0xBB;
    utf8[2] = (char)0xBF;
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8 + 3, cb, NULL, NULL);
    DWORD total = (DWORD)(cb - 1) + 3u; /* the terminator does not belong in the file */

    HANDLE file = CreateFileW(normalized, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        free(utf8);
        return FALSE;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(file, utf8, total, &written, NULL) && written == total;

    /* Not GetFileTime on this handle: Windows does not commit a file's time
     * stamps until it is closed, so asking here answers with the time of the
     * previous save and the list would show every note one edit behind. The
     * write just happened, so now is the answer. */
    if (ok)
        GetLocalTime(&note->modified);

    CloseHandle(file);
    free(utf8);
    return ok;
}

static void note_save_archived_flag(const HgNote *note)
{
    WCHAR section[32];
    note_section_name(note, section, HG_ARRAYSIZE(section));
    note_ini_set_int(section, L"archived", note->archived ? 1 : 0);
}

/* The file name only carries the day. Sorting by creation asked for the time of
 * day too, so it is written out once and read back beside the note. */
static void note_save_created(const HgNote *note)
{
    WCHAR section[32];
    WCHAR stamp[24];
    note_section_name(note, section, HG_ARRAYSIZE(section));
    hellgates_wsprintf(stamp, HG_ARRAYSIZE(stamp), L"%04d%02d%02d%02d%02d%02d", note->created.wYear,
                       note->created.wMonth, note->created.wDay, note->created.wHour, note->created.wMinute,
                       note->created.wSecond);
    note_ini_set_str(section, L"created", stamp);
}

static void note_load_created(HgNote *note)
{
    WCHAR section[32];
    WCHAR stamp[24];
    note_section_name(note, section, HG_ARRAYSIZE(section));
    note_ini_get_str(section, L"created", L"", stamp, HG_ARRAYSIZE(stamp));
    if (wcslen(stamp) != 14)
        return; /* the file name's date stands on its own */

    for (int i = 0; i < 14; ++i) {
        if (stamp[i] < L'0' || stamp[i] > L'9')
            return;
    }

    int parts[6] = {0, 0, 0, 0, 0, 0};
    static const int widths[6] = {4, 2, 2, 2, 2, 2};
    int at = 0;
    for (int p = 0; p < 6; ++p) {
        for (int i = 0; i < widths[p]; ++i)
            parts[p] = parts[p] * 10 + (int)(stamp[at++] - L'0');
    }

    note->created.wYear = (WORD)parts[0];
    note->created.wMonth = (WORD)parts[1];
    note->created.wDay = (WORD)parts[2];
    note->created.wHour = (WORD)parts[3];
    note->created.wMinute = (WORD)parts[4];
    note->created.wSecond = (WORD)parts[5];
}

static void note_save_editor_rect(HgNote *note)
{
    if (!note->editor || !IsWindow(note->editor) || IsIconic(note->editor))
        return;

    RECT rc;
    if (!GetWindowRect(note->editor, &rc))
        return;

    note->editor_rect = rc;
    note->editor_rect_valid = TRUE;

    WCHAR section[32];
    note_section_name(note, section, HG_ARRAYSIZE(section));
    note_ini_set_int(section, L"x", (int)rc.left);
    note_ini_set_int(section, L"y", (int)rc.top);
    note_ini_set_int(section, L"w", (int)(rc.right - rc.left));
    note_ini_set_int(section, L"h", (int)(rc.bottom - rc.top));
}

static void note_load_editor_rect(HgNote *note)
{
    WCHAR section[32];
    note_section_name(note, section, HG_ARRAYSIZE(section));

    int w = note_ini_get_int(section, L"w", 0);
    int h = note_ini_get_int(section, L"h", 0);
    if (w <= 0 || h <= 0)
        return;

    int x = note_ini_get_int(section, L"x", 0);
    int y = note_ini_get_int(section, L"y", 0);
    SetRect(&note->editor_rect, x, y, x + w, y + h);
    note->editor_rect_valid = TRUE;
}

/* ------------------------------------------------------------ note lifetime */

static HgNote *note_slot(void)
{
    for (int i = 0; i < s_note_count; ++i) {
        if (!s_notes[i].used)
            return &s_notes[i];
    }
    if (s_note_count >= HG_MAX_NOTES)
        return NULL;
    return &s_notes[s_note_count++];
}

/* Identifiers only have to be unique among the files in one directory, and they
 * end up in a file name, so they stay short and hex. */
static void note_make_id(WCHAR *out, size_t out_cch)
{
    static unsigned counter = 0;
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    unsigned seed = (unsigned)(now.dwLowDateTime ^ (now.dwHighDateTime << 3)) + (++counter * 2654435761u);
    hellgates_wsprintf(out, out_cch, L"%08x", seed);
}

static HgNote *note_create(void)
{
    HgNote *note = note_slot();
    if (!note)
        return NULL;

    SecureZeroMemory(note, sizeof(*note));
    note->used = TRUE;
    GetLocalTime(&note->created);
    note->modified = note->created;
    note_make_id(note->id, HG_ARRAYSIZE(note->id));

    hellgates_wsprintf(note->file_name, HG_ARRAYSIZE(note->file_name), L"note-%ls-%04d%02d%02d.txt", note->id,
                       note->created.wYear, note->created.wMonth, note->created.wDay);

    note_set_text(note, L"");
    note_write_file(note);
    note_save_created(note);
    return note;
}

/* note-<id>-YYYYMMDD.txt: the identifier and the creation date both come back
 * out of the name, so the files carry their own bookkeeping. */
static BOOL note_parse_name(const WCHAR *name, WCHAR *out_id, size_t out_id_cch, SYSTEMTIME *out_created)
{
    if (wcsncmp(name, L"note-", 5) != 0)
        return FALSE;

    const WCHAR *id_start = name + 5;
    const WCHAR *dash = wcschr(id_start, L'-');
    if (!dash)
        return FALSE;

    size_t id_len = (size_t)(dash - id_start);
    if (id_len == 0 || id_len + 1 > out_id_cch)
        return FALSE;
    memcpy(out_id, id_start, sizeof(WCHAR) * id_len);
    out_id[id_len] = L'\0';

    int year = 0, month = 0, day = 0;
    const WCHAR *date = dash + 1;
    for (int i = 0; i < 8; ++i) {
        if (date[i] < L'0' || date[i] > L'9')
            return FALSE;
    }
    for (int i = 0; i < 4; ++i)
        year = year * 10 + (int)(date[i] - L'0');
    for (int i = 4; i < 6; ++i)
        month = month * 10 + (int)(date[i] - L'0');
    for (int i = 6; i < 8; ++i)
        day = day * 10 + (int)(date[i] - L'0');

    SecureZeroMemory(out_created, sizeof(*out_created));
    out_created->wYear = (WORD)year;
    out_created->wMonth = (WORD)month;
    out_created->wDay = (WORD)day;
    return TRUE;
}

void hg_notes_load(void)
{
    if (s_notes_loaded)
        return;
    s_notes_loaded = TRUE;

    WCHAR dir[HG_MAX_PATH];
    note_directory(dir, HG_MAX_PATH);
    SHCreateDirectoryExW(NULL, dir, NULL);

    note_load_settings();

    WCHAR pattern[HG_MAX_PATH];
    hellgates_wsprintf(pattern, HG_MAX_PATH, L"%ls\\note-*.txt", dir);

    WCHAR normalized[HG_MAX_PATH];
    normalize_path_for_api(pattern, normalized, HG_MAX_PATH);

    WIN32_FIND_DATAW find;
    HANDLE search = FindFirstFileW(normalized, &find);
    if (search == INVALID_HANDLE_VALUE)
        return;

    do {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        WCHAR id[16];
        SYSTEMTIME created;
        if (!note_parse_name(find.cFileName, id, HG_ARRAYSIZE(id), &created))
            continue;

        HgNote *note = note_slot();
        if (!note)
            break;

        SecureZeroMemory(note, sizeof(*note));
        note->used = TRUE;
        note->created = created;
        StringCchCopyW(note->id, HG_ARRAYSIZE(note->id), id);
        StringCchCopyW(note->file_name, HG_ARRAYSIZE(note->file_name), find.cFileName);

        FILETIME local;
        if (FileTimeToLocalFileTime(&find.ftLastWriteTime, &local)) {
            FileTimeToSystemTime(&local, &note->modified);
        } else {
            note->modified = created;
        }

        WCHAR path[HG_MAX_PATH];
        note_file_path(note, path, HG_MAX_PATH);
        WCHAR *text = note_read_file(path);
        note_set_text(note, text ? text : L"");
        free(text);

        WCHAR section[32];
        note_section_name(note, section, HG_ARRAYSIZE(section));
        note->archived = (note_ini_get_int(section, L"archived", 0) != 0);
        note_load_created(note);
        note_load_editor_rect(note);
    } while (FindNextFileW(search, &find));

    FindClose(search);
}

void hg_notes_flush(BOOL force)
{
    ULONGLONG now = GetTickCount64();
    for (int i = 0; i < s_note_count; ++i) {
        HgNote *note = &s_notes[i];
        if (!note->used || !note->dirty)
            continue;
        if (!force && now - note->changed_tick < HG_NOTE_SAVE_DELAY_MS)
            continue;
        if (note_write_file(note))
            note->dirty = FALSE;
    }
}

void hg_notes_shutdown(void)
{
    for (int i = 0; i < s_note_count; ++i) {
        if (s_notes[i].used)
            note_save_editor_rect(&s_notes[i]);
    }
    hg_notes_flush(TRUE);
    for (int i = 0; i < s_note_count; ++i) {
        free(s_notes[i].text);
        s_notes[i].text = NULL;
    }
}

/* Deleting a note is one keystroke or one menu pick, so it goes to the Recycle
 * Bin rather than straight off the disk: changing your mind is then Windows'
 * problem, not something this app has to grow a dialog for. A path with no bin
 * behind it - a network share, a removable drive - falls back to a plain
 * delete. SHFileOperationW wants a plain path and a double terminator, so it
 * cannot reuse the \\?\-prefixed form the other file calls take. */
static BOOL note_recycle_file(const WCHAR *path)
{
    size_t len = wcslen(path);
    if (len == 0)
        return FALSE;

    WCHAR *from = (WCHAR *)calloc(len + 2u, sizeof(WCHAR));
    if (!from)
        return FALSE;
    memcpy(from, path, sizeof(WCHAR) * len); /* calloc leaves both terminators */

    SHFILEOPSTRUCTW op;
    SecureZeroMemory(&op, sizeof(op));
    op.wFunc = FO_DELETE;
    op.pFrom = from;
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    int result = SHFileOperationW(&op);
    BOOL ok = (result == 0) && !op.fAnyOperationsAborted;
    free(from);
    return ok;
}

static void note_delete(HgNote *note)
{
    if (note->editor && IsWindow(note->editor))
        DestroyWindow(note->editor);

    WCHAR path[HG_MAX_PATH];
    note_file_path(note, path, HG_MAX_PATH);
    if (!note_recycle_file(path)) {
        WCHAR normalized[HG_MAX_PATH];
        normalize_path_for_api(path, normalized, HG_MAX_PATH);
        DeleteFileW(normalized);
    }

    /* The note's whole section goes with it, so a recycled identifier cannot
     * inherit a dead note's position or archive state. */
    WCHAR section[32];
    note_section_name(note, section, HG_ARRAYSIZE(section));
    note_ini_set_str(section, NULL, NULL);

    free(note->text);
    SecureZeroMemory(note, sizeof(*note));
}

static void note_toggle_archived(HgNote *note)
{
    if (!note || !note->used)
        return;
    note->archived = !note->archived;
    note_save_archived_flag(note);
    note_editor_apply_archived(note);
    note_list_refresh();
}

/* ------------------------------------------------------------- editor window */

static void note_editor_show_context_menu(HWND editor, HWND edit, POINT screen_pt);

/* The edit control eats Escape, so a subclass hands it back to the window that
 * knows what closing means. */
static LRESULT CALLBACK note_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                                UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;
    if (msg == WM_KEYDOWN && w_param == VK_ESCAPE) {
        PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }

    /* Ctrl+Z already reaches the control; Ctrl+Y is the half RichEdit does not
     * bind by default. */
    if (msg == WM_KEYDOWN && w_param == 'Y' && (GetKeyState(VK_CONTROL) < 0)) {
        /* Read-only refuses the edit itself, but not the undo stack it kept
         * from before it was archived, so the key is swallowed rather than
         * forwarded. */
        if (!(GetWindowLongPtrW(hwnd, GWL_STYLE) & ES_READONLY))
            SendMessageW(hwnd, EM_REDO, 0, 0);
        return 0;
    }
    /* Only the Ctrl-held wheel is the window's; a plain one still scrolls. */
    if (msg == WM_MOUSEWHEEL && (GET_KEYSTATE_WPARAM(w_param) & MK_CONTROL)) {
        SendMessageW(GetParent(hwnd), msg, w_param, l_param);
        return 0;
    }

    /* Handling this replaces the edit control's stock menu, which knows about
     * the text but nothing about the note the text belongs to. */
    if (msg == WM_CONTEXTMENU) {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        if (l_param == (LPARAM)-1) {
            /* Raised from the keyboard: anchor it to the control, not to
             * wherever the pointer happens to be resting. */
            RECT rc;
            GetClientRect(hwnd, &rc);
            pt.x = rc.left;
            pt.y = rc.top;
            ClientToScreen(hwnd, &pt);
        }
        note_editor_show_context_menu(GetParent(hwnd), hwnd, pt);
        return 0;
    }

    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

static void note_editor_pull_text(HgNote *note)
{
    if (!note->editor || !IsWindow(note->editor))
        return;
    HWND edit = GetDlgItem(note->editor, HG_NOTE_EDIT_ID);
    if (!edit)
        return;

    WCHAR *buffer = note_edit_read_text(edit);
    if (!buffer)
        return;

    free(note->text);
    note->text = buffer;
    note_refresh_title(note);

    note->dirty = TRUE;
    note->changed_tick = GetTickCount64();
    SetWindowTextW(note->editor, note->title);
}

/* Archiving a note files it away, and something filed away has stopped being
 * written: its editor becomes read-only rather than quietly accepting changes
 * to a note the list is showing as finished. Restoring it makes it writable
 * again, so this is a state and not a one-way door. The caption says which it
 * is, because a window that ignores typing without explanation reads as broken.
 *
 * Copying and selecting stay available - reading an archived note is the whole
 * point of keeping it. */
static void note_editor_apply_archived(HgNote *note)
{
    if (!note || !note->used || !note->editor || !IsWindow(note->editor))
        return;

    HWND edit = GetDlgItem(note->editor, HG_NOTE_EDIT_ID);
    if (edit)
        SendMessageW(edit, EM_SETREADONLY, (WPARAM)(note->archived ? TRUE : FALSE), 0);

    WCHAR caption[HG_NOTE_TITLE_CCH + 32];
    if (note->archived)
        hellgates_wsprintf(caption, HG_ARRAYSIZE(caption), L"%ls  [archived - read only]", note->title);
    else
        StringCchCopyW(caption, HG_ARRAYSIZE(caption), note->title);
    SetWindowTextW(note->editor, caption);
}

static HgNote *note_from_editor(HWND hwnd)
{
    LONG_PTR index = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (index < 0 || index >= s_note_count)
        return NULL;
    HgNote *note = &s_notes[index];
    return note->used ? note : NULL;
}

/* MF_* are long constants, so the enabled/greyed choice is made once here rather
 * than casting at every AppendMenuW call site. */
static UINT note_menu_flags(BOOL enabled)
{
    return enabled ? (UINT)MF_STRING : (UINT)(MF_STRING | MF_GRAYED);
}

/* The clipboard verbs the edit control already implements, plus the two that
 * belong to the note rather than to its text. Each entry is greyed when it would
 * do nothing, so the menu says what is possible instead of failing quietly. */
static void note_editor_show_context_menu(HWND editor, HWND edit, POINT screen_pt)
{
    HgNote *note = note_from_editor(editor);
    if (!note || !edit)
        return;

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    /* Asked through pointers rather than the packed return value, which cannot
     * describe a selection past 64K characters. */
    DWORD sel_start = 0;
    DWORD sel_end = 0;
    SendMessageW(edit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);

    /* An archived note is read-only, so every entry that would change the text
     * is greyed. Copy and Select All are not among them. */
    BOOL writable = !note->archived;
    UINT selection = note_menu_flags(sel_start != sel_end);
    UINT change_sel = note_menu_flags(writable && sel_start != sel_end);
    UINT undo = note_menu_flags(writable && SendMessageW(edit, EM_CANUNDO, 0, 0) != 0);
    /* Redo is RichEdit's; the EDIT fallback has only the one-level toggle and
     * says so by leaving the entry greyed. */
    UINT redo = note_menu_flags(writable && s_richedit_ready && SendMessageW(edit, EM_CANREDO, 0, 0) != 0);
    UINT paste = note_menu_flags(writable && IsClipboardFormatAvailable(CF_UNICODETEXT));
    UINT any_text = note_menu_flags(note_edit_text_length(edit) > 0);

    AppendMenuW(menu, undo, HG_NOTE_CMD_UNDO, L"Undo");
    AppendMenuW(menu, redo, HG_NOTE_CMD_REDO, L"Redo");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, change_sel, HG_NOTE_CMD_CUT, L"Cut");
    AppendMenuW(menu, selection, HG_NOTE_CMD_COPY, L"Copy");
    AppendMenuW(menu, paste, HG_NOTE_CMD_PASTE, L"Paste");
    AppendMenuW(menu, change_sel, HG_NOTE_CMD_CLEAR, L"Delete");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, any_text, HG_NOTE_CMD_SELECT_ALL, L"Select All");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, HG_NOTE_CMD_ARCHIVE, note->archived ? L"Restore" : L"Archive");
    AppendMenuW(menu, MF_STRING, HG_NOTE_CMD_DELETE, L"Delete Note and Close");

    int cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_pt.x, screen_pt.y, editor, NULL);
    DestroyMenu(menu);

    switch (cmd) {
    case HG_NOTE_CMD_UNDO:
        SendMessageW(edit, WM_UNDO, 0, 0);
        break;
    case HG_NOTE_CMD_REDO:
        SendMessageW(edit, EM_REDO, 0, 0);
        break;
    case HG_NOTE_CMD_CUT:
        SendMessageW(edit, WM_CUT, 0, 0);
        break;
    case HG_NOTE_CMD_COPY:
        SendMessageW(edit, WM_COPY, 0, 0);
        break;
    case HG_NOTE_CMD_PASTE:
        SendMessageW(edit, WM_PASTE, 0, 0);
        break;
    case HG_NOTE_CMD_CLEAR:
        SendMessageW(edit, WM_CLEAR, 0, 0);
        break;
    case HG_NOTE_CMD_SELECT_ALL:
        SendMessageW(edit, EM_SETSEL, 0, (LPARAM)-1);
        SetFocus(edit);
        break;
    case HG_NOTE_CMD_ARCHIVE:
        note_toggle_archived(note);
        break;
    case HG_NOTE_CMD_DELETE:
        /* Deleting takes this window down with it, and we are inside its child's
         * message handling, so it waits for the stack to unwind. */
        PostMessageW(editor, HG_NOTE_MSG_DELETE_SELF, 0, 0);
        break;
    default:
        break;
    }
}

static void note_open_editor(int index)
{
    if (index < 0 || index >= s_note_count || !s_notes[index].used)
        return;
    HgNote *note = &s_notes[index];

    if (note->editor && IsWindow(note->editor)) {
        hg_force_foreground(note->editor);
        return;
    }

    POINT pt = {0, 0};
    GetCursorPos(&pt);
    double ws = hg_point_scale(pt);

    int x, y, w, h;
    if (note->editor_rect_valid) {
        /* A note reopens where its own window was left, on whichever monitor
         * that was. */
        x = note->editor_rect.left;
        y = note->editor_rect.top;
        w = note->editor_rect.right - note->editor_rect.left;
        h = note->editor_rect.bottom - note->editor_rect.top;
    } else {
        /* A note opened for the first time steps down and right from the last
         * one instead of landing on top of it. */
        static int cascade = 0;
        int offset = SCW(ws, 28) * (cascade++ % 8);
        x = pt.x + offset;
        y = pt.y + offset;
        w = SCW(ws, 380);
        h = SCW(ws, 300);
    }

    HWND wnd = CreateWindowExW(WS_EX_TOOLWINDOW, HG_CLASS_NOTE_EDIT, note->title,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN, x, y, w, h, NULL,
                               NULL, GetModuleHandle(NULL), NULL);
    if (!wnd)
        return;

    SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG_PTR)index);
    note->editor = wnd;

    note_apply_font(wnd, HG_NOTE_EDIT_ID, &note->editor_font);

    HWND edit = GetDlgItem(wnd, HG_NOTE_EDIT_ID);
    if (edit) {
        SetWindowTextW(edit, note->text ? note->text : L"");
        /* After the text, so colouring all of it has something to colour. */
        note_edit_apply_theme(edit);
        SendMessageW(edit, EM_EMPTYUNDOBUFFER, 0, 0); /* loading is not an edit to undo */
    }
    note_editor_apply_archived(note);

    ensure_window_visible(wnd, NULL);
    ShowWindow(wnd, SW_SHOWNORMAL);
    hg_force_foreground(wnd);
    if (edit)
        SetFocus(edit);
}

LRESULT CALLBACK note_edit_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE: {
        hg_apply_class_background(hwnd);
        apply_dwm_attributes(hwnd);
        HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, note_edit_class(), NULL,
                                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                        ES_WANTRETURN | ES_NOHIDESEL,
                                    0, 0, 0, 0, hwnd, (HMENU)HG_NOTE_EDIT_ID, GetModuleHandle(NULL), NULL);
        if (edit) {
            if (s_richedit_ready) {
                /* RichEdit stays quiet unless asked for change notifications,
                 * caps itself at 64K unless told otherwise, and keeps only a
                 * shallow undo stack by default. */
                SendMessageW(edit, EM_SETEVENTMASK, 0, ENM_CHANGE);
                SendMessageW(edit, EM_EXLIMITTEXT, 0, (LPARAM)(4 * 1024 * 1024));
                SendMessageW(edit, EM_SETUNDOLIMIT, 100, 0);
            }
            SetWindowSubclass(edit, note_edit_subclass_proc, 1, 0);
        }
        SetTimer(hwnd, HG_NOTE_TIMER_SAVE, HG_NOTE_SAVE_DELAY_MS, NULL);
        return 0;
    }

    case WM_SIZE: {
        HWND edit = GetDlgItem(hwnd, HG_NOTE_EDIT_ID);
        if (edit) {
            double ws = hg_window_scale(hwnd);
            int pad = SCW(ws, 6);
            int w = (int)LOWORD(l_param) - pad * 2;
            int h = (int)HIWORD(l_param) - pad * 2;
            MoveWindow(edit, pad, pad, (w > 0) ? w : 0, (h > 0) ? h : 0, TRUE);
        }
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(w_param) == HG_NOTE_EDIT_ID && HIWORD(w_param) == EN_CHANGE) {
            HgNote *note = note_from_editor(hwnd);
            if (note)
                note_editor_pull_text(note);
            return 0;
        }
        break;

    case WM_TIMER:
        if (w_param == HG_NOTE_TIMER_SAVE) {
            /* Only the notes that changed, and only once the typing has settled. */
            hg_notes_flush(FALSE);
            return 0;
        }
        break;

    /* Once per drag rather than per pixel, the same bargain the other widgets
     * strike with the config file. */
    case WM_EXITSIZEMOVE: {
        HgNote *note = note_from_editor(hwnd);
        if (note)
            note_save_editor_rect(note);
        return 0;
    }

    case WM_MOUSEWHEEL:
        if (note_handle_wheel(w_param))
            return 0;
        break;

    case HG_NOTE_MSG_DELETE_SELF: {
        HgNote *note = note_from_editor(hwnd);
        if (note) {
            note_delete(note); /* closes this window on its way through */
            note_list_refresh();
        }
        return 0;
    }

    /* The system theme can change while a note is open, and RichEdit does not
     * ask for its colours the way an EDIT does. */
    case WM_SETTINGCHANGE:
        if (should_refresh_theme_on_setting_change(l_param))
            note_edit_apply_theme(GetDlgItem(hwnd, HG_NOTE_EDIT_ID));
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        /* Only reached on the EDIT fallback; RichEdit is coloured explicitly. */
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_DPICHANGED: {
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        /* The size is the same number; the display it renders on is not. */
        HgNote *note = note_from_editor(hwnd);
        if (note)
            note_apply_font(hwnd, HG_NOTE_EDIT_ID, &note->editor_font);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        HgNote *note = note_from_editor(hwnd);
        if (note) {
            note_save_editor_rect(note);
            note->editor = NULL;
            if (note->editor_font) {
                DeleteObject(note->editor_font);
                note->editor_font = NULL;
            }
            if (note->dirty && note_write_file(note))
                note->dirty = FALSE;
        }
        KillTimer(hwnd, HG_NOTE_TIMER_SAVE);
        if (s_note_list_wnd && IsWindow(s_note_list_wnd))
            PostMessageW(s_note_list_wnd, HG_NOTE_MSG_REFILL, 0, 0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

/* --------------------------------------------------------------- list window */

static void note_list_add_section(HWND list, int section, BOOL show_heading)
{
    int order[HG_MAX_NOTES];
    int count = 0;
    BOOL want_archived = (section == HG_NOTE_SECTION_ARCHIVED);

    for (int i = 0; i < s_note_count; ++i) {
        if (s_notes[i].used && (s_notes[i].archived ? TRUE : FALSE) == want_archived)
            order[count++] = i;
    }
    s_sort_in_use = s_sort[section];
    if (count > 1)
        qsort(order, (size_t)count, sizeof(order[0]), note_compare);

    /* Before the early return for an empty half: the heading is the only row a
     * right-click can reach to set that half's order, so a half with nothing in
     * it yet still has to be nameable. */
    if (show_heading) {
        int row = (int)SendMessageW(list, LB_ADDSTRING, 0,
                                    (LPARAM)(want_archived ? L"-- Archived --" : L"-- Active --"));
        if (row >= 0) {
            SendMessageW(list, LB_SETITEMDATA, (WPARAM)row,
                         (LPARAM)(want_archived ? HG_NOTE_ITEM_HEAD_ARCHIVED : HG_NOTE_ITEM_HEAD_ACTIVE));
        }
    }

    if (count == 0)
        return;

    for (int i = 0; i < count; ++i) {
        const HgNote *note = &s_notes[order[i]];
        WCHAR line[HG_NOTE_TITLE_CCH + 48];
        hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"%02d%02d%02d  %02d%02d%02d  %ls", note->created.wYear % 100,
                           note->created.wMonth, note->created.wDay, note->modified.wYear % 100,
                           note->modified.wMonth, note->modified.wDay, note->title);
        int row = (int)SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)line);
        if (row >= 0)
            SendMessageW(list, LB_SETITEMDATA, (WPARAM)row, (LPARAM)order[i]);
    }
}

static void note_list_fill(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    if (!list)
        return;

    int selected = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);

    /* The action comes first and is never a note, so making one needs no
     * remembered key and an empty list still offers something to do. */
    int add_row = (int)SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)L"+Add Note");
    if (add_row >= 0)
        SendMessageW(list, LB_SETITEMDATA, (WPARAM)add_row, (LPARAM)HG_NOTE_ITEM_ADD);

    /* Headings only earn their rows once something has been archived; until then
     * the list is just notes and a label would be noise. They are still what a
     * right-click aims at to sort an empty-looking half, so they appear for both
     * groups at once or not at all. */
    BOOL show_headings = FALSE;
    for (int i = 0; i < s_note_count && !show_headings; ++i) {
        if (s_notes[i].used && s_notes[i].archived)
            show_headings = TRUE;
    }

    /* Notes being written stay on top; archived ones settle underneath. */
    note_list_add_section(list, HG_NOTE_SECTION_ACTIVE, show_headings);
    note_list_add_section(list, HG_NOTE_SECTION_ARCHIVED, show_headings);

    int last_row = (int)SendMessageW(list, LB_GETCOUNT, 0, 0) - 1;
    if (selected < 0)
        selected = 0;
    if (selected > last_row)
        selected = last_row;
    if (selected >= 0)
        SendMessageW(list, LB_SETCURSEL, (WPARAM)selected, 0);
}

static int note_list_item(HWND list, int row)
{
    if (!list || row < 0)
        return HG_NOTE_ITEM_NONE;
    return (int)SendMessageW(list, LB_GETITEMDATA, (WPARAM)row, 0);
}

/* The note the selection points at, or -1 when it rests on a row that is not
 * one: the action row or a section heading. */
static int note_list_selected(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    if (!list)
        return -1;
    int item = note_list_item(list, (int)SendMessageW(list, LB_GETCURSEL, 0, 0));
    return (item >= 0) ? item : -1;
}

/* Make a note, show it in the list, and open it ready to type into. */
static void note_list_create(HWND hwnd)
{
    HgNote *note = note_create();
    if (!note)
        return;
    note_list_fill(hwnd);
    note_open_editor((int)(note - s_notes));
}

static void note_list_save_geometry(HWND hwnd)
{
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return;
    RECT rc;
    if (!GetWindowRect(hwnd, &rc))
        return;
    note_ini_set_int(L"list", L"x", (int)rc.left);
    note_ini_set_int(L"list", L"y", (int)rc.top);
    note_ini_set_int(L"list", L"w", (int)(rc.right - rc.left));
    note_ini_set_int(L"list", L"h", (int)(rc.bottom - rc.top));
}

void show_note_list_window(void)
{
    hg_notes_load();

    if (s_note_list_wnd && IsWindow(s_note_list_wnd)) {
        note_list_fill(s_note_list_wnd);
        ShowWindow(s_note_list_wnd, SW_SHOWNORMAL);
        hg_force_foreground(s_note_list_wnd);
        return;
    }

    POINT pt = {0, 0};
    GetCursorPos(&pt);
    double ws = hg_point_scale(pt);

    int w = note_ini_get_int(L"list", L"w", 0);
    int h = note_ini_get_int(L"list", L"h", 0);
    int x = note_ini_get_int(L"list", L"x", pt.x);
    int y = note_ini_get_int(L"list", L"y", pt.y);
    if (w <= 0 || h <= 0) {
        w = SCW(ws, 460);
        h = SCW(ws, 320);
        x = pt.x;
        y = pt.y;
    }

    s_note_list_wnd = CreateWindowExW(WS_EX_TOOLWINDOW, HG_CLASS_NOTE_LIST, L"Notes",
                                      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN, x, y, w,
                                      h, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!s_note_list_wnd)
        return;

    note_list_fill(s_note_list_wnd);
    ensure_window_visible(s_note_list_wnd, NULL);
    ShowWindow(s_note_list_wnd, SW_SHOWNORMAL);
    hg_force_foreground(s_note_list_wnd);
}

/* Enter on the action row makes a note; on a note row it opens one; on a
 * heading it does nothing. */
static void note_list_open_selected(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    int item = note_list_item(list, (int)SendMessageW(list, LB_GETCURSEL, 0, 0));

    if (item == HG_NOTE_ITEM_ADD) {
        note_list_create(hwnd);
    } else if (item >= 0) {
        note_open_editor(item);
    }
}

/* Any note can be deleted, archived or not: archiving files a note away, it does
 * not lock it. */
static void note_list_delete(HWND hwnd, int index)
{
    if (index < 0 || index >= s_note_count || !s_notes[index].used)
        return;
    note_delete(&s_notes[index]);
    note_list_fill(hwnd);
}

static void note_list_refresh(void)
{
    if (s_note_list_wnd && IsWindow(s_note_list_wnd))
        note_list_fill(s_note_list_wnd);
}

static void note_list_toggle_archived(HWND hwnd, int index)
{
    (void)hwnd; /* the refresh finds the list itself */
    if (index < 0 || index >= s_note_count)
        return;
    note_toggle_archived(&s_notes[index]);
}

/* ------------------------------------------------------------ context menu */

/* Which half of the list a row belongs to, so a right-click sorts the group it
 * was aimed at. -1 when the row belongs to neither. */
static int note_row_section(HWND list, int row)
{
    int item = note_list_item(list, row);
    if (item == HG_NOTE_ITEM_HEAD_ACTIVE)
        return HG_NOTE_SECTION_ACTIVE;
    if (item == HG_NOTE_ITEM_HEAD_ARCHIVED)
        return HG_NOTE_SECTION_ARCHIVED;
    if (item >= 0 && item < s_note_count && s_notes[item].used)
        return s_notes[item].archived ? HG_NOTE_SECTION_ARCHIVED : HG_NOTE_SECTION_ACTIVE;
    return -1;
}

static void note_append_sort_items(HMENU menu, int section)
{
    static const WCHAR *const labels[HG_NOTE_SORT_KEY_COUNT][2] = {
        {L"Created, oldest first", L"Created, newest first"},
        {L"Modified, oldest first", L"Modified, newest first"},
        {L"Title, A to Z", L"Title, Z to A"},
    };

    for (int key = 0; key < HG_NOTE_SORT_KEY_COUNT; ++key) {
        for (int desc = 0; desc < 2; ++desc) {
            UINT flags = MF_STRING;
            if (s_sort[section].key == (HgNoteSortKey)key && (s_sort[section].descending ? 1 : 0) == desc)
                flags |= MF_CHECKED;
            AppendMenuW(menu, flags, (UINT_PTR)(HG_NOTE_CMD_SORT_BASE + key * 2 + desc), labels[key][desc]);
        }
        if (key + 1 < HG_NOTE_SORT_KEY_COUNT)
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    }
}

static void note_list_show_context_menu(HWND hwnd, HWND list, int row, POINT screen_pt)
{
    int section = note_row_section(list, row);
    if (section < 0)
        return; /* the action row has nothing to offer */

    int note_index = note_list_item(list, row);
    if (note_index < 0)
        note_index = -1;

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    SendMessageW(list, LB_SETCURSEL, (WPARAM)row, 0);

    if (note_index >= 0) {
        AppendMenuW(menu, MF_STRING, HG_NOTE_CMD_OPEN, L"Open");
        AppendMenuW(menu, MF_STRING, HG_NOTE_CMD_ARCHIVE,
                    s_notes[note_index].archived ? L"Restore" : L"Archive");
        AppendMenuW(menu, MF_STRING, HG_NOTE_CMD_DELETE, L"Delete");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    }

    /* The heading says which group the sort lands on, since the menu can be
     * raised from either half. */
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0,
                (section == HG_NOTE_SECTION_ARCHIVED) ? L"Sort archived by" : L"Sort active by");
    note_append_sort_items(menu, section);

    if (note_index >= 0)
        SetMenuDefaultItem(menu, HG_NOTE_CMD_OPEN, FALSE);

    int cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screen_pt.x, screen_pt.y, hwnd, NULL);
    DestroyMenu(menu);

    if (cmd == HG_NOTE_CMD_OPEN) {
        note_open_editor(note_index);
    } else if (cmd == HG_NOTE_CMD_ARCHIVE) {
        note_list_toggle_archived(hwnd, note_index);
    } else if (cmd == HG_NOTE_CMD_DELETE) {
        note_list_delete(hwnd, note_index);
    } else if (cmd >= HG_NOTE_CMD_SORT_BASE && cmd < HG_NOTE_CMD_SORT_BASE + HG_NOTE_SORT_KEY_COUNT * 2) {
        int offset = cmd - HG_NOTE_CMD_SORT_BASE;
        s_sort[section].key = (HgNoteSortKey)(offset / 2);
        s_sort[section].descending = ((offset % 2) != 0);
        note_save_sort_settings(section);
        note_list_fill(hwnd);
    }
}

/* The row under a point, or -1 when the pointer is past the last one. */
static int note_list_row_at(HWND list, LPARAM l_param)
{
    DWORD hit = (DWORD)SendMessageW(list, LB_ITEMFROMPOINT, 0, l_param);
    if (HIWORD(hit) != 0)
        return -1; /* outside the items */
    return (int)LOWORD(hit);
}

static LRESULT CALLBACK note_list_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                                UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;
    /* The list box would swallow these; the window above it owns what they mean. */
    if (msg == WM_KEYDOWN && (w_param == VK_ESCAPE || w_param == VK_RETURN || w_param == VK_DELETE ||
                              w_param == VK_INSERT || w_param == 'K')) {
        SendMessageW(GetParent(hwnd), WM_KEYDOWN, w_param, l_param);
        return 0;
    }

    /* The action row reads as a button, so it answers a single click rather than
     * the double click a note needs. Both button messages are swallowed so a
     * quick second click cannot make a second note. */
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) {
        int row = note_list_row_at(hwnd, l_param);
        if (note_list_item(hwnd, row) == HG_NOTE_ITEM_ADD) {
            SendMessageW(hwnd, LB_SETCURSEL, (WPARAM)row, 0);
            if (msg == WM_LBUTTONDOWN)
                PostMessageW(GetParent(hwnd), HG_NOTE_MSG_ADD, 0, 0);
            return 0;
        }
    }

    if (msg == WM_MOUSEWHEEL && (GET_KEYSTATE_WPARAM(w_param) & MK_CONTROL)) {
        SendMessageW(GetParent(hwnd), msg, w_param, l_param);
        return 0;
    }

    if (msg == WM_RBUTTONUP) {
        int row = note_list_row_at(hwnd, l_param);
        if (row >= 0) {
            POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            ClientToScreen(hwnd, &pt);
            note_list_show_context_menu(GetParent(hwnd), hwnd, row, pt);
        }
        return 0;
    }

    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK note_list_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE: {
        hg_apply_class_background(hwnd);
        apply_dwm_attributes(hwnd);
        HWND list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
                                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS, 0, 0, 0, 0,
                                    hwnd, (HMENU)HG_NOTE_LIST_ID, GetModuleHandle(NULL), NULL);
        if (list)
            SetWindowSubclass(list, note_list_subclass_proc, 1, 0);
        note_apply_font(hwnd, HG_NOTE_LIST_ID, &s_list_font);
        return 0;
    }

    case WM_SIZE: {
        HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
        if (list) {
            double ws = hg_window_scale(hwnd);
            int pad = SCW(ws, 6);
            int w = (int)LOWORD(l_param) - pad * 2;
            int h = (int)HIWORD(l_param) - pad * 2;
            MoveWindow(list, pad, pad, (w > 0) ? w : 0, (h > 0) ? h : 0, TRUE);
        }
        return 0;
    }

    case WM_EXITSIZEMOVE:
        note_list_save_geometry(hwnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(w_param) == HG_NOTE_LIST_ID && HIWORD(w_param) == LBN_DBLCLK) {
            note_list_open_selected(hwnd);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        switch (w_param) {
        case VK_RETURN:
            note_list_open_selected(hwnd);
            return 0;
        case VK_INSERT:
            note_list_create(hwnd);
            return 0;
        case VK_DELETE:
            note_list_delete(hwnd, note_list_selected(hwnd));
            return 0;
        case 'K':
            note_list_toggle_archived(hwnd, note_list_selected(hwnd));
            return 0;
        case VK_ESCAPE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        default:
            break;
        }
        break;

    /* An editor posts this on the way out so its row picks up the new title and
     * modification date. */
    case HG_NOTE_MSG_REFILL:
        note_list_fill(hwnd);
        return 0;

    case HG_NOTE_MSG_ADD:
        note_list_create(hwnd);
        return 0;

    case WM_MOUSEWHEEL:
        if (note_handle_wheel(w_param))
            return 0;
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_DPICHANGED:
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        note_apply_font(hwnd, HG_NOTE_LIST_ID, &s_list_font);
        return 0;

    case WM_CLOSE:
        note_list_save_geometry(hwnd);
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (s_note_list_wnd == hwnd)
            s_note_list_wnd = NULL;
        if (s_list_font) {
            DeleteObject(s_list_font);
            s_list_font = NULL;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

/* --------------------------------------------------- the command box's view
 *
 * Identifier order, for the reason the header gives: it is the one ordering
 * nothing in the interface can change, so a number the reader wrote down still
 * means the note they wrote it for. */
static int note_command_order(int *out, int max)
{
    hg_notes_load();

    int count = 0;
    for (int i = 0; i < s_note_count && count < max; ++i) {
        if (s_notes[i].used)
            out[count++] = i;
    }

    /* Insertion sort: the table holds a couple of hundred slots at most and this
     * runs when a human typed a command, so the simple one is the right one. */
    for (int i = 1; i < count; ++i) {
        int value = out[i];
        int j = i - 1;
        while (j >= 0 && wcscmp(s_notes[out[j]].id, s_notes[value].id) > 0) {
            out[j + 1] = out[j];
            --j;
        }
        out[j + 1] = value;
    }
    return count;
}

static HgNote *note_by_number(int number)
{
    int order[HG_MAX_NOTES];
    int count = note_command_order(order, HG_MAX_NOTES);
    if (number < 1 || number > count)
        return NULL;
    return &s_notes[order[number - 1]];
}

BOOL hg_note_command_new(void)
{
    hg_notes_load();
    HgNote *note = note_create();
    if (!note)
        return FALSE;
    note_list_refresh();
    note_open_editor((int)(note - s_notes));
    return TRUE;
}

int hg_note_font_size(void)
{
    hg_notes_load(); /* the size is read with the rest of note.ini */
    return s_font_size;
}

void hg_note_set_font_size(int size)
{
    hg_notes_load();
    if (size < HG_NOTE_FONT_MIN)
        size = HG_NOTE_FONT_MIN;
    if (size > HG_NOTE_FONT_MAX)
        size = HG_NOTE_FONT_MAX;
    if (size == s_font_size)
        return;
    s_font_size = size;
    note_ini_set_int(L"font", L"size", s_font_size);
    note_refresh_fonts();
}

int hg_note_command_count(void)
{
    int order[HG_MAX_NOTES];
    return note_command_order(order, HG_MAX_NOTES);
}

BOOL hg_note_command_brief(int number, HgNoteBrief *out)
{
    if (!out)
        return FALSE;
    const HgNote *note = note_by_number(number);
    if (!note)
        return FALSE;
    StringCchCopyW(out->title, HG_ARRAYSIZE(out->title), note->title);
    out->archived = note->archived;
    return TRUE;
}

/* Title and body both, because a note is looked for by what is in it as often
 * as by what it is called. */
BOOL hg_note_command_matches(int number, const WCHAR *needle)
{
    if (!needle || !*needle)
        return FALSE;
    const HgNote *note = note_by_number(number);
    if (!note)
        return FALSE;
    if (StrStrIW(note->title, needle))
        return TRUE;
    return note->text && StrStrIW(note->text, needle) != NULL;
}

BOOL hg_note_command_open(int number)
{
    HgNote *note = note_by_number(number);
    if (!note)
        return FALSE;
    note_open_editor((int)(note - s_notes));
    return TRUE;
}

BOOL hg_note_command_set_archived(int number, BOOL archived, BOOL *out_changed)
{
    HgNote *note = note_by_number(number);
    if (!note)
        return FALSE;
    if (out_changed)
        *out_changed = (note->archived != archived);
    if (note->archived != archived)
        note_toggle_archived(note); /* saves the flag and tells both windows */
    return TRUE;
}

BOOL hg_note_command_delete(int number)
{
    HgNote *note = note_by_number(number);
    if (!note)
        return FALSE;
    note_delete(note);
    note_list_refresh();
    return TRUE;
}
