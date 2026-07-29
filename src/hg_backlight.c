/* The internal panel's backlight, over WMI.
 *
 * A laptop's built-in display is not a DDC/CI device, so every path in
 * hg_display.c's ladder misses it and it lands on the gamma ramp - which dims
 * the picture while the lamp stays where it was. Windows exposes the real
 * backlight through WMI in root\WMI, which is what its own brightness slider
 * drives:
 *
 *   WmiMonitorBrightness              - CurrentBrightness, already a percentage
 *   WmiMonitorBrightnessMethods       - WmiSetBrightness(Timeout, Brightness)
 *
 * See docs/RFC-2026-07-brightness-control.md. Everything here is bounded in
 * time: WMI is a cross-process call and this code runs on the UI thread. */
#include "hg_utils.h"
#include <wbemidl.h>

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
