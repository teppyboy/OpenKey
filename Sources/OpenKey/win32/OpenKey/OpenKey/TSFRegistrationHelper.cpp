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
#include "TSFRegistrationHelper.h"

namespace {
	// Keep in sync with CLSID_OpenKeyTIP in OpenKeyTIP/globals.h
	const wchar_t* TIP_CLSID = L"{A942CCFA-976D-4D60-93AD-5CEBE269751F}";
	typedef HRESULT(STDAPICALLTYPE* DllRegistrationProc)();

	bool fileExists(const std::wstring& path) {
		DWORD attrs = GetFileAttributesW(path.c_str());
		return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool isAbsolutePath(const std::wstring& path) {
		return (path.size() > 2 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/')) ||
			(path.size() > 1 && path[0] == L'\\' && path[1] == L'\\');
	}

	std::wstring getFullPath(const std::wstring& path) {
		if (path.empty())
			return L"";

		DWORD len = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
		if (len == 0)
			return L"";

		std::wstring fullPath(len, L'\0');
		DWORD written = GetFullPathNameW(path.c_str(), len, &fullPath[0], NULL);
		if (written == 0 || written >= len)
			return L"";
		fullPath.resize(written);
		return fullPath;
	}

	bool samePath(const std::wstring& lhs, const std::wstring& rhs) {
		std::wstring left = getFullPath(lhs);
		std::wstring right = getFullPath(rhs);
		if (left.empty() || right.empty())
			return false;
		return _wcsicmp(left.c_str(), right.c_str()) == 0;
	}

	std::wstring getExecutableDirectory() {
		std::wstring path(32768, L'\0');
		DWORD len = GetModuleFileNameW(NULL, &path[0], (DWORD)path.size());
		if (len == 0 || len >= path.size())
			return L"";
		path.resize(len);

		size_t slash = path.find_last_of(L"\\/");
		if (slash == std::wstring::npos)
			return L"";
		return path.substr(0, slash + 1);
	}

	bool callRegistrationExport(const char* exportName) {
		std::wstring dllPath = TSFRegistrationHelper::getTIPDllPath();
		if (!isAbsolutePath(dllPath) || !fileExists(dllPath))
			return false;

		HMODULE dll = LoadLibraryW(dllPath.c_str());
		if (!dll)
			return false;

		DllRegistrationProc proc = (DllRegistrationProc)GetProcAddress(dll, exportName);
		HRESULT hr = proc ? proc() : E_FAIL;
		FreeLibrary(dll);
		return SUCCEEDED(hr);
	}
}

namespace TSFRegistrationHelper {
	std::wstring getTIPDllPath() {
		std::wstring dir = getExecutableDirectory();
		if (dir.empty())
			return L"";

#ifdef _WIN64
		std::wstring platformDll = dir + L"OpenKeyTIP64.dll";
#else
		std::wstring platformDll = dir + L"OpenKeyTIP32.dll";
#endif
		if (fileExists(platformDll))
			return platformDll;

		std::wstring genericDll = dir + L"OpenKeyTIP.dll";
		return fileExists(genericDll) ? genericDll : L"";
	}

	bool isTIPRegistered() {
		std::wstring keyPath = L"Software\\Classes\\CLSID\\";
		keyPath += TIP_CLSID;
		keyPath += L"\\InProcServer32";

		HKEY key = NULL;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
			return false;

		wchar_t value[32768] = {};
		DWORD type = REG_SZ;
		DWORD size = sizeof(value);
		LSTATUS status = RegQueryValueExW(key, NULL, NULL, &type, (LPBYTE)value, &size);
		RegCloseKey(key);

		if (status != ERROR_SUCCESS || type != REG_SZ || size == 0 || (size % sizeof(wchar_t)) != 0)
			return false;

		DWORD charCount = size / sizeof(wchar_t);
		if (charCount == 0 || charCount > ARRAYSIZE(value))
			return false;

		bool hasTerminator = value[charCount - 1] == L'\0';
		DWORD contentCount = hasTerminator ? charCount - 1 : charCount;
		if (contentCount == 0 || contentCount >= ARRAYSIZE(value))
			return false;
		for (DWORD i = 0; i < contentCount; ++i) {
			if (value[i] == L'\0')
				return false;
		}
		value[contentCount] = L'\0';

		std::wstring expectedPath = getTIPDllPath();
		return !expectedPath.empty() && fileExists(value) && samePath(value, expectedPath);
	}

	bool registerTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		return callRegistrationExport("DllRegisterServer");
	}

	bool unregisterTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		return callRegistrationExport("DllUnregisterServer");
	}
}
