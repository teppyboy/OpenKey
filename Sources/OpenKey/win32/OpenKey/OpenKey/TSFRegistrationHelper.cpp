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
#include <msctf.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

namespace {
	// Keep in sync with CLSID_OpenKeyTIP in OpenKeyTIP/globals.h
	const wchar_t* TIP_CLSID = L"{A942CCFA-976D-4D60-93AD-5CEBE269751F}";
	const CLSID TIP_CLSID_VALUE =
	{ 0xa942ccfa, 0x976d, 0x4d60, { 0x93, 0xad, 0x5c, 0xeb, 0xe2, 0x69, 0x75, 0x1f } };
	const GUID TIP_PROFILE_GUID =
	{ 0x8bcb2f64, 0x9491, 0x4b57, { 0x86, 0x6a, 0xe2, 0xf3, 0x9d, 0xbb, 0x06, 0x68 } };
	const LANGID TIP_LANGID = 0x042A;
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
		DEBUG_LOG(L"TSF callRegistrationExport export=%S path=%s", exportName, dllPath.c_str());
		if (!isAbsolutePath(dllPath) || !fileExists(dllPath))
		{
			DEBUG_LOG(L"TSF registration export aborted: invalid dll path");
			return false;
		}

		HMODULE dll = LoadLibraryW(dllPath.c_str());
		if (!dll) {
			DEBUG_LOG(L"TSF LoadLibrary failed error=%lu", GetLastError());
			return false;
		}

		DllRegistrationProc proc = (DllRegistrationProc)GetProcAddress(dll, exportName);
		HRESULT hr = proc ? proc() : E_FAIL;
		DEBUG_LOG(L"TSF registration export result export=%S proc=%d hr=0x%08X", exportName, proc ? 1 : 0, (unsigned int)hr);
		FreeLibrary(dll);
		return SUCCEEDED(hr);
	}
}

namespace TSFRegistrationHelper {
	std::wstring getTIPDllPath() {
		std::wstring dir = getExecutableDirectory();
		if (dir.empty()) {
			DEBUG_LOG(L"TSF getTIPDllPath: executable directory unavailable");
			return L"";
		}

#ifdef _WIN64
		std::wstring platformDll = dir + L"OpenKeyTIP64.dll";
#else
		std::wstring platformDll = dir + L"OpenKeyTIP32.dll";
#endif
		if (fileExists(platformDll))
		{
			DEBUG_LOG(L"TSF getTIPDllPath: platform dll=%s", platformDll.c_str());
			return platformDll;
		}

		std::wstring genericDll = dir + L"OpenKeyTIP.dll";
		if (fileExists(genericDll)) {
			DEBUG_LOG(L"TSF getTIPDllPath: generic dll=%s", genericDll.c_str());
			return genericDll;
		}
		DEBUG_LOG(L"TSF getTIPDllPath: no bundled TIP dll beside OpenKey");
		return L"";
	}

	bool isTIPRegistered() {
		std::wstring keyPath = L"Software\\Classes\\CLSID\\";
		keyPath += TIP_CLSID;
		keyPath += L"\\InProcServer32";

		HKEY key = NULL;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
			DEBUG_LOG(L"TSF isTIPRegistered: registry key missing");
			return false;
		}

		wchar_t value[32768] = {};
		DWORD type = REG_SZ;
		DWORD size = sizeof(value);
		LSTATUS status = RegQueryValueExW(key, NULL, NULL, &type, (LPBYTE)value, &size);
		RegCloseKey(key);

		if (status != ERROR_SUCCESS || type != REG_SZ || size == 0 || (size % sizeof(wchar_t)) != 0) {
			DEBUG_LOG(L"TSF isTIPRegistered: invalid registry value status=%ld type=%lu size=%lu", status, type, size);
			return false;
		}

		DWORD charCount = size / sizeof(wchar_t);
		if (charCount == 0 || charCount > ARRAYSIZE(value)) {
			DEBUG_LOG(L"TSF isTIPRegistered: invalid char count=%lu", charCount);
			return false;
		}

		bool hasTerminator = value[charCount - 1] == L'\0';
		DWORD contentCount = hasTerminator ? charCount - 1 : charCount;
		if (contentCount == 0 || contentCount >= ARRAYSIZE(value)) {
			DEBUG_LOG(L"TSF isTIPRegistered: invalid content count=%lu", contentCount);
			return false;
		}
		for (DWORD i = 0; i < contentCount; ++i) {
			if (value[i] == L'\0') {
				DEBUG_LOG(L"TSF isTIPRegistered: embedded NUL in registry path");
				return false;
			}
		}
		value[contentCount] = L'\0';

		std::wstring expectedPath = getTIPDllPath();
		bool registered = !expectedPath.empty() && fileExists(value) && samePath(value, expectedPath);
		DEBUG_LOG(L"TSF isTIPRegistered: registered=%d registryPath=%s expectedPath=%s", registered ? 1 : 0, value, expectedPath.c_str());
		return registered;
	}

	bool registerTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		DEBUG_LOG(L"TSF registerTIP elevated=%d", elevated ? 1 : 0);
		return callRegistrationExport("DllRegisterServer");
	}

	bool unregisterTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		DEBUG_LOG(L"TSF unregisterTIP elevated=%d", elevated ? 1 : 0);
		return callRegistrationExport("DllUnregisterServer");
	}

	bool activateTIP() {
		DEBUG_LOG(L"TSF activateTIP begin");
		HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		bool shouldUninitialize = SUCCEEDED(hrCo);
		if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE) {
			DEBUG_LOG(L"TSF activateTIP CoInitializeEx failed hr=0x%08X", (unsigned int)hrCo);
			return false;
		}

		ITfInputProcessorProfiles* profiles = NULL;
		HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
			IID_ITfInputProcessorProfiles, (void**)&profiles);
		if (SUCCEEDED(hr) && profiles) {
			HRESULT hrEnable = profiles->EnableLanguageProfile(TIP_CLSID_VALUE, TIP_LANGID, TIP_PROFILE_GUID, TRUE);
			HRESULT hrDefault = profiles->EnableLanguageProfileByDefault(TIP_CLSID_VALUE, TIP_LANGID, TIP_PROFILE_GUID, TRUE);
			DEBUG_LOG(L"TSF activateTIP enable hr=0x%08X default=0x%08X", (unsigned int)hrEnable, (unsigned int)hrDefault);

			ITfInputProcessorProfileMgr* profileMgr = NULL;
			HRESULT hrMgr = profiles->QueryInterface(IID_ITfInputProcessorProfileMgr, (void**)&profileMgr);
			if (SUCCEEDED(hrMgr) && profileMgr) {
			hr = profileMgr->ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
				TIP_LANGID,
				TIP_CLSID_VALUE,
				TIP_PROFILE_GUID,
				NULL,
				TF_IPPMF_FORSESSION | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE | TF_IPPMF_ENABLEPROFILE);
				DEBUG_LOG(L"TSF activateTIP ActivateProfile hr=0x%08X", (unsigned int)hr);
				profileMgr->Release();
			}
			else {
				DEBUG_LOG(L"TSF activateTIP QueryInterface ITfInputProcessorProfileMgr failed hr=0x%08X", (unsigned int)hrMgr);
				hr = profiles->ActivateLanguageProfile(TIP_CLSID_VALUE, TIP_LANGID, TIP_PROFILE_GUID);
				DEBUG_LOG(L"TSF activateTIP ActivateLanguageProfile fallback hr=0x%08X", (unsigned int)hr);
			}
			profiles->Release();
		}
		DEBUG_LOG(L"TSF activateTIP result hr=0x%08X", (unsigned int)hr);

		if (shouldUninitialize)
			CoUninitialize();
		return SUCCEEDED(hr);
	}
}
