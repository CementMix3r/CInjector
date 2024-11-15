#include "injector.h"
#include "processes.h"

#include <stdio.h>
#include <stdbool.h>
#include <psapi.h>

static intptr_t get_load_library_address_offset() {
	HMODULE k32 = GetModuleHandleW(L"Kernel32.dll");
	intptr_t loadLibAddr = (intptr_t)GetProcAddress(k32, "LoadLibraryW");
	MODULEINFO info;
	if (GetModuleInformation(GetCurrentProcess(), k32, &info, sizeof(info)) == 0)
		return (intptr_t)LoadLibraryW;
	return loadLibAddr - (intptr_t)info.lpBaseOfDll;
}

static void* get_remote_load_library_address(HANDLE hProc) {
	intptr_t offset = get_load_library_address_offset();
	if (offset == (intptr_t)LoadLibraryW) {
		return (void*)offset;
	}
	ArrayList remoteModules = get_process_modules(hProc);
	const wchar_t* targetModuleName = L"kernel32.dll";
	for (size_t i = 0; i < remoteModules.elementCount; i++) {
		HMODULE hMod = ((HMODULE*)remoteModules.memory)[i];
		wchar_t* name = get_process_module_name(hProc, hMod);
		if (!name) continue;
		bool identical = true;
		int textI = 0;
		for (; name[textI] != L'\0' && targetModuleName[textI] != L'\0'; textI++) {
			if (tolower(name[textI]) != tolower(targetModuleName[textI])) {
				identical = false;
				break;
			}
		}
		identical = identical && name[textI] == L'\0' && targetModuleName[textI] == L'\0';
		free(name);

		if (identical) {
			MODULEINFO info;
			if (K32GetModuleInformation(hProc, hMod, &info, sizeof(info)) == 0) {
				break;
			}
			ArrayList_free(&remoteModules);
			return (void*)((intptr_t)info.lpBaseOfDll + offset);
		}
	}
	ArrayList_free(&remoteModules);
	return LoadLibraryW;
}

DWORD inject_dll(DWORD pid, const wchar_t* dllPath) {
	printf("Injecting into process %d\n", (DWORD)pid);
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (!hProc) {
		printf("Failed to open process: %d\n", (DWORD)GetLastError());
		return 1;
	}
	size_t dllPathSize = sizeof(wchar_t) * (wcslen(dllPath) + 1);
	
	void* remotePathMemory = VirtualAllocEx(hProc, NULL, dllPathSize + 60, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remotePathMemory) {
		printf("Failed to allocate memory in the remote process: %d\n", (DWORD)GetLastError());
		CloseHandle(hProc);
		return 2;
	}

	BOOL writePath = WriteProcessMemory(hProc, remotePathMemory, dllPath, dllPathSize, NULL);
	if (!writePath) {
		printf("Failed to write the path to the remote address: %d\n", (DWORD)GetLastError());
		VirtualFreeEx(hProc, remotePathMemory, 0, MEM_RELEASE);
		return 3;
	}

	LPTHREAD_START_ROUTINE remoteLoadLibrary = (LPTHREAD_START_ROUTINE)get_remote_load_library_address(hProc);
	HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, remoteLoadLibrary, remotePathMemory, 0, NULL);
	if (!hThread) {
		printf("Failed to create remote thread: %d\n", (DWORD)GetLastError());
		VirtualFreeEx(hProc, remotePathMemory, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return 4;
	}
	// wait for the remote thread to finish.
	WaitForSingleObject(hThread, 2000);
	VirtualFreeEx(hProc, remotePathMemory, 0, MEM_RELEASE);
	CloseHandle(hThread);
	return 0;
}