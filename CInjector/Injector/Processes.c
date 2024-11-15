#include "textconv.h"
#include "Processes.h"
#include "Arraylist.h"
#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <stdbool.h>

ArrayList get_process_modules(HANDLE hProc) {
    ArrayList modules = ArrayList_new(300, sizeof(HMODULE));
    DWORD neededSize = 0;

    while (true) {
	    if (!K32EnumProcessModules(hProc, (HMODULE*)modules.memory, (DWORD)(modules.capacity * sizeof(DWORD)), &neededSize)) {
            printf("Failed to obtain the list of modules.\n");
            ArrayList_free(&modules);
            return modules;
	    }
        if (neededSize / sizeof(HMODULE) < modules.capacity) {
            modules.elementCount = neededSize / sizeof(HMODULE);
            return modules;
        }
        ArrayList_resize(&modules, modules.capacity * 2);
    }
}

wchar_t* get_process_module_name(HANDLE hProc, HMODULE hMod) {
    wchar_t* result = (wchar_t*)malloc(1024 * sizeof(wchar_t));
    DWORD currentSize = 1024;
	while (true) {
        DWORD written = K32GetModuleFileNameExW(hProc, hMod, result, currentSize);
        if (written >= currentSize) {
            currentSize *= 2;
            free(result);
            result = malloc(currentSize * sizeof(wchar_t));
        } else  {
            result[written] = L'\0';
            break;
        }
    }
    return result;
}

ArrayList get_pids() {
    ArrayList processes = ArrayList_new(300, sizeof(DWORD));
    DWORD neededSize = 0;

    while (true) {
        if (!K32EnumProcesses((DWORD*)processes.memory, (DWORD)(processes.capacity * sizeof(DWORD)), &neededSize)) {
            printf("Failed to obtain the list of processes.\n");
            ArrayList_free(&processes);
            return processes;
        }
        if (neededSize/4 < processes.capacity) {
            processes.elementCount = neededSize / 4;
            return processes;
        }
        ArrayList_resize(&processes, processes.capacity * 2);
    }
}

wchar_t* get_process_name(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc) {
        return NULL;
    }
    HMODULE mainModule;
    DWORD fat;
    if (K32EnumProcessModules(hProc, &mainModule, sizeof(HMODULE), &fat) == 0) {
        CloseHandle(hProc);
        return NULL;
    }

    wchar_t* text = malloc(1024 * sizeof(wchar_t));
    DWORD textLen = K32GetModuleBaseNameW(hProc, mainModule, text, 1024);
    text[textLen] = L'\0';
    CloseHandle(hProc);

    if (textLen == 0) {
        free(text);
        return NULL;
    }
    return text;
}

ArrayList find_pids_for_name(const wchar_t* procName) {
    ArrayList result = ArrayList_new(0, sizeof(DWORD));
    ArrayList pids = get_pids();

    for (size_t i = 0; i < pids.elementCount; i++) {
        // Extract information for i
        DWORD pid = ((DWORD*)pids.memory)[i];
        wchar_t* name = get_process_name(pid);
        if (name == NULL) {
            continue;
        }

        // Check if the lowercase process name matches what we are looking for
        bool identical = true;
        int textI = 0;
        for (; name[textI] != L'\0' && procName[textI] != L'\0'; textI++) {
            if (tolower(name[textI]) != tolower(procName[textI])) {
                identical = false;
                break;
            }
        }
        identical = identical && name[textI] == L'\0' && procName[textI] == L'\0';

        // add to list if identical
        if (identical) {
            ArrayList_add(&result, &pid);
        }

        // clean up anything we allocated so that we dont leak that memory
        free(name);
    }

    // and clean up the list of pids we got
    ArrayList_free(&pids);

    return result;
}

ArrayList find_pids_for_name_ascii(const char* procName) {
    wchar_t* wstr = textconv_UTF8_to_wchar(procName);
    ArrayList result = find_pids_for_name(wstr);
    free(wstr);
    return result;
}

void print_pids() {
    ArrayList pidsArray = get_pids();
    DWORD* pids = (DWORD*)pidsArray.memory;

    for (size_t i = 0; i < pidsArray.elementCount; i++) {
        wchar_t* name = get_process_name(pids[i]);
        if (name == NULL) {
            continue;
        }
        printf("PID: %d Name: %ls\n", pids[i], name);
        free(name);
    }
}
