/* The recorder's clock and settings file (R1.10), from BakREC's right-click
 * menu. See hg_recdev.h for the shape; the facts about the device come from
 * the recorder's own tools as ReteTime's protocol notes record them:
 *
 * - USB vendor id 0x10D6 (Actions Semiconductor); product id 0xFF77 is the
 *   firmware-update mode and is never touched.
 * - The clock: CDB B0 00 00 00 00 00 00 07 00 ... (16 bytes), host to device,
 *   seven bytes of local time: year (little-endian 16 bits), month, day, hour,
 *   minute, second. Done only when the device answers SCSI status 0 - the
 *   vendor's tool looked at DeviceIoControl alone and could report success
 *   for a refusal.
 * - The settings file `Seo&0h.txt` in the drive's root: exactly 1024 bytes,
 *   ASCII, CRLF lines of KEY:value, then tabs and a (comment), padded with
 *   spaces. A value is changed in place; tabs, comments and lines this program
 *   does not know are kept as they were. */
#include "hg_recdev.h"
#include "hg_recbak.h"
#include "hg_utils.h"
#include "widgets/hg_floater.h"
#include <cfgmgr32.h>
#include <ntddscsi.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include <wctype.h>
#include <winioctl.h>

#define RECDEV_TITLE L"hgfloater - BakREC"
#define RECDEV_CLI_SWITCH L"--rec-clock"
#define RECDEV_VID 0x10D6
#define RECDEV_PID_FIRMWARE 0xFF77
#define RECDEV_CONFIG_NAME L"Seo&0h.txt"
#define RECDEV_CONFIG_SIZE 1024
#define RECDEV_FLASH_TICKS 10 /* five flashes at 100 ms a half */

/* ------------------------------------------------------------ which device */

/* GUID_DEVINTERFACE_DISK, here so no header needs initguid.h. */
static const GUID recdev_disk_interface = {0x53f56307, 0xb6bf, 0x11d0, {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

static BOOL recdev_disk_number(const WCHAR *path, DWORD *number)
{
    HANDLE h = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    STORAGE_DEVICE_NUMBER n;
    DWORD got = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &n, sizeof(n), &got, NULL);
    CloseHandle(h);
    if (!ok || n.DeviceType != FILE_DEVICE_DISK)
        return FALSE;
    *number = n.DeviceNumber;
    return TRUE;
}

/* The device node of the disk with that number, or 0. */
static DEVINST recdev_disk_devinst(DWORD number)
{
    DEVINST found = 0;
    HDEVINFO set = SetupDiGetClassDevsW(&recdev_disk_interface, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE)
        return 0;
    SP_DEVICE_INTERFACE_DATA ifd;
    ifd.cbSize = sizeof(ifd);
    for (DWORD i = 0; !found && SetupDiEnumDeviceInterfaces(set, NULL, &recdev_disk_interface, i, &ifd); ++i) {
        DWORD need = 0;
        SetupDiGetDeviceInterfaceDetailW(set, &ifd, NULL, 0, &need, NULL);
        if (need == 0)
            continue;
        SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail =
            (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)HeapAlloc(GetProcessHeap(), 0, need);
        if (!detail)
            break;
        detail->cbSize = sizeof(*detail);
        SP_DEVINFO_DATA dev;
        dev.cbSize = sizeof(dev);
        DWORD n = 0;
        if (SetupDiGetDeviceInterfaceDetailW(set, &ifd, detail, need, NULL, &dev) &&
            recdev_disk_number(detail->DevicePath, &n) && n == number)
            found = dev.DevInst;
        HeapFree(GetProcessHeap(), 0, detail);
    }
    SetupDiDestroyDeviceInfoList(set);
    return found;
}

/* The USB ids of the device the drive is on: up from the disk to the first
 * USB\VID_xxxx&PID_yyyy node. */
static BOOL recdev_usb_ids(WCHAR drive, unsigned *vid, unsigned *pid)
{
    WCHAR volume[] = {L'\\', L'\\', L'.', L'\\', drive, L':', 0};
    DWORD number = 0;
    if (!recdev_disk_number(volume, &number))
        return FALSE;
    DEVINST inst = recdev_disk_devinst(number);
    for (int depth = 0; inst && depth < 8; ++depth) {
        WCHAR id[MAX_DEVICE_ID_LEN];
        if (CM_Get_Device_IDW(inst, id, MAX_DEVICE_ID_LEN, 0) == CR_SUCCESS && _wcsnicmp(id, L"USB\\VID_", 8) == 0 &&
            swscanf(id + 8, L"%4x&PID_%4x", vid, pid) == 2)
            return TRUE;
        DEVINST parent;
        if (CM_Get_Parent(&parent, inst, 0) != CR_SUCCESS)
            break;
        inst = parent;
    }
    return FALSE;
}

/* ERROR_SUCCESS when the drive is the recorder the clock command is for. */
static DWORD recdev_check_device(WCHAR drive, unsigned *vid, unsigned *pid)
{
    *vid = *pid = 0;
    if (!recdev_usb_ids(drive, vid, pid))
        return ERROR_DEV_NOT_EXIST;
    if (*vid != RECDEV_VID || *pid == RECDEV_PID_FIRMWARE)
        return ERROR_NOT_SUPPORTED;
    return ERROR_SUCCESS;
}

/* ------------------------------------------------------------ the clock */

typedef struct RecdevSpt {
    SCSI_PASS_THROUGH spt;
    ULONG align;
    UCHAR sense[32];
    UCHAR data[16];
} RecdevSpt;

/* Sends this computer's local time, read now. */
static DWORD recdev_send_clock(WCHAR drive, SYSTEMTIME *sent)
{
    WCHAR volume[] = {L'\\', L'\\', L'.', L'\\', drive, L':', 0};
    HANDLE h = CreateFileW(volume, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return GetLastError();

    SYSTEMTIME t;
    GetLocalTime(&t);
    RecdevSpt b;
    ZeroMemory(&b, sizeof(b));
    b.spt.Length = sizeof(b.spt);
    b.spt.CdbLength = 16;
    b.spt.SenseInfoLength = sizeof(b.sense);
    b.spt.DataIn = SCSI_IOCTL_DATA_OUT;
    b.spt.DataTransferLength = 7;
    b.spt.TimeOutValue = 20;
    b.spt.DataBufferOffset = offsetof(RecdevSpt, data);
    b.spt.SenseInfoOffset = (ULONG)offsetof(RecdevSpt, sense);
    b.spt.Cdb[0] = 0xB0;
    b.spt.Cdb[7] = 7;
    b.data[0] = (UCHAR)(t.wYear & 0xFF);
    b.data[1] = (UCHAR)(t.wYear >> 8);
    b.data[2] = (UCHAR)t.wMonth;
    b.data[3] = (UCHAR)t.wDay;
    b.data[4] = (UCHAR)t.wHour;
    b.data[5] = (UCHAR)t.wMinute;
    b.data[6] = (UCHAR)t.wSecond;
    DWORD got = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_SCSI_PASS_THROUGH, &b, sizeof(b), &b, sizeof(b), &got, NULL);
    DWORD err = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(h);
    if (err != ERROR_SUCCESS)
        return err;
    if (b.spt.ScsiStatus != 0)
        return ERROR_GEN_FAILURE; /* the device refused it */
    if (sent)
        *sent = t;
    return ERROR_SUCCESS;
}

static BOOL s_clock_pending; /* an elevated copy is running */

typedef struct RecdevWait {
    HANDLE process;
} RecdevWait;

static DWORD WINAPI recdev_wait_thread(LPVOID param)
{
    RecdevWait *w = (RecdevWait *)param;
    DWORD code = ERROR_GEN_FAILURE;
    WaitForSingleObject(w->process, INFINITE);
    if (!GetExitCodeProcess(w->process, &code))
        code = GetLastError();
    CloseHandle(w->process);
    HeapFree(GetProcessHeap(), 0, w);
    if (hg_g_floater_wnd)
        PostMessageW(hg_g_floater_wnd, HG_MSG_RECCLOCK_DONE, (WPARAM)code, 0);
    return 0;
}

/* What recdev_check_device found wrong, after `lead` ("The recorder's clock
 * was not set"); FALSE when the code is not one of its. */
static BOOL recdev_device_problem(DWORD code, unsigned vid, unsigned pid, const WCHAR *lead, WCHAR *msg, size_t cch)
{
    if (code == ERROR_NOT_SUPPORTED) {
        StringCchPrintfW(msg, cch,
                         L"%ls: this drive is not the recorder this is for (USB id %04X:%04X; expected vendor "
                         L"%04X, not in firmware-update mode).",
                         lead, vid, pid, RECDEV_VID);
        return TRUE;
    }
    if (code == ERROR_DEV_NOT_EXIST) {
        StringCchPrintfW(msg, cch, L"%ls: the drive's USB device could not be found.", lead);
        return TRUE;
    }
    return FALSE;
}

static void recdev_clock_failed(HWND owner, DWORD code, unsigned vid, unsigned pid)
{
    WCHAR msg[512];
    if (recdev_device_problem(code, vid, pid, L"The recorder's clock was not set", msg, HG_ARRAYSIZE(msg)))
        ; /* said */
    else if (code == ERROR_GEN_FAILURE) {
        StringCchCopyW(msg, HG_ARRAYSIZE(msg), L"The recorder did not accept the clock command.");
    } else if (code == ERROR_ACCESS_DENIED) {
        StringCchCopyW(msg, HG_ARRAYSIZE(msg),
                       L"The recorder's clock was not set: Windows refused the command even with administrator "
                       L"rights.");
    } else {
        StringCchPrintfW(msg, HG_ARRAYSIZE(msg), L"The recorder's clock was not set (error %lu).",
                         (unsigned long)code);
    }
    MessageBoxW(owner, msg, RECDEV_TITLE, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

static void recdev_clock_done(void)
{
    append_message(L"Recorder clock set to this computer's time");
    hg_floater_flash(RECDEV_FLASH_TICKS);
}

/* The second copy, elevated, with nothing to show: the result comes back as
 * its exit code. */
static void recdev_launch_elevated(HWND owner, WCHAR drive)
{
    WCHAR exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, exe, HG_ARRAYSIZE(exe));
    if (n == 0 || n >= HG_ARRAYSIZE(exe))
        return;
    WCHAR params[32];
    StringCchPrintfW(params, HG_ARRAYSIZE(params), RECDEV_CLI_SWITCH L" %lc", drive);

    SHELLEXECUTEINFOW info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    info.hwnd = owner;
    info.lpVerb = L"runas";
    info.lpFile = exe;
    info.lpParameters = params;
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info)) {
        DWORD e = GetLastError();
        if (e != ERROR_CANCELLED) /* declining the UAC prompt is an answer */
            recdev_clock_failed(owner, e, 0, 0);
        return;
    }
    if (!info.hProcess)
        return;
    RecdevWait *w = (RecdevWait *)HeapAlloc(GetProcessHeap(), 0, sizeof(RecdevWait));
    HANDLE thread = NULL;
    if (w) {
        w->process = info.hProcess;
        thread = CreateThread(NULL, 0, recdev_wait_thread, w, 0, NULL);
    }
    if (!thread) {
        /* No way to hear back; the copy still does its job. */
        CloseHandle(info.hProcess);
        if (w)
            HeapFree(GetProcessHeap(), 0, w);
        return;
    }
    CloseHandle(thread);
    s_clock_pending = TRUE;
}

void hg_recdev_set_clock(HWND owner, WCHAR drive)
{
    if (!drive || s_clock_pending)
        return;
    unsigned vid = 0, pid = 0;
    DWORD e = recdev_check_device(drive, &vid, &pid);
    if (e == ERROR_SUCCESS)
        e = recdev_send_clock(drive, NULL);
    if (e == ERROR_SUCCESS)
        recdev_clock_done();
    else if (e == ERROR_ACCESS_DENIED)
        recdev_launch_elevated(owner, drive);
    else
        recdev_clock_failed(owner, e, vid, pid);
}

void hg_recdev_on_clock_done(HWND owner, DWORD code)
{
    s_clock_pending = FALSE;
    if (code == ERROR_SUCCESS) {
        recdev_clock_done();
    } else {
        unsigned vid = 0, pid = 0;
        WCHAR drive = hg_recbak_drive();
        if (code == ERROR_NOT_SUPPORTED && drive)
            recdev_usb_ids(drive, &vid, &pid);
        recdev_clock_failed(owner, code, vid, pid);
    }
}

int hg_recdev_cli(LPCWSTR cmd_line)
{
    if (!cmd_line || !cmd_line[0])
        return -1;
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(cmd_line, &argc);
    if (!argv)
        return -1;
    int result = -1;
    for (int i = 0; i < argc; ++i) {
        if (lstrcmpiW(argv[i], RECDEV_CLI_SWITCH) != 0)
            continue;
        result = (int)ERROR_INVALID_PARAMETER;
        const WCHAR *arg = (i + 1 < argc) ? argv[i + 1] : NULL;
        WCHAR drive = arg ? (WCHAR)towupper(arg[0]) : 0;
        if (drive >= L'C' && drive <= L'Z' && (arg[1] == 0 || (arg[1] == L':' && arg[2] == 0))) {
            unsigned vid = 0, pid = 0;
            DWORD e = recdev_check_device(drive, &vid, &pid);
            if (e == ERROR_SUCCESS)
                e = recdev_send_clock(drive, NULL);
            result = (int)e;
        }
        break;
    }
    LocalFree(argv);
    return result;
}

/* ------------------------------------------------------------ the settings file */

#define RECDEV_MAX_LINES 48
#define RECDEV_LINE_CCH 200

typedef struct RecdevConfig {
    char lines[RECDEV_MAX_LINES][RECDEV_LINE_CCH];
    int count;
} RecdevConfig;

/* The recorder's standard file, for a recorder that has none. */
static const char recdev_template[] =
    "VOR:0\t\t\t(0:Voice-activated off 1-7:Voice-activated sensitivity,Higher means record less)\r\n"
    "BIT RATE:4\t\t(0:32Kbps 1:64Kbps 2:128Kbps 3:192Kbps 4:Translate ON 5:512Kbps 6:768Kbps 7:1024Kbps "
    "8:1536Kbps)\r\n"
    "GAIN:2\t\t\t(0-7 Record sensitivity 8 grades,The higher value, The louder sound)\r\n"
    "SECTION:(300)\t\t(0-999 Record time exceed this,file will auto save,uint minutes,0 means no this function)\r\n"
    "DATE:05-02-2026\t\t(Day-Month-Year)\r\n"
    "TIME:12:00:00\t\t(Hour:Minute:Second)\r\n"
    "TIMER:0\t\t\t(Timed record 1:On 0:Off)\r\n"
    "START:16:18:43\t\t(Timed record start time,Hour:Minute:Second)\r\n"
    "TIMELONG:(120)\t\t(1-720,Timed record length,Unit is minute)\r\n"
    "CYCLE:(030)\t\t(1-999,Recording cycle days,0:Everyday)";

/* Lines split at CRLF, the space padding dropped. FALSE for a file this cannot
 * keep intact (a NUL inside, too many or too long lines). */
static BOOL recdev_config_parse(RecdevConfig *c, const char *data, size_t len)
{
    c->count = 0;
    while (len > 0 && (data[len - 1] == ' ' || data[len - 1] == 0))
        --len;
    size_t from = 0;
    for (;;) {
        size_t nl = from;
        while (nl + 1 < len && !(data[nl] == '\r' && data[nl + 1] == '\n'))
            ++nl;
        BOOL last = (nl + 1 >= len);
        size_t stop = last ? len : nl;
        if (c->count == RECDEV_MAX_LINES || stop - from >= RECDEV_LINE_CCH)
            return FALSE;
        if (memchr(data + from, 0, stop - from))
            return FALSE;
        memcpy(c->lines[c->count], data + from, stop - from);
        c->lines[c->count][stop - from] = 0;
        c->count++;
        if (last)
            break;
        from = nl + 2;
    }
    return TRUE;
}

/* The line whose key (before the colon, spaces trimmed, any case) is `key`. */
static int recdev_config_find(const RecdevConfig *c, const char *key)
{
    size_t key_len = strlen(key);
    for (int i = 0; i < c->count; ++i) {
        const char *line = c->lines[i];
        const char *colon = strchr(line, ':');
        if (!colon)
            continue;
        const char *a = line, *b = colon;
        while (a < b && *a == ' ')
            ++a;
        while (b > a && b[-1] == ' ')
            --b;
        if ((size_t)(b - a) == key_len && _strnicmp(a, key, key_len) == 0)
            return i;
    }
    return -1;
}

/* The value: after the colon, up to the first tab, spaces and the brackets
 * some values wear taken off. */
static BOOL recdev_config_get(const RecdevConfig *c, const char *key, char *out, size_t cap)
{
    int i = recdev_config_find(c, key);
    if (i < 0)
        return FALSE;
    const char *v = strchr(c->lines[i], ':') + 1;
    const char *tab = strchr(v, '\t');
    size_t n = tab ? (size_t)(tab - v) : strlen(v);
    while (n && *v == ' ') {
        ++v;
        --n;
    }
    while (n && v[n - 1] == ' ')
        --n;
    if (n >= 2 && v[0] == '(' && v[n - 1] == ')') {
        ++v;
        n -= 2;
    }
    if (n >= cap)
        n = cap - 1;
    memcpy(out, v, n);
    out[n] = 0;
    return TRUE;
}

/* Replaces the value, keeping the key, the tabs and the comment. */
static BOOL recdev_config_set(RecdevConfig *c, const char *key, const char *value)
{
    int i = recdev_config_find(c, key);
    if (i < 0)
        return FALSE;
    char *line = c->lines[i];
    char *colon = strchr(line, ':');
    const char *tail = strchr(colon + 1, '\t');
    char next[RECDEV_LINE_CCH];
    int n = snprintf(next, sizeof(next), "%.*s%s%s", (int)(colon - line + 1), line, value, tail ? tail : "");
    if (n < 0 || n >= (int)sizeof(next))
        return FALSE;
    memcpy(line, next, (size_t)n + 1);
    return TRUE;
}

/* CRLF between the lines, none after the last, spaces up to the file's size. */
static BOOL recdev_config_serialize(const RecdevConfig *c, char *out, size_t size)
{
    size_t n = 0;
    for (int i = 0; i < c->count; ++i) {
        size_t k = strlen(c->lines[i]);
        if (n + (i ? 2 : 0) + k > size)
            return FALSE;
        if (i) {
            out[n++] = '\r';
            out[n++] = '\n';
        }
        memcpy(out + n, c->lines[i], k);
        n += k;
    }
    memset(out + n, ' ', size - n);
    return TRUE;
}

static void recdev_config_path(WCHAR drive, WCHAR *out, size_t cch)
{
    StringCchPrintfW(out, cch, L"%lc:\\" RECDEV_CONFIG_NAME, drive);
}

/* ERROR_FILE_NOT_FOUND when the recorder has no settings file. */
static DWORD recdev_config_read(WCHAR drive, RecdevConfig *c)
{
    WCHAR path[32];
    recdev_config_path(drive, path, HG_ARRAYSIZE(path));
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        return (e == ERROR_PATH_NOT_FOUND) ? ERROR_FILE_NOT_FOUND : e;
    }
    char buf[4096];
    DWORD got = 0;
    BOOL ok = ReadFile(h, buf, sizeof(buf), &got, NULL);
    DWORD e = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(h);
    if (e != ERROR_SUCCESS)
        return e;
    if (got == sizeof(buf) || !recdev_config_parse(c, buf, got))
        return ERROR_INVALID_DATA;
    return ERROR_SUCCESS;
}

static DWORD recdev_config_write(WCHAR drive, const RecdevConfig *c)
{
    char buf[RECDEV_CONFIG_SIZE];
    if (!recdev_config_serialize(c, buf, sizeof(buf)))
        return ERROR_FILE_TOO_LARGE;
    WCHAR path[32];
    recdev_config_path(drive, path, HG_ARRAYSIZE(path));
    /* Opened, not re-created, so the file keeps its place and attributes. */
    HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return GetLastError();
    DWORD put = 0;
    BOOL ok = WriteFile(h, buf, sizeof(buf), &put, NULL) && put == sizeof(buf) && SetEndOfFile(h) &&
              FlushFileBuffers(h);
    DWORD e = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(h);
    return (ok || e) ? e : ERROR_WRITE_FAULT;
}

/* ------------------------------------------------------------ the settings box */

typedef enum RecdevKind { RECDEV_NUMBER, RECDEV_CHOICE, RECDEV_DATE, RECDEV_TIME } RecdevKind;

typedef struct RecdevField {
    const char *key;
    const WCHAR *label;
    RecdevKind kind;
    int min, max;
    BOOL bracketed; /* stored as "(nnn)" */
    const WCHAR *const *choices;
} RecdevField;

static const WCHAR *const recdev_bit_rates[] = {
    L"32 kbps", L"64 kbps", L"128 kbps", L"192 kbps", L"Translate ON", L"512 kbps", L"768 kbps", L"1024 kbps",
    L"1536 kbps", NULL,
};
static const WCHAR *const recdev_on_off[] = {L"Off", L"On", NULL};

static const RecdevField recdev_fields[] = {
    {"VOR", L"Voice activation (0 off, 1-7; higher records less)", RECDEV_NUMBER, 0, 7, FALSE, NULL},
    {"BIT RATE", L"Bit rate", RECDEV_CHOICE, 0, 8, FALSE, recdev_bit_rates},
    {"GAIN", L"Recording gain (0-7, higher is louder)", RECDEV_NUMBER, 0, 7, FALSE, NULL},
    {"SECTION", L"Split into files every N minutes (0 off)", RECDEV_NUMBER, 0, 999, TRUE, NULL},
    {"DATE", L"Date in the file (DD-MM-YYYY)", RECDEV_DATE, 0, 0, FALSE, NULL},
    {"TIME", L"Time in the file (HH:MM:SS)", RECDEV_TIME, 0, 0, FALSE, NULL},
    {"TIMER", L"Timed recording", RECDEV_CHOICE, 0, 1, FALSE, recdev_on_off},
    {"START", L"Timed recording starts at (HH:MM:SS)", RECDEV_TIME, 0, 0, FALSE, NULL},
    {"TIMELONG", L"Timed recording length (1-720 minutes)", RECDEV_NUMBER, 1, 720, TRUE, NULL},
    {"CYCLE", L"Timed recording every N days (0 every day)", RECDEV_NUMBER, 0, 999, TRUE, NULL},
};
#define RECDEV_FIELD_COUNT ((int)HG_ARRAYSIZE(recdev_fields))

#define RECDEV_ID_PROMPT 100
#define RECDEV_ID_NOW 101
#define RECDEV_ID_LABEL0 200
#define RECDEV_ID_FIELD0 300

typedef struct RecdevForm {
    WCHAR drive;
    BOOL missing; /* no file yet: the template is shown, Save makes the file */
    RecdevConfig config;
} RecdevForm;

static BOOL recdev_parse_int(const char *s, int *v)
{
    char *end = NULL;
    while (*s == ' ')
        ++s;
    if (!*s)
        return FALSE;
    long x = strtol(s, &end, 10);
    while (*end == ' ')
        ++end;
    if (*end || x < -100000 || x > 100000)
        return FALSE;
    *v = (int)x;
    return TRUE;
}

static int recdev_two(const char *s)
{
    return (s[0] >= '0' && s[0] <= '9' && s[1] >= '0' && s[1] <= '9') ? (s[0] - '0') * 10 + (s[1] - '0') : -1;
}

static BOOL recdev_valid_time(const char *s)
{
    return strlen(s) == 8 && s[2] == ':' && s[5] == ':' && recdev_two(s) >= 0 && recdev_two(s) <= 23 &&
           recdev_two(s + 3) >= 0 && recdev_two(s + 3) <= 59 && recdev_two(s + 6) >= 0 && recdev_two(s + 6) <= 59;
}

static BOOL recdev_valid_date(const char *s)
{
    static const int days[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (strlen(s) != 10 || s[2] != '-' || s[5] != '-')
        return FALSE;
    int d = recdev_two(s), m = recdev_two(s + 3), y1 = recdev_two(s + 6), y2 = recdev_two(s + 8);
    if (d < 1 || m < 1 || m > 12 || y1 < 0 || y2 < 0)
        return FALSE;
    int y = y1 * 100 + y2;
    BOOL leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    int max = (m == 2 && !leap) ? 28 : days[m - 1];
    return y >= 1980 && y <= 2099 && d <= max;
}

static void recdev_form_fill(HWND dlg, const RecdevForm *form)
{
    for (int i = 0; i < RECDEV_FIELD_COUNT; ++i) {
        const RecdevField *f = &recdev_fields[i];
        HWND ctl = GetDlgItem(dlg, RECDEV_ID_FIELD0 + i);
        char value[64];
        BOOL have = recdev_config_get(&form->config, f->key, value, sizeof(value));
        if (!have) {
            /* Not in this recorder's file: shown, not offered. */
            EnableWindow(ctl, FALSE);
            EnableWindow(GetDlgItem(dlg, RECDEV_ID_LABEL0 + i), FALSE);
            continue;
        }
        if (f->kind == RECDEV_CHOICE) {
            for (int k = 0; f->choices[k]; ++k)
                SendMessageW(ctl, CB_ADDSTRING, 0, (LPARAM)f->choices[k]);
            int v = 0;
            if (recdev_parse_int(value, &v) && v >= f->min && v <= f->max)
                SendMessageW(ctl, CB_SETCURSEL, (WPARAM)(v - f->min), 0);
        } else {
            WCHAR text[64];
            int v = 0;
            if (f->kind == RECDEV_NUMBER && recdev_parse_int(value, &v))
                StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%d", v);
            else
                StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%hs", value);
            SendMessageW(ctl, EM_LIMITTEXT, 10, 0);
            SetWindowTextW(ctl, text);
        }
    }
}

/* The fields into the file's lines; FALSE after saying which one is wrong. */
static BOOL recdev_form_take(HWND dlg, RecdevForm *form)
{
    RecdevConfig next = form->config;
    for (int i = 0; i < RECDEV_FIELD_COUNT; ++i) {
        const RecdevField *f = &recdev_fields[i];
        HWND ctl = GetDlgItem(dlg, RECDEV_ID_FIELD0 + i);
        if (!IsWindowEnabled(ctl))
            continue;
        char out[32];
        const WCHAR *problem = NULL;
        if (f->kind == RECDEV_CHOICE) {
            LRESULT sel = SendMessageW(ctl, CB_GETCURSEL, 0, 0);
            if (sel == CB_ERR)
                continue; /* an unknown value, left as it was */
            StringCchPrintfA(out, sizeof(out), "%d", f->min + (int)sel);
        } else {
            WCHAR wtext[32];
            char text[32];
            GetWindowTextW(ctl, wtext, HG_ARRAYSIZE(wtext));
            StrTrimW(wtext, L" \t");
            int n = WideCharToMultiByte(CP_ACP, 0, wtext, -1, text, sizeof(text), NULL, NULL);
            if (n <= 0)
                text[0] = 0;
            int v = 0;
            if (f->kind == RECDEV_NUMBER) {
                if (!recdev_parse_int(text, &v) || v < f->min || v > f->max)
                    problem = L"Type a whole number in the range shown.";
                else if (f->bracketed)
                    StringCchPrintfA(out, sizeof(out), "(%03d)", v);
                else
                    StringCchPrintfA(out, sizeof(out), "%d", v);
            } else if (f->kind == RECDEV_TIME) {
                if (!recdev_valid_time(text))
                    problem = L"Type the time as HH:MM:SS, for example 09:30:00.";
                else
                    StringCchCopyA(out, sizeof(out), text);
            } else {
                if (!recdev_valid_date(text))
                    problem = L"Type the date as DD-MM-YYYY (day first), for example 10-11-2026.";
                else
                    StringCchCopyA(out, sizeof(out), text);
            }
        }
        if (!problem && !recdev_config_set(&next, f->key, out))
            problem = L"That line of the settings file is too long to change.";
        if (problem) {
            WCHAR msg[256];
            StringCchPrintfW(msg, HG_ARRAYSIZE(msg), L"%ls:\n%ls", f->label, problem);
            MessageBoxW(dlg, msg, RECDEV_TITLE, MB_OK | MB_ICONWARNING);
            SetFocus(ctl);
            if (f->kind != RECDEV_CHOICE)
                SendMessageW(ctl, EM_SETSEL, 0, -1);
            return FALSE;
        }
    }
    form->config = next;
    return TRUE;
}

static void recdev_form_now(HWND dlg)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    for (int i = 0; i < RECDEV_FIELD_COUNT; ++i) {
        const RecdevField *f = &recdev_fields[i];
        HWND ctl = GetDlgItem(dlg, RECDEV_ID_FIELD0 + i);
        if (!IsWindowEnabled(ctl))
            continue;
        WCHAR text[16];
        if (strcmp(f->key, "DATE") == 0)
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%02u-%02u-%04u", t.wDay, t.wMonth, t.wYear);
        else if (strcmp(f->key, "TIME") == 0)
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"%02u:%02u:%02u", t.wHour, t.wMinute, t.wSecond);
        else
            continue;
        SetWindowTextW(ctl, text);
    }
}

static INT_PTR CALLBACK recdev_dialog_proc(HWND dlg, UINT msg, WPARAM w_param, LPARAM l_param)
{
    RecdevForm *form = (RecdevForm *)GetWindowLongPtrW(dlg, DWLP_USER);
    switch (msg) {
    case WM_INITDIALOG: {
        form = (RecdevForm *)l_param;
        SetWindowLongPtrW(dlg, DWLP_USER, l_param);
        WCHAR prompt[256];
        if (form->missing)
            StringCchPrintfW(prompt, HG_ARRAYSIZE(prompt),
                             L"The recorder (%lc:) has no settings file yet. These are the standard values; Save "
                             L"writes them to %lc:\\" RECDEV_CONFIG_NAME L".",
                             form->drive, form->drive);
        else
            StringCchPrintfW(prompt, HG_ARRAYSIZE(prompt),
                             L"The settings file on the recorder, %lc:\\" RECDEV_CONFIG_NAME
                             L". Save writes the changes to it; Cancel leaves it as it was.",
                             form->drive);
        SetDlgItemTextW(dlg, RECDEV_ID_PROMPT, prompt);
        recdev_form_fill(dlg, form);
        SetForegroundWindow(dlg);
        return TRUE; /* focus to the first control */
    }
    case WM_COMMAND:
        switch (LOWORD(w_param)) {
        case RECDEV_ID_NOW:
            recdev_form_now(dlg);
            return TRUE;
        case IDOK: {
            if (!form || !recdev_form_take(dlg, form))
                return TRUE;
            if (!hg_recbak_drive()) {
                MessageBoxW(dlg, L"The recorder is no longer connected. Nothing was saved.", RECDEV_TITLE,
                            MB_OK | MB_ICONWARNING);
                EndDialog(dlg, IDCANCEL);
                return TRUE;
            }
            DWORD e = recdev_config_write(form->drive, &form->config);
            if (e != ERROR_SUCCESS) {
                WCHAR text[256];
                StringCchPrintfW(text, HG_ARRAYSIZE(text), L"The settings were not saved (error %lu).",
                                 (unsigned long)e);
                MessageBoxW(dlg, text, RECDEV_TITLE, MB_OK | MB_ICONWARNING);
                return TRUE;
            }
            EndDialog(dlg, IDOK);
            return TRUE;
        }
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

void hg_recdev_settings(HWND owner, WCHAR drive)
{
    if (!drive)
        return;
    RecdevForm *form = (RecdevForm *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RecdevForm));
    if (!form)
        return;
    form->drive = drive;
    /* The same device check as the clock's: the file is this recorder's. */
    unsigned vid = 0, pid = 0;
    DWORD e = recdev_check_device(drive, &vid, &pid);
    if (e != ERROR_SUCCESS) {
        WCHAR text[512];
        if (!recdev_device_problem(e, vid, pid, L"The recorder settings were not opened", text, HG_ARRAYSIZE(text)))
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"The recorder settings were not opened (error %lu).",
                             (unsigned long)e);
        MessageBoxW(owner, text, RECDEV_TITLE, MB_OK | MB_ICONWARNING);
        HeapFree(GetProcessHeap(), 0, form);
        return;
    }
    e = recdev_config_read(drive, &form->config);
    if (e == ERROR_FILE_NOT_FOUND) {
        form->missing = TRUE;
        recdev_config_parse(&form->config, recdev_template, sizeof(recdev_template) - 1);
    } else if (e != ERROR_SUCCESS) {
        WCHAR text[256];
        if (e == ERROR_INVALID_DATA)
            StringCchPrintfW(text, HG_ARRAYSIZE(text),
                             L"%lc:\\" RECDEV_CONFIG_NAME L" is not a settings file this can edit without "
                             L"damaging it, so it was left alone.",
                             drive);
        else
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"Could not read %lc:\\" RECDEV_CONFIG_NAME L" (error %lu).",
                             drive, (unsigned long)e);
        MessageBoxW(owner, text, RECDEV_TITLE, MB_OK | MB_ICONWARNING);
        HeapFree(GetProcessHeap(), 0, form);
        return;
    }

    /* Rows: a label, its control; Now beside the file's time; Save, Cancel. */
    enum { CX = 300, LABEL_X = 8, LABEL_CX = 196, CTL_X = 208, CTL_CX = 84, ROW0 = 36, ROW = 16 };
    DWORD *storage = (DWORD *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 8192 * sizeof(DWORD));
    if (!storage) {
        HeapFree(GetProcessHeap(), 0, form);
        return;
    }
    short rows_end = (short)(ROW0 + RECDEV_FIELD_COUNT * ROW);
    DLGTEMPLATE *tmpl = (DLGTEMPLATE *)storage;
    tmpl->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT | DS_SETFOREGROUND | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl->dwExtendedStyle = WS_EX_TOPMOST;
    tmpl->cdit = (WORD)(1 + 2 * RECDEV_FIELD_COUNT + 3);
    tmpl->cx = CX;
    tmpl->cy = (short)(rows_end + 26);
    WORD *p = (WORD *)(tmpl + 1);
    *p++ = 0; /* no menu */
    *p++ = 0; /* the standard dialog class */
    p = hg_dlg_put_text(p, L"BakREC - Recorder Settings");
    *p++ = 9;
    p = hg_dlg_put_text(p, L"Segoe UI");
    p = hg_dlg_put_item(p, SS_LEFT | SS_NOPREFIX, 8, 6, CX - 16, 26, RECDEV_ID_PROMPT, 0x0082, L"");
    for (int i = 0; i < RECDEV_FIELD_COUNT; ++i) {
        const RecdevField *f = &recdev_fields[i];
        short y = (short)(ROW0 + i * ROW);
        short cx = CTL_CX;
        if (strcmp(f->key, "TIME") == 0)
            cx = CTL_CX - 30; /* room for Now */
        p = hg_dlg_put_item(p, SS_LEFT | SS_NOPREFIX, LABEL_X, (short)(y + 2), LABEL_CX, 10,
                            (WORD)(RECDEV_ID_LABEL0 + i), 0x0082, f->label);
        if (f->kind == RECDEV_CHOICE)
            p = hg_dlg_put_item(p, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, CTL_X, y, cx, 120,
                                (WORD)(RECDEV_ID_FIELD0 + i), 0x0085, L"");
        else
            p = hg_dlg_put_item(p, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, CTL_X, y, cx, 12,
                                (WORD)(RECDEV_ID_FIELD0 + i), 0x0081, L"");
    }
    {
        int time_row = 0;
        for (int i = 0; i < RECDEV_FIELD_COUNT; ++i)
            if (strcmp(recdev_fields[i].key, "TIME") == 0)
                time_row = i;
        p = hg_dlg_put_item(p, BS_PUSHBUTTON | WS_TABSTOP, CTL_X + CTL_CX - 26, (short)(ROW0 + time_row * ROW - 1),
                            26, 14, RECDEV_ID_NOW, 0x0080, L"Now");
    }
    p = hg_dlg_put_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP, CX - 116, (short)(rows_end + 6), 52, 15, IDOK, 0x0080,
                        L"Save");
    hg_dlg_put_item(p, BS_PUSHBUTTON | WS_TABSTOP, CX - 60, (short)(rows_end + 6), 52, 15, IDCANCEL, 0x0080,
                    L"Cancel");
    DialogBoxIndirectParamW(GetModuleHandleW(NULL), tmpl, owner, recdev_dialog_proc, (LPARAM)form);
    HeapFree(GetProcessHeap(), 0, storage);
    HeapFree(GetProcessHeap(), 0, form);
}
