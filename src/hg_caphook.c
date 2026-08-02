/* A menu on every window's maximize button.
 *
 * See docs/RFC-2026-07-caption-button-menu.md. Three rules, and the first one
 * is the whole reason the other two exist:
 *
 * - The hook is on the desktop's input path. It runs for every mouse event on
 *   the machine, and Windows stops calling a low-level hook that takes too
 *   long. So it does one comparison and returns, and everything expensive
 *   happens only after a right button press.
 * - The hook posts; it never acts. Opening a menu inside the hook would run a
 *   modal loop inside the mouse's own input path.
 * - Asking another window anything is bounded. SendMessageTimeout with
 *   SMTO_ABORTIFHUNG, never a plain SendMessage: a hung application must cost
 *   us the menu, not the mouse. */
#include "hg_caphook.h"
#include "hg_utils.h"
#include "hg_globals.h"
#include "widgets/hg_taskbox.h"

static BOOL s_read = FALSE;
static BOOL s_enabled = FALSE;
static HHOOK s_hook = NULL;

/* The watchdog's evidence. Written only by the hook, which runs on the thread
 * that installed it - our UI thread - and read only by the watchdog, which runs
 * on that same thread's timer. One thread, so no interlocking. */
static unsigned s_callbacks = 0;

BOOL hg_caphook_enabled(void)
{
    if (!s_read) {
        s_read = TRUE;
        /* On by default: this is meant to replace a separate utility, and a
         * feature nobody can find is a feature nobody has. */
        s_enabled = (GetPrivateProfileIntW(L"etc", L"caption_menu", 1, hg_g_config_path) != 0);
    }
    return s_enabled;
}

void hg_caphook_set_enabled(BOOL enabled)
{
    (void)hg_caphook_enabled();
    s_enabled = enabled ? TRUE : FALSE;
    WritePrivateProfileStringW(L"etc", L"caption_menu", s_enabled ? L"1" : L"0", hg_g_config_path);
    hg_caphook_apply();
}

/* Is this point the maximize button of that window?
 *
 * First the window's own answer, which is right by construction. Then, only if
 * it declined to say, the geometry the system computed for its caption buttons:
 * minimize, maximize and close divide that rectangle in three, so the middle
 * third is the one. A guess, but a narrow one, and only ever a fallback. */
static BOOL caphook_is_maximize_button(HWND hwnd, POINT screen_pt)
{
    DWORD_PTR hit = 0;
    LPARAM packed = MAKELPARAM((WORD)screen_pt.x, (WORD)screen_pt.y);
    if (SendMessageTimeoutW(hwnd, WM_NCHITTEST, 0, packed, SMTO_ABORTIFHUNG, 60, &hit)) {
        if (hit == HTMAXBUTTON)
            return TRUE;
        /* A definite answer that is not the maximize button ends it; only
         * silence or a client-area answer is worth a second opinion, because an
         * application drawing its own title bar reports the whole thing as
         * client. */
        if (hit != HTCLIENT && hit != HTNOWHERE)
            return FALSE;
    }

    RECT buttons;
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_CAPTION_BUTTON_BOUNDS, &buttons, sizeof(buttons))))
        return FALSE;

    RECT window_rc;
    if (!GetWindowRect(hwnd, &window_rc))
        return FALSE;

    /* The bounds come back relative to the window. */
    LONG left = window_rc.left + buttons.left;
    LONG top = window_rc.top + buttons.top;
    LONG right = window_rc.left + buttons.right;
    LONG bottom = window_rc.top + buttons.bottom;
    LONG width = right - left;
    if (width <= 0 || bottom <= top)
        return FALSE;
    if (screen_pt.y < top || screen_pt.y >= bottom)
        return FALSE;

    LONG third = width / 3;
    if (third <= 0)
        return FALSE;
    return screen_pt.x >= left + third && screen_pt.x < left + third * 2;
}

static BOOL caphook_is_our_window(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid == GetCurrentProcessId();
}

static LRESULT CALLBACK caphook_proc(int code, WPARAM w_param, LPARAM l_param)
{
    /* One increment, so the watchdog can tell a living hook from a dead one.
     * It is the cheapest thing that can be done per event and it is the only
     * work done for the events this feature does not care about. */
    ++s_callbacks;

    /* Everything that is not a right button press leaves immediately. This
     * function is on the path of every mouse event on the desktop. */
    if (code != HC_ACTION || w_param != WM_RBUTTONDOWN)
        return CallNextHookEx(s_hook, code, w_param, l_param);

    const MSLLHOOKSTRUCT *info = (const MSLLHOOKSTRUCT *)l_param;
    if (!info)
        return CallNextHookEx(s_hook, code, w_param, l_param);

    HWND under = WindowFromPoint(info->pt);
    if (!under)
        return CallNextHookEx(s_hook, code, w_param, l_param);

    HWND top = GetAncestor(under, GA_ROOT);
    if (!top || caphook_is_our_window(top))
        return CallNextHookEx(s_hook, code, w_param, l_param);

    if (!caphook_is_maximize_button(top, info->pt))
        return CallNextHookEx(s_hook, code, w_param, l_param);

    if (!hg_g_taskbox_wnd || !IsWindow(hg_g_taskbox_wnd))
        return CallNextHookEx(s_hook, code, w_param, l_param);

    /* Posted, not shown here: a menu is a modal loop, and this is the mouse's
     * input path. */
    if (!PostMessageW(hg_g_taskbox_wnd, HG_MSG_CAPTION_MENU, (WPARAM)top, 0)) {
        /* The queue refused it, or the window went away between the check and
         * here. Eating the click now would take it and give nothing back, so it
         * goes through untouched instead. */
        return CallNextHookEx(s_hook, code, w_param, l_param);
    }

    /* Swallowed - and only now, having decided and having something to show for
     * it. Right-clicking a caption button does nothing in Windows, so nothing is
     * being taken away. */
    return 1;
}

/* The hook is a process-owned resource: Windows removes it when the process
 * ends, however it ends, so a force-kill leaves no behaviour behind and no
 * other application is left changed. That is the guarantee, and it is the
 * system's rather than ours.
 *
 * This handler does not exist because that guarantee is in doubt. It exists
 * because relying on a cleanup you never perform is how you find out it was
 * not doing what you assumed - so the hook comes out on every exit this
 * process can still run code on, and the system's cleanup stays the backstop
 * rather than the plan. */
static LPTOP_LEVEL_EXCEPTION_FILTER s_previous_filter = NULL;
static BOOL s_filter_installed = FALSE;

static LONG WINAPI caphook_unhandled_exception(EXCEPTION_POINTERS *info)
{
    hg_caphook_shutdown();
    /* Whatever was going to happen to this crash still happens: the previous
     * filter runs if there was one, and the default does if there was not. */
    return s_previous_filter ? s_previous_filter(info) : EXCEPTION_CONTINUE_SEARCH;
}

void hg_caphook_apply(void)
{
    BOOL wanted = hg_caphook_enabled();

    if (wanted && !s_hook) {
        s_hook = SetWindowsHookExW(WH_MOUSE_LL, caphook_proc, GetModuleHandleW(NULL), 0);
        if (s_hook && !s_filter_installed) {
            /* Installed with the first hook rather than at startup, so a run
             * that never turns this on never touches the process's exception
             * handling at all. */
            s_previous_filter = SetUnhandledExceptionFilter(caphook_unhandled_exception);
            s_filter_installed = TRUE;
        }
    } else if (!wanted && s_hook) {
        UnhookWindowsHookEx(s_hook);
        s_hook = NULL;
    }
}

void hg_caphook_shutdown(void)
{
    if (s_hook) {
        UnhookWindowsHookEx(s_hook);
        s_hook = NULL;
    }
}

/* Is the hook still being called?
 *
 * Windows drops a low-level hook that has been too slow and tells nobody: the
 * handle stays valid, UnhookWindowsHookEx would still succeed, and the only
 * symptom is that nothing happens any more. There is no API that answers
 * "am I still hooked", so this infers it.
 *
 * The inference has no false positives, which is the point. If the pointer has
 * moved since the last check then mouse events happened, and a hook that is
 * still installed must have been called for them. Moved but not called means
 * dropped. If the pointer has not moved we learn nothing and say nothing -
 * silence is not evidence of death, and re-installing on a hunch would churn
 * the input path for no reason.
 *
 * (A cursor moved by SetCursorPos rather than by a mouse could cost one
 * needless re-install. Re-installing is cheap and harmless; a feature that has
 * silently stopped working is neither.) */
void hg_caphook_watchdog(void)
{
    static POINT last_pt = {0, 0};
    static unsigned last_callbacks = 0;
    static BOOL primed = FALSE;
    static ULONGLONG last_check = 0;

    if (!hg_caphook_enabled() || !s_hook) {
        primed = FALSE;
        return;
    }

    /* The interval is kept here rather than in the caller's timer, so calling
     * this from more than one place - or from a timer whose rate changes - can
     * only ever be harmless. */
    ULONGLONG now = GetTickCount64();
    if (last_check != 0 && (now - last_check) < (ULONGLONG)HG_CAPHOOK_WATCHDOG_SECONDS * 1000ULL)
        return;
    last_check = now;

    POINT now_pt;
    if (!GetCursorPos(&now_pt))
        return;

    if (!primed) {
        /* First look: record and judge nothing. */
        primed = TRUE;
        last_pt = now_pt;
        last_callbacks = s_callbacks;
        return;
    }

    BOOL pointer_moved = (now_pt.x != last_pt.x || now_pt.y != last_pt.y);
    BOOL hook_ran = (s_callbacks != last_callbacks);

    last_pt = now_pt;
    last_callbacks = s_callbacks;

    if (!pointer_moved || hook_ran)
        return;

    /* Moved, and we were never called: put it back. Unhooking first because the
     * old registration may still be listed even though it is no longer being
     * called, and leaking it would stack a second one on the next round. */
    UnhookWindowsHookEx(s_hook);
    s_hook = SetWindowsHookExW(WH_MOUSE_LL, caphook_proc, GetModuleHandleW(NULL), 0);
    primed = FALSE; /* start the next judgement from a clean pair of readings */
}

/* The same entries the task icon's menu offers, from the same preset table.
 * A second list of sizes could disagree with the taskbox about what a preset
 * means, and two answers to that is worse than none. */
void hg_caphook_show_menu(HWND owner, HWND target)
{
    if (!target || !IsWindow(target))
        return;

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, HG_IDM_TASK_MOVETO_0_0, L"Move to (0, 0)");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    for (int i = 0; i < HG_RESIZE_PRESET_COUNT; ++i) {
        if (i == 3 || i == 7)
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(menu, MF_STRING, (UINT_PTR)(HG_IDM_TASK_RESIZE_4_3_1 + (UINT)i), hg_resize_presets[i].name);
    }

    /* A way out that is a click rather than a guess. Escape and a click outside
     * both dismiss a popup, but this menu appears on a button in someone else's
     * title bar, where a stray click is more likely to hit something that acts -
     * so the harmless option is on the menu, at the bottom, where a reader
     * looking for "never mind" looks. */
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, HG_IDM_CAPTION_DISMISS, L"Close");

    POINT pt;
    GetCursorPos(&pt);

    /* A popup owned by a window that is not in the foreground never gets the
     * click that should dismiss it; these two calls are the documented way
     * around that. */
    SetForegroundWindow(owner);
    int cmd = (int)TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, owner, NULL);
    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(menu);

    /* Dismissed, either by the entry or by escaping: the window is not touched.
     * Spelled out rather than left to fall through the tests below, so that
     * adding a command later cannot accidentally give this one a meaning. */
    if (cmd == 0 || cmd == HG_IDM_CAPTION_DISMISS || !IsWindow(target))
        return;

    if (cmd == HG_IDM_TASK_MOVETO_0_0) {
        SetWindowPos(target, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    int index = cmd - HG_IDM_TASK_RESIZE_4_3_1;
    if (index >= 0 && index < HG_RESIZE_PRESET_COUNT) {
        /* A maximized window ignores a size, so it comes out of that first -
         * otherwise the menu would appear to do nothing at all. */
        if (IsZoomed(target))
            ShowWindow(target, SW_RESTORE);
        SetWindowPos(target, NULL, 0, 0, hg_resize_presets[index].cx, hg_resize_presets[index].cy,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}
