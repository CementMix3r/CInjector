#pragma once
#include <Windows.h>
#include <stdbool.h>

typedef struct {
	DWORD pid;
	const wchar_t* process_name;
} UIProcessInfo;

void ProcessDialog_init(void(*callback)(DWORD pid, const wchar_t* name, bool success));