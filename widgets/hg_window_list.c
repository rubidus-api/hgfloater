/* Alt-tab window list refresh and Explorer path resolution. */
#include "hg_taskbox_internal.h"
#include "../hg_utils.h"
#include "../hg_config.h"
#include "../hg_globals.h"

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

void refresh_window_list(BOOL force)
{
    int new_count = 0;
    IShellWindows *shell_windows = NULL; /* acquired lazily, once per pass */
    ZeroMemory(hg_g_new_items, sizeof(hg_g_new_items));

    /* 1단계: 기존 창 유효성 체크 및 아이콘 재사용 */
    for (int i = 0; i < hg_g_window_count; i++) {
        if (new_count >= HG_MAX_WINDOW_ITEMS)
            break;

        if (IsWindow(hg_g_window_items[i].hwnd) && is_alt_tab_window(hg_g_window_items[i].hwnd)) {
            hg_g_new_items[new_count] = (WindowItem){0};
            hg_g_new_items[new_count].hwnd = hg_g_window_items[i].hwnd;
            if (force) {
                release_window_item_icon(&hg_g_window_items[i]);
                hg_g_new_items[new_count].icon = get_window_icon(hg_g_window_items[i].hwnd, ABS(hg_g_current_font_size),
                                                                 &hg_g_new_items[new_count].own_icon);
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
                if (get_explorer_path(refresh_acquire_shell_windows(&shell_windows), hg_g_new_items[new_count].hwnd,
                                      path, HG_MAX_PATH)) {
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
                    if (get_explorer_path(refresh_acquire_shell_windows(&shell_windows), hwnd, path, HG_MAX_PATH)) {
                        StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                    }
                }

                new_count++;
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    HG_RELEASE_COM(shell_windows);

    BOOL changed = force || (new_count != hg_g_window_count);
    if (!changed) {
        for (int i = 0; i < hg_g_window_count; i++) {
            if (hg_g_new_items[i].hwnd != hg_g_window_items[i].hwnd ||
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

