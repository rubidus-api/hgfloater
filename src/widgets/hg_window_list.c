/* Alt-tab window list refresh and Explorer path resolution. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"
#include "../hg_tabs.h"

static BOOL get_explorer_path(IShellWindows *psw, HWND target_hwnd, WCHAR *out_path, int max_len)
{
    if (!psw || !out_path || max_len <= 0)
        return FALSE;
    out_path[0] = L'\0';

    long count = 0;
    if (FAILED(psw->lpVtbl->get_Count(psw, &count))) {
        return FALSE;
    }

    BOOL found = FALSE;
    for (long i = 0; i < count; i++) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = i;
        IDispatch *pdisp = NULL;
        if (SUCCEEDED(psw->lpVtbl->Item(psw, v, &pdisp)) && pdisp) {
            IWebBrowser2 *pwb = NULL;
            if (SUCCEEDED(pdisp->lpVtbl->QueryInterface(pdisp, &IID_IWebBrowser2, (void **)&pwb)) && pwb) {
                /* Try reading hw as long or INT_PTR depending on headers. SHANDLE_PTR is standard */
                LONG_PTR hw = 0;
                if (SUCCEEDED(pwb->lpVtbl->get_HWND(pwb, &hw)) && (HWND)hw == target_hwnd) {
                    BSTR url = NULL;
                    if (SUCCEEDED(pwb->lpVtbl->get_LocationURL(pwb, &url)) && url) {
                        DWORD pc_len = (DWORD)max_len;
                        if (SUCCEEDED(PathCreateFromUrlW(url, out_path, &pc_len, 0))) {
                            found = TRUE;
                        } else {
                            /* Might be search-ms: or other specialized protocol; fallback */
                            BSTR title = NULL;
                            if (SUCCEEDED(pwb->lpVtbl->get_LocationName(pwb, &title)) && title) {
                                lstrcpynW(out_path, title, max_len);
                                release_bstr(&title);
                            } else {
                                lstrcpynW(out_path, url, max_len);
                            }
                            found = TRUE;
                        }
                        release_bstr(&url);
                    }
                }
                HG_RELEASE_COM(pwb);
            }
            HG_RELEASE_COM(pdisp);
        }
        if (found)
            break;
    }
    return found;
}

/* One IShellWindows instance per refresh pass; creating it per Explorer window
 * made every one-second refresh quadratic in COM traffic. */
static IShellWindows *refresh_acquire_shell_windows(IShellWindows **psw)
{
    if (!*psw) {
        if (FAILED(CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, (void **)psw))) {
            *psw = NULL;
        }
    }
    return *psw;
}

/* The resolved path is remembered per window: the IShellWindows walk above is
 * cross-process COM per collection entry, and it used to run once per Explorer
 * window per second. The answer only changes when the user navigates, and
 * navigating renames the window - so the raw title, already in hand for free,
 * is the revalidation key. A failure is remembered the same way: a window
 * whose path cannot be resolved will not resolve any better one second later,
 * only after it changes. */
#define HG_EXPLORER_CACHE 32
static BOOL explorer_path_for(IShellWindows **shell_windows, HWND hwnd, const WCHAR *raw_title, WCHAR *out,
                              int out_cch)
{
    typedef struct ExplorerPathEntry {
        HWND hwnd;
        BOOL resolved;
        WCHAR title[HG_MAX_STR];
        WCHAR path[HG_MAX_PATH];
    } ExplorerPathEntry;
    static ExplorerPathEntry s_cache[HG_EXPLORER_CACHE];
    static int s_next = 0;

    for (int i = 0; i < HG_EXPLORER_CACHE; ++i) {
        if (s_cache[i].hwnd == hwnd && lstrcmpW(s_cache[i].title, raw_title) == 0) {
            if (s_cache[i].resolved)
                StringCchCopyW(out, (size_t)out_cch, s_cache[i].path);
            return s_cache[i].resolved;
        }
    }

    ExplorerPathEntry *slot = NULL;
    for (int i = 0; i < HG_EXPLORER_CACHE; ++i) {
        if (s_cache[i].hwnd == NULL || s_cache[i].hwnd == hwnd || !IsWindow(s_cache[i].hwnd)) {
            slot = &s_cache[i];
            break;
        }
    }
    if (!slot) {
        slot = &s_cache[s_next];
        s_next = (s_next + 1) % HG_EXPLORER_CACHE;
    }

    slot->hwnd = hwnd;
    StringCchCopyW(slot->title, HG_ARRAYSIZE(slot->title), raw_title);
    slot->resolved =
        get_explorer_path(refresh_acquire_shell_windows(shell_windows), hwnd, slot->path, HG_MAX_PATH);
    if (slot->resolved)
        StringCchCopyW(out, (size_t)out_cch, slot->path);
    return slot->resolved;
}

/* One item per tab, for the windows that have them, in place of the one item
 * the window would have contributed.
 *
 * Its own clock: the enumeration is a cross-process call and the list refreshes
 * every second, so tabs are re-read every fifth pass and reused in between. A
 * tab title five seconds stale costs nobody anything; a taskbox that stutters
 * once a second costs everybody. */
static int expand_window_tabs(WindowItem *items, int count, BOOL force)
{
    if (!hg_tabs_enabled())
        return count;

    static int ticks = 0;
    BOOL requery = force || (++ticks >= 5);
    if (requery)
        ticks = 0;

    /* Titles are remembered between passes so the four seconds in between cost
     * nothing at all, keyed by the window they came from. */
#define HG_TABS_CACHE_WINDOWS 16
    static HWND cached_hwnd[HG_TABS_CACHE_WINDOWS];
    static WCHAR cached_titles[HG_TABS_CACHE_WINDOWS][HG_TABS_MAX_PER_WINDOW][HG_MAX_STR];
    static int cached_count[HG_TABS_CACHE_WINDOWS];
    static int cache_used = 0;

    /* Static, not local. A WindowItem is about 4 KB - two HG_MAX_STR strings -
     * so a thousand of them is a four-megabyte stack frame on a one-megabyte
     * stack, which is a crash at the first call and not a subtle one. The rest
     * of this file keeps its item arrays in static storage for exactly this
     * reason; this function has to as well. Only ever the UI thread. */
    static WindowItem out[HG_MAX_WINDOW_ITEMS];
    int out_count = 0;
    int consumed = 0;

    for (int i = 0; i < count && out_count < HG_MAX_WINDOW_ITEMS; ++i, consumed = i) {
        int tabs = 0;
        int slot = -1;

        if (hg_tabs_window_may_have_tabs(items[i].hwnd)) {
            for (int c = 0; c < cache_used; ++c) {
                if (cached_hwnd[c] == items[i].hwnd) {
                    slot = c;
                    break;
                }
            }
            if (slot < 0 && cache_used < (int)HG_ARRAYSIZE(cached_hwnd)) {
                slot = cache_used++;
                cached_hwnd[slot] = items[i].hwnd;
                cached_count[slot] = 0;
                requery = TRUE; /* a window we have not asked about yet */
            }
            if (slot >= 0) {
                if (requery)
                    cached_count[slot] = hg_tabs_enumerate(items[i].hwnd, cached_titles[slot],
                                                           HG_TABS_MAX_PER_WINDOW);
                tabs = cached_count[slot];
            }
        }

        /* One tab is a window with a tab strip nobody is using; showing it as a
         * tab item would say something the reader cannot act on. */
        if (tabs <= 1 || slot < 0) {
            out[out_count++] = items[i];
            continue;
        }

        for (int t = 0; t < tabs && out_count < HG_MAX_WINDOW_ITEMS; ++t) {
            out[out_count] = items[i];
            out[out_count].is_tab = TRUE;
            out[out_count].tab_index = t;
            StringCchCopyW(out[out_count].title, HG_MAX_STR, cached_titles[slot][t]);

            /* The icon handle is owned exactly once, by the first tab, which
             * inherits the window's handle and its ownership flag. The rest
             * share the same handle without owning it: every item of a fan-out
             * is rebuilt together each pass and released together when the
             * window goes, so the share can never outlive the owner - and it
             * replaces a CopyIcon plus DestroyIcon per extra tab per second. */
            if (t > 0) {
                out[out_count].icon = items[i].icon;
                out[out_count].own_icon = FALSE;
            }
            ++out_count;
        }
    }

    /* A full output table ends the loop with input items still unconsumed, and
     * every one of those may own its icon handle. They are about to be
     * overwritten by the copy-back, so this is the last moment to let go. */
    for (int i = consumed; i < count; ++i)
        release_window_item_icon(&items[i]);

    /* Windows that have gone leave their cache entry behind; compact it here so
     * a long session does not fill the table with dead handles. */
    int kept = 0;
    for (int c = 0; c < cache_used; ++c) {
        if (!IsWindow(cached_hwnd[c]))
            continue;
        if (kept != c) {
            cached_hwnd[kept] = cached_hwnd[c];
            cached_count[kept] = cached_count[c];
            memcpy(cached_titles[kept], cached_titles[c], sizeof(cached_titles[c]));
        }
        ++kept;
    }
    cache_used = kept;

    for (int i = 0; i < out_count; ++i)
        items[i] = out[i];
    return out_count;
}

void refresh_window_list(BOOL force)
{
    int new_count = 0;
    IShellWindows *shell_windows = NULL; /* acquired lazily, once per pass */
    ZeroMemory(hg_g_new_items, sizeof(hg_g_new_items));

    /* 1단계: 기존 창 유효성 체크 및 아이콘 재사용 */
    for (int i = 0; i < hg_g_window_count; i++) {
        if (new_count >= HG_MAX_WINDOW_ITEMS) {
            /* No room left: the remaining old items are dropped, and each may
             * still own its icon. Breaking here would leak them. */
            release_window_item_icon(&hg_g_window_items[i]);
            continue;
        }

        /* The previous pass may have expanded a window into several tab items.
         * Reuse rebuilds windows, not tabs, so a window that is already in the
         * new list is skipped and the expansion below does the fan-out again. */
        BOOL already_added = FALSE;
        for (int j = 0; j < new_count; ++j) {
            if (hg_g_new_items[j].hwnd == hg_g_window_items[i].hwnd) {
                already_added = TRUE;
                break;
            }
        }
        if (already_added) {
            release_window_item_icon(&hg_g_window_items[i]);
            continue;
        }

        if (IsWindow(hg_g_window_items[i].hwnd) && is_alt_tab_window(hg_g_window_items[i].hwnd)) {
            hg_g_new_items[new_count] = (WindowItem){0};
            hg_g_new_items[new_count].hwnd = hg_g_window_items[i].hwnd;
            BOOL retry_now = TRUE;
            if (!force && !hg_g_window_items[i].icon && hg_g_window_items[i].icon_retry_wait > 0) {
                /* Still waiting out the backoff from earlier failures. */
                hg_g_new_items[new_count].icon_misses = hg_g_window_items[i].icon_misses;
                hg_g_new_items[new_count].icon_retry_wait = hg_g_window_items[i].icon_retry_wait - 1;
                retry_now = FALSE;
            }
            if (retry_now && (force || !hg_g_window_items[i].icon)) {
                /* A window asked for its icon while it was still starting up can
                 * answer nothing, and the old code kept that nothing for the life
                 * of the window. Asking again stops a permanently blank cell -
                 * but each ask blocks on the target window and reads disk, so a
                 * window that keeps failing earns a growing pause: 3 quick
                 * retries, then doubling waits capped near half a minute. */
                release_window_item_icon(&hg_g_window_items[i]);
                hg_g_new_items[new_count].icon = get_window_icon(hg_g_window_items[i].hwnd, ABS(hg_g_current_font_size),
                                                                 &hg_g_new_items[new_count].own_icon);
                if (hg_g_new_items[new_count].icon) {
                    hg_g_new_items[new_count].icon_misses = 0;
                    hg_g_new_items[new_count].icon_retry_wait = 0;
                } else {
                    int misses = hg_g_window_items[i].icon_misses + 1;
                    hg_g_new_items[new_count].icon_misses = misses;
                    if (misses > 3) {
                        int wait = 1 << ((misses - 3 < 5) ? (misses - 3) : 5);
                        hg_g_new_items[new_count].icon_retry_wait = wait;
                    }
                }
            } else if (!retry_now) {
                /* keep nothing this pass; the fields above already carried over */
            } else {
                hg_g_new_items[new_count].icon = hg_g_window_items[i].icon;
                hg_g_new_items[new_count].own_icon = hg_g_window_items[i].own_icon;
                hg_g_window_items[i].icon = NULL;
                hg_g_window_items[i].own_icon = FALSE;
            }

            hg_g_new_items[new_count].exists = TRUE;
            StringCchCopyW(hg_g_new_items[new_count].process_name, HG_MAX_STR, hg_g_window_items[i].process_name);
            hg_g_new_items[new_count].process_id = hg_g_window_items[i].process_id;

            if (GetWindowTextW(hg_g_new_items[new_count].hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                hg_g_new_items[new_count].title[0] = L'\0';
            }

            if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                static WCHAR path[HG_MAX_PATH];
                if (explorer_path_for(&shell_windows, hg_g_new_items[new_count].hwnd,
                                      hg_g_new_items[new_count].title, path, HG_MAX_PATH)) {
                    StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                }
            }

            new_count++;
        } else {
            release_window_item_icon(&hg_g_window_items[i]);
        }
    }

    /* 2단계: 새로 나타난 창 추가 */
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        if (is_alt_tab_window(hwnd)) {
            BOOL exists = FALSE;
            for (int i = 0; i < new_count; i++) {
                if (hg_g_new_items[i].hwnd == hwnd) {
                    exists = TRUE;
                    break;
                }
            }

            if (!exists) {
                if (new_count >= HG_MAX_WINDOW_ITEMS)
                    break;

                hg_g_new_items[new_count] = (WindowItem){0};
                hg_g_new_items[new_count].hwnd = hwnd;
                hg_g_new_items[new_count].icon =
                    get_window_icon(hwnd, ABS(hg_g_current_font_size), &hg_g_new_items[new_count].own_icon);
                hg_g_new_items[new_count].exists = TRUE;
                get_process_name_by_hwnd(hwnd, hg_g_new_items[new_count].process_name, HG_MAX_STR,
                                         &hg_g_new_items[new_count].process_id);

                if (GetWindowTextW(hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                    hg_g_new_items[new_count].title[0] = L'\0';
                }

                if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                    static WCHAR path[HG_MAX_PATH];
                    if (explorer_path_for(&shell_windows, hwnd, hg_g_new_items[new_count].title, path,
                                          HG_MAX_PATH)) {
                        StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                    }
                }

                new_count++;
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    HG_RELEASE_COM(shell_windows);

    new_count = expand_window_tabs(hg_g_new_items, new_count, force);

    BOOL changed = force || (new_count != hg_g_window_count);
    if (!changed) {
        for (int i = 0; i < hg_g_window_count; i++) {
            if (hg_g_new_items[i].hwnd != hg_g_window_items[i].hwnd ||
                hg_g_new_items[i].tab_index != hg_g_window_items[i].tab_index ||
                lstrcmpW(hg_g_new_items[i].title, hg_g_window_items[i].title) != 0) {
                changed = TRUE;
                break;
            }
        }
    }

    hg_g_window_count = new_count;
    for (int i = 0; i < hg_g_window_count; i++) {
        hg_g_window_items[i] = hg_g_new_items[i];
    }

    if (changed && hg_g_taskbox_wnd) {
        update_layout(hg_g_taskbox_wnd);
        if (hg_g_toolbar_wnd)
            InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
}

