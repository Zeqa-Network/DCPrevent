#include "pch.h"
#include "include/main.h"

#include "include/config.h"
#include "include/hooks.h"
#include "include/utils.h"
#include "include/wndproc.h"

const std::wstring CURRENT_VERSION = L"1.1.5"; // CHANGE THIS WHEN UPDATING

extern HHOOK hMouseHook;
extern HICON hCustomIcon;
NOTIFYICONDATA nid;
extern HWND hLeftTrackbar, hRightTrackbar, hLeftDebounceEdit, hRightDebounceEdit, hNotificationField;
extern HWND hCopyLogsButton, hHideToTrayButton, hOpenSourceButton, hLeftResetButton, hRightResetButton, hLinkDebouncesCheckbox, hLockCheckbox, hCheckForUpdatesButton, hOnlyWhenMCFocused;
extern bool isHiddenToTray;
extern bool linkDebounces;
extern bool internalUpdate;
extern bool lockDebounces;
extern bool onlyApplyToMinecraftWindow;
extern Config config;

extern std::chrono::steady_clock::time_point lastLeftClickTime;
extern std::chrono::steady_clock::time_point lastRightClickTime;
extern int leftDebounceTime;
extern int rightDebounceTime;

HICON hCustomIcon;

void UpdateUIFromConfig() {
    SendMessage(hLeftTrackbar, TBM_SETPOS, TRUE, config.GetLeftDebounceTime());
    SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, config.GetRightDebounceTime());
    SetWindowText(hLeftDebounceEdit, std::to_wstring(config.GetLeftDebounceTime()).c_str());
    SetWindowText(hRightDebounceEdit, std::to_wstring(config.GetRightDebounceTime()).c_str());
    SendMessage(hLinkDebouncesCheckbox, BM_SETCHECK, config.IsLinkDebounces() ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(hOnlyWhenMCFocused, BM_SETCHECK, config.GetOnlyApplyToMinecraftWindow() ? BST_CHECKED : BST_UNCHECKED, 0);
	isHiddenToTray = config.IsHiddenToTray();
}

void LoadConfigChanges() {
	leftDebounceTime = config.GetLeftDebounceTime();
	rightDebounceTime = config.GetRightDebounceTime();
	linkDebounces = config.IsLinkDebounces();
	isHiddenToTray = config.IsHiddenToTray();
	onlyApplyToMinecraftWindow = config.GetOnlyApplyToMinecraftWindow();
}

void SaveConfigChanges() {
    config.SetLeftDebounceTime(leftDebounceTime);
    config.SetRightDebounceTime(rightDebounceTime);
    config.SetLinkDebounces(linkDebounces);
    config.SetHiddenToTray(isHiddenToTray);
	config.SetOnlyApplyToMinecraftWindow(onlyApplyToMinecraftWindow);
}

void UpdateNotificationField(const std::wstring& message) {
    int length = GetWindowTextLength(hNotificationField);
    std::wstring currentText(length, L'\0');
    GetWindowText(hNotificationField, &currentText[0], length + 1);

	if (length > 0)
		currentText += L"\r\n";
    else
		currentText += L"";
	currentText += message;

    SetWindowText(hNotificationField, currentText.c_str());
    SendMessage(hNotificationField, EM_SETSEL, currentText.length(), currentText.length());
    SendMessage(hNotificationField, EM_SCROLLCARET, 0, 0);
}

void SetControlFont(HWND hwnd, int height, bool bold) {
    LOGFONT lf = { 0 };
    lf.lfHeight = height;
	lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");

    HFONT hFont = CreateFontIndirect(&lf);
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
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

struct UpdateParams {
    HWND hwnd;
    bool isStartup;
};

DWORD WINAPI CheckForUpdatesThread(LPVOID lpParam) {
    UpdateParams* params = (UpdateParams*)lpParam;
    HWND hwnd = params->hwnd;
    bool isStartup = params->isStartup;

    const std::wstring versionUrl = L"https://raw.githubusercontent.com/jqms/BetterDCPrevent/master/version.txt";
    const std::wstring latestReleaseUrl = L"https://github.com/jqms/BetterDCPrevent/releases/latest";

    HINTERNET hInternet = InternetOpen(L"Version Checker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        MessageBox(hwnd, L"Failed to initialize internet connection.", L"Error", MB_OK | MB_ICONERROR);
        delete params;
        return 0;
    }

    HINTERNET hConnect = InternetOpenUrl(hInternet, versionUrl.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        MessageBox(hwnd, L"Failed to open URL for version check.", L"Error", MB_OK | MB_ICONERROR);
        delete params;
        return 0;
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
    } else {
        if (!isStartup) {
            MessageBox(hwnd, (L"You are up to date.\nCurrent version: " + CURRENT_VERSION + L"\nServer version: " + latestVersionW).c_str(),
                        L"No Update Needed", MB_OK | MB_ICONINFORMATION);
        }
    }

    delete params;
    return 1;
}

bool CheckForUpdates(HWND hwnd, bool isStartup) {
    UpdateParams* params = new UpdateParams;
    params->hwnd = hwnd;
    params->isStartup = isStartup;

    HANDLE hThread = CreateThread(
        NULL,                   
        0,                      
        CheckForUpdatesThread,  
        params,                 
        0,                      
        NULL);        

    if (hThread) {
        CloseHandle(hThread);
        return true;
    } else {
        MessageBox(hwnd, L"Failed to create update check thread.", L"Error", MB_OK | MB_ICONERROR);
        delete params;
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

void ShowTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_ICON;
    nid.hIcon = hCustomIcon;
    wcscpy_s(nid.szTip, L"Zeqa Double Click Prevent");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &nid);
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = { 0 };
    hCustomIcon = (HICON)LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszClassName = L"myWindowClass";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc))
        return -1;

    HWND hwnd = CreateWindow(wc.lpszClassName, (L"Zeqa Double Click Prevent (v" + CURRENT_VERSION + L")").c_str(),
        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE),
        100, 100, 460, 350,
        NULL, NULL, hInstance, NULL);

	LoadConfigChanges();
    if (isHiddenToTray) {
		ShowTrayIcon(hwnd);
		ShowWindow(hwnd, SW_HIDE);
	}
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    if (!hMouseHook) {
        MessageBox(NULL, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);
        return 1;
    }

    CheckForUpdates(hwnd, true);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hMouseHook);
    SaveConfigChanges();
    return (int)msg.wParam;
}
