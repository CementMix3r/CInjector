#include <stdbool.h>
#include <limits.h>
#include <Richedit.h>
#include <Commctrl.h>
#include <stdio.h>

#include "ProcessDialog.h"
#include "../UI/Helpers/WindowHelper.h"
#include "../Injector/Processes.h"
#include "../Injector/Arraylist.h"

static HBRUSH BackgroundColorBrush;
static HBRUSH AccentColorBrush;
static HBRUSH AccentColorHighlightBrush;
static HBRUSH WhiteColorBrush;
static COLORREF WhiteColorRef = RGB(255, 255, 255);
static COLORREF AccentColorRef = RGB(130, 76, 169);
static COLORREF AccentColorHighlightRef = RGB(150, 85, 187);
static COLORREF BackgroundColorRef = RGB(45, 45, 45);
static HANDLE processSearchTextboxHandle;
static void(*processDialogCallback)(DWORD pid, const wchar_t* name, bool success);
static size_t selectedProcess = SIZE_MAX;
static size_t selectedProcessBtn = SIZE_MAX;
static ArrayList UIProcessesList;


// you are responsible for freeing the memory returned.
wchar_t* process_button_text(DWORD pid) {
	wchar_t* procName = get_process_name(pid);
	if (procName == NULL) {
		wchar_t* text = (wchar_t*)malloc(69 * sizeof(wchar_t));
		wsprintf(text, L"%d", pid);
		return text;
	}
	else {
		size_t procNameLen = strlen(procName) + 100;
		wchar_t* text = (wchar_t*)malloc(procNameLen * sizeof(wchar_t));
		wsprintf(text, L"%d - %ls", pid, procName);
		free(procName);
		return text;
	}
}

bool search_case_insensitively(wchar_t* string, wchar_t* substr) {
	size_t k = 0;
	while (string[k] != L'\0') {
		size_t i = 0;
		bool fullMatch = true;
		for (; string[k + i] != L'\0' && substr[i] != L'\0'; i++) {
			if (tolower(string[k + i]) != tolower(substr[i])) {
				fullMatch = false;
				break;
			}
		}
		if (fullMatch) {
			if (substr[i] == L'\0') {
				return true;
			}
		}

		k++;
	}
	return false;
}

void fill_ui_processes(const wchar_t* searchText) {
	ArrayList pidsArray = get_pids();
	ArrayList temp = ArrayList_new(pidsArray.elementCount, sizeof(UIProcessInfo));
	temp.elementCount = 0;
	DWORD* pids = (DWORD*)pidsArray.memory;
	for (size_t i = 0; i < pidsArray.elementCount; i++) {
		DWORD pid = pids[i];
		wchar_t* procName = process_button_text(pid);
		if (*searchText == L'\0' || search_case_insensitively(procName, searchText)) {
			UIProcessInfo info = { pid, procName };
			ArrayList_add(&temp, &info);
		}
	}
	UIProcessesList = temp;
}

void set_scrollbar_for_window(HWND window) {
	RECT windowRect;
	GetClientRect(window, &windowRect);
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nPage = windowRect.bottom - 40;
	si.nMin = 0;
	si.nMax = UIProcessesList.elementCount * 30 + 20;
	SetScrollInfo(window, SB_VERT, &si, TRUE);
}

LRESULT ProcessDialog_WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_SIZE:
		set_scrollbar_for_window(hwnd);
		break;
	case WM_VSCROLL: {
		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
		GetScrollInfo(hwnd, SB_VERT, &si);
		int yPos = si.nPos;

		switch (LOWORD(wparam)) {
		case SB_LINEUP: yPos -= 10; break;
		case SB_LINEDOWN: yPos += 10; break;
		case SB_PAGEUP: yPos -= si.nPage; break;
		case SB_PAGEDOWN: yPos += si.nPage; break;
		case SB_THUMBTRACK: yPos = HIWORD(wparam); break;
		default: break;
		}
		yPos = max(0, min(yPos, si.nMax - si.nPage + 1));

		if (yPos != si.nPos) {
			si.nPos = yPos;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		// Create a back buffer to render onto
		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
		HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

		FillRect(memDC, &clientRect, BackgroundColorBrush);
		SetBkMode(memDC, TRANSPARENT);
		SetTextColor(memDC, WhiteColorRef);

		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
		GetScrollInfo(hwnd, SB_VERT, &si);

		clientRect.top += 40;
		HRGN clipArea = CreateRectRgn(clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
		SelectClipRgn(memDC, clipArea);

		RECT btnRect;
		btnRect.left = 10;
		btnRect.right = clientRect.right - 10;
		btnRect.top = clientRect.top + 10 - si.nPos;
		btnRect.bottom = btnRect.top + 24;

		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(hwnd, &cursorPos); // Simplified error handling

		for (int i = 0; i < (int)UIProcessesList.elementCount; i++) {
			if (btnRect.bottom > clientRect.top && btnRect.top < clientRect.bottom) {
				UIProcessInfo* process = ((UIProcessInfo*)UIProcessesList.memory) + i;
				bool isHovered = btnRect.left <= cursorPos.x && cursorPos.x <= btnRect.right &&
					btnRect.top <= cursorPos.y && cursorPos.y <= btnRect.bottom;
				HBRUSH brush = isHovered ? AccentColorHighlightBrush : AccentColorBrush;
				HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);

				RoundRect(memDC, btnRect.left, btnRect.top, btnRect.right, btnRect.bottom, 6, 6);
				SelectObject(memDC, oldBrush);

				DrawTextW(memDC, process->process_name, -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			btnRect.top += 30;
			btnRect.bottom += 30;
		}

		DeleteObject(clipArea);

		// Copy back buffer to window surface
		BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

		SelectObject(memDC, oldBitmap);
		DeleteObject(memBitmap);
		DeleteDC(memDC);

		EndPaint(hwnd, &ps);

		break;
	}
	case WM_LBUTTONDOWN: {
		if (selectedProcessBtn != SIZE_MAX) {
			selectedProcess = selectedProcessBtn;
			DestroyWindow(hwnd);
		}
		break;
	}
	case WM_MOUSEMOVE: {
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);

		SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
		GetScrollInfo(hwnd, SB_VERT, &si);

		clientRect.top += 40;

		RECT btnRect;
		btnRect.left = 10;
		btnRect.right = clientRect.right - 10;
		btnRect.top = clientRect.top + 10 - si.nPos;
		btnRect.bottom = btnRect.top + 24;

		POINT cursorPos;
		GetCursorPos(&cursorPos);
		bool preventClick = false;
		if (!ScreenToClient(hwnd, &cursorPos)) {
			preventClick = true;
		} else if (cursorPos.y < 40) {
			preventClick = true;
		}


		size_t newSelectedButton = SIZE_MAX;
		for (int i = 0; i < (int)UIProcessesList.elementCount; i++) {
			UIProcessInfo* process = ((UIProcessInfo*)UIProcessesList.memory) + i;
			bool isHovered = btnRect.left <= cursorPos.x && cursorPos.x <= btnRect.right &&
				btnRect.top <= cursorPos.y && cursorPos.y <= btnRect.bottom;

			if (isHovered && !preventClick) {
				newSelectedButton = i;
				break;
			}

			btnRect.top += 30;
			btnRect.bottom += 30;
		}

		if (newSelectedButton != selectedProcessBtn) {
			selectedProcessBtn = newSelectedButton;
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;
	}
	case WM_CTLCOLORSTATIC: {
		HDC hdcStatic = (HDC)wparam;
		SetBkColor(hdcStatic, BackgroundColorBrush);
		SetBkMode(hdcStatic, TRANSPARENT);
		SetTextColor(hdcStatic, WhiteColorRef);
		return (INT_PTR)BackgroundColorBrush;
	}
	case WM_CTLCOLORBTN: {
		HDC hdcButton = (HDC)wparam;
		SetBkColor(hdcButton, AccentColorBrush);
		SetBkMode(hdcButton, TRANSPARENT);
		SetTextColor(hdcButton, WhiteColorBrush);
		return (INT_PTR)AccentColorBrush;
	}
	case WM_DRAWITEM: {
		LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lparam;
		if (lpDrawItem->CtlType == ODT_BUTTON) {
			SetBkColor(lpDrawItem->hDC, AccentColorRef);
			FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, AccentColorRef);
			SetTextColor(lpDrawItem->hDC, WhiteColorRef); // White text
			wchar_t* text = get_control_text(lpDrawItem->hwndItem);
			DrawTextW(lpDrawItem->hDC, text, -1, &lpDrawItem->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			free(text);
			return TRUE;
		}
	}
	case WM_DESTROY:
		UnregisterClassW(L"ProcessSelectorDialog", GetModuleHandleW(NULL));
		if (selectedProcess != SIZE_MAX) {
			UIProcessInfo* process = ((UIProcessInfo*)UIProcessesList.memory) + selectedProcess;
			processDialogCallback(process->pid, process->process_name, true);
		} else {
			processDialogCallback(0, NULL, false);
		}
		for (size_t i = 0; i < UIProcessesList.elementCount; i++) {
			free(((UIProcessInfo*)UIProcessesList.memory + i)->process_name);
		}
		ArrayList_free(&UIProcessesList);

		break;
	case WM_COMMAND:
		if (HIWORD(wparam) == EN_CHANGE && LOWORD(wparam) == 2) {
			wchar_t* text = get_control_text(processSearchTextboxHandle);
			fill_ui_processes(text);
			free(text);
			set_scrollbar_for_window(hwnd);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		break;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void ProcessDialog_init(void(*callback)(DWORD pid, const wchar_t* name, bool success)) {
	BackgroundColorBrush = CreateSolidBrush(BackgroundColorRef);
	AccentColorBrush = CreateSolidBrush(AccentColorRef);
	AccentColorHighlightBrush = CreateSolidBrush(AccentColorHighlightRef);
	WhiteColorBrush = (HBRUSH)WHITE_BRUSH;
	selectedProcess = SIZE_MAX;
	selectedProcessBtn = SIZE_MAX;
	processDialogCallback = callback;

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = BackgroundColorBrush;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = ProcessDialog_WindowProc;
	wc.lpszClassName = L"ProcessSelectorDialog";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassEx(&wc);

	int windowSizeX = 300;
	int windowSizeY = 700;
	RECT windowPosAndSize = get_centered_window_position(windowSizeX, windowSizeY);

	HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Select a Process... please?", WS_OVERLAPPEDWINDOW,
		windowPosAndSize.left, windowPosAndSize.top, windowPosAndSize.right, windowPosAndSize.bottom, NULL, NULL, wc.hInstance,
		NULL);
	ShowWindow(hwnd, SW_SHOW);

	HWND searchTextbox = CreateWindowEx(0, MSFTEDIT_CLASS, L"",
		WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL,
		8, 8, 284, 24, hwnd, (HMENU)2, GetModuleHandle(NULL), hwnd);
	make_font_hot_or_well_at_least_way_hotter(searchTextbox);
	SendMessage(searchTextbox, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search process...");
	SendMessage(searchTextbox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(35, 35, 35));
	SendMessage(searchTextbox, EM_SETEVENTMASK, 0, ENM_CHANGE);

	CHARFORMAT cf = get_char_fmt(searchTextbox, SCF_DEFAULT);
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = RGB(174, 118, 214);
	SendMessage(searchTextbox, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SetWindowLong(searchTextbox, GWL_STYLE, GetWindowLong(searchTextbox, GWL_STYLE) & ~WS_BORDER);
	processSearchTextboxHandle = searchTextbox;

	fill_ui_processes(L"");
	set_scrollbar_for_window(hwnd);
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	si.nPos = 0;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}