#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wininet.h>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "Resource.h"

#pragma comment(lib, "wininet.lib")

const std::wstring CURRENT_VERSION = L"1.0.7"; // CHANGE THIS WHEN UPDATING

HHOOK hMouseHook;
std::chrono::steady_clock::time_point lastLeftClickTime = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point lastRightClickTime = std::chrono::steady_clock::now();
int leftDebounceTime = 50;
int rightDebounceTime = 50;

HWND hLeftTrackbar, hRightTrackbar;
HWND hLeftDebounceEdit, hRightDebounceEdit, hNotificationField, hCopyLogsButton, hHideToTrayButton, hOpenSourceButton, hLeftResetButton, hRightResetButton, hLinkDebouncesCheckbox, hLockCheckbox, hCheckForUpdatesButton;
NOTIFYICONDATA nid;
bool isHiddenToTray = false;
bool linkDebounces = false;
bool internalUpdate = false;
bool lockDebounces = false;



#define WM_UPDATE_LEFT_DEBOUNCE (WM_USER + 1)
#define WM_UPDATE_RIGHT_DEBOUNCE (WM_USER + 2)
#define WM_COPY_LOGS (WM_USER + 3)
#define WM_HIDE_TO_TRAY (WM_USER + 4)
#define WM_TRAY_ICON (WM_USER + 5)
#define WM_LINK_DEBOUNCES (WM_USER + 6)

#define TRAY_ICON_ID 1

class Config {
public:
    Config() {
        LoadConfiguration();
    }

    ~Config() {
        SaveConfiguration();
    }

    int GetLeftDebounceTime() const { return leftDebounceTime; }
    void SetLeftDebounceTime(int time) { leftDebounceTime = time; }

    int GetRightDebounceTime() const { return rightDebounceTime; }
    void SetRightDebounceTime(int time) { rightDebounceTime = time; }

    bool IsLinkDebounces() const { return linkDebounces; }
    void SetLinkDebounces(bool link) { linkDebounces = link; }

    bool IsLockDebounces() const { return lockDebounces; }
    void SetLockDebounces(bool lock) { lockDebounces = lock; }

    bool IsHiddenToTray() const { return isHiddenToTray; }
    void SetHiddenToTray(bool hidden) { isHiddenToTray = hidden; }

private:
    int leftDebounceTime = 0;
    int rightDebounceTime = 0;
    bool linkDebounces = false;
    bool lockDebounces = false;
    bool isHiddenToTray = false;

    void CreateConfigDirectory() {
        std::filesystem::path configPath = GetConfigPath();
        if (!std::filesystem::exists(configPath)) {
            std::filesystem::create_directory(configPath);
        }
    }

    void SaveConfiguration() {
        CreateConfigDirectory();
        std::filesystem::path configFilePath = GetConfigPath() / L"config.txt";
        
        std::wofstream configFile(configFilePath);
        if (configFile.is_open()) {
            configFile << leftDebounceTime << L'\n';
            configFile << rightDebounceTime << L'\n';
            configFile << (linkDebounces ? 1 : 0) << L'\n';
            configFile << (lockDebounces ? 1 : 0) << L'\n';
            configFile << (isHiddenToTray ? 1 : 0) << L'\n';
            configFile.close();
        }
    }

    void LoadConfiguration() {
        CreateConfigDirectory();
        std::filesystem::path configFilePath = GetConfigPath() / L"config.txt";
        
        std::wifstream configFile(configFilePath);
        if (configFile.is_open()) {
            configFile >> leftDebounceTime;
            configFile >> rightDebounceTime;
            int linkDebouncesInt;
            int lockDebouncesInt;
            int isHiddenToTrayInt;
            configFile >> linkDebouncesInt;
            configFile >> lockDebouncesInt;
            configFile >> isHiddenToTrayInt;
            
            linkDebounces = (linkDebouncesInt != 0);
            lockDebounces = (lockDebouncesInt != 0);
            isHiddenToTray = (isHiddenToTrayInt != 0);
            
            configFile.close();
        }
    }

    std::filesystem::path GetConfigPath() const {
        wchar_t appdataPath[MAX_PATH];
        if (GetEnvironmentVariableW(L"APPDATA", appdataPath, MAX_PATH)) {
            return std::filesystem::path(appdataPath) / L"BetterDCPrevent";
        } else {
            return L"";
        }
    }
};

Config config;

void UpdateUIFromConfig() {
    SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, config.GetLeftDebounceTime());
    SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, config.GetRightDebounceTime());
    SetWindowText(hLeftDebounceEdit, std::to_wstring(config.GetLeftDebounceTime()).c_str());
    SetWindowText(hRightDebounceEdit, std::to_wstring(config.GetRightDebounceTime()).c_str());
    SendMessage(hLinkDebouncesCheckbox, BM_SETCHECK, config.IsLinkDebounces() ? BST_CHECKED : BST_UNCHECKED, 0);
    isHiddenToTray = config.IsHiddenToTray();
}

void SaveConfigChanges() {
    config.SetLeftDebounceTime(leftDebounceTime);
    config.SetRightDebounceTime(rightDebounceTime);
    config.SetLinkDebounces(linkDebounces);
    config.SetHiddenToTray(isHiddenToTray);
}

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

std::wstring ConvertUtf8ToWString(const std::string& utf8Str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), &wstrTo[0], size_needed);
    return wstrTo;
}

bool CheckForUpdates(HWND hwnd, bool isStartup) {
    const std::wstring versionUrl = L"https://raw.githubusercontent.com/jqms/BetterDCPrevent/master/version.txt";
    const std::wstring latestReleaseUrl = L"https://github.com/jqms/BetterDCPrevent/releases/latest";

    HINTERNET hInternet = InternetOpen(L"Version Checker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        MessageBox(hwnd, L"Failed to initialize internet connection.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    HINTERNET hConnect = InternetOpenUrl(hInternet, versionUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        MessageBox(hwnd, L"Failed to open URL for version check.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::string latestVersion;
    char buffer[1024];
    DWORD bytesRead = 0;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        latestVersion.append(buffer);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    std::wstring latestVersionW = ConvertUtf8ToWString(latestVersion);

    latestVersionW.erase(std::remove(latestVersionW.begin(), latestVersionW.end(), L'\r'), latestVersionW.end());
    latestVersionW.erase(std::remove(latestVersionW.begin(), latestVersionW.end(), L'\n'), latestVersionW.end());

    if (isStartup) {
	    UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] Current version: " + CURRENT_VERSION);
		UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] Server version: " + latestVersionW);
    }

    if (latestVersionW != CURRENT_VERSION) {
        int result = MessageBox(hwnd, (L"Your version is outdated.\nCurrent version: " + CURRENT_VERSION + L".\nLatest version: " + latestVersionW + L"\nDo you want to update?").c_str(),
                                L"Update Available", MB_YESNO | MB_ICONINFORMATION);
        if (result == IDYES) {
            ShellExecute(hwnd, L"open", latestReleaseUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        return true;
    } else {
        if (!isStartup) {
            MessageBox(hwnd, (L"You are up to date.\nCurrent version: " + CURRENT_VERSION + L"\nServer version: " + latestVersionW).c_str(),
                        L"No Update Needed", MB_OK | MB_ICONINFORMATION);
        }
        return false;
    }
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
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(leftDebounceTime) + L"ms] Suppressed double click (Left Button)");
                return 1;
            }
            lastLeftClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(leftDebounceTime) + L"ms] Detected left click");
        }
        else if (wParam == WM_RBUTTONDOWN) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastRightClickTime);
            if (duration.count() < rightDebounceTime) {
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(rightDebounceTime) + L"ms] Suppressed double click (Right Button)");
                return 1;
            }
            lastRightClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(rightDebounceTime) + L"ms] Detected right click");
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

void HandleDebounceUpdate(HWND editControl, int& debounceTime, HWND trackbarControl, WPARAM wParam) {
    debounceTime = (int)wParam;

    SendMessage(editControl, WM_SETREDRAW, FALSE, 0);
    SetWindowText(editControl, std::to_wstring(debounceTime).c_str());
    SendMessage(editControl, WM_SETREDRAW, TRUE, 0);

    SendMessage(trackbarControl, TBM_SETPOS, TRUE, debounceTime);

    InvalidateRect(editControl, NULL, TRUE);
}


void PostDebounceUpdate(HWND hwnd, UINT msg, int debounceTime) {
    if (!internalUpdate) {
        internalUpdate = true;
        PostMessage(hwnd, msg, (WPARAM)debounceTime, 0);
        SaveConfigChanges();
    }
}

void ResetDebounceValue(HWND editControl, int& debounceTime, HWND trackbarControl) {
    debounceTime = 50;
    SendMessage(trackbarControl, TBM_SETPOS, TRUE, debounceTime);
    SetWindowText(editControl, std::to_wstring(debounceTime).c_str());
    SaveConfigChanges();
}

void SafeUpdate(HWND editControl, HWND trackbarControl, int& debounceTime, bool isLinking) {
    if (editControl && trackbarControl) {
        if (isLinking) {
            SendMessage(trackbarControl, TBM_SETPOS, TRUE, debounceTime);
            SetWindowText(editControl, std::to_wstring(debounceTime).c_str());
        }
    }
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
	        320, 45, 50, 20,
	        hwnd, NULL, GetModuleHandle(NULL), NULL);

	    hLinkDebouncesCheckbox = CreateWindow(L"BUTTON", L"Link",
	        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
	        310, 80, 60, 20,
	        hwnd, (HMENU)4, GetModuleHandle(NULL), NULL);

		hLockCheckbox = CreateWindow(L"BUTTON", L"Lock",
		    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		    380, 80, 60, 20,
		    hwnd, (HMENU)8, GetModuleHandle(NULL), NULL);
        CheckDlgButton(hwnd, 8, BST_UNCHECKED);


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

		CreateWindow(L"STATIC", L"By Jqms", WS_VISIBLE | WS_CHILD, 380, 10, 60, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);

	    hRightDebounceEdit = CreateWindow(L"EDIT", std::to_wstring(rightDebounceTime).c_str(),
	        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
	        320, 115, 50, 20,
	        hwnd, NULL, GetModuleHandle(NULL), NULL);

	    hLeftResetButton = CreateWindow(L"BUTTON", L"Reset",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        380, 40, 60, 30,
	        hwnd, (HMENU)5, GetModuleHandle(NULL), NULL);

	    hRightResetButton = CreateWindow(L"BUTTON", L"Reset",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        380, 110, 60, 30,
	        hwnd, (HMENU)6, GetModuleHandle(NULL), NULL);

	    hCopyLogsButton = CreateWindow(L"BUTTON", L"Copy Logs",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        10, 275, 80, 30,
	        hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

	    hCheckForUpdatesButton = CreateWindow(L"BUTTON", L"Check for Updates",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        100, 275, 130, 30,
	        hwnd, (HMENU)7, GetModuleHandle(NULL), NULL);

	    hHideToTrayButton = CreateWindow(L"BUTTON", L"Hide to Tray",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        240, 275, 90, 30,
	        hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);

	    hOpenSourceButton = CreateWindow(L"BUTTON", L"Source Code",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        340, 275, 90, 30,
	        hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);

	    hNotificationField = CreateWindow(L"EDIT", L"",
	        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
	        10, 150, 420, 120,
	        hwnd, NULL, GetModuleHandle(NULL), NULL);
	    SendMessage(hNotificationField, EM_SETREADONLY, TRUE, 0);

        UpdateUIFromConfig();
	    UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] First Opened");
	    break;

    case WM_HSCROLL:
        if ((HWND)lParam == hLeftTrackbar) {
            leftDebounceTime = static_cast<int>(SendMessage(hLeftTrackbar, TBM_GETPOS, 0, 0));
            if (!linkDebounces || !internalUpdate) {
                PostDebounceUpdate(hwnd, WM_UPDATE_LEFT_DEBOUNCE, leftDebounceTime);
            }
        }
        else if ((HWND)lParam == hRightTrackbar) {
            rightDebounceTime = static_cast<int>(SendMessage(hRightTrackbar, TBM_GETPOS, 0, 0));
            if (!linkDebounces || !internalUpdate) {
                PostDebounceUpdate(hwnd, WM_UPDATE_RIGHT_DEBOUNCE, rightDebounceTime);
            }
        }
        break;

	case WM_COMMAND:
	    if (HIWORD(wParam) == BN_CLICKED) {
	        switch (LOWORD(wParam)) {
	            case 1:
	                CopyLogsToClipboard(hwnd);
	                MessageBox(hwnd, L"Logs copied to clipboard!", L"Info", MB_OK | MB_ICONINFORMATION);
					UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] Logs copied to clipboard");
	                break;

	            case 2:
	                ShowWindow(hwnd, SW_HIDE);
					SaveConfigChanges();
	                ShowTrayIcon(hwnd);
	                isHiddenToTray = true;
	                break;

				case 3:
	                ShellExecute(hwnd, L"open", L"https://github.com/jqms/BetterDCPrevent", NULL, NULL, SW_SHOWNORMAL);
	                break;

	            case 4:
	                linkDebounces = !linkDebounces;
	                SendMessage(hLinkDebouncesCheckbox, BM_SETCHECK, linkDebounces ? BST_CHECKED : BST_UNCHECKED, 0);
	                break;

	            case 5:
	                ResetDebounceValue(hLeftDebounceEdit, leftDebounceTime, hLeftTrackbar);
	                PostDebounceUpdate(hwnd, WM_UPDATE_LEFT_DEBOUNCE, leftDebounceTime);
	                break;

	            case 6:
	                ResetDebounceValue(hRightDebounceEdit, rightDebounceTime, hRightTrackbar);
	                PostDebounceUpdate(hwnd, WM_UPDATE_RIGHT_DEBOUNCE, rightDebounceTime);
	                break;

	            case 7:
	                CheckForUpdates(hwnd, false);
					SaveConfigChanges();
	                break;
                case 8:
	                if (SendMessage(hLockCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED) {
	                    int result = MessageBox(hwnd,
	                        L"Are you sure you want to lock the debounce settings? You will need to restart the program to modify them again.",
	                        L"Lock Settings", MB_YESNO | MB_ICONQUESTION);
	                    
	                    if (result == IDYES)
	                    {
	                        lockDebounces = true;
	                        EnableWindow(hLeftTrackbar, FALSE);
	                        EnableWindow(hRightTrackbar, FALSE);
	                        EnableWindow(hLeftDebounceEdit, FALSE);
	                        EnableWindow(hRightDebounceEdit, FALSE);
	                        EnableWindow(hLeftResetButton, FALSE);
	                        EnableWindow(hRightResetButton, FALSE);
	                        EnableWindow(hLinkDebouncesCheckbox, FALSE);
	                        EnableWindow(hLockCheckbox, FALSE);

                            UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] Debounce settings locked");
	                    }
	                    else
	                    {
	                        CheckDlgButton(hwnd, 8, BST_UNCHECKED);
	                    }
	                }
					break;
	        }
	    } else if (HIWORD(wParam) == EN_CHANGE) {
	        if ((HWND)lParam == hLeftDebounceEdit) {
	            int newLeftDebounceTime = 0;
	            WCHAR text[256];
	            GetWindowText(hLeftDebounceEdit, text, 256);
	            std::wstringstream ss(text);
	            ss >> newLeftDebounceTime;

                if (newLeftDebounceTime < 0) {
                    newLeftDebounceTime = 0;
					PostDebounceUpdate(hwnd, WM_UPDATE_LEFT_DEBOUNCE, newLeftDebounceTime);
                }
                else if (newLeftDebounceTime > 100) {
                    newLeftDebounceTime = 100;
					PostDebounceUpdate(hwnd, WM_UPDATE_LEFT_DEBOUNCE, newLeftDebounceTime);
                }

	            if (leftDebounceTime != newLeftDebounceTime) {
	                leftDebounceTime = newLeftDebounceTime;
	                SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, leftDebounceTime);
	                if (linkDebounces && !internalUpdate) {
	                    internalUpdate = true;
	                    SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, leftDebounceTime);
	                    SetWindowText(hRightDebounceEdit, std::to_wstring(leftDebounceTime).c_str());
	                    internalUpdate = false;
	                }
	            }
	        } else if ((HWND)lParam == hRightDebounceEdit) {
	            int newRightDebounceTime = 0;
	            WCHAR text[256];
	            GetWindowText(hRightDebounceEdit, text, 256);
	            std::wstringstream ss(text);
	            ss >> newRightDebounceTime;

                if (newRightDebounceTime < 0) {
                    newRightDebounceTime = 0;
					PostDebounceUpdate(hwnd, WM_UPDATE_RIGHT_DEBOUNCE, newRightDebounceTime);
                }
                else if (newRightDebounceTime > 100) {
                    newRightDebounceTime = 100;
					PostDebounceUpdate(hwnd, WM_UPDATE_RIGHT_DEBOUNCE, newRightDebounceTime);
                }

	            if (rightDebounceTime != newRightDebounceTime) {
	                rightDebounceTime = newRightDebounceTime;
	                SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, rightDebounceTime);
	                if (linkDebounces && !internalUpdate) {
	                    internalUpdate = true;
	                    SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, rightDebounceTime);
	                    SetWindowText(hLeftDebounceEdit, std::to_wstring(rightDebounceTime).c_str());
	                    internalUpdate = false;
	                }
	            }
	        }
	    }
	    break;

    case WM_UPDATE_LEFT_DEBOUNCE:
        internalUpdate = true;
        HandleDebounceUpdate(hLeftDebounceEdit, leftDebounceTime, hLeftTrackbar, wParam);
        if (linkDebounces) {
            SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, leftDebounceTime);
            SetWindowText(hRightDebounceEdit, std::to_wstring(leftDebounceTime).c_str());
        }
        internalUpdate = false;
        break;

    case WM_UPDATE_RIGHT_DEBOUNCE:
        internalUpdate = true;
        HandleDebounceUpdate(hRightDebounceEdit, rightDebounceTime, hRightTrackbar, wParam);
        if (linkDebounces) {
            SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, rightDebounceTime);
            SetWindowText(hLeftDebounceEdit, std::to_wstring(rightDebounceTime).c_str());
        }
        internalUpdate = false;
        break;

    case WM_SETFOCUS:
		SaveConfigChanges();
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
		SaveConfigChanges();
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

    HWND hwnd = CreateWindow(wc.lpszClassName, (L"Double Click Prevent v" + CURRENT_VERSION).c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 460, 350,
        NULL, NULL, hInstance, NULL);

    CheckForUpdates(hwnd, true);

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