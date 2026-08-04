/* Lightweight system status readouts for the floater: battery, CPU, memory.
 * Each is one cheap Win32 call; CPU is a delta between successive calls, so it
 * needs a steady caller (the floater clock timer) to be meaningful. */
#include "hg_utils.h"

BOOL hg_get_battery_percent(int *out_percent, BOOL *out_charging)
{
    SYSTEM_POWER_STATUS sps = {0};

    if (!out_percent || !out_charging || !GetSystemPowerStatus(&sps))
        return FALSE;
    /* 128: no system battery, 255: unknown; life percent 255 means unknown. */
    if ((sps.BatteryFlag & 128) || sps.BatteryFlag == 255 || sps.BatteryLifePercent > 100)
        return FALSE;
    *out_percent = (int)sps.BatteryLifePercent;
    *out_charging = (sps.ACLineStatus == 1);
    return TRUE;
}

static ULONGLONG hg_filetime_to_u64(FILETIME ft)
{
    ULARGE_INTEGER value;
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return value.QuadPart;
}

int hg_get_cpu_percent(void)
{
    /* GetSystemTimes kernel time includes idle time, so busy = total - idle. */
    static ULONGLONG prev_idle = 0;
    static ULONGLONG prev_total = 0;
    static BOOL primed = FALSE;
    FILETIME idle_ft, kernel_ft, user_ft;
    int pct = -1;

    if (!GetSystemTimes(&idle_ft, &kernel_ft, &user_ft))
        return -1;

    ULONGLONG idle = hg_filetime_to_u64(idle_ft);
    ULONGLONG total = hg_filetime_to_u64(kernel_ft) + hg_filetime_to_u64(user_ft);

    if (primed && total > prev_total) {
        ULONGLONG d_total = total - prev_total;
        ULONGLONG d_idle = idle - prev_idle;
        if (d_idle > d_total)
            d_idle = d_total;
        pct = (int)(((d_total - d_idle) * 100u) / d_total);
    }
    prev_idle = idle;
    prev_total = total;
    primed = TRUE;
    return pct;
}

int hg_get_memory_percent(void)
{
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
        return -1;
    return (int)ms.dwMemoryLoad;
}

/* =========================================================================
 * GPU temperature.
 *
 * There is no vendor-neutral, driver-free API for this. What Task Manager
 * itself reads is the WDDM kernel thunk: D3DKMTQueryAdapterInfo with
 * KMTQAITYPE_ADAPTERPERFDATA, whose D3DKMT_ADAPTER_PERFDATA carries the
 * adapter's main temperature sensor in tenths of a degree Celsius. The
 * functions are exported from gdi32.dll, so this needs no library, no driver,
 * and no elevation - the same bargain the rest of this app takes.
 *
 * The declarations below are transcribed from the Microsoft DDI reference for
 * these structures, because the WDK header they live in is not part of the
 * mingw-w64 toolchain. See docs/RFC-2026-07-temperature.md.
 *
 * Plenty of drivers answer zero. That is a display with no reading, not an
 * error, and it is handled the same way the CPU thermal zone's absence is.
 * ========================================================================= */

typedef UINT32 HgKmtHandle;

/* KMTQUERYADAPTERINFOTYPE. Only the one member this needs is named; its value
 * is its position in that enumeration. */
#define HG_KMTQAITYPE_ADAPTERPERFDATA 62

typedef struct HgKmtAdapterInfo {
    HgKmtHandle hAdapter;
    LUID AdapterLuid;
    ULONG NumOfSources;
    BOOL bPresentMoveRegionsPreferred;
} HgKmtAdapterInfo;

typedef struct HgKmtEnumAdapters2 {
    ULONG NumAdapters;
    HgKmtAdapterInfo *pAdapters;
} HgKmtEnumAdapters2;

typedef struct HgKmtQueryAdapterInfo {
    HgKmtHandle hAdapter;
    int Type;
    VOID *pPrivateDriverData;
    UINT PrivateDriverDataSize;
} HgKmtQueryAdapterInfo;

/* The ULONGLONG members are 8-byte aligned, which is what the WDK's
 * D3DKMT_ALIGN64 asks for and what this target does by default. */
typedef struct HgKmtAdapterPerfData {
    UINT32 PhysicalAdapterIndex;
    ULONGLONG MemoryFrequency;
    ULONGLONG MaxMemoryFrequency;
    ULONGLONG MaxMemoryFrequencyOC;
    ULONGLONG MemoryBandwidth;
    ULONGLONG PCIEBandwidth;
    ULONG FanRPM;
    ULONG Power;
    ULONG Temperature; /* tenths of a degree Celsius: 1 = 0.1C */
    UCHAR PowerStateOverride;
} HgKmtAdapterPerfData;

typedef struct HgKmtCloseAdapter {
    HgKmtHandle hAdapter;
} HgKmtCloseAdapter;

typedef LONG (WINAPI *PFN_D3DKMTEnumAdapters2)(HgKmtEnumAdapters2 *);
typedef LONG (WINAPI *PFN_D3DKMTQueryAdapterInfo)(HgKmtQueryAdapterInfo *);
typedef LONG (WINAPI *PFN_D3DKMTCloseAdapter)(const HgKmtCloseAdapter *);

static PFN_D3DKMTEnumAdapters2 s_pfnEnumAdapters2 = NULL;
static PFN_D3DKMTQueryAdapterInfo s_pfnQueryAdapterInfo = NULL;
static PFN_D3DKMTCloseAdapter s_pfnCloseAdapter = NULL;
static BOOL s_kmt_initialized = FALSE;

static BOOL hg_kmt_ready(void)
{
    if (!s_kmt_initialized) {
        s_kmt_initialized = TRUE;
        HMODULE gdi = GetModuleHandleW(L"gdi32.dll");
        if (gdi) {
            union {
                FARPROC proc;
                PFN_D3DKMTEnumAdapters2 enum2;
                PFN_D3DKMTQueryAdapterInfo query;
                PFN_D3DKMTCloseAdapter close;
            } loader;
            loader.proc = GetProcAddress(gdi, "D3DKMTEnumAdapters2");
            s_pfnEnumAdapters2 = loader.enum2;
            loader.proc = GetProcAddress(gdi, "D3DKMTQueryAdapterInfo");
            s_pfnQueryAdapterInfo = loader.query;
            loader.proc = GetProcAddress(gdi, "D3DKMTCloseAdapter");
            s_pfnCloseAdapter = loader.close;
        }
    }
    return s_pfnEnumAdapters2 && s_pfnQueryAdapterInfo && s_pfnCloseAdapter;
}

/* Deci-Celsius to Celsius, refusing anything that is not a temperature. Zero is
 * what a driver that does not report says, and it is not a reading. */
static BOOL hg_kmt_temperature(const HgKmtAdapterPerfData *data, int *out_celsius)
{
    if (data->Temperature == 0)
        return FALSE;
    int celsius = (int)((data->Temperature + 5u) / 10u);
    if (celsius < 1 || celsius > 150)
        return FALSE;
    *out_celsius = celsius;
    return TRUE;
}

BOOL hg_get_gpu_temperature(int *out_celsius)
{
    if (!out_celsius || !hg_kmt_ready())
        return FALSE;

    HgKmtEnumAdapters2 enumeration;
    SecureZeroMemory(&enumeration, sizeof(enumeration));
    if (s_pfnEnumAdapters2(&enumeration) != 0 || enumeration.NumAdapters == 0)
        return FALSE;
    if (enumeration.NumAdapters > 64)
        return FALSE; /* a count this large is not a machine, it is a wrong answer */

    /* The count is validated at 64 just above, so the array lives in fixed
     * storage rather than being heap-allocated and freed every 5-second poll.
     * Only ever the UI thread. */
    static HgKmtAdapterInfo s_adapters[64];
    SecureZeroMemory(s_adapters, sizeof(s_adapters));
    enumeration.pAdapters = s_adapters;

    BOOL found = FALSE;
    int hottest = 0;
    if (s_pfnEnumAdapters2(&enumeration) == 0) {
        for (ULONG i = 0; i < enumeration.NumAdapters; ++i) {
            HgKmtAdapterPerfData perf;
            SecureZeroMemory(&perf, sizeof(perf));

            HgKmtQueryAdapterInfo request;
            SecureZeroMemory(&request, sizeof(request));
            request.hAdapter = enumeration.pAdapters[i].hAdapter;
            request.Type = HG_KMTQAITYPE_ADAPTERPERFDATA;
            request.pPrivateDriverData = &perf;
            request.PrivateDriverDataSize = (UINT)sizeof(perf);

            int celsius = 0;
            if (s_pfnQueryAdapterInfo(&request) == 0 && hg_kmt_temperature(&perf, &celsius)) {
                /* A hybrid machine has more than one adapter; the one worth
                 * showing is whichever is actually working. */
                if (!found || celsius > hottest) {
                    hottest = celsius;
                    found = TRUE;
                }
            }

            HgKmtCloseAdapter closing;
            closing.hAdapter = enumeration.pAdapters[i].hAdapter;
            s_pfnCloseAdapter(&closing);
        }
    }

    if (found)
        *out_celsius = hottest;
    return found;
}
