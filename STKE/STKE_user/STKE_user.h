#pragma once
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <Psapi.h>

#include "list_info.h"
#include "user_input.h"
#include "loading.h"

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;