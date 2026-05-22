#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

LRESULT CALLBACK mock_controlbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW cwc = {0};
    cwc.cbSize = sizeof(WNDCLASSEXW);
    cwc.lpfnWndProc = mock_controlbox_proc;
    cwc.hInstance = hInstance;
    cwc.lpszClassName = L"hgcontrolbox_class_test";
    cwc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    cwc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    ATOM atom = RegisterClassExW(&cwc);
    if (!atom) {
        printf("Class registration failed in TDD test.\n");
        return 1;
    }

    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"hgcontrolbox_class_test", L"test_controlbox", WS_POPUP,
                                100, 100, 300, 200, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        printf("Window creation failed in TDD test.\n");
        return 1;
    }

    // Try creating trackbar control inside test
    HWND slider = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                                  WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                  10, 50, 280, 40, hwnd, NULL, hInstance, NULL);
    if (!slider) {
        printf("Trackbar control creation failed in TDD test.\n");
        DestroyWindow(hwnd);
        return 1;
    }

    // Pass
    DestroyWindow(hwnd);
    printf("Passed test_controlbox\n");
    return 0;
}
