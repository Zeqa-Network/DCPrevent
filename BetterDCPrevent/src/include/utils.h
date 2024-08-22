#pragma once
#include "../pch.h"

#define WM_UPDATE_LEFT_DEBOUNCE (WM_USER + 1)
#define WM_UPDATE_RIGHT_DEBOUNCE (WM_USER + 2)
#define WM_COPY_LOGS (WM_USER + 3)
#define WM_HIDE_TO_TRAY (WM_USER + 4)
#define WM_TRAY_ICON (WM_USER + 5)
#define WM_LINK_DEBOUNCES (WM_USER + 6)
#define TRAY_ICON_ID 1

void UpdateDebounceValuesFromEdit(HWND editControl, int& debounceTime, HWND trackbarControl);
void HandleDebounceUpdate(HWND editControl, int& debounceTime, HWND trackbarControl, WPARAM wParam);
void PostDebounceUpdate(HWND hwnd, UINT msg, int debounceTime);
