/* Shell integration: alt-tab window filtering, process introspection, and
 * the Win32/UWP/shortcut icon resolution pipeline. */
#include "hg_utils.h"
#include <knownfolders.h>

/* =========================================================================
 * Start with Windows.
 *
 * The per-user Run key, which needs no installer and no elevation, and which
 * the reader can inspect and delete without this app's help. It is the only
 * registry value hgfloater ever writes, and only when asked.
 * ========================================================================= */

#if !HG_TEMP_DISABLE_FLAGGED_FEATURES
static const WCHAR HG_RUN_KEY[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR HG_RUN_VALUE[] = L"hgfloater";

/* Quoted, because an unquoted path with a space in it is read as a command and
 * its first word as the program. */
static BOOL hg_startup_command(WCHAR *out, size_t out_cch)
{
    WCHAR path[HG_MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, path, (DWORD)HG_ARRAYSIZE(path));
    if (len == 0 || len >= HG_ARRAYSIZE(path))
        return FALSE;
    return SUCCEEDED(StringCchPrintfW(out, out_cch, L"\"%ls\"", path));
}
#endif /* !HG_TEMP_DISABLE_FLAGGED_FEATURES */

BOOL hg_startup_is_enabled(void)
{
#if HG_TEMP_DISABLE_FLAGGED_FEATURES
    /* Reported as off while the feature is suspended, so the menu draws
     * unchecked and nothing claims a state the build cannot deliver. Reading
     * the key would be harmless, but a checkmark on a disabled entry is a
     * promise that the program is starting with Windows when it is not. */
    return FALSE;
#else
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, HG_RUN_KEY, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return FALSE;

    LSTATUS status = RegQueryValueExW(key, HG_RUN_VALUE, NULL, NULL, NULL, NULL);
    RegCloseKey(key);
    return (status == ERROR_SUCCESS);
#endif
}

BOOL hg_startup_set_enabled(BOOL enabled)
{
#if HG_TEMP_DISABLE_FLAGGED_FEATURES
    /* The one place this build touches the registry, and it does not. */
    (void)enabled;
    return FALSE;
#else
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, HG_RUN_KEY, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return FALSE;

    LSTATUS status;
    if (enabled) {
        WCHAR command[HG_MAX_PATH + 4];
        if (!hg_startup_command(command, HG_ARRAYSIZE(command))) {
            RegCloseKey(key);
            return FALSE;
        }
        /* Written every time it is switched on, so an executable that has since
         * moved re-registers where it actually is. */
        DWORD bytes = (DWORD)((wcslen(command) + 1) * sizeof(WCHAR));
        status = RegSetValueExW(key, HG_RUN_VALUE, 0, REG_SZ, (const BYTE *)command, bytes);
    } else {
        status = RegDeleteValueW(key, HG_RUN_VALUE);
        if (status == ERROR_FILE_NOT_FOUND)
            status = ERROR_SUCCESS; /* already gone is the state that was asked for */
    }

    RegCloseKey(key);
    return (status == ERROR_SUCCESS);
#endif
}

/* Our own windows that are documents rather than the widget itself: a note
 * editor, the note list, the clipboard history, the command box.
 *
 * The monitor previews are WS_EX_TOOLWINDOW, which keeps them out of the
 * system's Alt-Tab - right for a widget's controls, but the taskbox is a list
 * of windows to reach, so it carries them anyway; this predicate is what lets
 * them through its filters. The note windows, the command box and the
 * clipboard history are ordinary WS_EX_APPWINDOW windows now and pass the
 * filters on their own; they stay listed here so nothing depends on that
 * distinction. The floater, the taskbox and the toolbar stay out, because they
 * are the thing you are looking at. */
static BOOL hg_is_own_reachable_window(HWND hwnd)
{
    WCHAR class_name[64];
    if (GetClassNameW(hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0)
        return FALSE;

    return lstrcmpiW(class_name, HG_CLASS_NOTE_EDIT) == 0 || lstrcmpiW(class_name, HG_CLASS_NOTE_LIST) == 0 ||
           lstrcmpiW(class_name, HG_CLASS_CLIP) == 0 || lstrcmpiW(class_name, HG_CLASS_COMMANDBOX) == 0 ||
           lstrcmpiW(class_name, HG_CLASS_MONITOR) == 0;
}

/* Windows take the digits, shortcuts take the letters.
 *
 * They used to share one run of labels - 0-9 then A-Z, all of it windows - so
 * which key opened what depended on how many windows happened to be open, and a
 * shortcut had no key at all. Splitting them makes both halves stable: Shift+3
 * is the fourth window whatever else is running, and Shift+K is the same
 * shortcut every time, because the shortcuts folder is a list the reader keeps.
 *
 * Ten windows and twenty-six shortcuts is the whole supply. Past that there is
 * no badge and no key, which is honest: a label nobody can press is furniture. */
WCHAR hg_task_badge_char(int index)
{
    if (index < 0 || index >= HG_TASK_BADGE_COUNT)
        return 0;
    return (WCHAR)(L'0' + index);
}

WCHAR hg_shortcut_badge_char(int index)
{
    if (index < 0 || index >= HG_SHORTCUT_BADGE_COUNT)
        return 0;
    return (WCHAR)(L'A' + index);
}

/* The label as it is drawn: "S0", not "0".
 *
 * The key is Shift and the label, and it always was - the bare digit moves the
 * grid. A badge that showed only the digit was telling half the truth, and the
 * missing half is the half nobody guesses. */
static BOOL badge_text_from_char(WCHAR ch, WCHAR *out, size_t out_cch)
{
    if (!out || out_cch < 4 || !ch)
        return FALSE;
    out[0] = L'S';
    out[1] = ch;
    out[2] = L'\0';
    return TRUE;
}

BOOL hg_task_badge_text(int index, WCHAR *out, size_t out_cch)
{
    return badge_text_from_char(hg_task_badge_char(index), out, out_cch);
}

BOOL hg_shortcut_badge_text(int index, WCHAR *out, size_t out_cch)
{
    return badge_text_from_char(hg_shortcut_badge_char(index), out, out_cch);
}

int hg_task_badge_index(WCHAR ch)
{
    if (ch >= L'0' && ch <= L'9')
        return ch - L'0';
    return -1;
}

int hg_shortcut_badge_index(WCHAR ch)
{
    if (ch >= L'A' && ch <= L'Z')
        return ch - L'A';
    if (ch >= L'a' && ch <= L'z')
        return ch - L'a';
    return -1;
}

BOOL is_alt_tab_window(HWND hwnd)
{
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd || hwnd == hg_g_about_wnd)
        return FALSE;
    if (!IsWindow(hwnd))
        return FALSE;
    if (!IsWindowVisible(hwnd))
        return FALSE;

    if (GetWindowTextLengthW(hwnd) == 0)
        return FALSE;

    BOOL own = hg_is_own_reachable_window(hwnd);

    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex_style & WS_EX_TOOLWINDOW) && !own)
        return FALSE;

    HWND owner_hwnd = GetWindow(hwnd, GW_OWNER);
    if (owner_hwnd != NULL && !(ex_style & WS_EX_APPWINDOW) && !own)
        return FALSE;

    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if (cloaked != 0)
            return FALSE;
    }

    return TRUE;
}

void get_process_name_by_hwnd(HWND hwnd, WCHAR *out_name, size_t out_size, DWORD *out_pid)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid)
        *out_pid = pid;

    if (out_name && out_size > 0) {
        out_name[0] = L'\0';
    }
    if (!out_name || out_size == 0 || pid == 0) {
        return;
    }

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process_handle) {
        StringCchCopyW(out_name, out_size, L"Unknown");
        return;
    }

    WCHAR path[HG_MAX_PATH] = {0};
    DWORD q_size = HG_ARRAYSIZE(path);
    if (QueryFullProcessImageNameW(process_handle, 0, path, &q_size)) {
        LPCWSTR exe_name = PathFindFileNameW(path);
        if (!exe_name || !*exe_name)
            exe_name = path;
        StringCchCopyW(out_name, out_size, exe_name);
    } else {
        StringCchCopyW(out_name, out_size, L"Unknown");
    }

    CloseHandle(process_handle);
}

void get_process_path_by_hwnd(HWND hwnd, WCHAR *out_path, size_t out_size, DWORD *out_pid)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid)
        *out_pid = pid;

    if (out_path && out_size > 0) {
        out_path[0] = L'\0';
    }
    if (!out_path || out_size == 0 || pid == 0) {
        return;
    }

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process_handle) {
        return;
    }

    DWORD q_size = (DWORD)out_size;
    QueryFullProcessImageNameW(process_handle, 0, out_path, &q_size);
    CloseHandle(process_handle);
}

static int hg_clamp_icon_size_px(int size_px)
{
    if (size_px < HG_UWP_MIN_ICON_PX)
        return HG_UWP_MIN_ICON_PX;
    if (size_px > HG_UWP_MAX_ICON_PX)
        return HG_UWP_MAX_ICON_PX;
    return size_px;
}

static BOOL CALLBACK find_uwp_child_window(HWND hwnd, LPARAM lParam)
{
    FindUWPChildData *data = (FindUWPChildData *)lParam;
    if (!data || !IsWindow(hwnd))
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == 0 || pid == data->frame_pid) {
        return TRUE;
    }

    WCHAR cls[128] = {0};
    GetClassNameW(hwnd, cls, HG_ARRAYSIZE(cls));

    if (wcscmp(cls, L"Windows.UI.Core.CoreWindow") == 0) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
        return FALSE;
    }

    if (!data->best_hwnd) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
    }

    return TRUE;
}

static BOOL get_real_uwp_child(HWND frame_hwnd, HWND *out_hwnd, DWORD *out_pid)
{
    if (out_hwnd)
        *out_hwnd = NULL;
    if (out_pid)
        *out_pid = 0;
    if (!IsWindow(frame_hwnd))
        return FALSE;

    DWORD frame_pid = 0;
    GetWindowThreadProcessId(frame_hwnd, &frame_pid);
    if (!frame_pid)
        return FALSE;

    FindUWPChildData data;
    ZeroMemory(&data, sizeof(data));
    data.frame_pid = frame_pid;

    EnumChildWindows(frame_hwnd, find_uwp_child_window, (LPARAM)&data);

    if (!data.best_hwnd || !data.best_pid || !IsWindow(data.best_hwnd)) {
        return FALSE;
    }

    if (out_hwnd)
        *out_hwnd = data.best_hwnd;
    if (out_pid)
        *out_pid = data.best_pid;
    return TRUE;
}

static HICON get_icon_from_hwnd_msg(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return NULL;

    HICON h_icon = NULL;
    DWORD_PTR res = 0;

    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon && SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon && SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    return h_icon;
}

static HICON get_icon_from_hwnd_class(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return NULL;

    HICON h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    if (!h_icon)
        h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);

    return h_icon;
}

static HICON get_icon_from_process_exe(DWORD pid, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc)
        return NULL;

    WCHAR path[HG_MAX_PATH] = {0};
    DWORD size = HG_ARRAYSIZE(path);
    HICON icon = NULL;

    if (QueryFullProcessImageNameW(h_proc, 0, path, &size)) {
        SHFILEINFOW sfi = {0};
        if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
            icon = sfi.hIcon;
            if (own_icon)
                *own_icon = TRUE;
        } else if (ExtractIconExW(path, 0, &icon, NULL, 1) > 0 && icon) {
            if (own_icon)
                *own_icon = TRUE;
        }
    }

    CloseHandle(h_proc);
    return icon;
}

static WCHAR *get_aumid_from_hwnd(HWND hwnd)
{
    if (!hwnd)
        return NULL;
    IPropertyStore *pps = NULL;
    HRESULT hr = SHGetPropertyStoreForWindow(hwnd, &IID_IPropertyStore, (void **)&pps);
    if (SUCCEEDED(hr) && pps) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        hr = pps->lpVtbl->GetValue(pps, &PKEY_AppUserModel_ID, &pv);
        WCHAR *aumid = NULL;
        if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal) {
            size_t len = wcslen(pv.pwszVal) + 1;
            aumid = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
            if (aumid) {
                StringCchCopyW(aumid, len, pv.pwszVal);
            }
        }
        PropVariantClear(&pv);
        HG_RELEASE_COM(pps);
        return aumid;
    }
    return NULL;
}

static WCHAR *get_app_user_model_id_alloc(DWORD pid)
{
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc)
        return NULL;

    UINT32 len = 0;
    LONG rc = GetApplicationUserModelId(h_proc, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        CloseHandle(h_proc);
        return NULL;
    }

    WCHAR *aumid = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!aumid) {
        CloseHandle(h_proc);
        return NULL;
    }

    rc = GetApplicationUserModelId(h_proc, &len, aumid);
    CloseHandle(h_proc);

    if (rc != ERROR_SUCCESS) {
        HG_HEAP_FREE(aumid);
        return NULL;
    }

    return aumid;
}

static WCHAR *get_package_full_name_alloc(DWORD pid)
{
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc) {
        uwp_debug_log(L"UWP icon: OpenProcess failed");
        return NULL;
    }

    UINT32 len = 0;
    LONG rc = GetPackageFullName(h_proc, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        CloseHandle(h_proc);
        return NULL;
    }

    WCHAR *full = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!full) {
        CloseHandle(h_proc);
        return NULL;
    }

    rc = GetPackageFullName(h_proc, &len, full);
    CloseHandle(h_proc);

    if (rc != ERROR_SUCCESS) {
        uwp_debug_log(L"UWP icon: GetPackageFullName failed");
        HG_HEAP_FREE(full);
        return NULL;
    }

    return full;
}

static WCHAR *get_package_path_alloc(const WCHAR *full_name)
{
    if (!full_name || !*full_name)
        return NULL;

    UINT32 len = 0;
    LONG rc = GetPackagePathByFullName(full_name, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        uwp_debug_log(L"UWP icon: GetPackagePathByFullName length query failed");
        return NULL;
    }

    WCHAR *path = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!path)
        return NULL;

    rc = GetPackagePathByFullName(full_name, &len, path);
    if (rc != ERROR_SUCCESS) {
        uwp_debug_log(L"UWP icon: GetPackagePathByFullName failed");
        HG_HEAP_FREE(path);
        return NULL;
    }

    return path;
}

static BOOL is_xml_name_char(WCHAR ch)
{
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9') || ch == L'_' ||
           ch == L'-' || ch == L':' || ch == L'.';
}

static BOOL find_xml_attr_value_safe(const WCHAR *text, const WCHAR *attr, WCHAR *out, size_t out_cch)
{
    if (!text || !attr || !*attr || !out || out_cch == 0)
        return FALSE;
    out[0] = L'\0';

    size_t attr_len = wcslen(attr);
    const WCHAR *p = text;

    while ((p = wcsstr(p, attr)) != NULL) {
        BOOL valid_prev = (p == text) || (!is_xml_name_char(*(p - 1)));
        BOOL valid_next = (!is_xml_name_char(*(p + attr_len)));

        if (valid_prev && valid_next) {
            const WCHAR *cur = p + attr_len;
            while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n')
                cur++;

            if (*cur == L'=') {
                cur++;
                while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n')
                    cur++;

                WCHAR quote = *cur;
                if (quote == L'\'' || quote == L'"') {
                    cur++;
                    const WCHAR *q = wcschr(cur, quote);
                    if (q) {
                        size_t n = (size_t)(q - cur);
                        if (n >= out_cch)
                            n = out_cch - 1;
                        if (FAILED(StringCchCopyNW(out, out_cch, cur, n))) {
                            out[0] = L'\0';
                            return FALSE;
                        }
                        out[n] = L'\0';
                        return TRUE;
                    }
                }
            }
        }

        p += attr_len;
    }

    return FALSE;
}

static BOOL read_utf8_file_to_wide(const WCHAR *path, WCHAR **out_text)
{
    if (!out_text)
        return FALSE;
    *out_text = NULL;
    if (!path || !*path)
        return FALSE;

    /* Static scratch: 64 KB buffers stacked several frames deep from window-proc
     * depth risk stack exhaustion; the UI is single-threaded. */
    static WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);

    HANDLE h = CreateFileW(norm, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER li;
    if (!GetFileSizeEx(h, &li) || li.QuadPart <= 0 || li.QuadPart > HG_UWP_MAX_MANIFEST_BYTES) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD bytes = (DWORD)li.QuadPart;
    BYTE *buf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)bytes + 1u);
    if (!buf) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD read = 0;
    BOOL ok = ReadFile(h, buf, bytes, &read, NULL);
    CloseHandle(h);

    if (!ok || read == 0) {
        HG_HEAP_FREE(buf);
        return FALSE;
    }

    UINT codepage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int wlen = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, NULL, 0);

    if (wlen <= 0) {
        codepage = CP_ACP;
        flags = 0;
        wlen = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, NULL, 0);
    }

    if (wlen <= 0 || (size_t)wlen > HG_UWP_MAX_MANIFEST_BYTES) {
        HG_HEAP_FREE(buf);
        return FALSE;
    }

    WCHAR *wbuf = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((SIZE_T)wlen + 1u) * sizeof(WCHAR));
    if (!wbuf) {
        HG_HEAP_FREE(buf);
        return FALSE;
    }

    int converted = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, wbuf, wlen);
    HG_HEAP_FREE(buf);

    if (converted != wlen) {
        HG_HEAP_FREE(wbuf);
        return FALSE;
    }

    wbuf[wlen] = L'\0';
    *out_text = wbuf;
    return TRUE;
}

static BOOL get_logo_relpath_from_manifest(const WCHAR *package_path, WCHAR *out_rel, size_t out_cch)
{
    if (!package_path || !out_rel || out_cch == 0)
        return FALSE;
    out_rel[0] = L'\0';

    WCHAR manifest_path[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(manifest_path, HG_ARRAYSIZE(manifest_path), L"%ls\\AppxManifest.xml", package_path);
    if (FAILED(hr))
        return FALSE;

    WCHAR *manifest = NULL;
    if (!read_utf8_file_to_wide(manifest_path, &manifest)) {
        uwp_debug_log(L"UWP icon: Failed to read AppxManifest.xml");
        return FALSE;
    }

    BOOL ok = find_xml_attr_value_safe(manifest, L"Square44x44Logo", out_rel, out_cch) ||
              find_xml_attr_value_safe(manifest, L"Square150x150Logo", out_rel, out_cch) ||
              find_xml_attr_value_safe(manifest, L"Logo", out_rel, out_cch);

    if (!ok)
        uwp_debug_log(L"UWP icon: Could not find logo attribute");
    HG_HEAP_FREE(manifest);
    return ok;
}

static BOOL file_exists_w(const WCHAR *path)
{
    /* Static scratch: 64 KB buffers stacked several frames deep from window-proc
     * depth risk stack exhaustion; the UI is single-threaded. */
    static WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);
    DWORD attr = GetFileAttributesW(norm);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void normalize_slashes(WCHAR *s)
{
    if (!s)
        return;
    for (; *s; s++) {
        if (*s == L'/')
            *s = L'\\';
    }
}

static BOOL resolve_logo_asset_file(const WCHAR *package_path, const WCHAR *rel_logo, int size_px, WCHAR *out_file,
                                    size_t out_cch)
{
    if (!package_path || !rel_logo || !out_file || out_cch == 0)
        return FALSE;
    out_file[0] = L'\0';
    size_px = hg_clamp_icon_size_px(size_px);

    /* Static scratch: six 64 KB locals here plus the callers' buffers approached
     * the default 1 MB stack from window-proc depth; the UI is single-threaded. */
    static WCHAR rel[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(rel, HG_ARRAYSIZE(rel), rel_logo)))
        return FALSE;
    normalize_slashes(rel);

    static WCHAR base[HG_MAX_PATH];
    if (FAILED(StringCchPrintfW(base, HG_ARRAYSIZE(base), L"%ls\\%ls", package_path, rel))) {
        return FALSE;
    }

    if (file_exists_w(base)) {
        return SUCCEEDED(StringCchCopyW(out_file, out_cch, base));
    }

    static WCHAR dir[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), base)))
        return FALSE;

    WCHAR *slash = wcsrchr(dir, L'\\');
    const WCHAR *filename = base;
    if (slash) {
        *slash = L'\0';
        filename = slash + 1;
    } else {
        if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), package_path)))
            return FALSE;
    }

    static WCHAR stem[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(stem, HG_ARRAYSIZE(stem), filename)))
        return FALSE;

    WCHAR *dot = wcsrchr(stem, L'.');
    WCHAR ext[16] = L".png";
    if (dot) {
        if (FAILED(StringCchCopyW(ext, HG_ARRAYSIZE(ext), dot)))
            return FALSE;
        *dot = L'\0';
    }

    static WCHAR candidate[HG_MAX_PATH];

    const WCHAR *dyn_patterns[] = {L"%ls\\%ls.targetsize-%d_altform-unplated%ls", L"%ls\\%ls.targetsize-%d%ls"};

    for (size_t i = 0; i < HG_ARRAYSIZE(dyn_patterns); i++) {
        if (SUCCEEDED(StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate), dyn_patterns[i], dir, stem, size_px, ext)) &&
            file_exists_w(candidate)) {
            return SUCCEEDED(StringCchCopyW(out_file, out_cch, candidate));
        }
    }

    const WCHAR *suffixes[] = {L".targetsize-16_altform-unplated",
                               L".targetsize-24_altform-unplated",
                               L".targetsize-32_altform-unplated",
                               L".targetsize-48_altform-unplated",
                               L".targetsize-256_altform-unplated",
                               L".targetsize-16",
                               L".targetsize-24",
                               L".targetsize-32",
                               L".targetsize-48",
                               L".targetsize-256",
                               L".scale-100",
                               L".scale-125",
                               L".scale-150",
                               L".scale-200",
                               L".scale-400"};

    for (size_t i = 0; i < HG_ARRAYSIZE(suffixes); i++) {
        if (SUCCEEDED(
                StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate), L"%ls\\%ls%ls%ls", dir, stem, suffixes[i], ext)) &&
            file_exists_w(candidate)) {
            return SUCCEEDED(StringCchCopyW(out_file, out_cch, candidate));
        }
    }

    static WCHAR pattern[HG_MAX_PATH];
    if (FAILED(StringCchPrintfW(pattern, HG_ARRAYSIZE(pattern), L"%ls\\%ls*.png", dir, stem))) {
        return FALSE;
    }

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if (SUCCEEDED(StringCchPrintfW(out_file, out_cch, L"%ls\\%ls", dir, fd.cFileName))) {
                    FindClose(h);
                    return TRUE;
                }
            }
        } while (FindNextFileW(h, &fd));

        FindClose(h);
    }

    uwp_debug_log(L"UWP icon: Required asset file not found");
    return FALSE;
}

static HICON create_icon_from_bitmap(HBITMAP hbmp)
{
    if (!hbmp)
        return NULL;

    BITMAP bm;
    if (!GetObjectW(hbmp, sizeof(bm), &bm)) {
        return NULL;
    }

    if (bm.bmWidth <= 0 || bm.bmHeight <= 0) {
        return NULL;
    }

    if (bm.bmWidth > 4096 || bm.bmHeight > 4096) {
        return NULL;
    }

    size_t stride_bits = ((size_t)bm.bmWidth + 15u) & ~15u;
    size_t stride_bytes = stride_bits / 8u;
    size_t mask_size = stride_bytes * (size_t)bm.bmHeight;

    BYTE *mask_bits = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mask_size);
    if (!mask_bits)
        return NULL;

    HBITMAP hmask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, mask_bits);
    HeapFree(GetProcessHeap(), 0, mask_bits);

    if (!hmask)
        return NULL;

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = TRUE;
    ii.hbmColor = hbmp;
    ii.hbmMask = hmask;

    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(hmask);
    return icon;
}

static HRESULT hg_get_shell_image_bitmap(IShellItemImageFactory *factory, SIZE sz, HBITMAP *out_bmp)
{
    HRESULT hr;
    if (!out_bmp)
        return E_POINTER;
    *out_bmp = NULL;
    if (!factory)
        return E_POINTER;

    hr = factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT | SIIGBF_THUMBNAILONLY,
                                   out_bmp);
    if (SUCCEEDED(hr) && *out_bmp)
        return hr;

    hr = factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT, out_bmp);
    if (SUCCEEDED(hr) && *out_bmp)
        return hr;

    return factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, out_bmp);
}

static HICON load_icon_from_image_file(const WCHAR *file, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!file || !*file)
        return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    IShellItemImageFactory *factory = NULL;
    HRESULT hr = SHCreateItemFromParsingName(file, NULL, &IID_IShellItemImageFactory, (void **)&factory);

    if (FAILED(hr) || !factory) {
        uwp_debug_log(L"UWP icon: SHCreateItemFromParsingName failed");
        return NULL;
    }

    SIZE sz;
    sz.cx = size_px;
    sz.cy = size_px;

    HBITMAP hbmp = NULL;
    hr = hg_get_shell_image_bitmap(factory, sz, &hbmp);
    HG_RELEASE_COM(factory);

    if (FAILED(hr) || !hbmp) {
        uwp_debug_log(L"UWP icon: IShellItemImageFactory::GetImage failed");
        return NULL;
    }

    HICON icon = create_icon_from_bitmap(hbmp);
    DeleteObject(hbmp);

    if (icon && own_icon) {
        *own_icon = TRUE;
    }

    return icon;
}

static HICON load_icon_from_aumid(const WCHAR *aumid, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!aumid || !*aumid)
        return NULL;

    static WCHAR parsing_name[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(parsing_name, HG_ARRAYSIZE(parsing_name), L"shell:AppsFolder\\%ls", aumid);
    if (FAILED(hr))
        return NULL;

    IShellItem *psi = NULL;
    hr = SHCreateItemFromParsingName(parsing_name, NULL, &IID_IShellItem, (void **)&psi);
    if (FAILED(hr) || !psi)
        return NULL;

    IShellItemImageFactory *factory = NULL;
    hr = psi->lpVtbl->QueryInterface(psi, &IID_IShellItemImageFactory, (void **)&factory);
    HG_RELEASE_COM(psi);
    if (FAILED(hr) || !factory)
        return NULL;

    SIZE sz;
    sz.cx = hg_clamp_icon_size_px(size_px);
    sz.cy = sz.cx;

    HBITMAP hbmp = NULL;
    hr = hg_get_shell_image_bitmap(factory, sz, &hbmp);
    HG_RELEASE_COM(factory);

    if (FAILED(hr) || !hbmp)
        return NULL;

    HICON icon = create_icon_from_bitmap(hbmp);
    DeleteObject(hbmp);

    if (icon && own_icon) {
        *own_icon = TRUE;
    }

    return icon;
}

static HICON get_icon_from_package_pid(DWORD pid, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    size_px = hg_clamp_icon_size_px(size_px);

    WCHAR *aumid = get_app_user_model_id_alloc(pid);
    if (aumid) {
        HICON icon = load_icon_from_aumid(aumid, size_px, own_icon);
        HG_HEAP_FREE(aumid);
        if (icon)
            return icon;
    }

    WCHAR *full_name = get_package_full_name_alloc(pid);
    if (!full_name)
        return NULL;

    WCHAR *package_path = get_package_path_alloc(full_name);
    HG_HEAP_FREE(full_name);
    if (!package_path)
        return NULL;

    static WCHAR rel_logo[HG_MAX_PATH];
    rel_logo[0] = L'\0';
    if (!get_logo_relpath_from_manifest(package_path, rel_logo, HG_ARRAYSIZE(rel_logo))) {
        HG_HEAP_FREE(package_path);
        return NULL;
    }

    static WCHAR logo_file[HG_MAX_PATH];
    logo_file[0] = L'\0';
    if (!resolve_logo_asset_file(package_path, rel_logo, size_px, logo_file, HG_ARRAYSIZE(logo_file))) {
        HG_HEAP_FREE(package_path);
        return NULL;
    }

    HG_HEAP_FREE(package_path);
    return load_icon_from_image_file(logo_file, size_px, own_icon);
}

HICON get_window_icon(HWND hwnd, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!IsWindow(hwnd))
        return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    WCHAR proc_name[HG_MAX_STR] = {0};
    get_process_name_by_hwnd(hwnd, proc_name, HG_MAX_STR, NULL);

    HICON h_icon = NULL;

    if (_wcsicmp(proc_name, L"ApplicationFrameHost.exe") == 0) {
        WCHAR *frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HG_HEAP_FREE(frame_aumid);
            if (h_icon)
                return h_icon;
        }

        HWND child_hwnd = NULL;
        DWORD child_pid = 0;

        if (get_real_uwp_child(hwnd, &child_hwnd, &child_pid)) {
            WCHAR *child_aumid = get_aumid_from_hwnd(child_hwnd);
            if (child_aumid) {
                h_icon = load_icon_from_aumid(child_aumid, size_px, own_icon);
                HG_HEAP_FREE(child_aumid);
                if (h_icon)
                    return h_icon;
            }

            h_icon = get_icon_from_package_pid(child_pid, size_px, own_icon);
            if (h_icon)
                return h_icon;

            h_icon = get_icon_from_hwnd_msg(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon)
                        *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon)
                    *own_icon = FALSE;
                return h_icon;
            }

            h_icon = get_icon_from_hwnd_class(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon)
                        *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon)
                    *own_icon = FALSE;
                return h_icon;
            }

            h_icon = get_icon_from_process_exe(child_pid, own_icon);
            if (h_icon)
                return h_icon;
        }

        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon)
                    *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon)
                *own_icon = FALSE;
            return h_icon;
        }

    } else {
        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon)
                    *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon)
                *own_icon = FALSE;
            return h_icon;
        }

        WCHAR *frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HG_HEAP_FREE(frame_aumid);
            if (h_icon)
                return h_icon;
        }

        h_icon = get_icon_from_package_pid(pid, size_px, own_icon);
        if (h_icon)
            return h_icon;
    }

    h_icon = get_icon_from_hwnd_class(hwnd);
    if (h_icon) {
        HICON h_copy = CopyIcon(h_icon);
        if (h_copy) {
            if (own_icon)
                *own_icon = TRUE;
            return h_copy;
        }
        if (own_icon)
            *own_icon = FALSE;
        return h_icon;
    }

    return get_icon_from_process_exe(pid, own_icon);
}

void release_window_item_icon(WindowItem *item)
{
    if (!item)
        return;
    if (item->own_icon && item->icon) {
        DestroyIcon(item->icon);
    }
    item->icon = NULL;
    item->own_icon = FALSE;
}

void release_shortcut_item_icon(ShortcutItem *item)
{
    if (!item)
        return;
    if (item->icon) {
        DestroyIcon(item->icon);
    }
    item->icon = NULL;
}

int compare_shortcuts(const void *a, const void *b)
{
    ShortcutItem *item_a = (ShortcutItem *)a;
    ShortcutItem *item_b = (ShortcutItem *)b;
    return lstrcmpiW(item_a->name, item_b->name);
}

void load_shortcuts_if_changed(void)
{
    static FILETIME last_write = {0, 0};
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};

    if (!hg_g_shortcuts_path[0])
        return;

    if (GetFileAttributesExW(hg_g_shortcuts_path, GetFileExInfoStandard, &attr)) {
        if (CompareFileTime(&attr.ftLastWriteTime, &last_write) == 0)
            return;
        last_write = attr.ftLastWriteTime;
    }
    load_shortcuts();
}

void load_shortcuts()
{
    for (int i = 0; i < hg_g_shortcut_count; i++) {
        release_shortcut_item_icon(&hg_g_shortcuts[i]);
    }
    ZeroMemory(hg_g_shortcuts, sizeof(hg_g_shortcuts));
    hg_g_shortcut_count = 0;
    for (int i = 0; i < hg_g_folder_count; i++) {
        release_shortcut_item_icon(&hg_g_folders[i]);
    }
    ZeroMemory(hg_g_folders, sizeof(hg_g_folders));
    hg_g_folder_count = 0;

    if (!hg_g_shortcuts_path[0])
        return;

    static WCHAR search_path[HG_MAX_PATH];
    search_path[0] = L'\0';
    if (FAILED(StringCchPrintfW(search_path, HG_ARRAYSIZE(search_path), L"%ls\\*", hg_g_shortcuts_path))) {
        return;
    }

    static WCHAR norm_search[HG_MAX_PATH];
    normalize_path_for_api(search_path, norm_search, HG_MAX_PATH);

    WIN32_FIND_DATAW ffd = {0};
    HANDLE find_handle = FindFirstFileW(norm_search, &ffd);
    if (find_handle == INVALID_HANDLE_VALUE)
        return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        if (hg_g_shortcut_count >= HG_MAX_SHORTCUTS)
            break;

        int raw_len = lstrlenW(ffd.cFileName);
        if (raw_len <= 4)
            continue;

        size_t len = (size_t)raw_len;
        BOOL is_lnk = (lstrcmpiW(ffd.cFileName + len - 4, L".lnk") == 0);
        BOOL is_url = (lstrcmpiW(ffd.cFileName + len - 4, L".url") == 0);
        if (!is_lnk && !is_url)
            continue;

        ShortcutItem *item = &hg_g_shortcuts[hg_g_shortcut_count];
        ZeroMemory(item, sizeof(*item));

        if (FAILED(StringCchPrintfW(item->path, HG_ARRAYSIZE(item->path), L"%ls\\%ls", hg_g_shortcuts_path,
                                    ffd.cFileName))) {
            continue;
        }

        if (FAILED(StringCchCopyW(item->name, HG_ARRAYSIZE(item->name), ffd.cFileName))) {
            continue;
        }
        WCHAR *dot = wcsrchr(item->name, L'.');
        if (dot)
            *dot = L'\0';

        int icon_size = ABS(hg_g_current_font_size);
        if (icon_size < SC(32))
            icon_size = SC(32);

        if (is_lnk) {
            IShellLinkW *psl = NULL;
            HRESULT hr =
                CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&psl);
            if (SUCCEEDED(hr) && psl) {
                IPersistFile *ppf = NULL;
                if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void **)&ppf)) && ppf) {
                    if (SUCCEEDED(ppf->lpVtbl->Load(ppf, item->path, STGM_READ))) {
                        WCHAR target_path[HG_MAX_PATH] = {0};
                        if (SUCCEEDED(psl->lpVtbl->GetPath(psl, target_path, HG_ARRAYSIZE(target_path), NULL, 0)) &&
                            target_path[0] != L'\0') {
                            StringCchCopyW(item->target, HG_ARRAYSIZE(item->target), target_path);
                            HICON ext_icon = NULL;
                            UINT icon_id = 0;
                            if (PrivateExtractIconsW(target_path, 0, icon_size, icon_size, &ext_icon, &icon_id, 1,
                                                     LR_LOADFROMFILE) > 0 &&
                                ext_icon) {
                                item->icon = ext_icon;
                            }
                        }
                    }
                    HG_RELEASE_COM(ppf);
                }
                HG_RELEASE_COM(psl);
            }
        }

        if (!item->icon) {
            SHFILEINFOW sfi = {0};
            if (SHGetFileInfoW(item->path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
                item->icon = sfi.hIcon;
            }
        }

        /* A shortcut whose target is a directory is a place, and places live
         * behind the Dir button. Asked of the resolved target rather than of
         * the .lnk, which is a file whatever it points at. A shortcut that does
         * not resolve - a virtual folder, a moved target - stays an icon: the
         * grid is where anything we cannot classify still works. */
        BOOL to_folder = FALSE;
        if (item->target[0]) {
            DWORD attr = GetFileAttributesW(item->target);
            to_folder = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
        }

        if (to_folder) {
            if (hg_g_folder_count < HG_MAX_SHORTCUTS) {
                hg_g_folders[hg_g_folder_count] = *item;
                hg_g_folder_count++;
            } else {
                release_shortcut_item_icon(item);
            }
            ZeroMemory(item, sizeof(*item));
            continue;
        }

        hg_g_shortcut_count++;
    } while (FindNextFileW(find_handle, &ffd));

    FindClose(find_handle);

    if (hg_g_shortcut_count > 1) {
        qsort(hg_g_shortcuts, (size_t)hg_g_shortcut_count, sizeof(ShortcutItem), compare_shortcuts);
    }
    if (hg_g_folder_count > 1) {
        qsort(hg_g_folders, (size_t)hg_g_folder_count, sizeof(ShortcutItem), compare_shortcuts);
    }
}

