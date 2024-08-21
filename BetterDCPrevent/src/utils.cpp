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