#include "pch.h"
#include "include/utils.h"
#include "include/config.h"
#include "include/main.h"

bool internalUpdate = false;
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

DWORD Utils::getPID(const char* procName) {
    DWORD* PidList = new DWORD[4096];
    DWORD damn = 0;
    if (!K32EnumProcesses(PidList, 4096 * sizeof(DWORD), &damn)) {
        delete[] PidList;
        return 0;
    }
    damn = min((int)damn, 4096);

    for (DWORD i = 0; i < damn; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PidList[i]);
        if (hProcess) {
            DWORD cbNeeded;
            HMODULE mod;
            if (!K32EnumProcessModules(hProcess, &mod, sizeof(HMODULE), &cbNeeded)) continue;
            char textualContent[128];
            K32GetModuleBaseNameA(hProcess, mod, textualContent, sizeof(textualContent) / sizeof(textualContent[0]));
            CloseHandle(hProcess);
            if (!strcmp(textualContent, procName)) {
                delete[] PidList;
                return PidList[i];
            }
        }
    }
    delete[] PidList;
    return 0;
}

bool Utils::isMinecraftOpen() {
    return getPID("Minecraft.Windows.exe") != 0;
}

bool Utils::isMinecraftOpen_Slow(int delay) {
    DWORD* PidList = new DWORD[4096];
    Sleep(1000);
    DWORD damn = 0;
    if (!K32EnumProcesses(PidList, 4096 * sizeof(DWORD), &damn)) {
        delete[] PidList;
        return false;
    }
    damn = min((int)damn, 4096);
    Sleep(1000);

    for (DWORD i = 0; i < damn; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PidList[i]);
        if (hProcess) {
            DWORD cbNeeded;
            HMODULE mod;
            if (!K32EnumProcessModules(hProcess, &mod, sizeof(HMODULE), &cbNeeded)) continue;
            char textualContent[128];
            K32GetModuleBaseNameA(hProcess, mod, textualContent, sizeof(textualContent) / sizeof(textualContent[0]));
            CloseHandle(hProcess);
            Sleep(delay);
            if (!strcmp(textualContent, "Minecraft.Windows.exe")) {
                delete[] PidList;
                Sleep(200);
                return true;
            }
        }
    }
    delete[] PidList;
    Sleep(200);
    return false;
}

HWND Utils::getMinecraftWindow() {
    HWND hwnd = FindWindowW(L"ApplicationFrameWindow", L"Minecraft");
    if (hwnd)
        return hwnd;

    std::vector<HWND> windows;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto& windows = *reinterpret_cast<std::vector<HWND>*>(lParam);
        windows.push_back(hwnd);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));

    for (HWND window : windows) {
        int len = GetWindowTextLengthW(window);
        std::wstring windowTitle;
        windowTitle.resize(len);
        GetWindowTextW(window, windowTitle.data(), len + 1);
        const size_t off = windowTitle.find(L" \u200E- Minecraft");
        if (off != std::wstring::npos) {
            return window;
        }
    }
    return 0;
}

bool Utils::isMinecraftFocused() {
    return GetForegroundWindow() == getMinecraftWindow();
}