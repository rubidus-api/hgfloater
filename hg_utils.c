#include "hg_utils.h"
#include <knownfolders.h>

const GUID CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
const GUID IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
const GUID IID_IAudioEndpointVolume = {0x5cdf2c82, 0x841e, 0x4546, {0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a}};

static const GUID CLSID_CPolicyConfigClient = {
    0x870af99c, 0x171d, 0x4f9e, {0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9}};

static const GUID IID_IPolicyConfig = {0xf8679f50, 0x850a, 0x41cf, {0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8}};

typedef struct IPolicyConfigVtbl {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(void *This, REFIID riid, void **ppvObject);
    ULONG(STDMETHODCALLTYPE *AddRef)(void *This);
    ULONG(STDMETHODCALLTYPE *Release)(void *This);

    HRESULT(STDMETHODCALLTYPE *GetMixFormat)(void *This, PCWSTR pszDeviceName, WAVEFORMATEX **ppFormat);
    HRESULT(STDMETHODCALLTYPE *GetDeviceFormat)(void *This, PCWSTR pszDeviceName, INT bDefault,
                                                WAVEFORMATEX **ppFormat);
    HRESULT(STDMETHODCALLTYPE *ResetDeviceFormat)(void *This, PCWSTR pszDeviceName);
    HRESULT(STDMETHODCALLTYPE *SetDeviceFormat)(void *This, PCWSTR pszDeviceName, WAVEFORMATEX *pEndpointFormat,
                                                WAVEFORMATEX *pMixFormat);
    HRESULT(STDMETHODCALLTYPE *GetProcessingPeriod)(void *This, PCWSTR pszDeviceName, INT bDefault,
                                                    REFERENCE_TIME *pmntDefaultPeriod,
                                                    REFERENCE_TIME *pmntMinimumPeriod);
    HRESULT(STDMETHODCALLTYPE *SetProcessingPeriod)(void *This, PCWSTR pszDeviceName, REFERENCE_TIME *pmntPeriod);
    HRESULT(STDMETHODCALLTYPE *GetShareMode)(void *This, PCWSTR pszDeviceName, INT *pMode);
    HRESULT(STDMETHODCALLTYPE *SetShareMode)(void *This, PCWSTR pszDeviceName, INT mode);
    HRESULT(STDMETHODCALLTYPE *GetPropertyValue)(void *This, PCWSTR pszDeviceName, const PROPERTYKEY *key,
                                                 PROPVARIANT *pv);
    HRESULT(STDMETHODCALLTYPE *SetPropertyValue)(void *This, PCWSTR pszDeviceName, const PROPERTYKEY *key,
                                                 PROPVARIANT *pv);
    HRESULT(STDMETHODCALLTYPE *SetDefaultEndpoint)(void *This, PCWSTR pszDeviceName, ERole role);
    HRESULT(STDMETHODCALLTYPE *SetEndpointVisibility)(void *This, PCWSTR pszDeviceName, INT bVisible);
} IPolicyConfigVtbl;

typedef struct IPolicyConfig {
    IPolicyConfigVtbl *lpVtbl;
} IPolicyConfig;

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

int get_system_brightness(void)
{
    init_dxva2();
    if (!s_hDxva2 || !s_pfnGetNum || !s_pfnGetPhys || !s_pfnDestroy || !s_pfnGetBright)
        return hg_g_global_brightness;

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

int get_system_volume()
{
    float volume = 0.0f;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
                                                   (void **)&endpointVolume))) {
                endpointVolume->lpVtbl->GetMasterVolumeLevelScalar(endpointVolume, &volume);
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
    return (int)(volume * 100.0f + 0.5f);
}

void set_system_volume(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    float volume = (float)percent / 100.0f;

    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
                                                   (void **)&endpointVolume))) {
                endpointVolume->lpVtbl->SetMasterVolumeLevelScalar(endpointVolume, volume, NULL);
                if (percent > 0) {
                    endpointVolume->lpVtbl->SetMute(endpointVolume, FALSE, NULL);
                }
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
}

int get_system_mute(void)
{
    BOOL mute = FALSE;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
                                                   (void **)&endpointVolume))) {
                endpointVolume->lpVtbl->GetMute(endpointVolume, &mute);
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
    return (int)mute;
}

void set_system_mute(int mute)
{
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
                                                   (void **)&endpointVolume))) {
                endpointVolume->lpVtbl->SetMute(endpointVolume, (BOOL)mute, NULL);
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
}

void update_audio_device_list()
{
    hg_g_audio_device_count = 0;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDeviceCollection *collection = NULL;
    IMMDevice *default_dev = NULL;
    LPWSTR default_id = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &default_dev))) {
            default_dev->lpVtbl->GetId(default_dev, &default_id);
            default_dev->lpVtbl->Release(default_dev);
        }

        if (SUCCEEDED(enumerator->lpVtbl->EnumAudioEndpoints(enumerator, eRender, DEVICE_STATE_ACTIVE, &collection))) {
            UINT count = 0;
            collection->lpVtbl->GetCount(collection, &count);
            for (UINT i = 0; i < count && hg_g_audio_device_count < HG_MAX_AUDIO_DEVICES; i++) {
                IMMDevice *device = NULL;
                if (SUCCEEDED(collection->lpVtbl->Item(collection, i, &device))) {
                    LPWSTR id = NULL;
                    if (SUCCEEDED(device->lpVtbl->GetId(device, &id))) {
                        IPropertyStore *props = NULL;
                        if (SUCCEEDED(device->lpVtbl->OpenPropertyStore(device, STGM_READ, &props))) {
                            PROPVARIANT var;
                            PropVariantInit(&var);
                            if (SUCCEEDED(props->lpVtbl->GetValue(props, &PKEY_Device_FriendlyName, &var)) &&
                                var.vt == VT_LPWSTR && var.pwszVal) {

                                if (SUCCEEDED(StringCchCopyW(hg_g_audio_devices[hg_g_audio_device_count].name,
                                                             HG_MAX_STR, var.pwszVal)) &&
                                    SUCCEEDED(StringCchCopyW(hg_g_audio_devices[hg_g_audio_device_count].id, HG_MAX_STR,
                                                             id))) {

                                    hg_g_audio_devices[hg_g_audio_device_count].is_default =
                                        (default_id && wcscmp(id, default_id) == 0);
                                    hg_g_audio_device_count++;
                                }
                            }
                            PropVariantClear(&var);
                            props->lpVtbl->Release(props);
                        }
                        CoTaskMemFree(id);
                    }
                    device->lpVtbl->Release(device);
                }
            }
            collection->lpVtbl->Release(collection);
        }
        if (default_id)
            CoTaskMemFree(default_id);
        enumerator->lpVtbl->Release(enumerator);
    }
}

BOOL set_default_audio_device(const WCHAR *device_id)
{
    if (!device_id || !*device_id)
        return FALSE;

    IPolicyConfig *policy = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_CPolicyConfigClient, NULL, CLSCTX_ALL, &IID_IPolicyConfig, (void **)&policy);

    if (FAILED(hr) || !policy) {
        return FALSE;
    }

    BOOL ok = TRUE;

    if (FAILED(policy->lpVtbl->SetDefaultEndpoint(policy, device_id, eConsole)))
        ok = FALSE;
    if (FAILED(policy->lpVtbl->SetDefaultEndpoint(policy, device_id, eMultimedia)))
        ok = FALSE;
    if (FAILED(policy->lpVtbl->SetDefaultEndpoint(policy, device_id, eCommunications)))
        ok = FALSE;

    policy->lpVtbl->Release(policy);
    return ok;
}

static void refresh_system_accent_color(void)
{
    DWORD accent_color = 0;
    BOOL opaque_blend = FALSE;

    if (SUCCEEDED(DwmGetColorizationColor(&accent_color, &opaque_blend))) {
        hg_g_system_accent_color = (COLORREF)(accent_color & 0x00FFFFFFu);
        hg_g_has_system_accent_color = TRUE;
    } else {
        hg_g_system_accent_color = GetSysColor(COLOR_HOTLIGHT);
        hg_g_has_system_accent_color = FALSE;
    }
}

void init_color_scheme(void)
{
    hg_g_color_scheme_light = (color_scheme_t){
        .bg = HG_THEME_CUSTOM_BG,
        .border = HG_THEME_CUSTOM_BORDER,
        .text = HG_THEME_CUSTOM_TEXT,
        .flash = HG_THEME_CUSTOM_FLASH,
        .selected = HG_THEME_CUSTOM_SELECTED,
    };

    hg_g_color_scheme_dark = (color_scheme_t){
        .bg = GetSysColor(COLOR_WINDOW),
        .border = GetSysColor(COLOR_WINDOWFRAME),
        .text = GetSysColor(COLOR_WINDOWTEXT),
        .flash = GetSysColor(COLOR_HOTLIGHT),
        .selected = GetSysColor(COLOR_HIGHLIGHT),
    };
}

void update_theme_colors()
{
    HIGHCONTRASTW hc = {0};
    hc.cbSize = sizeof(HIGHCONTRASTW);
    BOOL is_hc = FALSE;
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &hc, 0)) {
        if (hc.dwFlags & HCF_HIGHCONTRASTON)
            is_hc = TRUE;
    }

    hg_g_is_high_contrast = is_hc;
    refresh_system_accent_color();

    hg_g_is_dark_mode = 0;
    DWORD use_light_theme = 1;
    DWORD cbData = sizeof(use_light_theme);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&use_light_theme, &cbData);
        RegCloseKey(hKey);
    }
    if (!hg_g_is_high_contrast && use_light_theme == 0)
        hg_g_is_dark_mode = 1;

    init_color_scheme();

    if (!hg_g_is_high_contrast && hg_g_has_system_accent_color) {
        hg_g_color_scheme_dark.flash = hg_g_system_accent_color;
    }

    if (hg_g_is_high_contrast) {
        hg_g_color_scheme_selected = (color_scheme_t){
            .bg = GetSysColor(COLOR_BTNFACE),
            .border = GetSysColor(COLOR_WINDOWFRAME),
            .text = GetSysColor(COLOR_BTNTEXT),
            .flash = GetSysColor(COLOR_HOTLIGHT),
            .selected = GetSysColor(COLOR_HIGHLIGHT),
        };
    } else if (hg_g_is_dark_mode) {
        hg_g_color_scheme_selected = hg_g_color_scheme_dark;
    } else {
        hg_g_color_scheme_selected = hg_g_color_scheme_light;
    }
}

BOOL should_refresh_theme_on_setting_change(LPARAM l_param)
{
    const WCHAR *setting = (const WCHAR *)l_param;

    if (!setting || !*setting) {
        return TRUE;
    }

    return lstrcmpiW(setting, L"ImmersiveColorSet") == 0 || lstrcmpiW(setting, L"AppsUseLightTheme") == 0 ||
           lstrcmpiW(setting, L"WindowsThemeElement") == 0 || lstrcmpiW(setting, L"HighContrast") == 0;
}

void apply_dwm_attributes(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return;
    BOOL use_immersive_dark_mode = hg_g_is_dark_mode && !hg_g_is_high_contrast;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_immersive_dark_mode,
                          sizeof(use_immersive_dark_mode));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &hg_g_color_scheme_selected.border,
                          sizeof(hg_g_color_scheme_selected.border));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &hg_g_color_scheme_selected.bg,
                          sizeof(hg_g_color_scheme_selected.bg));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &hg_g_color_scheme_selected.text,
                          sizeof(hg_g_color_scheme_selected.text));

    int corner_pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));
}

void refresh_theme_surfaces(HWND hwnd)
{
    update_theme_colors();

    apply_dwm_attributes(hwnd);
    if (hg_g_floater_wnd && hg_g_floater_wnd != hwnd) {
        apply_dwm_attributes(hg_g_floater_wnd);
    }
    if (hg_g_about_wnd && hg_g_about_wnd != hwnd) {
        apply_dwm_attributes(hg_g_about_wnd);
    }

    if (hg_g_main_bg_brush) {
        DeleteObject(hg_g_main_bg_brush);
        hg_g_main_bg_brush = NULL;
    }
    hg_g_main_bg_brush = CreateSolidBrush(HG_CLICKABLE_BG);
    if (hwnd && IsWindow(hwnd)) {
        SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hg_g_main_bg_brush);
    }

    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        SetClassLongPtrW(hg_g_about_wnd, GCLP_HBRBACKGROUND, (LONG_PTR)hg_g_main_bg_brush);
        InvalidateRect(hg_g_about_wnd, NULL, TRUE);
        HWND edit_wnd = GetDlgItem(hg_g_about_wnd, 100);
        if (edit_wnd) {
            InvalidateRect(edit_wnd, NULL, TRUE);
        }
    }

    if (hg_g_edit_bg_brush) {
        DeleteObject(hg_g_edit_bg_brush);
        hg_g_edit_bg_brush = NULL;
    }

    if (hg_g_tooltip_wnd && IsWindow(hg_g_tooltip_wnd)) {
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
        SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
    }



    if (hwnd) {
        InvalidateRect(hwnd, NULL, TRUE);
    }
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
    }
    if (hg_g_toolbar_wnd && IsWindow(hg_g_toolbar_wnd)) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
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

HRESULT hellgates_wsprintf(LPWSTR dest, size_t dest_size, LPCWSTR format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    HRESULT hr = StringCchVPrintfW(dest, dest_size, format, arg_list);
    va_end(arg_list);
    return hr;
}

void normalize_path_for_api(const WCHAR *input, WCHAR *output, size_t output_size)
{
    if (!input || !output || output_size == 0)
        return;

    WCHAR full[HG_MAX_PATH];
    if (GetFullPathNameW(input, HG_MAX_PATH, full, NULL) == 0) {
        StringCchCopyW(output, output_size, input);
        return;
    }

    if (wcsncmp(full, L"\\\\?\\", 4) == 0) {
        StringCchCopyW(output, output_size, full);
        return;
    }

    if (iswalpha(full[0]) && full[1] == L':' && full[2] == L'\\') {
        StringCchPrintfW(output, output_size, L"\\\\?\\%ls", full);
    } else if (full[0] == L'\\' && full[1] == L'\\') {
        StringCchPrintfW(output, output_size, L"\\\\?\\UNC\\%ls", full + 2);
    } else {
        StringCchCopyW(output, output_size, full);
    }
}

void init_paths()
{
    WCHAR profile[HG_MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"USERPROFILE", profile, HG_MAX_PATH) == 0) {
        StringCchCopyW(profile, HG_MAX_PATH, L"C:\\");
    }
    hellgates_wsprintf(hg_g_base_path, HG_MAX_PATH, L"%ls\\.HellGates\\hgfloater", profile);
    hellgates_wsprintf(hg_g_shortcuts_path, HG_MAX_PATH, L"%ls\\shortcuts", hg_g_base_path);
    hellgates_wsprintf(hg_g_config_path, HG_MAX_PATH, L"%ls\\config.ini", hg_g_base_path);

    SHCreateDirectoryExW(NULL, hg_g_base_path, NULL);
    SHCreateDirectoryExW(NULL, hg_g_shortcuts_path, NULL);

    WCHAR legacy_config_path[HG_MAX_PATH];
    hellgates_wsprintf(legacy_config_path, HG_MAX_PATH, L"%ls\\config.ini.txt", hg_g_base_path);

    WCHAR norm_legacy[HG_MAX_PATH], norm_config[HG_MAX_PATH];
    normalize_path_for_api(legacy_config_path, norm_legacy, HG_MAX_PATH);
    normalize_path_for_api(hg_g_config_path, norm_config, HG_MAX_PATH);

    if (GetFileAttributesW(norm_legacy) != INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(norm_config) == INVALID_FILE_ATTRIBUTES) {
        MoveFileW(norm_legacy, norm_config);
    }
}

BOOL is_alt_tab_window(HWND hwnd)
{
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd || hwnd == hg_g_about_wnd)
        return FALSE;
    if (!IsWindow(hwnd))
        return FALSE;
    if (!IsWindowVisible(hwnd))
        return FALSE;

    if (GetWindowTextLengthW(hwnd) == 0)
        return FALSE;

    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ex_style & WS_EX_TOOLWINDOW)
        return FALSE;

    HWND owner_hwnd = GetWindow(hwnd, GW_OWNER);
    if (owner_hwnd != NULL && !(ex_style & WS_EX_APPWINDOW))
        return FALSE;

    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if (cloaked != 0)
            return FALSE;
    }

    return TRUE;
}

void get_process_name_by_hwnd(HWND hwnd, WCHAR *out_name, size_t out_size, DWORD *out_pid)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid)
        *out_pid = pid;

    if (out_name && out_size > 0) {
        out_name[0] = L'\0';
    }
    if (!out_name || out_size == 0 || pid == 0) {
        return;
    }

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process_handle) {
        StringCchCopyW(out_name, out_size, L"Unknown");
        return;
    }

    WCHAR path[HG_MAX_PATH] = {0};
    DWORD q_size = HG_ARRAYSIZE(path);
    if (QueryFullProcessImageNameW(process_handle, 0, path, &q_size)) {
        LPCWSTR exe_name = PathFindFileNameW(path);
        if (!exe_name || !*exe_name)
            exe_name = path;
        StringCchCopyW(out_name, out_size, exe_name);
    } else {
        StringCchCopyW(out_name, out_size, L"Unknown");
    }

    CloseHandle(process_handle);
}

void get_process_path_by_hwnd(HWND hwnd, WCHAR *out_path, size_t out_size, DWORD *out_pid)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid)
        *out_pid = pid;

    if (out_path && out_size > 0) {
        out_path[0] = L'\0';
    }
    if (!out_path || out_size == 0 || pid == 0) {
        return;
    }

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process_handle) {
        return;
    }

    DWORD q_size = (DWORD)out_size;
    QueryFullProcessImageNameW(process_handle, 0, out_path, &q_size);
    CloseHandle(process_handle);
}

static int hg_clamp_icon_size_px(int size_px)
{
    if (size_px < HG_UWP_MIN_ICON_PX)
        return HG_UWP_MIN_ICON_PX;
    if (size_px > HG_UWP_MAX_ICON_PX)
        return HG_UWP_MAX_ICON_PX;
    return size_px;
}

static BOOL CALLBACK find_uwp_child_window(HWND hwnd, LPARAM lParam)
{
    FindUWPChildData *data = (FindUWPChildData *)lParam;
    if (!data || !IsWindow(hwnd))
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == 0 || pid == data->frame_pid) {
        return TRUE;
    }

    WCHAR cls[128] = {0};
    GetClassNameW(hwnd, cls, HG_ARRAYSIZE(cls));

    if (wcscmp(cls, L"Windows.UI.Core.CoreWindow") == 0) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
        return FALSE;
    }

    if (!data->best_hwnd) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
    }

    return TRUE;
}

static BOOL get_real_uwp_child(HWND frame_hwnd, HWND *out_hwnd, DWORD *out_pid)
{
    if (out_hwnd)
        *out_hwnd = NULL;
    if (out_pid)
        *out_pid = 0;
    if (!IsWindow(frame_hwnd))
        return FALSE;

    DWORD frame_pid = 0;
    GetWindowThreadProcessId(frame_hwnd, &frame_pid);
    if (!frame_pid)
        return FALSE;

    FindUWPChildData data;
    ZeroMemory(&data, sizeof(data));
    data.frame_pid = frame_pid;

    EnumChildWindows(frame_hwnd, find_uwp_child_window, (LPARAM)&data);

    if (!data.best_hwnd || !data.best_pid || !IsWindow(data.best_hwnd)) {
        return FALSE;
    }

    if (out_hwnd)
        *out_hwnd = data.best_hwnd;
    if (out_pid)
        *out_pid = data.best_pid;
    return TRUE;
}

static HICON get_icon_from_hwnd_msg(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return NULL;

    HICON h_icon = NULL;
    DWORD_PTR res = 0;

    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon && SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon && SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    return h_icon;
}

static HICON get_icon_from_hwnd_class(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return NULL;

    HICON h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    if (!h_icon)
        h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);

    return h_icon;
}

static HICON get_icon_from_process_exe(DWORD pid, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc)
        return NULL;

    WCHAR path[HG_MAX_PATH] = {0};
    DWORD size = HG_ARRAYSIZE(path);
    HICON icon = NULL;

    if (QueryFullProcessImageNameW(h_proc, 0, path, &size)) {
        SHFILEINFOW sfi = {0};
        if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
            icon = sfi.hIcon;
            if (own_icon)
                *own_icon = TRUE;
        } else if (ExtractIconExW(path, 0, &icon, NULL, 1) > 0 && icon) {
            if (own_icon)
                *own_icon = TRUE;
        }
    }

    CloseHandle(h_proc);
    return icon;
}

static WCHAR *get_aumid_from_hwnd(HWND hwnd)
{
    if (!hwnd)
        return NULL;
    IPropertyStore *pps = NULL;
    HRESULT hr = SHGetPropertyStoreForWindow(hwnd, &IID_IPropertyStore, (void **)&pps);
    if (SUCCEEDED(hr) && pps) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        hr = pps->lpVtbl->GetValue(pps, &PKEY_AppUserModel_ID, &pv);
        WCHAR *aumid = NULL;
        if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal) {
            size_t len = wcslen(pv.pwszVal) + 1;
            aumid = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
            if (aumid) {
                StringCchCopyW(aumid, len, pv.pwszVal);
            }
        }
        PropVariantClear(&pv);
        pps->lpVtbl->Release(pps);
        return aumid;
    }
    return NULL;
}

static WCHAR *get_app_user_model_id_alloc(DWORD pid)
{
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc)
        return NULL;

    UINT32 len = 0;
    LONG rc = GetApplicationUserModelId(h_proc, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        CloseHandle(h_proc);
        return NULL;
    }

    WCHAR *aumid = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!aumid) {
        CloseHandle(h_proc);
        return NULL;
    }

    rc = GetApplicationUserModelId(h_proc, &len, aumid);
    CloseHandle(h_proc);

    if (rc != ERROR_SUCCESS) {
        HeapFree(GetProcessHeap(), 0, aumid);
        return NULL;
    }

    return aumid;
}

static WCHAR *get_package_full_name_alloc(DWORD pid)
{
    if (pid == 0)
        return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc) {
        uwp_debug_log(L"UWP icon: OpenProcess failed");
        return NULL;
    }

    UINT32 len = 0;
    LONG rc = GetPackageFullName(h_proc, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        CloseHandle(h_proc);
        return NULL;
    }

    WCHAR *full = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!full) {
        CloseHandle(h_proc);
        return NULL;
    }

    rc = GetPackageFullName(h_proc, &len, full);
    CloseHandle(h_proc);

    if (rc != ERROR_SUCCESS) {
        uwp_debug_log(L"UWP icon: GetPackageFullName failed");
        HeapFree(GetProcessHeap(), 0, full);
        return NULL;
    }

    return full;
}

static WCHAR *get_package_path_alloc(const WCHAR *full_name)
{
    if (!full_name || !*full_name)
        return NULL;

    UINT32 len = 0;
    LONG rc = GetPackagePathByFullName(full_name, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        uwp_debug_log(L"UWP icon: GetPackagePathByFullName length query failed");
        return NULL;
    }

    WCHAR *path = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!path)
        return NULL;

    rc = GetPackagePathByFullName(full_name, &len, path);
    if (rc != ERROR_SUCCESS) {
        uwp_debug_log(L"UWP icon: GetPackagePathByFullName failed");
        HeapFree(GetProcessHeap(), 0, path);
        return NULL;
    }

    return path;
}

static BOOL is_xml_name_char(WCHAR ch)
{
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9') || ch == L'_' ||
           ch == L'-' || ch == L':' || ch == L'.';
}

static BOOL find_xml_attr_value_safe(const WCHAR *text, const WCHAR *attr, WCHAR *out, size_t out_cch)
{
    if (!text || !attr || !*attr || !out || out_cch == 0)
        return FALSE;
    out[0] = L'\0';

    size_t attr_len = wcslen(attr);
    const WCHAR *p = text;

    while ((p = wcsstr(p, attr)) != NULL) {
        BOOL valid_prev = (p == text) || (!is_xml_name_char(*(p - 1)));
        BOOL valid_next = (!is_xml_name_char(*(p + attr_len)));

        if (valid_prev && valid_next) {
            const WCHAR *cur = p + attr_len;
            while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n')
                cur++;

            if (*cur == L'=') {
                cur++;
                while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n')
                    cur++;

                WCHAR quote = *cur;
                if (quote == L'\'' || quote == L'"') {
                    cur++;
                    const WCHAR *q = wcschr(cur, quote);
                    if (q) {
                        size_t n = (size_t)(q - cur);
                        if (n >= out_cch)
                            n = out_cch - 1;
                        if (FAILED(StringCchCopyNW(out, out_cch, cur, n))) {
                            out[0] = L'\0';
                            return FALSE;
                        }
                        out[n] = L'\0';
                        return TRUE;
                    }
                }
            }
        }

        p += attr_len;
    }

    return FALSE;
}

static BOOL read_utf8_file_to_wide(const WCHAR *path, WCHAR **out_text)
{
    if (!out_text)
        return FALSE;
    *out_text = NULL;
    if (!path || !*path)
        return FALSE;

    WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);

    HANDLE h = CreateFileW(norm, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER li;
    if (!GetFileSizeEx(h, &li) || li.QuadPart <= 0 || li.QuadPart > HG_UWP_MAX_MANIFEST_BYTES) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD bytes = (DWORD)li.QuadPart;
    BYTE *buf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)bytes + 1u);
    if (!buf) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD read = 0;
    BOOL ok = ReadFile(h, buf, bytes, &read, NULL);
    CloseHandle(h);

    if (!ok || read == 0) {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    UINT codepage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int wlen = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, NULL, 0);

    if (wlen <= 0) {
        codepage = CP_ACP;
        flags = 0;
        wlen = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, NULL, 0);
    }

    if (wlen <= 0 || (size_t)wlen > HG_UWP_MAX_MANIFEST_BYTES) {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    WCHAR *wbuf = (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((SIZE_T)wlen + 1u) * sizeof(WCHAR));
    if (!wbuf) {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    int converted = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, wbuf, wlen);
    HeapFree(GetProcessHeap(), 0, buf);

    if (converted != wlen) {
        HeapFree(GetProcessHeap(), 0, wbuf);
        return FALSE;
    }

    wbuf[wlen] = L'\0';
    *out_text = wbuf;
    return TRUE;
}

static BOOL get_logo_relpath_from_manifest(const WCHAR *package_path, WCHAR *out_rel, size_t out_cch)
{
    if (!package_path || !out_rel || out_cch == 0)
        return FALSE;
    out_rel[0] = L'\0';

    WCHAR manifest_path[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(manifest_path, HG_ARRAYSIZE(manifest_path), L"%ls\\AppxManifest.xml", package_path);
    if (FAILED(hr))
        return FALSE;

    WCHAR *manifest = NULL;
    if (!read_utf8_file_to_wide(manifest_path, &manifest)) {
        uwp_debug_log(L"UWP icon: Failed to read AppxManifest.xml");
        return FALSE;
    }

    BOOL ok = find_xml_attr_value_safe(manifest, L"Square44x44Logo", out_rel, out_cch) ||
              find_xml_attr_value_safe(manifest, L"Square150x150Logo", out_rel, out_cch) ||
              find_xml_attr_value_safe(manifest, L"Logo", out_rel, out_cch);

    if (!ok)
        uwp_debug_log(L"UWP icon: Could not find logo attribute");
    HeapFree(GetProcessHeap(), 0, manifest);
    return ok;
}

static BOOL file_exists_w(const WCHAR *path)
{
    WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);
    DWORD attr = GetFileAttributesW(norm);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void normalize_slashes(WCHAR *s)
{
    if (!s)
        return;
    for (; *s; s++) {
        if (*s == L'/')
            *s = L'\\';
    }
}

static BOOL resolve_logo_asset_file(const WCHAR *package_path, const WCHAR *rel_logo, int size_px, WCHAR *out_file,
                                    size_t out_cch)
{
    if (!package_path || !rel_logo || !out_file || out_cch == 0)
        return FALSE;
    out_file[0] = L'\0';
    size_px = hg_clamp_icon_size_px(size_px);

    WCHAR rel[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(rel, HG_ARRAYSIZE(rel), rel_logo)))
        return FALSE;
    normalize_slashes(rel);

    WCHAR base[HG_MAX_PATH];
    if (FAILED(StringCchPrintfW(base, HG_ARRAYSIZE(base), L"%ls\\%ls", package_path, rel))) {
        return FALSE;
    }

    if (file_exists_w(base)) {
        return SUCCEEDED(StringCchCopyW(out_file, out_cch, base));
    }

    WCHAR dir[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), base)))
        return FALSE;

    WCHAR *slash = wcsrchr(dir, L'\\');
    const WCHAR *filename = base;
    if (slash) {
        *slash = L'\0';
        filename = slash + 1;
    } else {
        if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), package_path)))
            return FALSE;
    }

    WCHAR stem[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(stem, HG_ARRAYSIZE(stem), filename)))
        return FALSE;

    WCHAR *dot = wcsrchr(stem, L'.');
    WCHAR ext[16] = L".png";
    if (dot) {
        if (FAILED(StringCchCopyW(ext, HG_ARRAYSIZE(ext), dot)))
            return FALSE;
        *dot = L'\0';
    }

    WCHAR candidate[HG_MAX_PATH];

    const WCHAR *dyn_patterns[] = {L"%ls\\%ls.targetsize-%d_altform-unplated%ls", L"%ls\\%ls.targetsize-%d%ls"};

    for (size_t i = 0; i < HG_ARRAYSIZE(dyn_patterns); i++) {
        if (SUCCEEDED(StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate), dyn_patterns[i], dir, stem, size_px, ext)) &&
            file_exists_w(candidate)) {
            return SUCCEEDED(StringCchCopyW(out_file, out_cch, candidate));
        }
    }

    const WCHAR *suffixes[] = {L".targetsize-16_altform-unplated",
                               L".targetsize-24_altform-unplated",
                               L".targetsize-32_altform-unplated",
                               L".targetsize-48_altform-unplated",
                               L".targetsize-256_altform-unplated",
                               L".targetsize-16",
                               L".targetsize-24",
                               L".targetsize-32",
                               L".targetsize-48",
                               L".targetsize-256",
                               L".scale-100",
                               L".scale-125",
                               L".scale-150",
                               L".scale-200",
                               L".scale-400"};

    for (size_t i = 0; i < HG_ARRAYSIZE(suffixes); i++) {
        if (SUCCEEDED(
                StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate), L"%ls\\%ls%ls%ls", dir, stem, suffixes[i], ext)) &&
            file_exists_w(candidate)) {
            return SUCCEEDED(StringCchCopyW(out_file, out_cch, candidate));
        }
    }

    WCHAR pattern[HG_MAX_PATH];
    if (FAILED(StringCchPrintfW(pattern, HG_ARRAYSIZE(pattern), L"%ls\\%ls*.png", dir, stem))) {
        return FALSE;
    }

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if (SUCCEEDED(StringCchPrintfW(out_file, out_cch, L"%ls\\%ls", dir, fd.cFileName))) {
                    FindClose(h);
                    return TRUE;
                }
            }
        } while (FindNextFileW(h, &fd));

        FindClose(h);
    }

    uwp_debug_log(L"UWP icon: Required asset file not found");
    return FALSE;
}

static HICON create_icon_from_bitmap(HBITMAP hbmp)
{
    if (!hbmp)
        return NULL;

    BITMAP bm;
    if (!GetObjectW(hbmp, sizeof(bm), &bm)) {
        return NULL;
    }

    if (bm.bmWidth <= 0 || bm.bmHeight <= 0) {
        return NULL;
    }

    if (bm.bmWidth > 4096 || bm.bmHeight > 4096) {
        return NULL;
    }

    size_t stride_bits = ((size_t)bm.bmWidth + 15u) & ~15u;
    size_t stride_bytes = stride_bits / 8u;
    size_t mask_size = stride_bytes * (size_t)bm.bmHeight;

    BYTE *mask_bits = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mask_size);
    if (!mask_bits)
        return NULL;

    HBITMAP hmask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, mask_bits);
    HeapFree(GetProcessHeap(), 0, mask_bits);

    if (!hmask)
        return NULL;

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = TRUE;
    ii.hbmColor = hbmp;
    ii.hbmMask = hmask;

    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(hmask);
    return icon;
}

static HRESULT hg_get_shell_image_bitmap(IShellItemImageFactory *factory, SIZE sz, HBITMAP *out_bmp)
{
    HRESULT hr;
    if (!out_bmp)
        return E_POINTER;
    *out_bmp = NULL;
    if (!factory)
        return E_POINTER;

    hr = factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT | SIIGBF_THUMBNAILONLY,
                                   out_bmp);
    if (SUCCEEDED(hr) && *out_bmp)
        return hr;

    hr = factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT, out_bmp);
    if (SUCCEEDED(hr) && *out_bmp)
        return hr;

    return factory->lpVtbl->GetImage(factory, sz, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, out_bmp);
}

static HICON load_icon_from_image_file(const WCHAR *file, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!file || !*file)
        return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    IShellItemImageFactory *factory = NULL;
    HRESULT hr = SHCreateItemFromParsingName(file, NULL, &IID_IShellItemImageFactory, (void **)&factory);

    if (FAILED(hr) || !factory) {
        uwp_debug_log(L"UWP icon: SHCreateItemFromParsingName failed");
        return NULL;
    }

    SIZE sz;
    sz.cx = size_px;
    sz.cy = size_px;

    HBITMAP hbmp = NULL;
    hr = hg_get_shell_image_bitmap(factory, sz, &hbmp);
    factory->lpVtbl->Release(factory);

    if (FAILED(hr) || !hbmp) {
        uwp_debug_log(L"UWP icon: IShellItemImageFactory::GetImage failed");
        return NULL;
    }

    HICON icon = create_icon_from_bitmap(hbmp);
    DeleteObject(hbmp);

    if (icon && own_icon) {
        *own_icon = TRUE;
    }

    return icon;
}

static HICON load_icon_from_aumid(const WCHAR *aumid, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!aumid || !*aumid)
        return NULL;

    WCHAR parsing_name[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(parsing_name, HG_ARRAYSIZE(parsing_name), L"shell:AppsFolder\\%ls", aumid);
    if (FAILED(hr))
        return NULL;

    IShellItem *psi = NULL;
    hr = SHCreateItemFromParsingName(parsing_name, NULL, &IID_IShellItem, (void **)&psi);
    if (FAILED(hr) || !psi)
        return NULL;

    IShellItemImageFactory *factory = NULL;
    hr = psi->lpVtbl->QueryInterface(psi, &IID_IShellItemImageFactory, (void **)&factory);
    psi->lpVtbl->Release(psi);
    if (FAILED(hr) || !factory)
        return NULL;

    SIZE sz;
    sz.cx = hg_clamp_icon_size_px(size_px);
    sz.cy = sz.cx;

    HBITMAP hbmp = NULL;
    hr = hg_get_shell_image_bitmap(factory, sz, &hbmp);
    factory->lpVtbl->Release(factory);

    if (FAILED(hr) || !hbmp)
        return NULL;

    HICON icon = create_icon_from_bitmap(hbmp);
    DeleteObject(hbmp);

    if (icon && own_icon) {
        *own_icon = TRUE;
    }

    return icon;
}

static HICON get_icon_from_package_pid(DWORD pid, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    size_px = hg_clamp_icon_size_px(size_px);

    WCHAR *aumid = get_app_user_model_id_alloc(pid);
    if (aumid) {
        HICON icon = load_icon_from_aumid(aumid, size_px, own_icon);
        HeapFree(GetProcessHeap(), 0, aumid);
        if (icon)
            return icon;
    }

    WCHAR *full_name = get_package_full_name_alloc(pid);
    if (!full_name)
        return NULL;

    WCHAR *package_path = get_package_path_alloc(full_name);
    HeapFree(GetProcessHeap(), 0, full_name);
    if (!package_path)
        return NULL;

    WCHAR rel_logo[HG_MAX_PATH] = {0};
    if (!get_logo_relpath_from_manifest(package_path, rel_logo, HG_ARRAYSIZE(rel_logo))) {
        HeapFree(GetProcessHeap(), 0, package_path);
        return NULL;
    }

    WCHAR logo_file[HG_MAX_PATH] = {0};
    if (!resolve_logo_asset_file(package_path, rel_logo, size_px, logo_file, HG_ARRAYSIZE(logo_file))) {
        HeapFree(GetProcessHeap(), 0, package_path);
        return NULL;
    }

    HeapFree(GetProcessHeap(), 0, package_path);
    return load_icon_from_image_file(logo_file, size_px, own_icon);
}

HICON get_window_icon(HWND hwnd, int size_px, BOOL *own_icon)
{
    if (own_icon)
        *own_icon = FALSE;
    if (!IsWindow(hwnd))
        return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    WCHAR proc_name[HG_MAX_STR] = {0};
    get_process_name_by_hwnd(hwnd, proc_name, HG_MAX_STR, NULL);

    HICON h_icon = NULL;

    if (_wcsicmp(proc_name, L"ApplicationFrameHost.exe") == 0) {
        WCHAR *frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HeapFree(GetProcessHeap(), 0, frame_aumid);
            if (h_icon)
                return h_icon;
        }

        HWND child_hwnd = NULL;
        DWORD child_pid = 0;

        if (get_real_uwp_child(hwnd, &child_hwnd, &child_pid)) {
            WCHAR *child_aumid = get_aumid_from_hwnd(child_hwnd);
            if (child_aumid) {
                h_icon = load_icon_from_aumid(child_aumid, size_px, own_icon);
                HeapFree(GetProcessHeap(), 0, child_aumid);
                if (h_icon)
                    return h_icon;
            }

            h_icon = get_icon_from_package_pid(child_pid, size_px, own_icon);
            if (h_icon)
                return h_icon;

            h_icon = get_icon_from_hwnd_msg(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon)
                        *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon)
                    *own_icon = FALSE;
                return h_icon;
            }

            h_icon = get_icon_from_hwnd_class(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon)
                        *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon)
                    *own_icon = FALSE;
                return h_icon;
            }

            h_icon = get_icon_from_process_exe(child_pid, own_icon);
            if (h_icon)
                return h_icon;
        }

        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon)
                    *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon)
                *own_icon = FALSE;
            return h_icon;
        }

    } else {
        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon)
                    *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon)
                *own_icon = FALSE;
            return h_icon;
        }

        WCHAR *frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HeapFree(GetProcessHeap(), 0, frame_aumid);
            if (h_icon)
                return h_icon;
        }

        h_icon = get_icon_from_package_pid(pid, size_px, own_icon);
        if (h_icon)
            return h_icon;
    }

    h_icon = get_icon_from_hwnd_class(hwnd);
    if (h_icon) {
        HICON h_copy = CopyIcon(h_icon);
        if (h_copy) {
            if (own_icon)
                *own_icon = TRUE;
            return h_copy;
        }
        if (own_icon)
            *own_icon = FALSE;
        return h_icon;
    }

    return get_icon_from_process_exe(pid, own_icon);
}

int compare_shortcuts(const void *a, const void *b)
{
    ShortcutItem *item_a = (ShortcutItem *)a;
    ShortcutItem *item_b = (ShortcutItem *)b;
    return lstrcmpiW(item_a->name, item_b->name);
}

void load_shortcuts()
{
    for (int i = 0; i < hg_g_shortcut_count; i++) {
        if (hg_g_shortcuts[i].icon) {
            DestroyIcon(hg_g_shortcuts[i].icon);
            hg_g_shortcuts[i].icon = NULL;
        }
    }
    ZeroMemory(hg_g_shortcuts, sizeof(hg_g_shortcuts));
    hg_g_shortcut_count = 0;

    if (!hg_g_shortcuts_path[0])
        return;

    WCHAR search_path[HG_MAX_PATH] = {0};
    if (FAILED(StringCchPrintfW(search_path, HG_ARRAYSIZE(search_path), L"%ls\\*", hg_g_shortcuts_path))) {
        return;
    }

    WCHAR norm_search[HG_MAX_PATH];
    normalize_path_for_api(search_path, norm_search, HG_MAX_PATH);

    WIN32_FIND_DATAW ffd = {0};
    HANDLE find_handle = FindFirstFileW(norm_search, &ffd);
    if (find_handle == INVALID_HANDLE_VALUE)
        return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        if (hg_g_shortcut_count >= HG_MAX_SHORTCUTS)
            break;

        int raw_len = lstrlenW(ffd.cFileName);
        if (raw_len <= 4)
            continue;

        size_t len = (size_t)raw_len;
        BOOL is_lnk = (lstrcmpiW(ffd.cFileName + len - 4, L".lnk") == 0);
        BOOL is_url = (lstrcmpiW(ffd.cFileName + len - 4, L".url") == 0);
        if (!is_lnk && !is_url)
            continue;

        ShortcutItem *item = &hg_g_shortcuts[hg_g_shortcut_count];
        ZeroMemory(item, sizeof(*item));

        if (FAILED(StringCchPrintfW(item->path, HG_ARRAYSIZE(item->path), L"%ls\\%ls", hg_g_shortcuts_path,
                                    ffd.cFileName))) {
            continue;
        }

        if (FAILED(StringCchCopyW(item->name, HG_ARRAYSIZE(item->name), ffd.cFileName))) {
            continue;
        }
        WCHAR *dot = wcsrchr(item->name, L'.');
        if (dot)
            *dot = L'\0';

        int icon_size = ABS(hg_g_current_font_size);
        if (icon_size < SC(32))
            icon_size = SC(32);

        if (is_lnk) {
            IShellLinkW *psl = NULL;
            HRESULT hr =
                CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void **)&psl);
            if (SUCCEEDED(hr) && psl) {
                IPersistFile *ppf = NULL;
                if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void **)&ppf)) && ppf) {
                    if (SUCCEEDED(ppf->lpVtbl->Load(ppf, item->path, STGM_READ))) {
                        WCHAR target_path[HG_MAX_PATH] = {0};
                        if (SUCCEEDED(psl->lpVtbl->GetPath(psl, target_path, HG_ARRAYSIZE(target_path), NULL, 0)) &&
                            target_path[0] != L'\0') {
                            HICON ext_icon = NULL;
                            UINT icon_id = 0;
                            if (PrivateExtractIconsW(target_path, 0, icon_size, icon_size, &ext_icon, &icon_id, 1,
                                                     LR_LOADFROMFILE) > 0 &&
                                ext_icon) {
                                item->icon = ext_icon;
                            }
                        }
                    }
                    ppf->lpVtbl->Release(ppf);
                }
                psl->lpVtbl->Release(psl);
            }
        }

        if (!item->icon) {
            SHFILEINFOW sfi = {0};
            if (SHGetFileInfoW(item->path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
                item->icon = sfi.hIcon;
            }
        }

        hg_g_shortcut_count++;
    } while (FindNextFileW(find_handle, &ffd));

    FindClose(find_handle);

    if (hg_g_shortcut_count > 1) {
        qsort(hg_g_shortcuts, (size_t)hg_g_shortcut_count, sizeof(ShortcutItem), compare_shortcuts);
    }
}

void append_message(const WCHAR *msg)
{
    if (!hg_g_edit_msg_wnd || !msg)
        return;

    int line_count = (int)SendMessageW(hg_g_edit_msg_wnd, EM_GETLINECOUNT, 0, 0);
    if (line_count > HG_TASKBOX_EDIT_CONTROL_TRIM_THRESHOLD) {
        int remove_count = line_count - HG_TASKBOX_EDIT_CONTROL_RETAIN_COUNT;
        int char_index = (int)SendMessageW(hg_g_edit_msg_wnd, EM_LINEINDEX, (WPARAM)remove_count, 0);
        if (char_index != -1) {
            SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, 0, (LPARAM)char_index);
            SendMessageW(hg_g_edit_msg_wnd, EM_REPLACESEL, FALSE, (LPARAM)L"");
        }
    }

    int len = GetWindowTextLengthW(hg_g_edit_msg_wnd);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    if (len > 0)
        SendMessageW(hg_g_edit_msg_wnd, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    int new_line_start = GetWindowTextLengthW(hg_g_edit_msg_wnd);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, (WPARAM)new_line_start, (LPARAM)new_line_start);
    SendMessageW(hg_g_edit_msg_wnd, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, (WPARAM)new_line_start, (LPARAM)new_line_start);
    SendMessageW(hg_g_edit_msg_wnd, EM_SCROLLCARET, 0, 0);
}

void draw_outlined_text(HDC hdc, const WCHAR *text, int len, RECT *rc, UINT format, COLORREF text_color,
                        COLORREF outline_color)
{
    if (!hdc || !text || !rc || len <= 0)
        return;
    int old_bk_mode = SetBkMode(hdc, TRANSPARENT);
    COLORREF old_text_color = GetTextColor(hdc);

    SetTextColor(hdc, outline_color);
    RECT temp_rc;
    int offset = SC(1);
    if (offset < 1)
        offset = 1;

    int dx[] = {-offset, 0, offset, -offset, offset, -offset, 0, offset};
    int dy[] = {-offset, -offset, -offset, 0, 0, offset, offset, offset};

    for (int i = 0; i < 8; i++) {
        temp_rc = *rc;
        OffsetRect(&temp_rc, dx[i], dy[i]);
        DrawTextW(hdc, text, len, &temp_rc, format);
    }

    SetTextColor(hdc, text_color);
    DrawTextW(hdc, text, len, rc, format);

    SetTextColor(hdc, old_text_color);
    SetBkMode(hdc, old_bk_mode);
}

int get_items_per_row(int width, int icon_size)
{
    if (width <= 0)
        return 1;
    int denom = icon_size + SC(15);
    if (denom <= 0)
        return 1;
    int n = (width - icon_size - SC(20)) / denom + 1;
    return (n > 0) ? n : 1;
}

typedef enum HgToolbarBuiltinValueKind {
    HG_TOOLBAR_VALUE_NONE = 0,
    HG_TOOLBAR_VALUE_ALPHA,
    HG_TOOLBAR_VALUE_BRIGHTNESS,
    HG_TOOLBAR_VALUE_VOLUME
} HgToolbarBuiltinValueKind;

typedef struct HgToolbarBuiltinDescriptor {
    int index;
    WCHAR label;
    const WCHAR *focus_text;
    const WCHAR *tooltip_text;
    HgToolbarBuiltinValueKind value_kind;
    HgToolbarClickRole click_role;
    HgToolbarDragRole drag_role;
} HgToolbarBuiltinDescriptor;

static const HgToolbarBuiltinDescriptor hg_toolbar_builtin_descriptors[] = {
    {HG_TOOL_ICON_RESIZE, L'R', L"Drag to Resize Window", L"Drag to Resize Window", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_RESIZE_TASKBOX},
    {HG_TOOL_ICON_MOVE, L'M', L"Drag to Move Window", L"Drag to Move Window", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_MOVE_TASKBOX},
    {HG_TOOL_ICON_CLOSE, L'X', L"Hide Dashboard", L"Hide Dashboard (Reload Shortcuts)", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_HIDE_TASKBOX, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_DESKTOP, L'D', L"Show Desktop", L"Show Desktop", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_TOGGLE_DESKTOP, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_MENU, L'P', L"Menu", L"Menu", HG_TOOLBAR_VALUE_NONE, HG_TOOLBAR_CLICK_OPEN_MENU,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_COMMAND, L'C', L"Command Box", L"Command Box", HG_TOOLBAR_VALUE_NONE,
     HG_TOOLBAR_CLICK_SHOW_COMMANDBOX, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_ALPHA, L'A', NULL, NULL, HG_TOOLBAR_VALUE_ALPHA, HG_TOOLBAR_CLICK_NONE, HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_BRIGHTNESS, L'B', NULL, NULL, HG_TOOLBAR_VALUE_BRIGHTNESS, HG_TOOLBAR_CLICK_NONE,
     HG_TOOLBAR_DRAG_NONE},
    {HG_TOOL_ICON_VOLUME, L'V', NULL, NULL, HG_TOOLBAR_VALUE_VOLUME, HG_TOOLBAR_CLICK_TOGGLE_MUTE,
     HG_TOOLBAR_DRAG_NONE},
};

enum {
    HG_TOOLBAR_BUILTIN_DESCRIPTOR_COUNT_CHECK =
        1 / ((HG_ARRAYSIZE(hg_toolbar_builtin_descriptors) == HG_NUM_BASIC_ICONS) ? 1 : 0)
};

static const HgToolbarBuiltinDescriptor *hg_toolbar_builtin_descriptor(int index)
{
    for (size_t i = 0; i < HG_ARRAYSIZE(hg_toolbar_builtin_descriptors); ++i) {
        if (hg_toolbar_builtin_descriptors[i].index == index)
            return &hg_toolbar_builtin_descriptors[i];
    }
    return NULL;
}

WCHAR hg_toolbar_builtin_label(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->label : L'?';
}

const WCHAR *hg_toolbar_builtin_focus_text(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->focus_text : NULL;
}

const WCHAR *hg_toolbar_builtin_tooltip_text(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->tooltip_text : NULL;
}

BOOL hg_toolbar_builtin_has_value(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc && desc->value_kind != HG_TOOLBAR_VALUE_NONE;
}

BOOL hg_toolbar_builtin_value_text(int index, HgToolbarTextMode mode, WCHAR *buffer, size_t buffer_cch)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    if (!desc || desc->value_kind == HG_TOOLBAR_VALUE_NONE || !buffer || buffer_cch == 0)
        return FALSE;

    switch (desc->value_kind) {
    case HG_TOOLBAR_VALUE_ALPHA: {
        int pct = (hg_g_taskbox_alpha * 100 + 127) / 255;
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"Alpha: %d%%", pct));
    }
    case HG_TOOLBAR_VALUE_BRIGHTNESS:
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"Brightness: %d%%", get_system_brightness()));
    case HG_TOOLBAR_VALUE_VOLUME: {
        const WCHAR *label = (mode == HG_TOOLBAR_TEXT_TOOLTIP) ? L"Vol" : L"System Volume";
        if (get_system_mute()) {
            return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"%ls: %d%% (Muted)", label, get_system_volume()));
        }
        return SUCCEEDED(hellgates_wsprintf(buffer, buffer_cch, L"%ls: %d%%", label, get_system_volume()));
    }
    case HG_TOOLBAR_VALUE_NONE:
    default:
        return FALSE;
    }
}

HgToolbarClickRole hg_toolbar_builtin_click_role(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->click_role : HG_TOOLBAR_CLICK_NONE;
}

HgToolbarDragRole hg_toolbar_builtin_drag_role(int index)
{
    const HgToolbarBuiltinDescriptor *desc = hg_toolbar_builtin_descriptor(index);
    return desc ? desc->drag_role : HG_TOOLBAR_DRAG_NONE;
}

void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT *out_rect)
{
    if (width <= 0 || !out_rect) {
        if (out_rect)
            SetRectEmpty(out_rect);
        return;
    }

    int cols = get_items_per_row(width, icon_size);
    if (cols <= 0)
        cols = 1;
    int row_height = icon_size + SC(10);

    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

    int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (min_required_rows <= 0)
        min_required_rows = 1;

    int visible_rows = (height - SC(20) + SC(10)) / row_height;
    if (visible_rows <= 0)
        visible_rows = 1;

    int rows = (visible_rows > min_required_rows) ? visible_rows : min_required_rows;

    int total_cells = rows * cols;

    int cell_index;
    if (item_type == 0) {
        cell_index = item_index;
    } else {
        cell_index = total_cells - 1 - item_index;
    }

    int row = cell_index / cols;
    int col = cell_index % cols;

    out_rect->left = SC(10) + col * (icon_size + SC(15));
    out_rect->top = SC(10) + row * row_height;
    out_rect->right = out_rect->left + icon_size;
    out_rect->bottom = out_rect->top + icon_size;
}

void update_toolbar_tooltips(HWND hwnd)
{
    if (!hg_g_tooltip_wnd || !hwnd)
        return;
    static int last_total_count = 0;

    for (int i = 0; i < last_total_count; i++) {
        TOOLINFOW ti = {0};
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.hwnd = hwnd;
        ti.uId = (UINT_PTR)i;
        SendMessageW(hg_g_tooltip_wnd, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }
    last_total_count = 0;

    RECT client_rc;
    GetClientRect(hwnd, &client_rc);
    if (client_rc.right <= 0 || client_rc.bottom <= 0)
        return;

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16))
        icon_size = SC(16);

    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    int id_counter = 0;

    for (int i = 0; i < total_tasks; i++) {
        RECT item_rc;
        get_toolbar_item_rect(0, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        ti_tool.lpszText = hg_g_window_items[i].title;
        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    for (int i = 0; i < total_shortcuts; i++) {
        RECT item_rc;
        get_toolbar_item_rect(1, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = {0};
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        const WCHAR *tooltip_text = hg_toolbar_builtin_tooltip_text(i);
        if (tooltip_text) {
            ti_tool.lpszText = (LPWSTR)tooltip_text;
        } else if (hg_toolbar_builtin_has_value(i)) {
            static WCHAR value_tips[HG_NUM_BASIC_ICONS][64];
            if (hg_toolbar_builtin_value_text(i, HG_TOOLBAR_TEXT_TOOLTIP, value_tips[i],
                                              HG_ARRAYSIZE(value_tips[i]))) {
                ti_tool.lpszText = value_tips[i];
            }
        } else {
            ti_tool.lpszText = hg_g_shortcuts[i - HG_NUM_BASIC_ICONS].name;
        }

        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    SendMessageW(hg_g_tooltip_wnd, TTM_ACTIVATE, TRUE, 0);
    last_total_count = id_counter;
}

BOOL CALLBACK minimize_restore_enum_proc(HWND hwnd, LPARAM l_param)
{
    BOOL is_minimize = (BOOL)l_param;
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd)
        return TRUE;
    if (!IsWindowVisible(hwnd))
        return TRUE;

    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (owner == NULL || (ex_style & WS_EX_APPWINDOW)) {
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
            if (cloaked != 0)
                return TRUE;
        }

        if (is_minimize) {
            if (!IsIconic(hwnd))
                ShowWindowAsync(hwnd, SW_MINIMIZE);
        } else {
            if (IsIconic(hwnd))
                ShowWindowAsync(hwnd, SW_RESTORE);
        }
    }
    return TRUE;
}

void move_window_by_offset(HWND hwnd, int dx, int dy)
{
    if (!IsWindow(hwnd))
        return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc))
        return;
    SetWindowPos(hwnd, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after movement */
    if (hwnd == hg_g_floater_wnd)
        save_floater_geometry_config(rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
    else if (hwnd == hg_g_taskbox_wnd)
        save_taskbox_geometry_config(rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);

}

void resize_window_by_offset(HWND hwnd, int dw, int dh)
{
    if (!IsWindow(hwnd))
        return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc))
        return;



    int new_w = (rc.right - rc.left) + dw;
    int new_h = (rc.bottom - rc.top) + dh;
    if (new_w < SC(HG_MIN_WINDOW_WIDTH))
        new_w = SC(HG_MIN_WINDOW_WIDTH);
    if (new_h < SC(HG_MIN_WINDOW_HEIGHT))
        new_h = SC(HG_MIN_WINDOW_HEIGHT);
    SetWindowPos(hwnd, NULL, 0, 0, new_w, new_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after resizing */
    if (hwnd == hg_g_floater_wnd)
        save_floater_geometry_config(rc.left, rc.top, new_w, new_h);
    else if (hwnd == hg_g_taskbox_wnd)
        save_taskbox_geometry_config(rc.left, rc.top, new_w, new_h);
}

void disable_window_ime(HWND hwnd)
{
    if (!hwnd) {
        return;
    }

    ImmAssociateContextEx(hwnd, NULL, IACE_DEFAULT);

    HIMC himc = ImmGetContext(hwnd);
    if (!himc) {
        return;
    }

    ImmSetOpenStatus(himc, FALSE);
    ImmReleaseContext(hwnd, himc);
}

BOOL readonly_edit_handle_ime_messages(HWND hwnd, UINT msg, WPARAM w_param)
{
    switch (msg) {
    case WM_IME_SETCONTEXT:
        if (w_param) {
            disable_window_ime(hwnd);
            return TRUE;
        }
        break;
    case WM_INPUTLANGCHANGE:
    case WM_INPUTLANGCHANGEREQUEST:
        disable_window_ime(hwnd);
        return TRUE;
    }

    return FALSE;
}
