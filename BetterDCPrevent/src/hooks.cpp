#include "pch.h"
#include "include/hooks.h"

#include "include/config.h"
#include "include/main.h"
#include "include/utils.h"

HHOOK hMouseHook;
Config config;
int leftDebounceTime = config.leftDebounceTime;
int rightDebounceTime = config.rightDebounceTime;
bool linkDebounces = config.linkDebounces;
bool isHiddenToTray = config.isHiddenToTray;
bool onlyApplyToMinecraftWindow = config.onlyApplyToMinecraftWindow;
std::chrono::steady_clock::time_point lastLeftClickTime;
std::chrono::steady_clock::time_point lastRightClickTime;
bool lockDebounces;
HWND hLeftTrackbar, hRightTrackbar, hLeftDebounceEdit, hRightDebounceEdit, hNotificationField;
HWND hCopyLogsButton, hHideToTrayButton, hOpenSourceButton, hLeftResetButton, hRightResetButton, hLinkDebouncesCheckbox, hLockCheckbox, hCheckForUpdatesButton, hStaticLeftClickDebounce, hStaticRightClickDebounce, hStaticByJqms, hOnlyWhenMCFocused;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        bool is50FromFocused = false;
        static bool hasSentMessage = false;
		int newLeftDebounceTime = leftDebounceTime;
        int newRightDebounceTime = rightDebounceTime;
        if (Utils::isMinecraftFocused()) {
            newLeftDebounceTime = 50;
			newRightDebounceTime = 50;
			is50FromFocused = true;
        }
        if (is50FromFocused) {
            if (!hasSentMessage) {
	            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(newLeftDebounceTime) + L"ms] Minecraft focused, forcing 50ms");
	            hasSentMessage = true;
	        }
			EnableWindow(hLeftTrackbar, FALSE);
			EnableWindow(hRightTrackbar, FALSE);
			EnableWindow(hLeftDebounceEdit, FALSE);
			EnableWindow(hRightDebounceEdit, FALSE);
			EnableWindow(hLeftResetButton, FALSE);
			EnableWindow(hRightResetButton, FALSE);
			EnableWindow(hLinkDebouncesCheckbox, FALSE);
			EnableWindow(hLockCheckbox, FALSE);
		}
        else {
            if (!lockDebounces) {
                if (hasSentMessage) {
					hasSentMessage = false; 
                }

                EnableWindow(hLeftTrackbar, TRUE);
                EnableWindow(hRightTrackbar, TRUE);
                EnableWindow(hLeftDebounceEdit, TRUE);
                EnableWindow(hRightDebounceEdit, TRUE);
                EnableWindow(hLeftResetButton, TRUE);
                EnableWindow(hRightResetButton, TRUE);
                EnableWindow(hLinkDebouncesCheckbox, TRUE);
                EnableWindow(hLockCheckbox, TRUE);
            }
        }
        if (onlyApplyToMinecraftWindow && !Utils::isMinecraftFocused()) {
            return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
        }
    	auto currentTime = std::chrono::steady_clock::now();
        if (wParam == WM_LBUTTONDOWN) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLeftClickTime);
            if (duration.count() < newLeftDebounceTime) {
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(newLeftDebounceTime) + L"ms] Suppressed double click (Left Button)");
                return 1;
            }
            lastLeftClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(newLeftDebounceTime) + L"ms] Detected left click");
        }
        else if (wParam == WM_RBUTTONDOWN) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastRightClickTime);
            if (duration.count() < newRightDebounceTime) {
                UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(newRightDebounceTime) + L"ms] Suppressed double click (Right Button)");
                return 1;
            }
            lastRightClickTime = currentTime;
            UpdateNotificationField(L"[" + GetCurrentDateTimeString(false) + L"] [" + std::to_wstring(newRightDebounceTime) + L"ms] Detected right click");
        }
    }

    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}
