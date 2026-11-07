/* HP Fn Keys: the Set list row that switches an HP laptop's BIOS setting
 * "Action Key Mode" (owner's request, 2026-11-08; the earlier standalone tool
 * it follows is AeroKeys, private archive).
 *
 * Writing a BIOS setting needs an elevated process, and this program runs
 * unelevated, so the work is in two halves:
 *
 *   hg_hpkeys_command   in the running program. Reads the setting, says which
 *                       way it is, asks whether to switch, and starts this exe
 *                       again with "runas" and --hp-fnkeys action|function.
 *   hg_hpkeys_cli       that elevated copy. Called from wWinMain before the
 *                       single-instance mutex, so it never becomes a second
 *                       floater. Reads again (the value may have moved), asks
 *                       for the BIOS password if one is set, writes, reads back,
 *                       offers a restart, and exits.
 *
 * When the unelevated read fails (some machines let only administrators read
 * the namespace) the command starts the elevated copy with no direction, and
 * that copy asks the question instead.
 *
 * The password exists only in the dialog's buffer and the string handed to
 * WMI; both are wiped. It is never on a command line, in a file, or in a log.
 * The value written is always one of the firmware's own PossibleValues. */
#include "hg_hpkeys.h"
#include "hg_utils.h"
#include "hg_calc.h"
#include <shellapi.h>
#include <strsafe.h>

#define HG_HP_SETTING L"Action Key Mode"
#define HG_HP_CLI_SWITCH L"--hp-fnkeys"
#define HG_HP_TITLE L"HP Fn Keys"
#define HG_HP_RETURN_ACCESS_DENIED 6UL

typedef struct HgHpMode {
    HgHpSetting setting;
    int on_index;  /* action keys first */
    int off_index; /* function keys first */
} HgHpMode;

static HRESULT hpkeys_read(HgHpMode *mode)
{
    HRESULT hr = hg_hpbios_read(HG_HP_SETTING, &mode->setting);
    if (FAILED(hr))
        return hr;
    const wchar_t *values[HG_HP_VALUES_MAX];
    for (int i = 0; i < mode->setting.value_count; ++i)
        values[i] = mode->setting.values[i];
    mode->on_index = hg_calc_hp_mode_value(values, mode->setting.value_count, 1);
    mode->off_index = hg_calc_hp_mode_value(values, mode->setting.value_count, 0);
    return S_OK;
}

/* 1 action keys first, 0 function keys first, -1 a value this does not know. */
static int hpkeys_current(const HgHpMode *mode)
{
    if (mode->on_index >= 0 && lstrcmpW(mode->setting.current, mode->setting.values[mode->on_index]) == 0)
        return 1;
    if (mode->off_index >= 0 && lstrcmpW(mode->setting.current, mode->setting.values[mode->off_index]) == 0)
        return 0;
    return -1;
}

static const WCHAR *hpkeys_describe(int action_first)
{
    return action_first ? L"Action keys first: brightness, volume and the rest on their own, F1-F12 with Fn."
                        : L"Function keys first: F1-F12 on their own, brightness, volume and the rest with Fn.";
}

static BOOL hpkeys_writable(const HgHpMode *mode)
{
    return !mode->setting.read_only && mode->on_index >= 0 && mode->off_index >= 0;
}

static void hpkeys_say(HWND owner, const WCHAR *text, UINT icon)
{
    MessageBoxW(owner, text, HG_HP_TITLE, MB_OK | icon | MB_SETFOREGROUND);
}

static void hpkeys_say_unsupported(HWND owner, const HgHpMode *mode)
{
    WCHAR text[512];
    StringCchPrintfW(text, HG_ARRAYSIZE(text),
                     L"This machine's BIOS does not let \"%s\" be switched from Windows%s.\r\n\r\n"
                     L"Current value: %s",
                     HG_HP_SETTING, mode->setting.read_only ? L" (it is read-only)" : L" (its values are not on/off)",
                     mode->setting.current);
    hpkeys_say(owner, text, MB_ICONWARNING);
}

/* Asks whether to switch; TRUE when the user said yes. */
static BOOL hpkeys_confirm(HWND owner, const HgHpMode *mode, int current, BOOL elevating)
{
    WCHAR text[768];
    StringCchPrintfW(text, HG_ARRAYSIZE(text),
                     L"Now: %s\r\n(BIOS \"%s\" = %s)\r\n\r\n"
                     L"Switch to - %s\r\n\r\n"
                     L"It takes effect after a restart.%s",
                     hpkeys_describe(current), HG_HP_SETTING, mode->setting.current, hpkeys_describe(!current),
                     elevating ? L" Windows will ask for administrator permission." : L"");
    return MessageBoxW(owner, text, HG_HP_TITLE, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND) ==
           IDYES;
}

static BOOL hpkeys_launch_elevated(HWND owner, const WCHAR *direction)
{
    WCHAR exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, exe, HG_ARRAYSIZE(exe));
    if (n == 0 || n >= HG_ARRAYSIZE(exe))
        return FALSE;

    WCHAR params[64];
    StringCchPrintfW(params, HG_ARRAYSIZE(params), L"%s%s%s", HG_HP_CLI_SWITCH, direction ? L" " : L"",
                     direction ? direction : L"");

    SHELLEXECUTEINFOW info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOASYNC;
    info.hwnd = owner;
    info.lpVerb = L"runas";
    info.lpFile = exe;
    info.lpParameters = params;
    info.nShow = SW_SHOWNORMAL;
    if (ShellExecuteExW(&info))
        return TRUE;
    /* Declining the UAC prompt is an answer, not an error. */
    if (GetLastError() != ERROR_CANCELLED) {
        WCHAR text[256];
        StringCchPrintfW(text, HG_ARRAYSIZE(text), L"Could not start the administrator step (error %lu).",
                         GetLastError());
        hpkeys_say(owner, text, MB_ICONERROR);
    }
    return FALSE;
}

void hg_hpkeys_command(HWND owner)
{
    HgHpMode mode;
    HCURSOR old = SetCursor(LoadCursorW(NULL, IDC_WAIT));
    HRESULT hr = hpkeys_read(&mode);
    SetCursor(old);

    if (FAILED(hr)) {
        /* Not readable from here: the elevated copy reads it and asks. */
        hpkeys_launch_elevated(owner, NULL);
        return;
    }
    if (!hpkeys_writable(&mode)) {
        hpkeys_say_unsupported(owner, &mode);
        return;
    }
    int current = hpkeys_current(&mode);
    if (current < 0) {
        hpkeys_say_unsupported(owner, &mode);
        return;
    }
    if (hpkeys_confirm(owner, &mode, current, TRUE))
        hpkeys_launch_elevated(owner, current ? L"function" : L"action");
}

/* ------------------------------------------------------------ password dialog */

#define HG_HP_ID_PROMPT 100
#define HG_HP_ID_EDIT 101

typedef struct HgHpPasswordAsk {
    const WCHAR *prompt;
    WCHAR *buffer;
    int capacity;
} HgHpPasswordAsk;

static INT_PTR CALLBACK hpkeys_password_proc(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HgHpPasswordAsk *ask = (HgHpPasswordAsk *)GetWindowLongPtrW(dlg, DWLP_USER);
    switch (msg) {
    case WM_INITDIALOG:
        ask = (HgHpPasswordAsk *)lparam;
        SetWindowLongPtrW(dlg, DWLP_USER, lparam);
        SetDlgItemTextW(dlg, HG_HP_ID_PROMPT, ask->prompt);
        SendDlgItemMessageW(dlg, HG_HP_ID_EDIT, EM_LIMITTEXT, (WPARAM)(ask->capacity - 1), 0);
        SetForegroundWindow(dlg);
        SetFocus(GetDlgItem(dlg, HG_HP_ID_EDIT));
        return FALSE; /* focus set here */
    case WM_COMMAND:
        if (LOWORD(wparam) == IDOK && ask) {
            GetDlgItemTextW(dlg, HG_HP_ID_EDIT, ask->buffer, ask->capacity);
            SetDlgItemTextW(dlg, HG_HP_ID_EDIT, L"");
            EndDialog(dlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wparam) == IDCANCEL) {
            SetDlgItemTextW(dlg, HG_HP_ID_EDIT, L"");
            EndDialog(dlg, IDCANCEL);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/* An in-memory dialog template: no resource script entry for one small box. */
static WORD *hpkeys_put_text(WORD *p, const WCHAR *text)
{
    do {
        *p++ = (WORD)*text;
    } while (*text++);
    return p;
}

static WORD *hpkeys_align(WORD *p)
{
    return (WORD *)(((ULONG_PTR)p + 3) & ~(ULONG_PTR)3);
}

static WORD *hpkeys_put_item(WORD *p, DWORD style, short x, short y, short cx, short cy, WORD id, WORD atom,
                             const WCHAR *text)
{
    p = hpkeys_align(p);
    DLGITEMTEMPLATE *item = (DLGITEMTEMPLATE *)p;
    item->style = style | WS_CHILD | WS_VISIBLE;
    item->dwExtendedStyle = 0;
    item->x = x;
    item->y = y;
    item->cx = cx;
    item->cy = cy;
    item->id = id;
    p = (WORD *)(item + 1);
    *p++ = 0xFFFF;
    *p++ = atom;
    p = hpkeys_put_text(p, text);
    *p++ = 0; /* no creation data */
    return p;
}

static BOOL hpkeys_ask_password(const WCHAR *prompt, WCHAR *buffer, int capacity)
{
    DWORD storage[512];
    ZeroMemory(storage, sizeof(storage));
    DLGTEMPLATE *tmpl = (DLGTEMPLATE *)storage;
    tmpl->style = DS_MODALFRAME | DS_CENTER | DS_SETFONT | DS_SETFOREGROUND | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl->dwExtendedStyle = WS_EX_TOPMOST;
    tmpl->cdit = 4;
    tmpl->x = 0;
    tmpl->y = 0;
    tmpl->cx = 230;
    tmpl->cy = 86;
    WORD *p = (WORD *)(tmpl + 1);
    *p++ = 0; /* no menu */
    *p++ = 0; /* the standard dialog class */
    p = hpkeys_put_text(p, L"HP BIOS Password");
    *p++ = 9;
    p = hpkeys_put_text(p, L"Segoe UI");
    p = hpkeys_put_item(p, SS_LEFT, 8, 8, 214, 34, HG_HP_ID_PROMPT, 0x0082, L"");
    p = hpkeys_put_item(p, ES_PASSWORD | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 8, 44, 214, 13, HG_HP_ID_EDIT,
                        0x0081, L"");
    p = hpkeys_put_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP, 118, 64, 50, 14, IDOK, 0x0080, L"OK");
    hpkeys_put_item(p, BS_PUSHBUTTON | WS_TABSTOP, 172, 64, 50, 14, IDCANCEL, 0x0080, L"Cancel");

    HgHpPasswordAsk ask = {prompt, buffer, capacity};
    buffer[0] = 0;
    return DialogBoxIndirectParamW(GetModuleHandleW(NULL), tmpl, NULL, hpkeys_password_proc, (LPARAM)&ask) == IDOK;
}

/* -------------------------------------------------------------- elevated copy */

static const WCHAR *hpkeys_return_text(unsigned long code)
{
    switch (code) {
    case 1:
        return L"the firmware does not support it";
    case 2:
        return L"an unspecified error";
    case 3:
        return L"it timed out";
    case 4:
        return L"it failed";
    case 5:
        return L"an invalid parameter";
    case 6:
        return L"access denied - the BIOS password was wrong or missing";
    default:
        return L"an unknown answer";
    }
}

static void hpkeys_offer_restart(void)
{
    if (MessageBoxW(NULL,
                    L"Saved. The keys switch after Windows restarts.\r\n\r\nRestart now? Programs get the usual "
                    L"chance to ask about unsaved work.",
                    HG_HP_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST) !=
        IDYES)
        return;

    HANDLE token = NULL;
    BOOL ok = FALSE;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        TOKEN_PRIVILEGES privileges;
        ZeroMemory(&privileges, sizeof(privileges));
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid) &&
            AdjustTokenPrivileges(token, FALSE, &privileges, 0, NULL, NULL) && GetLastError() == ERROR_SUCCESS) {
            /* No EWX_FORCE: a program with unsaved work may still say no. */
            ok = ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_RECONFIG |
                                               SHTDN_REASON_FLAG_PLANNED);
        }
        CloseHandle(token);
    }
    if (!ok)
        hpkeys_say(NULL, L"Windows did not start the restart. Restart from the Start menu when convenient.",
                   MB_ICONWARNING);
}

static int hpkeys_elevated(const WCHAR *direction)
{
    HgHpMode mode;
    HRESULT hr = hpkeys_read(&mode);
    if (FAILED(hr)) {
        WCHAR text[384];
        StringCchPrintfW(text, HG_ARRAYSIZE(text),
                         L"Could not read the BIOS setting \"%s\" (0x%08lX).\r\n\r\n"
                         L"This needs an HP machine whose BIOS offers the setting over WMI.",
                         HG_HP_SETTING, (unsigned long)hr);
        hpkeys_say(NULL, text, MB_ICONERROR);
        return 2;
    }
    int current = hpkeys_current(&mode);
    if (!hpkeys_writable(&mode) || current < 0) {
        hpkeys_say_unsupported(NULL, &mode);
        return 2;
    }

    int want;
    if (direction && lstrcmpiW(direction, L"action") == 0) {
        want = 1;
    } else if (direction && lstrcmpiW(direction, L"function") == 0) {
        want = 0;
    } else {
        /* Started without a direction: the unelevated read failed, so the
         * question was not asked yet. */
        if (!hpkeys_confirm(NULL, &mode, current, FALSE))
            return 1;
        want = !current;
    }
    if (want == current) {
        WCHAR text[256];
        StringCchPrintfW(text, HG_ARRAYSIZE(text), L"Already set. %s", hpkeys_describe(current));
        hpkeys_say(NULL, text, MB_ICONINFORMATION);
        return 0;
    }
    const WCHAR *target = mode.setting.values[want ? mode.on_index : mode.off_index];

    BOOL has_password = FALSE;
    BOOL password_known = SUCCEEDED(hg_hpbios_password_is_set(&has_password));
    WCHAR password[HG_HP_PASSWORD_MAX];
    password[0] = 0;
    BOOL ask = has_password;
    const WCHAR *prompt = L"This machine has a BIOS administrator (setup) password. Enter it to change the setting.";
    int exit_code = 2;

    for (;;) {
        if (ask && !hpkeys_ask_password(prompt, password, HG_ARRAYSIZE(password))) {
            exit_code = 1; /* cancelled */
            break;
        }
        unsigned long answer = (unsigned long)-1;
        hr = hg_hpbios_write(HG_HP_SETTING, target, ask ? password : NULL, &answer);
        SecureZeroMemory(password, sizeof(password));
        if (FAILED(hr)) {
            WCHAR text[256];
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"The BIOS could not be asked to change it (0x%08lX).",
                             (unsigned long)hr);
            hpkeys_say(NULL, text, MB_ICONERROR);
            break;
        }
        if (answer == HG_HP_RETURN_ACCESS_DENIED && (ask || !password_known)) {
            /* A wrong password - or one the IsSet query could not see - asks
             * again. HP may lock the BIOS after repeated wrong passwords, so the
             * prompt says it was wrong rather than looping silently. */
            prompt = ask ? L"The BIOS did not accept that password. Enter the BIOS administrator (setup) password."
                         : L"The BIOS asked for a password. Enter the BIOS administrator (setup) password.";
            ask = TRUE;
            continue;
        }
        if (answer != 0) {
            WCHAR text[256];
            StringCchPrintfW(text, HG_ARRAYSIZE(text), L"The BIOS refused the change: %s (code %lu).",
                             hpkeys_return_text(answer), answer);
            hpkeys_say(NULL, text, MB_ICONERROR);
            break;
        }

        /* Read back: the value may only show after the restart on some
         * firmware, which is said rather than claimed as done. */
        HgHpMode after;
        if (SUCCEEDED(hpkeys_read(&after)) && lstrcmpW(after.setting.current, target) != 0) {
            WCHAR text[384];
            StringCchPrintfW(text, HG_ARRAYSIZE(text),
                             L"The BIOS accepted the change, but still reads \"%s\". It should show after a restart.",
                             after.setting.current);
            hpkeys_say(NULL, text, MB_ICONINFORMATION);
        }
        hpkeys_offer_restart();
        exit_code = 0;
        break;
    }
    SecureZeroMemory(password, sizeof(password));
    return exit_code;
}

int hg_hpkeys_cli(LPCWSTR cmd_line)
{
    if (!cmd_line || !cmd_line[0])
        return -1;
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(cmd_line, &argc);
    if (!argv)
        return -1;
    int result = -1;
    for (int i = 0; i < argc; ++i) {
        if (lstrcmpiW(argv[i], HG_HP_CLI_SWITCH) == 0) {
            result = hpkeys_elevated(i + 1 < argc ? argv[i + 1] : NULL);
            break;
        }
    }
    LocalFree(argv);
    return result;
}
