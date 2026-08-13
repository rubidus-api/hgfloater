/* The window outline.
 *
 * Hovering a task icon says which window it is by name and icon; this says
 * where it is. A frame is drawn around the window itself, and nothing else
 * happens: no activation, no raise, no click taken. Pointing at something is
 * not the same as choosing it, and a switcher that rearranged the desktop
 * every time the pointer crossed an icon would be unusable.
 *
 * The whole thing is one layered popup whose interior is the colour key, so
 * only the frame is opaque. WS_EX_TRANSPARENT keeps it out of the mouse's way -
 * the outline can sit directly under the cursor's path back to the taskbox, and
 * an overlay that ate those messages would break the hover it exists to serve.
 * WS_EX_NOACTIVATE keeps it out of the keyboard's.
 */
#include "hg_hilite.h"
#include "../hg_utils.h"
#include "../hg_globals.h"

static const WCHAR HG_CLASS_HILITE[] = L"hghilite_class";

static HWND s_wnd = NULL;
static BOOL s_class_ready = FALSE;

/* Thick enough to read against a busy desktop, thin enough that it cannot be
 * mistaken for part of the window it surrounds. */
#define HG_HILITE_THICKNESS 3

static COLORREF hilite_color(void)
{
    /* The accent is what the rest of the program uses to mean "this one", and
     * on a high-contrast desktop the system's own highlight is the only colour
     * that is guaranteed to be visible. */
    if (!hg_g_is_high_contrast && hg_g_has_system_accent_color)
        return hg_g_system_accent_color;
    return GetSysColor(COLOR_HIGHLIGHT);
}

static LRESULT CALLBACK hilite_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH key = hg_cached_solid_brush(HG_TRANSPARENT_KEY);
        if (key)
            FillRect(hdc, &rc, key);

        int t = SCW(hg_window_scale(hwnd), HG_HILITE_THICKNESS);
        HBRUSH frame = hg_cached_solid_brush(hilite_color());
        if (frame) {
            RECT top = {0, 0, rc.right, t};
            RECT bottom = {0, rc.bottom - t, rc.right, rc.bottom};
            RECT left = {0, t, t, rc.bottom - t};
            RECT right = {rc.right - t, t, rc.right, rc.bottom - t};
            FillRect(hdc, &top, frame);
            FillRect(hdc, &bottom, frame);
            FillRect(hdc, &left, frame);
            FillRect(hdc, &right, frame);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    if (msg == WM_DESTROY) {
        if (s_wnd == hwnd)
            s_wnd = NULL;
        return 0;
    }

    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

static BOOL hilite_ensure_window(void)
{
    if (s_wnd && IsWindow(s_wnd))
        return TRUE;

    if (!s_class_ready) {
        WNDCLASSEXW wc;
        SecureZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = hilite_proc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = HG_CLASS_HILITE;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return FALSE;
        s_class_ready = TRUE;
    }

    s_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                            HG_CLASS_HILITE, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!s_wnd)
        return FALSE;

    /* Colour key only: the frame is fully opaque and the middle is fully gone.
     * An alpha here would fade the frame as well, which is the one part that
     * has to be seen. */
    SetLayeredWindowAttributes(s_wnd, HG_TRANSPARENT_KEY, 255, LWA_COLORKEY);
    return TRUE;
}

/* The window's visible rectangle. GetWindowRect returns the resize border too -
 * invisible since Windows 10 and several pixels wide - so an outline drawn on
 * it stands noticeably away from the window on every side. */
static BOOL hilite_target_rect(HWND target, RECT *out)
{
    if (SUCCEEDED(DwmGetWindowAttribute(target, DWMWA_EXTENDED_FRAME_BOUNDS, out, sizeof(*out))) &&
        out->right > out->left && out->bottom > out->top)
        return TRUE;
    return GetWindowRect(target, out) && out->right > out->left && out->bottom > out->top;
}

void hg_hilite_show(HWND target)
{
    /* A minimized window has no place on the desktop to point at, and a frame
     * around where it used to be would be a lie. Nothing is drawn, and nothing
     * is moved: the outline simply stays away. */
    if (!target || !IsWindow(target) || IsIconic(target) || !IsWindowVisible(target)) {
        hg_hilite_hide();
        return;
    }

    RECT rc;
    if (!hilite_target_rect(target, &rc)) {
        hg_hilite_hide();
        return;
    }

    if (!hilite_ensure_window())
        return;

    /* Outside the window rather than over it, so the outline adds to what is
     * on screen instead of hiding the window's own edge. */
    int t = SCW(hg_window_scale(s_wnd), HG_HILITE_THICKNESS);
    InflateRect(&rc, t, t);

    /* HWND_TOPMOST each time, which lands it at the top of the topmost band -
     * above the taskbox, which is topmost too. Buried windows get their outline
     * all the same: it says where the window is, not what is in front of it. */
    SetWindowPos(s_wnd, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(s_wnd, NULL, TRUE);
}

void hg_hilite_hide(void)
{
    if (s_wnd && IsWindow(s_wnd))
        ShowWindow(s_wnd, SW_HIDE);
}

void hg_hilite_shutdown(void)
{
    if (s_wnd && IsWindow(s_wnd))
        DestroyWindow(s_wnd);
    s_wnd = NULL;
}
