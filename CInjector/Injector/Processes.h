#pragma once
#include "arraylist.h"

#include <Windows.h>



void print_pids();
ArrayList get_pids();
// you are responsible for freeing the returned memory
wchar_t* get_process_name(DWORD pid);

ArrayList find_pids_for_name(const wchar_t* procName);
ArrayList find_pids_for_name_ascii(const char* procName);

// The handle needs at least PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
ArrayList get_process_modules(HANDLE hProc);

// The handle needs at least PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
// you are responsible for freeing the returned memory
wchar_t* get_process_module_name(HANDLE hProc, HMODULE hMod);