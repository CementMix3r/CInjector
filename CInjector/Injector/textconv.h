#pragma once
#include <windows.h>

// you are responsible for freeing the result
wchar_t* textconv_UTF8_to_wchar(const char* input);
// you are responsible for freeing the result
char* textconv_wchar_to_UTF8(const wchar_t* input);