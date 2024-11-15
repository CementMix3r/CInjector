#pragma once
#include <Windows.h>

DWORD inject_dll(DWORD pid, const wchar_t* dllPath);