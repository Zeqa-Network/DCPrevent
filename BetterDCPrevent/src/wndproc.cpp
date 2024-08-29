#include "pch.h"

#include "include/wndproc.h"
#include "include/main.h"
#include "include/config.h"
#include "include/utils.h"


extern int leftDebounceTime;
extern int rightDebounceTime;
extern bool internalUpdate;
extern bool linkDebounces;
extern bool lockDebounces;
extern bool isHiddenToTray;
extern bool onlyApplyToMinecraftWindow;
extern Config config;
bool isMovingOrResizing = false;

extern HWND hLeftTrackbar, hRightTrackbar, hLeftDebounceEdit, hRightDebounceEdit, hNotificationField;
extern HWND hCopyLogsButton, hHideToTrayButton, hOpenSourceButton, hLeftResetButton, hRightResetButton, hLinkDebouncesCheckbox, hLockCheckbox, hCheckForUpdatesButton, hStaticLeftClickDebounce, hStaticRightClickDebounce, hStaticByJqms, hOnlyWhenMCFocused;
extern HICON hCustomIcon;
extern void SetControlFont(HWND hwnd, int height, bool bold);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
	case WM_CREATE:
	    hCustomIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON2));

	    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hCustomIcon);
	    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hCustomIcon);

	    hStaticLeftClickDebounce = CreateWindow(L"STATIC", L"Left Click Debounce:", WS_VISIBLE | WS_CHILD,
	        10, 10, 145, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
		SetControlFont(hStaticLeftClickDebounce, 20, true);
		
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
		SetControlFont(hLeftDebounceEdit, 17, false);

	    hLinkDebouncesCheckbox = CreateWindow(L"BUTTON", L"Link",
	        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
	        310, 80, 60, 20,
	        hwnd, (HMENU)4, GetModuleHandle(NULL), NULL);
		SetControlFont(hLinkDebouncesCheckbox, 17, false);


		hOnlyWhenMCFocused = CreateWindow(L"BUTTON", L"Minecraft Only",
			WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
			// top right corner
			310, 10, 120, 20,
			hwnd, (HMENU)9, GetModuleHandle(NULL), NULL);
		SetControlFont(hOnlyWhenMCFocused, 17, false);


		hLockCheckbox = CreateWindow(L"BUTTON", L"Lock",
		    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		    380, 80, 60, 20,
		    hwnd, (HMENU)8, GetModuleHandle(NULL), NULL);
		SetControlFont(hLockCheckbox, 17, false);
        CheckDlgButton(hwnd, 8, BST_UNCHECKED);


	    hStaticRightClickDebounce = CreateWindow(L"STATIC", L"Right Click Debounce:", WS_VISIBLE | WS_CHILD,
	        10, 80, 155, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
		SetControlFont(hStaticRightClickDebounce, 20, true);

	    hRightTrackbar = CreateWindowEx(
	        0, TRACKBAR_CLASS, NULL,
	        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
	        10, 110, 300, 30,
	        hwnd, NULL, GetModuleHandle(NULL), NULL
	    );
	    SendMessage(hRightTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
	    SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, rightDebounceTime);
	    SendMessage(hRightTrackbar, TBM_SETTICFREQ, 10, 0);

		//hStaticByJqms = CreateWindow(L"STATIC", L"By Jqms", WS_VISIBLE | WS_CHILD, 380, 10, 60, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
		//SetControlFont(hStaticByJqms, 20, true);

	    hRightDebounceEdit = CreateWindow(L"EDIT", std::to_wstring(rightDebounceTime).c_str(),
	        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
	        320, 115, 50, 20,
	        hwnd, NULL, GetModuleHandle(NULL), NULL);
		SetControlFont(hRightDebounceEdit, 17, false);

	    hLeftResetButton = CreateWindow(L"BUTTON", L"Reset",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        380, 40, 60, 30,
	        hwnd, (HMENU)5, GetModuleHandle(NULL), NULL);
		SetControlFont(hLeftResetButton, 17, false);

	    hRightResetButton = CreateWindow(L"BUTTON", L"Reset",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        380, 110, 60, 30,
	        hwnd, (HMENU)6, GetModuleHandle(NULL), NULL);
		SetControlFont(hRightResetButton, 17, false);

	    hCopyLogsButton = CreateWindow(L"BUTTON", L"Copy Logs",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        10, 275, 80, 30,
	        hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
		SetControlFont(hCopyLogsButton, 17, false);

	    hCheckForUpdatesButton = CreateWindow(L"BUTTON", L"Check for Updates",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        100, 275, 130, 30,
	        hwnd, (HMENU)7, GetModuleHandle(NULL), NULL);
		SetControlFont(hCheckForUpdatesButton, 17, false);

	    hHideToTrayButton = CreateWindow(L"BUTTON", L"Hide to Tray",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        240, 275, 90, 30,
	        hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
		SetControlFont(hHideToTrayButton, 17, false);

	    hOpenSourceButton = CreateWindow(L"BUTTON", L"Source Code",
	        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
	        340, 275, 90, 30,
	        hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);
		SetControlFont(hOpenSourceButton, 17, false);

	    hNotificationField = CreateWindow(L"EDIT", L"",
	        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
	        10, 150, 420, 120,
	        hwnd, NULL, GetModuleHandle(NULL), NULL);
		SetControlFont(hNotificationField, 17, false);
	    SendMessage(hNotificationField, EM_SETREADONLY, TRUE, 0);

        UpdateUIFromConfig();
	    UpdateNotificationField(L"[" + GetCurrentDateTimeString(true) + L"] First Opened");
	    break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        {
            HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);
        }
        EndPaint(hwnd, &ps);
		}
		break;
	case WM_ENTERSIZEMOVE:
	    isMovingOrResizing = true;
	    break;
	case WM_EXITSIZEMOVE:
	    isMovingOrResizing = false;
	    //EnableAcrylicInThread(hwnd);
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
	                isHiddenToTray = true;
					SaveConfigChanges();
					config.SaveConfiguration();
	                ShowTrayIcon(hwnd);
	                break;

				case 3:
	                ShellExecute(hwnd, L"open", L"https://github.com/Zeqa-Network/DCPrevent", NULL, NULL, SW_SHOWNORMAL);
	                break;

	            case 4:
	                linkDebounces = !linkDebounces;
	                SendMessage(hLinkDebouncesCheckbox, BM_SETCHECK, linkDebounces ? BST_CHECKED : BST_UNCHECKED, 0);
					if (linkDebounces) {
						internalUpdate = true;
						SendMessage(hRightTrackbar, TBM_SETPOS, TRUE, leftDebounceTime);
						SetWindowText(hRightDebounceEdit, std::to_wstring(leftDebounceTime).c_str());
						internalUpdate = false;
					}
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
				case 9:
					onlyApplyToMinecraftWindow = !onlyApplyToMinecraftWindow;
					SendMessage(hOnlyWhenMCFocused, BM_SETCHECK, onlyApplyToMinecraftWindow ? BST_CHECKED : BST_UNCHECKED, 0);

					break;

				case 10001:
					if (isHiddenToTray) {
						ShowWindow(hwnd, SW_RESTORE);
						SetForegroundWindow(hwnd);
						RemoveTrayIcon(hwnd);
						isHiddenToTray = false;
					}
					break;
				case 10003:
					SendMessage(hwnd, WM_CLOSE, 0, 0);
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
        } else if (lParam == WM_RBUTTONUP) {
				HMENU hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, 10001, isHiddenToTray ? TEXT("Show") : TEXT("Hide"));
				AppendMenu(hMenu, MF_MENUBREAK, 10002, nullptr);
				AppendMenu(hMenu, MF_STRING, 10003, TEXT("Exit"));

				POINT pt;
				GetCursorPos(&pt);

				SetForegroundWindow(hwnd);
				TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
				DestroyMenu(hMenu);
			}
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}