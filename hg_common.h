#ifndef HG_COMMON_H
#define HG_COMMON_H

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
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

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
#pragma comment(                                                                                                       \
    linker,                                                                                                            \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
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

/* Build-time versioning */
#ifndef HG_VERSION_W
#define HG_VERSION_W L"v26.05.31"
#endif

/* Include generated About text from README.md if available */
#if __has_include("hg_about_text.h")
#include "hg_about_text.h"
#endif

#ifndef HG_ABOUT_README_W
#define HG_ABOUT_README_W L"(Additional information from README.md could not be loaded.)"
#endif

#define HG_ABOUT_FIXED_W                                                                                               \
    L"hgfloater " HG_VERSION_W L"\r\n"                                                                                 \
    L"A lightweight WinAPI-based floating widget & task switcher.\r\n\r\n"                                             \
    L"Homepage:\r\n"                                                                                                   \
    L"https://github.com/rubidus-api/hgfloater\r\n\r\n"                                                                \
    L"Developer: rubidus-api (rubidus@gmail.com)\r\n\r\n"                                                              \
    L"License: MIT License\r\n\r\n"                                                                                    \
    L"--- How to use Link ---\r\n"                                                                                     \
    L"You can select and copy the URL/Email above using context menu or Ctrl+C.\r\n"                                   \
    L"------------------------------\r\n\r\n"

#define HG_ABOUT_TEXT_W HG_ABOUT_FIXED_W HG_ABOUT_README_W

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
#define HG_WINDOW_WIDTH 480
#define HG_WINDOW_HEIGHT 160
#define HG_MIN_WINDOW_WIDTH 64
#define HG_MIN_WINDOW_HEIGHT 32

#define HG_MAX_PATH 32768
#define HG_MAX_STR 1024

#define HG_MAX_TITLE_LEN HG_MAX_STR
#define HG_MAX_WINDOW_ITEMS 1024
#define HG_MAX_SHORTCUTS 64
#define HG_MAX_AUDIO_DEVICES 16
#define HG_NUM_BASIC_ICONS 9

#define HG_TOOL_ICON_RESIZE 0
#define HG_TOOL_ICON_MOVE 1
#define HG_TOOL_ICON_CLOSE 2
#define HG_TOOL_ICON_DESKTOP 3
#define HG_TOOL_ICON_MENU 4
#define HG_TOOL_ICON_COMMAND 5
#define HG_TOOL_ICON_ALPHA 6
#define HG_TOOL_ICON_BRIGHTNESS 7
#define HG_TOOL_ICON_VOLUME 8

#define HG_IDM_MINIMIZE 201
#define HG_IDM_CLOSE 202
#define HG_IDM_MOVE 203
#define HG_IDM_SIZE 204
#define HG_IDM_CLOSE_APP 205
#define HG_IDM_CLEAR_EDIT 206
#define HG_IDM_EDIT_COPYALL 207
#define HG_IDM_ABOUT 208
#define HG_IDM_RESET_ALL 209
#define HG_IDM_FONT_UP 210
#define HG_IDM_FONT_DOWN 211
#define HG_IDM_POWER_OFF 212
#define HG_IDM_MUTE 214
#define HG_IDM_OPEN_SHORTCUTS 215
#define HG_IDM_EDIT_CONFIG 216

#define HG_COPYDATA_COMMAND_LINE 0x4847434CU

#define HG_IDM_TASK_MOVETO_0_0 300
#define HG_IDM_TASK_CLOSE 301
#define HG_IDM_TASK_RESIZE_4_3_1 302
#define HG_IDM_TASK_RESIZE_4_3_2 303
#define HG_IDM_TASK_RESIZE_4_3_3 304
#define HG_IDM_TASK_RESIZE_16_9_1 305
#define HG_IDM_TASK_RESIZE_16_9_2 306
#define HG_IDM_TASK_RESIZE_16_9_3 307
#define HG_IDM_TASK_RESIZE_16_9_4 308
#define HG_IDM_TASK_RESIZE_9_16_1 309
#define HG_IDM_TASK_RESIZE_9_16_2 310
#define HG_IDM_TASK_RESIZE_9_16_3 311
#define HG_IDM_TASK_RESIZE_9_16_4 312
#define HG_IDM_TASK_RESTORE 313

#define HG_IDM_VOLUME_PERCENT 400
#define HG_IDM_VOLUME_SET_0 401
#define HG_IDM_VOLUME_SET_25 402
#define HG_IDM_VOLUME_SET_50 403
#define HG_IDM_VOLUME_SET_75 404
#define HG_IDM_VOLUME_SET_100 405
#define HG_IDM_AUDIO_DEVICE_BASE 5000
#define HG_IDM_MONITOR_BASE 6000

#define HG_MAX_MONITORS 10

#define HG_IDM_SHORTCUT_RUN (UINT_MAX - 100)
#define HG_IDM_SHORTCUT_OPEN_DIR (UINT_MAX - 101)

#define HG_TASKBOX_EDIT_CONTROL_TRIM_THRESHOLD 64
#define HG_TASKBOX_EDIT_CONTROL_RETAIN_COUNT 32

#define HG_TIMER_HIGHLIGHT 1001
#define HG_TIMER_HOVER_CHECK 1002
#define HG_HIGHLIGHT_TICKS 6

#define HG_TRANSPARENT_KEY RGB(1, 2, 3)
#define HG_BORDER_THICKNESS 5
#define HG_FLOATER_MIN_FONT_SIZE 12
#define HG_FLOATER_MAX_FONT_SIZE 96

#define HG_THEME_CUSTOM_BG RGB(20, 20, 20)
#define HG_THEME_CUSTOM_BORDER RGB(160, 160, 160)
#define HG_THEME_CUSTOM_TEXT RGB(255, 255, 255)
#define HG_THEME_CUSTOM_FLASH RGB(180, 130, 240)
#define HG_THEME_CUSTOM_SELECTED RGB(80, 80, 80)

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

#define HG_COLOR_BG_DEFAULT hg_g_color_scheme_selected.bg
#define HG_COLOR_BG_TOOLBAR hg_g_color_scheme_selected.border
#define HG_COLOR_BG_FLASH hg_g_color_scheme_selected.flash
#define HG_COLOR_BG_SELECTED hg_g_color_scheme_selected.selected
#define HG_COLOR_BORDER_SELECTED hg_g_color_scheme_selected.flash
#define HG_COLOR_TEXT_DEFAULT hg_g_color_scheme_selected.text
#define HG_CLICKABLE_BG hg_g_color_scheme_selected.bg

#define HG_MIN_ALPHA ((int)(255 * (100 - 70) / 100))
#define HG_MAX_ALPHA 255

#ifndef ICON_SMALL2
#define ICON_SMALL2 2
#endif

#define HG_UWP_MAX_MANIFEST_BYTES (1024u * 1024u)
#define HG_UWP_MIN_ICON_PX 16
#define HG_UWP_MAX_ICON_PX 256

/* =========================================================================
 * 데이터 구조체 (Data Structures)
 * ========================================================================= */
typedef struct {
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

typedef struct WindowClassSpec {
    const WCHAR *class_name;
    WNDPROC wnd_proc;
    HBRUSH background;
    const WCHAR *fail_message;
} WindowClassSpec;

typedef enum HgCliAction {
    HG_CLI_ACTION_DEFAULT = 0,
    HG_CLI_ACTION_SHOW,
    HG_CLI_ACTION_HIDE,
    HG_CLI_ACTION_TOGGLE,
    HG_CLI_ACTION_ABOUT,
    HG_CLI_ACTION_EXIT,
    HG_CLI_ACTION_HELP
} HgCliAction;

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
 * 유틸리티 함수 & 매크로 (Utility Functions & Macros)
 * ========================================================================= */
#include "hg_globals.h"

static inline int sc(int x)
{
    return (int)(x * hg_g_scale_factor + (x >= 0 ? 0.5 : -0.5));
}
#define SC(x) sc(x)

static inline int hellgates_abs(int x)
{
    return (x < 0) ? -x : x;
}
#define ABS(x) hellgates_abs(x)

/* =========================================================================
 * 함수 포워드 선언 (Function Prototypes)
 * ========================================================================= */
void refresh_window_list(BOOL force);
void update_layout(HWND hwnd);
void update_floater_layout(HWND hwnd);
void update_focus_message(int override_type, int override_index);
void reset_taskbox_focus(void);
LRESULT CALLBACK toolbar_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK edit_subclass_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param, UINT_PTR mid_subclass,
                                    DWORD_PTR dw_ref_data);
LRESULT CALLBACK about_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK monitor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK commandbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void save_config(const WCHAR *section, int x, int y, int w, int h);
void save_window_geometry_config(const WCHAR *section, int x, int y, int w, int h);
void save_floater_geometry_config(int x, int y, int w, int h);
void save_taskbox_geometry_config(int x, int y, int w, int h);
void save_commandbox_geometry_config(int x, int y, int w, int h);
void save_floater_font_config(void);
void save_taskbox_font_config(void);
void load_commandbox_font_config(void);
void save_commandbox_font_config(void);
void save_font_name_config(void);
void save_commandbox_alpha_config(void);
BOOL register_global_hotkey(HWND hwnd, BOOL warn_on_failure);
void unregister_global_hotkey(HWND hwnd);
void hg_config_reset_all(HWND hwnd);
void hide_taskbox(HWND hwnd);
void show_commandbox_window(void);
void load_commandbox_font(void);
void ensure_window_visible(HWND hwnd, const WCHAR *section);
void toggle_monitor_window(int idx);
LRESULT handle_copydata_command_line(const COPYDATASTRUCT *cds);
void show_about_window(void);



static const WCHAR HG_CLASS_FLOATER_WIDGET[] = L"hgfloater_widget_class";
static const WCHAR HG_CLASS_ABOUT[] = L"hgabout_class";
static const WCHAR HG_CLASS_TASKBOX[] = L"hgfloater_class";
static const WCHAR HG_CLASS_MONITOR[] = L"hgmonitor_class";
static const WCHAR HG_CLASS_COMMANDBOX[] = L"hgcommandbox_class";
static const WCHAR HG_SINGLE_INSTANCE_MUTEX_NAME[] = L"Local\\hgfloater_single_instance_mutex";

#endif /* HG_COMMON_H */
