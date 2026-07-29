/* The root\WMI clients, sharing one connection.
 *
 * Two unrelated things this app needs live in the same namespace, and a COM
 * activation each would be wasteful, so they sit together behind one cached
 * connection rather than in two modules that each open their own.
 *
 *   WmiMonitorBrightness / WmiMonitorBrightnessMethods
 *       The internal panel's backlight. A laptop's built-in display is not a
 *       DDC/CI device, so every path in hg_display.c's ladder misses it and it
 *       would land on the gamma ramp - which dims the picture while the lamp
 *       stays where it was. This is what Windows' own slider drives.
 *       See docs/RFC-2026-07-brightness-control.md.
 *
 *   MSAcpi_ThermalZoneTemperature
 *       An ACPI thermal zone, in tenths of a degree Kelvin. It is the only way
 *       to a temperature that needs no driver and no elevation - and it is a
 *       zone on the board, not the CPU die. See docs/RFC-2026-07-temperature.md,
 *       which says why the accurate path is the one being declined.
 *
 * Everything here is bounded in time: WMI is a cross-process call and this code
 * runs on the UI thread. */
#include "hg_utils.h"
#include <wbemidl.h>
#include <pdh.h>
#include <pdhmsg.h>

/* Declared by hand for the same reason the audio GUIDs are: linking the uuid
 * import library clashes with this project's manual COM declarations. */
static const GUID HG_CLSID_WbemLocator = {
    0x4590f811, 0x1d3a, 0x11d0, {0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};
static const GUID HG_IID_IWbemLocator = {
    0xdc12a687, 0x737f, 0x11cf, {0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};

/* WMI can take a noticeable moment on a cold call and this is the UI thread, so
 * nothing here is allowed to wait indefinitely. */
#define HG_WMI_TIMEOUT_MS 1500
#define HG_WMI_SET_TIMEOUT_SEC 2

/* Connecting to WMI costs a COM activation and a cross-process handshake, and
 * the refresh timer asks for brightness every few seconds, so the connection is
 * made once and kept. It is dropped on any failure, which is what lets a
 * session that lost the service recover by reconnecting on the next call. */
static IWbemServices *s_services = NULL;

static IWbemServices *hg_wmi_open(void)
{
    IWbemLocator *locator = NULL;
    if (FAILED(CoCreateInstance(&HG_CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &HG_IID_IWbemLocator,
                                (void **)&locator)) ||
        !locator) {
        return NULL;
    }

    BSTR ns = SysAllocString(L"ROOT\\WMI");
    IWbemServices *services = NULL;
    HRESULT hr = ns ? locator->lpVtbl->ConnectServer(locator, ns, NULL, NULL, NULL, 0, NULL, NULL, &services)
                    : E_OUTOFMEMORY;
    if (ns)
        SysFreeString(ns);
    HG_RELEASE_COM(locator);

    if (FAILED(hr) || !services)
        return NULL;

    /* WMI rejects calls on a proxy left at the default security settings; this
     * is the blanket its own documentation asks every client to set. */
    if (FAILED(CoSetProxyBlanket((IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                 RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE))) {
        HG_RELEASE_COM(services);
        return NULL;
    }
    return services;
}

static void hg_wmi_drop(void)
{
    HG_RELEASE_COM(s_services);
}

static IWbemServices *hg_wmi_services(void)
{
    if (!s_services)
        s_services = hg_wmi_open();
    return s_services;
}

void hg_backlight_shutdown(void)
{
    hg_wmi_drop();
}

/* First instance of a class, or NULL. There is one integrated panel on the
 * machines this path exists for; a second would need its InstanceName matched
 * against the display, which is recorded as a limitation rather than guessed at. */
static IWbemClassObject *hg_wmi_first_instance(IWbemServices *services, const WCHAR *wql)
{
    BSTR language = SysAllocString(L"WQL");
    BSTR query = SysAllocString(wql);
    IEnumWbemClassObject *rows = NULL;

    HRESULT hr = (language && query)
                     ? services->lpVtbl->ExecQuery(services, language, query,
                                                   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &rows)
                     : E_OUTOFMEMORY;
    if (language)
        SysFreeString(language);
    if (query)
        SysFreeString(query);
    if (FAILED(hr) || !rows)
        return NULL;

    IWbemClassObject *object = NULL;
    ULONG returned = 0;
    if (FAILED(rows->lpVtbl->Next(rows, HG_WMI_TIMEOUT_MS, 1, &object, &returned)) || returned != 1) {
        object = NULL;
    }
    HG_RELEASE_COM(rows);
    return object;
}

/* WMI hands integers back in whichever integer variant it feels like, so the
 * caller asks for a number rather than for a type. */
static BOOL hg_variant_to_int(const VARIANT *value, int *out)
{
    switch (V_VT(value)) {
    case VT_UI1:
        *out = (int)V_UI1(value);
        return TRUE;
    case VT_I2:
        *out = (int)V_I2(value);
        return TRUE;
    case VT_I4:
        *out = (int)V_I4(value);
        return TRUE;
    case VT_UI2:
        *out = (int)V_UI2(value);
        return TRUE;
    case VT_UI4:
        *out = (int)V_UI4(value);
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL hg_backlight_get(int *out_percent)
{
    if (!out_percent)
        return FALSE;

    IWbemServices *services = hg_wmi_services();
    if (!services)
        return FALSE;

    BOOL ok = FALSE;
    IWbemClassObject *panel = hg_wmi_first_instance(services, L"SELECT CurrentBrightness FROM WmiMonitorBrightness");
    if (panel) {
        VARIANT value;
        VariantInit(&value);
        if (SUCCEEDED(panel->lpVtbl->Get(panel, L"CurrentBrightness", 0, &value, NULL, NULL))) {
            int percent = 0;
            if (hg_variant_to_int(&value, &percent)) {
                /* CurrentBrightness is already a percentage, unlike every
                 * DDC/CI path, which speaks the monitor's own scale. */
                if (percent < 0)
                    percent = 0;
                if (percent > 100)
                    percent = 100;
                *out_percent = percent;
                ok = TRUE;
            }
        }
        VariantClear(&value);
        HG_RELEASE_COM(panel);
    }

    if (!ok)
        hg_wmi_drop(); /* a dead connection must not be kept and retried forever */
    return ok;
}

BOOL hg_backlight_available(void)
{
    int percent = 0;
    return hg_backlight_get(&percent);
}

/* ------------------------------------------------------------- thermal zone */

/* Tenths of a degree Kelvin is what the class holds; nothing outside wants to
 * know that. Zero Celsius is 2731.5 tenths, and rounding is to nearest so a
 * reading does not drift a degree low. */
static BOOL hg_thermal_to_celsius(int tenths_kelvin, int *out_celsius)
{
    if (tenths_kelvin <= 0)
        return FALSE;

    int tenths_celsius = tenths_kelvin - 2732; /* 273.15 K, rounded to the tenth */
    int celsius = (tenths_celsius >= 0) ? (tenths_celsius + 5) / 10 : (tenths_celsius - 5) / 10;

    /* Firmware that answers with a placeholder rather than a sensor is common
     * enough to be worth refusing here; a reading outside this is not a
     * temperature this app should draw. */
    if (celsius < -50 || celsius > 200)
        return FALSE;

    *out_celsius = celsius;
    return TRUE;
}

/* Every zone the WMI class reports, not just the first. Firmware routinely
 * declares several and puts a placeholder in front of the live one. */
static int hg_wmi_zones(HgThermalZone *out, int max)
{
    IWbemServices *services = hg_wmi_services();
    if (!services)
        return 0;

    BSTR language = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT InstanceName, CurrentTemperature FROM MSAcpi_ThermalZoneTemperature");
    IEnumWbemClassObject *rows = NULL;
    HRESULT hr = (language && query)
                     ? services->lpVtbl->ExecQuery(services, language, query,
                                                   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &rows)
                     : E_OUTOFMEMORY;
    if (language)
        SysFreeString(language);
    if (query)
        SysFreeString(query);
    if (FAILED(hr) || !rows)
        return 0;

    int count = 0;
    while (count < max) {
        IWbemClassObject *object = NULL;
        ULONG returned = 0;
        if (FAILED(rows->lpVtbl->Next(rows, HG_WMI_TIMEOUT_MS, 1, &object, &returned)) || returned != 1 || !object)
            break;

        VARIANT value;
        VariantInit(&value);
        int tenths = 0;
        int celsius = 0;
        if (SUCCEEDED(object->lpVtbl->Get(object, L"CurrentTemperature", 0, &value, NULL, NULL)) &&
            hg_variant_to_int(&value, &tenths) && hg_thermal_to_celsius(tenths, &celsius)) {
            out[count].celsius = celsius;
            out[count].from_counter = FALSE;

            VARIANT name;
            VariantInit(&name);
            if (SUCCEEDED(object->lpVtbl->Get(object, L"InstanceName", 0, &name, NULL, NULL)) &&
                V_VT(&name) == VT_BSTR && V_BSTR(&name)) {
                StringCchCopyW(out[count].name, HG_ARRAYSIZE(out[count].name), V_BSTR(&name));
            } else {
                StringCchCopyW(out[count].name, HG_ARRAYSIZE(out[count].name), L"(zone)");
            }
            VariantClear(&name);
            count++;
        }
        VariantClear(&value);
        HG_RELEASE_COM(object);
    }

    HG_RELEASE_COM(rows);
    return count;
}

/* ------------------------------------------- the same zones, the other way
 *
 * Windows also publishes each ACPI thermal zone as a performance counter, in
 * degrees Kelvin. It is the same firmware data behind a different window, and
 * the two do not always agree about being available: a machine whose WMI class
 * answers nothing sometimes has counters, and the reverse happens too.
 *
 * PdhAddEnglishCounter, not PdhAddCounter: the counter object's name is
 * localised, so the literal path below would not resolve on a Windows whose
 * display language is not English. */

typedef PDH_STATUS (WINAPI *PFN_PdhOpenQueryW)(LPCWSTR, DWORD_PTR, PDH_HQUERY *);
typedef PDH_STATUS (WINAPI *PFN_PdhAddEnglishCounterW)(PDH_HQUERY, LPCWSTR, DWORD_PTR, PDH_HCOUNTER *);
typedef PDH_STATUS (WINAPI *PFN_PdhCollectQueryData)(PDH_HQUERY);
typedef PDH_STATUS (WINAPI *PFN_PdhGetFormattedCounterArrayW)(PDH_HCOUNTER, DWORD, LPDWORD, LPDWORD,
                                                              PPDH_FMT_COUNTERVALUE_ITEM_W);
typedef PDH_STATUS (WINAPI *PFN_PdhCloseQuery)(PDH_HQUERY);

static PFN_PdhOpenQueryW s_pfnPdhOpen = NULL;
static PFN_PdhAddEnglishCounterW s_pfnPdhAdd = NULL;
static PFN_PdhCollectQueryData s_pfnPdhCollect = NULL;
static PFN_PdhGetFormattedCounterArrayW s_pfnPdhArray = NULL;
static PFN_PdhCloseQuery s_pfnPdhClose = NULL;
static BOOL s_pdh_initialized = FALSE;

static BOOL hg_pdh_ready(void)
{
    if (!s_pdh_initialized) {
        s_pdh_initialized = TRUE;
        HMODULE pdh = LoadLibraryW(L"pdh.dll");
        if (pdh) {
            union {
                FARPROC proc;
                PFN_PdhOpenQueryW open;
                PFN_PdhAddEnglishCounterW add;
                PFN_PdhCollectQueryData collect;
                PFN_PdhGetFormattedCounterArrayW array;
                PFN_PdhCloseQuery close;
            } loader;
            loader.proc = GetProcAddress(pdh, "PdhOpenQueryW");
            s_pfnPdhOpen = loader.open;
            loader.proc = GetProcAddress(pdh, "PdhAddEnglishCounterW");
            s_pfnPdhAdd = loader.add;
            loader.proc = GetProcAddress(pdh, "PdhCollectQueryData");
            s_pfnPdhCollect = loader.collect;
            loader.proc = GetProcAddress(pdh, "PdhGetFormattedCounterArrayW");
            s_pfnPdhArray = loader.array;
            loader.proc = GetProcAddress(pdh, "PdhCloseQuery");
            s_pfnPdhClose = loader.close;
        }
    }
    return s_pfnPdhOpen && s_pfnPdhAdd && s_pfnPdhCollect && s_pfnPdhArray && s_pfnPdhClose;
}

static int hg_counter_zones(HgThermalZone *out, int max)
{
    if (!hg_pdh_ready())
        return 0;

    PDH_HQUERY query = NULL;
    if (s_pfnPdhOpen(NULL, 0, &query) != ERROR_SUCCESS || !query)
        return 0;

    int count = 0;
    PDH_HCOUNTER counter = NULL;
    if (s_pfnPdhAdd(query, L"\\Thermal Zone Information(*)\\Temperature", 0, &counter) == ERROR_SUCCESS &&
        s_pfnPdhCollect(query) == ERROR_SUCCESS) {
        DWORD size = 0;
        DWORD items = 0;
        PDH_STATUS status = s_pfnPdhArray(counter, PDH_FMT_DOUBLE, &size, &items, NULL);
        if (status == (PDH_STATUS)PDH_MORE_DATA && size > 0 && size < (1u << 20)) {
            PDH_FMT_COUNTERVALUE_ITEM_W *values = (PDH_FMT_COUNTERVALUE_ITEM_W *)malloc(size);
            if (values && s_pfnPdhArray(counter, PDH_FMT_DOUBLE, &size, &items, values) == ERROR_SUCCESS) {
                for (DWORD i = 0; i < items && count < max; ++i) {
                    double kelvin = values[i].FmtValue.doubleValue;
                    int celsius = (int)(kelvin - 273.15 + 0.5);
                    if (celsius < 1 || celsius > 150)
                        continue;
                    StringCchCopyW(out[count].name, HG_ARRAYSIZE(out[count].name),
                                   values[i].szName ? values[i].szName : L"(zone)");
                    out[count].celsius = celsius;
                    out[count].from_counter = TRUE;
                    count++;
                }
            }
            free(values);
        }
    }

    s_pfnPdhClose(query);
    return count;
}

int hg_thermal_enumerate(HgThermalZone *out, int max)
{
    if (!out || max <= 0)
        return 0;
    int count = hg_wmi_zones(out, max);
    if (count < max)
        count += hg_counter_zones(out + count, max - count);
    return count;
}

/* ------------------------------------------------------------ which zone
 *
 * Firmware commonly declares a zone it never updates. A reading that never
 * moves is indistinguishable from a working sensor in any single sample, so
 * the zones are remembered across samples and one that has been seen to change
 * is preferred over one that has not. */
typedef struct HgZoneMemory {
    WCHAR name[64];
    int first;
    BOOL moved;
    BOOL used;
} HgZoneMemory;

static HgZoneMemory s_zone_memory[HG_THERMAL_MAX_ZONES];

static BOOL hg_zone_remember(const WCHAR *name, int celsius)
{
    for (int i = 0; i < (int)HG_ARRAYSIZE(s_zone_memory); ++i) {
        if (!s_zone_memory[i].used)
            continue;
        if (wcscmp(s_zone_memory[i].name, name) != 0)
            continue;
        if (celsius != s_zone_memory[i].first)
            s_zone_memory[i].moved = TRUE;
        return s_zone_memory[i].moved;
    }

    for (int i = 0; i < (int)HG_ARRAYSIZE(s_zone_memory); ++i) {
        if (s_zone_memory[i].used)
            continue;
        s_zone_memory[i].used = TRUE;
        s_zone_memory[i].moved = FALSE;
        s_zone_memory[i].first = celsius;
        StringCchCopyW(s_zone_memory[i].name, HG_ARRAYSIZE(s_zone_memory[i].name), name);
        return FALSE;
    }
    return FALSE;
}

/* A zone whose name says CPU is worth more than one that says nothing. */
static BOOL hg_zone_names_cpu(const WCHAR *name)
{
    return StrStrIW(name, L"CPU") != NULL || StrStrIW(name, L"TZ0") != NULL;
}

BOOL hg_thermal_zone_celsius(int *out_celsius)
{
    if (!out_celsius)
        return FALSE;

    HgThermalZone zones[HG_THERMAL_MAX_ZONES];
    int count = hg_thermal_enumerate(zones, (int)HG_ARRAYSIZE(zones));
    if (count <= 0)
        return FALSE;

    int best = -1;
    int best_score = -1;
    for (int i = 0; i < count; ++i) {
        BOOL moved = hg_zone_remember(zones[i].name, zones[i].celsius);
        /* Having been seen to move outweighs being named after the CPU, which
         * outweighs nothing at all; the hottest wins an outright tie. */
        int score = (moved ? 4 : 0) + (hg_zone_names_cpu(zones[i].name) ? 2 : 0);
        if (score > best_score || (score == best_score && best >= 0 && zones[i].celsius > zones[best].celsius)) {
            best = i;
            best_score = score;
        }
    }

    if (best < 0)
        return FALSE;
    *out_celsius = zones[best].celsius;
    return TRUE;
}

/* ---------------------------------------------------------------- backlight */

BOOL hg_backlight_set(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;

    IWbemServices *services = hg_wmi_services();
    if (!services)
        return FALSE;

    BOOL ok = FALSE;
    IWbemClassObject *instance = hg_wmi_first_instance(services, L"SELECT * FROM WmiMonitorBrightnessMethods");
    if (instance) {
        /* The method is called on a particular instance, addressed by the
         * relative path WMI stamped on it. */
        VARIANT path;
        VariantInit(&path);
        if (SUCCEEDED(instance->lpVtbl->Get(instance, L"__RELPATH", 0, &path, NULL, NULL)) && V_VT(&path) == VT_BSTR) {
            IWbemClassObject *definition = NULL;
            IWbemClassObject *in_signature = NULL;
            BSTR method = SysAllocString(L"WmiSetBrightness");
            BSTR class_name = SysAllocString(L"WmiMonitorBrightnessMethods");

            if (method && class_name &&
                SUCCEEDED(services->lpVtbl->GetObject(services, class_name, 0, NULL, &definition, NULL)) &&
                definition &&
                SUCCEEDED(definition->lpVtbl->GetMethod(definition, L"WmiSetBrightness", 0, &in_signature, NULL)) &&
                in_signature) {
                IWbemClassObject *arguments = NULL;
                if (SUCCEEDED(in_signature->lpVtbl->SpawnInstance(in_signature, 0, &arguments)) && arguments) {
                    VARIANT timeout;
                    VariantInit(&timeout);
                    V_VT(&timeout) = VT_I4;
                    V_I4(&timeout) = HG_WMI_SET_TIMEOUT_SEC;

                    VARIANT level;
                    VariantInit(&level);
                    V_VT(&level) = VT_UI1;
                    V_UI1(&level) = (BYTE)percent;

                    if (SUCCEEDED(arguments->lpVtbl->Put(arguments, L"Timeout", 0, &timeout, 0)) &&
                        SUCCEEDED(arguments->lpVtbl->Put(arguments, L"Brightness", 0, &level, 0))) {
                        ok = SUCCEEDED(services->lpVtbl->ExecMethod(services, V_BSTR(&path), method, 0, NULL,
                                                                    arguments, NULL, NULL));
                    }

                    VariantClear(&timeout);
                    VariantClear(&level);
                    HG_RELEASE_COM(arguments);
                }
            }

            if (method)
                SysFreeString(method);
            if (class_name)
                SysFreeString(class_name);
            HG_RELEASE_COM(in_signature);
            HG_RELEASE_COM(definition);
        }
        VariantClear(&path);
        HG_RELEASE_COM(instance);
    }

    if (!ok)
        hg_wmi_drop();
    return ok;
}
