/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "stdafx.h"

#ifdef _DEBUG
#include <stdarg.h>

static HANDLE debugConsole = NULL;
static CRITICAL_SECTION debugLogLock;
static bool debugLogReady = false;
static bool debugLogLockReady = false;

void DebugLogInit() {
	if (debugLogReady)
		return;

	InitializeCriticalSection(&debugLogLock);
	debugLogLockReady = true;
	if (AllocConsole()) {
		SetConsoleTitleW(L"OpenKey Debug Log");
		debugConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	} else {
		debugConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	debugLogReady = true;
	DebugLog(L"debug console initialized");
}

void DebugLogShutdown() {
	DebugLog(L"debug console shutting down");
	if (debugLogLockReady)
		EnterCriticalSection(&debugLogLock);
	if (debugConsole)
		FlushFileBuffers(debugConsole);
	if (debugLogReady)
		FreeConsole();
	debugConsole = NULL;
	debugLogReady = false;
	if (debugLogLockReady)
		LeaveCriticalSection(&debugLogLock);
	if (debugLogLockReady) {
		DeleteCriticalSection(&debugLogLock);
		debugLogLockReady = false;
	}
}

void DebugLog(const wchar_t* format, ...) {
	WCHAR message[2048];
	va_list args;
	va_start(args, format);
	_vsnwprintf_s(message, _TRUNCATE, format, args);
	va_end(args);

	SYSTEMTIME now;
	GetLocalTime(&now);
	WCHAR line[2300];
	_snwprintf_s(line, _TRUNCATE,
		L"[%02u:%02u:%02u.%03u][tid:%lu] %s\r\n",
		now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
		GetCurrentThreadId(), message);

	if (debugLogLockReady)
		EnterCriticalSection(&debugLogLock);
	OutputDebugStringW(line);
	if (debugConsole && debugConsole != INVALID_HANDLE_VALUE) {
		DWORD written = 0;
		WriteConsoleW(debugConsole, line, (DWORD)lstrlenW(line), &written, NULL);
	}
	if (debugLogLockReady)
		LeaveCriticalSection(&debugLogLock);
}
#endif
