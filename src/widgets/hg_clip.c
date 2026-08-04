/* Clipboard history.
 *
 * A listener on a message-only window records every text clip for the life of
 * the process; the L button opens a window over that list. Design and the
 * alternatives that were turned down are in the private RFC.
 *
 * Three rules carry most of the behaviour:
 *
 * - A clip identical to the newest entry is not recorded. That is what stops an
 *   application re-setting the same text from filling the list, and it is also
 *   what stops our own write in clip_promote from coming straight back in as a
 *   new entry - no flag, no suppression window, nothing to get out of sync.
 * - Choosing an old entry rotates it to the front rather than swapping it with
 *   the front, so the list ends up in the order it would have been in had the
 *   reader simply copied that text again.
 * - Lowering the maximum drops the oldest immediately, because "at most this
 *   many" would otherwise be false the moment it was set. Raising it fills
 *   forward only: the dropped clips are gone, and inventing history would be
 *   worse than not having it. */
#include "hg_clip.h"
#include "../hg_utils.h"
#include "../hg_globals.h"

#define HG_CLIP_SEARCH_ID 100
#define HG_CLIP_LIST_ID 101
#define HG_CLIP_MAX_ID 102
#define HG_CLIP_SPIN_ID 103

#define HG_CLIP_LIMIT 64 /* the most the maximum may be set to */
#define HG_CLIP_DEFAULT_MAX 16
#define HG_CLIP_ROW_CCH 160 /* how much of a clip a row shows */

/* Above this a clip is not recorded at all. Truncating would leave a row that
 * looks like the whole clip and pastes as a fragment, which is worse than an
 * absent one; the clipboard itself still holds the text either way. */
#define HG_CLIP_TEXT_CEILING (1024u * 1024u)

static WCHAR *s_clips[HG_CLIP_LIMIT];
static int s_clip_count = 0;
static int s_clip_max = HG_CLIP_DEFAULT_MAX;
static HWND s_clip_wnd = NULL;      /* the history window, or NULL */
static HWND s_clip_listener = NULL; /* message-only; outlives the window */
static HFONT s_clip_font = NULL;
static BOOL s_clip_started = FALSE;

static void clip_list_fill(void);

/* ----------------------------------------------------------------- settings */

static int clip_clamp_max(int value)
{
    if (value < 1)
        return 1;
    if (value > HG_CLIP_LIMIT)
        return HG_CLIP_LIMIT;
    return value;
}

static void clip_load_max(void)
{
    s_clip_max = clip_clamp_max((int)GetPrivateProfileIntW(L"clipboard", L"max", HG_CLIP_DEFAULT_MAX,
                                                           hg_g_config_path));
}

static void clip_save_max(void)
{
    WCHAR text[16];
    hellgates_wsprintf(text, HG_ARRAYSIZE(text), L"%d", s_clip_max);
    WritePrivateProfileStringW(L"clipboard", L"max", text, hg_g_config_path);
}

/* -------------------------------------------------------------- the history */

static void clip_drop_from(int first)
{
    for (int i = first; i < s_clip_count; ++i) {
        free(s_clips[i]);
        s_clips[i] = NULL;
    }
    if (first < s_clip_count)
        s_clip_count = first;
}

static void clip_set_max(int value)
{
    value = clip_clamp_max(value);
    if (value == s_clip_max)
        return;
    s_clip_max = value;
    clip_save_max();

    /* Lowering takes effect now; raising has nothing to bring back. */
    if (s_clip_count > s_clip_max) {
        clip_drop_from(s_clip_max);
        clip_list_fill();
    }
}

/* The count cap alone leaves the total unbounded: 64 clips near the per-clip
 * ceiling is over a hundred megabytes held for the life of the process. The
 * total gets its own hard cap, enforced newest-first so what goes is always
 * the oldest history. Enforced on every push, the walk below only ever
 * measures a total already at or near the cap. */
#define HG_CLIP_TOTAL_CCH (4u * 1024u * 1024u)

static void clip_enforce_total(void)
{
    size_t total = 0;
    int keep = 0;
    while (keep < s_clip_count) {
        total += (s_clips[keep] ? wcslen(s_clips[keep]) + 1 : 0);
        if (total > HG_CLIP_TOTAL_CCH && keep > 0) /* the newest always stays */
            break;
        ++keep;
    }
    clip_drop_from(keep);
}

/* Takes ownership of text on every path, including the ones that refuse it. */
static void clip_push(WCHAR *text)
{
    if (!text)
        return;
    if (s_clip_count > 0 && s_clips[0] && wcscmp(s_clips[0], text) == 0) {
        free(text);
        return;
    }

    if (s_clip_count >= s_clip_max) {
        clip_drop_from(s_clip_max - 1);
    } else if (s_clip_count >= HG_CLIP_LIMIT) {
        clip_drop_from(HG_CLIP_LIMIT - 1);
    }

    if (s_clip_count > 0)
        memmove(&s_clips[1], &s_clips[0], sizeof(s_clips[0]) * (size_t)s_clip_count);
    s_clips[0] = text;
    ++s_clip_count;

    clip_enforce_total();
    clip_list_fill();
}

/* --------------------------------------------------------- the clipboard API */

/* The handle belongs to the clipboard, not to us: it is locked to read, never
 * freed, and valid only until CloseClipboard. */
static WCHAR *clip_read_clipboard(void)
{
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return NULL;
    /* The clipboard is a machine-wide lock and another process holding it is
     * routine rather than exceptional. The clip is skipped; the next
     * notification carries on. */
    if (!OpenClipboard(s_clip_listener))
        return NULL;

    WCHAR *copy = NULL;
    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (handle) {
        const WCHAR *text = (const WCHAR *)GlobalLock(handle);
        if (text) {
            size_t cch = wcslen(text);
            if (cch > 0 && cch <= HG_CLIP_TEXT_CEILING) {
                copy = (WCHAR *)malloc(sizeof(WCHAR) * (cch + 1u));
                if (copy)
                    memcpy(copy, text, sizeof(WCHAR) * (cch + 1u));
            }
            GlobalUnlock(handle);
        }
    }
    CloseClipboard();
    return copy;
}

/* On success the clipboard owns the block, so freeing it here would be a double
 * free; on failure nothing else will, so it has to be freed here. */
static BOOL clip_write_clipboard(const WCHAR *text)
{
    if (!text)
        return FALSE;

    size_t cch = wcslen(text);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (cch + 1u));
    if (!handle)
        return FALSE;

    WCHAR *dest = (WCHAR *)GlobalLock(handle);
    if (!dest) {
        GlobalFree(handle);
        return FALSE;
    }
    memcpy(dest, text, sizeof(WCHAR) * (cch + 1u));
    GlobalUnlock(handle);

    if (!OpenClipboard(s_clip_listener)) {
        GlobalFree(handle);
        return FALSE;
    }
    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, handle)) {
        CloseClipboard();
        GlobalFree(handle);
        return FALSE;
    }
    CloseClipboard();
    return TRUE;
}

/* A rotation rather than a swap: everything above the chosen entry moves down
 * one, so the clip that was current is kept one place lower and nothing below
 * the choice moves. Swapping would fling the previously-current clip down to
 * wherever the reader happened to click. */
static void clip_promote(int index)
{
    if (index < 0 || index >= s_clip_count)
        return;

    if (index > 0) {
        WCHAR *chosen = s_clips[index];
        memmove(&s_clips[1], &s_clips[0], sizeof(s_clips[0]) * (size_t)index);
        s_clips[0] = chosen;
    }
    /* Our own write comes back as WM_CLIPBOARDUPDATE and is dropped by the
     * duplicate rule in clip_push, which is already there for its own reasons. */
    clip_write_clipboard(s_clips[0]);
}

/* ------------------------------------------------------------------ the list */

/* One row, one line: tabs and line breaks become spaces so a multi-line clip
 * does not become a row of boxes, and a long one is cut with an ellipsis that
 * says it was cut. */
static void clip_row_text(const WCHAR *text, WCHAR *out, size_t out_cch)
{
    size_t written = 0;
    BOOL pending_space = FALSE;
    size_t limit = (out_cch > 4) ? (out_cch - 4) : 0;

    for (size_t i = 0; text[i] && written < limit; ++i) {
        WCHAR ch = text[i];
        if (ch == L'\r' || ch == L'\n' || ch == L'\t' || ch == L' ') {
            if (written > 0)
                pending_space = TRUE;
            continue;
        }
        if (pending_space) {
            out[written++] = L' ';
            pending_space = FALSE;
            if (written >= limit)
                break;
        }
        out[written++] = ch;
    }

    if (text[0] && written == 0) {
        /* Whitespace only, which is a real thing to have copied. */
        StringCchCopyW(out, out_cch, L"(blank)");
        return;
    }

    out[written] = L'\0';
    if (written >= limit && limit > 0)
        StringCchCatW(out, out_cch, L"...");
}

static void clip_list_fill(void)
{
    if (!s_clip_wnd || !IsWindow(s_clip_wnd))
        return;
    HWND list = GetDlgItem(s_clip_wnd, HG_CLIP_LIST_ID);
    if (!list)
        return;

    WCHAR needle[HG_MAX_STR];
    needle[0] = L'\0';
    HWND search = GetDlgItem(s_clip_wnd, HG_CLIP_SEARCH_ID);
    if (search)
        GetWindowTextW(search, needle, (int)HG_ARRAYSIZE(needle));

    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < s_clip_count; ++i) {
        if (!s_clips[i])
            continue;
        if (needle[0] && !StrStrIW(s_clips[i], needle))
            continue;

        WCHAR row[HG_CLIP_ROW_CCH];
        clip_row_text(s_clips[i], row, HG_ARRAYSIZE(row));
        int added = (int)SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)row);
        /* The row carries the history position, so filtering needs no second
         * mapping and a hidden entry simply has no row to click. */
        if (added >= 0)
            SendMessageW(list, LB_SETITEMDATA, (WPARAM)added, (LPARAM)i);
    }

    if (SendMessageW(list, LB_GETCOUNT, 0, 0) > 0)
        SendMessageW(list, LB_SETCURSEL, 0, 0);
}

static void clip_take_selected(HWND hwnd)
{
    HWND list = GetDlgItem(hwnd, HG_CLIP_LIST_ID);
    if (!list)
        return;
    int row = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
    if (row < 0)
        return;
    int index = (int)SendMessageW(list, LB_GETITEMDATA, (WPARAM)row, 0);
    if (index < 0 || index >= s_clip_count)
        return;

    clip_promote(index);
    /* Choosing a clip is the first half of pasting it, so the window gets out
     * of the way rather than leaving the reader to dismiss it first. */
    ShowWindow(hwnd, SW_HIDE);
    clip_list_fill();
}

/* ------------------------------------------------------------------- capture */

static LRESULT CALLBACK clip_listener_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (msg == WM_CLIPBOARDUPDATE) {
        clip_push(clip_read_clipboard());
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

void hg_clip_init(void)
{
    if (s_clip_started)
        return;
    s_clip_started = TRUE;

    clip_load_max();

    static const WCHAR listener_class[] = L"hgclip_listener_class";
    WNDCLASSEXW wc;
    SecureZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = clip_listener_proc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = listener_class;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return;

    /* Message-only: it receives WM_CLIPBOARDUPDATE without ever being a window
     * anyone can see, which is what lets capture continue with the history
     * window closed. */
    s_clip_listener = CreateWindowExW(0, listener_class, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL,
                                      GetModuleHandleW(NULL), NULL);
    if (!s_clip_listener)
        return;

    AddClipboardFormatListener(s_clip_listener);
    /* Whatever is on the clipboard at startup is the first entry: it is what
     * the next paste would produce, so the list would be lying without it. */
    clip_push(clip_read_clipboard());
}

void hg_clip_shutdown(void)
{
    if (s_clip_listener) {
        RemoveClipboardFormatListener(s_clip_listener);
        DestroyWindow(s_clip_listener);
        s_clip_listener = NULL;
    }
    clip_drop_from(0);
    if (s_clip_font) {
        DeleteObject(s_clip_font);
        s_clip_font = NULL;
    }
    s_clip_started = FALSE;
}

/* ------------------------------------------------------------- the window */

static void clip_apply_font(HWND hwnd)
{
    double ws = hg_window_scale(hwnd);
    HFONT font = CreateFontW(-SCW(ws, hg_g_edit_font_size), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!font)
        return;

    const int ids[] = {HG_CLIP_SEARCH_ID, HG_CLIP_LIST_ID, HG_CLIP_MAX_ID};
    for (int i = 0; i < (int)HG_ARRAYSIZE(ids); ++i) {
        HWND child = GetDlgItem(hwnd, ids[i]);
        if (child)
            SendMessageW(child, WM_SETFONT, (WPARAM)font, TRUE);
    }
    if (s_clip_font)
        DeleteObject(s_clip_font);
    s_clip_font = font;
}

/* The spin sits at the right of the top row with the search box taking what is
 * left, and the list has everything below. */
static void clip_layout(HWND hwnd, int width, int height)
{
    double ws = hg_window_scale(hwnd);
    int pad = SCW(ws, 6);
    int row_h = SCW(ws, 24);
    int max_w = SCW(ws, 56);

    int search_w = width - pad * 3 - max_w;
    if (search_w < SCW(ws, 40))
        search_w = SCW(ws, 40);

    HWND search = GetDlgItem(hwnd, HG_CLIP_SEARCH_ID);
    if (search)
        MoveWindow(search, pad, pad, search_w, row_h, TRUE);

    HWND max_edit = GetDlgItem(hwnd, HG_CLIP_MAX_ID);
    if (max_edit)
        MoveWindow(max_edit, pad * 2 + search_w, pad, max_w, row_h, TRUE);

    /* The up-down puts itself back against its buddy's edge when told about it
     * again, which is how it follows a resize. */
    HWND spin = GetDlgItem(hwnd, HG_CLIP_SPIN_ID);
    if (spin && max_edit)
        SendMessageW(spin, UDM_SETBUDDY, (WPARAM)max_edit, 0);

    HWND list = GetDlgItem(hwnd, HG_CLIP_LIST_ID);
    if (list) {
        int list_y = pad * 2 + row_h;
        int list_h = height - list_y - pad;
        MoveWindow(list, pad, list_y, (width - pad * 2 > 0) ? width - pad * 2 : 0, (list_h > 0) ? list_h : 0, TRUE);
    }
}

/* The wheel over the spin adjusts the maximum wherever the focus happens to be,
 * so the three children hand that case up and keep everything else. */
static LRESULT CALLBACK clip_child_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param,
                                                 UINT_PTR subclass_id, DWORD_PTR ref_data)
{
    (void)subclass_id;
    (void)ref_data;

    if (msg == WM_MOUSEWHEEL) {
        HWND parent = GetParent(hwnd);
        POINT pt;
        pt.x = GET_X_LPARAM(l_param);
        pt.y = GET_Y_LPARAM(l_param);
        HWND under = WindowFromPoint(pt);
        if (parent && under &&
            (under == GetDlgItem(parent, HG_CLIP_MAX_ID) || under == GetDlgItem(parent, HG_CLIP_SPIN_ID))) {
            clip_set_max(s_clip_max + ((GET_WHEEL_DELTA_WPARAM(w_param) > 0) ? 1 : -1));
            HWND spin = GetDlgItem(parent, HG_CLIP_SPIN_ID);
            if (spin)
                SendMessageW(spin, UDM_SETPOS32, 0, (LPARAM)s_clip_max);
            return 0;
        }
    }

    /* One click takes the clip. Not LBN_SELCHANGE, which the arrow keys also
     * raise: browsing the list with the keyboard would then promote whatever it
     * passed over. The default handler runs first so the selection has landed
     * on the row that was actually clicked. */
    if (msg == WM_LBUTTONUP) {
        HWND parent = GetParent(hwnd);
        if (parent && hwnd == GetDlgItem(parent, HG_CLIP_LIST_ID)) {
            LRESULT result = DefSubclassProc(hwnd, msg, w_param, l_param);
            clip_take_selected(parent);
            return result;
        }
    }

    if (msg == WM_KEYDOWN && w_param == VK_ESCAPE) {
        PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }
    if (msg == WM_KEYDOWN && w_param == VK_RETURN) {
        clip_take_selected(GetParent(hwnd));
        return 0;
    }

    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK clip_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_CREATE: {
        hg_apply_class_background(hwnd);
        apply_dwm_attributes(hwnd);
        HINSTANCE instance = GetModuleHandleW(NULL);

        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 0, 0, 0,
                        0, hwnd, (HMENU)HG_CLIP_SEARCH_ID, instance, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd,
                        (HMENU)HG_CLIP_MAX_ID, instance, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_HASSTRINGS, 0, 0, 0, 0,
                        hwnd, (HMENU)HG_CLIP_LIST_ID, instance, NULL);

        HWND spin = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
                                    WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS |
                                        UDS_NOTHOUSANDS,
                                    0, 0, 0, 0, hwnd, (HMENU)HG_CLIP_SPIN_ID, instance, NULL);
        if (spin) {
            SendMessageW(spin, UDM_SETRANGE32, (WPARAM)1, (LPARAM)HG_CLIP_LIMIT);
            SendMessageW(spin, UDM_SETBUDDY, (WPARAM)GetDlgItem(hwnd, HG_CLIP_MAX_ID), 0);
            SendMessageW(spin, UDM_SETPOS32, 0, (LPARAM)s_clip_max);
        }

        const int ids[] = {HG_CLIP_SEARCH_ID, HG_CLIP_LIST_ID, HG_CLIP_MAX_ID};
        for (int i = 0; i < (int)HG_ARRAYSIZE(ids); ++i) {
            HWND child = GetDlgItem(hwnd, ids[i]);
            if (child)
                SetWindowSubclass(child, clip_child_subclass_proc, 1, 0);
        }

        clip_apply_font(hwnd);
        return 0;
    }

    case WM_SIZE:
        clip_layout(hwnd, (int)LOWORD(l_param), (int)HIWORD(l_param));
        return 0;

    case WM_COMMAND:
        if (LOWORD(w_param) == HG_CLIP_SEARCH_ID && HIWORD(w_param) == EN_CHANGE) {
            clip_list_fill();
            return 0;
        }
        if (LOWORD(w_param) == HG_CLIP_MAX_ID && HIWORD(w_param) == EN_CHANGE) {
            /* Typed or spun, the number arrives here the same way, because the
             * up-down writes it into its buddy. */
            HWND spin = GetDlgItem(hwnd, HG_CLIP_SPIN_ID);
            if (spin) {
                /* The out parameter is an error flag, not a success one:
                 * FALSE means the buddy held a number. */
                BOOL failed = FALSE;
                int value = (int)SendMessageW(spin, UDM_GETPOS32, 0, (LPARAM)&failed);
                if (!failed)
                    clip_set_max(value);
            }
            return 0;
        }
        if (LOWORD(w_param) == HG_CLIP_LIST_ID && HIWORD(w_param) == LBN_DBLCLK) {
            clip_take_selected(hwnd);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (w_param == VK_ESCAPE) {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        if (w_param == VK_RETURN) {
            clip_take_selected(hwnd);
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        return hg_on_ctlcolor_edit((HDC)w_param);

    case WM_DPICHANGED:
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        clip_apply_font(hwnd);
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (s_clip_wnd == hwnd)
            s_clip_wnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

/* ------------------------------------------------- the command box's view */

int hg_clip_count(void)
{
    hg_clip_init();
    return s_clip_count;
}

int hg_clip_max(void)
{
    hg_clip_init();
    return s_clip_max;
}

void hg_clip_set_max(int value)
{
    hg_clip_init();
    clip_set_max(value);
}

BOOL hg_clip_row(int number, WCHAR *out, size_t out_cch)
{
    hg_clip_init();
    if (!out || out_cch == 0 || number < 1 || number > s_clip_count || !s_clips[number - 1])
        return FALSE;
    clip_row_text(s_clips[number - 1], out, out_cch);
    return TRUE;
}

BOOL hg_clip_take(int number)
{
    hg_clip_init();
    if (number < 1 || number > s_clip_count)
        return FALSE;
    clip_promote(number - 1);
    clip_list_fill();
    return TRUE;
}

void hg_clip_toggle_window(void)
{
    hg_clip_init(); /* the L button may be the first thing that runs */

    if (s_clip_wnd && IsWindow(s_clip_wnd)) {
        if (IsWindowVisible(s_clip_wnd)) {
            ShowWindow(s_clip_wnd, SW_HIDE);
            return;
        }
        clip_list_fill();
        ShowWindow(s_clip_wnd, SW_SHOWNORMAL);
        hg_force_foreground(s_clip_wnd);
        return;
    }

    POINT pt = {0, 0};
    GetCursorPos(&pt);
    double ws = hg_point_scale(pt);

    s_clip_wnd = CreateWindowExW(WS_EX_TOOLWINDOW, HG_CLASS_CLIP, L"Clipboard History",
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN, pt.x, pt.y,
                                 SCW(ws, 440), SCW(ws, 320), NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!s_clip_wnd)
        return;

    clip_list_fill();
    ensure_window_visible(s_clip_wnd, NULL);
    ShowWindow(s_clip_wnd, SW_SHOWNORMAL);
    hg_force_foreground(s_clip_wnd);
    HWND search = GetDlgItem(s_clip_wnd, HG_CLIP_SEARCH_ID);
    if (search)
        SetFocus(search);
}
