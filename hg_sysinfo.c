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
