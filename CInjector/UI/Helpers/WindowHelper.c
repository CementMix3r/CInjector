#include "WindowHelper.h"

RECT get_centered_window_position(int windowWidth, int windowHeight) {
	POINT mousePosition;
	GetCursorPos(&mousePosition);
	HMONITOR activeMonitor = MonitorFromPoint(mousePosition, MONITOR_DEFAULTTONEAREST);
	MONITORINFO activeMonitorInfo;
	memset(&activeMonitorInfo, 0, sizeof(activeMonitorInfo));
	activeMonitorInfo.cbSize = sizeof(activeMonitorInfo);
	GetMonitorInfoW(activeMonitor, &activeMonitorInfo);
	RECT windowRect;

	int monitorWidth = activeMonitorInfo.rcMonitor.right - activeMonitorInfo.rcMonitor.left;
	int monitorHeight = activeMonitorInfo.rcMonitor.bottom - activeMonitorInfo.rcMonitor.top;
	windowRect.left = monitorWidth / 2 - windowWidth / 2 + activeMonitorInfo.rcMonitor.left;
	windowRect.right = windowRect.left + windowWidth;
	windowRect.top = monitorHeight / 2 - windowHeight / 2 + activeMonitorInfo.rcMonitor.top;
	windowRect.bottom = windowRect.top + windowHeight;
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int adjustedWindowSizeX = windowRect.right - windowRect.left;
	int adjustedWindowSizeY = windowRect.bottom - windowRect.top;
	windowRect.left = monitorWidth / 2 - adjustedWindowSizeX / 2 + activeMonitorInfo.rcMonitor.left;
	windowRect.right = adjustedWindowSizeX;
	windowRect.top = monitorHeight / 2 - adjustedWindowSizeY / 2 + activeMonitorInfo.rcMonitor.top;
	windowRect.bottom = adjustedWindowSizeY;
	return windowRect;
}

wchar_t* get_control_text(HWND hwnd) {
	int len = GetWindowTextLengthW(hwnd);
	wchar_t* text = malloc((len + 2) * sizeof(wchar_t));
	GetWindowTextW(hwnd, text, len + 1);
	return text;
}

void make_font_hot_or_well_at_least_way_hotter(HWND hwnd) {
	LOGFONT lf = { 0 };
	lf.lfHeight = 20;
	lf.lfQuality = CLEARTYPE_QUALITY; // Enable ClearType anti-aliasing
	lf.lfWeight = FW_NORMAL;
	wcscpy_s(lf.lfFaceName, 32, L"Segoe UI");

	HFONT hFont = CreateFontIndirect(&lf);
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}



CHARFORMAT get_char_fmt(HWND hwnd, DWORD range) {
	CHARFORMAT cf;
	SendMessage(hwnd, EM_GETCHARFORMAT, range, (LPARAM)&cf);
	return cf;
}