/* Display control: DDC/CI and gamma-fallback brightness plus monitor
 * enumeration bookkeeping. */
#include "hg_utils.h"

typedef struct _HG_PHYSICAL_MONITOR {
    HANDLE hPhysicalMonitor;
    WCHAR szPhysicalMonitorDescription[128];
} HG_PHYSICAL_MONITOR;

typedef BOOL (WINAPI *PFN_GetNumberOfPhysicalMonitorsFromHMONITOR)(HMONITOR, LPDWORD);
typedef BOOL (WINAPI *PFN_GetPhysicalMonitorsFromHMONITOR)(HMONITOR, DWORD, HG_PHYSICAL_MONITOR*);
typedef BOOL (WINAPI *PFN_DestroyPhysicalMonitors)(DWORD, HG_PHYSICAL_MONITOR*);
typedef BOOL (WINAPI *PFN_GetMonitorBrightness)(HANDLE, LPDWORD, LPDWORD, LPDWORD);
typedef BOOL (WINAPI *PFN_SetMonitorBrightness)(HANDLE, DWORD);

static FARPROC hg_get_proc_address(HMODULE module, LPCSTR name)
{
    if (!module || !name)
        return NULL;
    return GetProcAddress(module, name);
}

static int hg_g_global_brightness = 55;
static HMODULE s_hDxva2 = NULL;
static PFN_GetNumberOfPhysicalMonitorsFromHMONITOR s_pfnGetNum = NULL;
static PFN_GetPhysicalMonitorsFromHMONITOR s_pfnGetPhys = NULL;
static PFN_DestroyPhysicalMonitors s_pfnDestroy = NULL;
static PFN_GetMonitorBrightness s_pfnGetBright = NULL;
static PFN_SetMonitorBrightness s_pfnSetBright = NULL;
static BOOL s_dxva2_initialized = FALSE;

static void init_dxva2(void)
{
    if (s_dxva2_initialized)
        return;
    s_hDxva2 = LoadLibraryW(L"dxva2.dll");
    if (s_hDxva2) {
        union {
            FARPROC proc;
            PFN_GetNumberOfPhysicalMonitorsFromHMONITOR get_num;
            PFN_GetPhysicalMonitorsFromHMONITOR get_phys;
            PFN_DestroyPhysicalMonitors destroy;
            PFN_GetMonitorBrightness get_bright;
            PFN_SetMonitorBrightness set_bright;
        } loader;

        loader.proc = hg_get_proc_address(s_hDxva2, "GetNumberOfPhysicalMonitorsFromHMONITOR");
        s_pfnGetNum = loader.get_num;
        loader.proc = hg_get_proc_address(s_hDxva2, "GetPhysicalMonitorsFromHMONITOR");
        s_pfnGetPhys = loader.get_phys;
        loader.proc = hg_get_proc_address(s_hDxva2, "DestroyPhysicalMonitors");
        s_pfnDestroy = loader.destroy;
        loader.proc = hg_get_proc_address(s_hDxva2, "GetMonitorBrightness");
        s_pfnGetBright = loader.get_bright;
        loader.proc = hg_get_proc_address(s_hDxva2, "SetMonitorBrightness");
        s_pfnSetBright = loader.set_bright;
    }
    s_dxva2_initialized = TRUE;
}

static WORD s_original_gamma_ramp[3][256];
static BOOL s_gamma_backup_valid = FALSE;

void restore_system_gamma(void)
{
    if (s_gamma_backup_valid) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            SetDeviceGammaRamp(hdc, s_original_gamma_ramp);
            ReleaseDC(NULL, hdc);
        }
    }
}

static void backup_original_gamma(void)
{
    if (s_gamma_backup_valid)
        return;
    HDC hdc = GetDC(NULL);
    if (hdc) {
        if (GetDeviceGammaRamp(hdc, s_original_gamma_ramp)) {
            s_gamma_backup_valid = TRUE;
        }
        ReleaseDC(NULL, hdc);
    }
}

static BOOL set_software_brightness(int brightness)
{
    backup_original_gamma();
    HDC hdc = GetDC(NULL);
    if (!hdc)
        return FALSE;

    WORD ramp[3][256];
    double factor = (double)brightness / 100.0;
    if (factor < 0.1)
        factor = 0.1; /* Prevent screen from becoming pitch black (min 10%) */

    if (s_gamma_backup_valid) {
        for (int c = 0; c < 3; c++) {
            for (int i = 0; i < 256; i++) {
                double val = (double)s_original_gamma_ramp[c][i] * factor;
                if (val > 65535.0)
                    val = 65535.0;
                ramp[c][i] = (WORD)val;
            }
        }
    } else {
        for (int i = 0; i < 256; i++) {
            double val = (double)i * 256.0 * factor;
            if (val > 65535.0)
                val = 65535.0;
            ramp[0][i] = ramp[1][i] = ramp[2][i] = (WORD)val;
        }
    }

    BOOL res = SetDeviceGammaRamp(hdc, ramp);
    ReleaseDC(NULL, hdc);
    return res;
}

void hg_refresh_brightness_cache(void)
{
    init_dxva2();
    if (!s_hDxva2 || !s_pfnGetNum || !s_pfnGetPhys || !s_pfnDestroy || !s_pfnGetBright)
        return;

    POINT ptCursor = {0};
    GetCursorPos(&ptCursor);
    HMONITOR hMonitor = MonitorFromPoint(ptCursor, MONITOR_DEFAULTTONEAREST);

    DWORD numMonitors = 0;
    if (s_pfnGetNum(hMonitor, &numMonitors) && numMonitors > 0) {
        HG_PHYSICAL_MONITOR *pPhysicalMonitors = (HG_PHYSICAL_MONITOR *)malloc(sizeof(HG_PHYSICAL_MONITOR) * numMonitors);
        if (pPhysicalMonitors) {
            if (s_pfnGetPhys(hMonitor, numMonitors, pPhysicalMonitors)) {
                DWORD minBright = 0, curBright = 55, maxBright = 100;
                if (s_pfnGetBright(pPhysicalMonitors[0].hPhysicalMonitor, &minBright, &curBright, &maxBright)) {
                    hg_g_global_brightness = (int)curBright;
                }
                s_pfnDestroy(numMonitors, pPhysicalMonitors);
            }
            free(pPhysicalMonitors);
        }
    }
}

int get_system_brightness(void)
{
    /* Cache-only: DDC/CI reads block for tens of milliseconds and must stay out of
     * paint and tooltip paths. Startup, the refresh timer, and setters prime it. */
    return hg_g_global_brightness;
}

void set_system_brightness(int brightness)
{
    if (brightness < 0)
        brightness = 0;
    if (brightness > 100)
        brightness = 100;
    hg_g_global_brightness = brightness;

    init_dxva2();
    BOOL hw_success = FALSE;
    if (s_hDxva2 && s_pfnGetNum && s_pfnGetPhys && s_pfnDestroy && s_pfnSetBright) {
        POINT ptCursor = {0};
        GetCursorPos(&ptCursor);
        HMONITOR hMonitor = MonitorFromPoint(ptCursor, MONITOR_DEFAULTTONEAREST);

        DWORD numMonitors = 0;
        if (s_pfnGetNum(hMonitor, &numMonitors) && numMonitors > 0) {
            HG_PHYSICAL_MONITOR *pPhysicalMonitors = (HG_PHYSICAL_MONITOR *)malloc(sizeof(HG_PHYSICAL_MONITOR) * numMonitors);
            if (pPhysicalMonitors) {
                if (s_pfnGetPhys(hMonitor, numMonitors, pPhysicalMonitors)) {
                    hw_success = TRUE;
                    for (DWORD i = 0; i < numMonitors; i++) {
                        if (!s_pfnSetBright(pPhysicalMonitors[i].hPhysicalMonitor, (DWORD)brightness)) {
                            hw_success = FALSE;
                        }
                    }
                    s_pfnDestroy(numMonitors, pPhysicalMonitors);
                }
                free(pPhysicalMonitors);
            }
        }
    }

    if (!hw_success) {
        set_software_brightness(brightness);
    } else {
        restore_system_gamma();
    }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor;
    (void)lprcMonitor;
    (void)dwData;
    if (hg_g_monitor_count >= HG_MAX_MONITORS)
        return FALSE;
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, (MONITORINFO *)&mi)) {
        hg_g_monitors[hg_g_monitor_count].hMonitor = hMonitor;
        hg_g_monitors[hg_g_monitor_count].rcMonitor = mi.rcMonitor;
        StringCchCopyW(hg_g_monitors[hg_g_monitor_count].name, 64, mi.szDevice);
        hg_g_monitor_count++;
    }
    return TRUE;
}

void update_monitor_enum()
{
    MonitorInfo saved_monitors[HG_MAX_MONITORS];
    int saved_count = hg_g_monitor_count;
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        saved_monitors[i] = hg_g_monitors[i];
    }

    hg_g_monitor_count = 0;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    for (int i = 0; i < hg_g_monitor_count; ++i) {
        hg_g_monitors[i].active = FALSE;
        hg_g_monitors[i].hwnd = NULL;
        for (int j = 0; j < saved_count; ++j) {
            if (wcscmp(hg_g_monitors[i].name, saved_monitors[j].name) == 0) {
                hg_g_monitors[i].active = saved_monitors[j].active;
                hg_g_monitors[i].hwnd = saved_monitors[j].hwnd;
                saved_monitors[j].hwnd = NULL;
                break;
            }
        }
    }

    for (int j = 0; j < saved_count; ++j) {
        if (saved_monitors[j].hwnd && IsWindow(saved_monitors[j].hwnd)) {
            SendMessageW(saved_monitors[j].hwnd, WM_CLOSE, 0, 0);
        }
    }
}

