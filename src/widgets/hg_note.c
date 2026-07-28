/* Notes: a list window, one editor window per note, and the files behind them.
 *
 * A note is a plain .txt file whose first line is the title and whose remaining
 * lines are the body, so Notepad or any other editor reads and writes them
 * without help. The only thing a text file cannot carry is the keep flag, and
 * that lives beside them in note/index.ini.
 *
 * Editing never touches the disk directly: the text lives in memory and a timer
 * writes out only the notes that actually changed, so holding a key down does
 * not turn into a write per character. */
#include "hg_note.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

#define HG_MAX_NOTES 256
#define HG_NOTE_TITLE_CCH 128
#define HG_NOTE_SAVE_DELAY_MS 2000 /* how long a change rests before it is written */
#define HG_NOTE_EDIT_ID 100
#define HG_NOTE_LIST_ID 101
#define HG_NOTE_TIMER_SAVE 1
#define HG_NOTE_ROW_ADD 0            /* the list's first row is the New Note action */
#define HG_NOTE_MSG_REFILL (WM_APP + 0) /* an editor closed; its row needs redrawing */
#define HG_NOTE_MSG_ADD (WM_APP + 1)    /* the action row was clicked */

typedef struct HgNote {
    BOOL used;
    WCHAR id[16];
    WCHAR file[HG_MAX_PATH];
    SYSTEMTIME created;  /* taken from the file name, which carries the date */
    SYSTEMTIME modified; /* the file's own last-write time, in local time */
    WCHAR title[HG_NOTE_TITLE_CCH];
    WCHAR *text; /* the whole note, title line included; never NULL once used */
    BOOL keep;
    BOOL dirty;
    ULONGLONG changed_tick;
    HWND editor;
} HgNote;

static HgNote s_notes[HG_MAX_NOTES];
static int s_note_count = 0;
static HWND s_note_list_wnd = NULL;
static BOOL s_notes_loaded = FALSE;

static void note_directory(WCHAR *out, size_t out_cch)
{
    hellgates_wsprintf(out, out_cch, L"%ls\\note", hg_g_base_path);
}

static void note_index_path(WCHAR *out, size_t out_cch)
{
    hellgates_wsprintf(out, out_cch, L"%ls\\note\\index.ini", hg_g_base_path);
}

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
    normalize_path_for_api(note->file, normalized, HG_MAX_PATH);

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

    FILETIME write_time;
    if (ok && GetFileTime(file, NULL, NULL, &write_time)) {
        FILETIME local;
        if (FileTimeToLocalFileTime(&write_time, &local))
            FileTimeToSystemTime(&local, &note->modified);
    }

    CloseHandle(file);
    free(utf8);
    return ok;
}

static void note_save_keep_flag(const HgNote *note)
{
    WCHAR index[HG_MAX_PATH];
    note_index_path(index, HG_MAX_PATH);
    WritePrivateProfileStringW(L"keep", note->id, note->keep ? L"1" : L"0", index);
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

    WCHAR dir[HG_MAX_PATH];
    note_directory(dir, HG_MAX_PATH);
    hellgates_wsprintf(note->file, HG_MAX_PATH, L"%ls\\note-%ls-%04d%02d%02d.txt", dir, note->id, note->created.wYear,
                       note->created.wMonth, note->created.wDay);

    note_set_text(note, L"");
    note_write_file(note);
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

    WCHAR index[HG_MAX_PATH];
    note_index_path(index, HG_MAX_PATH);

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
        hellgates_wsprintf(note->file, HG_MAX_PATH, L"%ls\\%ls", dir, find.cFileName);

        FILETIME local;
        if (FileTimeToLocalFileTime(&find.ftLastWriteTime, &local)) {
            FileTimeToSystemTime(&local, &note->modified);
        } else {
            note->modified = created;
        }

        WCHAR *text = note_read_file(note->file);
        note_set_text(note, text ? text : L"");
        free(text);

        note->keep = (GetPrivateProfileIntW(L"keep", note->id, 0, index) != 0);
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
    hg_notes_flush(TRUE);
    for (int i = 0; i < s_note_count; ++i) {
        free(s_notes[i].text);
        s_notes[i].text = NULL;
    }
}

static void note_delete(HgNote *note)
{
    if (note->keep)
        return;

    if (note->editor && IsWindow(note->editor))
        DestroyWindow(note->editor);

    WCHAR normalized[HG_MAX_PATH];
    normalize_path_for_api(note->file, normalized, HG_MAX_PATH);
    DeleteFileW(normalized);

    WCHAR index[HG_MAX_PATH];
    note_index_path(index, HG_MAX_PATH);
    WritePrivateProfileStringW(L"keep", note->id, NULL, index);

    free(note->text);
    SecureZeroMemory(note, sizeof(*note));
}

/* ------------------------------------------------------------- editor window */

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
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

static void note_editor_pull_text(HgNote *note)
{
    if (!note->editor || !IsWindow(note->editor))
        return;
    HWND edit = GetDlgItem(note->editor, HG_NOTE_EDIT_ID);
    if (!edit)
        return;

    int len = GetWindowTextLengthW(edit);
    if (len < 0)
        len = 0;
    WCHAR *buffer = (WCHAR *)malloc(sizeof(WCHAR) * ((size_t)len + 1u));
    if (!buffer)
        return;
    GetWindowTextW(edit, buffer, len + 1);

    free(note->text);
    note->text = buffer;
    note_refresh_title(note);

    note->dirty = TRUE;
    note->changed_tick = GetTickCount64();
    SetWindowTextW(note->editor, note->title);
}

static HgNote *note_from_editor(HWND hwnd)
{
    LONG_PTR index = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (index < 0 || index >= s_note_count)
        return NULL;
    HgNote *note = &s_notes[index];
    return note->used ? note : NULL;
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

    /* Editors stack up as the reader opens more of them, so each one steps down
     * and right instead of landing on top of the last. */
    static int cascade = 0;
    POINT pt = {0, 0};
    GetCursorPos(&pt);
    double ws = hg_point_scale(pt);
    int offset = SCW(ws, 28) * (cascade++ % 8);

    HWND wnd = CreateWindowExW(WS_EX_TOOLWINDOW, HG_CLASS_NOTE_EDIT, note->title,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
                               pt.x + offset, pt.y + offset, SCW(ws, 380), SCW(ws, 300), NULL, NULL,
                               GetModuleHandle(NULL), NULL);
    if (!wnd)
        return;

    SetWindowLongPtrW(wnd, GWLP_USERDATA, (LONG_PTR)index);
    note->editor = wnd;

    HWND edit = GetDlgItem(wnd, HG_NOTE_EDIT_ID);
    if (edit) {
        SetWindowTextW(edit, note->text ? note->text : L"");
    }

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
        HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                        ES_WANTRETURN | ES_NOHIDESEL,
                                    0, 0, 0, 0, hwnd, (HMENU)HG_NOTE_EDIT_ID, GetModuleHandle(NULL), NULL);
        if (edit) {
            SendMessageW(edit, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
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

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_DPICHANGED:
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        HgNote *note = note_from_editor(hwnd);
        if (note) {
            note->editor = NULL;
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

static int note_compare_recent(const void *a, const void *b)
{
    const HgNote *left = &s_notes[*(const int *)a];
    const HgNote *right = &s_notes[*(const int *)b];

    FILETIME lft, rft;
    if (!SystemTimeToFileTime(&left->modified, &lft) || !SystemTimeToFileTime(&right->modified, &rft))
        return 0;
    return CompareFileTime(&rft, &lft); /* most recently touched first */
}

static void note_list_fill(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    if (!list)
        return;

    int selected = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);

    /* Row zero is always the action, never a note, so making a note needs no
     * remembered key and an empty list still offers something to do. */
    SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)L"+Add Note");
    SendMessageW(list, LB_SETITEMDATA, (WPARAM)HG_NOTE_ROW_ADD, (LPARAM)-1);

    int order[HG_MAX_NOTES];
    int count = 0;
    for (int i = 0; i < s_note_count; ++i) {
        if (s_notes[i].used)
            order[count++] = i;
    }
    if (count > 1)
        qsort(order, (size_t)count, sizeof(order[0]), note_compare_recent);

    for (int i = 0; i < count; ++i) {
        const HgNote *note = &s_notes[order[i]];
        WCHAR line[HG_NOTE_TITLE_CCH + 48];
        hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"%02d%02d%02d  %02d%02d%02d  %ls%ls",
                           note->created.wYear % 100, note->created.wMonth, note->created.wDay,
                           note->modified.wYear % 100, note->modified.wMonth, note->modified.wDay,
                           note->keep ? L"* " : L"", note->title);
        int row = (int)SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)line);
        if (row >= 0)
            SendMessageW(list, LB_SETITEMDATA, (WPARAM)row, (LPARAM)order[i]);
    }

    int last_row = count; /* the action row pushed every note down by one */
    if (selected < 0)
        selected = HG_NOTE_ROW_ADD;
    if (selected > last_row)
        selected = last_row;
    SendMessageW(list, LB_SETCURSEL, (WPARAM)selected, 0);
}

/* The note the selection points at, or -1 when it rests on the action row. */
static int note_list_selected(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    if (!list)
        return -1;
    int row = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    if (row <= HG_NOTE_ROW_ADD)
        return -1;
    return (int)SendMessageW(list, LB_GETITEMDATA, (WPARAM)row, 0);
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

    s_note_list_wnd = CreateWindowExW(WS_EX_TOOLWINDOW, HG_CLASS_NOTE_LIST, L"Notes",
                                      WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN, pt.x,
                                      pt.y, SCW(ws, 460), SCW(ws, 320), NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!s_note_list_wnd)
        return;

    note_list_fill(s_note_list_wnd);
    ensure_window_visible(s_note_list_wnd, NULL);
    ShowWindow(s_note_list_wnd, SW_SHOWNORMAL);
    hg_force_foreground(s_note_list_wnd);
}

/* Enter on the action row makes a note; on any other row it opens one. */
static void note_list_open_selected(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_NOTE_LIST_ID);
    if (list && (int)SendMessageW(list, LB_GETCURSEL, 0, 0) == HG_NOTE_ROW_ADD) {
        note_list_create(hwnd);
        return;
    }

    int index = note_list_selected(hwnd);
    if (index >= 0)
        note_open_editor(index);
}

/* The row under a click, or -1 when the pointer is past the last one. */
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
    if ((msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) && note_list_row_at(hwnd, l_param) == HG_NOTE_ROW_ADD) {
        SendMessageW(hwnd, LB_SETCURSEL, (WPARAM)HG_NOTE_ROW_ADD, 0);
        if (msg == WM_LBUTTONDOWN)
            PostMessageW(GetParent(hwnd), HG_NOTE_MSG_ADD, 0, 0);
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
        if (list) {
            SendMessageW(list, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
            SetWindowSubclass(list, note_list_subclass_proc, 1, 0);
        }
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
        case VK_DELETE: {
            int index = note_list_selected(hwnd);
            if (index >= 0) {
                if (s_notes[index].keep) {
                    append_message(L"Note is kept; press K to release it first");
                } else {
                    note_delete(&s_notes[index]);
                    note_list_fill(hwnd);
                }
            }
            return 0;
        }
        case 'K': {
            int index = note_list_selected(hwnd);
            if (index >= 0) {
                s_notes[index].keep = !s_notes[index].keep;
                note_save_keep_flag(&s_notes[index]);
                note_list_fill(hwnd);
            }
            return 0;
        }
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

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_DPICHANGED:
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (s_note_list_wnd == hwnd)
            s_note_list_wnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
