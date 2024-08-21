#include <windows.h>
#include <commctrl.h>
#include <chrono>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <shellapi.h>

HHOOK hMouseHook;
std::chrono::steady_clock::time_point lastLeftClickTime = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point lastRightClickTime = std::chrono::steady_clock::now();
int leftDebounceTime = 50;
int rightDebounceTime = 50;

HWND hLeftTrackbar, hRightTrackbar;
HWND hLeftDebounceEdit, hRightDebounceEdit, hNotificationField, hCopyLogsButton, hHideToTrayButton, hOpenSourceButton;
NOTIFYICONDATA nid;
bool isHiddenToTray = false;

#define WM_UPDATE_LEFT_DEBOUNCE (WM_USER + 1)
#define WM_UPDATE_RIGHT_DEBOUNCE (WM_USER + 2)
#define WM_COPY_LOGS (WM_USER + 3)
#define WM_HIDE_TO_TRAY (WM_USER + 4)
#define WM_TRAY_ICON (WM_USER + 5)

#define TRAY_ICON_ID 1

bool updatingControls = false;

void UpdateNotificationField(const std::wstring& message) {
    int length = GetWindowTextLength(hNotificationField);
    std::wstring currentText(length, L'\0');
    GetWindowText(hNotificationField, &currentText[0], length + 1);
    currentText += message + L"\r\n";

    SetWindowText(hNotificationField, currentText.c_str());
    SendMessage(hNotificationField, EM_SETSEL, currentText.length(), currentText.length());
    SendMessage(hNotificationField, EM_SCROLLCARET, 0, 0);
}

std::wstring GetCurrentDateTimeString(bool includeDate) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    struct tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);

    std::wstringstream ss;
    if (includeDate) {
        ss << std::put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S");
    }
    else {
        ss << std::put_time(&timeinfo, L"%H:%M:%S");
    }
    return ss.str();
}

void PostDebounceUpdate(HWND hwnd, UINT msg, int debounceTime) {
    if (!updatingControls) {
        updatingControls = true;
        PostMessage(hwnd, msg, (WPARAM)debounceTime, 0);
    }
}

void HandleDebounceUpdate(HWND editControl, int& debounceTime, HWND trackbarControl, WPARAM wParam) {
    debounceTime = (int)wParam;

    SendMessage(editControl, WM_SETREDRAW, FALSE, 0);
    SetWindowText(editControl, std::to_wstring(debounceTime).c_str());
    SendMessage(editControl, WM_SETREDRAW, TRUE, 0);

    SendMessage(trackbarControl, TBM_SETPOS, TRUE, debounceTime);

    InvalidateRect(editControl, NULL, TRUE);
    updatingControls = false;
}

void CopyLogsToClipboard(HWND hwnd) {
    int length = GetWindowTextLength(hNotificationField);
    if (length == 0) return;

    std::wstring logText(length, L'\0');
    GetWindowText(hNotificationField, &logText[0], length + 1);

    if (OpenClipboard(hwnd)) {
        EmptyClipboard();

        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, (logText.length() + 1) * sizeof(wchar_t));
        if (hClipboardData) {
            wchar_t* pchData = (wchar_t*)GlobalLock(hClipboardData);
            if (pchData) {
                wcscpy_s(pchData, logText.length() + 1, logText.c_str());
                GlobalUnlock(hClipboardData);

                SetClipboardData(CF_UNICODETEXT, hClipboardData);
            }
        }

        CloseClipboard();
    }
}

HICON hCustomIcon;
#include "Resource.h"
void ShowTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_ICON;
    nid.hIcon = hCustomIcon;
    wcscpy_s(nid.szTip, L"Double Click Prevent");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        auto currentTime = std::chrono::steady_clock::now();

        if (wParam == WM_LBUTTONDOWN) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLeftClickTime);
            if (duration.count() < leftDebounceTime) {
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(leftDebounceTime) + L"ms] Suppressed a DC Left Button");
                return 1;
            }
            lastLeftClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(leftDebounceTime) + L"ms] MouseDown Left Button");
        }
        else if (wParam == WM_RBUTTONDOWN) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastRightClickTime);
            if (duration.count() < rightDebounceTime) {
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(rightDebounceTime) + L"ms] Suppressed a DC Right Button");
                return 1;
            }
            lastRightClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(rightDebounceTime) + L"ms] MouseDown Right Button");
        }
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

void UpdateDebounceValuesFromEdit(HWND editControl, int& debounceTime, HWND trackbarControl) {
    WCHAR text[256];
    GetWindowText(editControl, text, 256);

    int newDebounceTime = 0;
    std::wstringstream ss(text);
    ss >> newDebounceTime;

    if (newDebounceTime < 0) {
        newDebounceTime = 0;
    }
    else if (newDebounceTime > 100) {
        newDebounceTime = 100;
    }

    debounceTime = newDebounceTime;

    SendMessage(trackbarControl, TBM_SETPOS, TRUE, debounceTime);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hCustomIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hCustomIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hCustomIcon);
        CreateWindow(L"STATIC", L"Left Click Debounce:", WS_VISIBLE | WS_CHILD,
            10, 10, 150, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

        hLeftTrackbar = CreateWindowEx(
            0, TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            10, 40, 300, 30,
            hwnd, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessage(hLeftTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, leftDebounceTime);
        SendMessage(hLeftTrackbar, TBM_SETTICFREQ, 10, 0);

        hLeftDebounceEdit = CreateWindow(L"EDIT", std::to_wstring(leftDebounceTime).c_str(),
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            320, 40, 50, 20,
            hwnd, NULL, GetModuleHandle(NULL), NULL);

        CreateWindow(L"STATIC", L"Right Click Debounce:", WS_VISIBLE | WS_CHILD,
            10, 80, 150, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

        hRightTrackbar = CreateWindowEx(
            0, TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            10, 110, 300, 30,
            hwnd, NULL, GetModuleHandle(NULL), NULL
        );
        SendMessage(hRightTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, rightDebounceTime);
        SendMessage(hRightTrackbar, TBM_SETTICFREQ, 10, 0);

        hRightDebounceEdit = CreateWindow(L"EDIT", std::to_wstring(rightDebounceTime).c_str(),
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            320, 110, 50, 20,
            hwnd, NULL, GetModuleHandle(NULL), NULL);

        hNotificationField = CreateWindow(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
            10, 150, 420, 120,
            hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hNotificationField, EM_SETREADONLY, TRUE, 0);

        hCopyLogsButton = CreateWindow(L"BUTTON", L"Copy Logs",
            WS_VISIBLE | WS_CHILD,
            10, 275, 80, 30,
            hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

        hHideToTrayButton = CreateWindow(L"BUTTON", L"Hide to Tray",
            WS_VISIBLE | WS_CHILD,
            100, 275, 90, 30,
            hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);

        hOpenSourceButton = CreateWindow(L"BUTTON", L"Source Code",
            WS_VISIBLE | WS_CHILD,
            200, 275, 100, 30,
            hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);

        UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] First Opened");

        break;

    case WM_HSCROLL:
        if ((HWND)lParam == hLeftTrackbar) {
            leftDebounceTime = static_cast<int>(SendMessage(hLeftTrackbar, TBM_GETPOS, 0, 0));
            PostDebounceUpdate(hwnd, WM_UPDATE_LEFT_DEBOUNCE, leftDebounceTime);
        }
        else if ((HWND)lParam == hRightTrackbar) {
            rightDebounceTime = static_cast<int>(SendMessage(hRightTrackbar, TBM_GETPOS, 0, 0));
            PostDebounceUpdate(hwnd, WM_UPDATE_RIGHT_DEBOUNCE, rightDebounceTime);
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE) {
            if ((HWND)lParam == hLeftDebounceEdit) {
                UpdateDebounceValuesFromEdit(hLeftDebounceEdit, leftDebounceTime, hLeftTrackbar);
            }
            else if ((HWND)lParam == hRightDebounceEdit) {
                UpdateDebounceValuesFromEdit(hRightDebounceEdit, rightDebounceTime, hRightTrackbar);
            }
        }
        else if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == 1) {
                CopyLogsToClipboard(hwnd);
                MessageBox(hwnd, L"Logs copied to clipboard!", L"Info", MB_OK | MB_ICONINFORMATION);
            }
            else if (LOWORD(wParam) == 2) {
                ShowWindow(hwnd, SW_HIDE);
                ShowTrayIcon(hwnd);
                isHiddenToTray = true;
            }
            else if (LOWORD(wParam) == 3) {
                ShellExecute(hwnd, L"open", L"https://github.com/jqms/BetterDCPrevent", NULL, NULL, SW_SHOWNORMAL);
            }
        }
        break;

    case WM_UPDATE_LEFT_DEBOUNCE:
        HandleDebounceUpdate(hLeftDebounceEdit, leftDebounceTime, hLeftTrackbar, wParam);
        break;

    case WM_UPDATE_RIGHT_DEBOUNCE:
        HandleDebounceUpdate(hRightDebounceEdit, rightDebounceTime, hRightTrackbar, wParam);
        break;

    case WM_SETFOCUS:
        if ((HWND)lParam == hLeftDebounceEdit || (HWND)lParam == hRightDebounceEdit) {
            SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
        }
        break;

    case WM_KILLFOCUS:
        if ((HWND)lParam == hLeftDebounceEdit || (HWND)lParam == hRightDebounceEdit) {
            SendMessage((HWND)lParam, EM_SETSEL, 0, -1);
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_DESTROY:
        RemoveTrayIcon(hwnd);
        DestroyIcon(hCustomIcon);
        PostQuitMessage(0);
        break;

    case WM_TRAY_ICON:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            if (isHiddenToTray) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                RemoveTrayIcon(hwnd);
                isHiddenToTray = false;
            }
        }
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = { 0 };
    hCustomIcon = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszClassName = L"myWindowClass";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc))
        return -1;

    HWND hwnd = CreateWindow(wc.lpszClassName, L"Double Click Prevent by Jqms",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 460, 350,
        NULL, NULL, hInstance, NULL);

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    if (!hMouseHook) {
        MessageBox(NULL, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);
    return (int)msg.wParam;
}
