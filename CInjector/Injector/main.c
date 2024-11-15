#include <wchar.h>
#include <stdio.h>
#include "arraylist.h"
#include "Processes.h"
#include "injector.h"
#include "../UI/InjectorWindow.h"
#include <Commctrl.h>

// makes the buttons look like windows vista and later instead of win95
#pragma comment(linker, \
    "\"/manifestdependency:type='win32' " \
    "name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' " \
    "processorArchitecture='*' " \
    "publicKeyToken='6595b64144ccf1df' " \
    "language='*'\"")

#pragma comment(lib, "Comctl32.lib")

void print_process_name(DWORD pid, size_t* id) {
	wchar_t* procName = get_process_name(pid);
	if (id) {
		wprintf(L"[%lu] %i ", *id+1, pid);
	} else {
		wprintf(L"%i - ", pid);
	}

	if (procName != NULL) {
		wprintf(L"%s", procName);
		free(procName);
	}
	printf("\n");
}

int main() {
	InitCommonControls();
	LoadLibrary(TEXT("Msftedit.dll"));
	/*print_pids();
	wchar_t* input = (wchar_t*)malloc(1024 * sizeof(wchar_t));
	wprintf(L"Enter a process name to search for: ");
	if (wscanf_s(L"%1023ls", input, 1024) != 1) {
		wprintf(L"Failed to read input: %d\n", GetLastError());
		free(input);
		return 1;
	}

	ArrayList pids = find_pids_for_name(input);
	DWORD targetPid = 0;
	if (pids.elementCount != 0) {
		wprintf(L"Found %zu processes with the name %ls\n", pids.elementCount, input);

		if (pids.elementCount == 1) {
			targetPid = ((DWORD*)pids.memory)[0];
			print_process_name(targetPid, NULL);
		} else {
			printf("Multiple processes found, which one do you want to use?\n");
			for (size_t i = 0; i < pids.elementCount; i++) {
				print_process_name(((DWORD*)pids.memory)[i], &i);
			}

			int pidChoice = 0;
			wprintf(L"Which process would you like to inject to (1 - %zu):\n", pids.elementCount);
			if (wscanf_s(L"%d", &pidChoice) != 1 || pidChoice < 1 || pidChoice >(int)pids.elementCount) {
				wprintf(L"Invalid process, defaulting to the first process.\n");
				targetPid = ((DWORD*)pids.memory)[0];
			} else {
				targetPid = ((DWORD*)pids.memory)[pidChoice - 1];
			}
		}
	} else {
		wprintf(L"No processes found with the name %ls\n", input);
	}

	ArrayList_free(&pids);
	free(input);

	if (targetPid != 0) {
		inject_dll(targetPid, L"C:\\Users\\cemen\\AppData\\Local\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe\\RoamingState\\OnixClient.dll");
		printf("Injected into process %d\n", (DWORD)targetPid);
	} else {
		wprintf(L"Cannot inject into an invalid process.\n");
	}*/
	InjectorWindow window;
	window.hwnd = 0;
	window.class_name = L"InjectorWindow";
	window.window_title = L"Injector";
	InjectorWindow_run(&window);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main();
}