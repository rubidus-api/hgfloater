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

/* Defined with the DisplayConfig helpers below; the brightness paths key their
 * per-display state on the same GDI device name. */
static BOOL hg_monitor_device_name(HMONITOR monitor, WCHAR *out, size_t out_cch);

/* Gamma is the fallback path, so the untouched ramp is kept per display device
 * rather than once for the desktop: dimming one monitor must leave its
 * neighbours alone, and exiting must hand every one of them back. */
typedef struct HgGammaBackup {
    WCHAR device[64];
    WORD ramp[3][256];
    BOOL valid;
} HgGammaBackup;

static HgGammaBackup s_gamma_backups[HG_MAX_MONITORS];
static int s_gamma_backup_count = 0;

static HDC hg_monitor_dc(const WCHAR *device)
{
    if (!device || !device[0])
        return NULL;
    return CreateDCW(NULL, device, NULL, NULL);
}

static HgGammaBackup *hg_gamma_backup_for(const WCHAR *device, HDC hdc)
{
    for (int i = 0; i < s_gamma_backup_count; ++i) {
        if (wcscmp(s_gamma_backups[i].device, device) == 0)
            return &s_gamma_backups[i];
    }
    if (s_gamma_backup_count >= HG_MAX_MONITORS)
        return NULL;

    HgGammaBackup *slot = &s_gamma_backups[s_gamma_backup_count];
    SecureZeroMemory(slot, sizeof(*slot));
    StringCchCopyW(slot->device, HG_ARRAYSIZE(slot->device), device);
    slot->valid = GetDeviceGammaRamp(hdc, slot->ramp) ? TRUE : FALSE;
    s_gamma_backup_count++;
    return slot;
}

void restore_system_gamma(void)
{
    for (int i = 0; i < s_gamma_backup_count; ++i) {
        if (!s_gamma_backups[i].valid)
            continue;
        HDC hdc = hg_monitor_dc(s_gamma_backups[i].device);
        if (hdc) {
            SetDeviceGammaRamp(hdc, s_gamma_backups[i].ramp);
            DeleteDC(hdc);
        }
    }
}

static BOOL hg_set_gamma_brightness(const WCHAR *device, int brightness)
{
    HDC hdc = hg_monitor_dc(device);
    if (!hdc)
        return FALSE;

    HgGammaBackup *backup = hg_gamma_backup_for(device, hdc);
    double factor = (double)brightness / 100.0;
    if (factor < 0.1)
        factor = 0.1; /* Prevent screen from becoming pitch black (min 10%) */

    WORD ramp[3][256];
    if (backup && backup->valid) {
        for (int c = 0; c < 3; c++) {
            for (int i = 0; i < 256; i++) {
                double val = (double)backup->ramp[c][i] * factor;
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
    DeleteDC(hdc);
    return res;
}

/* Undo a gamma dim on one display only, so a monitor that starts answering
 * DDC/CI again is not left with a stacked software curve on top. */
static void hg_restore_gamma_for(const WCHAR *device)
{
    for (int i = 0; i < s_gamma_backup_count; ++i) {
        if (wcscmp(s_gamma_backups[i].device, device) != 0)
            continue;
        if (!s_gamma_backups[i].valid)
            return;
        HDC hdc = hg_monitor_dc(device);
        if (hdc) {
            SetDeviceGammaRamp(hdc, s_gamma_backups[i].ramp);
            DeleteDC(hdc);
        }
        return;
    }
}

/* Monitors are free to report any range, so a raw DDC/CI level is mapped onto
 * 0..100 in both directions rather than assumed to be a percentage. */
static int hg_level_to_percent(DWORD lo, DWORD cur, DWORD hi)
{
    if (hi <= lo)
        return (int)((cur > 100) ? 100 : cur);
    if (cur < lo)
        cur = lo;
    if (cur > hi)
        cur = hi;
    double span = (double)(hi - lo);
    double pos = (double)(cur - lo) * 100.0 / span;
    return (int)(pos + 0.5);
}

static DWORD hg_percent_to_level(DWORD lo, DWORD hi, int percent)
{
    if (hi <= lo)
        return (DWORD)percent;
    double span = (double)(hi - lo);
    double val = (double)lo + span * (double)percent / 100.0;
    return (DWORD)(val + 0.5);
}

static BOOL hg_ddc_ready(BOOL for_write)
{
    init_dxva2();
    if (!s_hDxva2 || !s_pfnGetNum || !s_pfnGetPhys || !s_pfnDestroy)
        return FALSE;
    return for_write ? (s_pfnSetBright != NULL) : (s_pfnGetBright != NULL);
}

BOOL hg_query_monitor_brightness(HMONITOR monitor, int *out_percent)
{
    if (!monitor || !out_percent || !hg_ddc_ready(FALSE))
        return FALSE;

    DWORD count = 0;
    if (!s_pfnGetNum(monitor, &count) || count == 0)
        return FALSE;

    HG_PHYSICAL_MONITOR *phys = (HG_PHYSICAL_MONITOR *)malloc(sizeof(HG_PHYSICAL_MONITOR) * count);
    if (!phys)
        return FALSE;

    BOOL ok = FALSE;
    if (s_pfnGetPhys(monitor, count, phys)) {
        DWORD lo = 0, cur = 0, hi = 100;
        if (s_pfnGetBright(phys[0].hPhysicalMonitor, &lo, &cur, &hi)) {
            *out_percent = hg_level_to_percent(lo, cur, hi);
            ok = TRUE;
        }
        s_pfnDestroy(count, phys);
    }
    free(phys);
    return ok;
}

static BOOL hg_ddc_set_brightness(HMONITOR monitor, int percent)
{
    if (!monitor || !hg_ddc_ready(TRUE))
        return FALSE;

    DWORD count = 0;
    if (!s_pfnGetNum(monitor, &count) || count == 0)
        return FALSE;

    HG_PHYSICAL_MONITOR *phys = (HG_PHYSICAL_MONITOR *)malloc(sizeof(HG_PHYSICAL_MONITOR) * count);
    if (!phys)
        return FALSE;

    BOOL ok = FALSE;
    if (s_pfnGetPhys(monitor, count, phys)) {
        ok = TRUE;
        for (DWORD i = 0; i < count; i++) {
            DWORD lo = 0, cur = 0, hi = 100;
            DWORD level = (DWORD)percent;
            if (s_pfnGetBright && s_pfnGetBright(phys[i].hPhysicalMonitor, &lo, &cur, &hi)) {
                level = hg_percent_to_level(lo, hi, percent);
            }
            if (!s_pfnSetBright(phys[i].hPhysicalMonitor, level)) {
                ok = FALSE;
            }
        }
        s_pfnDestroy(count, phys);
    }
    free(phys);
    return ok;
}

/* Remember what a monitor now sits at so the menu can tick the right step
 * without paying for another DDC/CI round trip. */
static void hg_cache_monitor_brightness(HMONITOR monitor, int percent)
{
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hMonitor == monitor) {
            hg_g_monitors[i].brightness = percent;
            return;
        }
    }
}

static HMONITOR hg_cursor_monitor(void)
{
    POINT pt = {0, 0};
    GetCursorPos(&pt);
    return MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
}

void hg_set_monitor_brightness(HMONITOR monitor, int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    WCHAR device[64];
    BOOL have_device = hg_monitor_device_name(monitor, device, HG_ARRAYSIZE(device));

    if (hg_ddc_set_brightness(monitor, percent)) {
        if (have_device)
            hg_restore_gamma_for(device);
    } else if (have_device) {
        hg_set_gamma_brightness(device, percent);
    }

    hg_cache_monitor_brightness(monitor, percent);
    /* The toolbar gauge reads one number for the display under the pointer, so
     * keep it in step when that is the display just changed. */
    if (monitor == hg_cursor_monitor()) {
        hg_g_global_brightness = percent;
    }
}

void hg_refresh_brightness_cache(void)
{
    HMONITOR cursor_monitor = hg_cursor_monitor();
    int percent = 0;
    BOOL cursor_known = hg_query_monitor_brightness(cursor_monitor, &percent);
    if (cursor_known) {
        hg_g_global_brightness = percent;
        hg_cache_monitor_brightness(cursor_monitor, percent);
    }

    /* One further display per call, round-robin. A DDC/CI read blocks for tens of
     * milliseconds and this runs on the taskbox timer, so the array converges
     * over a few ticks instead of stalling the UI thread on every one. */
    if (hg_g_monitor_count <= 0)
        return;

    static int next_monitor = 0;
    if (next_monitor >= hg_g_monitor_count)
        next_monitor = 0;
    int idx = next_monitor++;

    HMONITOR monitor = hg_g_monitors[idx].hMonitor;
    if (!monitor || monitor == cursor_monitor)
        return;

    int value = 0;
    hg_g_monitors[idx].brightness = hg_query_monitor_brightness(monitor, &value) ? value : -1;
}

void hg_refresh_all_monitor_brightness(void)
{
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        int value = 0;
        hg_g_monitors[i].brightness =
            hg_query_monitor_brightness(hg_g_monitors[i].hMonitor, &value) ? value : -1;
    }

    int percent = 0;
    if (hg_query_monitor_brightness(hg_cursor_monitor(), &percent)) {
        hg_g_global_brightness = percent;
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

    /* The toolbar slider acts on the display the pointer is over; the O menu
     * reaches any of them by name. */
    hg_set_monitor_brightness(hg_cursor_monitor(), brightness);
}

const int hg_brightness_options[HG_BRIGHTNESS_OPTION_COUNT] = {0, 25, 50, 75, 100};

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
 * matches gdi_name, and, when asked, its friendly monitor name and the connector
 * technology the target reports. */
static BOOL hg_find_display_source(const WCHAR *gdi_name, LUID *adapter_id, UINT32 *source_id, WCHAR *out_name,
                                   size_t out_name_cch, UINT32 *out_technology)
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

            if ((out_name && out_name_cch > 0) || out_technology) {
                DISPLAYCONFIG_TARGET_DEVICE_NAME tgt;
                SecureZeroMemory(&tgt, sizeof(tgt));
                tgt.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                tgt.header.size = sizeof(tgt);
                tgt.header.adapterId = paths[i].targetInfo.adapterId;
                tgt.header.id = paths[i].targetInfo.id;
                BOOL have_target = (DisplayConfigGetDeviceInfo(&tgt.header) == ERROR_SUCCESS);

                if (out_name && out_name_cch > 0) {
                    if (have_target && tgt.monitorFriendlyDeviceName[0] != L'\0') {
                        StringCchCopyW(out_name, out_name_cch, tgt.monitorFriendlyDeviceName);
                    } else {
                        StringCchCopyW(out_name, out_name_cch, gdi_name);
                    }
                }
                if (out_technology) {
                    /* The path carries the technology too, and it is the value that
                     * survives when the target name query fails. */
                    *out_technology = have_target ? (UINT32)tgt.outputTechnology
                                                  : (UINT32)paths[i].targetInfo.outputTechnology;
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
    if (!hg_find_display_source(device, &adapter_id, &source_id, out->name, HG_ARRAYSIZE(out->name), NULL))
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
    if (!hg_find_display_source(device, &adapter_id, &source_id, NULL, 0, NULL))
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

/* =========================================================================
 * Naming a display the way its owner recognises it.
 *
 * `\\.\DISPLAY2` says nothing, so the label is built from what the driver
 * actually knows: the monitor name out of the EDID, plus the connector the
 * target hangs off. Windows names the socket on the graphics adapter, not the
 * cable, so a USB-C screen running DisplayPort alt mode reports plain
 * DisplayPort and is indistinguishable from a DP socket; only DisplayPort
 * tunnelled over USB4 or Thunderbolt reports separately, and that is the case
 * labelled USB-C here.
 * ========================================================================= */

/* Spelled out numerically rather than by header constant: the enumeration has
 * grown over Windows releases and the newest members are missing from older
 * mingw-w64 headers. */
static const WCHAR *hg_output_technology_label(UINT32 technology)
{
    switch (technology) {
    case 0u:  return L"RGB"; /* HD15, the VGA connector */
    case 1u:  return L"S-Video";
    case 2u:  return L"Composite";
    case 3u:  return L"Component";
    case 4u:  return L"DVI";
    case 5u:  return L"HDMI";
    case 6u:  return L"LVDS";
    case 8u:  return L"D-Jpn";
    case 9u:  return L"SDI";
    case 10u: return L"DP";
    case 11u: return L"eDP";
    case 12u: return L"UDI";
    case 13u: return L"UDI";
    case 14u: return L"SDTV";
    case 15u: return L"Miracast";
    case 16u: return L"Wired";
    case 17u: return L"Virtual";
    case 18u: return L"USB-C"; /* DisplayPort tunnelled over USB4 or Thunderbolt */
    case 0x80000000u: return L"Internal";
    default: return NULL;
    }
}

/* `\\.\DISPLAY3` -> 3, matching the number the Settings app puts on the display. */
int hg_monitor_display_number(const WCHAR *gdi_name)
{
    if (!gdi_name)
        return 0;
    int value = 0;
    for (const WCHAR *p = gdi_name; *p; ++p) {
        if (*p >= L'0' && *p <= L'9') {
            value = value * 10 + (int)(*p - L'0');
        } else {
            value = 0; /* only a trailing run of digits counts */
        }
    }
    return value;
}

/* Last resort before the raw device name: the PnP monitor string the display
 * driver registers, which at least reads as a monitor. */
static BOOL hg_monitor_pnp_name(const WCHAR *gdi_name, WCHAR *out, size_t out_cch)
{
    DISPLAY_DEVICEW dd;
    SecureZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
    if (!EnumDisplayDevicesW(gdi_name, 0, &dd, 0) || dd.DeviceString[0] == L'\0')
        return FALSE;
    StringCchCopyW(out, out_cch, dd.DeviceString);
    return TRUE;
}

void hg_describe_monitor(const WCHAR *gdi_name, WCHAR *out, size_t out_cch)
{
    if (!out || out_cch == 0)
        return;
    out[0] = L'\0';
    if (!gdi_name || !gdi_name[0])
        return;

    WCHAR friendly[128];
    friendly[0] = L'\0';
    UINT32 technology = 0xFFFFFFFFu;
    LUID adapter_id;
    UINT32 source_id = 0;
    BOOL have_source =
        hg_find_display_source(gdi_name, &adapter_id, &source_id, friendly, HG_ARRAYSIZE(friendly), &technology);

    if (friendly[0] == L'\0' || wcscmp(friendly, gdi_name) == 0) {
        WCHAR pnp[128];
        if (hg_monitor_pnp_name(gdi_name, pnp, HG_ARRAYSIZE(pnp)))
            StringCchCopyW(friendly, HG_ARRAYSIZE(friendly), pnp);
    }
    if (friendly[0] == L'\0')
        StringCchCopyW(friendly, HG_ARRAYSIZE(friendly), gdi_name);

    const WCHAR *port = have_source ? hg_output_technology_label(technology) : NULL;
    int number = hg_monitor_display_number(gdi_name);

    if (number > 0 && port) {
        StringCchPrintfW(out, out_cch, L"%d. %ls (%ls)", number, friendly, port);
    } else if (number > 0) {
        StringCchPrintfW(out, out_cch, L"%d. %ls", number, friendly);
    } else if (port) {
        StringCchPrintfW(out, out_cch, L"%ls (%ls)", friendly, port);
    } else {
        StringCchCopyW(out, out_cch, friendly);
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
        MonitorInfo *slot = &hg_g_monitors[hg_g_monitor_count];
        slot->hMonitor = hMonitor;
        slot->rcMonitor = mi.rcMonitor;
        StringCchCopyW(slot->name, HG_ARRAYSIZE(slot->name), mi.szDevice);
        hg_describe_monitor(mi.szDevice, slot->label, HG_ARRAYSIZE(slot->label));
        slot->brightness = -1;
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
                hg_g_monitors[i].brightness = saved_monitors[j].brightness;
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

