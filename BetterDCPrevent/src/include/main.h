#pragma once
#include "../pch.h"

void UpdateUIFromConfig();
void SaveConfigChanges();
void UpdateNotificationField(const std::wstring& message);
std::wstring GetCurrentDateTimeString(bool includeDate);
std::wstring ConvertUtf8ToWString(const std::string& utf8Str);
bool CheckForUpdates(HWND hwnd, bool isStartup);
void CopyLogsToClipboard(HWND hwnd);
void ShowTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void ResetDebounceValue(HWND editControl, int& debounceTime, HWND trackbarControl);
void SafeUpdate(HWND editControl, HWND trackbarControl, int& debounceTime, bool isLinking);