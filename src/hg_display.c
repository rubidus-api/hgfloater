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

/* Effective scale of the monitor hosting the window or point, falling back to
 * the process scale. Widgets outside the co-located floater/taskbox pair use
 * this so they render correctly on mixed-DPI setups. */
double hg_window_scale(HWND hwnd)
{
    if (hwnd && IsWindow(hwnd)) {
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        UINT dpi_x = 96;
        UINT dpi_y = 96;
        if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y)) && dpi_x > 0) {
            return (double)dpi_x / 96.0;
        }
    }
    return hg_g_scale_factor;
}

double hg_point_scale(POINT pt)
{
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    UINT dpi_x = 96;
    UINT dpi_y = 96;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y)) && dpi_x > 0) {
        return (double)dpi_x / 96.0;
    }
    return hg_g_scale_factor;
}

/* =========================================================================
 * Per-monitor display scale (DPI) control.
 *
 * Windows exposes no public API for reading or changing a monitor's scaling
 * percentage, so this uses the same undocumented DisplayConfig device-info
 * interface the Settings app itself drives. The scale is expressed as an index
 * into a fixed table of percentages, given relative to the monitor's
 * recommended value; converting to and from an absolute percentage just means
 * indexing that table.
 * ========================================================================= */

const int hg_display_scale_options[HG_SCALE_OPTION_COUNT] = {100, 125, 150, 175, 200, 225};

/* The full percentage ladder Windows steps through, recommended value included.
 * The relative indices the OS returns are offsets within this table. */
static const int hg_dpi_ladder[] = {100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500};

#define HG_DISPLAYCONFIG_GET_DPI_SCALE ((DISPLAYCONFIG_DEVICE_INFO_TYPE)(-3))
#define HG_DISPLAYCONFIG_SET_DPI_SCALE ((DISPLAYCONFIG_DEVICE_INFO_TYPE)(-4))

typedef struct HgDpiScaleGet {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32 min_rel; /* smallest step relative to recommended (<= 0) */
    INT32 cur_rel; /* current step relative to recommended */
    INT32 max_rel; /* largest step relative to recommended (>= 0) */
} HgDpiScaleGet;

typedef struct HgDpiScaleSet {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32 scale_rel; /* requested step relative to recommended */
} HgDpiScaleSet;

static int hg_ladder_index_of(int percent)
{
    for (int i = 0; i < (int)HG_ARRAYSIZE(hg_dpi_ladder); ++i) {
        if (hg_dpi_ladder[i] == percent)
            return i;
    }
    return -1;
}

/* Locate the active DisplayConfig source (adapter + id) whose GDI device name
 * matches gdi_name, and, when out_name is given, its friendly monitor name. */
static BOOL hg_find_display_source(const WCHAR *gdi_name, LUID *adapter_id, UINT32 *source_id, WCHAR *out_name,
                                   size_t out_name_cch)
{
    UINT32 path_count = 0;
    UINT32 mode_count = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count) != ERROR_SUCCESS)
        return FALSE;
    if (path_count == 0)
        return FALSE;

    DISPLAYCONFIG_PATH_INFO *paths = calloc(path_count, sizeof(*paths));
    DISPLAYCONFIG_MODE_INFO *modes = calloc(mode_count ? mode_count : 1, sizeof(*modes));
    if (!paths || !modes) {
        free(paths);
        free(modes);
        return FALSE;
    }

    BOOL found = FALSE;
    if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &path_count, paths, &mode_count, modes, NULL) == ERROR_SUCCESS) {
        for (UINT32 i = 0; i < path_count && !found; ++i) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME src;
            SecureZeroMemory(&src, sizeof(src));
            src.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            src.header.size = sizeof(src);
            src.header.adapterId = paths[i].sourceInfo.adapterId;
            src.header.id = paths[i].sourceInfo.id;
            if (DisplayConfigGetDeviceInfo(&src.header) != ERROR_SUCCESS)
                continue;
            if (wcscmp(src.viewGdiDeviceName, gdi_name) != 0)
                continue;

            *adapter_id = paths[i].sourceInfo.adapterId;
            *source_id = paths[i].sourceInfo.id;
            found = TRUE;

            if (out_name && out_name_cch > 0) {
                DISPLAYCONFIG_TARGET_DEVICE_NAME tgt;
                SecureZeroMemory(&tgt, sizeof(tgt));
                tgt.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                tgt.header.size = sizeof(tgt);
                tgt.header.adapterId = paths[i].targetInfo.adapterId;
                tgt.header.id = paths[i].targetInfo.id;
                if (DisplayConfigGetDeviceInfo(&tgt.header) == ERROR_SUCCESS &&
                    tgt.monitorFriendlyDeviceName[0] != L'\0') {
                    StringCchCopyW(out_name, out_name_cch, tgt.monitorFriendlyDeviceName);
                } else {
                    StringCchCopyW(out_name, out_name_cch, gdi_name);
                }
            }
        }
    }

    free(paths);
    free(modes);
    return found;
}

/* Fill a MONITORINFOEXW so its szDevice (the GDI device name) can drive the
 * DisplayConfig lookup. */
static BOOL hg_monitor_device_name(HMONITOR monitor, WCHAR *out, size_t out_cch)
{
    if (!monitor)
        return FALSE;
    MONITORINFOEXW mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, (MONITORINFO *)&mi))
        return FALSE;
    StringCchCopyW(out, out_cch, mi.szDevice);
    return TRUE;
}

BOOL hg_query_display_scale(HMONITOR monitor, HgDisplayScale *out)
{
    if (!out)
        return FALSE;
    SecureZeroMemory(out, sizeof(*out));

    WCHAR device[64];
    if (!hg_monitor_device_name(monitor, device, HG_ARRAYSIZE(device)))
        return FALSE;

    LUID adapter_id;
    UINT32 source_id = 0;
    if (!hg_find_display_source(device, &adapter_id, &source_id, out->name, HG_ARRAYSIZE(out->name)))
        return FALSE;

    HgDpiScaleGet get;
    SecureZeroMemory(&get, sizeof(get));
    get.header.type = HG_DISPLAYCONFIG_GET_DPI_SCALE;
    get.header.size = sizeof(get);
    get.header.adapterId = adapter_id;
    get.header.id = source_id;
    if (DisplayConfigGetDeviceInfo(&get.header) != ERROR_SUCCESS)
        return FALSE;

    /* The recommended (100%) value sits at index abs(min_rel) in the ladder, so
     * an absolute value is the ladder entry at that base plus the relative step. */
    int base = -get.min_rel;
    int cur_idx = base + get.cur_rel;
    int min_idx = base + get.min_rel; /* == 0 */
    int max_idx = base + get.max_rel;
    int last = (int)HG_ARRAYSIZE(hg_dpi_ladder) - 1;
    if (min_idx < 0)
        min_idx = 0;
    if (max_idx > last)
        max_idx = last;

    out->min_percent = hg_dpi_ladder[min_idx];
    out->max_percent = hg_dpi_ladder[max_idx];
    out->current_percent = (cur_idx >= 0 && cur_idx <= last) ? hg_dpi_ladder[cur_idx] : 0;
    out->valid = TRUE;
    return TRUE;
}

BOOL hg_set_display_scale(HMONITOR monitor, int percent)
{
    int target_idx = hg_ladder_index_of(percent);
    if (target_idx < 0)
        return FALSE;

    WCHAR device[64];
    if (!hg_monitor_device_name(monitor, device, HG_ARRAYSIZE(device)))
        return FALSE;

    LUID adapter_id;
    UINT32 source_id = 0;
    if (!hg_find_display_source(device, &adapter_id, &source_id, NULL, 0))
        return FALSE;

    HgDpiScaleGet get;
    SecureZeroMemory(&get, sizeof(get));
    get.header.type = HG_DISPLAYCONFIG_GET_DPI_SCALE;
    get.header.size = sizeof(get);
    get.header.adapterId = adapter_id;
    get.header.id = source_id;
    if (DisplayConfigGetDeviceInfo(&get.header) != ERROR_SUCCESS)
        return FALSE;

    /* Convert the absolute target back to a step relative to recommended, then
     * hold it inside the range this monitor actually supports. */
    int base = -get.min_rel;
    int scale_rel = target_idx - base;
    if (scale_rel < get.min_rel)
        scale_rel = get.min_rel;
    if (scale_rel > get.max_rel)
        scale_rel = get.max_rel;

    HgDpiScaleSet set;
    SecureZeroMemory(&set, sizeof(set));
    set.header.type = HG_DISPLAYCONFIG_SET_DPI_SCALE;
    set.header.size = sizeof(set);
    set.header.adapterId = adapter_id;
    set.header.id = source_id;
    set.scale_rel = scale_rel;
    return DisplayConfigSetDeviceInfo(&set.header) == ERROR_SUCCESS;
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

