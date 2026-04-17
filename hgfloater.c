#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <time.h>
#include <psapi.h>

/* 라이브러리 명시적 링크 */
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "psapi.lib")

/* 비주얼 스타일(Common Controls 6.0) 사용을 위한 매니페스트 설정 */
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

/* 툴팁 구조체 호환 크기 정의 */
#if defined(_WIN64)
#define TOOLINFO_V1_SIZE 64
#else
#define TOOLINFO_V1_SIZE 44
#endif

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

/* 
 * HGFloater (HellGates Series)
 * - Drag & Drop Reordering
 * - Real-time Clock Titlebar
 * - Fixed List Order (Incremental Update)
 * 
 * [Resource Audit]
 * - Stack: Large arrays (windowItems, shortcuts) moved to global scope to prevent stack overflow.
 * - Memory: Shortcut icons are destroyed before reload; GDI objects in WM_PAINT are properly released.
 * - Performance: Double buffering prevents flickering; incremental window list updates reduce CPU load.
 */

#define IDC_LISTBOX 101
#define IDC_TOOLBAR 102
#define IDC_EDIT_MSG 103
#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 160
#define MIN_WINDOW_WIDTH 64
#define MIN_WINDOW_HEIGHT 32

#define MAX_TITLE_LEN 1024
#define MAX_WINDOW_ITEMS 1024
#define MAX_SHORTCUTS 64

#define IDM_MINIMIZE 201
#define IDM_CLOSE    202
#define IDM_MOVE     203
#define IDM_SIZE     204
#define IDM_CLOSE_APP 205
#define IDM_CLEAR_EDIT 206
#define IDM_EDIT_COPYALL 207

#define TIMER_HIGHLIGHT 1001
#define HIGHLIGHT_TICKS 6 /* 3번 깜빡임 (On/Off) */

#define TRANSPARENT_KEY RGB(1, 2, 3)
#define COLOR_PASTEL_GREEN RGB(220, 255, 220)
#define COLOR_PASTEL_BLUE  RGB(180, 220, 255)
#define COLOR_TEXT_DARK    RGB(80, 80, 80)
#define CLICKABLE_BG       COLOR_PASTEL_GREEN
#define BORDER_THICKNESS   3

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#define DWMWCP_DONOTROUND 2

/* DPI 스케일링 관련 */
double g_scale = 1.0;
#define SC(x) ((int)((x) * g_scale))

HWND hTaskBarWnd;
HWND hToolbarWnd;
HWND hEditMsg;
HWND hToolTip;
HWND hMainWnd;
HWND hFloaterWnd;
HFONT hFont;
HBRUSH hbrMainBg = NULL;
HBRUSH hbrEditBg = NULL;
int currentFontSize = -22; /* 초기 아이콘 크기 (기존 32의 약 2/3) */
BYTE currentAlpha = 204;   /* 초기 투명도 20% (Alpha 80%) */
int g_floaterHighlightTicks = 0;
int g_taskBoxHighlightTicks = 0;
int g_focusArea = 0; /* 0: Toolbar, 1: ListView */
int g_toolbarFocusIndex = 0;
UINT g_hotkeyModifiers = MOD_WIN | MOD_ALT;
UINT g_hotkeyKey = VK_SPACE;

int g_scrollX = 0;
int g_scrollY = 0;
WCHAR g_basePath[MAX_PATH];
WCHAR g_shortcutsPath[MAX_PATH];
WCHAR g_configPath[MAX_PATH];

void InitPaths() {
    WCHAR profile[MAX_PATH];
    GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    swprintf(g_basePath, MAX_PATH, L"%ls\\.HellGates\\HGFloater", profile);
    swprintf(g_shortcutsPath, MAX_PATH, L"%ls\\shortcuts", g_basePath);
    swprintf(g_configPath, MAX_PATH, L"%ls\\config.ini.txt", g_basePath);
    
    SHCreateDirectoryExW(NULL, g_basePath, NULL);
    SHCreateDirectoryExW(NULL, g_shortcutsPath, NULL);
}

typedef struct {
    WCHAR path[MAX_PATH];
    WCHAR name[MAX_PATH];
    HICON hIcon;
} ShortcutItem;

ShortcutItem shortcuts[MAX_SHORTCUTS];
int shortcutCount = 0;

typedef struct {
    HWND hwnd;
    HICON hIcon;
    BOOL bOwnIcon;
    WCHAR title[MAX_TITLE_LEN];
    WCHAR processName[MAX_PATH];
    DWORD processId;
    BOOL exists;
    int imageIndex;
} WindowItem;

WindowItem windowItems[MAX_WINDOW_ITEMS];
WindowItem newItems[MAX_WINDOW_ITEMS]; /* RefreshWindowList용 임시 배열 */
int windowCount = 0;

void RefreshWindowList(BOOL force);
void UpdateLayout(HWND hwnd);
void UpdateFocusMessage();
LRESULT CALLBACK TaskBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* 드래그 앤 드롭 관련 상태 */
BOOL isDragging = FALSE;
int dragSourceIndex = -1;
POINT dragStartPt = {0, 0};

/* 창 필터링: 윈도우 11 작업 표시줄과 유사한 로직 */
BOOL IsAltTabWindow(HWND hwnd) {
    if (hwnd == hMainWnd) return FALSE;
    if (!IsWindow(hwnd)) return FALSE;
    if (!IsWindowVisible(hwnd)) return FALSE;
    
    /* 텍스트가 없는 창은 제외 */
    if (GetWindowTextLengthW(hwnd) == 0) return FALSE;

    /* 도구 창 제외 */
    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return FALSE;

    /* 소유자가 있는 창은 기본적으로 제외 (단, 앱 윈도우 스타일이 있으면 포함) */
    HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    if (hwndOwner != NULL && !(exStyle & WS_EX_APPWINDOW)) return FALSE;

    /* Cloaked 상태 체크 (가상 데스크톱 등 숨겨진 창 제외) */
    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        if (cloaked != 0) return FALSE; 
    }

    return TRUE;
}

/* 프로세스 이름 가져오기 */
void GetProcessNameByHWND(HWND hwnd, WCHAR* outName, DWORD* outPid) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (outPid) *outPid = pid;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        WCHAR path[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
            WCHAR* exeName = wcsrchr(path, L'\\');
            if (exeName) wcscpy(outName, exeName + 1);
            else wcscpy(outName, path);
        } else {
            wcscpy(outName, L"Unknown");
        }
        CloseHandle(hProcess);
    } else {
        wcscpy(outName, L"Unknown");
    }
}

HICON GetWindowIcon(HWND hwnd, int targetSize, BOOL* pbOwnIcon) {
    if (pbOwnIcon) *pbOwnIcon = FALSE;
    HICON hIcon = NULL;
    
    /* 큰 사이즈를 원할 경우 BIG 아이콘 우선 시도 */
    if (abs(targetSize) > 32) {
        hIcon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_BIG, 0);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    } else {
        hIcon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0);
        if (!hIcon) hIcon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_BIG, 0);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    }
    
    /* 여전히 없거나 고해상도가 필요한 경우 프로세스 파일에서 직접 추출 */
    if (!hIcon || abs(targetSize) > 32) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            WCHAR path[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                SHFILEINFOW sfi = {0};
                if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
                    if (hIcon && pbOwnIcon && !*pbOwnIcon) {
                        /* 기존 아이콘이 작거나 품질이 낮으면 교체 */
                        hIcon = sfi.hIcon;
                        if (pbOwnIcon) *pbOwnIcon = TRUE;
                    } else if (!hIcon) {
                        hIcon = sfi.hIcon;
                        if (pbOwnIcon) *pbOwnIcon = TRUE;
                    } else {
                        DestroyIcon(sfi.hIcon);
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }
    return hIcon;
}

/* 창 목록 정렬 제거 (사용자 순서 유지) */

/* 단축아이콘 정렬을 위한 비교 함수 */
int CompareShortcuts(const void* a, const void* b) {
    ShortcutItem* itemA = (ShortcutItem*)a;
    ShortcutItem* itemB = (ShortcutItem*)b;
    return _wcsicmp(itemA->name, itemB->name);
}

/* 단축아이콘 로드 */
void LoadShortcuts() {
    /* 기존 아이콘 정리 */
    for(int i=0; i<shortcutCount; i++) if(shortcuts[i].hIcon) DestroyIcon(shortcuts[i].hIcon);
    shortcutCount = 0;

    WCHAR searchPath[MAX_PATH];
    swprintf(searchPath, MAX_PATH, L"%ls\\*", g_shortcutsPath);
    
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(searchPath, &ffd);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                /* 확장자 확인 (.lnk 및 .url 허용) */
                size_t len = wcslen(ffd.cFileName);
                BOOL isLnk = (len > 4 && _wcsicmp(ffd.cFileName + len - 4, L".lnk") == 0);
                BOOL isUrl = (len > 4 && _wcsicmp(ffd.cFileName + len - 4, L".url") == 0);

                if (isLnk || isUrl) {
                    swprintf(shortcuts[shortcutCount].path, MAX_PATH, L"%ls\\%ls", g_shortcutsPath, ffd.cFileName);
                    
                    /* 이름 저장 (확장자 제외) */
                    wcscpy(shortcuts[shortcutCount].name, ffd.cFileName);
                    WCHAR* dot = wcsrchr(shortcuts[shortcutCount].name, L'.');
                    if (dot) *dot = L'\0';

                    SHFILEINFOW sfi = {0};
                    if (SHGetFileInfoW(shortcuts[shortcutCount].path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
                        shortcuts[shortcutCount].hIcon = sfi.hIcon;
                    }
                    
                    shortcutCount++;
                    if (shortcutCount >= MAX_SHORTCUTS) break;
                }
            }
        } while (FindNextFileW(hFind, &ffd));
        FindClose(hFind);
    }

    /* 파일명 기준 정렬 */
    if (shortcutCount > 1) {
        qsort(shortcuts, shortcutCount, sizeof(ShortcutItem), CompareShortcuts);
    }
}

/* 메시지 에디트 컨트롤에 텍스트 추가 및 스크롤 (최대 100행 유지) */
void AppendMessage(const WCHAR* msg) {
    if (!hEditMsg) return;
    
    int lineCount = (int)SendMessageW(hEditMsg, EM_GETLINECOUNT, 0, 0);
    if (lineCount > 100) {
        int firstLineEnd = (int)SendMessageW(hEditMsg, EM_LINEINDEX, 1, 0);
        if (firstLineEnd != -1) {
            SendMessageW(hEditMsg, EM_SETSEL, 0, firstLineEnd);
            SendMessageW(hEditMsg, EM_REPLACESEL, FALSE, (LPARAM)L"");
        }
    }
    
    int len = GetWindowTextLengthW(hEditMsg);
    SendMessageW(hEditMsg, EM_SETSEL, len, len);
    if (len > 0) SendMessageW(hEditMsg, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    SendMessageW(hEditMsg, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageW(hEditMsg, EM_SCROLLCARET, 0, 0);
}

/* 외곽선 있는 텍스트 그리기 헬퍼 */
void DrawOutlinedTextW(HDC hdc, const WCHAR* text, int len, RECT* rc, UINT format, COLORREF textColor, COLORREF outlineColor) {
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldTextColor = GetTextColor(hdc);
    
    SetTextColor(hdc, outlineColor);
    RECT tempRc;
    /* 8방향 외곽선으로 가독성 극대화 */
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int i = 0; i < 8; i++) {
        tempRc = *rc;
        OffsetRect(&tempRc, dx[i], dy[i]);
        DrawTextW(hdc, text, len, &tempRc, format);
    }
    
    SetTextColor(hdc, textColor);
    DrawTextW(hdc, text, len, rc, format);
    
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);
}

/* 개별 툴바 아이템의 위치 계산 (통합 레이아웃 엔진) */
int GetItemsPerRow(int width, int iconSize) {
    if (width <= 0) return 1;
    int n = (width - iconSize - SC(20)) / (iconSize + SC(15)) + 1;
    return (n > 0) ? n : 1;
}

void GetToolbarItemRect(int index, int width, int iconSize, RECT* outRect) {
    if (index < 0 || width <= 0 || !outRect) {
        if (outRect) SetRectEmpty(outRect);
        return;
    }
    int x = SC(10), y = SC(10);
    int rowHeight = iconSize + SC(10);
    
    for (int i = 0; i <= index; i++) {
        if (i > 0 && x + iconSize + SC(10) > width) {
            x = SC(10);
            y += rowHeight;
        }
        if (i == index) {
            outRect->left = x;
            outRect->top = y;
            outRect->right = x + iconSize;
            outRect->bottom = y + iconSize;
            return;
        }
        x += iconSize + SC(15);
    }
}

/* 툴바 높이 계산 (통합 레이아웃 사용) */
int GetToolbarHeight(int width) {
    if (width <= 0) return 0;
    int iconSize = abs(currentFontSize);
    if (iconSize < SC(16)) iconSize = SC(16);
    
    RECT rcLast;
    int totalItems = shortcutCount + 4;
    if (totalItems <= 0) return SC(20);
    
    GetToolbarItemRect(totalItems - 1, width, iconSize, &rcLast);
    return rcLast.bottom + SC(10);
}

int GetTaskBarHeight(int width) {
    if (width <= 0) return 0;
    int iconSize = abs(currentFontSize);
    if (iconSize < SC(16)) iconSize = SC(16);
    
    RECT rcLast;
    if (windowCount <= 0) return SC(20);
    
    GetToolbarItemRect(windowCount - 1, width, iconSize, &rcLast);
    return rcLast.bottom + SC(10);
}

/* 툴바 툴팁 업데이트 (통합 레이아웃 사용) */
void UpdateToolbarTooltips(HWND hwnd) {
    static int lastTotalItems = 0;
    if (!hToolTip || !hwnd) return;

    /* 이전 툴들 제거 */
    for (int i = 0; i < lastTotalItems; i++) {
        TOOLINFOW ti = { 0 };
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.hwnd = hwnd;
        ti.uId = i;
        SendMessageW(hToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }

    RECT clientRc;
    GetClientRect(hwnd, &clientRc);
    if (clientRc.right <= 0) return;

    int iconSize = abs(currentFontSize);
    if (iconSize < SC(16)) iconSize = SC(16);
    int totalItems = shortcutCount + 4;
    for (int i = 0; i < totalItems; i++) {
        RECT rcItem;
        GetToolbarItemRect(i, clientRc.right, iconSize, &rcItem);

        TOOLINFOW tiTool = { 0 };
        tiTool.cbSize = TOOLINFO_V1_SIZE;
        tiTool.uFlags = TTF_SUBCLASS;
        tiTool.hwnd = hwnd;
        tiTool.uId = i;
        if (i == 0) tiTool.lpszText = L"Drag to Resize Window";
        else if (i == 1) tiTool.lpszText = L"Hide Dashboard";
        else if (i == 2) tiTool.lpszText = L"Open Shortcuts Folder";
        else if (i == 3) tiTool.lpszText = L"Show Desktop";
        else tiTool.lpszText = shortcuts[i - 4].name;

        /* 인식 영역을 아이콘 크기에 맞게 확장 */
        tiTool.rect = rcItem;
        InflateRect(&tiTool.rect, SC(4), SC(4));

        SendMessageW(hToolTip, TTM_ADDTOOLW, 0, (LPARAM)&tiTool);
    }
    
    SendMessageW(hToolTip, TTM_ACTIVATE, TRUE, 0);
    lastTotalItems = totalItems;
}

void UpdateTaskBarTooltips(HWND hwnd) {
    static int lastTotalItems = 0;
    if (!hToolTip || !hwnd) return;

    for (int i = 0; i < lastTotalItems; i++) {
        TOOLINFOW ti = { 0 };
        ti.cbSize = TOOLINFO_V1_SIZE;
        ti.hwnd = hwnd;
        ti.uId = i + 1000;
        SendMessageW(hToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }

    RECT clientRc;
    GetClientRect(hwnd, &clientRc);
    if (clientRc.right <= 0) return;

    int iconSize = abs(currentFontSize);
    if (iconSize < SC(16)) iconSize = SC(16);
    for (int i = 0; i < windowCount; i++) {
        RECT rcItem;
        GetToolbarItemRect(i, clientRc.right, iconSize, &rcItem);

        TOOLINFOW tiTool = { 0 };
        tiTool.cbSize = TOOLINFO_V1_SIZE;
        tiTool.uFlags = TTF_SUBCLASS;
        tiTool.hwnd = hwnd;
        tiTool.uId = i + 1000;
        tiTool.lpszText = windowItems[i].title;
        tiTool.rect = rcItem;
        InflateRect(&tiTool.rect, SC(4), SC(4));

        SendMessageW(hToolTip, TTM_ADDTOOLW, 0, (LPARAM)&tiTool);
    }
    SendMessageW(hToolTip, TTM_ACTIVATE, TRUE, 0);
    lastTotalItems = windowCount;
}

/* 바탕화면 보기 토글을 위한 EnumWindows 콜백 */
BOOL CALLBACK MinimizeRestoreEnumProc(HWND hwnd, LPARAM lParam) {
    BOOL isMinimize = (BOOL)lParam;
    if (hwnd == hMainWnd) return TRUE;
    if (!IsWindowVisible(hwnd)) return TRUE;

    /* 소유자가 없는 최상위 창이거나 작업 표시줄에 나타나는 창 스타일인 경우 */
    HWND owner = GetWindow(hwnd, GW_OWNER);
    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    
    if (owner == NULL || (exStyle & WS_EX_APPWINDOW)) {
        /* Cloaked 상태 체크 (가상 데스크톱 등 숨겨진 창 제외) */
        int cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
            if (cloaked != 0) return TRUE; 
        }

        if (isMinimize) {
            if (!IsIconic(hwnd)) ShowWindowAsync(hwnd, SW_MINIMIZE);
        } else {
            if (IsIconic(hwnd)) ShowWindowAsync(hwnd, SW_RESTORE);
        }
    }
    return TRUE;
}

/* INI 설정 로드/저장 */
void LoadConfig(const WCHAR* section, int* x, int* y, int* w, int* h, int defX, int defY, int defW, int defH) {
    *x = GetPrivateProfileIntW(section, L"X", defX, g_configPath);
    *y = GetPrivateProfileIntW(section, L"Y", defY, g_configPath);
    *w = GetPrivateProfileIntW(section, L"W", defW, g_configPath);
    *h = GetPrivateProfileIntW(section, L"H", defH, g_configPath);
}

void SaveConfig(const WCHAR* section, int x, int y, int w, int h) {
    WCHAR buf[32];
    swprintf(buf, 32, L"%d", x);
    WritePrivateProfileStringW(section, L"X", buf, g_configPath);
    swprintf(buf, 32, L"%d", y);
    WritePrivateProfileStringW(section, L"Y", buf, g_configPath);
    swprintf(buf, 32, L"%d", w);
    WritePrivateProfileStringW(section, L"W", buf, g_configPath);
    swprintf(buf, 32, L"%d", h);
    WritePrivateProfileStringW(section, L"H", buf, g_configPath);
}

void LoadHotkeyConfig() {
    g_hotkeyModifiers = GetPrivateProfileIntW(L"Hotkey", L"Modifiers", MOD_WIN | MOD_ALT, g_configPath);
    g_hotkeyKey = GetPrivateProfileIntW(L"Hotkey", L"Key", VK_SPACE, g_configPath);
}

void SaveHotkeyConfig() {
    WCHAR buf[32];
    swprintf(buf, 32, L"%u", g_hotkeyModifiers);
    WritePrivateProfileStringW(L"Hotkey", L"Modifiers", buf, g_configPath);
    swprintf(buf, 32, L"%u", g_hotkeyKey);
    WritePrivateProfileStringW(L"Hotkey", L"Key", buf, g_configPath);
}

void MoveWindowByOffset(HWND hwnd, int dx, int dy) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    SetWindowPos(hwnd, NULL, rc.left + dx, rc.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    /* Save position and size after movement */
    if (hwnd == hFloaterWnd) SaveConfig(L"Floater", rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
    else if (hwnd == hMainWnd) SaveConfig(L"TaskBox", rc.left + dx, rc.top + dy, rc.right - rc.left, rc.bottom - rc.top);
}

void UpdateSize(int delta) {
    int newSize = abs(currentFontSize) + (delta > 0 ? SC(2) : -SC(2));
    if (newSize < 16) newSize = 16;
    if (newSize > 128) newSize = 128;
    currentFontSize = -newSize;

    RECT rc; GetClientRect(hMainWnd, &rc);
    SendMessageW(hMainWnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
    RefreshWindowList(TRUE);
}

void UpdateAlpha(int delta) {
    int newAlpha = (int)currentAlpha + (delta > 0 ? 15 : -15);
    if (newAlpha > 255) newAlpha = 255;
    if (newAlpha < 128) newAlpha = 128;
    currentAlpha = (BYTE)newAlpha;
    SetLayeredWindowAttributes(hFloaterWnd, TRANSPARENT_KEY, currentAlpha, LWA_COLORKEY | LWA_ALPHA);
    SetLayeredWindowAttributes(hMainWnd, TRANSPARENT_KEY, currentAlpha, LWA_COLORKEY | LWA_ALPHA);
}

void UpdateFloaterLayout(HWND hwnd) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR timeStr[16], dateStr[32];
    swprintf(timeStr, 16, L"%02d:%02d", st.wHour, st.wMinute);
    const WCHAR* months[] = { L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
    const WCHAR* days[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
    swprintf(dateStr, 32, L"%ls %d, %ls", months[st.wMonth], st.wDay, days[st.wDayOfWeek]);

    HDC hdc = GetDC(hwnd);
    HFONT hTimeFont = CreateFontW(SC(28), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
    HFONT hDateFont = CreateFontW(SC(16), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
    
    SIZE szTime, szDate;
    HFONT oldFont = (HFONT)SelectObject(hdc, hTimeFont);
    GetTextExtentPoint32W(hdc, timeStr, (int)wcslen(timeStr), &szTime);
    SelectObject(hdc, hDateFont);
    GetTextExtentPoint32W(hdc, dateStr, (int)wcslen(dateStr), &szDate);
    SelectObject(hdc, oldFont);
    
    int penWidth = SC(BORDER_THICKNESS);
    int width = (szTime.cx > szDate.cx ? szTime.cx : szDate.cx) + penWidth + SC(10);
    int height = szTime.cy + szDate.cy + penWidth + SC(0);
    
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.right - rc.left != width || rc.bottom - rc.top != height) {
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    
    DeleteObject(hTimeFont);
    DeleteObject(hDateFont);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK FloaterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEACTIVATE: {
            /* 태스크 박스가 떠 있을 때는 클릭해도 포커스를 뺏어오지 않음 (드래그/우클릭 가능하게) */
            if (hMainWnd && IsWindowVisible(hMainWnd)) return MA_NOACTIVATE;
            return MA_ACTIVATE;
        }
        case WM_ACTIVATE: {
            if (LOWORD(wParam) != WA_INACTIVE) {
                if (hMainWnd && IsWindowVisible(hMainWnd)) {
                    SetForegroundWindow(hMainWnd);
                }
            }
            return 0;
        }
        case WM_CREATE: {
            SetLayeredWindowAttributes(hwnd, 0, currentAlpha, LWA_ALPHA);
            
            /* 창 모서리 각지게 설정 */
            int cornerPref = DWMWCP_DONOTROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
            
            UpdateFloaterLayout(hwnd);
            SetTimer(hwnd, 1, 1000, NULL);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            
            /* 하이라이트 효과 (깜빡임) - 배경색 변경 */
            COLORREF bgColor = COLOR_PASTEL_GREEN;
            if (g_floaterHighlightTicks > 0 && (g_floaterHighlightTicks % 2 != 0)) {
                bgColor = RGB(255, 255, 0); /* 밝은 노란색 */
            }
            HBRUSH hbrBg = CreateSolidBrush(bgColor);
            int penWidth = SC(BORDER_THICKNESS);
            
            COLORREF borderColor = COLOR_PASTEL_BLUE;
            HPEN hPenBorder = CreatePen(PS_SOLID, penWidth, borderColor);
            
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hbrBg);
            HPEN oldPen = (HPEN)SelectObject(hdc, hPenBorder);
            
            /* 외곽선 두께를 고려하여 내부로 수축 */
            RECT rcDraw = rc;
            InflateRect(&rcDraw, -penWidth/2, -penWidth/2);
            Rectangle(hdc, rcDraw.left, rcDraw.top, rcDraw.right, rcDraw.bottom);
            
            /* 텍스트 그리기 */
            SYSTEMTIME st;
            GetLocalTime(&st);
            WCHAR timeStr[16], dateStr[32];
            swprintf(timeStr, 16, L"%02d:%02d", st.wHour, st.wMinute);
            const WCHAR* months[] = { L"", L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
            const WCHAR* days[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
            swprintf(dateStr, 32, L"%ls %d, %ls", months[st.wMonth], st.wDay, days[st.wDayOfWeek]);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT_DARK);
            
            HFONT hTimeFont = CreateFontW(SC(28), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
            HFONT hDateFont = CreateFontW(SC(16), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
            
            /* 텍스트 크기 측정하여 수직 중앙 정렬 */
            SIZE szTime, szDate;
            HFONT oldFont = (HFONT)SelectObject(hdc, hTimeFont);
            GetTextExtentPoint32W(hdc, timeStr, (int)wcslen(timeStr), &szTime);
            SelectObject(hdc, hDateFont);
            GetTextExtentPoint32W(hdc, dateStr, (int)wcslen(dateStr), &szDate);

            int totalTextHeight = szTime.cy + szDate.cy;
            int startY = (rc.bottom - rc.top - totalTextHeight) / 2;

            RECT rcTime = rc; 
            rcTime.top = startY;
            rcTime.bottom = rcTime.top + szTime.cy;
            
            RECT rcDate = rc; 
            rcDate.top = rcTime.bottom;
            rcDate.bottom = rcDate.top + szDate.cy;
            
            SelectObject(hdc, hTimeFont);
            DrawTextW(hdc, timeStr, -1, &rcTime, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            SelectObject(hdc, hDateFont);
            DrawTextW(hdc, dateStr, -1, &rcDate, DT_CENTER | DT_TOP | DT_SINGLELINE);
            
            SelectObject(hdc, oldFont);
            DeleteObject(hTimeFont);
            DeleteObject(hDateFont);
            
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(hbrBg);
            DeleteObject(hPenBorder);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN: {
            int dx = 0, dy = 0;
            int moveStep = SC(20);
            BOOL isCtrl = (GetKeyState(VK_CONTROL) < 0);
            BOOL isAlt = (GetKeyState(VK_MENU) < 0);
            
            /* Ctrl + 방향키/hjkl/wasd: 태스크 박스 창 이동 */
            if (isCtrl) {
                if (wParam == VK_LEFT || wParam == 'H' || wParam == 'A') dx = -moveStep;
                else if (wParam == VK_RIGHT || wParam == 'L' || wParam == 'D') dx = moveStep;
                else if (wParam == VK_UP || wParam == 'K' || wParam == 'W') dy = -moveStep;
                else if (wParam == VK_DOWN || wParam == 'J' || wParam == 'S') dy = moveStep;
                
                if (dx != 0 || dy != 0) {
                    MoveWindowByOffset(hMainWnd, dx, dy);
                    return 0;
                }

                /* Ctrl + +/-: 크기 조절 */
                if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                    UpdateSize(1);
                    return 0;
                } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                    UpdateSize(-1);
                    return 0;
                }
            }

            /* Alt + +/-: 투명도 조절, Alt + 방향키/hjkl/wasd: 크기 조절 */
            if (isAlt) {
                if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                    UpdateAlpha(1);
                    return 0;
                } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                    UpdateAlpha(-1);
                    return 0;
                } else if (wParam == VK_UP || wParam == 'K' || wParam == 'W' || wParam == VK_RIGHT || wParam == 'L' || wParam == 'D') {
                    UpdateSize(1);
                    return 0;
                } else if (wParam == VK_DOWN || wParam == 'J' || wParam == 'S' || wParam == VK_LEFT || wParam == 'H' || wParam == 'A') {
                    UpdateSize(-1);
                    return 0;
                }
            }
            break;
        }
        case WM_LBUTTONDOWN: {
            dragStartPt.x = GET_X_LPARAM(lParam);
            dragStartPt.y = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (GetCapture() == hwnd) {
                ReleaseCapture();
                /* 드래그가 아니었을 때만 토글 */
                if (IsWindowVisible(hMainWnd)) {
                    ShowWindow(hMainWnd, SW_HIDE);
                } else {
                    ShowWindow(hMainWnd, SW_SHOW);
                    SetForegroundWindow(hMainWnd);
                    if (hEditMsg) SetWindowTextW(hEditMsg, L"RClick: Menu | Ctrl+Wheel: Size | Alt+Wheel: Alpha");
                }
            }
            return 0;
        }
        case WM_RBUTTONUP: {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_CLOSE_APP, L"Exit (&X)");
            POINT pt; GetCursorPos(&pt);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDM_CLOSE_APP) {
                PostQuitMessage(0);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (GetKeyState(VK_MENU) < 0) {
                short delta = (short)HIWORD(wParam);
                int newAlpha = (int)currentAlpha + (delta > 0 ? 15 : -15);
                if (newAlpha > 255) newAlpha = 255;
                if (newAlpha < 128) newAlpha = 128;
                currentAlpha = (BYTE)newAlpha;
                SetLayeredWindowAttributes(hwnd, 0, currentAlpha, LWA_ALPHA);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (wParam & MK_LBUTTON && GetCapture() == hwnd) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (abs(pt.x - dragStartPt.x) > GetSystemMetrics(SM_CXDRAG) ||
                    abs(pt.y - dragStartPt.y) > GetSystemMetrics(SM_CYDRAG)) {
                    ReleaseCapture();
                    PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
                }
            }
            return 0;
        }
        case WM_NCHITTEST: {
            return HTCLIENT; /* 직접 드래그 구현을 위해 HTCLIENT 반환 */
        }
        case WM_EXITSIZEMOVE: {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            SaveConfig(L"Floater", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            return 0;
        }
        case WM_TIMER: {
            if (wParam == 1) {
                UpdateFloaterLayout(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (wParam == TIMER_HIGHLIGHT) {
                g_floaterHighlightTicks--;
                if (g_floaterHighlightTicks <= 0) {
                    KillTimer(hwnd, TIMER_HIGHLIGHT);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_HOTKEY: {
            if (wParam == 1) {
                /* 플로팅 박스와 태스크 박스를 최상위로 올림 */
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                
                /* 플로팅 박스 하이라이트 효과 시작 */
                g_floaterHighlightTicks = HIGHLIGHT_TICKS;
                SetTimer(hwnd, TIMER_HIGHLIGHT, 100, NULL);
                InvalidateRect(hwnd, NULL, FALSE);

                /* 태스크 박스 표시 및 최상위 포커스 */
                if (hMainWnd) {
                    if (!IsWindowVisible(hMainWnd)) {
                        ShowWindow(hMainWnd, SW_SHOW);
                    }
                    SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetForegroundWindow(hMainWnd);
                    
                    /* 하이라이트 효과 */
                    g_taskBoxHighlightTicks = HIGHLIGHT_TICKS;
                    SetTimer(hMainWnd, TIMER_HIGHLIGHT, 100, NULL);
                    InvalidateRect(hMainWnd, NULL, FALSE);
                    
                    /* 태스크 리스트 첫 번째 아이콘에 포커스 */
                    g_focusArea = 1;
                    SetFocus(hTaskBarWnd);
                    if (windowCount > 0) {
                        g_toolbarFocusIndex = 0;
                    }
                    UpdateFocusMessage();
                    InvalidateRect(hTaskBarWnd, NULL, FALSE);
                    InvalidateRect(hToolbarWnd, NULL, FALSE);
                } else {
                    /* 태스크 박스가 없거나 숨겨진 상태라면 플로팅 박스를 포커스 */
                    SetForegroundWindow(hwnd);
                }
            }
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void ActivateToolbarItem(int index) {
    if (index < 0) return;
    if (index == 0) {
        /* R button - handled by drag, no click action needed or could be a toggle? 
           User said "이 버튼을 드래그하면 그것에 맞춰 창 크기를 변경하게 해 줘".
           Clicking it doesn't have a specified action. */
    } else if (index == 1) {
        ShowWindow(hMainWnd, SW_HIDE);
    } else if (index == 2) {
        ShellExecuteW(NULL, L"open", g_shortcutsPath, NULL, NULL, SW_SHOWNORMAL);
    } else if (index == 3) {
        static BOOL isDesktopShown = FALSE;
        isDesktopShown = !isDesktopShown;
        EnumWindows(MinimizeRestoreEnumProc, (LPARAM)isDesktopShown);
    } else {
        int sIdx = index - 4;
        if (sIdx >= 0 && sIdx < shortcutCount) {
            ShellExecuteW(NULL, L"open", shortcuts[sIdx].path, NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

void ActivateTaskBarItem(int index) {
    if (index < 0 || index >= windowCount) return;
    HWND target = windowItems[index].hwnd;
    if (IsWindow(target)) {
        if (IsIconic(target)) ShowWindow(target, SW_RESTORE);
        SetForegroundWindow(target);
    }
}

void UpdateFocusMessage() {
    if (g_focusArea == 0) {
        int totalItems = shortcutCount + 4;
        if (g_toolbarFocusIndex >= 0 && g_toolbarFocusIndex < totalItems) {
            if (g_toolbarFocusIndex == 0) AppendMessage(L"Drag to Resize Window");
            else if (g_toolbarFocusIndex == 1) AppendMessage(L"Hide Dashboard");
            else if (g_toolbarFocusIndex == 2) AppendMessage(L"Open Shortcuts Folder");
            else if (g_toolbarFocusIndex == 3) AppendMessage(L"Show Desktop");
            else AppendMessage(shortcuts[g_toolbarFocusIndex - 4].name);
        }
    } else {
        if (g_toolbarFocusIndex >= 0 && g_toolbarFocusIndex < windowCount) {
            AppendMessage(windowItems[g_toolbarFocusIndex].title);
        }
    }
}

LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int hoveredIndex = -1;
    static int pressedIndex = -1;
    static HFONT hBtnFont = NULL;
    static int cachedIconSize = 0;
    static BOOL isResizing = FALSE;
    static POINT startMouse;
    static RECT startRect;

    switch (uMsg) {
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            int totalItems = shortcutCount + 4; /* X, S, D, R */
            for (int i = 0; i < totalItems; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) return HTCLIENT;
            }
            return HTTRANSPARENT;
        }
        case WM_SIZE: {
            UpdateToolbarTooltips(hwnd);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            
            if (rc.right > 0 && rc.bottom > 0) {
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);
                
                /* 하이라이트 효과 (깜빡임) */
                COLORREF bgColor = COLOR_PASTEL_BLUE;
                if (g_taskBoxHighlightTicks > 0 && (g_taskBoxHighlightTicks % 2 != 0)) {
                    bgColor = RGB(255, 255, 0);
                }
                HBRUSH hbrBg = CreateSolidBrush(bgColor);
                FillRect(memDC, &rc, hbrBg);
                DeleteObject(hbrBg);

                int iconSize = abs(currentFontSize);
                if (iconSize < SC(16)) iconSize = SC(16);
                
                /* 폰트 캐싱 */
                if (iconSize != cachedIconSize || !hBtnFont) {
                    if (hBtnFont) DeleteObject(hBtnFont);
                    hBtnFont = CreateFontW(iconSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                    cachedIconSize = iconSize;
                }

                int totalItems = shortcutCount + 4;
                for (int i = 0; i < totalItems; i++) {
                    RECT rcItem;
                    GetToolbarItemRect(i, rc.right, iconSize, &rcItem);

                    RECT rcBtn = rcItem;
                    InflateRect(&rcBtn, SC(4), SC(4));

                    if (i == pressedIndex) {
                        HBRUSH hbr = CreateSolidBrush(RGB(80, 80, 80));
                        FillRect(memDC, &rcBtn, hbr);
                        DeleteObject(hbr);
                        DrawEdge(memDC, &rcBtn, EDGE_SUNKEN, BF_RECT);
                    } else if (i == hoveredIndex || (g_focusArea == 0 && i == g_toolbarFocusIndex)) {
                        HBRUSH hbr = CreateSolidBrush(RGB(60, 60, 60));
                        FillRect(memDC, &rcBtn, hbr);
                        DeleteObject(hbr);
                        DrawEdge(memDC, &rcBtn, BDR_RAISEDINNER, BF_RECT);
                        
                        /* 포커스 표시 (노란색 테두리) */
                        if (g_focusArea == 0 && i == g_toolbarFocusIndex) {
                            HBRUSH hbrFocus = CreateSolidBrush(RGB(255, 255, 0));
                            FrameRect(memDC, &rcBtn, hbrFocus);
                            DeleteObject(hbrFocus);
                        }
                    }

                    if (i >= 4) {
                        int sIdx = i - 4;
                        if (shortcuts[sIdx].hIcon) {
                            DrawIconEx(memDC, rcItem.left, rcItem.top, shortcuts[sIdx].hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                        }
                    } else {
                        SetTextColor(memDC, COLOR_TEXT_DARK);
                        SetBkMode(memDC, TRANSPARENT);
                        HFONT hOldFont = (HFONT)SelectObject(memDC, hBtnFont);
                        
                        WCHAR btnText[2] = {0};
                        if (i == 0) btnText[0] = L'R';
                        else if (i == 1) btnText[0] = L'X';
                        else if (i == 2) btnText[0] = L'S';
                        else if (i == 3) btnText[0] = L'D';
                        DrawOutlinedTextW(memDC, btnText, 1, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE, COLOR_TEXT_DARK, RGB(255, 255, 255));
                        SelectObject(memDC, hOldFont);
                    }
                }
                
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
                SelectObject(memDC, oldBM);
                DeleteObject(memBM);
                DeleteDC(memDC);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN: {
            SendMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            g_focusArea = 0;
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            int totalItems = shortcutCount + 4;
            for (int i = 0; i < totalItems; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) {
                    pressedIndex = i;
                    if (i == 0) {
                        isResizing = TRUE;
                        GetCursorPos(&startMouse);
                        GetWindowRect(hMainWnd, &startRect);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    SetCapture(hwnd);
                    break;
                }
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (pressedIndex != -1) {
                if (isResizing) {
                    isResizing = FALSE;
                    RECT rc;
                    GetWindowRect(hMainWnd, &rc);
                    SaveConfig(L"TaskBox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
                } else {
                    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    RECT rc; GetClientRect(hwnd, &rc);
                    int iconSize = abs(currentFontSize);
                    if (iconSize < SC(16)) iconSize = SC(16);
                    int totalItems = shortcutCount + 4;
                    
                    int currentIndex = -1;
                    for (int i = 0; i < totalItems; i++) {
                        RECT rcItem;
                        GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                        RECT rcBtn = rcItem;
                        InflateRect(&rcBtn, SC(4), SC(4));
                        if (PtInRect(&rcBtn, pt)) {
                            currentIndex = i;
                            break;
                        }
                    }

                    if (currentIndex == pressedIndex) {
                        ActivateToolbarItem(currentIndex);
                    }
                }
                
                pressedIndex = -1;
                ReleaseCapture();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (isResizing) {
                POINT curMouse;
                GetCursorPos(&curMouse);
                int newWidth = (startRect.right - startRect.left) + (curMouse.x - startMouse.x);
                int newHeight = (startRect.bottom - startRect.top) + (curMouse.y - startMouse.y);
                SetWindowPos(hMainWnd, NULL, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            
            int currentIndex = -1;
            int totalItems = shortcutCount + 4;
            for (int i = 0; i < totalItems; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) {
                    currentIndex = i;
                    break;
                }
            }

            if (currentIndex != hoveredIndex) {
                hoveredIndex = currentIndex;
                if (hoveredIndex != -1) {
                    if (hoveredIndex == 0) {
                        AppendMessage(L"Drag to Resize Window");
                    } else if (hoveredIndex == 1) {
                        AppendMessage(L"Hide Dashboard");
                    } else if (hoveredIndex == 2) {
                        AppendMessage(L"Open Shortcuts Folder");
                    } else if (hoveredIndex == 3) {
                        AppendMessage(L"Show Desktop");
                    } else {
                        AppendMessage(shortcuts[hoveredIndex - 4].name);
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }

            /* 마우스가 윈도우를 나가는 것을 감지하기 위해 등록 */
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            break;
        }
        case WM_MOUSELEAVE: {
            hoveredIndex = -1;
            pressedIndex = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_NCDESTROY: {
            if (hBtnFont) { DeleteObject(hBtnFont); hBtnFont = NULL; }
            break;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
/* 증분 업데이트: 순서 고정 및 스마트 리프레시 */
void RefreshWindowList(BOOL force) {
    int newCount = 0;
    
    /* 1단계: 기존 창 유효성 체크 및 아이콘 재사용 */
    for (int i = 0; i < windowCount; i++) {
        if (IsWindow(windowItems[i].hwnd) && IsAltTabWindow(windowItems[i].hwnd)) {
            newItems[newCount].hwnd = windowItems[i].hwnd;
            newItems[newCount].hIcon = windowItems[i].hIcon;
            newItems[newCount].bOwnIcon = windowItems[i].bOwnIcon;
            windowItems[i].bOwnIcon = FALSE; 
            
            newItems[newCount].exists = TRUE;
            GetProcessNameByHWND(newItems[newCount].hwnd, newItems[newCount].processName, &newItems[newCount].processId);
            if (GetWindowTextW(newItems[newCount].hwnd, newItems[newCount].title, MAX_TITLE_LEN) == 0) {
                newItems[newCount].title[0] = L'\0';
            }
            newCount++;
        } else {
            if (windowItems[i].bOwnIcon && windowItems[i].hIcon) {
                DestroyIcon(windowItems[i].hIcon);
            }
        }
    }

    /* 2단계: 새로 나타난 창 추가 */
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        if (IsAltTabWindow(hwnd)) {
            BOOL exists = FALSE;
            for (int i = 0; i < newCount; i++) {
                if (newItems[i].hwnd == hwnd) {
                    exists = TRUE;
                    break;
                }
            }
            if (!exists && newCount < MAX_WINDOW_ITEMS) {
                newItems[newCount].hwnd = hwnd;
                newItems[newCount].hIcon = GetWindowIcon(hwnd, abs(currentFontSize), &newItems[newCount].bOwnIcon);
                newItems[newCount].exists = TRUE;
                GetProcessNameByHWND(hwnd, newItems[newCount].processName, &newItems[newCount].processId);
                if (GetWindowTextW(hwnd, newItems[newCount].title, MAX_TITLE_LEN) == 0) {
                    newItems[newCount].title[0] = L'\0';
                }
                newCount++;
            }
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    BOOL changed = force || (newCount != windowCount);
    if (!changed) {
        for (int i = 0; i < windowCount; i++) {
            if (newItems[i].hwnd != windowItems[i].hwnd || 
                wcscmp(newItems[i].title, windowItems[i].title) != 0) {
                changed = TRUE;
                break;
            }
        }
    }

    if (changed) {
        windowCount = newCount;
        for (int i = 0; i < windowCount; i++) {
            windowItems[i] = newItems[i];
        }
        
        UpdateLayout(hMainWnd);
        UpdateTaskBarTooltips(hTaskBarWnd);
        InvalidateRect(hTaskBarWnd, NULL, TRUE);
    } else {
        for (int i = 0; i < newCount; i++) {
            windowItems[i].hIcon = newItems[i].hIcon;
            windowItems[i].bOwnIcon = newItems[i].bOwnIcon;
        }
    }
}

LRESULT CALLBACK TaskBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int hoveredIndex = -1;
    static int pressedIndex = -1;

    switch (uMsg) {
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            for (int i = 0; i < windowCount; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) return HTCLIENT;
            }
            return HTTRANSPARENT;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            if (rc.right > 0 && rc.bottom > 0) {
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
                HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);
                
                /* 하이라이트 효과 (깜빡임) */
                COLORREF bgColor = CLICKABLE_BG;
                if (g_taskBoxHighlightTicks > 0 && (g_taskBoxHighlightTicks % 2 != 0)) {
                    bgColor = RGB(255, 255, 0);
                }
                HBRUSH hbrBg = CreateSolidBrush(bgColor);
                FillRect(memDC, &rc, hbrBg);
                DeleteObject(hbrBg);

                int iconSize = abs(currentFontSize);
                if (iconSize < SC(16)) iconSize = SC(16);
                
                for (int i = 0; i < windowCount; i++) {
                    RECT rcItem;
                    GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                    RECT rcBtn = rcItem;
                    InflateRect(&rcBtn, SC(4), SC(4));

                    if (i == pressedIndex) {
                        HBRUSH hbr = CreateSolidBrush(RGB(80, 80, 80));
                        FillRect(memDC, &rcBtn, hbr);
                        DeleteObject(hbr);
                        DrawEdge(memDC, &rcBtn, EDGE_SUNKEN, BF_RECT);
                    } else if (i == hoveredIndex || (g_focusArea == 1 && i == g_toolbarFocusIndex)) {
                        HBRUSH hbr = CreateSolidBrush(RGB(60, 60, 60));
                        FillRect(memDC, &rcBtn, hbr);
                        DeleteObject(hbr);
                        DrawEdge(memDC, &rcBtn, BDR_RAISEDINNER, BF_RECT);
                        
                        if (g_focusArea == 1 && i == g_toolbarFocusIndex) {
                            HBRUSH hbrFocus = CreateSolidBrush(RGB(255, 255, 0));
                            FrameRect(memDC, &rcBtn, hbrFocus);
                            DeleteObject(hbrFocus);
                        }
                    }

                    if (windowItems[i].hIcon) {
                        DrawIconEx(memDC, rcItem.left, rcItem.top, windowItems[i].hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                    }
                }
                
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
                SelectObject(memDC, oldBM);
                DeleteObject(memBM);
                DeleteDC(memDC);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN: {
            SendMessage(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            g_focusArea = 1;
            InvalidateRect(hToolbarWnd, NULL, FALSE);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            for (int i = 0; i < windowCount; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) {
                    pressedIndex = i;
                    isDragging = FALSE;
                    dragSourceIndex = i;
                    dragStartPt = pt;
                    InvalidateRect(hwnd, NULL, FALSE);
                    SetCapture(hwnd);
                    break;
                }
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            
            int currentIndex = -1;
            for (int i = 0; i < windowCount; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) {
                    currentIndex = i;
                    break;
                }
            }

            if (currentIndex != hoveredIndex) {
                hoveredIndex = currentIndex;
                if (hoveredIndex != -1) {
                    AppendMessage(windowItems[hoveredIndex].title);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }

            /* 드래그 판정 */
            if (dragSourceIndex != -1 && !isDragging && GetCapture() == hwnd) {
                if (abs(pt.x - dragStartPt.x) > GetSystemMetrics(SM_CXDRAG) ||
                    abs(pt.y - dragStartPt.y) > GetSystemMetrics(SM_CYDRAG)) {
                    isDragging = TRUE;
                }
            }

            if (isDragging && currentIndex != -1 && currentIndex != dragSourceIndex) {
                WindowItem temp = windowItems[dragSourceIndex];
                windowItems[dragSourceIndex] = windowItems[currentIndex];
                windowItems[currentIndex] = temp;
                dragSourceIndex = currentIndex;
                g_toolbarFocusIndex = currentIndex;
                InvalidateRect(hwnd, NULL, FALSE);
            }

            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            break;
        }
        case WM_LBUTTONUP: {
            if (dragSourceIndex != -1) {
                BOOL wasDragging = isDragging;
                int finalIndex = dragSourceIndex;
                dragSourceIndex = -1;
                pressedIndex = -1;
                isDragging = FALSE;
                ReleaseCapture();
                
                if (!wasDragging) {
                    ActivateTaskBarItem(finalIndex);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_RBUTTONUP: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc; GetClientRect(hwnd, &rc);
            int iconSize = abs(currentFontSize);
            if (iconSize < SC(16)) iconSize = SC(16);
            int index = -1;
            for (int i = 0; i < windowCount; i++) {
                RECT rcItem;
                GetToolbarItemRect(i, rc.right, iconSize, &rcItem);
                RECT rcBtn = rcItem;
                InflateRect(&rcBtn, SC(4), SC(4));
                if (PtInRect(&rcBtn, pt)) {
                    index = i;
                    break;
                }
            }
            if (index != -1) {
                HWND target = windowItems[index].hwnd;
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, IDM_CLOSE, L"Close (&C)");
                POINT screenPt; GetCursorPos(&screenPt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, screenPt.x, screenPt.y, 0, hwnd, NULL);
                if (cmd == IDM_CLOSE) PostMessageW(target, WM_CLOSE, 0, 0);
                DestroyMenu(hMenu);
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            hoveredIndex = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_MOUSEWHEEL: {
            if (LOWORD(wParam) & MK_CONTROL) {
                short delta = (short)HIWORD(wParam);
                int newSize = abs(currentFontSize) + (delta > 0 ? SC(2) : -SC(2));
                if (newSize < SC(16)) newSize = SC(16);
                if (newSize > SC(128)) newSize = SC(128);
                currentFontSize = (currentFontSize > 0) ? newSize : -newSize;
                
                RECT rc; GetClientRect(hMainWnd, &rc);
                SendMessageW(hMainWnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
                RefreshWindowList(TRUE);
                return 0;
            }
            return SendMessageW(GetParent(hwnd), WM_MOUSEWHEEL, wParam, lParam);
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_LBUTTONDOWN:
            /* Forward to parent for dragging */
            PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, HTCAPTION, lParam);
            return 0;
        case WM_CONTEXTMENU: {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, IDM_EDIT_COPYALL, L"Copy All (&A)");
            
            POINT pt;
            if ((int)lParam == -1) { /* Keyboard shortcut */
                RECT rc; GetWindowRect(hWnd, &rc);
                pt.x = rc.left + 5; pt.y = rc.top + 5;
            } else {
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
            }
            
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            
            if (cmd == IDM_EDIT_COPYALL) {
                SendMessageW(hWnd, EM_SETSEL, 0, -1);
                SendMessageW(hWnd, WM_COPY, 0, 0);
                SendMessageW(hWnd, EM_SETSEL, -1, 0);
            }
            
            DestroyMenu(hMenu);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void UpdateLayout(HWND hwnd) {
    if (!hEditMsg || !hToolbarWnd || !hTaskBarWnd) return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right;
    int height = rc.bottom;
    int border = SC(BORDER_THICKNESS);

    /* Position children */
    HDC hdc = GetDC(hEditMsg);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    int editHeight = (tm.tmHeight + tm.tmExternalLeading) * 1 + SC(6);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hEditMsg, hdc);

    int tbWidth = width - (border * 2);
    if (tbWidth < 0) tbWidth = 0;
    int tbHeight = GetToolbarHeight(tbWidth);
    int listHeight = height - editHeight - tbHeight - (border * 2);
    if (listHeight < 0) listHeight = 0;
    
    MoveWindow(hEditMsg, border, border, tbWidth, editHeight, TRUE);
    MoveWindow(hToolbarWnd, border, border + editHeight, tbWidth, tbHeight, TRUE);
    MoveWindow(hTaskBarWnd, border, border + editHeight + tbHeight, tbWidth, listHeight, TRUE);
    UpdateTaskBarTooltips(hTaskBarWnd);
    UpdateToolbarTooltips(hToolbarWnd);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = SC(MIN_WINDOW_WIDTH);
            mmi->ptMinTrackSize.y = SC(MIN_WINDOW_HEIGHT);
            return 0;
        }
        case WM_CREATE: {
            /* 툴바 클래스 등록 */
            WNDCLASSW twc = {0};
            twc.style = CS_HREDRAW | CS_VREDRAW;
            twc.lpfnWndProc = ToolbarProc;
            twc.hInstance = GetModuleHandle(NULL);
            twc.lpszClassName = L"HGToolbarClass";
            twc.hCursor = LoadCursor(NULL, IDC_HAND);
            RegisterClassW(&twc);

            /* 태스크바 클래스 등록 */
            WNDCLASSW tcwc = {0};
            tcwc.style = CS_HREDRAW | CS_VREDRAW;
            tcwc.lpfnWndProc = TaskBarProc;
            tcwc.hInstance = GetModuleHandle(NULL);
            tcwc.lpszClassName = L"HGTaskBarClass";
            tcwc.hCursor = LoadCursor(NULL, IDC_HAND);
            RegisterClassW(&tcwc);

            /* UI용 12pt Segoe UI 일반 폰트 생성 (아이콘 크기와 분리) */
            int uiFontSize = SC(-16); /* 12pt approx 16px */
            hFont = CreateFontW(uiFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            hToolbarWnd = CreateWindowExW(0, L"HGToolbarClass", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_TOOLBAR, GetModuleHandle(NULL), NULL);
            
            hEditMsg = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL, 
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT_MSG, GetModuleHandle(NULL), NULL);
            SendMessageW(hEditMsg, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetWindowTextW(hEditMsg, L"RClick: Menu | Ctrl+Wheel: Size | Alt+Wheel: Alpha");
            SetWindowSubclass(hEditMsg, EditSubclassProc, 0, 0);

            /* 툴팁 생성: 메인 윈도우를 소유자로 지정하되 TOPMOST 유지 */
            hToolTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                     WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                     hwnd, NULL, GetModuleHandle(NULL), NULL);
            
            if (hToolTip) {
                SendMessageW(hToolTip, TTM_SETMAXTIPWIDTH, 0, SC(1000));
                /* 툴팁 폰트도 DPI 배율이 적용된 폰트로 설정 */
                SendMessageW(hToolTip, WM_SETFONT, (WPARAM)hFont, TRUE);
                /* 즉시 표시되도록 설정 (약간의 지연 시간 부여) */
                SendMessageW(hToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
                SendMessageW(hToolTip, CCM_SETWINDOWTHEME, 0, (LPARAM)L"Explorer");
            }

            hTaskBarWnd = CreateWindowExW(0, L"HGTaskBarClass", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LISTBOX, GetModuleHandle(NULL), NULL);
            
            LoadShortcuts();
            RefreshWindowList(TRUE);
            UpdateToolbarTooltips(hToolbarWnd);
            
            /* 창 테두리 및 모서리 설정 */
            COLORREF borderColor = RGB(150, 150, 150);
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
            int cornerPref = DWMWCP_DONOTROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

            SetLayeredWindowAttributes(hwnd, TRANSPARENT_KEY, currentAlpha, LWA_COLORKEY | LWA_ALPHA);
            SetTimer(hwnd, 1, 1000, NULL); /* 1초마다 시계 및 목록 갱신 */
            return 0;
        }
        case WM_MOUSEWHEEL: {
            if (GetKeyState(VK_MENU) < 0) {
                /* Alt 키가 눌린 상태에서만 투명도 조절 실행 */
                /* 0% 투명(불투명) = Alpha 255, 50% 투명 = Alpha 128 */
                short delta = (short)HIWORD(wParam);
                int newAlpha = (int)currentAlpha + (delta > 0 ? 15 : -15);
                if (newAlpha > 255) newAlpha = 255;
                if (newAlpha < 128) newAlpha = 128;
                currentAlpha = (BYTE)newAlpha;
                SetLayeredWindowAttributes(hwnd, TRANSPARENT_KEY, currentAlpha, LWA_COLORKEY | LWA_ALPHA);
            } else {
                /* Alt 키가 없으면 태스크바 창으로 메시지를 전달하여 창 전체에서 스크롤 가능하게 함 */
                /* Ctrl+휠(크기조절) 메시지도 태스크바 창에서 처리되도록 전달됨 */
                SendMessageW(hTaskBarWnd, WM_MOUSEWHEEL, wParam, lParam);
            }
            return 0;
        }
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            /* ListView 관련 통지 제거됨 */
            break;
        }
        case WM_EXITSIZEMOVE: {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            SaveConfig(L"TaskBox", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            return 0;
        }
        case WM_TIMER:
            if (wParam == 1) {
                RefreshWindowList(FALSE);
            } else if (wParam == TIMER_HIGHLIGHT) {
                g_taskBoxHighlightTicks--;
                if (g_taskBoxHighlightTicks <= 0) {
                    KillTimer(hwnd, TIMER_HIGHLIGHT);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            
            /* 하이라이트 효과 (깜빡임) */
            COLORREF bgColor = CLICKABLE_BG;
            if (g_taskBoxHighlightTicks > 0 && (g_taskBoxHighlightTicks % 2 != 0)) {
                bgColor = RGB(255, 255, 0);
            }
            HBRUSH hbrBg = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            /* 외곽선 그리기 */
            int border = SC(BORDER_THICKNESS);
            HBRUSH hbrBorder = CreateSolidBrush(COLOR_PASTEL_BLUE);
            
            /* 상하좌우 사각형으로 채우기 */
            RECT rcTop = {0, 0, rc.right, border};
            RECT rcBottom = {0, rc.bottom - border, rc.right, rc.bottom};
            RECT rcLeft = {0, border, border, rc.bottom - border};
            RECT rcRight = {rc.right - border, border, rc.right, rc.bottom - border};
            
            FillRect(hdc, &rcTop, hbrBorder);
            FillRect(hdc, &rcBottom, hbrBorder);
            FillRect(hdc, &rcLeft, hbrBorder);
            FillRect(hdc, &rcRight, hbrBorder);
            
            DeleteObject(hbrBorder);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc; GetClientRect(hwnd, &rc);
            int border = SC(BORDER_THICKNESS);

            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;

            return HTCAPTION; /* 그 외 모든 클라이언트 영역은 드래그 가능하게 */
        }
        case WM_KEYDOWN: {
            int dx = 0, dy = 0;
            int moveStep = SC(20);
            BOOL isCtrl = (GetKeyState(VK_CONTROL) < 0);
            BOOL isAlt = (GetKeyState(VK_MENU) < 0);
            
            /* Ctrl + 방향키/hjkl/wasd: 창 이동 */
            if (isCtrl) {
                if (wParam == VK_LEFT || wParam == 'H' || wParam == 'A') dx = -moveStep;
                else if (wParam == VK_RIGHT || wParam == 'L' || wParam == 'D') dx = moveStep;
                else if (wParam == VK_UP || wParam == 'K' || wParam == 'W') dy = -moveStep;
                else if (wParam == VK_DOWN || wParam == 'J' || wParam == 'S') dy = moveStep;
                
                if (dx != 0 || dy != 0) {
                    MoveWindowByOffset(hwnd, dx, dy);
                    return 0;
                }

                /* Ctrl + +/-: 크기 조절 */
                if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                    UpdateSize(1);
                    return 0;
                } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                    UpdateSize(-1);
                    return 0;
                }
            }

            /* Alt + +/-: 투명도 조절, Alt + 방향키/hjkl/wasd: 크기 조절 */
            if (isAlt) {
                if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                    UpdateAlpha(1);
                    return 0;
                } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                    UpdateAlpha(-1);
                    return 0;
                } else if (wParam == VK_UP || wParam == 'K' || wParam == 'W' || wParam == VK_RIGHT || wParam == 'L' || wParam == 'D') {
                    UpdateSize(1);
                    return 0;
                } else if (wParam == VK_DOWN || wParam == 'J' || wParam == 'S' || wParam == VK_LEFT || wParam == 'H' || wParam == 'A') {
                    UpdateSize(-1);
                    return 0;
                }
            }

            /* Esc: 창 닫기 */
            if (wParam == VK_ESCAPE) {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }

            /* 탐색 및 선택 */
            int tbTotal = shortcutCount + 4;
            int taskTotal = windowCount;
            
            if (tbTotal > 0 || taskTotal > 0) {
                int iconSize = abs(currentFontSize);
                if (iconSize < SC(16)) iconSize = SC(16);
                
                RECT rcToolbar, rcTaskbar;
                GetClientRect(hToolbarWnd, &rcToolbar);
                GetClientRect(hTaskBarWnd, &rcTaskbar);
                
                int tbN = GetItemsPerRow(rcToolbar.right, iconSize);
                int taskN = GetItemsPerRow(rcTaskbar.right, iconSize);
                
                int currentTotal = (g_focusArea == 0) ? tbTotal : taskTotal;
                int currentN = (g_focusArea == 0) ? tbN : taskN;
                
                int r = g_toolbarFocusIndex / currentN;
                int c = g_toolbarFocusIndex % currentN;
                int maxRows = (currentTotal + currentN - 1) / currentN;

                BOOL changed = FALSE;
                if (wParam == VK_LEFT || wParam == 'H' || wParam == 'A') {
                    g_toolbarFocusIndex = (g_toolbarFocusIndex > 0) ? g_toolbarFocusIndex - 1 : currentTotal - 1;
                    changed = TRUE;
                } else if (wParam == VK_RIGHT || wParam == 'L' || wParam == 'D') {
                    g_toolbarFocusIndex = (g_toolbarFocusIndex < currentTotal - 1) ? g_toolbarFocusIndex + 1 : 0;
                    changed = TRUE;
                } else if (wParam == VK_UP || wParam == 'K' || wParam == 'W') {
                    if (r > 0) {
                        g_toolbarFocusIndex -= currentN;
                        changed = TRUE;
                    } else if (g_focusArea == 1) {
                        /* Taskbar(1) -> Toolbar(0) */
                        if (tbTotal > 0) {
                            g_focusArea = 0;
                            SetFocus(hToolbarWnd);
                            int tbRows = (tbTotal + tbN - 1) / tbN;
                            int targetRow = tbRows - 1;
                            int targetIdx = targetRow * tbN + c;
                            g_toolbarFocusIndex = (targetIdx < tbTotal) ? targetIdx : tbTotal - 1;
                            changed = TRUE;
                        }
                    }
                } else if (wParam == VK_DOWN || wParam == 'J' || wParam == 'S') {
                    if (r < maxRows - 1) {
                        int nextIdx = g_toolbarFocusIndex + currentN;
                        if (nextIdx < currentTotal) {
                            g_toolbarFocusIndex = nextIdx;
                        } else {
                            /* 현재 영역의 다음 줄에 항목이 없으면 마지막 항목으로 */
                            g_toolbarFocusIndex = currentTotal - 1;
                        }
                        changed = TRUE;
                    } else if (g_focusArea == 0) {
                        /* Toolbar(0) -> Taskbar(1) */
                        if (taskTotal > 0) {
                            g_focusArea = 1;
                            SetFocus(hTaskBarWnd);
                            g_toolbarFocusIndex = (c < taskTotal) ? c : taskTotal - 1;
                            changed = TRUE;
                        }
                    }
                } else if (wParam == VK_SPACE || wParam == VK_RETURN) {
                    if (g_focusArea == 0) ActivateToolbarItem(g_toolbarFocusIndex);
                    else ActivateTaskBarItem(g_toolbarFocusIndex);
                }

                if (changed) UpdateFocusMessage();
                InvalidateRect(hTaskBarWnd, NULL, FALSE);
                InvalidateRect(hToolbarWnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDM_CLOSE_APP) {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            break;
        }
        case WM_SIZE: {
            UpdateLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE); /* 외곽선 다시 그리기 */
            return 0;
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, COLOR_TEXT_DARK);
            
            COLORREF bgColor = CLICKABLE_BG;
            if (g_taskBoxHighlightTicks > 0 && (g_taskBoxHighlightTicks % 2 != 0)) {
                bgColor = RGB(255, 255, 0);
            }
            SetBkColor(hdcStatic, bgColor);
            
            /* 하이라이트 중일 때는 매번 브러시를 새로 생성하여 반환 (GDI 누수 방지를 위해 정적 변수 관리) */
            static HBRUSH hbrHighlight = NULL;
            if (g_taskBoxHighlightTicks > 0 && (g_taskBoxHighlightTicks % 2 != 0)) {
                if (hbrHighlight) DeleteObject(hbrHighlight);
                hbrHighlight = CreateSolidBrush(RGB(255, 255, 0));
                return (LRESULT)hbrHighlight;
            }
            
            if (!hbrEditBg) hbrEditBg = CreateSolidBrush(CLICKABLE_BG);
            return (LRESULT)hbrEditBg;
        }
        case WM_MEASUREITEM:
        case WM_DRAWITEM:
            return TRUE;
        case WM_DESTROY: 
            if (hFont) { DeleteObject(hFont); hFont = NULL; }
            if (hbrEditBg) { DeleteObject(hbrEditBg); hbrEditBg = NULL; }
            if (hToolTip) { DestroyWindow(hToolTip); hToolTip = NULL; }
            for (int i = 0; i < shortcutCount; i++) {
                if (shortcuts[i].hIcon) { DestroyIcon(shortcuts[i].hIcon); shortcuts[i].hIcon = NULL; }
            }
            for (int i = 0; i < windowCount; i++) {
                if (windowItems[i].bOwnIcon && windowItems[i].hIcon) {
                    DestroyIcon(windowItems[i].hIcon);
                    windowItems[i].hIcon = NULL;
                }
            }
            PostQuitMessage(0); 
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    /* 단일 인스턴스 실행 보장 (Mutex 사용) */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\HGFloater_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(L"HGFloaterWidgetClass", NULL);
        if (hExisting) {
            SetForegroundWindow(hExisting);
        }
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    InitPaths();
    SetProcessDPIAware();
    
    /* DPI 스케일 계산 */
    HDC hdcScreen = GetDC(NULL);
    int dpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
    ReleaseDC(NULL, hdcScreen);
    g_scale = dpiX / 96.0;
    
    /* 초기 값들에 DPI 배율 적용 */
    currentFontSize = SC(currentFontSize);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    /* Extract icons first to share across classes */
    HICON hIconLarge = NULL, hIconSmall = NULL;
    ExtractIconExW(L"shell32.dll", 14, &hIconLarge, &hIconSmall, 1);

    /* 위젯 클래스 등록 */
    WNDCLASSEXW wwc = {0};
    wwc.cbSize = sizeof(WNDCLASSEXW);
    wwc.lpfnWndProc = FloaterProc;
    wwc.hInstance = hInstance;
    wwc.lpszClassName = L"HGFloaterWidgetClass";
    wwc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wwc.hIcon = hIconLarge;
    wwc.hIconSm = hIconSmall;
    RegisterClassExW(&wwc);

    /* 메인 대시보드 클래스 등록 */
    const WCHAR CLASS_NAME[] = L"HGFloaterClass";
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    if (!hbrMainBg) hbrMainBg = CreateSolidBrush(CLICKABLE_BG);
    wc.hbrBackground = hbrMainBg;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = hIconLarge;
    wc.hIconSm = hIconSmall;
    RegisterClassExW(&wc);

    int fx, fy, fw, fh, tx, ty, tw, th;
    LoadConfig(L"Floater", &fx, &fy, &fw, &fh, 100, 100, SC(80), SC(55));
    LoadConfig(L"TaskBox", &tx, &ty, &tw, &th, 200, 200, SC(WINDOW_WIDTH), SC(WINDOW_HEIGHT));
    LoadHotkeyConfig();

    hFloaterWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_APPWINDOW, L"HGFloaterWidgetClass", L"HGFloaterWidget", WS_POPUP | WS_VISIBLE,
        fx, fy, fw, fh, NULL, NULL, hInstance, NULL);

    if (hFloaterWnd) {
        RegisterHotKey(hFloaterWnd, 1, g_hotkeyModifiers | MOD_NOREPEAT, g_hotkeyKey);
    }

    hMainWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, CLASS_NAME, L"HGFloater", WS_POPUP,
        tx, ty, tw, th, NULL, NULL, hInstance, NULL);

    ACCEL accel[] = { { FCONTROL | FVIRTKEY, 'Q', IDM_CLOSE_APP } };
    HACCEL hAccel = CreateAcceleratorTableW(accel, 1);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(hMainWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hAccel) DestroyAcceleratorTable(hAccel);
    if (hMutex) CloseHandle(hMutex);
    if (hbrMainBg) { DeleteObject(hbrMainBg); hbrMainBg = NULL; }
    if (hIconLarge) DestroyIcon(hIconLarge);
    if (hIconSmall) DestroyIcon(hIconSmall);
    return 0;
}
