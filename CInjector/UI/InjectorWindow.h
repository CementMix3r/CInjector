#pragma once
#include <Windows.h>


typedef struct {
	HANDLE hwnd;
	const wchar_t* class_name;
	const wchar_t* window_title;
} InjectorWindow;

void InjectorWindow_run(InjectorWindow* injectorWindow);