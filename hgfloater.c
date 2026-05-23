#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0603
#endif

#include <windows.h>
#include <appmodel.h>
#include <shobjidl.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <propsys.h>
#include <propkey.h>
#include <shellapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <stdarg.h>
#include <strsafe.h>
#include <pathcch.h>
#include <shellscalingapi.h>
#include <exdisp.h>
#include <shlwapi.h>
#include <imm.h>
#include <wctype.h>

/* 라이브러리 명시적 링크 - MSVC 전용 */
#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "pathcch.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

/* 비주얼 스타일(Common Controls 6.0) 사용을 위한 매니페스트 설정 */
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

/* 툴팁 구조체 호환 크기 정의 */
#if defined(_WIN64)
#define TOOLINFO_V1_SIZE 64
#else
#define TOOLINFO_V1_SIZE 44
#endif

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

/* Build-time versioning */
#ifndef HG_VERSION_W
#define HG_VERSION_W L"v26.05.18b"
#endif

/* Include generated About text from README.md if available */
#if __has_include("hg_about_text.h")
#include "hg_about_text.h"
#endif

#ifndef HG_ABOUT_README_W
#define HG_ABOUT_README_W L"(Additional information from README.md could not be loaded.)"
#endif

#define HG_ABOUT_FIXED_W \
    L"hgfloater " HG_VERSION_W L"\r\n" \
    L"A lightweight WinAPI-based floating widget & task switcher.\r\n\r\n" \
    L"Homepage:\r\n" \
    L"https://github.com/rubidus-api/hgfloater\r\n\r\n" \
    L"Developer: rubidus-api (rubidus@gmail.com)\r\n\r\n" \
    L"License: MIT License\r\n\r\n" \
    L"--- How to use Link ---\r\n" \
    L"You can select and copy the URL/Email above using context menu or Ctrl+C.\r\n" \
    L"------------------------------\r\n\r\n"

#define HG_ABOUT_TEXT_W HG_ABOUT_FIXED_W HG_ABOUT_README_W

/*
 * hgfloater (HellGates Series)
 * - Drag & Drop Reordering
 * - Real-time Clock Titlebar
 * - Fixed List Order (Incremental Update)
 *
 * [Resource Audit]
 * - Stack: Large arrays (windowItems, hg_g_shortcuts) moved to global scope to prevent stack overflow.
 * - Memory: Shortcut icons are destroyed before reload; GDI objects in WM_PAINT are properly released.
 * - Performance: Double buffering prevents flickering; incremental window list updates reduce CPU load.
 */


#ifndef HG_ARRAYSIZE
#define HG_ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef HG_DEBUG_UWP_ICON
#define uwp_debug_log(x) append_message(x)
#else
#define uwp_debug_log(x) ((void)0)
#endif

/* =========================================================================
 * 매크로 상수 (Macro Constants)
 * ========================================================================= */
#define HG_IDC_LISTBOX 101
#define HG_IDC_TOOLBAR 102
#define HG_IDC_EDIT_MSG 103
#define HG_IDC_CONTROLBOX_VALUE 104
#define HG_IDC_CONTROLBOX_SLIDER 105
#define HG_WINDOW_WIDTH 480
#define HG_WINDOW_HEIGHT 160
#define HG_MIN_WINDOW_WIDTH 64
#define HG_MIN_WINDOW_HEIGHT 32
#define HG_CONTROLBOX_MIN_WIDTH 150
#define HG_CONTROLBOX_MAX_WIDTH 1000
#define HG_CONTROLBOX_TRACKBAR_HEIGHT 28
#define HG_CONTROLBOX_LABEL_GAP 6
#define HG_CONTROLBOX_DEFAULT_WIDTH 260

#define HG_MAX_PATH 32768
#define HG_MAX_STR  1024

#define HG_MAX_TITLE_LEN HG_MAX_STR
#define HG_MAX_WINDOW_ITEMS 1024
#define HG_MAX_SHORTCUTS 64
#define HG_MAX_AUDIO_DEVICES 16
#define HG_NUM_BASIC_ICONS 5

#define HG_IDM_MINIMIZE 201
#define HG_IDM_CLOSE    202
#define HG_IDM_MOVE     203
#define HG_IDM_SIZE     204
#define HG_IDM_CLOSE_APP 205
#define HG_IDM_CLEAR_EDIT 206
#define HG_IDM_EDIT_COPYALL 207
#define HG_IDM_ABOUT        208
#define HG_IDM_RESET_ALL    209
#define HG_IDM_FONT_UP      210
#define HG_IDM_FONT_DOWN    211
#define HG_IDM_POWER_OFF    212
#define HG_IDM_OPEN_CONTROLBOX 213

#define HG_IDM_TASK_MOVETO_0_0    300
#define HG_IDM_TASK_CLOSE         301
#define HG_IDM_TASK_RESIZE_4_3_1  302
#define HG_IDM_TASK_RESIZE_4_3_2  303
#define HG_IDM_TASK_RESIZE_4_3_3  304
#define HG_IDM_TASK_RESIZE_16_9_1 305
#define HG_IDM_TASK_RESIZE_16_9_2 306
#define HG_IDM_TASK_RESIZE_16_9_3 307
#define HG_IDM_TASK_RESIZE_16_9_4 308
#define HG_IDM_TASK_RESIZE_9_16_1 309
#define HG_IDM_TASK_RESIZE_9_16_2 310
#define HG_IDM_TASK_RESIZE_9_16_3 311
#define HG_IDM_TASK_RESIZE_9_16_4 312
#define HG_IDM_TASK_RESTORE       313

#define HG_IDM_VOLUME_PERCENT       400
#define HG_IDM_VOLUME_SET_0         401
#define HG_IDM_VOLUME_SET_25        402
#define HG_IDM_VOLUME_SET_50        403
#define HG_IDM_VOLUME_SET_75        404
#define HG_IDM_VOLUME_SET_100       405
#define HG_IDM_AUDIO_DEVICE_BASE    5000
#define HG_IDM_MONITOR_BASE       6000

#define HG_MAX_MONITORS 10

#define HG_IDM_SHORTCUT_RUN       (UINT_MAX - 100)
#define HG_IDM_SHORTCUT_OPEN_DIR  (UINT_MAX - 101)

#define HG_TASKBOX_EDIT_CONTROL_TRIM_THRESHOLD 64
#define HG_TASKBOX_EDIT_CONTROL_RETAIN_COUNT   32

#define HG_TIMER_HIGHLIGHT 1001
#define HG_HIGHLIGHT_TICKS 6 /* 3번 깜빡임 (On/Off) */

#define HG_TRANSPARENT_KEY RGB(1, 2, 3)
#define HG_BORDER_THICKNESS   5
#define HG_FLOATER_MIN_FONT_SIZE 12
#define HG_FLOATER_MAX_FONT_SIZE 96

/* 색 조합 (Color Scheme) 매직넘버 설정 */
#define HG_THEME_CUSTOM_BG        RGB(20, 20, 20)
#define HG_THEME_CUSTOM_BORDER    RGB(160, 160, 160)
#define HG_THEME_CUSTOM_TEXT      RGB(255, 255, 255)
#define HG_THEME_CUSTOM_FLASH     RGB(180, 130, 240)
#define HG_THEME_CUSTOM_SELECTED  RGB(80, 80, 80)

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#define DWMWCP_DONOTROUND 2

#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#define WM_DWMCOLORIZATIONCOLORCHANGED 0x0320
#endif

#define HG_COLOR_BG_DEFAULT    hg_g_color_scheme_selected.bg
#define HG_COLOR_BG_TOOLBAR    hg_g_color_scheme_selected.border
#define HG_COLOR_BG_FLASH      hg_g_color_scheme_selected.flash
#define HG_COLOR_BG_SELECTED   hg_g_color_scheme_selected.selected
#define HG_COLOR_BORDER_SELECTED hg_g_color_scheme_selected.flash
#define HG_COLOR_TEXT_DEFAULT  hg_g_color_scheme_selected.text
#define HG_CLICKABLE_BG        hg_g_color_scheme_selected.bg

#define HG_MIN_ALPHA ((int)(255 * (100 - 70) / 100)) /* 70% transparency limit */
#define HG_MAX_ALPHA 255

#ifndef ICON_SMALL2
#define ICON_SMALL2 2
#endif

#define HG_UWP_MAX_MANIFEST_BYTES (1024u * 1024u)
#define HG_UWP_MIN_ICON_PX        16
#define HG_UWP_MAX_ICON_PX        256

/* =========================================================================
 * 데이터 구조체 (Data Structures)
 * ========================================================================= */
typedef struct
{
    COLORREF bg, border, text, flash, selected;
} color_scheme_t;

typedef struct {
    WCHAR path[HG_MAX_PATH];
    WCHAR name[HG_MAX_PATH];
    HICON icon;
} ShortcutItem;

typedef struct {
    HWND hwnd;
    HICON icon;
    BOOL own_icon;
    WCHAR title[HG_MAX_STR];
    WCHAR process_name[HG_MAX_STR];
    DWORD process_id;
    BOOL exists;
    int image_index;
} WindowItem;

typedef struct {
    DWORD frame_pid;
    HWND best_hwnd;
    DWORD best_pid;
} FindUWPChildData;

typedef struct {
    WCHAR name[HG_MAX_STR];
    WCHAR id[HG_MAX_STR];
    BOOL is_default;
} AudioDevice;

typedef struct {
    HMONITOR hMonitor;
    RECT rcMonitor;
    WCHAR name[64];
    HWND hwnd;
    BOOL active;
} MonitorInfo;

MonitorInfo hg_g_monitors[HG_MAX_MONITORS];
int hg_g_monitor_count = 0;

/* =========================================================================
 * 전역 변수 (Global Variables)
 * ========================================================================= */
color_scheme_t hg_g_color_scheme_dark, hg_g_color_scheme_light, hg_g_color_scheme_selected;
int hg_g_is_dark_mode = 1;
static BOOL hg_g_is_high_contrast = FALSE;
static COLORREF hg_g_system_accent_color = 0;
static BOOL hg_g_has_system_accent_color = FALSE;

double hg_g_scale_factor = 1.0;

HWND hg_g_toolbar_wnd;
HWND hg_g_edit_msg_wnd;
HWND hg_g_tooltip_wnd;
HWND hg_g_taskbox_wnd;
HWND hg_g_controlbox_wnd = NULL;
HWND hg_g_controlbox_value_wnd = NULL;
HWND hg_g_controlbox_slider_wnd = NULL;
HMENU hg_g_h_audio_submenu = NULL;

UINT hg_g_shellhook_msg = 0;
HWND hg_g_floater_wnd;
HWND hg_g_about_wnd = NULL;
HFONT hg_g_main_font;
HFONT hg_g_floater_time_font = NULL;
HFONT hg_g_floater_date_font = NULL;
HFONT hg_g_toolbar_btn_font = NULL;
HBRUSH hg_g_main_bg_brush = NULL;
HBRUSH hg_g_edit_bg_brush = NULL;
HBRUSH hg_g_hbr_highlight = NULL;
int hg_g_floater_font_size = 28;
int hg_g_current_font_size = -22; /* 초기 아이콘 크기 (기존 32의 약 2/3) */
int hg_g_edit_font_size = -16; /* Edit text font size */
BYTE hg_g_floater_alpha = 204;
BYTE hg_g_taskbox_alpha = 204;
int hg_g_floater_highlight_ticks = 0;
int hg_g_taskbox_highlight_ticks = 0;
int hg_g_focus_area = 0; /* Only Toolbar (Unified grid) */
int hg_g_toolbar_focus_index = 0;
UINT hg_g_hotkey_modifiers = MOD_WIN | MOD_ALT;
UINT hg_g_hotkey_key = VK_SPACE;
BOOL hg_g_hotkey_registered = FALSE;

WCHAR hg_g_base_path[HG_MAX_PATH];
WCHAR hg_g_shortcuts_path[HG_MAX_PATH];
WCHAR hg_g_config_path[HG_MAX_PATH];
WCHAR hg_g_font_name[64] = L"Segoe UI";

ShortcutItem hg_g_shortcuts[HG_MAX_SHORTCUTS];
int hg_g_shortcut_count = 0;

AudioDevice hg_g_audio_devices[HG_MAX_AUDIO_DEVICES];
int hg_g_audio_device_count = 0;

WindowItem hg_g_window_items[HG_MAX_WINDOW_ITEMS];
WindowItem hg_g_new_items[HG_MAX_WINDOW_ITEMS]; /* refresh_window_list용 임시 배열 */
int hg_g_window_count = 0;

BOOL hg_g_in_sizemove = FALSE;
RECT hg_g_drag_start_rect = {0};

/* 드래그 앤 드롭 관련 상태 */
BOOL hg_g_is_dragging = FALSE;
int hg_g_drag_source_index = -1;
POINT hg_g_drag_start_pt = {0, 0};
POINT hg_g_drag_current_pt = {0, 0};
int hg_g_drag_target_index = -1;

/* =========================================================================
 * 함수 포워드 선언 & 유틸리티 (Function Prototypes & Util Macros)
 * ========================================================================= */
static inline int sc(int x) {
    return (int)(x * hg_g_scale_factor + (x >= 0 ? 0.5 : -0.5));
}
#define SC(x) sc(x)

static inline int hellgates_abs(int x) {
    return (x < 0) ? -x : x;
}
#define ABS(x) hellgates_abs(x)

void refresh_window_list(BOOL force);
void update_layout(HWND hwnd);
void update_floater_layout(HWND hwnd);
void update_focus_message(int override_type, int override_index);
LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass, DWORD_PTR dw_ref_data);
LRESULT CALLBACK about_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK controlbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK monitor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void save_config(const WCHAR* section, int x, int y, int w, int h);
void save_floater_font_config(void);
void save_taskbox_font_config(void);
static int get_controlbox_required_height(void);

static const WCHAR HG_CLASS_FLOATER_WIDGET[] = L"hgfloater_widget_class";
static const WCHAR HG_CLASS_CONTROLBOX[] = L"hgcontrolbox_class";
static const WCHAR HG_CLASS_ABOUT[] = L"hgabout_class";
static const WCHAR HG_CLASS_TASKBOX[] = L"hgfloater_class";
static const WCHAR HG_CLASS_MONITOR[] = L"hgmonitor_class";

typedef struct WindowClassSpec {
    const WCHAR* class_name;
    WNDPROC wnd_proc;
    HBRUSH background;
    const WCHAR* fail_message;
} WindowClassSpec;

static BOOL register_app_window_classes(HINSTANCE instance, HICON icon_large, HICON icon_small);
static void unregister_app_window_classes(HINSTANCE instance);

typedef struct ToolbarControllerState {
    int hovered_type;
    int hovered_index;
    int pressed_type;
    int pressed_index;
    int cached_icon_size;
    BOOL is_resizing;
    BOOL is_moving_taskbox;
    POINT start_mouse;
    RECT start_rect;
} ToolbarControllerState;

/* =========================================================================
 * 오디오 제어 기능 (Audio Control Implementation)
 * ========================================================================= */

/* Manual GUID definitions for Core Audio to avoid including initguid.h which clashes with uuid.lib */
#ifndef CLSID_MMDeviceEnumerator
const GUID CLSID_MMDeviceEnumerator = { 0xbcde0395, 0xe52f, 0x467c, { 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e } };
#endif
#ifndef IID_IMMDeviceEnumerator
const GUID IID_IMMDeviceEnumerator = { 0xa95664d2, 0x9614, 0x4f35, { 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6 } };
#endif
#ifndef IID_IAudioEndpointVolume
const GUID IID_IAudioEndpointVolume = { 0x5cdf2c82, 0x841e, 0x4546, { 0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a } };
#endif

/* Undocumented IPolicyConfig interface for setting default audio endpoint */
static const GUID CLSID_CPolicyConfigClient =
    { 0x870af99c, 0x171d, 0x4f9e,
      { 0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9 } };

static const GUID IID_IPolicyConfig =
    { 0xf8679f50, 0x850a, 0x41cf,
      { 0x9c, 0x72, 0x43, 0x0f, 0x29, 0x02, 0x90, 0xc8 } };

typedef struct IPolicyConfigVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(void* This, REFIID riid, void** ppvObject);
    ULONG   (STDMETHODCALLTYPE *AddRef)(void* This);
    ULONG   (STDMETHODCALLTYPE *Release)(void* This);

    HRESULT (STDMETHODCALLTYPE *GetMixFormat)(void* This, PCWSTR pszDeviceName, WAVEFORMATEX** ppFormat);
    HRESULT (STDMETHODCALLTYPE *GetDeviceFormat)(void* This, PCWSTR pszDeviceName, INT bDefault, WAVEFORMATEX** ppFormat);
    HRESULT (STDMETHODCALLTYPE *ResetDeviceFormat)(void* This, PCWSTR pszDeviceName);

    HRESULT (STDMETHODCALLTYPE *SetDeviceFormat)(void* This, PCWSTR pszDeviceName,
                                                 WAVEFORMATEX* pEndpointFormat,
                                                 WAVEFORMATEX* pMixFormat);
    HRESULT (STDMETHODCALLTYPE *GetProcessingPeriod)(void* This, PCWSTR pszDeviceName,
                                                     INT bDefault,
                                                     INT64* pmnPeriod,
                                                     INT64* pmxPeriod);
    HRESULT (STDMETHODCALLTYPE *SetProcessingPeriod)(void* This, PCWSTR pszDeviceName,
                                                     INT64* pPeriod);
    HRESULT (STDMETHODCALLTYPE *GetShareMode)(void* This, PCWSTR pszDeviceName, void* pMode);
    HRESULT (STDMETHODCALLTYPE *SetShareMode)(void* This, PCWSTR pszDeviceName, void* pMode);
    HRESULT (STDMETHODCALLTYPE *GetPropertyValue)(void* This, PCWSTR pszDeviceName,
                                                  const PROPERTYKEY* pKey,
                                                  PROPVARIANT* pv);
    HRESULT (STDMETHODCALLTYPE *SetPropertyValue)(void* This, PCWSTR pszDeviceName,
                                                  const PROPERTYKEY* pKey,
                                                  PROPVARIANT* pv);
    HRESULT (STDMETHODCALLTYPE *SetDefaultEndpoint)(void* This, PCWSTR pszDeviceName, ERole role);
    HRESULT (STDMETHODCALLTYPE *SetEndpointVisibility)(void* This, PCWSTR pszDeviceName, INT bVisible);
} IPolicyConfigVtbl;

typedef struct IPolicyConfig {
    IPolicyConfigVtbl* lpVtbl;
} IPolicyConfig;

int get_system_volume() {
    float volume = 0.0f;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&endpointVolume))) {
                endpointVolume->lpVtbl->GetMasterVolumeLevelScalar(endpointVolume, &volume);
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
    return (int)(volume * 100.0f + 0.5f);
}

void set_system_volume(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    float volume = (float)percent / 100.0f;

    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
            if (SUCCEEDED(device->lpVtbl->Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&endpointVolume))) {
                endpointVolume->lpVtbl->SetMasterVolumeLevelScalar(endpointVolume, volume, NULL);
                if (percent > 0) {
                    /* Unmute if volume is non-zero */
                    endpointVolume->lpVtbl->SetMute(endpointVolume, FALSE, NULL);
                }
                endpointVolume->lpVtbl->Release(endpointVolume);
            }
            device->lpVtbl->Release(device);
        }
        enumerator->lpVtbl->Release(enumerator);
    }
}

void update_audio_device_list() {
    hg_g_audio_device_count = 0;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDeviceCollection* collection = NULL;
    IMMDevice* default_dev = NULL;
    LPWSTR default_id = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&enumerator))) {
        if (SUCCEEDED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &default_dev))) {
            default_dev->lpVtbl->GetId(default_dev, &default_id);
            default_dev->lpVtbl->Release(default_dev);
        }

        if (SUCCEEDED(enumerator->lpVtbl->EnumAudioEndpoints(enumerator, eRender, DEVICE_STATE_ACTIVE, &collection))) {
            UINT count = 0;
            collection->lpVtbl->GetCount(collection, &count);
            for (UINT i = 0; i < count && hg_g_audio_device_count < HG_MAX_AUDIO_DEVICES; i++) {
                IMMDevice* device = NULL;
                if (SUCCEEDED(collection->lpVtbl->Item(collection, i, &device))) {
                    LPWSTR id = NULL;
                    if (SUCCEEDED(device->lpVtbl->GetId(device, &id))) {
                        IPropertyStore* props = NULL;
                        if (SUCCEEDED(device->lpVtbl->OpenPropertyStore(device, STGM_READ, &props))) {
                            PROPVARIANT var;
                            PropVariantInit(&var);
                            if (SUCCEEDED(props->lpVtbl->GetValue(props, &PKEY_Device_FriendlyName, &var)) &&
                                var.vt == VT_LPWSTR &&
                                var.pwszVal) {

                                if (SUCCEEDED(StringCchCopyW(hg_g_audio_devices[hg_g_audio_device_count].name,
                                                             HG_MAX_STR, var.pwszVal)) &&
                                    SUCCEEDED(StringCchCopyW(hg_g_audio_devices[hg_g_audio_device_count].id,
                                                             HG_MAX_STR, id))) {

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
        if (default_id) CoTaskMemFree(default_id);
        enumerator->lpVtbl->Release(enumerator);
    }
}

BOOL set_default_audio_device(const WCHAR* device_id) {
    if (!device_id || !*device_id) return FALSE;

    IPolicyConfig* policy = NULL;
    HRESULT hr = CoCreateInstance(
        &CLSID_CPolicyConfigClient,
        NULL,
        CLSCTX_ALL,
        &IID_IPolicyConfig,
        (void**)&policy
    );

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

/* =========================================================================
 * 핵심 기능 구현 (Core Implementation)
 * ========================================================================= */
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
    /* 기존 다크 모드의 색상(THEME_CUSTOM_*)을 라이트 모드에, 기존 라이트 모드의 색상을 다크 모드에 스왑하여 설정 */
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

void update_theme_colors() {
    HIGHCONTRASTW hc = {0};
    hc.cbSize = sizeof(HIGHCONTRASTW);
    BOOL is_hc = FALSE;
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &hc, 0)) {
        if (hc.dwFlags & HCF_HIGHCONTRASTON) is_hc = TRUE;
    }

    hg_g_is_high_contrast = is_hc;
    refresh_system_accent_color();

    hg_g_is_dark_mode = 0;
    DWORD use_light_theme = 1;
    DWORD cbData = sizeof(use_light_theme);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&use_light_theme, &cbData);
        RegCloseKey(hKey);
    }
    if (!hg_g_is_high_contrast && use_light_theme == 0) hg_g_is_dark_mode = 1;

    init_color_scheme();

    if (!hg_g_is_high_contrast && hg_g_has_system_accent_color) {
        hg_g_color_scheme_dark.flash = hg_g_system_accent_color;
    }

    if (hg_g_is_high_contrast) {
        hg_g_color_scheme_selected = (color_scheme_t) {
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

void apply_dwm_attributes(HWND hwnd);

static BOOL should_refresh_theme_on_setting_change(LPARAM l_param)
{
    const WCHAR* setting = (const WCHAR*)l_param;

    if (!setting || !*setting) {
        return TRUE;
    }

    return lstrcmpiW(setting, L"ImmersiveColorSet") == 0
        || lstrcmpiW(setting, L"AppsUseLightTheme") == 0
        || lstrcmpiW(setting, L"WindowsThemeElement") == 0
        || lstrcmpiW(setting, L"HighContrast") == 0;
}

static void refresh_theme_surfaces(HWND hwnd)
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

    if (hg_g_controlbox_wnd && IsWindow(hg_g_controlbox_wnd)) {
        SetClassLongPtrW(hg_g_controlbox_wnd, GCLP_HBRBACKGROUND, (LONG_PTR)hg_g_main_bg_brush);
        InvalidateRect(hg_g_controlbox_wnd, NULL, TRUE);
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

static BOOL register_app_window_class(HINSTANCE instance, const WindowClassSpec* spec, HICON icon_large, HICON icon_small)
{
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = spec->wnd_proc;
    wc.hInstance = instance;
    wc.lpszClassName = spec->class_name;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = spec->background;
    wc.hIcon = icon_large;
    wc.hIconSm = icon_small;

    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            MessageBoxW(NULL, spec->fail_message, L"hgfloater", MB_ICONERROR);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL register_app_window_classes(HINSTANCE instance, HICON icon_large, HICON icon_small)
{
    const WindowClassSpec specs[] = {
        { HG_CLASS_FLOATER_WIDGET, floater_proc, hg_g_main_bg_brush, L"Failed to register floater class." },
        { HG_CLASS_CONTROLBOX, controlbox_proc, hg_g_main_bg_brush, L"Failed to register controlbox class." },
        { HG_CLASS_ABOUT, about_proc, hg_g_main_bg_brush, L"Failed to register about class." },
        { HG_CLASS_TASKBOX, window_proc, hg_g_main_bg_brush, L"Failed to register taskbox class." },
        { HG_CLASS_MONITOR, monitor_wnd_proc, (HBRUSH)GetStockObject(BLACK_BRUSH), L"Failed to register monitor class." },
    };

    for (int i = 0; i < (int)HG_ARRAYSIZE(specs); ++i) {
        if (!register_app_window_class(instance, &specs[i], icon_large, icon_small)) {
            return FALSE;
        }
    }
    return TRUE;
}

static void unregister_app_window_classes(HINSTANCE instance)
{
    const WCHAR* classes[] = {
        HG_CLASS_FLOATER_WIDGET,
        HG_CLASS_CONTROLBOX,
        HG_CLASS_ABOUT,
        HG_CLASS_TASKBOX,
        HG_CLASS_MONITOR,
    };

    for (int i = 0; i < (int)HG_ARRAYSIZE(classes); ++i) {
        if (!UnregisterClassW(classes[i], instance)) {
            DWORD err = GetLastError();
            if (err != ERROR_CLASS_DOES_NOT_EXIST) {
                /* Ignore cleanup noise; class teardown happens at process exit anyway. */
            }
        }
    }
}

void apply_dwm_attributes(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    BOOL use_immersive_dark_mode = hg_g_is_dark_mode && !hg_g_is_high_contrast;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_immersive_dark_mode, sizeof(use_immersive_dark_mode));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &hg_g_color_scheme_selected.border, sizeof(hg_g_color_scheme_selected.border));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &hg_g_color_scheme_selected.bg, sizeof(hg_g_color_scheme_selected.bg));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &hg_g_color_scheme_selected.text, sizeof(hg_g_color_scheme_selected.text));

    int corner_pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    if (hg_g_monitor_count >= HG_MAX_MONITORS) return FALSE;
    MONITORINFOEXW mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, (MONITORINFO*)&mi)) {
        hg_g_monitors[hg_g_monitor_count].hMonitor = hMonitor;
        hg_g_monitors[hg_g_monitor_count].rcMonitor = mi.rcMonitor;
        StringCchCopyW(hg_g_monitors[hg_g_monitor_count].name, 64, mi.szDevice);
        /* If window already exists, keep it */
        hg_g_monitor_count++;
    }
    return TRUE;
}

void update_monitor_enum() {
    /* Preserve existing active windows */
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
                saved_monitors[j].hwnd = NULL; /* Mark as matched */
                break;
            }
        }
    }

    /* Destroy any orphan windows that are no longer associated with a monitor */
    for (int j = 0; j < saved_count; ++j) {
        if (saved_monitors[j].hwnd && IsWindow(saved_monitors[j].hwnd)) {
            SendMessageW(saved_monitors[j].hwnd, WM_CLOSE, 0, 0);
        }
    }
}

HRESULT hellgates_wsprintf(LPWSTR dest, size_t dest_size, LPCWSTR format, ...) {
    va_list arg_list;
    va_start(arg_list, format);
    HRESULT hr = StringCchVPrintfW(dest, dest_size, format, arg_list);
    va_end(arg_list);
    return hr;
}

/*
 * Normalize a path for long path support by ensuring it's absolute
 * and optionally adding the \\?\ prefix if needed.
 * Note: shell functions like ShellExecute don't like \\?\, but
 * basic file APIs like GetFileAttributesW/CreateFileW love it.
 */
void normalize_path_for_api(const WCHAR* input, WCHAR* output, size_t output_size) {
    if (!input || !output || output_size == 0) return;

    /* Resolve relative paths to absolute first */
    WCHAR full[HG_MAX_PATH];
    if (GetFullPathNameW(input, HG_MAX_PATH, full, NULL) == 0) {
        StringCchCopyW(output, output_size, input);
        return;
    }

    /* Already has prefix? */
    if (wcsncmp(full, L"\\\\?\\", 4) == 0) {
        StringCchCopyW(output, output_size, full);
        return;
    }

    /* Add \\?\ prefix for local absolute paths (e.g. C:\...) */
    if (iswalpha(full[0]) && full[1] == L':' && full[2] == L'\\') {
        StringCchPrintfW(output, output_size, L"\\\\?\\%ls", full);
    } else if (full[0] == L'\\' && full[1] == L'\\') {
        /* UNC: \\server\share -> \\?\UNC\server\share */
        StringCchPrintfW(output, output_size, L"\\\\?\\UNC\\%ls", full + 2);
    } else {
        StringCchCopyW(output, output_size, full);
    }
}

void init_paths() {
    WCHAR profile[HG_MAX_PATH] = { 0 };
    if (GetEnvironmentVariableW(L"USERPROFILE", profile, HG_MAX_PATH) == 0) {
        StringCchCopyW(profile, HG_MAX_PATH, L"C:\\");
    }
    hellgates_wsprintf(hg_g_base_path, HG_MAX_PATH, L"%ls\\.HellGates\\hgfloater", profile);
    hellgates_wsprintf(hg_g_shortcuts_path, HG_MAX_PATH, L"%ls\\shortcuts", hg_g_base_path);
    hellgates_wsprintf(hg_g_config_path, HG_MAX_PATH, L"%ls\\config.ini", hg_g_base_path);

    SHCreateDirectoryExW(NULL, hg_g_base_path, NULL);
    SHCreateDirectoryExW(NULL, hg_g_shortcuts_path, NULL);

    /* Migrate legacy config.ini.txt to config.ini if it exists */
    WCHAR legacy_config_path[HG_MAX_PATH];
    hellgates_wsprintf(legacy_config_path, HG_MAX_PATH, L"%ls\\config.ini.txt", hg_g_base_path);

    WCHAR norm_legacy[HG_MAX_PATH], norm_config[HG_MAX_PATH];
    normalize_path_for_api(legacy_config_path, norm_legacy, HG_MAX_PATH);
    normalize_path_for_api(hg_g_config_path, norm_config, HG_MAX_PATH);

    if (GetFileAttributesW(norm_legacy) != INVALID_FILE_ATTRIBUTES && GetFileAttributesW(norm_config) == INVALID_FILE_ATTRIBUTES) {
        MoveFileW(norm_legacy, norm_config);
    }
}

/* 창 필터링: 윈도우 11 작업 표시줄과 유사한 로직 */
BOOL is_alt_tab_window(HWND hwnd) {
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd || hwnd == hg_g_about_wnd) return FALSE;
    if (!IsWindow(hwnd)) return FALSE;
    if (!IsWindowVisible(hwnd)) return FALSE;

    /* 텍스트가 없는 창은 제외 */
    if (GetWindowTextLengthW(hwnd) == 0) return FALSE;

    /* 도구 창 제외 */
    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ex_style & WS_EX_TOOLWINDOW) return FALSE;

    /* 소유자가 있는 창은 기본적으로 제외 (단, 앱 윈도우 스타일이 있으면 포함) */
    HWND owner_hwnd = GetWindow(hwnd, GW_OWNER);
    if (owner_hwnd != NULL && !(ex_style & WS_EX_APPWINDOW)) return FALSE;

    /* Cloaked 상태 체크 (가상 데스크톱 등 숨겨진 창 제외) */
    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if (cloaked != 0) return FALSE;
    }

    return TRUE;
}

/* 프로세스 이름 가져오기 */
void get_process_name_by_hwnd(HWND hwnd, WCHAR* out_name, size_t out_size, DWORD* out_pid) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid) *out_pid = pid;

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
        if (!exe_name || !*exe_name) exe_name = path;
        StringCchCopyW(out_name, out_size, exe_name);
    } else {
        StringCchCopyW(out_name, out_size, L"Unknown");
    }

    CloseHandle(process_handle);
}

/* get full path by hwnd */
void get_process_path_by_hwnd(HWND hwnd, WCHAR* out_path, size_t out_size, DWORD* out_pid) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (out_pid) *out_pid = pid;

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

#ifndef ICON_SMALL2
#define ICON_SMALL2 2
#endif

/*
 * uwp_icon_helpers.c - helper functions for retrieving Win32/UWP window icons.
 *
 * Integration notes:
 *   - Include this after windows.h, strsafe.h, shellapi.h, shlobj.h, and related Win32 headers.
 *   - Requires COM initialized on the calling thread before get_window_icon() is used
 *     if package-logo image loading is expected to work:
 *         CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)
 *   - The caller must provide:
 *         void get_process_name_by_hwnd(HWND hwnd, WCHAR* name, DWORD* id);
 *     or adjust the calls below if your get_process_name_by_hwnd() signature is different.
 *   - Returned icons follow this ownership rule:
 *         *own_icon == FALSE: borrowed icon; do not DestroyIcon().
 *         *own_icon == TRUE : owned icon; caller must DestroyIcon().
 */

static int hg_clamp_icon_size_px(int size_px)
{
    if (size_px < HG_UWP_MIN_ICON_PX) return HG_UWP_MIN_ICON_PX;
    if (size_px > HG_UWP_MAX_ICON_PX) return HG_UWP_MAX_ICON_PX;
    return size_px;
}

static BOOL CALLBACK find_uwp_child_window(HWND hwnd, LPARAM lParam)
{
    FindUWPChildData* data = (FindUWPChildData*)lParam;
    if (!data || !IsWindow(hwnd)) return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == 0 || pid == data->frame_pid) {
        return TRUE;
    }

    WCHAR cls[128] = {0};
    GetClassNameW(hwnd, cls, HG_ARRAYSIZE(cls));

    /* Prefer the actual UWP CoreWindow if present. */
    if (wcscmp(cls, L"Windows.UI.Core.CoreWindow") == 0) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
        return FALSE;
    }

    /* Fallback: first child owned by a different process. */
    if (!data->best_hwnd) {
        data->best_hwnd = hwnd;
        data->best_pid = pid;
    }

    return TRUE;
}

static BOOL get_real_uwp_child(HWND frame_hwnd, HWND* out_hwnd, DWORD* out_pid)
{
    if (out_hwnd) *out_hwnd = NULL;
    if (out_pid) *out_pid = 0;
    if (!IsWindow(frame_hwnd)) return FALSE;

    DWORD frame_pid = 0;
    GetWindowThreadProcessId(frame_hwnd, &frame_pid);
    if (!frame_pid) return FALSE;

    FindUWPChildData data;
    ZeroMemory(&data, sizeof(data));
    data.frame_pid = frame_pid;

    EnumChildWindows(frame_hwnd, find_uwp_child_window, (LPARAM)&data);

    if (!data.best_hwnd || !data.best_pid || !IsWindow(data.best_hwnd)) {
        return FALSE;
    }

    if (out_hwnd) *out_hwnd = data.best_hwnd;
    if (out_pid) *out_pid = data.best_pid;
    return TRUE;
}

static HICON get_icon_from_hwnd_msg(HWND hwnd)
{
    if (!IsWindow(hwnd)) return NULL;

    HICON h_icon = NULL;
    DWORD_PTR res = 0;

    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0,
                            SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon &&
        SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0,
                            SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    if (!h_icon &&
        SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0,
                            SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &res)) {
        h_icon = (HICON)res;
    }

    return h_icon;
}

static HICON get_icon_from_hwnd_class(HWND hwnd)
{
    if (!IsWindow(hwnd)) return NULL;

    HICON h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    if (!h_icon) h_icon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);

    return h_icon; /* Borrowed icon. */
}


static HICON get_icon_from_process_exe(DWORD pid, BOOL* own_icon)
{
    if (own_icon) *own_icon = FALSE;
    if (pid == 0) return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc) return NULL;

    WCHAR path[HG_MAX_PATH] = {0};
    DWORD size = HG_ARRAYSIZE(path);
    HICON icon = NULL;

    if (QueryFullProcessImageNameW(h_proc, 0, path, &size)) {
        SHFILEINFOW sfi = {0};
        if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
            icon = sfi.hIcon;
            if (own_icon) *own_icon = TRUE;
        } else if (ExtractIconExW(path, 0, &icon, NULL, 1) > 0 && icon) {
            if (own_icon) *own_icon = TRUE;
        }
    }

    CloseHandle(h_proc);
    return icon;
}

static WCHAR* get_aumid_from_hwnd(HWND hwnd)
{
    if (!hwnd) return NULL;
    IPropertyStore* pps = NULL;
    HRESULT hr = SHGetPropertyStoreForWindow(hwnd, &IID_IPropertyStore, (void**)&pps);
    if (SUCCEEDED(hr) && pps) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        hr = pps->lpVtbl->GetValue(pps, &PKEY_AppUserModel_ID, &pv);
        WCHAR* aumid = NULL;
        if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal) {
            size_t len = wcslen(pv.pwszVal) + 1;
            aumid = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
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

static WCHAR* get_app_user_model_id_alloc(DWORD pid)
{
    if (pid == 0) return NULL;

    HANDLE h_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h_proc) return NULL;

    UINT32 len = 0;
    LONG rc = GetApplicationUserModelId(h_proc, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        CloseHandle(h_proc);
        return NULL;
    }

    WCHAR* aumid = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
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

static WCHAR* get_package_full_name_alloc(DWORD pid)
{
    if (pid == 0) return NULL;

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

    WCHAR* full = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
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

static WCHAR* get_package_path_alloc(const WCHAR* full_name)
{
    if (!full_name || !*full_name) return NULL;

    UINT32 len = 0;
    LONG rc = GetPackagePathByFullName(full_name, &len, NULL);

    if (rc != ERROR_INSUFFICIENT_BUFFER || len == 0) {
        uwp_debug_log(L"UWP icon: GetPackagePathByFullName length query failed");
        return NULL;
    }

    WCHAR* path = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)len * sizeof(WCHAR));
    if (!path) return NULL;

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
    return (ch >= L'a' && ch <= L'z') ||
           (ch >= L'A' && ch <= L'Z') ||
           (ch >= L'0' && ch <= L'9') ||
           ch == L'_' || ch == L'-' || ch == L':' || ch == L'.';
}

static BOOL find_xml_attr_value_safe(const WCHAR* text, const WCHAR* attr, WCHAR* out, size_t out_cch)
{
    if (!text || !attr || !*attr || !out || out_cch == 0) return FALSE;
    out[0] = L'\0';

    size_t attr_len = wcslen(attr);
    const WCHAR* p = text;

    while ((p = wcsstr(p, attr)) != NULL) {
        BOOL valid_prev = (p == text) || (!is_xml_name_char(*(p - 1)));
        BOOL valid_next = (!is_xml_name_char(*(p + attr_len)));

        if (valid_prev && valid_next) {
            const WCHAR* cur = p + attr_len;
            while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n') cur++;

            if (*cur == L'=') {
                cur++;
                while (*cur == L' ' || *cur == L'\t' || *cur == L'\r' || *cur == L'\n') cur++;

                WCHAR quote = *cur;
                if (quote == L'\'' || quote == L'"') {
                    cur++;
                    const WCHAR* q = wcschr(cur, quote);
                    if (q) {
                        size_t n = (size_t)(q - cur);
                        if (n >= out_cch) n = out_cch - 1;
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

static BOOL read_utf8_file_to_wide(const WCHAR* path, WCHAR** out_text)
{
    if (!out_text) return FALSE;
    *out_text = NULL;
    if (!path || !*path) return FALSE;

    WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);

    HANDLE h = CreateFileW(norm, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return FALSE;

    LARGE_INTEGER li;
    if (!GetFileSizeEx(h, &li) || li.QuadPart <= 0 || li.QuadPart > HG_UWP_MAX_MANIFEST_BYTES) {
        CloseHandle(h);
        return FALSE;
    }

    DWORD bytes = (DWORD)li.QuadPart;
    BYTE* buf = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)bytes + 1u);
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
        /* Some manifests may not be strict UTF-8. Last-resort fallback. */
        codepage = CP_ACP;
        flags = 0;
        wlen = MultiByteToWideChar(codepage, flags, (LPCCH)buf, (int)read, NULL, 0);
    }

    if (wlen <= 0 || (size_t)wlen > HG_UWP_MAX_MANIFEST_BYTES) {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    WCHAR* wbuf = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    ((SIZE_T)wlen + 1u) * sizeof(WCHAR));
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

static BOOL get_logo_relpath_from_manifest(const WCHAR* package_path, WCHAR* out_rel, size_t out_cch)
{
    if (!package_path || !out_rel || out_cch == 0) return FALSE;
    out_rel[0] = L'\0';

    WCHAR manifest_path[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(manifest_path, HG_ARRAYSIZE(manifest_path),
                                  L"%ls\\AppxManifest.xml", package_path);
    if (FAILED(hr)) return FALSE;

    WCHAR* manifest = NULL;
    if (!read_utf8_file_to_wide(manifest_path, &manifest)) {
        uwp_debug_log(L"UWP icon: Failed to read AppxManifest.xml");
        return FALSE;
    }

    BOOL ok =
        find_xml_attr_value_safe(manifest, L"Square44x44Logo", out_rel, out_cch) ||
        find_xml_attr_value_safe(manifest, L"Square150x150Logo", out_rel, out_cch) ||
        find_xml_attr_value_safe(manifest, L"Logo", out_rel, out_cch);

    if (!ok) uwp_debug_log(L"UWP icon: Could not find logo attribute");
    HeapFree(GetProcessHeap(), 0, manifest);
    return ok;
}

static BOOL file_exists_w(const WCHAR* path)
{
    WCHAR norm[HG_MAX_PATH];
    normalize_path_for_api(path, norm, HG_MAX_PATH);
    DWORD attr = GetFileAttributesW(norm);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void normalize_slashes(WCHAR* s)
{
    if (!s) return;
    for (; *s; s++) {
        if (*s == L'/') *s = L'\\';
    }
}

static BOOL resolve_logo_asset_file(const WCHAR* package_path,
                                    const WCHAR* rel_logo,
                                    int size_px,
                                    WCHAR* out_file,
                                    size_t out_cch)
{
    if (!package_path || !rel_logo || !out_file || out_cch == 0) return FALSE;
    out_file[0] = L'\0';
    size_px = hg_clamp_icon_size_px(size_px);

    WCHAR rel[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(rel, HG_ARRAYSIZE(rel), rel_logo))) return FALSE;
    normalize_slashes(rel);

    WCHAR base[HG_MAX_PATH];
    if (FAILED(StringCchPrintfW(base, HG_ARRAYSIZE(base), L"%ls\\%ls", package_path, rel))) {
        return FALSE;
    }

    if (file_exists_w(base)) {
        return SUCCEEDED(StringCchCopyW(out_file, out_cch, base));
    }

    WCHAR dir[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), base))) return FALSE;

    WCHAR* slash = wcsrchr(dir, L'\\');
    const WCHAR* filename = base;
    if (slash) {
        *slash = L'\0';
        filename = slash + 1;
    } else {
        if (FAILED(StringCchCopyW(dir, HG_ARRAYSIZE(dir), package_path))) return FALSE;
    }

    WCHAR stem[HG_MAX_PATH];
    if (FAILED(StringCchCopyW(stem, HG_ARRAYSIZE(stem), filename))) return FALSE;

    WCHAR* dot = wcsrchr(stem, L'.');
    WCHAR ext[16] = L".png";
    if (dot) {
        if (FAILED(StringCchCopyW(ext, HG_ARRAYSIZE(ext), dot))) return FALSE;
        *dot = L'\0';
    }

    WCHAR candidate[HG_MAX_PATH];

    const WCHAR* dyn_patterns[] = {
        L"%ls\\%ls.targetsize-%d_altform-unplated%ls",
        L"%ls\\%ls.targetsize-%d%ls"
    };

    for (size_t i = 0; i < HG_ARRAYSIZE(dyn_patterns); i++) {
        if (SUCCEEDED(StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate),
                                      dyn_patterns[i], dir, stem, size_px, ext)) &&
            file_exists_w(candidate)) {
            return SUCCEEDED(StringCchCopyW(out_file, out_cch, candidate));
        }
    }

    const WCHAR* suffixes[] = {
        L".targetsize-16_altform-unplated",
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
        L".scale-400"
    };

    for (size_t i = 0; i < HG_ARRAYSIZE(suffixes); i++) {
        if (SUCCEEDED(StringCchPrintfW(candidate, HG_ARRAYSIZE(candidate),
                                      L"%ls\\%ls%ls%ls", dir, stem, suffixes[i], ext)) &&
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
    if (!hbmp) return NULL;

    BITMAP bm;
    if (!GetObjectW(hbmp, sizeof(bm), &bm)) {
        return NULL;
    }

    if (bm.bmWidth <= 0 || bm.bmHeight <= 0) {
        return NULL;
    }

    /* 1bpp mask stride is WORD-aligned. Guard against absurd dimensions. */
    if (bm.bmWidth > 4096 || bm.bmHeight > 4096) {
        return NULL;
    }

    size_t stride_bits = ((size_t)bm.bmWidth + 15u) & ~15u;
    size_t stride_bytes = stride_bits / 8u;
    size_t mask_size = stride_bytes * (size_t)bm.bmHeight;

    BYTE* mask_bits = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mask_size);
    if (!mask_bits) return NULL;

    HBITMAP hmask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, mask_bits);
    HeapFree(GetProcessHeap(), 0, mask_bits);

    if (!hmask) return NULL;

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = TRUE;
    ii.hbmColor = hbmp;
    ii.hbmMask = hmask;

    HICON icon = CreateIconIndirect(&ii);

    DeleteObject(hmask);
    return icon;
}

static HRESULT hg_get_shell_image_bitmap(IShellItemImageFactory* factory, SIZE sz, HBITMAP* out_bmp)
{
    HRESULT hr;
    if (!out_bmp) return E_POINTER;
    *out_bmp = NULL;
    if (!factory) return E_POINTER;

    hr = factory->lpVtbl->GetImage(
        factory,
        sz,
        SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT | SIIGBF_THUMBNAILONLY,
        out_bmp
    );
    if (SUCCEEDED(hr) && *out_bmp) return hr;

    hr = factory->lpVtbl->GetImage(
        factory,
        sz,
        SIIGBF_BIGGERSIZEOK | SIIGBF_RESIZETOFIT,
        out_bmp
    );
    if (SUCCEEDED(hr) && *out_bmp) return hr;

    return factory->lpVtbl->GetImage(
        factory,
        sz,
        SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY,
        out_bmp
    );
}

static HICON load_icon_from_image_file(const WCHAR* file, int size_px, BOOL* own_icon)
{
    if (own_icon) *own_icon = FALSE;
    if (!file || !*file) return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    IShellItemImageFactory* factory = NULL;
    HRESULT hr = SHCreateItemFromParsingName(
        file,
        NULL,
        &IID_IShellItemImageFactory,
        (void**)&factory
    );

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

#include <knownfolders.h>

static HICON load_icon_from_aumid(const WCHAR* aumid, int size_px, BOOL* own_icon)
{
    if (own_icon) *own_icon = FALSE;
    if (!aumid || !*aumid) return NULL;

    WCHAR parsing_name[HG_MAX_PATH];
    HRESULT hr = StringCchPrintfW(parsing_name, HG_ARRAYSIZE(parsing_name), L"shell:AppsFolder\\%ls", aumid);
    if (FAILED(hr)) return NULL;

    IShellItem* psi = NULL;
    hr = SHCreateItemFromParsingName(parsing_name, NULL, &IID_IShellItem, (void**)&psi);
    if (FAILED(hr) || !psi) return NULL;

    IShellItemImageFactory* factory = NULL;
    hr = psi->lpVtbl->QueryInterface(psi, &IID_IShellItemImageFactory, (void**)&factory);
    psi->lpVtbl->Release(psi);
    if (FAILED(hr) || !factory) return NULL;

    SIZE sz;
    sz.cx = hg_clamp_icon_size_px(size_px);
    sz.cy = sz.cx;

    HBITMAP hbmp = NULL;
    hr = hg_get_shell_image_bitmap(factory, sz, &hbmp);
    factory->lpVtbl->Release(factory);

    if (FAILED(hr) || !hbmp) return NULL;

    HICON icon = create_icon_from_bitmap(hbmp);
    DeleteObject(hbmp);

    if (icon && own_icon) {
        *own_icon = TRUE;
    }

    return icon;
}

static HICON get_icon_from_package_pid(DWORD pid, int size_px, BOOL* own_icon)
{
    if (own_icon) *own_icon = FALSE;
    size_px = hg_clamp_icon_size_px(size_px);

    WCHAR* aumid = get_app_user_model_id_alloc(pid);
    if (aumid) {
        HICON icon = load_icon_from_aumid(aumid, size_px, own_icon);
        HeapFree(GetProcessHeap(), 0, aumid);
        if (icon) return icon;
    }

    WCHAR* full_name = get_package_full_name_alloc(pid);
    if (!full_name) return NULL;

    WCHAR* package_path = get_package_path_alloc(full_name);
    HeapFree(GetProcessHeap(), 0, full_name);
    if (!package_path) return NULL;

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

HICON get_window_icon(HWND hwnd, int size_px, BOOL* own_icon)
{
    if (own_icon) *own_icon = FALSE;
    if (!IsWindow(hwnd)) return NULL;

    size_px = hg_clamp_icon_size_px(size_px);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    WCHAR proc_name[HG_MAX_STR] = {0};
    get_process_name_by_hwnd(hwnd, proc_name, HG_MAX_STR, NULL);

    HICON h_icon = NULL;

    if (_wcsicmp(proc_name, L"ApplicationFrameHost.exe") == 0) {
        /* ApplicationFrameHost specific logic */

        /* Frame AUMID */
        WCHAR* frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HeapFree(GetProcessHeap(), 0, frame_aumid);
            if (h_icon) return h_icon;
        }

        HWND child_hwnd = NULL;
        DWORD child_pid = 0;

        if (get_real_uwp_child(hwnd, &child_hwnd, &child_pid)) {
            /* Child's AUMID */
            WCHAR* child_aumid = get_aumid_from_hwnd(child_hwnd);
            if (child_aumid) {
                h_icon = load_icon_from_aumid(child_aumid, size_px, own_icon);
                HeapFree(GetProcessHeap(), 0, child_aumid);
                if (h_icon) return h_icon;
            }

            /* Child's package/AUMID icon */
            h_icon = get_icon_from_package_pid(child_pid, size_px, own_icon);
            if (h_icon) return h_icon;

            /* Child's WM_GETICON */
            h_icon = get_icon_from_hwnd_msg(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon) *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon) *own_icon = FALSE;
                return h_icon;
            }

            /* Child's class icon */
            h_icon = get_icon_from_hwnd_class(child_hwnd);
            if (h_icon) {
                HICON h_copy = CopyIcon(h_icon);
                if (h_copy) {
                    if (own_icon) *own_icon = TRUE;
                    return h_copy;
                }
                if (own_icon) *own_icon = FALSE;
                return h_icon;
            }

            /* Child's executable icon */
            h_icon = get_icon_from_process_exe(child_pid, own_icon);
            if (h_icon) return h_icon;
        }

        /* Fallback to frame WM_GETICON */
        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon) *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon) *own_icon = FALSE;
            return h_icon;
        }

    } else {
        /* Standard or standalone Packaged win32/UWP apps */

        /* 1. Try WM_GETICON first for standard apps. */
        h_icon = get_icon_from_hwnd_msg(hwnd);
        if (h_icon) {
            HICON h_copy = CopyIcon(h_icon);
            if (h_copy) {
                if (own_icon) *own_icon = TRUE;
                return h_copy;
            }
            if (own_icon) *own_icon = FALSE;
            return h_icon;
        }

        /* 2. HWND AUMID */
        WCHAR* frame_aumid = get_aumid_from_hwnd(hwnd);
        if (frame_aumid) {
            h_icon = load_icon_from_aumid(frame_aumid, size_px, own_icon);
            HeapFree(GetProcessHeap(), 0, frame_aumid);
            if (h_icon) return h_icon;
        }

        /* 3. Package/AUMID icon */
        h_icon = get_icon_from_package_pid(pid, size_px, own_icon);
        if (h_icon) return h_icon;
    }

    /* Fallback to original window class icon */
    h_icon = get_icon_from_hwnd_class(hwnd);
    if (h_icon) {
        HICON h_copy = CopyIcon(h_icon);
        if (h_copy) {
            if (own_icon) *own_icon = TRUE;
            return h_copy;
        }
        if (own_icon) *own_icon = FALSE;
        return h_icon;
    }

    /* Ultimate fallback to executable file icon */
    return get_icon_from_process_exe(pid, own_icon);
}

/* 창 목록 정렬 제거 (사용자 순서 유지) */

/* 단축아이콘 정렬을 위한 비교 함수 */
int compare_shortcuts(const void* a, const void* b) {
    ShortcutItem* item_a = (ShortcutItem*)a;
    ShortcutItem* item_b = (ShortcutItem*)b;
    return lstrcmpiW(item_a->name, item_b->name);
}

/* 단축아이콘 로드 */
void load_shortcuts() {
    for (int i = 0; i < hg_g_shortcut_count; i++) {
        if (hg_g_shortcuts[i].icon) {
            DestroyIcon(hg_g_shortcuts[i].icon);
            hg_g_shortcuts[i].icon = NULL;
        }
    }
    ZeroMemory(hg_g_shortcuts, sizeof(hg_g_shortcuts));
    hg_g_shortcut_count = 0;

    if (!hg_g_shortcuts_path[0]) return;

    WCHAR search_path[HG_MAX_PATH] = {0};
    if (FAILED(StringCchPrintfW(search_path, HG_ARRAYSIZE(search_path), L"%ls\\*", hg_g_shortcuts_path))) {
        return;
    }

    WCHAR norm_search[HG_MAX_PATH];
    normalize_path_for_api(search_path, norm_search, HG_MAX_PATH);

    WIN32_FIND_DATAW ffd = {0};
    HANDLE find_handle = FindFirstFileW(norm_search, &ffd);
    if (find_handle == INVALID_HANDLE_VALUE) return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (hg_g_shortcut_count >= HG_MAX_SHORTCUTS) break;

        int raw_len = lstrlenW(ffd.cFileName);
        if (raw_len <= 4) continue;

        size_t len = (size_t)raw_len;
        BOOL is_lnk = (lstrcmpiW(ffd.cFileName + len - 4, L".lnk") == 0);
        BOOL is_url = (lstrcmpiW(ffd.cFileName + len - 4, L".url") == 0);
        if (!is_lnk && !is_url) continue;

        ShortcutItem* item = &hg_g_shortcuts[hg_g_shortcut_count];
        ZeroMemory(item, sizeof(*item));

        if (FAILED(StringCchPrintfW(item->path, HG_ARRAYSIZE(item->path), L"%ls\\%ls", hg_g_shortcuts_path, ffd.cFileName))) {
            continue;
        }

        if (FAILED(StringCchCopyW(item->name, HG_ARRAYSIZE(item->name), ffd.cFileName))) {
            continue;
        }
        WCHAR* dot = wcsrchr(item->name, L'.');
        if (dot) *dot = L'\0';

        int icon_size = ABS(hg_g_current_font_size);
        if (icon_size < SC(32)) icon_size = SC(32);

        if (is_lnk) {
            IShellLinkW* psl = NULL;
            HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                          &IID_IShellLinkW, (void**)&psl);
            if (SUCCEEDED(hr) && psl) {
                IPersistFile* ppf = NULL;
                if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf)) && ppf) {
                    if (SUCCEEDED(ppf->lpVtbl->Load(ppf, item->path, STGM_READ))) {
                        WCHAR target_path[HG_MAX_PATH] = {0};
                        if (SUCCEEDED(psl->lpVtbl->GetPath(psl, target_path, HG_ARRAYSIZE(target_path), NULL, 0)) &&
                            target_path[0] != L'\0') {
                            HICON ext_icon = NULL;
                            UINT icon_id = 0;
                            if (PrivateExtractIconsW(target_path, 0, icon_size, icon_size,
                                                     &ext_icon, &icon_id, 1, LR_LOADFROMFILE) > 0 && ext_icon) {
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

/* 메시지 에디트 컨트롤에 텍스트 추가 및 스크롤 (효율적인 행 관리) */
void append_message(const WCHAR* msg) {
    if (!hg_g_edit_msg_wnd || !msg) return;

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
    if (len > 0) SendMessageW(hg_g_edit_msg_wnd, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    int new_line_start = GetWindowTextLengthW(hg_g_edit_msg_wnd);
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, (WPARAM)new_line_start, (LPARAM)new_line_start);
    SendMessageW(hg_g_edit_msg_wnd, EM_REPLACESEL, FALSE, (LPARAM)msg);
    /* 커서를 새 줄의 시작 위치로 이동시켜 가로 스크롤이 맨 좌측에 있게 함 */
    SendMessageW(hg_g_edit_msg_wnd, EM_SETSEL, (WPARAM)new_line_start, (LPARAM)new_line_start);
    SendMessageW(hg_g_edit_msg_wnd, EM_SCROLLCARET, 0, 0);
}

/* 외곽선 있는 텍스트 그리기 헬퍼 */
void draw_outlined_text(HDC hdc, const WCHAR* text, int len, RECT* rc, UINT format, COLORREF text_color, COLORREF outline_color) {
    if (!hdc || !text || !rc || len <= 0) return;
    int old_bk_mode = SetBkMode(hdc, TRANSPARENT);
    COLORREF old_text_color = GetTextColor(hdc);

    SetTextColor(hdc, outline_color);
    RECT temp_rc;
    /* 8방향 외곽선으로 가독성 극대화 (DPI 배율에 맞춰 두께 조절) */
    int offset = SC(1);
    if (offset < 1) offset = 1;

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

/* 개별 툴바 아이템의 위치 계산 (통합 레이아웃 엔진) */
int get_items_per_row(int width, int icon_size) {
    if (width <= 0) return 1;
    int denom = icon_size + SC(15);
    if (denom <= 0) return 1;
    int n = (width - icon_size - SC(20)) / denom + 1;
    return (n > 0) ? n : 1;
}

void get_toolbar_item_rect(int item_type, int item_index, int width, int height, int icon_size, RECT* out_rect) {
    if (width <= 0 || !out_rect) {
        if (out_rect) SetRectEmpty(out_rect);
        return;
    }

    int cols = get_items_per_row(width, icon_size);
    if (cols <= 0) cols = 1;
    int row_height = icon_size + SC(10);

    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

    int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (min_required_rows <= 0) min_required_rows = 1;

    int visible_rows = (height - SC(20) + SC(10)) / row_height;
    if (visible_rows <= 0) visible_rows = 1;

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

/* 툴바 툴팁 업데이트 (통합 레이아웃 사용) */
void update_toolbar_tooltips(HWND hwnd) {
    if (!hg_g_tooltip_wnd || !hwnd) return;
    static int last_total_count = 0;

    for (int i = 0; i < last_total_count; i++) {
        TOOLINFOW ti = { 0 };
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.hwnd = hwnd;
        ti.uId = (UINT_PTR)i;
        SendMessageW(hg_g_tooltip_wnd, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }
    last_total_count = 0;

    RECT client_rc;
    GetClientRect(hwnd, &client_rc);
    if (client_rc.right <= 0 || client_rc.bottom <= 0) return;

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);

    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    int id_counter = 0;

    for (int i = 0; i < total_tasks; i++) {
        RECT item_rc;
        get_toolbar_item_rect(0, i, client_rc.right, client_rc.bottom, icon_size, &item_rc);

        TOOLINFOW ti_tool = { 0 };
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

        TOOLINFOW ti_tool = { 0 };
        ti_tool.cbSize = TOOLINFO_V1_SIZE;
        ti_tool.uFlags = TTF_SUBCLASS;
        ti_tool.hwnd = hwnd;
        ti_tool.uId = (UINT_PTR)id_counter++;
        if (i == 0) ti_tool.lpszText = L"Drag to Resize Window";
        else if (i == 1) ti_tool.lpszText = L"Drag to Move Window";
        else if (i == 2) ti_tool.lpszText = L"Hide Dashboard (Reload Shortcuts)";
        else if (i == 3) ti_tool.lpszText = L"Show Desktop";
        else if (i == 4) ti_tool.lpszText = L"Settings";
        else ti_tool.lpszText = hg_g_shortcuts[i - HG_NUM_BASIC_ICONS].name;

        ti_tool.rect = item_rc;
        InflateRect(&ti_tool.rect, SC(4), SC(4));
        SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti_tool);
    }

    SendMessageW(hg_g_tooltip_wnd, TTM_ACTIVATE, TRUE, 0);
    last_total_count = id_counter;
}

/* 바탕화면 보기 토글을 위한 EnumWindows 콜백 */
BOOL CALLBACK minimize_restore_enum_proc(HWND hwnd, LPARAM l_param) {
    BOOL is_minimize = (BOOL)l_param;
    if (hwnd == hg_g_taskbox_wnd || hwnd == hg_g_floater_wnd) return TRUE;
    if (!IsWindowVisible(hwnd)) return TRUE;

    /* 소유자가 없는 최상위 창이거나 작업 표시줄에 나타나는 창 스타일인 경우 */
    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    if (owner == NULL || (ex_style & WS_EX_APPWINDOW)) {
        /* Cloaked 상태 체크 (가상 데스크톱 등 숨겨진 창 제외) */
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
            if (cloaked != 0) return TRUE;
        }

        if (is_minimize) {
            if (!IsIconic(hwnd)) ShowWindowAsync(hwnd, SW_MINIMIZE);
        } else {
            if (IsIconic(hwnd)) ShowWindowAsync(hwnd, SW_RESTORE);
        }
    }
    return TRUE;
}

/* INI 설정 로드/저장 */
void load_config(const WCHAR* section, int* x, int* y, int* w, int* h, int def_x, int def_y, int def_w, int def_h) {
    WCHAR buf[32] = {0};
    WCHAR def_buf[32] = {0};
    WCHAR* endptr;
    hellgates_wsprintf(def_buf, 32, L"%d", def_x);
    GetPrivateProfileStringW(section, L"x", def_buf, buf, 32, hg_g_config_path);
    *x = (int)wcstol(buf, &endptr, 10);
    if (endptr == buf) *x = def_x;

    hellgates_wsprintf(def_buf, 32, L"%d", def_y);
    GetPrivateProfileStringW(section, L"y", def_buf, buf, 32, hg_g_config_path);
    *y = (int)wcstol(buf, &endptr, 10);
    if (endptr == buf) *y = def_y;

    UINT uw = GetPrivateProfileIntW(section, L"w", def_w, hg_g_config_path);
    *w = (uw == 0 || uw > 30000) ? def_w : (int)uw;

    UINT uh = GetPrivateProfileIntW(section, L"h", def_h, hg_g_config_path);
    *h = (uh == 0 || uh > 30000) ? def_h : (int)uh;

    /* Auto-initialize the dimensions to the file */
    save_config(section, *x, *y, *w, *h);
}

void save_config(const WCHAR* section, int x, int y, int w, int h) {
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", x);
    WritePrivateProfileStringW(section, L"x", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", y);
    WritePrivateProfileStringW(section, L"y", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", w);
    WritePrivateProfileStringW(section, L"w", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%d", h);
    WritePrivateProfileStringW(section, L"h", buf, hg_g_config_path);
}

void load_floater_font_config() {
    UINT fs = GetPrivateProfileIntW(L"floater", L"font_size", 28, hg_g_config_path);
    if (fs < HG_FLOATER_MIN_FONT_SIZE) fs = HG_FLOATER_MIN_FONT_SIZE;
    if (fs > HG_FLOATER_MAX_FONT_SIZE) fs = HG_FLOATER_MAX_FONT_SIZE;
    hg_g_floater_font_size = (int)fs;

    /* Write back to ensure structural initialization of config.ini */
    save_floater_font_config();
}

void save_floater_font_config() {
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", hg_g_floater_font_size);
    WritePrivateProfileStringW(L"floater", L"font_size", buf, hg_g_config_path);
}

void load_taskbox_font_config() {
    UINT fs = GetPrivateProfileIntW(L"taskbox", L"font_size", 16, hg_g_config_path);
    if (fs < 12) fs = 12;
    if (fs > 128) fs = 128;
    hg_g_edit_font_size = -SC((int)fs);

    UINT is = GetPrivateProfileIntW(L"taskbox", L"icon_size", 22, hg_g_config_path);
    if (is < 16) is = 16;
    if (is > 128) is = 128;
    hg_g_current_font_size = -SC((int)is);

    /* Write back to ensure structural initialization of config.ini */
    save_taskbox_font_config();
}

void save_taskbox_font_config() {
    WCHAR buf[32];
    int unscaled = (int)(ABS(hg_g_edit_font_size) / (hg_g_scale_factor > 0 ? hg_g_scale_factor : 1.0) + 0.5);
    hellgates_wsprintf(buf, 32, L"%d", unscaled);
    WritePrivateProfileStringW(L"taskbox", L"font_size", buf, hg_g_config_path);

    int unscaled_icon = (int)(ABS(hg_g_current_font_size) / (hg_g_scale_factor > 0 ? hg_g_scale_factor : 1.0) + 0.5);
    hellgates_wsprintf(buf, 32, L"%d", unscaled_icon);
    WritePrivateProfileStringW(L"taskbox", L"icon_size", buf, hg_g_config_path);
}

void save_hotkey_config() {
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%u", hg_g_hotkey_modifiers);
    WritePrivateProfileStringW(L"hotkeys", L"global_focus_modifiers", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%u", hg_g_hotkey_key);
    WritePrivateProfileStringW(L"hotkeys", L"global_focus_key", buf, hg_g_config_path);
}

int get_alpha_config(const WCHAR* section, int def_alpha) {
    int a = (int)GetPrivateProfileIntW(section, L"alpha", def_alpha, hg_g_config_path);
    if (a < 128) a = 128;
    if (a > 255) a = 255;

    /* Auto-initialize / format the alpha value inside config.ini */
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%d", a);
    WritePrivateProfileStringW(section, L"alpha", buf, hg_g_config_path);

    return a;
}

void load_font_name_config() {
    GetPrivateProfileStringW(L"etc", L"font_name", L"Segoe UI", hg_g_font_name, 64, hg_g_config_path);
    if (wcslen(hg_g_font_name) == 0) {
        wcscpy(hg_g_font_name, L"Segoe UI");
    }
    WritePrivateProfileStringW(L"etc", L"font_name", hg_g_font_name, hg_g_config_path);
}

void load_hotkey_config() {
    /* 새로운 섹션과 키 이름으로 로드 시도 */
    hg_g_hotkey_modifiers = GetPrivateProfileIntW(L"hotkeys", L"global_focus_modifiers", 0, hg_g_config_path);
    hg_g_hotkey_key = GetPrivateProfileIntW(L"hotkeys", L"global_focus_key", 0, hg_g_config_path);

    /* 만약 값이 없거나 0이면(초기 상태), 디폴트 값을 설정하고 파일에 저장하여 자동 생성 효과 부여 */
    if (hg_g_hotkey_modifiers == 0 && hg_g_hotkey_key == 0) {
        hg_g_hotkey_modifiers = MOD_WIN | MOD_ALT;
        hg_g_hotkey_key = VK_SPACE;
        save_hotkey_config();
    }
}

void move_window_by_offset(HWND hwnd, int dx, int dy) {
    if (!IsWindow(hwnd)) return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc)) return;
    SetWindowPos(hwnd, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after movement */
    if (hwnd == hg_g_floater_wnd) save_config(L"floater", rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
    else if (hwnd == hg_g_taskbox_wnd) save_config(L"taskbox", rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
    else if (hwnd == hg_g_controlbox_wnd) save_config(L"controlbox", rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
}

void resize_window_by_offset(HWND hwnd, int dw, int dh) {
    if (!IsWindow(hwnd)) return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc)) return;

    if (hwnd == hg_g_controlbox_wnd) {
        int new_w = (rc.right - rc.left) + dw;
        int new_h = get_controlbox_required_height();
        if (new_w < SC(HG_CONTROLBOX_MIN_WIDTH)) new_w = SC(HG_CONTROLBOX_MIN_WIDTH);
        if (new_w > SC(HG_CONTROLBOX_MAX_WIDTH)) new_w = SC(HG_CONTROLBOX_MAX_WIDTH);
        SetWindowPos(hwnd, NULL, 0, 0, new_w, new_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        save_config(L"controlbox", rc.left, rc.top, new_w, new_h);
        return;
    }

    int new_w = (rc.right - rc.left) + dw;
    int new_h = (rc.bottom - rc.top) + dh;
    if (new_w < SC(HG_MIN_WINDOW_WIDTH)) new_w = SC(HG_MIN_WINDOW_WIDTH);
    if (new_h < SC(HG_MIN_WINDOW_HEIGHT)) new_h = SC(HG_MIN_WINDOW_HEIGHT);
    SetWindowPos(hwnd, NULL, 0, 0, new_w, new_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after resizing */
    if (hwnd == hg_g_floater_wnd) save_config(L"floater", rc.left, rc.top, new_w, new_h);
    else if (hwnd == hg_g_taskbox_wnd) save_config(L"taskbox", rc.left, rc.top, new_w, new_h);
}

static int get_controlbox_required_height(void) {
    int border = SC(HG_BORDER_THICKNESS);
    int label_height = SC(18);

    if (hg_g_main_font) {
        HDC hdc = GetDC(NULL);
        if (hdc) {
            HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
            TEXTMETRICW tm = {0};
            if (GetTextMetricsW(hdc, &tm)) {
                label_height = tm.tmHeight + tm.tmExternalLeading + SC(4);
            }
            if (old_font) SelectObject(hdc, old_font);
            ReleaseDC(NULL, hdc);
        }
    }

    return border * 2 + label_height + SC(HG_CONTROLBOX_LABEL_GAP) + SC(HG_CONTROLBOX_TRACKBAR_HEIGHT) + SC(8);
}

static void controlbox_sync_volume_ui(void) {
    if (!hg_g_controlbox_wnd || !IsWindow(hg_g_controlbox_wnd)) return;

    int cur_vol = get_system_volume();
    WCHAR vol_str[64];
    StringCchPrintfW(vol_str, 64, L"System Volume: %d%%", cur_vol);

    if (hg_g_controlbox_value_wnd && IsWindow(hg_g_controlbox_value_wnd)) {
        SetWindowTextW(hg_g_controlbox_value_wnd, vol_str);
    }
    if (hg_g_controlbox_slider_wnd && IsWindow(hg_g_controlbox_slider_wnd)) {
        SendMessageW(hg_g_controlbox_slider_wnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)cur_vol);
    }
}

static void update_controlbox_layout(HWND hwnd) {
    if (!IsWindow(hwnd)) return;

    RECT rc = {0};
    GetClientRect(hwnd, &rc);
    int border = SC(HG_BORDER_THICKNESS);
    int label_height = SC(18);

    if (hg_g_main_font) {
        HDC hdc = GetDC(hwnd);
        if (hdc) {
            HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
            TEXTMETRICW tm = {0};
            if (GetTextMetricsW(hdc, &tm)) {
                label_height = tm.tmHeight + tm.tmExternalLeading + SC(4);
            }
            if (old_font) SelectObject(hdc, old_font);
            ReleaseDC(hwnd, hdc);
        }
    }

    int slider_height = SC(HG_CONTROLBOX_TRACKBAR_HEIGHT);
    int required_height = border * 2 + label_height + SC(HG_CONTROLBOX_LABEL_GAP) + slider_height + SC(8);
    int current_height = rc.bottom;
    int current_width = rc.right;

    if (current_height != required_height && !hg_g_in_sizemove) {
        SetWindowPos(hwnd, NULL, 0, 0, current_width, required_height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return;
    }

    int inner_w = current_width - border * 2;
    if (inner_w < 0) inner_w = 0;

    if (hg_g_controlbox_value_wnd && IsWindow(hg_g_controlbox_value_wnd)) {
        MoveWindow(hg_g_controlbox_value_wnd, border, border, inner_w, label_height, TRUE);
    }
    if (hg_g_controlbox_slider_wnd && IsWindow(hg_g_controlbox_slider_wnd)) {
        MoveWindow(hg_g_controlbox_slider_wnd, border, border + label_height + SC(HG_CONTROLBOX_LABEL_GAP), inner_w, slider_height, TRUE);
    }
}

static void show_controlbox_window(void) {
    int x = 240, y = 240, w = SC(HG_CONTROLBOX_DEFAULT_WIDTH), h = get_controlbox_required_height();
    load_config(L"controlbox", &x, &y, &w, &h, 240, 240, SC(HG_CONTROLBOX_DEFAULT_WIDTH), h);
    if (w < SC(HG_CONTROLBOX_MIN_WIDTH)) w = SC(HG_CONTROLBOX_MIN_WIDTH);
    if (w > SC(HG_CONTROLBOX_MAX_WIDTH)) w = SC(HG_CONTROLBOX_MAX_WIDTH);
    h = get_controlbox_required_height();

    if (!IsWindow(hg_g_controlbox_wnd)) {
        hg_g_controlbox_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            HG_CLASS_CONTROLBOX, L"controlbox", WS_POPUP,
            x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (!hg_g_controlbox_wnd) return;
    }

    SetWindowPos(hg_g_controlbox_wnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ShowWindow(hg_g_controlbox_wnd, SW_SHOWNORMAL);
    SetForegroundWindow(hg_g_controlbox_wnd);
    update_controlbox_layout(hg_g_controlbox_wnd);
    controlbox_sync_volume_ui();
}

static void close_about_window(HWND hwnd) {
    if (hg_g_about_wnd == hwnd) {
        hg_g_about_wnd = NULL;
    }
    if (hwnd && IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
}

static void disable_window_ime(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    HIMC himc = ImmGetContext(hwnd);
    if (!himc) {
        return;
    }

    ImmSetOpenStatus(himc, FALSE);
    ImmReleaseContext(hwnd, himc);
}

static void show_about_window(void) {
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        ShowWindow(hg_g_about_wnd, SW_SHOWNORMAL);
        SetForegroundWindow(hg_g_about_wnd);
        return;
    }

    hg_g_about_wnd = CreateWindowExW(0, HG_CLASS_ABOUT, L"about hgfloater",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SC(400), SC(300),
        NULL, NULL, GetModuleHandle(NULL), NULL);
    if (hg_g_about_wnd) {
        ShowWindow(hg_g_about_wnd, SW_SHOW);
    }
}

static void close_monitor_window(int idx) {
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return;
    }

    hg_g_monitors[idx].active = FALSE;
    if (hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        DestroyWindow(hg_g_monitors[idx].hwnd);
    }
    hg_g_monitors[idx].hwnd = NULL;
}

static BOOL open_monitor_window(int idx) {
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return FALSE;
    }

    if (hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        ShowWindow(hg_g_monitors[idx].hwnd, SW_SHOWNORMAL);
        SetForegroundWindow(hg_g_monitors[idx].hwnd);
        return TRUE;
    }

    hg_g_monitors[idx].active = TRUE;

    int x, y, w, h;
    RECT m_rc = hg_g_monitors[idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;

    int def_w = 640;
    int def_h = (mw > 0) ? (def_w * mh / mw) : 480;

    WCHAR key_x[64], key_y[64], key_w[64], key_h[64], key_name[64];
    StringCchPrintfW(key_x, 64, L"monitor%d_x", idx + 1);
    StringCchPrintfW(key_y, 64, L"monitor%d_y", idx + 1);
    StringCchPrintfW(key_w, 64, L"monitor%d_w", idx + 1);
    StringCchPrintfW(key_h, 64, L"monitor%d_h", idx + 1);
    StringCchPrintfW(key_name, 64, L"monitor%d_name", idx + 1);
    WritePrivateProfileStringW(L"monitor", key_name, hg_g_monitors[idx].name, hg_g_config_path);

    x = (int)GetPrivateProfileIntW(L"monitor", key_x, 100, hg_g_config_path);
    y = (int)GetPrivateProfileIntW(L"monitor", key_y, 100, hg_g_config_path);
    w = (int)GetPrivateProfileIntW(L"monitor", key_w, def_w, hg_g_config_path);
    h = (int)GetPrivateProfileIntW(L"monitor", key_h, def_h, hg_g_config_path);

    HWND mwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_NOACTIVATE, HG_CLASS_MONITOR, hg_g_monitors[idx].name,
        WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (mwnd) {
        apply_dwm_attributes(mwnd);
        hg_g_monitors[idx].hwnd = mwnd;

        if (hg_g_tooltip_wnd) {
            TOOLINFOW ti = { 0 };
            ti.cbSize = TOOLINFO_V1_SIZE;
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = mwnd;
            ti.uId = (UINT_PTR)mwnd;
            ti.lpszText = hg_g_monitors[idx].name;
            SendMessageW(hg_g_tooltip_wnd, TTM_ADDTOOLW, 0, (LPARAM)&ti);
        }
        return TRUE;
    }

    hg_g_monitors[idx].active = FALSE;
    return FALSE;
}

static void toggle_monitor_window(int idx) {
    if (idx < 0 || idx >= hg_g_monitor_count) {
        return;
    }

    if (hg_g_monitors[idx].active && hg_g_monitors[idx].hwnd && IsWindow(hg_g_monitors[idx].hwnd)) {
        close_monitor_window(idx);
    } else {
        open_monitor_window(idx);
    }
}

void update_size(int delta) {
    int old_size = ABS(hg_g_current_font_size);
    if (old_size < SC(16)) old_size = SC(16);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(16)) new_size = SC(16);
    if (new_size > SC(128)) new_size = SC(128);
    hg_g_current_font_size = -new_size;
    save_taskbox_font_config();
    if (hg_g_toolbar_btn_font) { DeleteObject(hg_g_toolbar_btn_font); hg_g_toolbar_btn_font = NULL; }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        RECT rc;
        GetWindowRect(hg_g_taskbox_wnd, &rc);
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        if (tb_width <= 0) tb_width = 1;

        int cols = get_items_per_row(tb_width, old_size);
        if (cols <= 0) cols = 1;

        /* Calculate exact width required for the SAME number of columns but NEW icon size */
        int exact_tb_width = (cols - 1) * (new_size + SC(15)) + new_size + SC(20);
        int new_w = exact_tb_width + border * 2;

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        load_shortcuts();
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }
    refresh_window_list(TRUE);
}

void update_edit_font_size(int delta) {
    int old_size = ABS(hg_g_edit_font_size);
    if (old_size < SC(12)) old_size = SC(12);

    int new_size = old_size + (delta > 0 ? SC(2) : -SC(2));
    if (new_size < SC(12)) new_size = SC(12);
    if (new_size > SC(128)) new_size = SC(128);
    hg_g_edit_font_size = -new_size;
    save_taskbox_font_config();

    if (hg_g_main_font && hg_g_main_font != GetStockObject(DEFAULT_GUI_FONT)) {
        DeleteObject(hg_g_main_font);
        hg_g_main_font = NULL;
    }

    hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
    if (!hg_g_main_font) hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    if (hg_g_edit_msg_wnd) {
        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
        InvalidateRect(hg_g_edit_msg_wnd, NULL, TRUE);
    }

    if (hg_g_tooltip_wnd) {
        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        update_layout(hg_g_taskbox_wnd);
        InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
    }
    if (hg_g_controlbox_wnd && IsWindow(hg_g_controlbox_wnd)) {
        update_controlbox_layout(hg_g_controlbox_wnd);
        InvalidateRect(hg_g_controlbox_wnd, NULL, TRUE);
    }

    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hwnd && IsWindow(hg_g_monitors[i].hwnd)) {
            HWND edit_wnd = GetDlgItem(hg_g_monitors[i].hwnd, 104);
            if (edit_wnd) {
                SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                /* Trigger WM_SIZE to recalculate edit box height */
                RECT rc;
                GetClientRect(hg_g_monitors[i].hwnd, &rc);
                SendMessageW(hg_g_monitors[i].hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
                InvalidateRect(hg_g_monitors[i].hwnd, NULL, TRUE);
            }
        }
    }
}

void update_floater_font_size(int delta) {
    int new_size = hg_g_floater_font_size + (delta > 0 ? 2 : -2);
    if (new_size < HG_FLOATER_MIN_FONT_SIZE) new_size = HG_FLOATER_MIN_FONT_SIZE;
    if (new_size > HG_FLOATER_MAX_FONT_SIZE) new_size = HG_FLOATER_MAX_FONT_SIZE;
    if (hg_g_floater_font_size != new_size) {
        hg_g_floater_font_size = new_size;
        if (hg_g_floater_time_font) { DeleteObject(hg_g_floater_time_font); hg_g_floater_time_font = NULL; }
        if (hg_g_floater_date_font) { DeleteObject(hg_g_floater_date_font); hg_g_floater_date_font = NULL; }
        update_floater_layout(hg_g_floater_wnd);
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        save_floater_font_config();
    }
}

void update_floater_alpha(int delta) {
    int new_alpha = (int)hg_g_floater_alpha + (delta > 0 ? 15 : -15);
    if (new_alpha > HG_MAX_ALPHA) new_alpha = HG_MAX_ALPHA;
    if (new_alpha < HG_MIN_ALPHA) new_alpha = HG_MIN_ALPHA;
    hg_g_floater_alpha = (BYTE)new_alpha;
    SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
}

void update_taskbox_alpha(int delta) {
    int new_alpha = (int)hg_g_taskbox_alpha + (delta > 0 ? 15 : -15);
    if (new_alpha > HG_MAX_ALPHA) new_alpha = HG_MAX_ALPHA;
    if (new_alpha < HG_MIN_ALPHA) new_alpha = HG_MIN_ALPHA;
    hg_g_taskbox_alpha = (BYTE)new_alpha;
    SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
}

void save_alpha_config() {
    WCHAR buf[32];
    hellgates_wsprintf(buf, 32, L"%u", (UINT)hg_g_floater_alpha);
    WritePrivateProfileStringW(L"floater", L"alpha", buf, hg_g_config_path);
    hellgates_wsprintf(buf, 32, L"%u", (UINT)hg_g_taskbox_alpha);
    WritePrivateProfileStringW(L"taskbox", L"alpha", buf, hg_g_config_path);
}

void hide_taskbox(HWND hwnd) {
    if (hwnd == NULL) return;
    ShowWindow(hwnd, SW_HIDE);
    load_shortcuts();
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
    if (hg_g_floater_wnd) SetForegroundWindow(hg_g_floater_wnd);
}

void update_floater_layout(HWND hwnd) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR time_str[16], date_str[32];
    hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
    const WCHAR* months[] = { L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
    const WCHAR* days[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
    hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

    int time_size = hg_g_floater_font_size;
    int date_size = hg_g_floater_font_size * 16 / 28;
    if (date_size < 10) date_size = 10;
    if (!hg_g_floater_time_font) hg_g_floater_time_font = CreateFontW(SC(time_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    if (!hg_g_floater_date_font) hg_g_floater_date_font = CreateFontW(SC(date_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    // hg_g_floater_time_font가 NULL이면 NULL을 SelectObject에 전달 → UB
    if (!hg_g_floater_time_font || !hg_g_floater_date_font) return;  // 안전 가드 추가

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    SIZE sz_time = {0}, sz_date = {0};
    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_floater_time_font);
    GetTextExtentPoint32W(hdc, time_str, (int)lstrlenW(time_str), &sz_time);
    SelectObject(hdc, hg_g_floater_date_font);
    GetTextExtentPoint32W(hdc, date_str, (int)lstrlenW(date_str), &sz_date);
    SelectObject(hdc, old_font);

    int pen_width = SC(HG_BORDER_THICKNESS);
    int padding_x = pen_width + SC(20);
    int padding_y = pen_width + SC(10);
    int width = (sz_time.cx > sz_date.cx ? sz_time.cx : sz_date.cx) + padding_x;
    int height = sz_time.cy + sz_date.cy + padding_y;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.right - rc.left != width || rc.bottom - rc.top != height) {
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* 폰트 삭제 제거 (정적 캐시 사용) */
    ReleaseDC(hwnd, hdc);
}

/* 창이 화면을 벗어났는지 확인하고 0,0으로 복구 */
void ensure_window_visible(HWND hwnd, const WCHAR* section) {
    if (!IsWindow(hwnd) || !section) return;
    RECT rc = {0};
    if (!GetWindowRect(hwnd, &rc)) return;

    HMONITOR monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    SecureZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor_handle, &mi)) {
        /* 창의 일부라도 작업 영역(Work Area)을 벗어나면 0,0으로 이동 */
        if (rc.left < mi.rcWork.left || rc.top < mi.rcWork.top ||
            rc.right > mi.rcWork.right || rc.bottom > mi.rcWork.bottom) {
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            save_config(section, 0, 0, rc.right - rc.left, rc.bottom - rc.top);
        }
    }
}

static LRESULT floater_controller_on_create(HWND hwnd) {
            hg_g_shellhook_msg = RegisterWindowMessageW(L"SHELLHOOK");
            RegisterShellHookWindow(hwnd);

            SetLayeredWindowAttributes(hwnd, 0, hg_g_floater_alpha, LWA_ALPHA);

            /* 창 모서리 및 DWM 속성 설정 */
            apply_dwm_attributes(hwnd);

            update_floater_layout(hwnd);
            SetTimer(hwnd, 1, 1000, NULL);
            return 0;
}

static LRESULT floater_controller_on_paint(HWND hwnd) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);

            if (rc.right > 0 && rc.bottom > 0) {
                HDC mem_dc = CreateCompatibleDC(hdc);
                HBITMAP mem_bm = NULL, old_bm = NULL;

                if (mem_dc) {
                    mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                    if (mem_bm) {
                        old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
                        if (!old_bm) {
                            DeleteObject(mem_bm);
                            DeleteDC(mem_dc);
                            EndPaint(hwnd, &ps);
                            return 0;
                        }

                        int time_size = hg_g_floater_font_size;
                        int date_size = hg_g_floater_font_size * 16 / 28;
                        if (date_size < 10) date_size = 10;
                        if (!hg_g_floater_time_font) hg_g_floater_time_font = CreateFontW(SC(time_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
                        if (!hg_g_floater_date_font) hg_g_floater_date_font = CreateFontW(SC(date_size), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);

                        if (hg_g_floater_time_font && hg_g_floater_date_font) {
                            /* 하이라이트 효과 (깜빡임) - 배경색 변경 */
                            COLORREF bg_color = HG_COLOR_BG_DEFAULT;
                            if (hg_g_floater_highlight_ticks > 0 && (hg_g_floater_highlight_ticks % 2 != 0)) {
                                bg_color = HG_COLOR_BG_FLASH; /* 밝은 노란색 */
                            }
                            HBRUSH bg_brush = CreateSolidBrush(bg_color);
                            int pen_width = SC(HG_BORDER_THICKNESS);

                            COLORREF border_color = HG_COLOR_BG_TOOLBAR;
                            HPEN border_pen = CreatePen(PS_SOLID, pen_width, border_color);

                            HBRUSH old_brush = NULL;
                            HPEN old_pen = NULL;
                            if (bg_brush) old_brush = (HBRUSH)SelectObject(mem_dc, bg_brush);
                            if (border_pen) old_pen = (HPEN)SelectObject(mem_dc, border_pen);

                            /* 백그라운드 지우기 */
                            FillRect(mem_dc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH)); // Not actually needed but good practice before drawing

                            /* 외곽선 두께를 고려하여 내부로 수축 */
                            RECT draw_rc = rc;
                            InflateRect(&draw_rc, -pen_width/2, -pen_width/2);
                            if (bg_brush && border_pen) {
                                Rectangle(mem_dc, draw_rc.left, draw_rc.top, draw_rc.right, draw_rc.bottom);
                            }

                            /* 텍스트 그리기 */
                            SYSTEMTIME st;
                            GetLocalTime(&st);
                            WCHAR time_str[16], date_str[32];
                            hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
                            const WCHAR* months[] = { L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
                            const WCHAR* days[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
                            hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

                            SetBkMode(mem_dc, TRANSPARENT);
                            SetTextColor(mem_dc, HG_COLOR_TEXT_DEFAULT);

                            /* 텍스트 크기 측정하여 수직 중앙 정렬 */
                            SIZE sz_time = {0}, sz_date = {0};
                            HFONT old_font_in_paint = (HFONT)SelectObject(mem_dc, hg_g_floater_time_font);
                            GetTextExtentPoint32W(mem_dc, time_str, (int)lstrlenW(time_str), &sz_time);
                            SelectObject(mem_dc, hg_g_floater_date_font);
                            GetTextExtentPoint32W(mem_dc, date_str, (int)lstrlenW(date_str), &sz_date);

                            int total_text_height = sz_time.cy + sz_date.cy;
                            int start_y = (rc.bottom - rc.top - total_text_height) / 2;
                            if (start_y < 0) start_y = 0;

                            RECT time_rc = rc;
                            time_rc.top = start_y;
                            time_rc.bottom = time_rc.top + sz_time.cy;

                            RECT date_rc = rc;
                            date_rc.top = time_rc.bottom;
                            date_rc.bottom = date_rc.top + sz_date.cy;

                            SelectObject(mem_dc, hg_g_floater_time_font);
                            DrawTextW(mem_dc, time_str, -1, &time_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                            SelectObject(mem_dc, hg_g_floater_date_font);
                            DrawTextW(mem_dc, date_str, -1, &date_rc, DT_CENTER | DT_TOP | DT_SINGLELINE);

                            SelectObject(mem_dc, old_font_in_paint);

                            if (old_brush) SelectObject(mem_dc, old_brush);
                            if (old_pen) SelectObject(mem_dc, old_pen);
                            if (bg_brush) DeleteObject(bg_brush);
                            if (border_pen) DeleteObject(border_pen);
                        }

                        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
                        SelectObject(mem_dc, old_bm);
                        DeleteObject(mem_bm);
                    }
                    DeleteDC(mem_dc);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
}

static LRESULT floater_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    int dx = 0, dy = 0;
    int move_step = SC(20);
    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);

    /* Alt + 방향키/hjkl/wasd: 현재 창 이동 (플로팅 박스) */
    if (is_alt) {
        if (w_param == VK_LEFT || w_param == 'H' || w_param == 'A') dx = -move_step;
        else if (w_param == VK_RIGHT || w_param == 'L' || w_param == 'D') dx = move_step;
        else if (w_param == VK_UP || w_param == 'K' || w_param == 'W') dy = -move_step;
        else if (w_param == VK_DOWN || w_param == 'J' || w_param == 'S') dy = move_step;

        if (dx != 0 || dy != 0) {
            move_window_by_offset(hwnd, dx, dy);
            /* 이동 후에도 최소한 일부는 화면에 보이도록 보호 */
            ensure_window_visible(hwnd, L"floater");
            return 0;
        }

        /* Alt + +/-: 투명도 조절 */
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_alpha(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_alpha(-1);
            return 0;
        }
    }

    if (is_ctrl) {
        /* Ctrl + +/-: 플로터 글꼴 크기 조절 */
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_font_size(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_font_size(-1);
            return 0;
        }
    }
    /* F2: Floater Context Menu */
    if (w_param == VK_F2) {
        SendMessageW(hwnd, WM_RBUTTONUP, 0, 0);
        return 0;
    }
    /* Esc: Taskbox가 숨겨진 상태면 Floater 위치를 (0,0)으로 리셋 */
    if (w_param == VK_ESCAPE) {
        if (hg_g_taskbox_wnd && !IsWindowVisible(hg_g_taskbox_wnd)) {
            RECT rc_reset = {0};
            if (GetWindowRect(hwnd, &rc_reset)) {
                SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                save_config(L"floater", 0, 0, rc_reset.right - rc_reset.left, rc_reset.bottom - rc_reset.top);
                return 0;
            }
        }
    }
    if (msg == WM_SYSKEYDOWN) return DefWindowProcW(hwnd, msg, w_param, l_param);
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

static LRESULT floater_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param) {
            if (LOWORD(w_param) == HG_IDM_CLOSE_APP) {
                DestroyWindow(hwnd);
            } else if (LOWORD(w_param) >= HG_IDM_MONITOR_BASE && LOWORD(w_param) < HG_IDM_MONITOR_BASE + HG_MAX_MONITORS) {
                int idx = LOWORD(w_param) - HG_IDM_MONITOR_BASE;
                if (idx >= 0 && idx < hg_g_monitor_count) {
                    toggle_monitor_window(idx);
                }
            } else if (LOWORD(w_param) >= HG_IDM_AUDIO_DEVICE_BASE && LOWORD(w_param) < HG_IDM_AUDIO_DEVICE_BASE + HG_MAX_AUDIO_DEVICES) {
                int idx = LOWORD(w_param) - HG_IDM_AUDIO_DEVICE_BASE;
                if (idx >= 0 && idx < hg_g_audio_device_count) {
                    if (set_default_audio_device(hg_g_audio_devices[idx].id)) {
                        update_audio_device_list();
                    }
                }
            } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_0) {
                set_system_volume(0);
            } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_25) {
                set_system_volume(25);
            } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_50) {
                set_system_volume(50);
            } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_75) {
                set_system_volume(75);
            } else if (LOWORD(w_param) == HG_IDM_VOLUME_SET_100) {
                set_system_volume(100);
            } else if (LOWORD(w_param) == HG_IDM_OPEN_CONTROLBOX) {
                show_controlbox_window();
            } else if (LOWORD(w_param) == HG_IDM_ABOUT) {
                show_about_window();
            } else if (LOWORD(w_param) == HG_IDM_FONT_UP) {
                update_floater_font_size(1);
            } else if (LOWORD(w_param) == HG_IDM_FONT_DOWN) {
                update_floater_font_size(-1);
            } else if (LOWORD(w_param) == HG_IDM_POWER_OFF) {
                HWND h_shell = FindWindowW(L"Shell_TrayWnd", NULL);
                if (h_shell) PostMessageW(h_shell, WM_COMMAND, 506, 0);
            } else if (LOWORD(w_param) == HG_IDM_RESET_ALL) {
                /* Reset Floater */
                hg_g_floater_alpha = 204;
                SetLayeredWindowAttributes(hwnd, 0, hg_g_floater_alpha, LWA_ALPHA);
                save_alpha_config();

                hg_g_floater_font_size = 28;
                if (hg_g_floater_time_font) { DeleteObject(hg_g_floater_time_font); hg_g_floater_time_font = NULL; }
                if (hg_g_floater_date_font) { DeleteObject(hg_g_floater_date_font); hg_g_floater_date_font = NULL; }
                save_floater_font_config();

                SetWindowPos(hwnd, NULL, 100, 100, SC(80), SC(55), SWP_NOZORDER | SWP_NOACTIVATE);
                save_config(L"floater", 100, 100, SC(80), SC(55));
                update_floater_layout(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);

                /* Reset Taskbox */
                if (hg_g_taskbox_wnd) {
                    hg_g_taskbox_alpha = 204;
                    SetLayeredWindowAttributes(hg_g_taskbox_wnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha,
                                               LWA_COLORKEY | LWA_ALPHA);

                    int target_icon_size = SC(22);
                    hg_g_current_font_size = -target_icon_size;

                    hg_g_edit_font_size = -SC(16);
                    save_taskbox_font_config();

                    if (hg_g_toolbar_btn_font) {
                        DeleteObject(hg_g_toolbar_btn_font);
                        hg_g_toolbar_btn_font = NULL;
                    }
                    if (hg_g_main_font) {
                        DeleteObject(hg_g_main_font);
                        hg_g_main_font = NULL;
                    }

                    SetWindowPos(hg_g_taskbox_wnd, NULL, 200, 200,
                                 SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT),
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                    save_config(L"taskbox", 200, 200, SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT));

                    update_layout(hg_g_taskbox_wnd);
                    if (hg_g_controlbox_wnd && IsWindow(hg_g_controlbox_wnd)) {
                        update_controlbox_layout(hg_g_controlbox_wnd);
                        InvalidateRect(hg_g_controlbox_wnd, NULL, TRUE);
                    }

                    if (hg_g_edit_msg_wnd) {
                        if (!hg_g_main_font) {
                            hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
                            if (!hg_g_main_font) hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                        }
                        SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                    }

                    if (hg_g_tooltip_wnd && hg_g_main_font) {
                        SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                    }

                    if (hg_g_controlbox_wnd && IsWindow(hg_g_controlbox_wnd)) {
                        update_controlbox_layout(hg_g_controlbox_wnd);
                        InvalidateRect(hg_g_controlbox_wnd, NULL, TRUE);
                    }

                    InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);

                    /* Re-snap Taskbox */
                    RECT rc = {0};
                    GetWindowRect(hg_g_taskbox_wnd, &rc);

                    int border = SC(HG_BORDER_THICKNESS);
                    int tb_width = (rc.right - rc.left) - border * 2;
                    if (tb_width <= 0) tb_width = 1;

                    int cols = get_items_per_row(tb_width, target_icon_size);
                    if (cols <= 0) cols = 1;

                    int exact_tb_width =
                        (cols - 1) * (target_icon_size + SC(15)) +
                        target_icon_size +
                        SC(20);
                    int new_w = exact_tb_width + border * 2;

                    if ((rc.right - rc.left) != new_w) {
                        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top,
                                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                        GetWindowRect(hg_g_taskbox_wnd, &rc);
                        save_config(L"taskbox", rc.left, rc.top,
                                    rc.right - rc.left, rc.bottom - rc.top);
                    }
                } /* if (hg_g_taskbox_wnd) */

                /* Arrange Monitor Windows */
                update_monitor_enum();
                int mx = SC(300), my = SC(300);
                for (int i = 0; i < hg_g_monitor_count; i++) {
                    if (hg_g_monitors[i].active && hg_g_monitors[i].hwnd) {
                        int m_w = SC(640);
                        int mmw = hg_g_monitors[i].rcMonitor.right - hg_g_monitors[i].rcMonitor.left;
                        int mmh = hg_g_monitors[i].rcMonitor.bottom - hg_g_monitors[i].rcMonitor.top;
                        int m_h = (mmw > 0) ? (m_w * mmh / mmw) : SC(480);

                        SetWindowPos(hg_g_monitors[i].hwnd, NULL, mx, my, m_w, m_h, SWP_NOZORDER | SWP_NOACTIVATE);

                        WCHAR key_x[64], key_y[64], key_w[64], key_h[64], key_name[64];
                        StringCchPrintfW(key_x, 64, L"monitor%d_x", i + 1);
                        StringCchPrintfW(key_y, 64, L"monitor%d_y", i + 1);
                        StringCchPrintfW(key_w, 64, L"monitor%d_w", i + 1);
                        StringCchPrintfW(key_h, 64, L"monitor%d_h", i + 1);
                        StringCchPrintfW(key_name, 64, L"monitor%d_name", i + 1);
                        WritePrivateProfileStringW(L"monitor", key_name, hg_g_monitors[i].name, hg_g_config_path);

                        WCHAR buf[32];
                        hellgates_wsprintf(buf, 32, L"%d", mx);
                        WritePrivateProfileStringW(L"monitor", key_x, buf, hg_g_config_path);
                        hellgates_wsprintf(buf, 32, L"%d", my);
                        WritePrivateProfileStringW(L"monitor", key_y, buf, hg_g_config_path);
                        hellgates_wsprintf(buf, 32, L"%d", m_w);
                        WritePrivateProfileStringW(L"monitor", key_w, buf, hg_g_config_path);
                        hellgates_wsprintf(buf, 32, L"%d", m_h);
                        WritePrivateProfileStringW(L"monitor", key_h, buf, hg_g_config_path);

                        mx += SC(50); my += SC(50);
                    }
                }

            } /* else if (LOWORD(w_param) == HG_IDM_RESET_ALL) */
    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}

static LRESULT floater_controller_on_destroy(HWND hwnd) {
            DeregisterShellHookWindow(hwnd);
            KillTimer(hwnd, 1);
            KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
                DestroyWindow(hg_g_about_wnd);
                hg_g_about_wnd = NULL;
            }
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
                DestroyWindow(hg_g_taskbox_wnd);
                hg_g_taskbox_wnd = NULL;
            }
            if (hg_g_hotkey_registered) {
                UnregisterHotKey(hwnd, 1);
                hg_g_hotkey_registered = FALSE;
            }
            if (hg_g_floater_time_font) { DeleteObject(hg_g_floater_time_font); hg_g_floater_time_font = NULL; }
            if (hg_g_floater_date_font) { DeleteObject(hg_g_floater_date_font); hg_g_floater_date_font = NULL; }
            hg_g_floater_wnd = NULL;
            PostQuitMessage(0);
            return 0;
}

LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    static BOOL floater_moved = FALSE;
    static int initial_floater_font_size = 0;
    switch (msg) {
        case WM_DISPLAYCHANGE: {
            update_monitor_enum();
            return 0;
        }
        case WM_MOUSEACTIVATE: {
            /* 태스크 박스가 떠 있을 때는 클릭해도 포커스를 뺏어오지 않음 (드래그/우클릭 가능하게) */
            if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) return MA_NOACTIVATE;
            return MA_ACTIVATE;
        }
        case WM_ACTIVATE: {
            if (LOWORD(w_param) != WA_INACTIVE) {
                if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    SetForegroundWindow(hg_g_taskbox_wnd);
                }
            }
            return 0;
        }
        case WM_CREATE:
            return floater_controller_on_create(hwnd);
        case WM_PAINT:
            return floater_controller_on_paint(hwnd);
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            return floater_controller_on_keydown(hwnd, msg, w_param, l_param);
        case WM_LBUTTONDOWN: {
            floater_moved = FALSE;
            hg_g_drag_start_pt.x = GET_X_LPARAM(l_param);
            hg_g_drag_start_pt.y = GET_Y_LPARAM(l_param);
            if (GetKeyState(VK_CONTROL) < 0) {
                initial_floater_font_size = hg_g_floater_font_size;
            }
            SetCapture(hwnd);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (w_param & MK_LBUTTON && GetCapture() == hwnd) {
                POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
                if (GetKeyState(VK_CONTROL) < 0) {
                    int delta = (pt.x - hg_g_drag_start_pt.x) / 5; /* 1 size unit per 5 pixels */
                    int new_size = initial_floater_font_size + delta;
                    if (new_size < HG_FLOATER_MIN_FONT_SIZE) new_size = HG_FLOATER_MIN_FONT_SIZE;
                    if (new_size > HG_FLOATER_MAX_FONT_SIZE) new_size = HG_FLOATER_MAX_FONT_SIZE;

                    if (hg_g_floater_font_size != new_size) {
                        hg_g_floater_font_size = new_size;
                        floater_moved = TRUE;
                        if (hg_g_floater_time_font) { DeleteObject(hg_g_floater_time_font); hg_g_floater_time_font = NULL; }
                        if (hg_g_floater_date_font) { DeleteObject(hg_g_floater_date_font); hg_g_floater_date_font = NULL; }
                        update_floater_layout(hwnd);
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                } else {
                    if (ABS(pt.x - hg_g_drag_start_pt.x) > GetSystemMetrics(SM_CXDRAG) ||
                        ABS(pt.y - hg_g_drag_start_pt.y) > GetSystemMetrics(SM_CYDRAG)) {
                        floater_moved = TRUE;
                        ReleaseCapture();
                        SendMessageW(hwnd, WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
                    }
                }
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (GetCapture() == hwnd) {
                ReleaseCapture();
                BOOL was_moved = floater_moved;
                floater_moved = FALSE;
                if (was_moved) {
                    save_floater_font_config();
                } else {
                    /* 드래그가 아니었을 때만 토글 */
                    if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                        hide_taskbox(hg_g_taskbox_wnd);
                    } else if (hg_g_taskbox_wnd) {
                        refresh_window_list(TRUE);
                        ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                        SetForegroundWindow(hg_g_taskbox_wnd);
                        if (hg_g_edit_msg_wnd) append_message(L"RClick: Settings | Ctrl+Wheel: Size | Alt+Wheel: Alpha");
                    }
                }
            }
            return 0;
        }
        case WM_RBUTTONUP: {
            HMENU hMenu = CreatePopupMenu();
            if (!hMenu) return 0;

            /* Volume Controls */
            int cur_vol = get_system_volume();
            WCHAR vol_str[64];
            StringCchPrintfW(vol_str, 64, L"System Volume: %d%%", cur_vol);
            AppendMenuW(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, HG_IDM_VOLUME_PERCENT, vol_str);

            HMENU hVolMenu = CreatePopupMenu();
            AppendMenuW(hVolMenu, MF_STRING, HG_IDM_VOLUME_SET_0,   L"0%");
            AppendMenuW(hVolMenu, MF_STRING, HG_IDM_VOLUME_SET_25,  L"25%");
            AppendMenuW(hVolMenu, MF_STRING, HG_IDM_VOLUME_SET_50,  L"50%");
            AppendMenuW(hVolMenu, MF_STRING, HG_IDM_VOLUME_SET_75,  L"75%");
            AppendMenuW(hVolMenu, MF_STRING, HG_IDM_VOLUME_SET_100, L"100%");
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hVolMenu, L"Set Volume (&V)");
            AppendMenuW(hMenu, MF_STRING, HG_IDM_OPEN_CONTROLBOX, L"Open Controlbox (&C)");

            /* Audio Devices - Placeholder sub-menu */
            hg_g_h_audio_submenu = CreatePopupMenu();
            if (hg_g_h_audio_submenu) {
                /* Add a dummy item so the pop-up will trigger */
                AppendMenuW(hg_g_h_audio_submenu, MF_STRING | MF_GRAYED, 0, L"(Loading...)");
                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hg_g_h_audio_submenu, L"Audio Output (&O)");
            }

            /* Monitors Submenu */
            HMENU hMonMenu = CreatePopupMenu();
            update_monitor_enum();
            for (int i = 0; i < hg_g_monitor_count; i++) {
                UINT flags = MF_STRING;
                if (hg_g_monitors[i].active) flags |= MF_CHECKED;
                AppendMenuW(hMonMenu, flags, (UINT_PTR)(HG_IDM_MONITOR_BASE + i), hg_g_monitors[i].name);
            }
            if (hg_g_monitor_count > 0) {
                AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hMonMenu, L"Monitors (&M)");
            } else {
                DestroyMenu(hMonMenu);
            }

            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, HG_IDM_ABOUT, L"About (&A)");
            AppendMenuW(hMenu, MF_STRING, HG_IDM_POWER_OFF, L"System Shutdown(&S)");
            AppendMenuW(hMenu, MF_STRING, HG_IDM_CLOSE_APP, L"Exit (&X)");
            POINT pt;
            if (msg == WM_RBUTTONUP && l_param == 0) {
                /* Triggered by F2 */
                RECT rc_floater; GetWindowRect(hwnd, &rc_floater);
                pt.x = rc_floater.left; pt.y = rc_floater.top;
            } else {
                GetCursorPos(&pt);
            }
            TrackPopupMenuEx(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);
            DestroyMenu(hMenu);
            hg_g_h_audio_submenu = NULL;
            return 0;
        }
        case WM_INITMENUPOPUP: {
            if (hg_g_h_audio_submenu && (HMENU)w_param == hg_g_h_audio_submenu) {
                /* Clear placeholder items */
                while (GetMenuItemCount(hg_g_h_audio_submenu) > 0)
                    DeleteMenu(hg_g_h_audio_submenu, 0, MF_BYPOSITION);

                update_audio_device_list();
                if (hg_g_audio_device_count <= 0) {
                    AppendMenuW(hg_g_h_audio_submenu, MF_STRING | MF_DISABLED | MF_GRAYED, 0, L"No devices found");
                } else {
                    for (int i = 0; i < hg_g_audio_device_count; i++) {
                        UINT flags = MF_STRING;
                        if (hg_g_audio_devices[i].is_default) flags |= MF_CHECKED;
                        AppendMenuW(hg_g_h_audio_submenu, flags, (UINT_PTR)(HG_IDM_AUDIO_DEVICE_BASE + i), hg_g_audio_devices[i].name);
                    }
                }
            }
            return 0;
        }
        case WM_COMMAND:
            return floater_controller_on_command(hwnd, w_param, l_param);
        case WM_MOUSEWHEEL: {
            if (GetKeyState(VK_MENU) < 0) {
                short delta = (short)HIWORD(w_param);
                update_floater_alpha(delta > 0 ? 1 : -1);
            } else if (GetKeyState(VK_CONTROL) < 0) {
                short delta = (short)HIWORD(w_param);
                update_floater_font_size(delta > 0 ? 1 : -1);
            }
            return 0;
        }
        case WM_NCHITTEST: {
            return HTCLIENT; /* 직접 드래그 구현을 위해 HTCLIENT 반환 */
        }
        case WM_ENTERSIZEMOVE: { hg_g_in_sizemove = TRUE; return DefWindowProcW(hwnd, msg, w_param, l_param); } case WM_EXITSIZEMOVE: { hg_g_in_sizemove = FALSE;
            RECT rc = {0};
            GetWindowRect(hwnd, &rc);
            save_config(L"floater", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            return 0;
        }
        case WM_TIMER: {
            if (w_param == 1) {
                static SYSTEMTIME last_st = {0};
                SYSTEMTIME st;
                GetLocalTime(&st);
                if (st.wMinute != last_st.wMinute || st.wHour != last_st.wHour ||
                    st.wDay != last_st.wDay || st.wMonth != last_st.wMonth || st.wYear != last_st.wYear) {
                    last_st = st;
                    update_floater_layout(hwnd);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                /* Periodically checked */
                for (int i = 0; i < hg_g_monitor_count; i++) {
                    if (hg_g_monitors[i].active && hg_g_monitors[i].hwnd) {
                        InvalidateRect(hg_g_monitors[i].hwnd, NULL, FALSE);
                    }
                }
            } else if (w_param == HG_TIMER_HIGHLIGHT) {
                hg_g_floater_highlight_ticks--;
                if (hg_g_floater_highlight_ticks <= 0) {
                    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_HOTKEY: {
            if (w_param == 1) {
                /* 화면 이탈 여부 자동 체크 및 복구 (0,0) */
                ensure_window_visible(hg_g_floater_wnd, L"floater");
                ensure_window_visible(hg_g_taskbox_wnd, L"taskbox");

                /* 플로팅 박스와 태스크 박스를 최상위로 올림 */
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                /* 플로팅 박스 하이라이트 효과 시작 */
                hg_g_floater_highlight_ticks = HG_HIGHLIGHT_TICKS;
                if (!SetTimer(hwnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                    hg_g_floater_highlight_ticks = 0;
                }
                InvalidateRect(hwnd, NULL, FALSE);

                /* 태스크 박스 표시 및 최상위 포커스 */
                if (hg_g_taskbox_wnd) {
                    if (!IsWindowVisible(hg_g_taskbox_wnd)) {
                        refresh_window_list(TRUE);
                        ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                    } else {
                        refresh_window_list(FALSE);
                    }
                    SetWindowPos(hg_g_taskbox_wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetForegroundWindow(hg_g_taskbox_wnd);

                    /* 하이라이트 효과 */
                    hg_g_taskbox_highlight_ticks = HG_HIGHLIGHT_TICKS;
                    if (!SetTimer(hg_g_taskbox_wnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                        hg_g_taskbox_highlight_ticks = 0;
                    }
                    InvalidateRect(hg_g_taskbox_wnd, NULL, FALSE);

                    /* 태스크 리스트 첫 번째 아이콘에 포커스 (Unified Toolbar) */
                    hg_g_focus_area = 0;
                    SetFocus(hg_g_toolbar_wnd);
                    if (hg_g_window_count > 0) {
                        hg_g_toolbar_focus_index = 0;
                    }
                    update_focus_message(-2, -2);

                    InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
                } else {
                    /* 태스크 박스가 없거나 숨겨진 상태라면 플로팅 박스를 포커스 */
                    SetForegroundWindow(hwnd);
                }
            }
            return 0;
        }
        case WM_DESTROY:
            return floater_controller_on_destroy(hwnd);
        default: {
            if (hg_g_shellhook_msg && msg == hg_g_shellhook_msg) {
                if (w_param == HSHELL_REDRAW) {
                    HWND target_hwnd = (HWND)l_param;
                    BOOL found = FALSE;
                    for (int i = 0; i < hg_g_window_count; i++) {
                        if (hg_g_window_items[i].hwnd == target_hwnd) {
                            if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
                                DestroyIcon(hg_g_window_items[i].icon);
                            }
                            hg_g_window_items[i].icon = get_window_icon(target_hwnd, ABS(hg_g_current_font_size), &hg_g_window_items[i].own_icon);
                            found = TRUE;
                            break;
                        }
                    }
                    if (found && hg_g_toolbar_wnd && hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
                    }
                } else if (w_param == HSHELL_WINDOWCREATED || w_param == HSHELL_WINDOWDESTROYED || w_param == HSHELL_WINDOWREPLACED) {
                    /* Only trigger expensive refresh immediately if taskbox is visible, otherwise let the 1sec timer lazily handle it */
                    if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                        refresh_window_list(FALSE);
                    }
                }
                return 0;
            }
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

void activate_toolbar_item(int index) {
    if (index < 0) return;
    if (index == 0) {
        /* R button - handled by drag, no click action needed or could be a toggle?
           User said "이 버튼을 드래그하면 그것에 맞춰 창 크기를 변경하게 해 줘".
           Clicking it doesn't have a specified action. */
    } else if (index == 1) {
        /* Move handle: dragging logic is in toolbar_proc WM_LBUTTONDOWN. Clicking does nothing. */
    } else if (index == 2) {
        hide_taskbox(hg_g_taskbox_wnd);
    } else if (index == 3) {
        static BOOL is_desktop_shown = FALSE;
        is_desktop_shown = !is_desktop_shown;
        EnumWindows(minimize_restore_enum_proc, (LPARAM)is_desktop_shown);
    } else if (index == 4) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, L"Open Shortcuts Folder");
        AppendMenuW(hMenu, MF_STRING, 1002, L"Edit Configuration");
        AppendMenuW(hMenu, MF_STRING, 1003, L"Reset Settings");

        POINT pt;
        GetCursorPos(&pt);
        int cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hg_g_taskbox_wnd, NULL);
        DestroyMenu(hMenu);

        if (cmd == 1001) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts_path, NULL, NULL, SW_SHOWNORMAL);
        } else if (cmd == 1002) {
            ShellExecuteW(NULL, L"open", L"notepad.exe", hg_g_config_path, NULL, SW_SHOWNORMAL);
        } else if (cmd == 1003) {
            PostMessageW(hg_g_taskbox_wnd, WM_COMMAND, HG_IDM_RESET_ALL, 0);
        }
    } else {
        int s_idx = index - HG_NUM_BASIC_ICONS;
        if (s_idx >= 0 && s_idx < hg_g_shortcut_count) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts[s_idx].path, NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

void activate_taskbar_item(int index) {
    if (index < 0 || index >= hg_g_window_count) return;
    HWND target = hg_g_window_items[index].hwnd;
    if (IsWindow(target)) {
        if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    }
}

static LRESULT CALLBACK about_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_SETFOCUS) {
        disable_window_ime(hwnd);
    }
    if (msg == WM_KEYDOWN && w_param == VK_ESCAPE) {
        PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK about_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_CREATE: {
            apply_dwm_attributes(hwnd);
            HWND edit_wnd = CreateWindowExW(0, L"EDIT",
                HG_ABOUT_TEXT_W,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                SC(10), SC(10), SC(364), SC(240), hwnd, (HMENU)100, GetModuleHandle(NULL), NULL);
            if (edit_wnd) {
                SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                SetWindowSubclass(edit_wnd, about_edit_subclass_proc, 1, 0);
                disable_window_ime(edit_wnd);
            }
            return 0;
        }
        case WM_SIZE: {
            HWND edit_wnd = GetDlgItem(hwnd, 100);
            if (edit_wnd) {
                int w = (int)LOWORD(l_param) - SC(20);
                int h = (int)HIWORD(l_param) - SC(20);
                if (w < 0) w = 0;
                if (h < 0) h = 0;
                MoveWindow(edit_wnd, SC(10), SC(10), w, h, TRUE);
            }
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)w_param;
            SetTextColor(hdc, hg_g_color_scheme_selected.text);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, hg_g_color_scheme_selected.bg);
            return (LRESULT)GetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND);
        }
        case WM_KEYDOWN: {
            if (w_param == VK_ESCAPE) {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            return 0;
        }
        case WM_CLOSE: {
            close_about_window(hwnd);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

void update_focus_message(int override_type, int override_index) {
    if (override_type == -1 && override_index == -1) {
        return;
    }

    int type = (override_type == -2) ? hg_g_focus_area : override_type;
    int index = (override_index == -2) ? hg_g_toolbar_focus_index : override_index;

    if (type == 0) {
        if (index >= 0 && index < hg_g_window_count) {
            append_message(hg_g_window_items[index].title);
        }
    } else if (type == 1) {
        int total_items = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        if (index >= 0 && index < total_items) {
            if (index == 0) append_message(L"Drag to Resize Window");
            else if (index == 1) append_message(L"Drag to Move Window");
            else if (index == 2) append_message(L"Hide Dashboard");
            else if (index == 3) append_message(L"Show Desktop");
            else if (index == 4) append_message(L"Settings");
            else append_message(hg_g_shortcuts[index - HG_NUM_BASIC_ICONS].name);
        }
    }
}

int get_item_at_pt(POINT pt, int width, int height, int icon_size, int *out_type, int *out_index) {
    for(int i = 0; i < hg_g_window_count; i++) {
        RECT rc_item, rc_btn;
        get_toolbar_item_rect(0, i, width, height, icon_size, &rc_item);
        rc_btn = rc_item; InflateRect(&rc_btn, SC(4), SC(4));
        if (PtInRect(&rc_btn, pt)) {
            if (out_type) *out_type = 0;
            if (out_index) *out_index = i;
            return 1;
        }
    }
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    for(int i = 0; i < total_shortcuts; i++) {
        RECT rc_item, rc_btn;
        get_toolbar_item_rect(1, i, width, height, icon_size, &rc_item);
        rc_btn = rc_item; InflateRect(&rc_btn, SC(4), SC(4));
        if (PtInRect(&rc_btn, pt)) {
            if (out_type) *out_type = 1;
            if (out_index) *out_index = i;
            return 1;
        }
    }
    return 0;
}


static LRESULT toolbar_controller_on_paint(HWND hwnd, int hovered_type, int hovered_index, int pressed_type, int pressed_index, int *cached_icon_size) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HDC mem_dc = CreateCompatibleDC(hdc);
        HBITMAP mem_bm = NULL, old_bm = NULL;

        if (mem_dc) {
            mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            if (mem_bm) {
                old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
                if (!old_bm) {
                    DeleteObject(mem_bm);
                    DeleteDC(mem_dc);
                    EndPaint(hwnd, &ps);
                    return 0;
                }

                COLORREF bg_color = HG_COLOR_BG_TOOLBAR;
                if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
                    bg_color = HG_COLOR_BG_FLASH;
                }
                HBRUSH hbr_bg = CreateSolidBrush(bg_color);
                if (hbr_bg) { FillRect(mem_dc, &rc, hbr_bg); DeleteObject(hbr_bg); }

                int icon_size = ABS(hg_g_current_font_size);
                if (icon_size < SC(16)) icon_size = SC(16);

                if (icon_size != *cached_icon_size || !hg_g_toolbar_btn_font) {
                    if (hg_g_toolbar_btn_font) DeleteObject(hg_g_toolbar_btn_font);
                    hg_g_toolbar_btn_font = CreateFontW(icon_size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
                    *cached_icon_size = icon_size;
                }

                int visual_order[HG_MAX_WINDOW_ITEMS];
                for(int i=0; i<hg_g_window_count; i++) visual_order[i] = i;

                if (hg_g_is_dragging && hg_g_drag_source_index != -1) {
                    int src = hg_g_drag_source_index;
                    int dst = hg_g_drag_target_index;
                    if (dst != -1 && dst != src) {
                        int temp = visual_order[src];
                        if (src < dst) {
                            for(int i=src; i<dst; i++) visual_order[i] = visual_order[i+1];
                        } else {
                            for(int i=src; i>dst; i--) visual_order[i] = visual_order[i-1];
                        }
                        visual_order[dst] = temp;
                    }
                }

                // Draw tasks (type 0)
                for (int pos = 0; pos < hg_g_window_count; pos++) {
                    int r_idx = visual_order[pos];
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(0, pos, rc.right, rc.bottom, icon_size, &rc_item);
                    rc_btn = rc_item; InflateRect(&rc_btn, SC(4), SC(4));

                    if (hg_g_is_dragging && hg_g_drag_source_index != -1 && r_idx == hg_g_drag_source_index) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) { FrameRect(mem_dc, &rc_btn, hbr); DeleteObject(hbr); }
                        continue;
                    }

                    if (pressed_type == 0 && pressed_index == r_idx && !hg_g_is_dragging) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) { FillRect(mem_dc, &rc_btn, hbr); DeleteObject(hbr); }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 0 && hovered_index == pos && !hg_g_is_dragging) || (hg_g_focus_area == 0 && hg_g_toolbar_focus_index == r_idx)) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) { FillRect(mem_dc, &rc_btn, hbr); DeleteObject(hbr); }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_g_focus_area == 0 && hg_g_toolbar_focus_index == r_idx) {
                            HBRUSH hbr_focus = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
                            if (hbr_focus) { FrameRect(mem_dc, &rc_btn, hbr_focus); DeleteObject(hbr_focus); }
                        }
                    }
                    if (hg_g_window_items[r_idx].icon) {
                        DrawIconEx(mem_dc, rc_item.left, rc_item.top, hg_g_window_items[r_idx].icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
                    }
                }

                // Draw hg_g_shortcuts (type 1)
                int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
                for (int i = 0; i < total_shortcuts; i++) {
                    RECT rc_item, rc_btn;
                    get_toolbar_item_rect(1, i, rc.right, rc.bottom, icon_size, &rc_item);
                    rc_btn = rc_item; InflateRect(&rc_btn, SC(4), SC(4));

                    COLORREF opp_bg = ~bg_color & 0x00FFFFFF;
                    HBRUSH hbr_opp = CreateSolidBrush(opp_bg);
                    if (hbr_opp) { FillRect(mem_dc, &rc_btn, hbr_opp); DeleteObject(hbr_opp); }

                    if (pressed_type == 1 && pressed_index == i) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) { FillRect(mem_dc, &rc_btn, hbr); DeleteObject(hbr); }
                        DrawEdge(mem_dc, &rc_btn, EDGE_SUNKEN, BF_RECT);
                    } else if ((hovered_type == 1 && hovered_index == i) || (hg_g_focus_area == 1 && hg_g_toolbar_focus_index == i)) {
                        HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                        if (hbr) { FillRect(mem_dc, &rc_btn, hbr); DeleteObject(hbr); }
                        DrawEdge(mem_dc, &rc_btn, BDR_RAISEDINNER, BF_RECT);
                        if (hg_g_focus_area == 1 && hg_g_toolbar_focus_index == i) {
                            HBRUSH hbr_focus = CreateSolidBrush(HG_COLOR_BORDER_SELECTED);
                            if (hbr_focus) { FrameRect(mem_dc, &rc_btn, hbr_focus); DeleteObject(hbr_focus); }
                        }
                    }

                    if (i >= HG_NUM_BASIC_ICONS) {
                        int s_idx = i - HG_NUM_BASIC_ICONS;
                        if (hg_g_shortcuts[s_idx].icon) {
                            DrawIconEx(mem_dc, rc_item.left, rc_item.top, hg_g_shortcuts[s_idx].icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
                        }
                    } else if (hg_g_toolbar_btn_font) {
                        SetTextColor(mem_dc, HG_COLOR_TEXT_DEFAULT);
                        SetBkMode(mem_dc, TRANSPARENT);
                        HFONT old_font = (HFONT)SelectObject(mem_dc, hg_g_toolbar_btn_font);
                        WCHAR btn_text[2] = {0};
                        if (i == 0) btn_text[0] = L'R';
                        else if (i == 1) btn_text[0] = L'M';
                        else if (i == 2) btn_text[0] = L'X';
                        else if (i == 3) btn_text[0] = L'D';
                        else if (i == 4) btn_text[0] = L'S';
                        draw_outlined_text(mem_dc, btn_text, 1, &rc_item, DT_CENTER | DT_VCENTER | DT_SINGLELINE, HG_COLOR_TEXT_DEFAULT, HG_COLOR_BG_DEFAULT);
                        SelectObject(mem_dc, old_font);
                    }
                }

                if (hg_g_is_dragging && hg_g_drag_source_index != -1 && hg_g_drag_source_index < hg_g_window_count) {
                    int cx = hg_g_drag_current_pt.x - icon_size / 2;
                    int cy = hg_g_drag_current_pt.y - icon_size / 2;
                    RECT drag_rc = { cx - SC(4), cy - SC(4), cx + icon_size + SC(4), cy + icon_size + SC(4) };
                    HBRUSH hbr = CreateSolidBrush(HG_COLOR_BG_SELECTED);
                    if (hbr) { FillRect(mem_dc, &drag_rc, hbr); DeleteObject(hbr); }
                    DrawEdge(mem_dc, &drag_rc, BDR_RAISEDINNER, BF_RECT);
                    if (hg_g_window_items[hg_g_drag_source_index].icon) {
                        DrawIconEx(mem_dc, cx, cy, hg_g_window_items[hg_g_drag_source_index].icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
                    }
                }

                BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
                SelectObject(mem_dc, old_bm);
                DeleteObject(mem_bm);
            }
            DeleteDC(mem_dc);
        }
    }
    EndPaint(hwnd, &ps);
    return 0;
}

static LRESULT toolbar_controller_on_mouse_move(HWND hwnd, ToolbarControllerState *state, LPARAM l_param) {
    POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
    RECT rc; GetClientRect(hwnd, &rc);
    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);

    if (state->is_resizing && GetCapture() == hwnd) {
        POINT cur_mouse; GetCursorPos(&cur_mouse);
        int dw = cur_mouse.x - state->start_mouse.x;
        int dh = cur_mouse.y - state->start_mouse.y;
        int border = SC(HG_BORDER_THICKNESS);

        int total_tasks = hg_g_window_count;
        int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
        int total_items = total_tasks + total_shortcuts;
        if (total_items <= 0) total_items = 1;

        int cols = 1;
        if (ABS(dh) > ABS(dw)) {
            int new_height = (state->start_rect.bottom - state->start_rect.top) + dh;
            if (new_height < SC(HG_MIN_WINDOW_HEIGHT)) new_height = SC(HG_MIN_WINDOW_HEIGHT);

            int edit_height = SC(20);
            if (hg_g_edit_msg_wnd && hg_g_main_font) {
                HDC hdc = GetDC(hg_g_edit_msg_wnd);
                if (hdc) {
                    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                    TEXTMETRIC tm = {0};
                    GetTextMetrics(hdc, &tm);
                    edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
                    SelectObject(hdc, old_font);
                    ReleaseDC(hg_g_edit_msg_wnd, hdc);
                }
            }
            int row_height = icon_size + SC(10);
            int available_toolbar_h = new_height - (border * 2 + edit_height);
            int target_rows = (available_toolbar_h - SC(10) + row_height / 2) / row_height;
            if (target_rows < 1) target_rows = 1;
            if (target_rows > total_items) target_rows = total_items;
            cols = (total_items + target_rows - 1) / target_rows;
        } else {
            int new_width = (state->start_rect.right - state->start_rect.left) + dw;
            if (new_width < SC(HG_MIN_WINDOW_WIDTH)) new_width = SC(HG_MIN_WINDOW_WIDTH);
            int tb_width = new_width - (border * 2);
            if (tb_width <= 0) tb_width = 1;
            cols = get_items_per_row(tb_width, icon_size);
        }
        if (cols <= 0) cols = 1;

        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
        int req_w = exact_tb_width + border * 2;

        int rows = (total_items + cols - 1) / cols;
        if (rows <= 0) rows = 1;
        int row_height = icon_size + SC(10);
        int req_toolbar_height = SC(10) + rows * row_height;

        int edit_height = SC(20);
        if (hg_g_edit_msg_wnd && hg_g_main_font) {
            HDC hdc = GetDC(hg_g_edit_msg_wnd);
            if (hdc) {
                HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                TEXTMETRIC tm = {0};
                GetTextMetrics(hdc, &tm);
                edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
                SelectObject(hdc, old_font);
                ReleaseDC(hg_g_edit_msg_wnd, hdc);
            }
        }
        int req_h = border * 2 + edit_height + req_toolbar_height;

        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, req_w, req_h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }

    if (state->is_moving_taskbox && GetCapture() == hwnd) {
        POINT cur_mouse;
        GetCursorPos(&cur_mouse);
        int dx = cur_mouse.x - state->start_mouse.x;
        int dy = cur_mouse.y - state->start_mouse.y;
        if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
            SetWindowPos(hg_g_taskbox_wnd, NULL,
                         state->start_rect.left + dx,
                         state->start_rect.top + dy,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }

    int cur_type = -1, cur_index = -1;
    get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index);

    if (hg_g_drag_source_index != -1 && !hg_g_is_dragging && GetCapture() == hwnd && state->pressed_type == 0) {
        if (ABS(pt.x - hg_g_drag_start_pt.x) > GetSystemMetrics(SM_CXDRAG) ||
            ABS(pt.y - hg_g_drag_start_pt.y) > GetSystemMetrics(SM_CYDRAG)) {
            hg_g_is_dragging = TRUE;
        }
    }

    if (hg_g_is_dragging && hg_g_drag_source_index != -1) {
        hg_g_drag_current_pt = pt;
        if (cur_type == 0 && cur_index != -1) {
            hg_g_drag_target_index = cur_index;
        } else if (cur_type == -1 || cur_type == 1) {
            hg_g_drag_target_index = -1;
        }
        InvalidateRect(hwnd, NULL, FALSE);
    } else if (cur_type != state->hovered_type || cur_index != state->hovered_index) {
        state->hovered_type = cur_type;
        state->hovered_index = cur_index;
        update_focus_message(state->hovered_type, state->hovered_index);
        InvalidateRect(hwnd, NULL, FALSE);
    }

    TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
    TrackMouseEvent(&tme);
    return 0;
}

static LRESULT toolbar_controller_on_lbutton_up(HWND hwnd, ToolbarControllerState *state, LPARAM l_param) {
    if (state->is_resizing) {
        state->is_resizing = FALSE;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        /* 크기 조절 완료 시 불필요한 우측 여백을 제거하고 정확하게 스냅되도록 함 */
        RECT rc; GetWindowRect(hg_g_taskbox_wnd, &rc);
        int icon_size = ABS(hg_g_current_font_size);
        if (icon_size < SC(16)) icon_size = SC(16);
        int border = SC(HG_BORDER_THICKNESS);
        int tb_width = (rc.right - rc.left) - border * 2;
        int cols = get_items_per_row(tb_width, icon_size);
        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
        int new_w = exact_tb_width + border * 2;
        SetWindowPos(hg_g_taskbox_wnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        GetWindowRect(hg_g_taskbox_wnd, &rc);
        save_config(L"taskbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    } else if (state->is_moving_taskbox) {
        state->is_moving_taskbox = FALSE;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        RECT rc; GetWindowRect(hg_g_taskbox_wnd, &rc);
        save_config(L"taskbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    } else if (hg_g_drag_source_index != -1) {
        BOOL was_dragging = hg_g_is_dragging;
        int final_source = hg_g_drag_source_index;
        int final_target = hg_g_drag_target_index;
        hg_g_drag_source_index = -1;
        hg_g_drag_target_index = -1;
        state->pressed_type = -1;
        state->pressed_index = -1;
        hg_g_is_dragging = FALSE;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);

        if (!was_dragging) {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            RECT rc; GetClientRect(hwnd, &rc);
            int icon_size = ABS(hg_g_current_font_size);
            if (icon_size < SC(16)) icon_size = SC(16);
            int cur_type = -1, cur_index = -1;
            if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
                if (cur_type == 0 && cur_index == final_source) {
                    activate_taskbar_item(cur_index);
                }
            }
        } else {
            if (final_target != -1 && final_target != final_source) {
                 WindowItem temp = hg_g_window_items[final_source];
                 if (final_source < final_target) {
                     for (int i = final_source; i < final_target; i++) {
                         hg_g_window_items[i] = hg_g_window_items[i + 1];
                     }
                 } else {
                     for (int i = final_source; i > final_target; i--) {
                         hg_g_window_items[i] = hg_g_window_items[i - 1];
                     }
                 }
                 hg_g_window_items[final_target] = temp;
                 hg_g_toolbar_focus_index = final_target;
                 update_toolbar_tooltips(hwnd);
            }
        }
    } else if (state->pressed_type == 1 && state->pressed_index != -1) {
        int cur_index = state->pressed_index;
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        activate_toolbar_item(cur_index);
    } else {
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
}

static BOOL toolbar_controller_get_context_menu_point(HWND hwnd, int cur_type, int cur_index, int icon_size, LPARAM l_param, POINT *screen_pt) {
    if (!screen_pt) return FALSE;

    if (l_param == 0) {
        RECT rc;
        RECT rc_item;
        GetClientRect(hwnd, &rc);
        get_toolbar_item_rect(cur_type, cur_index, rc.right, rc.bottom, icon_size, &rc_item);
        screen_pt->x = rc_item.left;
        screen_pt->y = rc_item.top;
        ClientToScreen(hwnd, screen_pt);
    } else {
        GetCursorPos(screen_pt);
    }

    return TRUE;
}

static void toolbar_controller_show_task_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param) {
    HWND target = hg_g_window_items[cur_index].hwnd;
    HMENU h_menu = CreatePopupMenu();
    if (!h_menu) return;

    /* Windows: Focus only (remove Run) */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESTORE, L"Focus (&F)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_MOVETO_0_0, L"Move to (0, 0) (&0)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_CLOSE, L"Close Window (&X)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_1, L"640x480 (&A)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_2, L"800x600 (&S)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_4_3_3, L"1280x960 (&D)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_1, L"640x360 (&Q)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_2, L"800x480 (&W)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_3, L"960x540 (&E)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_16_9_4, L"1280x720 (&R)");
    AppendMenuW(h_menu, MF_SEPARATOR, 0, NULL);

    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_1, L"360x640 (&1)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_2, L"480x800 (&2)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_3, L"540x960 (&3)");
    AppendMenuW(h_menu, MF_STRING, HG_IDM_TASK_RESIZE_9_16_4, L"720x1280 (&4)");

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 0, cur_index, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetMenuDefaultItem(h_menu, HG_IDM_TASK_RESTORE, FALSE);
    int cmd = TrackPopupMenuEx(h_menu, TPM_RETURNCMD, screen_pt.x, screen_pt.y, hwnd, NULL);

    if (cmd == HG_IDM_TASK_RESTORE) {
        activate_taskbar_item(cur_index);
    } else if (cmd == HG_IDM_TASK_CLOSE) {
        PostMessageW(target, WM_CLOSE, 0, 0);
    } else if (cmd == HG_IDM_TASK_MOVETO_0_0) {
        SetWindowPos(target, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    } else if (cmd >= HG_IDM_TASK_RESIZE_4_3_1 && cmd <= HG_IDM_TASK_RESIZE_9_16_4) {
        int cx = 0, cy = 0;
        switch (cmd) {
            case HG_IDM_TASK_RESIZE_4_3_1: cx = 640; cy = 480; break;
            case HG_IDM_TASK_RESIZE_4_3_2: cx = 800; cy = 600; break;
            case HG_IDM_TASK_RESIZE_4_3_3: cx = 1280; cy = 960; break;
            case HG_IDM_TASK_RESIZE_16_9_1: cx = 640; cy = 360; break;
            case HG_IDM_TASK_RESIZE_16_9_2: cx = 800; cy = 480; break;
            case HG_IDM_TASK_RESIZE_16_9_3: cx = 960; cy = 540; break;
            case HG_IDM_TASK_RESIZE_16_9_4: cx = 1280; cy = 720; break;
            case HG_IDM_TASK_RESIZE_9_16_1: cx = 360; cy = 640; break;
            case HG_IDM_TASK_RESIZE_9_16_2: cx = 480; cy = 800; break;
            case HG_IDM_TASK_RESIZE_9_16_3: cx = 540; cy = 960; break;
            case HG_IDM_TASK_RESIZE_9_16_4: cx = 720; cy = 1280; break;
        }
        if (cx > 0 && cy > 0) {
            SetWindowPos(target, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    DestroyMenu(h_menu);
}

static void toolbar_controller_show_shortcut_context_menu(HWND hwnd, int cur_index, int icon_size, LPARAM l_param) {
    HMENU h_menu = CreatePopupMenu();
    if (!h_menu) return;

    /* Shortcuts: Run only (remove Focus) */
    AppendMenuW(h_menu, MF_STRING, HG_IDM_SHORTCUT_RUN, L"Run (&R)");
    if (cur_index >= HG_NUM_BASIC_ICONS) {
        AppendMenuW(h_menu, MF_STRING, HG_IDM_SHORTCUT_OPEN_DIR, L"Open File Location (&O)");
    }

    POINT screen_pt;
    if (!toolbar_controller_get_context_menu_point(hwnd, 1, cur_index, icon_size, l_param, &screen_pt)) {
        DestroyMenu(h_menu);
        return;
    }

    SetMenuDefaultItem(h_menu, HG_IDM_SHORTCUT_RUN, FALSE);
    int cmd = TrackPopupMenuEx(h_menu, TPM_RETURNCMD, screen_pt.x, screen_pt.y, hwnd, NULL);

    if ((UINT)cmd == HG_IDM_SHORTCUT_RUN) {
        activate_toolbar_item(cur_index);
    } else if ((UINT)cmd == HG_IDM_SHORTCUT_OPEN_DIR) {
        int s_idx = cur_index - HG_NUM_BASIC_ICONS;
        if (s_idx >= 0 && s_idx < hg_g_shortcut_count) {
            PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(hg_g_shortcuts[s_idx].path);
            if (pidl) {
                SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
                ILFree(pidl);
            }
        }
    }

    DestroyMenu(h_menu);
}

static LRESULT toolbar_controller_on_lbutton_down(HWND hwnd, ToolbarControllerState *state, LPARAM l_param) {
    POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);

    int cur_type = -1, cur_index = -1;
    if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
        state->hovered_type = cur_type;
        state->hovered_index = cur_index;
        hg_g_focus_area = cur_type;
        state->pressed_type = cur_type;
        state->pressed_index = cur_index;

        if (cur_type == 1 && cur_index == 0) { // Resize Drag Tool
            state->is_resizing = TRUE;
            GetCursorPos(&state->start_mouse);
            GetWindowRect(hg_g_taskbox_wnd, &state->start_rect);
        } else if (cur_type == 1 && cur_index == 1) { // Move Drag Tool
            state->is_moving_taskbox = TRUE;
            GetCursorPos(&state->start_mouse);
            GetWindowRect(hg_g_taskbox_wnd, &state->start_rect);
        } else if (cur_type == 0) { // Task drag start
            hg_g_is_dragging = FALSE;
            hg_g_drag_source_index = cur_index;
            hg_g_drag_start_pt = pt;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        SetCapture(hwnd);
    }
    return 0;
}

static LRESULT toolbar_controller_on_rbutton_up(HWND hwnd, LPARAM l_param) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);

    int cur_type = -1, cur_index = -1;
    if (l_param == 0) {
        cur_type = hg_g_focus_area;
        cur_index = hg_g_toolbar_focus_index;
    } else {
        POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
        get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index);
    }

    if (cur_type == 0 && cur_index != -1) {
        toolbar_controller_show_task_context_menu(hwnd, cur_index, icon_size, l_param);
    } else if (cur_type == 1 && cur_index != -1) {
        toolbar_controller_show_shortcut_context_menu(hwnd, cur_index, icon_size, l_param);
    }

    return 0;
}

static LRESULT toolbar_controller_on_mbutton_up(HWND hwnd, LPARAM l_param) {
    POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
    RECT rc;
    GetClientRect(hwnd, &rc);

    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);

    int cur_type = -1, cur_index = -1;
    if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, &cur_type, &cur_index)) {
        if (cur_type == 0) {
            HWND target = hg_g_window_items[cur_index].hwnd;
            if (IsWindow(target)) {
                PostMessageW(target, WM_CLOSE, 0, 0);
            }
        }
    }
    return 0;
}

static LRESULT toolbar_controller_on_mouse_leave(HWND hwnd, ToolbarControllerState *state) {
    state->hovered_type = -1;
    state->hovered_index = -1;
    if (!state->is_resizing && !state->is_moving_taskbox && !hg_g_is_dragging) {
        state->pressed_type = -1;
        state->pressed_index = -1;
        ReleaseCapture();
    }
    update_focus_message(-1, -1);
    InvalidateRect(hwnd, NULL, FALSE);
    return 0;
}

static LRESULT toolbar_controller_on_mouse_wheel(HWND hwnd, WPARAM w_param, LPARAM l_param) {
    if (LOWORD(w_param) & MK_CONTROL) {
        short delta = (short)HIWORD(w_param);
        update_size(delta > 0 ? 1 : -1);
        return 0;
    }
    if (GetKeyState(VK_MENU) < 0) {
        return SendMessageW(GetParent(hwnd), WM_MOUSEWHEEL, w_param, l_param);
    }
    return 0;
}

LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    static int hovered_type = -1, hovered_index = -1;
    static int pressed_type = -1, pressed_index = -1;
    static int cached_icon_size = 0;
    static BOOL is_resizing = FALSE;
    static BOOL is_moving_taskbox = FALSE;
    static POINT start_mouse;
    static RECT start_rect;

    switch (msg) {
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int icon_size = ABS(hg_g_current_font_size);
            if (icon_size < SC(16)) icon_size = SC(16);
            if (get_item_at_pt(pt, rc.right, rc.bottom, icon_size, NULL, NULL)) return HTCLIENT;
            return HTTRANSPARENT;
        }
        case WM_SIZE: {
            update_toolbar_tooltips(hwnd);
            return 0;
        }
        case WM_PAINT:
            return toolbar_controller_on_paint(hwnd, hovered_type, hovered_index, pressed_type, pressed_index, &cached_icon_size);
        case WM_KEYDOWN: {
            SendMessage(GetParent(hwnd), WM_KEYDOWN, w_param, l_param);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            ToolbarControllerState state = {
                hovered_type,
                hovered_index,
                pressed_type,
                pressed_index,
                cached_icon_size,
                is_resizing,
                is_moving_taskbox,
                start_mouse,
                start_rect
            };
            LRESULT result = toolbar_controller_on_lbutton_down(hwnd, &state, l_param);
            hovered_type = state.hovered_type;
            hovered_index = state.hovered_index;
            pressed_type = state.pressed_type;
            pressed_index = state.pressed_index;
            cached_icon_size = state.cached_icon_size;
            is_resizing = state.is_resizing;
            is_moving_taskbox = state.is_moving_taskbox;
            start_mouse = state.start_mouse;
            start_rect = state.start_rect;
            return result;
        }
        case WM_MOUSEMOVE: {
            ToolbarControllerState state = {
                hovered_type,
                hovered_index,
                pressed_type,
                pressed_index,
                cached_icon_size,
                is_resizing,
                is_moving_taskbox,
                start_mouse,
                start_rect
            };
            LRESULT result = toolbar_controller_on_mouse_move(hwnd, &state, l_param);
            hovered_type = state.hovered_type;
            hovered_index = state.hovered_index;
            pressed_type = state.pressed_type;
            pressed_index = state.pressed_index;
            cached_icon_size = state.cached_icon_size;
            is_resizing = state.is_resizing;
            is_moving_taskbox = state.is_moving_taskbox;
            start_mouse = state.start_mouse;
            start_rect = state.start_rect;
            return result;
        }
        case WM_LBUTTONUP: {
            ToolbarControllerState state = {
                hovered_type,
                hovered_index,
                pressed_type,
                pressed_index,
                cached_icon_size,
                is_resizing,
                is_moving_taskbox,
                start_mouse,
                start_rect
            };
            LRESULT result = toolbar_controller_on_lbutton_up(hwnd, &state, l_param);
            hovered_type = state.hovered_type;
            hovered_index = state.hovered_index;
            pressed_type = state.pressed_type;
            pressed_index = state.pressed_index;
            cached_icon_size = state.cached_icon_size;
            is_resizing = state.is_resizing;
            is_moving_taskbox = state.is_moving_taskbox;
            start_mouse = state.start_mouse;
            start_rect = state.start_rect;
            return result;
        }
        case WM_RBUTTONUP:
            return toolbar_controller_on_rbutton_up(hwnd, l_param);
        case WM_MBUTTONUP:
            return toolbar_controller_on_mbutton_up(hwnd, l_param);
        case WM_MOUSELEAVE: {
            ToolbarControllerState state = {
                hovered_type,
                hovered_index,
                pressed_type,
                pressed_index,
                cached_icon_size,
                is_resizing,
                is_moving_taskbox,
                start_mouse,
                start_rect
            };
            LRESULT result = toolbar_controller_on_mouse_leave(hwnd, &state);
            hovered_type = state.hovered_type;
            hovered_index = state.hovered_index;
            pressed_type = state.pressed_type;
            pressed_index = state.pressed_index;
            cached_icon_size = state.cached_icon_size;
            is_resizing = state.is_resizing;
            is_moving_taskbox = state.is_moving_taskbox;
            start_mouse = state.start_mouse;
            start_rect = state.start_rect;
            return result;
        }
        case WM_MOUSEWHEEL:
            return toolbar_controller_on_mouse_wheel(hwnd, w_param, l_param);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

BOOL get_explorer_path(HWND target_hwnd, WCHAR* out_path, int max_len) {
    if (!out_path || max_len <= 0) return FALSE;
    out_path[0] = L'\0';
    IShellWindows *psw = NULL;
    if (FAILED(CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_ALL, &IID_IShellWindows, (void**)&psw))) return FALSE;

    long count = 0;
    if (FAILED(psw->lpVtbl->get_Count(psw, &count))) {
        psw->lpVtbl->Release(psw);
        return FALSE;
    }

    BOOL found = FALSE;
    for (long i = 0; i < count; i++) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = i;
        IDispatch *pdisp = NULL;
        if (SUCCEEDED(psw->lpVtbl->Item(psw, v, &pdisp)) && pdisp) {
            IWebBrowser2 *pwb = NULL;
            if (SUCCEEDED(pdisp->lpVtbl->QueryInterface(pdisp, &IID_IWebBrowser2, (void**)&pwb)) && pwb) {
                /* Try reading hw as long or INT_PTR depending on headers. SHANDLE_PTR is standard */
                LONG_PTR hw = 0;
                if (SUCCEEDED(pwb->lpVtbl->get_HWND(pwb, &hw)) && (HWND)hw == target_hwnd) {
                    BSTR url = NULL;
                    if (SUCCEEDED(pwb->lpVtbl->get_LocationURL(pwb, &url)) && url) {
                        DWORD pc_len = (DWORD)max_len;
                        if (SUCCEEDED(PathCreateFromUrlW(url, out_path, &pc_len, 0))) {
                            found = TRUE;
                        } else {
                            /* Might be search-ms: or other specialized protocol; fallback */
                            BSTR title = NULL;
                            if (SUCCEEDED(pwb->lpVtbl->get_LocationName(pwb, &title)) && title) {
                                lstrcpynW(out_path, title, max_len);
                                SysFreeString(title);
                            } else {
                                lstrcpynW(out_path, url, max_len);
                            }
                            found = TRUE;
                        }
                        SysFreeString(url);
                    }
                }
                pwb->lpVtbl->Release(pwb);
            }
            pdisp->lpVtbl->Release(pdisp);
        }
        if (found) break;
    }
    psw->lpVtbl->Release(psw);
    return found;
}

/* 증분 업데이트: 순서 고정 및 스마트 리프레시 */
void refresh_window_list(BOOL force) {
    int new_count = 0;
    ZeroMemory(hg_g_new_items, sizeof(hg_g_new_items));

    /* 1단계: 기존 창 유효성 체크 및 아이콘 재사용 */
    for (int i = 0; i < hg_g_window_count; i++) {
        if (new_count >= HG_MAX_WINDOW_ITEMS) break;

        if (IsWindow(hg_g_window_items[i].hwnd) && is_alt_tab_window(hg_g_window_items[i].hwnd)) {
            hg_g_new_items[new_count] = (WindowItem){0};
            hg_g_new_items[new_count].hwnd = hg_g_window_items[i].hwnd;
            if (force) {
                if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
                    DestroyIcon(hg_g_window_items[i].icon);
                }
                hg_g_window_items[i].icon = NULL;
                hg_g_window_items[i].own_icon = FALSE;
                hg_g_new_items[new_count].icon = get_window_icon(hg_g_window_items[i].hwnd, ABS(hg_g_current_font_size), &hg_g_new_items[new_count].own_icon);
            } else {
                hg_g_new_items[new_count].icon = hg_g_window_items[i].icon;
                hg_g_new_items[new_count].own_icon = hg_g_window_items[i].own_icon;
                hg_g_window_items[i].icon = NULL;
                hg_g_window_items[i].own_icon = FALSE;
            }

            hg_g_new_items[new_count].exists = TRUE;
            StringCchCopyW(hg_g_new_items[new_count].process_name, HG_MAX_STR, hg_g_window_items[i].process_name);
            hg_g_new_items[new_count].process_id = hg_g_window_items[i].process_id;

            if (GetWindowTextW(hg_g_new_items[new_count].hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                hg_g_new_items[new_count].title[0] = L'\0';
            }

            if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                WCHAR path[HG_MAX_PATH];
                if (get_explorer_path(hg_g_new_items[new_count].hwnd, path, HG_MAX_PATH)) {
                    StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                }
            }

            new_count++;
        } else {
            if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
                DestroyIcon(hg_g_window_items[i].icon);
            }
            hg_g_window_items[i].icon = NULL;
            hg_g_window_items[i].own_icon = FALSE;
        }
    }

    /* 2단계: 새로 나타난 창 추가 */
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        if (is_alt_tab_window(hwnd)) {
            BOOL exists = FALSE;
            for (int i = 0; i < new_count; i++) {
                if (hg_g_new_items[i].hwnd == hwnd) {
                    exists = TRUE;
                    break;
                }
            }

            if (!exists) {
                if (new_count >= HG_MAX_WINDOW_ITEMS) break;

                hg_g_new_items[new_count] = (WindowItem){0};
                hg_g_new_items[new_count].hwnd = hwnd;
                hg_g_new_items[new_count].icon = get_window_icon(hwnd, ABS(hg_g_current_font_size), &hg_g_new_items[new_count].own_icon);
                hg_g_new_items[new_count].exists = TRUE;
                get_process_name_by_hwnd(hwnd, hg_g_new_items[new_count].process_name, HG_MAX_STR, &hg_g_new_items[new_count].process_id);

                if (GetWindowTextW(hwnd, hg_g_new_items[new_count].title, HG_MAX_STR) == 0) {
                    hg_g_new_items[new_count].title[0] = L'\0';
                }

                    if (lstrcmpiW(hg_g_new_items[new_count].process_name, L"explorer.exe") == 0) {
                        WCHAR path[HG_MAX_PATH];
                        if (get_explorer_path(hwnd, path, HG_MAX_PATH)) {
                            StringCchCopyW(hg_g_new_items[new_count].title, HG_MAX_STR, path);
                        }
                    }

                new_count++;
            }
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }

    BOOL changed = force || (new_count != hg_g_window_count);
    if (!changed) {
        for (int i = 0; i < hg_g_window_count; i++) {
            if (hg_g_new_items[i].hwnd != hg_g_window_items[i].hwnd ||
                lstrcmpW(hg_g_new_items[i].title, hg_g_window_items[i].title) != 0) {
                changed = TRUE;
                break;
            }
        }
    }

    hg_g_window_count = new_count;
    for (int i = 0; i < hg_g_window_count; i++) {
        hg_g_window_items[i] = hg_g_new_items[i];
    }

    if (changed && hg_g_taskbox_wnd) {
        update_layout(hg_g_taskbox_wnd);
        if (hg_g_toolbar_wnd) InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
}

/* taskbar_proc removed */

LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass, DWORD_PTR dw_ref_data) {
    switch (msg) {
        case WM_SETFOCUS: {
            disable_window_ime(hwnd);
            break;
        }
        case WM_LBUTTONDOWN: {
            ReleaseCapture();
            SendMessageW(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
            return 0;
        }
        case WM_KEYDOWN: {
            if (w_param == VK_F2) {
                if (hg_g_floater_wnd) PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
                return 0;
            }
            break;
        }
        case WM_MOUSEWHEEL: {
            if (LOWORD(w_param) & MK_CONTROL) {
                short delta = (short)HIWORD(w_param);
                update_edit_font_size(delta > 0 ? 1 : -1);
                return 0;
            } else if (GetKeyState(VK_MENU) < 0) {
                short delta = (short)HIWORD(w_param);
                update_taskbox_alpha(delta > 0 ? 1 : -1);
                return 0;
            }
            break;
        }
        case WM_CONTEXTMENU: {
            HMENU h_menu = CreatePopupMenu();
            if (!h_menu) return 0;
            AppendMenuW(h_menu, MF_STRING, HG_IDM_EDIT_COPYALL, L"Copy All (&A)");

            POINT pt;
            if (l_param == (LPARAM)-1) { /* Keyboard shortcut */
                RECT rc; GetWindowRect(hwnd, &rc);
                pt.x = rc.left + 5; pt.y = rc.top + 5;
            } else {
                pt.x = GET_X_LPARAM(l_param);
                pt.y = GET_Y_LPARAM(l_param);
            }

            int cmd = TrackPopupMenuEx(h_menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, hwnd, NULL);

            if (cmd == HG_IDM_EDIT_COPYALL) {
                SendMessageW(hwnd, EM_SETSEL, 0, (LPARAM)-1);
                SendMessageW(hwnd, WM_COPY, 0, 0);
                SendMessageW(hwnd, EM_SETSEL, (WPARAM)-1, 0);
            }

            DestroyMenu(h_menu);
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

void update_layout(HWND hwnd) {
    if (!hwnd || !hg_g_edit_msg_wnd || !hg_g_toolbar_wnd || !hg_g_main_font) return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right;
    int border = SC(HG_BORDER_THICKNESS);

    /* Get edit control height */
    HDC hdc = GetDC(hg_g_edit_msg_wnd);
    if (!hdc) return;
    HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
    TEXTMETRIC tm = {0};
    GetTextMetrics(hdc, &tm);
    int edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
    SelectObject(hdc, old_font);
    ReleaseDC(hg_g_edit_msg_wnd, hdc);

    int tb_width = width - (border * 2);
    if (tb_width <= 0) tb_width = 1;

    /* Calculate necessary height for the taskbox based on columns and rows */
    int icon_size = ABS(hg_g_current_font_size);
    if (icon_size < SC(16)) icon_size = SC(16);
    int cols = get_items_per_row(tb_width, icon_size);
    if (cols <= 0) cols = 1;
    int total_tasks = hg_g_window_count;
    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
    int rows = (total_tasks + total_shortcuts + cols - 1) / cols;
    if (rows <= 0) rows = 1;

    int row_height = icon_size + SC(10);

    RECT win_rc;
    GetWindowRect(hwnd, &win_rc);
    int current_height = win_rc.bottom - win_rc.top;
    int current_width = win_rc.right - win_rc.left;

    int req_toolbar_height = SC(10) + rows * row_height;
    int required_total_height = border * 2 + edit_height + req_toolbar_height;

    int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
    int required_total_width = border * 2 + exact_tb_width;

    /* If the current window size is different from required, resize it */
    if (!hg_g_in_sizemove) {
        if (current_height != required_total_height || current_width != required_total_width) {
            SetWindowPos(hwnd, NULL, 0, 0, required_total_width, required_total_height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            return; /* SetWindowPos will trigger WM_SIZE, calling update_layout again */
        }
    }

    MoveWindow(hg_g_edit_msg_wnd, border, border, exact_tb_width, edit_height, TRUE);
    MoveWindow(hg_g_toolbar_wnd, border, border + edit_height, exact_tb_width, req_toolbar_height, TRUE);
    update_toolbar_tooltips(hg_g_toolbar_wnd);
}

static LRESULT CALLBACK monitor_edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR uid, DWORD_PTR dw_ref) {
    switch (msg) {
        case WM_SETFOCUS:
            disable_window_ime(hwnd);
            break;
        case WM_LBUTTONDOWN:
            SendMessageW(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | 0x0002, 0);
            return 0;
        case WM_RBUTTONDOWN:
            PostMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
            return 0;
    }
    return DefSubclassProc(hwnd, msg, w_param, l_param);
}

static BOOL monitor_pt_to_screen_pt(HWND hwnd, int monitor_idx, POINT client_pt, POINT* out_screen_pt, BOOL allow_out) {
    if (monitor_idx < 0 || monitor_idx >= hg_g_monitor_count) return FALSE;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int border = SC(HG_BORDER_THICKNESS);
    int edit_height = 0;
    HWND edit_wnd = GetDlgItem(hwnd, 104);
    if (edit_wnd) {
        RECT erc;
        GetWindowRect(edit_wnd, &erc);
        edit_height = erc.bottom - erc.top;
    }
    int preview_top = border + edit_height;
    int preview_left = border;
    int preview_right = rc.right - border;
    int preview_bottom = rc.bottom - border;

    if (!allow_out && (client_pt.x < preview_left || client_pt.x > preview_right ||
        client_pt.y < preview_top || client_pt.y > preview_bottom)) {
        return FALSE;
    }

    int pw = preview_right - preview_left;
    int ph = preview_bottom - preview_top;
    if (pw <= 0 || ph <= 0) return FALSE;

    RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;

    double rx = (double)(client_pt.x - preview_left) / pw;
    double ry = (double)(client_pt.y - preview_top) / ph;

    out_screen_pt->x = m_rc.left + (int)(rx * mw);
    out_screen_pt->y = m_rc.top + (int)(ry * mh);
    return TRUE;
}

static BOOL screen_pt_to_monitor_pt(HWND hwnd, int monitor_idx, POINT screen_pt, POINT* out_client_pt) {
    if (monitor_idx < 0 || monitor_idx >= hg_g_monitor_count) return FALSE;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int border = SC(HG_BORDER_THICKNESS);
    int edit_height = 0;
    HWND edit_wnd = GetDlgItem(hwnd, 104);
    if (edit_wnd) {
        RECT erc;
        GetWindowRect(edit_wnd, &erc);
        edit_height = erc.bottom - erc.top;
    }
    int preview_top = border + edit_height;
    int preview_left = border;
    int preview_right = rc.right - border;
    int preview_bottom = rc.bottom - border;

    int pw = preview_right - preview_left;
    int ph = preview_bottom - preview_top;
    if (pw <= 0 || ph <= 0) return FALSE;

    RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
    int mw = m_rc.right - m_rc.left;
    int mh = m_rc.bottom - m_rc.top;
    if (mw <= 0 || mh <= 0) return FALSE;

    double rx = (double)(screen_pt.x - m_rc.left) / mw;
    double ry = (double)(screen_pt.y - m_rc.top) / mh;

    if (rx < 0.0 || rx > 1.0 || ry < 0.0 || ry > 1.0) return FALSE;

    out_client_pt->x = preview_left + (int)(rx * pw);
    out_client_pt->y = preview_top + (int)(ry * ph);
    return TRUE;
}

LRESULT CALLBACK monitor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    int monitor_idx = -1;
    for (int i = 0; i < hg_g_monitor_count; ++i) {
        if (hg_g_monitors[i].hwnd == hwnd) {
            monitor_idx = i;
            break;
        }
    }

    switch (msg) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_CREATE: {
            LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)l_param;
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            HWND edit_wnd = CreateWindowExW(0, L"EDIT", pcs->lpszName,
                WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0, hwnd, (HMENU)104, GetModuleHandle(NULL), NULL);
            if (edit_wnd) {
                SendMessageW(edit_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                SetWindowSubclass(edit_wnd, monitor_edit_subclass_proc, 1, 0);
                disable_window_ime(edit_wnd);
            }
            SetTimer(hwnd, 2, 100, NULL);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_TIMER: {
            if (w_param == 2) {
                if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            } else if (w_param == 3) {
                KillTimer(hwnd, 3);
                POINT screen_pt;
                screen_pt.x = (int)(INT_PTR)GetPropW(hwnd, L"drop_x");
                screen_pt.y = (int)(INT_PTR)GetPropW(hwnd, L"drop_y");

                POINT client_pt;
                if (screen_pt_to_monitor_pt(hwnd, monitor_idx, screen_pt, &client_pt)) {
                    POINT tb_screen_pt = client_pt;
                    ClientToScreen(hwnd, &tb_screen_pt);
                    SetCursorPos(tb_screen_pt.x, tb_screen_pt.y);
                }
                RemovePropW(hwnd, L"drop_x");
                RemovePropW(hwnd, L"drop_y");
            }
            return 0;
        }
        case WM_SIZE: {
            HWND edit_wnd = GetDlgItem(hwnd, 104);
            if (edit_wnd && hg_g_main_font) {
                int border = SC(HG_BORDER_THICKNESS);
                int w = (int)LOWORD(l_param) - border * 2;

                HDC hdc = GetDC(edit_wnd);
                HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                TEXTMETRIC tm = {0};
                GetTextMetrics(hdc, &tm);
                int edit_height = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
                SelectObject(hdc, old_font);
                ReleaseDC(edit_wnd, hdc);

                MoveWindow(edit_wnd, border, border, w, edit_height, TRUE);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)w_param;
            SetTextColor(hdc, hg_g_color_scheme_selected.text);
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, hg_g_color_scheme_selected.bg);
            if (!hg_g_edit_bg_brush) hg_g_edit_bg_brush = CreateSolidBrush(hg_g_color_scheme_selected.bg);
            return hg_g_edit_bg_brush ? (LRESULT)hg_g_edit_bg_brush : (LRESULT)GetStockObject(BLACK_BRUSH);
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            if (rc.right > 0 && rc.bottom > 0) {
                HDC mem_dc = CreateCompatibleDC(hdc);
                HBITMAP mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                HBITMAP old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
                if (!old_bm) {
                    DeleteObject(mem_bm);
                    DeleteDC(mem_dc);
                    EndPaint(hwnd, &ps);
                    return 0;
                }

                int border = SC(HG_BORDER_THICKNESS);
                int edit_height = 0;
                HWND edit_wnd = GetDlgItem(hwnd, 104);
                if (edit_wnd) {
                    RECT erc;
                    GetWindowRect(edit_wnd, &erc);
                    edit_height = erc.bottom - erc.top;
                }

                if (monitor_idx >= 0 && hg_g_monitors[monitor_idx].active) {
                    RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
                    int mw = m_rc.right - m_rc.left;
                    int mh = m_rc.bottom - m_rc.top;

                    HDC hdc_screen = GetDC(NULL);
                    SetStretchBltMode(mem_dc, HALFTONE);
                    /* Draw preview area below the edit box + border */
                    int preview_top = border + edit_height;
                    StretchBlt(mem_dc, border, preview_top, rc.right - 2 * border, rc.bottom - preview_top - border, hdc_screen, m_rc.left, m_rc.top, mw, mh, SRCCOPY);
                    ReleaseDC(NULL, hdc_screen);

                    /* Draw cursor if needed */
                    POINT cursor_pt;
                    if (GetCursorPos(&cursor_pt)) {
                        POINT client_pt;
                        if (screen_pt_to_monitor_pt(hwnd, monitor_idx, cursor_pt, &client_pt)) {
                            COLORREF fill_color = RGB(255, 0, 0); /* red by default */
                            if (GetAsyncKeyState(VK_MBUTTON) < 0) fill_color = RGB(255, 255, 0); /* yellow */
                            else if (GetAsyncKeyState(VK_RBUTTON) < 0) fill_color = RGB(0, 0, 255); /* blue */
                            else if (GetAsyncKeyState(VK_LBUTTON) < 0) fill_color = RGB(0, 255, 0); /* green */

                            int len = SC(8);
                            /* Draw black crosshair background for contrast */
                            HPEN hPenBg = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
                            HPEN oldPen = (HPEN)SelectObject(mem_dc, hPenBg);
                            MoveToEx(mem_dc, client_pt.x - len, client_pt.y, NULL);
                            LineTo(mem_dc, client_pt.x + len + 1, client_pt.y);
                            MoveToEx(mem_dc, client_pt.x, client_pt.y - len, NULL);
                            LineTo(mem_dc, client_pt.x, client_pt.y + len + 1);

                            /* Draw colored crosshair core */
                            HPEN hPenFg = CreatePen(PS_SOLID, 1, fill_color);
                            SelectObject(mem_dc, hPenFg);
                            MoveToEx(mem_dc, client_pt.x - len, client_pt.y, NULL);
                            LineTo(mem_dc, client_pt.x + len + 1, client_pt.y);
                            MoveToEx(mem_dc, client_pt.x, client_pt.y - len, NULL);
                            LineTo(mem_dc, client_pt.x, client_pt.y + len + 1);

                            SelectObject(mem_dc, oldPen);
                            DeleteObject(hPenBg);
                            DeleteObject(hPenFg);
                        }
                    }
                }

                /* Draw border like taskbox/floater */
                HBRUSH border_brush = CreateSolidBrush(HG_COLOR_BG_TOOLBAR);
                if (border_brush) {
                    RECT rc_top = {0, 0, rc.right, border};
                    RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
                    RECT rc_left = {0, border, border, rc.bottom - border};
                    RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};
                    RECT rc_edit_bg = {0, border, rc.right, border + edit_height};

                    FillRect(mem_dc, &rc_top, border_brush);
                    FillRect(mem_dc, &rc_bottom, border_brush);
                    FillRect(mem_dc, &rc_left, border_brush);
                    FillRect(mem_dc, &rc_right, border_brush);
                    FillRect(mem_dc, &rc_edit_bg, border_brush);
                    DeleteObject(border_brush);
                }

                BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);

                SelectObject(mem_dc, old_bm);
                DeleteObject(mem_bm);
                DeleteDC(mem_dc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            HMONITOR my_phys_mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor_idx >= 0 && my_phys_mon == hg_g_monitors[monitor_idx].hMonitor) {
                return 0;
            }

            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            POINT screen_pt;
            if (monitor_pt_to_screen_pt(hwnd, monitor_idx, pt, &screen_pt, FALSE)) {
                INPUT inputs[2] = {0};

                inputs[0].type = INPUT_MOUSE;
                if (msg == WM_LBUTTONDOWN) inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                else if (msg == WM_RBUTTONDOWN) inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                else if (msg == WM_MBUTTONDOWN) inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

                SetCursorPos(screen_pt.x, screen_pt.y);

                inputs[1].type = INPUT_MOUSE;
                if (msg == WM_LBUTTONDOWN) inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                else if (msg == WM_RBUTTONDOWN) inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                else if (msg == WM_MBUTTONDOWN) inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;

                SendInput(2, inputs, sizeof(INPUT));
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            HMONITOR my_phys_mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor_idx >= 0 && my_phys_mon == hg_g_monitors[monitor_idx].hMonitor) {
                return 0;
            }

            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            POINT screen_pt;
            if (monitor_pt_to_screen_pt(hwnd, monitor_idx, pt, &screen_pt, FALSE)) {
                INPUT inputs[1] = {0};
                inputs[0].type = INPUT_MOUSE;
                if (msg == WM_LBUTTONUP) inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                else if (msg == WM_RBUTTONUP) inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                else if (msg == WM_MBUTTONUP) inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

                SetCursorPos(screen_pt.x, screen_pt.y);
                SendInput(1, inputs, sizeof(INPUT));
            }
            return 0;
        }
        case WM_CLOSE: {
            if (monitor_idx >= 0) {
                close_monitor_window(monitor_idx);
            } else {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        case WM_NCHITTEST: {
            POINT pt;
            pt.x = GET_X_LPARAM(l_param);
            pt.y = GET_Y_LPARAM(l_param);
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int border = SC(8);
            if (pt.x < rc.left + border && pt.y < rc.top + border) return HTTOPLEFT;
            if (pt.x >= rc.right - border && pt.y < rc.top + border) return HTTOPRIGHT;
            if (pt.x < rc.left + border && pt.y >= rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x >= rc.right - border && pt.y >= rc.bottom - border) return HTBOTTOMRIGHT;
            if (pt.x < rc.left + border) return HTLEFT;
            if (pt.x >= rc.right - border) return HTRIGHT;
            if (pt.y < rc.top + border) return HTTOP;
            if (pt.y >= rc.bottom - border) return HTBOTTOM;

            return HTCLIENT;
        }
        case WM_SIZING: {
            /* Force Aspect Ratio */
            if (monitor_idx >= 0) {
                RECT *prc = (RECT*)l_param;
                RECT m_rc = hg_g_monitors[monitor_idx].rcMonitor;
                int mw = m_rc.right - m_rc.left;
                int mh = m_rc.bottom - m_rc.top;
                if (mh > 0 && mw > 0) {
                    int w = prc->right - prc->left;
                    int h = prc->bottom - prc->top;

                    /* Depending on sizing edge, adjust width or height */
                    switch (w_param) {
                        case WMSZ_LEFT:
                        case WMSZ_RIGHT:
                            prc->bottom = prc->top + (w * mh) / mw;
                            break;
                        case WMSZ_TOP:
                        case WMSZ_BOTTOM:
                            prc->right = prc->left + (h * mw) / mh;
                            break;
                        case WMSZ_TOPLEFT:
                        case WMSZ_TOPRIGHT:
                        case WMSZ_BOTTOMLEFT:
                        case WMSZ_BOTTOMRIGHT:
                            /* just adjust bottom to keep ratio with width */
                            prc->bottom = prc->top + (w * mh) / mw;
                            break;
                    }
                }
                return TRUE;
            }
            break;
        }
        case WM_EXITSIZEMOVE: {
            if (monitor_idx >= 0) {
                RECT rc;
                GetWindowRect(hwnd, &rc);
                WCHAR key_x[64], key_y[64], key_w[64], key_h[64], key_name[64];
                StringCchPrintfW(key_x, 64, L"monitor%d_x", monitor_idx + 1);
                StringCchPrintfW(key_y, 64, L"monitor%d_y", monitor_idx + 1);
                StringCchPrintfW(key_w, 64, L"monitor%d_w", monitor_idx + 1);
                StringCchPrintfW(key_h, 64, L"monitor%d_h", monitor_idx + 1);
                StringCchPrintfW(key_name, 64, L"monitor%d_name", monitor_idx + 1);
                WritePrivateProfileStringW(L"monitor", key_name, hg_g_monitors[monitor_idx].name, hg_g_config_path);

                WCHAR buf[32];
                hellgates_wsprintf(buf, 32, L"%d", rc.left);
                WritePrivateProfileStringW(L"monitor", key_x, buf, hg_g_config_path);
                hellgates_wsprintf(buf, 32, L"%d", rc.top);
                WritePrivateProfileStringW(L"monitor", key_y, buf, hg_g_config_path);
                hellgates_wsprintf(buf, 32, L"%d", rc.right - rc.left);
                WritePrivateProfileStringW(L"monitor", key_w, buf, hg_g_config_path);
                hellgates_wsprintf(buf, 32, L"%d", rc.bottom - rc.top);
                WritePrivateProfileStringW(L"monitor", key_h, buf, hg_g_config_path);
            }
            break;
        }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

static LRESULT taskbox_controller_on_create(HWND hwnd) {
            /* 툴바 클래스 등록 */
            WNDCLASSW twc = {0};
            twc.style = CS_HREDRAW | CS_VREDRAW;
            twc.lpfnWndProc = toolbar_proc;
            twc.hInstance = GetModuleHandle(NULL);
            twc.lpszClassName = L"hgtoolbar_class";
            twc.hCursor = LoadCursor(NULL, IDC_HAND);
            RegisterClassW(&twc);

            /* UI용 일반 폰트 생성 (아이콘 크기와 분리) */
            hg_g_main_font = CreateFontW(hg_g_edit_font_size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, hg_g_font_name);
            if (!hg_g_main_font) hg_g_main_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            hg_g_toolbar_wnd = CreateWindowExW(0, L"hgtoolbar_class", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)HG_IDC_TOOLBAR, GetModuleHandle(NULL), NULL);

            hg_g_edit_msg_wnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0, hwnd, (HMENU)HG_IDC_EDIT_MSG, GetModuleHandle(NULL), NULL);
            if (hg_g_edit_msg_wnd) {
                SendMessageW(hg_g_edit_msg_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                SetWindowTextW(hg_g_edit_msg_wnd, L"RClick: Settings | Ctrl+Arrow/Wheel: Grid/Size | Alt+Arrow/Wheel: Move/Alpha");
                SetWindowSubclass(hg_g_edit_msg_wnd, edit_subclass_proc, 0, 0);
                disable_window_ime(hg_g_edit_msg_wnd);
            }

            /* 툴팁 생성: 메인 윈도우를 소유자로 지정하되 TOPMOST 유지 */
            hg_g_tooltip_wnd = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                     WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                     hwnd, NULL, GetModuleHandle(NULL), NULL);

            if (hg_g_tooltip_wnd) {
                SendMessageW(hg_g_tooltip_wnd, TTM_SETMAXTIPWIDTH, 0, SC(1000));
                /* 툴팁 폰트도 DPI 배율이 적용된 폰트로 설정 */
                SendMessageW(hg_g_tooltip_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
                /* 즉시 표시되도록 설정 (약간의 지연 시간 부여) */
                SendMessageW(hg_g_tooltip_wnd, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
                /* 시스템 테마를 해제하여 설정한 폰트와 컬러가 적용되게 함 */
                SendMessageW(hg_g_tooltip_wnd, CCM_SETWINDOWTHEME, 0, (LPARAM)L"");
                SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPBKCOLOR, hg_g_color_scheme_selected.bg, 0);
                SendMessageW(hg_g_tooltip_wnd, TTM_SETTIPTEXTCOLOR, hg_g_color_scheme_selected.text, 0);
            }

            /* taskbar_wnd creation removed */

            load_shortcuts();
            refresh_window_list(TRUE);
            update_toolbar_tooltips(hg_g_toolbar_wnd);

            /* 창 테두리 및 모서리 등 DWM 속성 설정 */
            apply_dwm_attributes(hwnd);

            SetLayeredWindowAttributes(hwnd, HG_TRANSPARENT_KEY, hg_g_taskbox_alpha, LWA_COLORKEY | LWA_ALPHA);
            SetTimer(hwnd, 1, 1000, NULL); /* 1초마다 시계 및 목록 갱신 */
            return 0;
        }

static LRESULT taskbox_controller_on_paint(HWND hwnd) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);

            /* 하이라이트 효과 (깜빡임) */
            COLORREF bg_color = HG_CLICKABLE_BG;
            if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
                bg_color = HG_COLOR_BG_FLASH;
            }
            HBRUSH hbr_bg = CreateSolidBrush(bg_color);
            if (hbr_bg) {
                FillRect(hdc, &rc, hbr_bg);
                DeleteObject(hbr_bg);
            }

            /* 외곽선 그리기 */
            int border = SC(HG_BORDER_THICKNESS);
            HBRUSH hbr_border = CreateSolidBrush(HG_COLOR_BG_TOOLBAR);
            if (hbr_border) {
                /* 상하좌우 사각형으로 채우기 */
                RECT rc_top = {0, 0, rc.right, border};
                RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
                RECT rc_left = {0, border, border, rc.bottom - border};
                RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};

                FillRect(hdc, &rc_top, hbr_border);
                FillRect(hdc, &rc_bottom, hbr_border);
                FillRect(hdc, &rc_left, hbr_border);
                FillRect(hdc, &rc_right, hbr_border);

                DeleteObject(hbr_border);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
static LRESULT taskbox_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
            int dx = 0, dy = 0;
            int move_step = SC(20);
            BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
            BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);

            /* Alt + 방향키/hjkl/wasd: 현재 창 이동 (태스크 박스) */
            if (is_alt) {
                if (w_param == VK_LEFT || w_param == 'H' || w_param == 'A') dx = -move_step;
                else if (w_param == VK_RIGHT || w_param == 'L' || w_param == 'D') dx = move_step;
                else if (w_param == VK_UP || w_param == 'K' || w_param == 'W') dy = -move_step;
                else if (w_param == VK_DOWN || w_param == 'J' || w_param == 'S') dy = move_step;

                if (dx != 0 || dy != 0) {
                    move_window_by_offset(hwnd, dx, dy);
                    /* 이동 후에도 최소한 일부는 화면에 보이도록 보호 */
                    ensure_window_visible(hwnd, L"taskbox");
                    return 0;
                }

                /* Alt + +/-: 투명도 조절 */
                if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
                    update_taskbox_alpha(1);
                    return 0;
                } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
                    update_taskbox_alpha(-1);
                    return 0;
                }
            }

            /* Ctrl + +/-: 텍스트 글꼴 크기 조절, Ctrl + 방향키/hjkl/wasd: 창 크기/그리드 조절 */
            if (is_ctrl) {
                int icon_size = ABS(hg_g_current_font_size);
                if (icon_size < SC(16)) icon_size = SC(16);

                if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
                    update_edit_font_size(1);
                    return 0;
                } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
                    update_edit_font_size(-1);
                    return 0;
                } else if (w_param == VK_LEFT || w_param == 'H' || w_param == 'A') {
                    RECT rc; GetWindowRect(hwnd, &rc);
                    int border = SC(HG_BORDER_THICKNESS);
                    int tb_width = (rc.right - rc.left) - border * 2;
                    int cols = get_items_per_row(tb_width, icon_size);
                    int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);

                    if (tb_width > exact_tb_width + SC(5)) {
                        /* Right padding exists, snap to current cols */
                    } else if (cols > 1) {
                        cols--;
                        exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
                    }

                    int new_w = exact_tb_width + border * 2;
                    SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    return 0;
                } else if (w_param == VK_RIGHT || w_param == 'L' || w_param == 'D') {
                    RECT rc; GetWindowRect(hwnd, &rc);
                    int border = SC(HG_BORDER_THICKNESS);
                    int tb_width = (rc.right - rc.left) - border * 2;
                    int cols = get_items_per_row(tb_width, icon_size);
                    cols++;
                    int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
                    int new_w = exact_tb_width + border * 2;
                    SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    return 0;
                } else if (w_param == VK_UP || w_param == 'K' || w_param == 'W') {
                    RECT rc; GetWindowRect(hwnd, &rc);
                    int border = SC(HG_BORDER_THICKNESS);
                    int tb_width = (rc.right - rc.left) - border * 2;
                    int cols = get_items_per_row(tb_width, icon_size);
                    int total_tasks = hg_g_window_count;
                    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
                    int total_items = total_tasks + total_shortcuts;
                    if (total_items <= 0) total_items = 1;

                    int current_rows = (total_items + cols - 1) / cols;
                    if (current_rows > 1) {
                        while (cols < total_items) {
                            cols++;
                            int new_rows = (total_items + cols - 1) / cols;
                            if (new_rows < current_rows) break;
                        }
                        int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
                        int new_w = exact_tb_width + border * 2;
                        SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    return 0;
                } else if (w_param == VK_DOWN || w_param == 'J' || w_param == 'S') {
                    RECT rc; GetWindowRect(hwnd, &rc);
                    int border = SC(HG_BORDER_THICKNESS);
                    int tb_width = (rc.right - rc.left) - border * 2;
                    int cols = get_items_per_row(tb_width, icon_size);
                    int total_tasks = hg_g_window_count;
                    int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
                    int total_items = total_tasks + total_shortcuts;
                    if (total_items <= 0) total_items = 1;

                    int current_rows = (total_items + cols - 1) / cols;
                    while (cols > 1) {
                        cols--;
                        int new_rows = (total_items + cols - 1) / cols;
                        if (new_rows > current_rows) break;
                    }

                    int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
                    int new_w = exact_tb_width + border * 2;
                    SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                    return 0;
                }
            }

            /* Esc: 창 닫기 */
            if (w_param == VK_ESCAPE) {
                hide_taskbox(hwnd);
                return 0;
            }
            if (msg == WM_SYSKEYDOWN) return DefWindowProcW(hwnd, msg, w_param, l_param);

            /* 탐색 및 선택 */
            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;

            if (total_tasks > 0 || total_shortcuts > 0) {
                int icon_size = ABS(hg_g_current_font_size);
                if (icon_size < SC(16)) icon_size = SC(16);

                RECT rc_toolbar;
                GetClientRect(hg_g_toolbar_wnd, &rc_toolbar);

                int cols = get_items_per_row(rc_toolbar.right, icon_size);
                int min_required_rows = (total_tasks + total_shortcuts + cols - 1) / cols;
                if (min_required_rows <= 0) min_required_rows = 1;

                int visible_rows = (rc_toolbar.bottom - SC(20) + SC(10)) / (icon_size + SC(10));
                if (visible_rows <= 0) visible_rows = 1;

                int rows = (visible_rows > min_required_rows) ? visible_rows : min_required_rows;
                int total_cells = rows * cols;

                int current_cell = -1;
                if (hg_g_focus_area == 0) {
                    current_cell = hg_g_toolbar_focus_index;
                } else {
                    current_cell = total_cells - 1 - hg_g_toolbar_focus_index;
                }

                if (current_cell < 0) current_cell = 0;

                int r = current_cell / cols;
                int c = current_cell % cols;
                BOOL changed = FALSE;

                if (w_param == VK_LEFT || w_param == 'H' || w_param == 'A') {
                    c--; changed = TRUE;
                } else if (w_param == VK_RIGHT || w_param == 'L' || w_param == 'D') {
                    c++; changed = TRUE;
                } else if (w_param == VK_UP || w_param == 'K' || w_param == 'W') {
                    r--; changed = TRUE;
                } else if (w_param == VK_DOWN || w_param == 'J' || w_param == 'S') {
                    r++; changed = TRUE;
                } else if (w_param == VK_F2) {
                    if (hg_g_floater_wnd) PostMessageW(hg_g_floater_wnd, WM_RBUTTONUP, 0, 0);
                } else if (w_param == VK_SPACE) {
                    if (hg_g_focus_area == 0) activate_taskbar_item(hg_g_toolbar_focus_index);
                    else activate_toolbar_item(hg_g_toolbar_focus_index);
                } else if (w_param == VK_RETURN) {
                    RECT rc_item;
                    get_toolbar_item_rect(hg_g_focus_area, hg_g_toolbar_focus_index, rc_toolbar.right, rc_toolbar.bottom, icon_size, &rc_item);
                    PostMessageW(hg_g_toolbar_wnd, WM_RBUTTONUP, 0, 0); // Send 0 for l_param to indicate keyboard trigger
                }

                if (changed) {
                    if (c < 0) { c = cols - 1; r--; }
                    if (c >= cols) { c = 0; r++; }
                    if (r < 0) r = 0;
                    if (r >= rows) r = rows - 1;

                    int new_cell = r * cols + c;

                    // 빈 공간이라면 가장 가까운 유효한 셀로 이동 (이 경우는 Task의 마지막이나 Shortcut의 첫번째가 될 것)
                    if (new_cell >= total_tasks && new_cell < total_cells - total_shortcuts) {
                        if (new_cell > current_cell) {
                            new_cell = total_cells - total_shortcuts;
                        } else {
                            new_cell = total_tasks - 1;
                        }
                    }
                    if (new_cell < 0) new_cell = 0;
                    if (new_cell >= total_cells) new_cell = total_cells - 1;

                    // 새로운 cell의 정보를 다시 item_type/index로 매핑
                    if (new_cell < total_tasks) {
                        hg_g_focus_area = 0;
                        hg_g_toolbar_focus_index = new_cell;
                    } else if (new_cell >= total_cells - total_shortcuts) {
                        hg_g_focus_area = 1;
                        hg_g_toolbar_focus_index = total_cells - 1 - new_cell;
                    }
                    update_focus_message(-2, -2);
                }

                InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
            }
            return 0;
        }
static LRESULT taskbox_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param) {
            if (LOWORD(w_param) == HG_IDM_CLOSE_APP) {
                if (hg_g_floater_wnd) DestroyWindow(hg_g_floater_wnd);
            } else if (LOWORD(w_param) == HG_IDM_ABOUT || LOWORD(w_param) == HG_IDM_RESET_ALL) {
                if (hg_g_floater_wnd) SendMessage(hg_g_floater_wnd, WM_COMMAND, w_param, l_param);
            } else if (LOWORD(w_param) == HG_IDM_FONT_UP) {
                update_edit_font_size(1);
            } else if (LOWORD(w_param) == HG_IDM_FONT_DOWN) {
                update_edit_font_size(-1);
            }
            return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
        }
static LRESULT taskbox_controller_on_destroy(HWND hwnd) {
            hg_g_taskbox_highlight_ticks = 0;
            KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            KillTimer(hwnd, 1);

            if (hg_g_main_font && hg_g_main_font != GetStockObject(DEFAULT_GUI_FONT)) {
                DeleteObject(hg_g_main_font);
            }
            hg_g_main_font = NULL;

            if (hg_g_edit_bg_brush) { DeleteObject(hg_g_edit_bg_brush); hg_g_edit_bg_brush = NULL; }
            if (hg_g_toolbar_btn_font) { DeleteObject(hg_g_toolbar_btn_font); hg_g_toolbar_btn_font = NULL; }
            if (hg_g_hbr_highlight) { DeleteObject(hg_g_hbr_highlight); hg_g_hbr_highlight = NULL; }
            if (hg_g_tooltip_wnd && IsWindow(hg_g_tooltip_wnd)) { DestroyWindow(hg_g_tooltip_wnd); hg_g_tooltip_wnd = NULL; }
            for (int i = 0; i < hg_g_shortcut_count; i++) {
                if (hg_g_shortcuts[i].icon) { DestroyIcon(hg_g_shortcuts[i].icon); hg_g_shortcuts[i].icon = NULL; }
            }
            for (int i = 0; i < hg_g_window_count; i++) {
                if (hg_g_window_items[i].own_icon && hg_g_window_items[i].icon) {
                    DestroyIcon(hg_g_window_items[i].icon);
                    hg_g_window_items[i].icon = NULL;
                }
            }
            /* 전역 종료는 floater_proc에서 처리하므로 주석 처리 */
            /* PostQuitMessage(0); */
            return 0;
    }

static LRESULT controlbox_controller_on_create(HWND hwnd) {
    SetLayeredWindowAttributes(hwnd, 0, hg_g_taskbox_alpha, LWA_ALPHA);
    apply_dwm_attributes(hwnd);

    hg_g_controlbox_value_wnd = CreateWindowExW(0, L"STATIC", L"System Volume: 0%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 0, 0, hwnd, (HMENU)HG_IDC_CONTROLBOX_VALUE, GetModuleHandle(NULL), NULL);
    if (hg_g_controlbox_value_wnd && hg_g_main_font) {
        SendMessageW(hg_g_controlbox_value_wnd, WM_SETFONT, (WPARAM)hg_g_main_font, TRUE);
    }

    hg_g_controlbox_slider_wnd = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        0, 0, 0, 0, hwnd, (HMENU)HG_IDC_CONTROLBOX_SLIDER, GetModuleHandle(NULL), NULL);
    if (hg_g_controlbox_slider_wnd) {
        SendMessageW(hg_g_controlbox_slider_wnd, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessageW(hg_g_controlbox_slider_wnd, TBM_SETTICFREQ, 25, 0);
        SendMessageW(hg_g_controlbox_slider_wnd, TBM_SETPAGESIZE, 0, 10);
        SendMessageW(hg_g_controlbox_slider_wnd, TBM_SETPOS, TRUE, get_system_volume());
    }

    update_controlbox_layout(hwnd);
    controlbox_sync_volume_ui();
    return 0;
}

static LRESULT controlbox_controller_on_paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HDC mem_dc = CreateCompatibleDC(hdc);
        if (mem_dc) {
            HBITMAP mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            if (mem_bm) {
                HBITMAP old_bm = (HBITMAP)SelectObject(mem_dc, mem_bm);
                if (old_bm) {
                    HBRUSH hbr_bg = hg_g_main_bg_brush ? hg_g_main_bg_brush : CreateSolidBrush(HG_COLOR_BG_DEFAULT);
                    if (hbr_bg) FillRect(mem_dc, &rc, hbr_bg);
                    if (!hg_g_main_bg_brush && hbr_bg) DeleteObject(hbr_bg);

                    int border = SC(HG_BORDER_THICKNESS);
                    HBRUSH hbr_border = CreateSolidBrush(HG_COLOR_BG_TOOLBAR);
                    if (hbr_border) {
                        RECT rc_top = {0, 0, rc.right, border};
                        RECT rc_bottom = {0, rc.bottom - border, rc.right, rc.bottom};
                        RECT rc_left = {0, border, border, rc.bottom - border};
                        RECT rc_right = {rc.right - border, border, rc.right, rc.bottom - border};
                        FillRect(mem_dc, &rc_top, hbr_border);
                        FillRect(mem_dc, &rc_bottom, hbr_border);
                        FillRect(mem_dc, &rc_left, hbr_border);
                        FillRect(mem_dc, &rc_right, hbr_border);
                        DeleteObject(hbr_border);
                    }

                    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
                    SelectObject(mem_dc, old_bm);
                }
                DeleteObject(mem_bm);
            }
            DeleteDC(mem_dc);
        }
    }

    EndPaint(hwnd, &ps);
    return 0;
}

static LRESULT controlbox_controller_on_scroll(HWND hwnd, HWND slider_hwnd) {
    (void)hwnd;
    if (!slider_hwnd || slider_hwnd != hg_g_controlbox_slider_wnd) return 0;
    int pos = (int)SendMessageW(slider_hwnd, TBM_GETPOS, 0, 0);
    set_system_volume(pos);
    controlbox_sync_volume_ui();
    return 0;
}

static LRESULT controlbox_controller_on_command(HWND hwnd, int id) {
    (void)hwnd;
    (void)id;
    return 0;
}

static LRESULT controlbox_controller_on_destroy(HWND hwnd) {
    RECT rc = {0};
    if (GetWindowRect(hwnd, &rc)) {
        save_config(L"controlbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
    hg_g_controlbox_wnd = NULL;
    hg_g_controlbox_value_wnd = NULL;
    hg_g_controlbox_slider_wnd = NULL;
    return 0;
}

LRESULT CALLBACK controlbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;
        case WM_CREATE:
            return controlbox_controller_on_create(hwnd);
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            return controlbox_controller_on_paint(hwnd);
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)l_param;
            int required_height = get_controlbox_required_height();
            mmi->ptMinTrackSize.x = SC(HG_CONTROLBOX_MIN_WIDTH);
            mmi->ptMaxTrackSize.x = SC(HG_CONTROLBOX_MAX_WIDTH);
            mmi->ptMinTrackSize.y = required_height;
            mmi->ptMaxTrackSize.y = required_height;
            return 0;
        }
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;
            return HTCAPTION;
        }
        case WM_SIZE:
            update_controlbox_layout(hwnd);
            return 0;
        case WM_HSCROLL:
            return controlbox_controller_on_scroll(hwnd, (HWND)l_param);
        case WM_COMMAND:
            return controlbox_controller_on_command(hwnd, LOWORD(w_param));
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
            BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);
            if (is_alt) {
                int dx = 0, dy = 0;
                int move_step = SC(20);
                if (w_param == VK_LEFT || w_param == 'A') dx = -move_step;
                else if (w_param == VK_RIGHT || w_param == 'D') dx = move_step;
                else if (w_param == VK_UP || w_param == 'W') dy = -move_step;
                else if (w_param == VK_DOWN || w_param == 'S') dy = move_step;
                if (dx || dy) {
                    move_window_by_offset(hwnd, dx, dy);
                    ensure_window_visible(hwnd, L"controlbox");
                    return 0;
                }
            }
            if (is_ctrl) {
                if (w_param == VK_LEFT || w_param == 'A') {
                    resize_window_by_offset(hwnd, -SC(20), 0);
                    return 0;
                }
                if (w_param == VK_RIGHT || w_param == 'D') {
                    resize_window_by_offset(hwnd, SC(20), 0);
                    return 0;
                }
            }
            if (w_param == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }
        case WM_EXITSIZEMOVE: {
            RECT rc = {0};
            if (GetWindowRect(hwnd, &rc)) {
                save_config(L"controlbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            return controlbox_controller_on_destroy(hwnd);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_DISPLAYCHANGE: {
            update_monitor_enum();
            return 0;
        }
        case WM_SETTINGCHANGE:
            if (!should_refresh_theme_on_setting_change(l_param)) {
                break;
            }
            /* fallthrough */
        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
        case WM_DWMCOLORIZATIONCOLORCHANGED: {
            refresh_theme_surfaces(hwnd);
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)l_param;
            mmi->ptMinTrackSize.x = SC(HG_MIN_WINDOW_WIDTH);
            mmi->ptMinTrackSize.y = SC(HG_MIN_WINDOW_HEIGHT);
            return 0;
        }
        case WM_CREATE:
            return taskbox_controller_on_create(hwnd);
        case WM_MOUSEWHEEL: {
            if (GetKeyState(VK_MENU) < 0) {
                /* Alt 키가 눌린 상태에서만 투명도 조절 실행 */
                short delta = (short)HIWORD(w_param);
                update_taskbox_alpha(delta > 0 ? 1 : -1);
            } else {
                /* Alt 키가 없으면 태스크바 창으로 메시지를 전달하여 창 전체에서 스크롤 가능하게 함 */
                /* Ctrl+휠(크기조절) 메시지도 태스크바 창에서 처리되도록 전달됨 */
                SendMessageW(hg_g_toolbar_wnd, WM_MOUSEWHEEL, w_param, l_param);
            }
            return 0;
        }
        case WM_NOTIFY: {
            /* ListView 관련 통지 제거됨 */
            break;
        }
        case WM_ENTERSIZEMOVE: {
            hg_g_in_sizemove = TRUE;
            GetWindowRect(hwnd, &hg_g_drag_start_rect);
            return DefWindowProcW(hwnd, msg, w_param, l_param);
        }
        case WM_EXITSIZEMOVE: {
            hg_g_in_sizemove = FALSE;
            RECT rc = {0};
            GetWindowRect(hwnd, &rc);

            int dw = (rc.right - rc.left) - (hg_g_drag_start_rect.right - hg_g_drag_start_rect.left);
            int dh = (rc.bottom - rc.top) - (hg_g_drag_start_rect.bottom - hg_g_drag_start_rect.top);

            int icon_size = ABS(hg_g_current_font_size);
            if (icon_size < SC(16)) icon_size = SC(16);
            int border = SC(HG_BORDER_THICKNESS);

            int total_tasks = hg_g_window_count;
            int total_shortcuts = hg_g_shortcut_count + HG_NUM_BASIC_ICONS;
            int total_items = total_tasks + total_shortcuts;
            if (total_items <= 0) total_items = 1;

            int cols;
            if (ABS(dh) > ABS(dw)) {
                int edit_height = SC(20);
                if (hg_g_edit_msg_wnd && hg_g_main_font) {
                    HDC hdc = GetDC(hg_g_edit_msg_wnd);
                    if (hdc) {
                        HFONT old_font = (HFONT)SelectObject(hdc, hg_g_main_font);
                        TEXTMETRIC tm = {0};
                        if (GetTextMetrics(hdc, &tm)) {
                            edit_height = (tm.tmHeight + tm.tmExternalLeading) + SC(6);
                        }
                        if (old_font) SelectObject(hdc, old_font);
                        ReleaseDC(hg_g_edit_msg_wnd, hdc);
                    }
                }

                int row_height = icon_size + SC(10);
                int current_h = rc.bottom - rc.top;
                int available_toolbar_h = current_h - (border * 2 + edit_height);
                int target_rows = (available_toolbar_h - SC(10) + row_height / 2) / row_height;
                if (target_rows < 1) target_rows = 1;
                if (target_rows > total_items) target_rows = total_items;
                cols = (total_items + target_rows - 1) / target_rows;
            } else {
                int tb_width = (rc.right - rc.left) - border * 2;
                cols = get_items_per_row(tb_width, icon_size);
            }
            if (cols <= 0) cols = 1;

            int exact_tb_width = (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
            int new_w = exact_tb_width + border * 2;

            SetWindowPos(hwnd, NULL, 0, 0, new_w, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            update_layout(hwnd); /* Will trigger snap exact height */
            GetWindowRect(hwnd, &rc);
            save_config(L"taskbox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            return 0;
        }
        case WM_TIMER:
            if (w_param == 1) {
                if (IsWindowVisible(hwnd)) {
                    refresh_window_list(FALSE);
                }
            } else if (w_param == HG_TIMER_HIGHLIGHT) {
                hg_g_taskbox_highlight_ticks--;
                if (hg_g_taskbox_highlight_ticks <= 0) {
                    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_PAINT:
            return taskbox_controller_on_paint(hwnd);
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int border = SC(HG_BORDER_THICKNESS);

            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;

            return HTCAPTION; /* 그 외 모든 클라이언트 영역은 드래그 가능하게 */
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            return taskbox_controller_on_keydown(hwnd, msg, w_param, l_param);
        case WM_COMMAND:
            return taskbox_controller_on_command(hwnd, w_param, l_param);
        case WM_SIZE: {
            update_layout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE); /* 외곽선 다시 그리기 */
            return 0;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
            /* fallthrough */
        case WM_CTLCOLORLISTBOX: {
            HDC hdc_static = (HDC)w_param;
            SetTextColor(hdc_static, hg_g_color_scheme_selected.text);
            /* 투명 배경 모드 시 글자가 겹쳐 그려지는 문제 방지를 위해 배경을 칠함 */
            SetBkMode(hdc_static, OPAQUE);

            COLORREF bg_color = hg_g_color_scheme_selected.bg;
            if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
                bg_color = HG_COLOR_BG_FLASH;
            }
            SetBkColor(hdc_static, bg_color);

            /* 하이라이트 중일 때는 브러시를 캐싱하여 반환 (GDI 누수 방지) */
            if (hg_g_taskbox_highlight_ticks > 0 && (hg_g_taskbox_highlight_ticks % 2 != 0)) {
                if (!hg_g_hbr_highlight) {
                    hg_g_hbr_highlight = CreateSolidBrush(HG_COLOR_BG_FLASH);
                }
                return hg_g_hbr_highlight ? (LRESULT)hg_g_hbr_highlight : (LRESULT)GetStockObject(BLACK_BRUSH);
            }

            if (!hg_g_edit_bg_brush) hg_g_edit_bg_brush = CreateSolidBrush(hg_g_color_scheme_selected.bg);
            return hg_g_edit_bg_brush ? (LRESULT)hg_g_edit_bg_brush : (LRESULT)GetStockObject(BLACK_BRUSH);
        }
        case WM_DESTROY:
            return taskbox_controller_on_destroy(hwnd);
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, LPWSTR cmd_line, int cmd_show) {
    HRESULT co_hr;
    BOOL com_initialized = FALSE;
    HANDLE mutex = NULL;
    HICON icon_large = NULL;
    HICON icon_small = NULL;
    HACCEL accel_table = NULL;
    int exit_code = 0;

    (void)prev_instance;
    (void)cmd_line;
    (void)cmd_show;

    /* DPI awareness must be set before UI/DC related work. */
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    co_hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    com_initialized = SUCCEEDED(co_hr);

    /* 단일 인스턴스 실행 보장 (Mutex 사용) */
    mutex = CreateMutexW(NULL, TRUE, L"Local\\hgfloater_single_instance_mutex");
    if (!mutex) {
        exit_code = 1;
        goto cleanup_finish;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing_wnd = FindWindowW(HG_CLASS_FLOATER_WIDGET, NULL);
        if (existing_wnd) {
            SetForegroundWindow(existing_wnd);
            PostMessageW(existing_wnd, WM_HOTKEY, 1, 0);
        }
        goto cleanup_finish;
    }

    init_paths();
    init_color_scheme();
    update_theme_colors();

    /* DPI 스케일 계산 */
    HDC screen_dc = GetDC(NULL);
    if (screen_dc) {
        int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
        ReleaseDC(NULL, screen_dc);
        if (dpi_x > 0) hg_g_scale_factor = dpi_x / 96.0;
    }

    /* 초기 값들에 DPI 배율 적용 */

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    /* Extract icons first to share across classes */
    icon_large = (HICON)LoadImageW(instance, MAKEINTRESOURCE(1), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    icon_small = (HICON)LoadImageW(instance, MAKEINTRESOURCE(1), IMAGE_ICON,
                                   GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    if (!hg_g_main_bg_brush) hg_g_main_bg_brush = CreateSolidBrush(HG_CLICKABLE_BG);
    if (!register_app_window_classes(instance, icon_large, icon_small)) {
        exit_code = 1;
        goto cleanup_finish;
    }

    int fx, fy, fw, fh, tx, ty, tw, th;
    load_config(L"floater", &fx, &fy, &fw, &fh, 100, 100, SC(80), SC(55));
    load_config(L"taskbox", &tx, &ty, &tw, &th, 200, 200, SC(HG_WINDOW_WIDTH), SC(HG_WINDOW_HEIGHT));
    hg_g_floater_alpha = (BYTE)get_alpha_config(L"floater", 204);
    hg_g_taskbox_alpha = (BYTE)get_alpha_config(L"taskbox", 204);
    load_font_name_config();
    load_hotkey_config();
    load_floater_font_config();
    load_taskbox_font_config();

    hg_g_floater_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_APPWINDOW,
        HG_CLASS_FLOATER_WIDGET, L"floater", WS_POPUP | WS_VISIBLE,
        fx, fy, fw, fh, NULL, NULL, instance, NULL);
    if (!hg_g_floater_wnd) {
        MessageBoxW(NULL, L"Failed to create floater window.", L"hgfloater", MB_ICONERROR);
        exit_code = 1;
        goto cleanup_finish;
    }

    hg_g_hotkey_registered = RegisterHotKey(hg_g_floater_wnd, 1,
        hg_g_hotkey_modifiers | MOD_NOREPEAT, hg_g_hotkey_key);
    if (!hg_g_hotkey_registered) {
        MessageBoxW(NULL,
            L"Global hotkey registration failed. Another program may be using the assigned hotkey.",
            L"hgfloater", MB_ICONWARNING);
    }

    hg_g_taskbox_wnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        HG_CLASS_TASKBOX, L"taskbox", WS_POPUP,
        tx, ty, tw, th, NULL, NULL, instance, NULL);
    if (!hg_g_taskbox_wnd) {
        MessageBoxW(NULL, L"Failed to create taskbox window.", L"hgfloater", MB_ICONERROR);
        exit_code = 1;
        goto cleanup_finish;
    }

    update_monitor_enum();

    ACCEL accel[] = {
        { FCONTROL | FVIRTKEY, 'Q', HG_IDM_CLOSE_APP },
        { FCONTROL | FVIRTKEY, 'X', HG_IDM_CLOSE_APP },
        { FALT     | FVIRTKEY, VK_F4, HG_IDM_CLOSE_APP },
        { FVIRTKEY, VK_F1, HG_IDM_ABOUT },
        { FCONTROL | FSHIFT | FVIRTKEY, 'R', HG_IDM_RESET_ALL },
        { FCONTROL | FVIRTKEY, 'R', HG_IDM_RESET_ALL },
        { FVIRTKEY, VK_F5, HG_IDM_RESET_ALL },
        { FCONTROL | FVIRTKEY, '0', HG_IDM_RESET_ALL },
        { FCONTROL | FVIRTKEY, VK_OEM_PLUS, HG_IDM_FONT_UP },
        { FCONTROL | FVIRTKEY, VK_OEM_MINUS, HG_IDM_FONT_DOWN },
        { FCONTROL | FVIRTKEY, VK_ADD, HG_IDM_FONT_UP },
        { FCONTROL | FVIRTKEY, VK_SUBTRACT, HG_IDM_FONT_DOWN }
    };
    accel_table = CreateAcceleratorTableW(accel, HG_ARRAYSIZE(accel));

    MSG msg_struct;
    BOOL bRet;
    while ((bRet = GetMessageW(&msg_struct, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
            exit_code = 1;
            break;
        }

        BOOL accel_handled = FALSE;
        if (accel_table && hg_g_taskbox_wnd && TranslateAcceleratorW(hg_g_taskbox_wnd, accel_table, &msg_struct)) {
            accel_handled = TRUE;
        }
        if (!accel_handled && accel_table && hg_g_floater_wnd &&
            TranslateAcceleratorW(hg_g_floater_wnd, accel_table, &msg_struct)) {
            accel_handled = TRUE;
        }

        if (!accel_handled) {
            TranslateMessage(&msg_struct);
            DispatchMessageW(&msg_struct);
        }
    }

    save_alpha_config();

cleanup_finish:
    if (accel_table) {
        DestroyAcceleratorTable(accel_table);
        accel_table = NULL;
    }

    if (hg_g_hotkey_registered && hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        UnregisterHotKey(hg_g_floater_wnd, 1);
        hg_g_hotkey_registered = FALSE;
    }

    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        DestroyWindow(hg_g_taskbox_wnd);
        hg_g_taskbox_wnd = NULL;
    }
    if (hg_g_controlbox_wnd && IsWindow(hg_g_controlbox_wnd)) {
        DestroyWindow(hg_g_controlbox_wnd);
        hg_g_controlbox_wnd = NULL;
    }
    if (hg_g_floater_wnd && IsWindow(hg_g_floater_wnd)) {
        DestroyWindow(hg_g_floater_wnd);
        hg_g_floater_wnd = NULL;
    }

    unregister_app_window_classes(instance);

    if (mutex) {
        CloseHandle(mutex);
        mutex = NULL;
    }
    if (hg_g_main_bg_brush) {
        DeleteObject(hg_g_main_bg_brush);
        hg_g_main_bg_brush = NULL;
    }
    if (icon_large) DestroyIcon(icon_large);
    if (icon_small) DestroyIcon(icon_small);
    if (com_initialized) CoUninitialize();
    return exit_code;
}

