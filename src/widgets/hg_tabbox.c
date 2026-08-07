/* The tab sub-box. See hg_tabbox.h and docs/RFC-2026-07-tabs-as-task-icons.md
 * section D8 for why the tabs left the grid.
 *
 * Three properties shape the code:
 *
 * - It never takes the focus. WS_EX_NOACTIVATE, and the keys arrive because
 *   the toolbar offers them here before using them itself. A hover box that
 *   stole the keyboard would make hovering a destructive act.
 * - It draws from the cache and asks in the background. Opening posts one
 *   request for one window; the rows appear immediately from whatever was
 *   known, and HG_MSG_TABS_READY brings the fresh answer through
 *   hg_tabbox_refresh. Nothing is ever waited for.
 * - It is one box. Opening it for another icon re-targets the same window
 *   rather than stacking a second one. */
#include "hg_tabbox.h"
#include "../hg_utils.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"
#include "hg_taskbox.h"

/* Whether the keyboard is *in* the box.
 *
 * An open box and a focused box are different states, because the letters
 * that label its rows are also the taskbox's own keys - WASD moves the grid,
 * C opens the command box, N the notes. A box that grabbed those the moment
 * it appeared would break navigation every time the focus passed a browser.
 *
 * So: an open box takes the digits (which the grid does not use) and Tab.
 * Tab enters it, and a focused box takes everything - arrows, letters, Enter,
 * Space. Escape leaves, closing the box, which is where the reader started. */
static BOOL s_focused = FALSE;
static HWND s_wnd = NULL;
static HWND s_target = NULL;
static RECT s_anchor = {0, 0, 0, 0};
static WCHAR s_titles[HG_TABS_MAX_PER_WINDOW][HG_MAX_STR];
static int s_count = 0;
static int s_selected = 0;

/* Labels run 1-9, then a-z, then A-Z: sixty-one of them for a cap of
 * twenty-four, so the alphabet never runs out and the digits - the easiest
 * keys - go to the tabs most likely to be wanted. */
static WCHAR tabbox_label_for(int index)
{
    if (index < 0)
        return 0;
    if (index < 9)
        return (WCHAR)(L'1' + index);
    index -= 9;
    if (index < 26)
        return (WCHAR)(L'a' + index);
    index -= 26;
    if (index < 26)
        return (WCHAR)(L'A' + index);
    return 0;
}

/* The row a key names, or -1. Zero is the last row: it sits beside the 9 on
 * every keyboard and "the end" is the one position a reader can always name
 * without counting. */
static int tabbox_index_for_key(WPARAM key)
{
    if (s_count <= 0)
        return -1;
    if (key == L'0')
        return s_count - 1;
    if (key >= L'1' && key <= L'9')
        return (int)(key - L'1');
    if (key >= L'A' && key <= L'Z') {
        /* Windows delivers letters as uppercase virtual keys; the case the
         * reader sees on the row depends on the shift state at the time. */
        int upper = 9 + 26 + (int)(key - L'A');
        int lower = 9 + (int)(key - L'A');
        return (GetKeyState(VK_SHIFT) < 0) ? upper : lower;
    }
    return -1;
}

static int tabbox_row_height(void)
{
    double ws = hg_window_scale(s_wnd ? s_wnd : hg_g_taskbox_wnd);
    int size = ABS(hg_g_edit_font_size);
    if (size < SC(12))
        size = SC(12);
    (void)ws;
    return size + SC(8);
}

static void tabbox_layout(void)
{
    if (!s_wnd)
        return;

    double ws = hg_window_scale(s_wnd);
    int row_h = tabbox_row_height();
    int pad = SCW(ws, 6);
    int rows = (s_count > 0) ? s_count : 1;
    int height = rows * row_h + pad * 2;
    int width = SCW(ws, 320);

    /* Beside the icon, and flipped to whichever side of it has room. */
    HMONITOR monitor = MonitorFromRect(&s_anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    RECT work = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (GetMonitorInfoW(monitor, &mi))
        work = mi.rcWork;

    int x = s_anchor.right + SCW(ws, 4);
    if (x + width > work.right)
        x = s_anchor.left - width - SCW(ws, 4);
    if (x < work.left)
        x = work.left;

    int y = s_anchor.top;
    if (y + height > work.bottom)
        y = work.bottom - height;
    if (y < work.top)
        y = work.top;

    SetWindowPos(s_wnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(s_wnd, NULL, TRUE);
}

/* Take whatever the worker has for this window, and ask for more. */
static void tabbox_pull(void)
{
    if (!s_target)
        return;
    HgTabsAnswer answer;
    int fresh = hg_tabs_take_result(s_target, s_titles, HG_TABS_MAX_PER_WINDOW, &answer);
    if (fresh >= 0 && !answer.failed)
        s_count = fresh;
    if (s_selected >= s_count)
        s_selected = (s_count > 0) ? s_count - 1 : 0;
}

void hg_tabbox_open(HWND target, const RECT *anchor_screen_rc)
{
    if (!target || !IsWindow(target) || !anchor_screen_rc)
        return;
    if (!hg_tabs_enabled() || !hg_tabs_window_may_have_tabs(target))
        return;

    if (!s_wnd) {
        s_wnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, HG_CLASS_TABBOX, L"Tabs",
                                WS_POPUP | WS_BORDER, 0, 0, 0, 0, hg_g_taskbox_wnd, NULL, GetModuleHandle(NULL),
                                NULL);
        if (!s_wnd)
            return;
    }

    if (s_target != target) {
        s_target = target;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE; /* a new box is never entered until Tab says so */
    }
    s_anchor = *anchor_screen_rc;

    /* Draw what is known now, ask for the rest. This one request is the only
     * enumeration this feature performs - there is no cadence behind it. */
    tabbox_pull();
    hg_tabs_request(&target, 1);
    tabbox_layout();
}

void hg_tabbox_close(void)
{
    if (!s_wnd)
        return;
    ShowWindow(s_wnd, SW_HIDE);
    s_target = NULL;
    s_count = 0;
    s_selected = 0;
    s_focused = FALSE;
}

BOOL hg_tabbox_is_open(void)
{
    return s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd) && s_target != NULL;
}

HWND hg_tabbox_target(void)
{
    return hg_tabbox_is_open() ? s_target : NULL;
}

HWND hg_tabbox_window(void)
{
    return s_wnd;
}

void hg_tabbox_refresh(void)
{
    if (!hg_tabbox_is_open())
        return;
    int before = s_count;
    tabbox_pull();
    if (s_count != before)
        tabbox_layout();
    else
        InvalidateRect(s_wnd, NULL, TRUE);
}

/* Switch to the selected tab and put the box away: picking a tab is a
 * destination, and staying open over the window you just went to is clutter. */
static void tabbox_activate(int index)
{
    HWND target = s_target;
    if (!target || index < 0 || index >= s_count)
        return;
    hg_tabbox_close();
    hide_taskbox(hg_g_taskbox_wnd);
    hg_tabs_activate(target, index);
}

BOOL hg_tabbox_handle_key(WPARAM key)
{
    if (!hg_tabbox_is_open())
        return FALSE;

    if (!s_focused) {
        /* Open, not entered: the digits and Tab, and nothing else. */
        if (key == VK_TAB) {
            s_focused = TRUE;
            InvalidateRect(s_wnd, NULL, TRUE);
            return TRUE;
        }
        if (key >= L'0' && key <= L'9') {
            int index = tabbox_index_for_key(key);
            if (index >= 0 && index < s_count) {
                tabbox_activate(index);
                return TRUE;
            }
        }
        return FALSE;
    }

    switch (key) {
    case VK_TAB:
        return TRUE; /* already in */
    case VK_ESCAPE:
        /* Closes the box and nothing else: the keyboard is already the
         * taskbox's, which is where the reader was. */
        hg_tabbox_close();
        return TRUE;
    case VK_UP:
        if (s_selected > 0)
            --s_selected;
        InvalidateRect(s_wnd, NULL, TRUE);
        return TRUE;
    case VK_DOWN:
        if (s_selected + 1 < s_count)
            ++s_selected;
        InvalidateRect(s_wnd, NULL, TRUE);
        return TRUE;
    case VK_HOME:
        s_selected = 0;
        InvalidateRect(s_wnd, NULL, TRUE);
        return TRUE;
    case VK_END:
        s_selected = (s_count > 0) ? s_count - 1 : 0;
        InvalidateRect(s_wnd, NULL, TRUE);
        return TRUE;
    case VK_RETURN:
    case VK_SPACE:
        tabbox_activate(s_selected);
        return TRUE;
    default:
        break;
    }

    int index = tabbox_index_for_key(key);
    if (index >= 0 && index < s_count) {
        /* A label key goes straight there - the point of labelling rows. */
        tabbox_activate(index);
        return TRUE;
    }
    return FALSE;
}

void hg_tabbox_pointer_moved(void)
{
    if (!hg_tabbox_is_open())
        return;

    POINT pt;
    if (!GetCursorPos(&pt))
        return;

    RECT box;
    GetWindowRect(s_wnd, &box);
    if (PtInRect(&box, pt) || PtInRect(&s_anchor, pt))
        return;

    hg_tabbox_close();
}

static int tabbox_row_at(POINT client_pt)
{
    double ws = hg_window_scale(s_wnd);
    int pad = SCW(ws, 6);
    int row_h = tabbox_row_height();
    if (row_h <= 0 || client_pt.y < pad)
        return -1;
    int index = (client_pt.y - pad) / row_h;
    return (index >= 0 && index < s_count) ? index : -1;
}

LRESULT CALLBACK tabbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_MOUSEACTIVATE:
        /* Clicking a row must not move the focus away from the taskbox. */
        return MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HgPaintBuffer buffer;
        HDC dc = hdc;
        BOOL buffered = hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &buffer);
        if (buffered)
            dc = buffer.dc;

        HBRUSH bg = CreateSolidBrush(hg_g_color_scheme_selected.bg);
        if (bg) {
            FillRect(dc, &rc, bg);
            DeleteObject(bg);
        }

        HFONT font = hg_g_main_font ? hg_g_main_font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT old_font = (HFONT)SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);

        double ws = hg_window_scale(hwnd);
        int pad = SCW(ws, 6);
        int row_h = tabbox_row_height();

        if (s_count <= 0) {
            SetTextColor(dc, hg_g_color_scheme_selected.text);
            RECT row = {pad, pad, rc.right - pad, pad + row_h};
            DrawTextW(dc, L"(reading tabs...)", -1, &row, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        }

        /* An entered box says so with a frame, because the keys it answers
         * change the moment it is entered. */
        if (s_focused) {
            HBRUSH frame = CreateSolidBrush(hg_g_has_system_accent_color ? hg_g_system_accent_color
                                                                        : GetSysColor(COLOR_HIGHLIGHT));
            if (frame) {
                RECT edge = rc;
                for (int i = 0; i < SCW(ws, 2); ++i) {
                    FrameRect(dc, &edge, frame);
                    InflateRect(&edge, -1, -1);
                }
                DeleteObject(frame);
            }
        }

        for (int i = 0; i < s_count; ++i) {
            RECT row = {pad, pad + i * row_h, rc.right - pad, pad + (i + 1) * row_h};
            if (i == s_selected && s_focused) {
                HBRUSH sel = CreateSolidBrush(hg_g_has_system_accent_color ? hg_g_system_accent_color
                                                                           : GetSysColor(COLOR_HIGHLIGHT));
                if (sel) {
                    FillRect(dc, &row, sel);
                    DeleteObject(sel);
                }
            }
            SetTextColor(dc, hg_g_color_scheme_selected.text);

            WCHAR line[HG_MAX_STR + 8];
            WCHAR label = tabbox_label_for(i);
            if (label)
                hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"%lc: %ls", label, s_titles[i]);
            else
                StringCchCopyW(line, HG_ARRAYSIZE(line), s_titles[i]);

            RECT text_rc = row;
            text_rc.left += SCW(ws, 4);
            DrawTextW(dc, line, -1, &text_rc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        }

        SelectObject(dc, old_font);
        if (buffered) {
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, buffer.dc, 0, 0, SRCCOPY);
            hg_paint_buffer_end(&buffer);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0 && row != s_selected) {
            s_selected = row;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0)
            tabbox_activate(row);
        return 0;
    }

    case WM_RBUTTONUP: {
        /* Right-click closes that tab, and the box stays: closing several in a
         * row is the reason anyone right-clicks a tab list. */
        POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
        int row = tabbox_row_at(pt);
        if (row >= 0 && s_target) {
            hg_tabs_close(s_target, row);
            hg_tabs_request(&s_target, 1); /* the list just changed */
        }
        return 0;
    }

    case WM_DESTROY:
        if (s_wnd == hwnd) {
            s_wnd = NULL;
            s_target = NULL;
            s_count = 0;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
