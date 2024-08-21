#include "include/hooks.h"
#include <windows.h>
#include <chrono>

#include "include/config.h"
#include "include/main.h"

HHOOK hMouseHook;
Config config;
int leftDebounceTime = config.leftDebounceTime;
int rightDebounceTime = config.rightDebounceTime;
bool linkDebounces = config.linkDebounces;
bool isHiddenToTray = config.isHiddenToTray;
std::chrono::steady_clock::time_point lastLeftClickTime;
std::chrono::steady_clock::time_point lastRightClickTime;
bool lockDebounces;

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
