/* Recorder backup (R1.8): the BakREC button, its folder box, and the move.
 *
 * The recorder: a USB voice recorder (the Sanno kind, an Actions Semiconductor
 * chip) whose drive has its recordings in \RECORDED, named
 * R<YYYYMMDD>-<HHMMSS>.WAV. Windows mounts it as an ordinary drive, so
 * everything here goes through the file system; nothing here talks to the
 * device (its clock and settings file are hg_recdev.c's, R1.10).
 *
 * A drive counts as the recorder when it is on the USB bus (the storage
 * property query, which needs no rights and no SetupAPI) and has a RECORDED
 * folder at its root. The first such drive is the one used.
 *
 * Load: nothing runs while nothing changes. Windows broadcasts a volume
 * arriving or leaving to every top-level window (WM_DEVICECHANGE with
 * DBT_DEVTYP_VOLUME); that has the next few clock ticks look. Once a minute
 * the tick compares GetLogicalDrives with the mask last looked at, as a
 * fallback for an announcement that never came, and looks only if it differs.
 *
 * The move: a worker thread copies each recording with CopyFileExW to a .part
 * name, compares the sizes, renames it, and deletes the original only then. It stops at the first
 * failure. The percent done (bytes) is the button's word while it runs. */
#define COBJMACROS /* the IFileOpenDialog_* call macros */
#include "hg_recbak.h"
#include "hg_utils.h"
#include "widgets/hg_taskbox.h"
#include <dbt.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <stdlib.h>
#include <wctype.h>
#include <winioctl.h>

#define RECBAK_FOLDER L"RECORDED"
#define RECBAK_SECTION L"recorder"
#define RECBAK_KEY L"backup_folder"
#define RECBAK_PATH_CCH 1024
#define RECBAK_POLL_TICKS 60 /* the floater's clock ticks once a second */
/* After a volume arrives, look on this many ticks until the recorder is seen:
 * a slow device can be announced before its RECORDED folder can be read, and
 * the drive mask would then match and keep the minute check from looking. */
#define RECBAK_ARRIVAL_TICKS 5

#define RECBAK_ID_PROMPT 100
#define RECBAK_ID_EDIT 101
#define RECBAK_ID_BROWSE 102

static WCHAR s_drive;            /* the recorder's drive letter, or 0 */
static DWORD s_mask_seen;        /* GetLogicalDrives when the drives were last looked at */
static BOOL s_scanned;           /* s_mask_seen is valid */
static int s_look_ticks;         /* ticks left to look on after WM_DEVICECHANGE */
static int s_ticks;

static HANDLE s_thread;
static BOOL s_busy;              /* UI thread only: set before the worker starts, cleared on its DONE */
static volatile LONG s_percent;
static volatile BOOL s_cancel;   /* CopyFileExW reads it through pbCancel */
static WCHAR s_label[8];

/* The worker's report, read by the UI thread after DONE. */
typedef struct RecbakJob {
    WCHAR drive;
    WCHAR dest[RECBAK_PATH_CCH];
    int total;           /* recordings found */
    int moved;           /* copied, checked and the original deleted */
    ULONGLONG moved_bytes;
    DWORD error;         /* 0, or the failure that stopped it */
    WCHAR error_file[MAX_PATH];
    BOOL cancelled;
    ULONGLONG total_bytes;
    ULONGLONG done_bytes; /* the files finished before the current one */
    LONG last_percent;
} RecbakJob;

static RecbakJob *s_job;

BOOL hg_recbak_present(void)
{
    return s_drive != 0 || s_busy;
}

BOOL hg_recbak_busy(void)
{
    return s_busy;
}

WCHAR hg_recbak_drive(void)
{
    return s_drive;
}

const WCHAR *hg_recbak_busy_label(void)
{
    if (!s_busy)
        return NULL;
    StringCchPrintfW(s_label, HG_ARRAYSIZE(s_label), L"%ld%%", (long)s_percent);
    return s_label;
}

/* ------------------------------------------------------------ detection */

static BOOL recbak_on_usb(WCHAR letter)
{
    WCHAR volume[] = {L'\\', L'\\', L'.', L'\\', letter, L':', 0};
    HANDLE h = CreateFileW(volume, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    STORAGE_PROPERTY_QUERY query;
    ZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    union {
        STORAGE_DEVICE_DESCRIPTOR desc;
        BYTE bytes[512];
    } out;
    ZeroMemory(&out, sizeof(out));
    DWORD got = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &out, sizeof(out), &got, NULL);
    CloseHandle(h);
    return ok && got >= sizeof(STORAGE_DEVICE_DESCRIPTOR) && out.desc.BusType == BusTypeUsb;
}

static BOOL recbak_has_folder(WCHAR letter)
{
    WCHAR path[16];
    StringCchPrintfW(path, HG_ARRAYSIZE(path), L"%lc:\\" RECBAK_FOLDER, letter);
    DWORD attr = GetFileAttributesW(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

static WCHAR recbak_find(DWORD mask)
{
    /* A card reader with no card must not raise "insert a disk". */
    DWORD old_mode = 0;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &old_mode);
    WCHAR found = 0;
    for (int i = 2; i < 26 && !found; ++i) { /* not A: or B: */
        if (!(mask & (1u << i)))
            continue;
        WCHAR letter = (WCHAR)(L'A' + i);
        WCHAR root[] = {letter, L':', L'\\', 0};
        UINT type = GetDriveTypeW(root);
        if (type != DRIVE_REMOVABLE && type != DRIVE_FIXED)
            continue;
        /* The folder first: it is one attribute read, and the drives that
         * have it are few. */
        if (recbak_has_folder(letter) && recbak_on_usb(letter))
            found = letter;
    }
    SetThreadErrorMode(old_mode, NULL);
    return found;
}

void hg_recbak_poll(BOOL force)
{
    if (s_busy)
        return; /* the drive in use stays the drive until the move is over */
    if (!force) {
        if (s_look_ticks > 0) {
            --s_look_ticks;
        } else {
            if (++s_ticks < RECBAK_POLL_TICKS)
                return;
            s_ticks = 0;
            if (s_scanned && GetLogicalDrives() == s_mask_seen)
                return;
        }
    }
    s_ticks = 0;
    DWORD mask = GetLogicalDrives();
    WCHAR letter = recbak_find(mask);
    s_mask_seen = mask;
    s_scanned = TRUE;
    BOOL was = (s_drive != 0);
    s_drive = letter;
    if (letter)
        s_look_ticks = 0;
    if (was != (letter != 0))
        hg_taskbox_buttons_changed();
}

void hg_recbak_on_device_change(WPARAM w_param, LPARAM l_param)
{
    if (w_param != DBT_DEVICEARRIVAL && w_param != DBT_DEVICEREMOVECOMPLETE)
        return;
    const DEV_BROADCAST_HDR *hdr = (const DEV_BROADCAST_HDR *)l_param;
    if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
        s_look_ticks = RECBAK_ARRIVAL_TICKS; /* from the next tick: a removal is seen at once, an arrival
                                                when its folder can be read */
}

/* ------------------------------------------------------------ the move */

static BOOL recbak_is_recording(const WCHAR *name)
{
    const WCHAR *ext = PathFindExtensionW(name);
    return lstrcmpiW(ext, L".WAV") == 0 || lstrcmpiW(ext, L".MP3") == 0;
}

/* `dir\name`, or `dir\base (n).ext` when that is taken. */
static BOOL recbak_target(const WCHAR *dir, const WCHAR *name, WCHAR *out, size_t cch)
{
    size_t n = wcslen(dir);
    const WCHAR *sep = (n && dir[n - 1] == L'\\') ? L"" : L"\\";
    if (FAILED(StringCchPrintfW(out, cch, L"%ls%ls%ls", dir, sep, name)))
        return FALSE;
    const WCHAR *dot = PathFindExtensionW(name);
    int base = (int)(dot - name);
    for (int i = 1; GetFileAttributesW(out) != INVALID_FILE_ATTRIBUTES; ++i) {
        if (i > 9999)
            return FALSE;
        if (FAILED(StringCchPrintfW(out, cch, L"%ls%ls%.*ls (%d)%ls", dir, sep, base, name, i, dot)))
            return FALSE;
    }
    return TRUE;
}

static void recbak_post_percent(RecbakJob *job, ULONGLONG bytes_now)
{
    LONG percent = job->total_bytes ? (LONG)(bytes_now * 100 / job->total_bytes) : 100;
    if (percent > 100)
        percent = 100;
    if (percent == job->last_percent)
        return;
    job->last_percent = percent;
    InterlockedExchange(&s_percent, percent);
    if (hg_g_floater_wnd)
        PostMessageW(hg_g_floater_wnd, HG_MSG_RECBAK_PROGRESS, 0, 0);
}

static DWORD CALLBACK recbak_copy_progress(LARGE_INTEGER total, LARGE_INTEGER transferred, LARGE_INTEGER stream_size,
                                           LARGE_INTEGER stream_done, DWORD stream, DWORD reason, HANDLE src,
                                           HANDLE dst, LPVOID ctx)
{
    RecbakJob *job = (RecbakJob *)ctx;
    recbak_post_percent(job, job->done_bytes + (ULONGLONG)transferred.QuadPart);
    return s_cancel ? PROGRESS_CANCEL : PROGRESS_CONTINUE;
}

typedef struct RecbakFile {
    WCHAR name[MAX_PATH];
    ULONGLONG size;
} RecbakFile;

static int recbak_by_name(const void *a, const void *b)
{
    return lstrcmpiW(((const RecbakFile *)a)->name, ((const RecbakFile *)b)->name);
}

static DWORD WINAPI recbak_worker(LPVOID param)
{
    RecbakJob *job = (RecbakJob *)param;
    DWORD old_mode = 0;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &old_mode);

    WCHAR folder[16], pattern[24];
    StringCchPrintfW(folder, HG_ARRAYSIZE(folder), L"%lc:\\" RECBAK_FOLDER, job->drive);
    StringCchPrintfW(pattern, HG_ARRAYSIZE(pattern), L"%ls\\*", folder);

    int cap = 64, count = 0;
    RecbakFile *files = (RecbakFile *)HeapAlloc(GetProcessHeap(), 0, sizeof(RecbakFile) * (size_t)cap);
    WIN32_FIND_DATAW fd;
    HANDLE find = files ? FindFirstFileW(pattern, &fd) : INVALID_HANDLE_VALUE;
    if (!files) {
        job->error = ERROR_NOT_ENOUGH_MEMORY;
    } else if (find == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e != ERROR_FILE_NOT_FOUND)
            job->error = e;
    } else {
        do {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !recbak_is_recording(fd.cFileName))
                continue;
            if (count == cap) {
                RecbakFile *more =
                    (RecbakFile *)HeapReAlloc(GetProcessHeap(), 0, files, sizeof(RecbakFile) * (size_t)cap * 2);
                if (!more) {
                    job->error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                files = more;
                cap *= 2;
            }
            StringCchCopyW(files[count].name, MAX_PATH, fd.cFileName);
            files[count].size = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            job->total_bytes += files[count].size;
            ++count;
        } while (FindNextFileW(find, &fd));
        FindClose(find);
    }
    job->total = count;
    if (count > 1)
        qsort(files, (size_t)count, sizeof(RecbakFile), recbak_by_name); /* oldest first, by the name's time */

    for (int i = 0; i < count && !job->error && !job->cancelled; ++i) {
        if (s_cancel) {
            job->cancelled = TRUE;
            break;
        }
        WCHAR src[MAX_PATH + 32], dst[RECBAK_PATH_CCH], part[RECBAK_PATH_CCH];
        StringCchPrintfW(src, HG_ARRAYSIZE(src), L"%ls\\%ls", folder, files[i].name);
        StringCchCopyW(job->error_file, MAX_PATH, files[i].name);
        size_t dn = wcslen(job->dest);
        /* Copied under a .part name and renamed when whole, so a copy cut off
         * by a crash or a power cut never sits there under the real name. */
        if (FAILED(StringCchPrintfW(part, HG_ARRAYSIZE(part), L"%ls%ls%ls.part", job->dest,
                                    (dn && job->dest[dn - 1] == L'\\') ? L"" : L"\\", files[i].name))) {
            job->error = ERROR_FILENAME_EXCED_RANGE;
            break;
        }
        if (!CopyFileExW(src, part, recbak_copy_progress, job, (LPBOOL)&s_cancel, 0)) {
            DWORD e = GetLastError();
            if (e == ERROR_REQUEST_ABORTED || s_cancel)
                job->cancelled = TRUE; /* CopyFileExW removed the partial copy */
            else
                job->error = e;
            DeleteFileW(part);
            break;
        }
        /* The original goes only when the copy is all there. */
        WIN32_FILE_ATTRIBUTE_DATA a_src, a_dst;
        if (!GetFileAttributesExW(src, GetFileExInfoStandard, &a_src) ||
            !GetFileAttributesExW(part, GetFileExInfoStandard, &a_dst)) {
            job->error = GetLastError();
            DeleteFileW(part);
            break;
        }
        if (a_src.nFileSizeHigh != a_dst.nFileSizeHigh || a_src.nFileSizeLow != a_dst.nFileSizeLow) {
            DeleteFileW(part);
            job->error = ERROR_CRC;
            break;
        }
        /* A copy keeps the original's attributes; the copy is the user's now. */
        if (a_dst.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            SetFileAttributesW(part, a_dst.dwFileAttributes & ~(DWORD)FILE_ATTRIBUTE_READONLY);
        if (!recbak_target(job->dest, files[i].name, dst, HG_ARRAYSIZE(dst))) {
            DeleteFileW(part);
            job->error = ERROR_FILE_EXISTS;
            break;
        }
        if (!MoveFileExW(part, dst, MOVEFILE_WRITE_THROUGH)) {
            job->error = GetLastError();
            DeleteFileW(part);
            break;
        }
        if (a_src.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            SetFileAttributesW(src, a_src.dwFileAttributes & ~(DWORD)FILE_ATTRIBUTE_READONLY);
        if (!DeleteFileW(src)) {
            job->error = GetLastError(); /* the copy stays; so does the original */
            break;
        }
        job->moved++;
        job->moved_bytes += files[i].size;
        job->done_bytes += files[i].size;
        recbak_post_percent(job, job->done_bytes);
    }
    if (files)
        HeapFree(GetProcessHeap(), 0, files);
    SetThreadErrorMode(old_mode, NULL);
    if (hg_g_floater_wnd)
        PostMessageW(hg_g_floater_wnd, HG_MSG_RECBAK_DONE, 0, 0);
    return 0;
}

static void recbak_repaint(void)
{
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd) && IsWindowVisible(hg_g_toolbar_wnd))
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
}

void hg_recbak_on_progress(void)
{
    recbak_repaint();
}

/* Waits for the worker and takes its report; NULL if none ran. */
static RecbakJob *recbak_join(DWORD timeout_ms)
{
    if (!s_thread)
        return NULL;
    if (WaitForSingleObject(s_thread, timeout_ms) != WAIT_OBJECT_0)
        return NULL; /* still copying: leave it, and its job, alone */
    CloseHandle(s_thread);
    s_thread = NULL;
    s_busy = FALSE;
    RecbakJob *job = s_job;
    s_job = NULL;
    return job;
}

void hg_recbak_on_done(HWND owner)
{
    RecbakJob *job = recbak_join(INFINITE);
    if (!job)
        return;

    WCHAR size_text[32] = L"";
    StrFormatByteSizeW((LONGLONG)job->moved_bytes, size_text, HG_ARRAYSIZE(size_text));
    WCHAR msg[RECBAK_PATH_CCH + 512];
    UINT icon = MB_ICONINFORMATION;
    if (job->error) {
        WCHAR reason[256] = L"";
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, job->error, 0, reason,
                       HG_ARRAYSIZE(reason), NULL);
        StrTrimW(reason, L" \r\n");
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg),
                         L"Moved %d of %d recordings (%ls) to\n%ls\n\nStopped at %ls: %ls (error %lu).\n"
                         L"That recording and the ones after it are still on the recorder.",
                         job->moved, job->total, size_text, job->dest, job->error_file, reason,
                         (unsigned long)job->error);
        icon = MB_ICONWARNING;
    } else if (job->cancelled) {
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg),
                         L"Stopped. Moved %d of %d recordings (%ls) to\n%ls\n\nThe rest are still on the recorder.",
                         job->moved, job->total, size_text, job->dest);
    } else if (job->total == 0) {
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg), L"There are no recordings in %lc:\\" RECBAK_FOLDER L".", job->drive);
    } else {
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg), L"Moved %d recordings (%ls) to\n%ls", job->moved, size_text,
                         job->dest);
    }
    HeapFree(GetProcessHeap(), 0, job);

    /* The recorder may have gone during the move; the button follows. */
    hg_recbak_poll(TRUE);
    recbak_repaint();
    MessageBoxW(owner, msg, L"hgfloater - BakREC", MB_OK | icon);
}

/* ------------------------------------------------------------ the folder box */

typedef struct RecbakAsk {
    WCHAR drive;
    WCHAR folder[RECBAK_PATH_CCH];
} RecbakAsk;

static void recbak_browse(HWND dlg)
{
    IFileOpenDialog *pick = NULL;
    if (FAILED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog,
                                (void **)&pick)))
        return;
    FILEOPENDIALOGOPTIONS opts = 0;
    IFileOpenDialog_GetOptions(pick, &opts);
    IFileOpenDialog_SetOptions(pick, opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    IFileOpenDialog_SetTitle(pick, L"Folder for the recordings");

    WCHAR current[RECBAK_PATH_CCH];
    GetDlgItemTextW(dlg, RECBAK_ID_EDIT, current, HG_ARRAYSIZE(current));
    if (PathIsDirectoryW(current)) {
        IShellItem *start = NULL;
        if (SUCCEEDED(SHCreateItemFromParsingName(current, NULL, &IID_IShellItem, (void **)&start))) {
            IFileOpenDialog_SetFolder(pick, start);
            IShellItem_Release(start);
        }
    }
    if (SUCCEEDED(IFileOpenDialog_Show(pick, dlg))) {
        IShellItem *item = NULL;
        if (SUCCEEDED(IFileOpenDialog_GetResult(pick, &item))) {
            PWSTR path = NULL;
            if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path)) && path) {
                SetDlgItemTextW(dlg, RECBAK_ID_EDIT, path);
                CoTaskMemFree(path);
            }
            IShellItem_Release(item);
        }
    }
    IFileOpenDialog_Release(pick);
}

/* The folder in the box, made usable: trimmed, full, existing (made if need
 * be), and not on the recorder. FALSE after saying what is wrong. */
static BOOL recbak_accept_folder(HWND dlg, RecbakAsk *ask)
{
    WCHAR text[RECBAK_PATH_CCH];
    GetDlgItemTextW(dlg, RECBAK_ID_EDIT, text, HG_ARRAYSIZE(text));
    StrTrimW(text, L" \t\"");
    const WCHAR *problem = NULL;
    if (!text[0])
        problem = L"Type or choose the folder to move the recordings to.";
    else if (PathIsRelativeW(text))
        problem = L"The folder has to be a full path, such as D:\\Recordings.";
    else if (towupper(text[0]) == towupper(ask->drive) && text[1] == L':')
        problem = L"That folder is on the recorder itself. Choose one on another drive.";
    if (!problem && !PathIsDirectoryW(text)) {
        int r = SHCreateDirectoryExW(dlg, text, NULL);
        if (r != ERROR_SUCCESS && r != ERROR_ALREADY_EXISTS && r != ERROR_FILE_EXISTS)
            problem = L"That folder does not exist and could not be made.";
        else if (!PathIsDirectoryW(text))
            problem = L"That path is a file, not a folder.";
    }
    if (problem) {
        MessageBoxW(dlg, problem, L"hgfloater - BakREC", MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(dlg, RECBAK_ID_EDIT));
        return FALSE;
    }
    StringCchCopyW(ask->folder, HG_ARRAYSIZE(ask->folder), text);
    return TRUE;
}

static INT_PTR CALLBACK recbak_dialog_proc(HWND dlg, UINT msg, WPARAM w_param, LPARAM l_param)
{
    RecbakAsk *ask = (RecbakAsk *)GetWindowLongPtrW(dlg, DWLP_USER);
    switch (msg) {
    case WM_INITDIALOG: {
        ask = (RecbakAsk *)l_param;
        SetWindowLongPtrW(dlg, DWLP_USER, l_param);
        WCHAR prompt[128];
        StringCchPrintfW(prompt, HG_ARRAYSIZE(prompt),
                         L"Move the recordings on the recorder (%lc:\\" RECBAK_FOLDER L") to this folder:", ask->drive);
        SetDlgItemTextW(dlg, RECBAK_ID_PROMPT, prompt);
        SendDlgItemMessageW(dlg, RECBAK_ID_EDIT, EM_LIMITTEXT, RECBAK_PATH_CCH - 1, 0);
        SetDlgItemTextW(dlg, RECBAK_ID_EDIT, ask->folder);
        SetForegroundWindow(dlg);
        HWND edit = GetDlgItem(dlg, RECBAK_ID_EDIT);
        SetFocus(edit);
        SendMessageW(edit, EM_SETSEL, 0, -1);
        return FALSE; /* focus set here */
    }
    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case RECBAK_ID_BROWSE:
            recbak_browse(dlg);
            return TRUE;
        case IDOK:
            if (ask && recbak_accept_folder(dlg, ask))
                EndDialog(dlg, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static BOOL recbak_ask_folder(HWND owner, RecbakAsk *ask)
{
    DWORD storage[1024];
    ZeroMemory(storage, sizeof(storage));
    DLGTEMPLATE *tmpl = (DLGTEMPLATE *)storage;
    tmpl->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT | DS_SETFOREGROUND | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl->dwExtendedStyle = WS_EX_TOPMOST;
    tmpl->cdit = 5;
    tmpl->cx = 280;
    tmpl->cy = 66;
    WORD *p = (WORD *)(tmpl + 1);
    *p++ = 0; /* no menu */
    *p++ = 0; /* the standard dialog class */
    p = hg_dlg_put_text(p, L"BakREC - Move Recordings");
    *p++ = 9;
    p = hg_dlg_put_text(p, L"Segoe UI");
    p = hg_dlg_put_item(p, SS_LEFT, 8, 8, 264, 10, RECBAK_ID_PROMPT, 0x0082, L"");
    p = hg_dlg_put_item(p, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 8, 22, 238, 13, RECBAK_ID_EDIT, 0x0081, L"");
    p = hg_dlg_put_item(p, BS_PUSHBUTTON | WS_TABSTOP, 250, 21, 22, 15, RECBAK_ID_BROWSE, 0x0080, L"...");
    p = hg_dlg_put_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP, 168, 44, 50, 15, IDOK, 0x0080, L"OK");
    hg_dlg_put_item(p, BS_PUSHBUTTON | WS_TABSTOP, 222, 44, 50, 15, IDCANCEL, 0x0080, L"Cancel");
    return DialogBoxIndirectParamW(GetModuleHandleW(NULL), tmpl, owner, recbak_dialog_proc, (LPARAM)ask) == IDOK;
}

void hg_recbak_click(HWND owner)
{
    if (s_busy) {
        WCHAR msg[256];
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg),
                         L"The recordings are being moved (%ld%%).\n\nStop now? The recording being copied stays on "
                         L"the recorder, and so do the ones after it.",
                         (long)s_percent);
        if (MessageBoxW(owner, msg, L"hgfloater - BakREC", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
            s_cancel = TRUE;
        return;
    }
    hg_recbak_poll(TRUE); /* the button may be a minute behind the plug */
    if (!s_drive)
        return;

    RecbakAsk *ask = (RecbakAsk *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RecbakAsk));
    if (!ask)
        return;
    ask->drive = s_drive;
    /* The folder chosen last time, back in the box. */
    GetPrivateProfileStringW(RECBAK_SECTION, RECBAK_KEY, L"", ask->folder, RECBAK_PATH_CCH, hg_g_config_path);
    BOOL go = recbak_ask_folder(owner, ask);
    if (go)
        WritePrivateProfileStringW(RECBAK_SECTION, RECBAK_KEY, ask->folder, hg_g_config_path);
    /* The recorder could have been pulled while the box was up. */
    if (go && !recbak_has_folder(ask->drive)) {
        MessageBoxW(owner, L"The recorder is no longer connected.", L"hgfloater - BakREC", MB_OK | MB_ICONWARNING);
        go = FALSE;
    }
    if (!go) {
        HeapFree(GetProcessHeap(), 0, ask);
        return;
    }

    RecbakJob *job = (RecbakJob *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RecbakJob));
    if (job) {
        job->drive = ask->drive;
        job->last_percent = -1;
        StringCchCopyW(job->dest, HG_ARRAYSIZE(job->dest), ask->folder);
    }
    HeapFree(GetProcessHeap(), 0, ask);
    if (!job)
        return;

    s_cancel = FALSE;
    InterlockedExchange(&s_percent, 0);
    s_job = job;
    s_busy = TRUE;
    s_thread = CreateThread(NULL, 0, recbak_worker, job, 0, NULL);
    if (!s_thread) {
        s_busy = FALSE;
        s_job = NULL;
        HeapFree(GetProcessHeap(), 0, job);
        MessageBoxW(owner, L"Could not start the move.", L"hgfloater - BakREC", MB_OK | MB_ICONWARNING);
        return;
    }
    recbak_repaint();
}

BOOL hg_recbak_confirm_quit(HWND owner)
{
    if (!s_busy)
        return TRUE;
    WCHAR msg[256];
    StringCchPrintfW(msg, HG_ARRAYSIZE(msg),
                     L"The recordings are still being moved (%ld%%).\n\nQuit anyway? The move stops; the recording "
                     L"being copied stays on the recorder, and so do the ones after it.",
                     (long)s_percent);
    if (MessageBoxW(owner, msg, L"hgfloater - BakREC", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return FALSE;
    s_cancel = TRUE;
    /* CopyFileExW looks at the flag every chunk, so this is quick; a device
     * that hangs mid-read is not waited for past the timeout. */
    RecbakJob *job = recbak_join(15000);
    if (job)
        HeapFree(GetProcessHeap(), 0, job);
    return TRUE;
}
