#pragma once
#include <Windows.h>
#include <Richedit.h>

RECT get_centered_window_position(int windowWidth, int windowHeight);
wchar_t* get_control_text(HWND hwnd);
void make_font_hot_or_well_at_least_way_hotter(HWND hwnd);


CHARFORMAT get_char_fmt(HWND hwnd, DWORD range);