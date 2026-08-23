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
#include "../hg_options.h"

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

/* Rows enough for the longest of the three lists: the folders can be as many as
 * the shortcuts folder holds, which is more than any window has tabs. */
#define HG_BOX_MAX_ROWS HG_MAX_SHORTCUTS
static WCHAR s_titles[HG_BOX_MAX_ROWS][HG_MAX_STR];
static int s_count = 0;
static int s_selected = 0;
static int s_mode = HG_BOX_TABS;
static BOOL s_open = FALSE;

/* The control rows, in the order the buttons stood in on the row. Volume first
 * because it is the one reached most often and the top row is the shortest
 * distance from the button. */
static const int s_control_ids[] = {HG_TOOL_ICON_VOLUME, HG_TOOL_ICON_BRIGHTNESS, HG_TOOL_ICON_ALPHA,
                                    HG_TOOL_ICON_PIN, HG_TOOL_ICON_MENU};

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

    /* Above or below the icon, flush against it, never beside it: the icon is
     * what the reader is pointing at and what they will point at next, so the
     * one thing this box must not cover is that. Four placements - the box
     * running right or left from the icon, sitting under it or over it - and
     * the first that fits the work area whole is the one used.
     *
     * Running "right" means the box's left edge starts at the icon's left, so
     * the icon's whole width lands inside the box's width and the two read as
     * attached. Running "left" mirrors it from the icon's right edge. */
    HMONITOR monitor = MonitorFromRect(&s_anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    RECT work = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    if (GetMonitorInfoW(monitor, &mi))
        work = mi.rcWork;

    int x_right = s_anchor.left;          /* box runs right from the icon's left edge */
    int x_left = s_anchor.right - width;  /* box runs left from the icon's right edge */
    BOOL fits_right = (x_right >= work.left) && (x_right + width <= work.right);
    BOOL fits_left = (x_left >= work.left) && (x_left + width <= work.right);

    int x;
    if (fits_right)
        x = x_right;
    else if (fits_left)
        x = x_left;
    else {
        /* Wider than the display it is on. Nothing to choose between, so keep
         * it on screen; the icon stays uncovered either way, because what
         * decides that is above-or-below and not this. */
        x = work.left;
        if (width > work.right - work.left)
            width = work.right - work.left;
    }

    int y_below = s_anchor.bottom;         /* flush under the icon */
    int y_above = s_anchor.top - height;   /* flush over it */
    BOOL fits_below = (y_below + height <= work.bottom) && (y_below >= work.top);
    BOOL fits_above = (y_above >= work.top) && (y_above + height <= work.bottom);

    int y;
    if (fits_below)
        y = y_below;
    else if (fits_above)
        y = y_above;
    else {
        /* Too tall for either side. Take the roomier one and give up the rows
         * that do not fit rather than any part of the icon: a clamped y would
         * slide the box back over the thing the reader is pointing at. */
        int room_below = work.bottom - s_anchor.bottom;
        int room_above = s_anchor.top - work.top;
        if (room_below >= room_above) {
            y = s_anchor.bottom;
            height = (room_below > 0) ? room_below : 0;
        } else {
            height = (room_above > 0) ? room_above : 0;
            y = s_anchor.top - height;
        }
    }

    SetWindowPos(s_wnd, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(s_wnd, NULL, TRUE);
}

/* Fill the rows for whichever list is up.
 *
 * Three sources, one row model: a string per row, a count, and a selection.
 * Everything past this function - the placement, the painting, the keys, the
 * clicking - is written against that and does not know which list it has. */
static void tabbox_pull(void)
{
    if (s_mode == HG_BOX_TABS) {
        if (!s_target)
            return;
        HgTabsAnswer answer;
        int fresh = hg_tabs_take_result(s_target, s_titles, HG_TABS_MAX_PER_WINDOW, &answer);
        if (fresh >= 0 && !answer.failed)
            s_count = fresh;
    } else if (s_mode == HG_BOX_DIRS) {
        s_count = 0;
        for (int i = 0; i < hg_g_folder_count && s_count < HG_BOX_MAX_ROWS; ++i) {
            StringCchCopyW(s_titles[s_count], HG_MAX_STR, hg_g_folders[i].name);
            ++s_count;
        }
    } else {
        s_count = 0;
        for (size_t i = 0; i < HG_ARRAYSIZE(s_control_ids) && s_count < HG_BOX_MAX_ROWS; ++i) {
            int id = s_control_ids[i];
            const WCHAR *label = hg_toolbar_builtin_label(id);
            int pct = 0;

            /* Name once, then the number: the reading is the reason to open the
             * box, and the row is what the wheel is spinning. The two rows that
             * are not values say their state in the same shape. */
            if (hg_toolbar_value_percent(id, &pct)) {
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-4ls %3d%%%ls", label, pct,
                                   (id == HG_TOOL_ICON_VOLUME && get_system_mute()) ? L"  (muted)" : L"");
            } else if (id == HG_TOOL_ICON_PIN) {
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-4ls %ls", label,
                                   hg_g_taskbox_pinned ? L"on" : L"off");
            } else {
                hellgates_wsprintf(s_titles[s_count], HG_MAX_STR, L"%-4ls %ls", label, L"options menu");
            }
            ++s_count;
        }
    }

    if (s_selected >= s_count)
        s_selected = (s_count > 0) ? s_count - 1 : 0;
}

/* The window, made once and reused by all three lists. */
static BOOL tabbox_ensure_window(void)
{
    if (s_wnd)
        return TRUE;
    s_wnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, HG_CLASS_TABBOX, L"Tabs",
                            WS_POPUP | WS_BORDER, 0, 0, 0, 0, hg_g_taskbox_wnd, NULL, GetModuleHandle(NULL),
                            NULL);
    return s_wnd != NULL;
}

void hg_tabbox_open(HWND target, const RECT *anchor_screen_rc)
{
    /* Switched off means switched off wherever the ask came from - the mouse in
     * the toolbar, the keyboard focus in the grid - so the gate is here rather
     * than at each call. */
    if (!hg_g_tabbox_on_hover) {
        hg_tabbox_close();
        return;
    }

    if (!target || !IsWindow(target) || !anchor_screen_rc)
        return;
    if (!hg_tabs_enabled() || !hg_tabs_window_may_have_tabs(target))
        return;

    if (!tabbox_ensure_window())
        return;

    if (s_mode != HG_BOX_TABS) {
        s_mode = HG_BOX_TABS;
        s_target = NULL;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE;
    }
    s_open = TRUE;

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

/* The two lists that belong to a button rather than to a window. Neither asks
 * anything of the tab reader, and neither is gated by the tabs option: that
 * switch is about hovering a task icon, which is not what these are. */
static void tabbox_open_list(int mode, const RECT *anchor_screen_rc)
{
    if (!anchor_screen_rc || !tabbox_ensure_window())
        return;

    if (s_mode != mode) {
        s_mode = mode;
        s_target = NULL;
        s_count = 0;
        s_selected = 0;
        s_focused = FALSE;
    }
    s_open = TRUE;
    s_anchor = *anchor_screen_rc;
    tabbox_pull();
    tabbox_layout();
}

void hg_tabbox_open_dirs(const RECT *anchor_screen_rc)
{
    tabbox_open_list(HG_BOX_DIRS, anchor_screen_rc);
}

void hg_tabbox_open_controls(const RECT *anchor_screen_rc)
{
    tabbox_open_list(HG_BOX_CONTROLS, anchor_screen_rc);
}

int hg_tabbox_mode(void)
{
    return hg_tabbox_is_open() ? s_mode : HG_BOX_TABS;
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
    s_open = FALSE;
    s_mode = HG_BOX_TABS;
}

BOOL hg_tabbox_is_open(void)
{
    return s_wnd && IsWindow(s_wnd) && IsWindowVisible(s_wnd) && s_open;
}

HWND hg_tabbox_target(void)
{
    return (hg_tabbox_is_open() && s_mode == HG_BOX_TABS) ? s_target : NULL;
}

HWND hg_tabbox_window(void)
{
    return s_wnd;
}

void hg_tabbox_refresh(void)
{
    if (!hg_tabbox_is_open() || s_mode != HG_BOX_TABS)
        return;
    int before = s_count;
    tabbox_pull();
    if (s_count != before)
        tabbox_layout();
    else
        InvalidateRect(s_wnd, NULL, TRUE);
}

/* What a row does when it is picked.
 *
 * A tab and a folder are both destinations, so the box and the taskbox both get
 * out of the way - staying open over the thing you just went to is clutter. A
 * control is not a destination: the box stays, because the reason to open it is
 * usually to move a value and then look at what moved. */
static void tabbox_activate(int index)
{
    if (index < 0 || index >= s_count)
        return;

    if (s_mode == HG_BOX_TABS) {
        HWND target = s_target;
        if (!target)
            return;
        hg_tabbox_close();
        hide_taskbox(hg_g_taskbox_wnd);
        hg_tabs_activate(target, index);
        return;
    }

    if (s_mode == HG_BOX_DIRS) {
        if (index >= hg_g_folder_count)
            return;
        WCHAR path[HG_MAX_PATH];
        StringCchCopyW(path, HG_ARRAYSIZE(path), hg_g_folders[index].target[0] ? hg_g_folders[index].target
                                                                              : hg_g_folders[index].path);
        hg_tabbox_close();
        hide_taskbox(hg_g_taskbox_wnd);
        ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
        return;
    }

    if (index >= (int)HG_ARRAYSIZE(s_control_ids))
        return;
    int id = s_control_ids[index];

    /* The same call the buttons made when they were on the row. Mute, the pin
     * and the options menu did not change by moving in here, and writing them
     * out a second time is how two copies of one behaviour start to differ. */
    if (hg_toolbar_builtin_click_role(id) == HG_TOOLBAR_CLICK_OPEN_MENU) {
        /* The menu is modal and would sit under a box that cannot lose a focus
         * it never had, so the box goes first. */
        hg_tabbox_close();
        activate_toolbar_item(id);
        return;
    }
    activate_toolbar_item(id);

    tabbox_pull();
    if (s_wnd)
        InvalidateRect(s_wnd, NULL, TRUE);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
}

BOOL hg_tabbox_pointer_over(void)
{
    if (!hg_tabbox_is_open())
        return FALSE;
    POINT pt;
    RECT box;
    if (!GetCursorPos(&pt) || !GetWindowRect(s_wnd, &box))
        return FALSE;
    return PtInRect(&box, pt);
}

BOOL hg_tabbox_handle_wheel(short delta)
{
    if (!hg_tabbox_is_open() || s_mode != HG_BOX_CONTROLS || !hg_tabbox_pointer_over())
        return FALSE;

    POINT pt;
    if (!GetCursorPos(&pt))
        return FALSE;
    ScreenToClient(s_wnd, &pt);

    double ws = hg_window_scale(s_wnd);
    int pad = SCW(ws, 6);
    int row_h = tabbox_row_height();
    int row = (row_h > 0 && pt.y >= pad) ? (pt.y - pad) / row_h : -1;
    if (row < 0 || row >= s_count || row >= (int)HG_ARRAYSIZE(s_control_ids))
        return FALSE;

    if (!hg_toolbar_value_wheel(s_control_ids[row], delta))
        return FALSE;

    s_selected = row;
    tabbox_pull();
    InvalidateRect(s_wnd, NULL, TRUE);
    if (hg_g_toolbar_wnd)
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    return TRUE;
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
            const WCHAR *empty = (s_mode == HG_BOX_TABS)
                                     ? L"(reading tabs...)"
                                     : (s_mode == HG_BOX_DIRS)
                                           ? L"(no folder shortcuts - put one in the shortcuts folder)"
                                           : L"(nothing to show)";
            DrawTextW(dc, empty, -1, &row, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
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
        if (row >= 0 && s_target && s_mode == HG_BOX_TABS) {
            hg_tabs_close(s_target, row);
            hg_tabs_request(&s_target, 1); /* the list just changed */
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
        /* Reached when the pointer is over the box and something gave it the
         * message; the toolbar forwards the ones that arrive there instead. */
        if (hg_tabbox_handle_wheel((short)HIWORD(w_param)))
            return 0;
        break;

    case WM_DESTROY:
        if (s_wnd == hwnd) {
            s_wnd = NULL;
            s_target = NULL;
            s_count = 0;
            s_open = FALSE;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
