#include "InjectorWindow.h"
#include "ProcessDialog.h"
#include "../UI/Helpers/WindowHelper.h"
#include "../Injector/arraylist.h"
#include "../Injector/Processes.h"
#include "../Injector/injector.h"

#include <windows.h>
#include <Commctrl.h>
#include <Richedit.h>
#include <stdbool.h>
#include <stdio.h>

static HBRUSH BackgroundColorBrush;
static HBRUSH AccentColorBrush;
static HBRUSH WhiteColorBrush;
static HBRUSH AccentColorHighlightBrush;
static COLORREF WhiteColorRef = RGB(255,255,255);
static COLORREF AccentColorRef = RGB(130, 76, 169);
static COLORREF AccentColorHighlightRef = RGB(150, 85, 187);
static COLORREF BackgroundColorRef = RGB(45, 45, 45);
static HWND dllPathTextboxHandle;
static HWND loadedDllLabelHandle;
static HWND selectedPidLabelHandle;
static HWND statusLabelHandle;
static DWORD selectedPid = 0;
static bool isPendingProcessSelection = false;

static int buttonsHovered[3] = {0,0,0};
static HWND buttonsHoveredHwnds[3];

void process_picked(DWORD pid, const wchar_t* name, bool success) {
	isPendingProcessSelection = false;
	if (!success) {
		selectedPid = 0;
		SetWindowTextW(selectedPidLabelHandle, L"Could not select process");
		return;
	}
	size_t procNameLen= strlen(name);
	wchar_t* buffer = (wchar_t*)malloc((procNameLen+69) * sizeof(wchar_t));
	buffer[0] = L'\0';
	wcscat_s(buffer, procNameLen + 69, L"Selected Process: ");
	wcscat_s(buffer, procNameLen + 69, name);
	SetWindowTextW(selectedPidLabelHandle, buffer);
	selectedPid = pid;
}

int WINAPI pick_file_thread(LPVOID param) {
	OPENFILENAMEW ofd;
	memset(&ofd, 0, sizeof(ofd));
	ofd.lStructSize = sizeof(ofd);
	ofd.hwndOwner = (HWND)param;
	ofd.lpstrFilter = L"Dll Files (*.dll)\0*.dll\0";
	ofd.lpstrTitle = L"Select a DLL File";
	ofd.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
	ofd.lpstrDefExt = L"dll";
	wchar_t buffer[4096] = { 0 };
	ofd.lpstrFile = buffer;
	ofd.nMaxFile = sizeof(buffer) / sizeof(buffer[0]);

	if (GetOpenFileNameW(&ofd)) {
		SetWindowTextW(dllPathTextboxHandle, buffer);
		
		wchar_t* lastSlash = buffer;
		wchar_t* it = buffer;
		while (*it != L'\0') {
			if (*it == L'/' || *it ==  L'\\') {
				lastSlash = it;
			}
			++it;
		}
		if (*lastSlash == L'/' || *lastSlash == L'\\') {
			++lastSlash;
		}
		size_t filenameSize = strlen(lastSlash) + 69;
		wchar_t* buffer = (wchar_t*)malloc(filenameSize * sizeof(wchar_t));
		buffer[0] = L'\0';
		wcscat_s(buffer, filenameSize, L"Selected DLL: ");
		wcscat_s(buffer, filenameSize, lastSlash);
		SetWindowTextW(loadedDllLabelHandle, buffer);
		
	}
	return 0;
}

wchar_t* get_control_text(HWND hwnd);
LRESULT InjectorWindow_WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	InjectorWindow* injectorWindow = (InjectorWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (message) {
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wparam;
		SetBkColor(hdcStatic, BackgroundColorBrush);
		SetBkMode(hdcStatic, TRANSPARENT);
		SetTextColor(hdcStatic, WhiteColorRef);
		return (INT_PTR)BackgroundColorBrush;
	}
	case WM_CTLCOLORBTN: {
		bool isHovered = false;
		for (int i = 0; i < (int)_countof(buttonsHoveredHwnds); i++) {
			if (buttonsHoveredHwnds[i] == (HWND)lparam) {
				isHovered = buttonsHovered[i];
				break;
			}
		}
		HDC hdcButton = (HDC)wparam;
		if (isHovered)
			SetBkColor(hdcButton, AccentColorHighlightRef);
		else
			SetBkColor(hdcButton, AccentColorRef);
		SetBkMode(hdcButton, TRANSPARENT);
		SetTextColor(hdcButton, WhiteColorBrush);
		return (INT_PTR)(isHovered ? AccentColorHighlightBrush : AccentColorBrush);
	}
    case WM_DRAWITEM: {
		LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lparam;
		if (lpDrawItem->CtlType == ODT_BUTTON) {
			bool isHovered = false;
			for (int i = 0; i < (int)_countof(buttonsHoveredHwnds); i++) {
				if (buttonsHoveredHwnds[i] == lpDrawItem->hwndItem) {
					isHovered = buttonsHovered[i];
					break;
				}
			}

			if (isHovered) {
				SetBkColor(lpDrawItem->hDC, AccentColorHighlightRef);
				FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, AccentColorHighlightRef);
			} else {
				SetBkColor(lpDrawItem->hDC, AccentColorRef);
				FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, AccentColorRef);
			}
			SetTextColor(lpDrawItem->hDC, WhiteColorRef); // White text
			wchar_t* text = get_control_text(lpDrawItem->hwndItem);
			DrawTextW(lpDrawItem->hDC, text, -1, &lpDrawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			free(text);
			return TRUE;
		}
	}
	break;
	case WM_MOUSEMOVE: {
        POINT cursorPos;
		GetCursorPos(&cursorPos);
                
		for (int i = 0; i < (int)_countof(buttonsHoveredHwnds); i++) {
			RECT buttonRect;
			GetWindowRect(buttonsHoveredHwnds[i], &buttonRect);
			if (PtInRect(&buttonRect, cursorPos)) {
				if (buttonsHovered[i] == 0) {
					buttonsHovered[i] = 1;
					InvalidateRect(buttonsHoveredHwnds[i], NULL, TRUE);
				}
			}
			else if (buttonsHovered[i] == 1) {
				buttonsHovered[i] = 0;
				InvalidateRect(buttonsHoveredHwnds[i], NULL, TRUE);
			}
		}
	}
	case WM_COMMAND: {
		int controlId = LOWORD(wparam);
		if (controlId == 3) { // "Browse Me" button
			CreateThread(0, 0, pick_file_thread, (LPVOID)hwnd, 0, 0);
		} else if (controlId == 1 && !isPendingProcessSelection) { // Select Process button
			isPendingProcessSelection = true;
			ProcessDialog_init(process_picked);
		} else if (controlId == 4) { // Inject Button
			if (selectedPid == 0) {
				break;
			}
			wchar_t* dll = get_control_text(dllPathTextboxHandle);
			FILE* f;
			_wfopen_s(&f, dll, L"rb");
			if (!f) {  
				SetWindowTextW(statusLabelHandle, L"The DLL is not present at this location.");
				return TRUE;
			}
			fclose(f); 

			DWORD injectStatus = inject_dll(selectedPid, dll);
			const wchar_t* statusText = L"Status: Unknown error occured, good luck :/";
			switch (injectStatus) {
			case 0:
				statusText = L"Status: Success";
				break;
			case 1:
				statusText = L"Status: The process could not be opened, is it closed?";
				break;
			case 2:
				statusText = L"Status: Could not allocate memory in the remote process.";
				break;
			case 3:
				statusText = L"Status: Failed to write path in the remote process memory."; 
				break;
			case 4:
				statusText = L"Status: Failed to create remote thread.";
				break;
			}
			SetWindowTextW(statusLabelHandle, statusText);
		}
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT InjectorWindow_Button_WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	if (message == WM_MOUSEMOVE) {
		return InjectorWindow_WindowProc(hwnd, message, wparam, lparam);
	}
	return DefSubclassProc(hwnd, message, wparam, lparam);
}

void InjectorWindow_init(InjectorWindow* injectorWindow) {
	//here is the window background color
	BackgroundColorBrush = CreateSolidBrush(BackgroundColorRef);
	AccentColorBrush = CreateSolidBrush(AccentColorRef);
	AccentColorHighlightBrush = CreateSolidBrush(AccentColorHighlightRef);
	WhiteColorBrush = (HBRUSH)WHITE_BRUSH;

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = BackgroundColorBrush;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = InjectorWindow_WindowProc;
	wc.lpszClassName = injectorWindow->class_name;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassEx(&wc);

	int windowSizeX = 420;
	int windowSizeY = 190;
	RECT windowPosAndSize = get_centered_window_position(windowSizeX, windowSizeY);

	HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Balls Injector", WS_OVERLAPPEDWINDOW,
		windowPosAndSize.left, windowPosAndSize.top, windowPosAndSize.right, windowPosAndSize.bottom, NULL, NULL, wc.hInstance, injectorWindow);
	ShowWindow(hwnd, SW_SHOW);

	HWND openProcessesButton = CreateWindowEx(0, L"BUTTON", L"Open Processes",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
		10, 10, 400, 30, hwnd, (HMENU)1, GetModuleHandle(NULL), injectorWindow);
	make_font_hot_or_well_at_least_way_hotter(openProcessesButton);
	buttonsHoveredHwnds[0] = openProcessesButton;
	SetWindowSubclass(openProcessesButton, InjectorWindow_Button_WindowProc, 0, 0);

	HWND dllPathTextbox = CreateWindowEx(0, MSFTEDIT_CLASS, L"",
		WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL,
		10, 50, 310, 24, hwnd, (HMENU)2, GetModuleHandle(NULL), injectorWindow);
	make_font_hot_or_well_at_least_way_hotter(dllPathTextbox);
	SendMessage(dllPathTextbox, EM_SETCUEBANNER, TRUE, (LPARAM)L"Dll Path...");
	SendMessage(dllPathTextbox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(35, 35, 35));

	CHARFORMAT cf = get_char_fmt(dllPathTextbox, SCF_DEFAULT);
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = RGB(174, 118, 214);
	SendMessage(dllPathTextbox, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SetWindowLong(dllPathTextbox, GWL_STYLE, GetWindowLong(dllPathTextbox, GWL_STYLE) & ~WS_BORDER);
	dllPathTextboxHandle = dllPathTextbox;

	HWND browseButton = CreateWindowEx(0, L"BUTTON", L"Browse Me",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
		330, 50, 80, 24, hwnd, (HMENU)3, GetModuleHandle(NULL), injectorWindow);
	make_font_hot_or_well_at_least_way_hotter(browseButton);
	buttonsHoveredHwnds[1] = browseButton;
	SetWindowSubclass(browseButton, InjectorWindow_Button_WindowProc, 0, 0);

	HWND hStatusLabel = CreateWindowEx(0, L"STATIC", L"Status: Waiting for you to decide wth ur doing man what are you waiting for",
		WS_CHILD | WS_VISIBLE,
		10, 76, 400, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
	make_font_hot_or_well_at_least_way_hotter(hStatusLabel);
	statusLabelHandle = hStatusLabel;

	HWND hLoadedProcessLabel = CreateWindowEx(0, L"STATIC", L"Selected Process: None, really now? what are you waiting for i need a process to inject into... if you dont hurry up ill just inject into explorer and crash it...",
		WS_CHILD | WS_VISIBLE,
		10, 96, 400, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
	make_font_hot_or_well_at_least_way_hotter(hLoadedProcessLabel);
	selectedPidLabelHandle = hLoadedProcessLabel;

	HWND hLoadedDllLabel = CreateWindowEx(0, L"STATIC", L"Selected DLL: None, really now? what are you waiting for i need a dll to inject...",
		WS_CHILD | WS_VISIBLE,
		10, 116, 400, 20, hwnd, NULL, GetModuleHandle(NULL), NULL);
	make_font_hot_or_well_at_least_way_hotter(hLoadedDllLabel);
	loadedDllLabelHandle = hLoadedDllLabel;

	HWND injectButton = CreateWindowEx(0, L"BUTTON", L"Inject",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
		10, 150, 400, 30, hwnd, (HMENU)4, GetModuleHandle(NULL), injectorWindow);
	make_font_hot_or_well_at_least_way_hotter(injectButton);
	buttonsHoveredHwnds[2] = injectButton;
	SetWindowSubclass(injectButton, InjectorWindow_Button_WindowProc, 0, 0);

	injectorWindow->hwnd = hwnd;
}

void InjectorWindow_show(InjectorWindow* injectorWindow) {


}

void InjectorWindow_run(InjectorWindow* injectorWindow) {
	InjectorWindow_init(injectorWindow);
	InjectorWindow_show(injectorWindow);

	bool shouldExit = false;
	MSG msg;
	while (!shouldExit) {
		while (GetMessageW(&msg, NULL, 0, 0) != 0) {
			if (msg.message == WM_QUIT) {
				shouldExit = true;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}