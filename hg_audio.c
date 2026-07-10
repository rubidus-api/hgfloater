/* Audio endpoint access: default device volume/mute, device list, and
 * default-device selection via the policy config interface. */
#include "hg_utils.h"

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

static BOOL hg_acquire_default_audio_endpoint_volume(IAudioEndpointVolume **out_endpoint)
{
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *endpoint = NULL;

    if (!out_endpoint)
        return FALSE;
    *out_endpoint = NULL;

    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                (void **)&enumerator)) ||
        !enumerator) {
        return FALSE;
    }

    if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device)) && device) {
        if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
                                               (void **)&endpoint)) &&
            endpoint) {
            *out_endpoint = endpoint;
        }
        HG_RELEASE_COM(device);
    }

    HG_RELEASE_COM(enumerator);
    return *out_endpoint != NULL;
}

/* Paint and tooltip paths read volume/mute every refresh; one cached endpoint
 * keeps them free of CoCreateInstance/Activate round trips. */
static IAudioEndpointVolume *s_cached_audio_endpoint = NULL;

void hg_reset_audio_endpoint_cache(void)
{
    HG_RELEASE_COM(s_cached_audio_endpoint);
}

static IAudioEndpointVolume *hg_cached_audio_endpoint(void)
{
    if (!s_cached_audio_endpoint) {
        hg_acquire_default_audio_endpoint_volume(&s_cached_audio_endpoint);
    }
    return s_cached_audio_endpoint;
}

/* On failure the endpoint is likely stale (device removed or default changed):
 * drop the cache and hand the caller one fresh endpoint to retry with. */
static IAudioEndpointVolume *hg_retry_audio_endpoint(void)
{
    hg_reset_audio_endpoint_cache();
    return hg_cached_audio_endpoint();
}

int get_system_volume()
{
    float volume = 0.0f;
    IAudioEndpointVolume *endpoint = hg_cached_audio_endpoint();

    if (endpoint && FAILED(endpoint->lpVtbl->GetMasterVolumeLevelScalar(endpoint, &volume))) {
        endpoint = hg_retry_audio_endpoint();
        if (endpoint) {
            endpoint->lpVtbl->GetMasterVolumeLevelScalar(endpoint, &volume);
        }
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

    IAudioEndpointVolume *endpoint = hg_cached_audio_endpoint();

    if (endpoint && FAILED(endpoint->lpVtbl->SetMasterVolumeLevelScalar(endpoint, volume, NULL))) {
        endpoint = hg_retry_audio_endpoint();
        if (endpoint) {
            endpoint->lpVtbl->SetMasterVolumeLevelScalar(endpoint, volume, NULL);
        }
    }
    if (endpoint && percent > 0) {
        endpoint->lpVtbl->SetMute(endpoint, FALSE, NULL);
    }
}

int get_system_mute(void)
{
    BOOL mute = FALSE;
    IAudioEndpointVolume *endpoint = hg_cached_audio_endpoint();

    if (endpoint && FAILED(endpoint->lpVtbl->GetMute(endpoint, &mute))) {
        endpoint = hg_retry_audio_endpoint();
        if (endpoint) {
            endpoint->lpVtbl->GetMute(endpoint, &mute);
        }
    }
    return (int)mute;
}

void set_system_mute(int mute)
{
    IAudioEndpointVolume *endpoint = hg_cached_audio_endpoint();

    if (endpoint && FAILED(endpoint->lpVtbl->SetMute(endpoint, (BOOL)mute, NULL))) {
        endpoint = hg_retry_audio_endpoint();
        if (endpoint) {
            endpoint->lpVtbl->SetMute(endpoint, (BOOL)mute, NULL);
        }
    }
}

void update_audio_device_list()
{
    /* Menu-open time is the natural refresh point for a changed default device. */
    hg_reset_audio_endpoint_cache();
    hg_g_audio_device_count = 0;
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDeviceCollection *collection = NULL;
    IMMDevice *default_dev = NULL;
    LPWSTR default_id = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                                   (void **)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &default_dev))) {
            default_dev->lpVtbl->GetId(default_dev, &default_id);
            HG_RELEASE_COM(default_dev);
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
                            HG_RELEASE_COM(props);
                        }
                        HG_COTASKMEM_FREE(id);
                    }
                    HG_RELEASE_COM(device);
                }
            }
            HG_RELEASE_COM(collection);
        }
        HG_COTASKMEM_FREE(default_id);
        HG_RELEASE_COM(enumerator);
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

    HG_RELEASE_COM(policy);
    return ok;
}

