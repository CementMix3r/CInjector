#include <windows.h>
#include "textconv.h"

wchar_t* textconv_UTF8_to_wchar(const char* input) {
	const int len = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
	wchar_t* result = malloc(len + 10);
	MultiByteToWideChar(CP_UTF8, 0, input, -1, result, len);
	result[len] = '\0';
	return result;
}

char* textconv_wchar_to_UTF8(const wchar_t* input) {
	const int len = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
	char* result = malloc(len + 10);
	WideCharToMultiByte(CP_UTF8, 0, input, -1, result, len, NULL, NULL);
	result[len] = L'\0';
	return result;
}