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
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "psapi.lib")

namespace {
	// Keep in sync with CLSID_OpenKeyTIP in OpenKeyTIP/globals.h
	const wchar_t* TIP_CLSID = L"{A942CCFA-976D-4D60-93AD-5CEBE269751F}";
	const CLSID TIP_CLSID_VALUE =
	{ 0xa942ccfa, 0x976d, 0x4d60, { 0x93, 0xad, 0x5c, 0xeb, 0xe2, 0x69, 0x75, 0x1f } };
	const GUID TIP_PROFILE_GUID =
	{ 0x8bcb2f64, 0x9491, 0x4b57, { 0x86, 0x6a, 0xe2, 0xf3, 0x9d, 0xbb, 0x06, 0x68 } };
	const LANGID TIP_LANGID = 0x042A;
	const wchar_t* TIP_PROFILE_GUID_STR = L"{8BCB2F64-9491-4B57-866A-E2F39DBB0668}";
	const wchar_t* CTF_LOCALE_ASSEMBLY_GUID = L"{34745C63-B2F0-4784-8B67-5E12C8701A31}";
	const wchar_t* OPENKEY_TIP = L"042a:{A942CCFA-976D-4D60-93AD-5CEBE269751F}{8BCB2F64-9491-4B57-866A-E2F39DBB0668}";
	const wchar_t* WINDOWS_VIETNAMESE_IME_TIP = L"042a:0000042a";
	const DWORD ILOT_UNINSTALL = 0x00000001;
	typedef HRESULT(STDAPICALLTYPE* DllRegistrationProc)();
	typedef BOOL(WINAPI* InstallLayoutOrTipProc)(LPCWSTR psz, DWORD dwFlags);

	bool sameInputMethodTip(const std::wstring& lhs, const wchar_t* rhs) {
		return _wcsicmp(lhs.c_str(), rhs) == 0;
	}

	bool hasInputMethodTip(const std::vector<std::wstring>& tips, const std::wstring& tip) {
		for (size_t i = 0; i < tips.size(); ++i) {
			if (_wcsicmp(tips[i].c_str(), tip.c_str()) == 0)
				return true;
		}
		return false;
	}

	bool isVietnameseInputMethodTip(const std::wstring& tip) {
		return tip.size() > 5 && _wcsnicmp(tip.c_str(), L"042a:", 5) == 0;
	}

	void addInputMethodTip(std::vector<std::wstring>& tips, const std::wstring& tip) {
		if (!tip.empty() && !hasInputMethodTip(tips, tip))
			tips.push_back(tip);
	}

	std::vector<std::wstring> readVietnameseInputMethodTips() {
		std::vector<std::wstring> tips;
		HKEY key = NULL;
		LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER,
			L"Control Panel\\International\\User Profile\\vi",
			0,
			KEY_READ,
			&key);
		if (status != ERROR_SUCCESS) {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME open vi profile failed status=%lu", status);
			return tips;
		}

		for (DWORD index = 0;; ++index) {
			wchar_t name[256] = {};
			DWORD nameSize = ARRAYSIZE(name);
			DWORD type = 0;
			status = RegEnumValueW(key, index, name, &nameSize, NULL, &type, NULL, NULL);
			if (status == ERROR_NO_MORE_ITEMS)
				break;
			if (status != ERROR_SUCCESS) {
				DEBUG_LOG(L"TSF removeDefaultVietnameseIME enum vi profile failed index=%lu status=%lu", index, status);
				continue;
			}

			std::wstring tip(name, nameSize);
			if (isVietnameseInputMethodTip(tip))
				addInputMethodTip(tips, tip);
		}

		RegCloseKey(key);
		return tips;
	}

	std::vector<std::wstring> readRegisteredVietnameseTIPs() {
		std::vector<std::wstring> tips;
		HKEY tipRoot = NULL;
		LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\CTF\\TIP",
			0,
			KEY_READ,
			&tipRoot);
		if (status != ERROR_SUCCESS) {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME open machine TIP root failed status=%lu", status);
			return tips;
		}

		for (DWORD tipIndex = 0;; ++tipIndex) {
			wchar_t tipClsid[64] = {};
			DWORD tipClsidSize = ARRAYSIZE(tipClsid);
			status = RegEnumKeyExW(tipRoot, tipIndex, tipClsid, &tipClsidSize, NULL, NULL, NULL, NULL);
			if (status == ERROR_NO_MORE_ITEMS)
				break;
			if (status != ERROR_SUCCESS)
				continue;

			std::wstring profilePath = std::wstring(tipClsid, tipClsidSize) + L"\\LanguageProfile\\0x0000042a";
			HKEY profileRoot = NULL;
			status = RegOpenKeyExW(tipRoot, profilePath.c_str(), 0, KEY_READ, &profileRoot);
			if (status != ERROR_SUCCESS)
				continue;

			for (DWORD profileIndex = 0;; ++profileIndex) {
				wchar_t profileGuid[64] = {};
				DWORD profileGuidSize = ARRAYSIZE(profileGuid);
				status = RegEnumKeyExW(profileRoot, profileIndex, profileGuid, &profileGuidSize, NULL, NULL, NULL, NULL);
				if (status == ERROR_NO_MORE_ITEMS)
					break;
				if (status != ERROR_SUCCESS)
					continue;

				std::wstring tip = L"042a:";
				tip.append(tipClsid, tipClsidSize);
				tip.append(profileGuid, profileGuidSize);
				addInputMethodTip(tips, tip);
			}

			RegCloseKey(profileRoot);
		}

		RegCloseKey(tipRoot);
		return tips;
	}

	bool parseTIPString(const std::wstring& tip, CLSID& clsid, GUID& profileGuid) {
		size_t clsidStart = tip.find(L'{');
		if (clsidStart == std::wstring::npos)
			return false;
		size_t clsidEnd = tip.find(L'}', clsidStart);
		if (clsidEnd == std::wstring::npos)
			return false;
		size_t profileStart = tip.find(L'{', clsidEnd + 1);
		if (profileStart == std::wstring::npos)
			return false;
		size_t profileEnd = tip.find(L'}', profileStart);
		if (profileEnd == std::wstring::npos)
			return false;

		std::wstring clsidString = tip.substr(clsidStart, clsidEnd - clsidStart + 1);
		std::wstring profileString = tip.substr(profileStart, profileEnd - profileStart + 1);
		return SUCCEEDED(CLSIDFromString(clsidString.c_str(), &clsid)) &&
			SUCCEEDED(CLSIDFromString(profileString.c_str(), &profileGuid));
	}

	bool writeLanguageProfileEnableState(const std::wstring& tip, DWORD enabled) {
		size_t clsidStart = tip.find(L'{');
		if (clsidStart == std::wstring::npos) return false;
		size_t clsidEnd = tip.find(L'}', clsidStart);
		if (clsidEnd == std::wstring::npos) return false;
		size_t profileStart = tip.find(L'{', clsidEnd + 1);
		if (profileStart == std::wstring::npos) return false;
		size_t profileEnd = tip.find(L'}', profileStart);
		if (profileEnd == std::wstring::npos) return false;

		std::wstring clsidStr = tip.substr(clsidStart, clsidEnd - clsidStart + 1);
		std::wstring profileStr = tip.substr(profileStart, profileEnd - profileStart + 1);

		std::wstring regPath = L"Software\\Microsoft\\CTF\\TIP\\";
		regPath += clsidStr;
		regPath += L"\\LanguageProfile\\0x0000042a\\";
		regPath += profileStr;

		LSTATUS status = RegSetKeyValueW(HKEY_CURRENT_USER,
			regPath.c_str(), L"Enable", REG_DWORD, &enabled, sizeof(enabled));
		DEBUG_LOG(L"TSF writeLanguageProfileEnableState tip=%s enabled=%lu status=%lu", tip.c_str(), enabled, status);
		return status == ERROR_SUCCESS;
	}

	bool setVietnameseAssemblyToOpenKey() {
		std::wstring regPath = L"Software\\Microsoft\\CTF\\Assemblies\\0x0000042a\\";
		regPath += CTF_LOCALE_ASSEMBLY_GUID;

		LSTATUS s1 = RegSetKeyValueW(HKEY_CURRENT_USER, regPath.c_str(), L"Default", REG_SZ,
			TIP_CLSID, (DWORD)((wcslen(TIP_CLSID) + 1) * sizeof(wchar_t)));
		LSTATUS s2 = RegSetKeyValueW(HKEY_CURRENT_USER, regPath.c_str(), L"Profile", REG_SZ,
			TIP_PROFILE_GUID_STR, (DWORD)((wcslen(TIP_PROFILE_GUID_STR) + 1) * sizeof(wchar_t)));
		DWORD layout = 0;
		LSTATUS s3 = RegSetKeyValueW(HKEY_CURRENT_USER, regPath.c_str(), L"KeyboardLayout", REG_DWORD,
			&layout, sizeof(layout));

		DEBUG_LOG(L"TSF setVietnameseAssemblyToOpenKey status=%lu %lu %lu", s1, s2, s3);
		return s1 == ERROR_SUCCESS && s2 == ERROR_SUCCESS && s3 == ERROR_SUCCESS;
	}

	bool disableLanguageProfileForTip(const std::wstring& tip) {
		CLSID clsid = {};
		GUID profileGuid = {};
		if (!parseTIPString(tip, clsid, profileGuid))
			return false;

		HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		bool shouldUninitialize = SUCCEEDED(hrCo);
		if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE) {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME disable profile CoInitializeEx failed tip=%s hr=0x%08X", tip.c_str(), (unsigned int)hrCo);
			return false;
		}

		bool disabled = false;
		ITfInputProcessorProfiles* profiles = NULL;
		HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
			IID_ITfInputProcessorProfiles, (void**)&profiles);
		if (SUCCEEDED(hr) && profiles) {
			HRESULT hrEnable = profiles->EnableLanguageProfile(clsid, TIP_LANGID, profileGuid, FALSE);
			HRESULT hrDefault = profiles->EnableLanguageProfileByDefault(clsid, TIP_LANGID, profileGuid, FALSE);
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME disable profile tip=%s enable=0x%08X default=0x%08X",
				tip.c_str(), (unsigned int)hrEnable, (unsigned int)hrDefault);
			disabled = SUCCEEDED(hrEnable) || SUCCEEDED(hrDefault);
			profiles->Release();
		}
		else {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME disable profile CoCreateInstance failed tip=%s hr=0x%08X", tip.c_str(), (unsigned int)hr);
		}

		// COM EnableLanguageProfile does not always persist to registry for system TIPs.
		// Write the user-level enable=0 flag directly so Windows CTF reads it correctly.
		bool regWritten = writeLanguageProfileEnableState(tip, 0);
		disabled = disabled || regWritten;

		if (shouldUninitialize)
			CoUninitialize();
		return disabled;
	}

	bool deleteVietnameseInputMethodTipValue(const std::wstring& tip) {
		LSTATUS status = RegDeleteKeyValueW(HKEY_CURRENT_USER,
			L"Control Panel\\International\\User Profile\\vi",
			tip.c_str());
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME registry delete tip=%s status=%lu", tip.c_str(), status);
		return status == ERROR_SUCCESS;
	}

	bool setVietnameseInputMethodTipValue(const wchar_t* tip) {
		DWORD enabled = 1;
		LSTATUS status = RegSetKeyValueW(HKEY_CURRENT_USER,
			L"Control Panel\\International\\User Profile\\vi",
			tip,
			REG_DWORD,
			&enabled,
			sizeof(enabled));
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME registry set OpenKey tip=%s status=%lu", tip, status);
		return status == ERROR_SUCCESS;
	}

	void clearInputMethodOverrideIfMatches(const std::wstring& tip) {
		wchar_t value[256] = {};
		DWORD type = REG_SZ;
		DWORD size = sizeof(value);
		LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
			L"Control Panel\\International\\User Profile",
			L"InputMethodOverride",
			RRF_RT_REG_SZ,
			&type,
			value,
			&size);
		if (status == ERROR_SUCCESS && type == REG_SZ && _wcsicmp(value, tip.c_str()) == 0) {
			LSTATUS deleteStatus = RegDeleteKeyValueW(HKEY_CURRENT_USER,
				L"Control Panel\\International\\User Profile",
				L"InputMethodOverride");
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME clear override tip=%s status=%lu", tip.c_str(), deleteStatus);
		}
	}

	void broadcastInputProfileChange() {
		const wchar_t* areas[] = {
			L"intl",
			L"Control Panel\\International",
			L"Control Panel\\International\\User Profile",
			L"Keyboard Layout"
		};

		for (int i = 0; i < ARRAYSIZE(areas); ++i) {
			DWORD_PTR result = 0;
			LRESULT sent = SendMessageTimeoutW(HWND_BROADCAST,
				WM_SETTINGCHANGE,
				0,
				(LPARAM)areas[i],
				SMTO_ABORTIFHUNG,
				200,
				&result);
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME broadcast area=%s sent=%ld result=%llu error=%lu",
				areas[i], sent, (unsigned long long)result, sent ? 0 : GetLastError());
		}
	}
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

	std::wstring readRegisteredTIPDllPath() {
		std::wstring keyPath = L"Software\\Classes\\CLSID\\";
		keyPath += TIP_CLSID;
		keyPath += L"\\InProcServer32";

		wchar_t value[32768] = {};
		DWORD type = REG_SZ;
		DWORD size = sizeof(value);
		LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
			keyPath.c_str(),
			NULL,
			RRF_RT_REG_SZ,
			&type,
			value,
			&size);
		if (status != ERROR_SUCCESS || type != REG_SZ || size == 0 || (size % sizeof(wchar_t)) != 0)
			return L"";

		DWORD charCount = size / sizeof(wchar_t);
		if (charCount == 0 || charCount > ARRAYSIZE(value))
			return L"";

		bool hasTerminator = value[charCount - 1] == L'\0';
		DWORD contentCount = hasTerminator ? charCount - 1 : charCount;
		if (contentCount == 0 || contentCount >= ARRAYSIZE(value))
			return L"";

		for (DWORD i = 0; i < contentCount; ++i) {
			if (value[i] == L'\0')
				return L"";
		}
		value[contentCount] = L'\0';
		return value;
	}

	bool callRegistrationExport(const char* exportName, const std::wstring& dllPath) {
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
		std::wstring value = readRegisteredTIPDllPath();
		if (value.empty()) {
			DEBUG_LOG(L"TSF isTIPRegistered: registry key missing");
			return false;
		}

		std::wstring expectedPath = getTIPDllPath();
		bool registered = !expectedPath.empty() && fileExists(value) && samePath(value, expectedPath);
		DEBUG_LOG(L"TSF isTIPRegistered: registered=%d registryPath=%s expectedPath=%s", registered ? 1 : 0, value.c_str(), expectedPath.c_str());
		return registered;
	}

	bool registerTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		DEBUG_LOG(L"TSF registerTIP elevated=%d", elevated ? 1 : 0);
		return callRegistrationExport("DllRegisterServer", getTIPDllPath());
	}

	bool removeDefaultVietnameseIME() {
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME begin flags=0x%08X", ILOT_UNINSTALL);
		HMODULE inputDll = LoadLibraryW(L"input.dll");
		if (!inputDll) {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME LoadLibrary input.dll failed error=%lu", GetLastError());
			return false;
		}

		InstallLayoutOrTipProc installLayoutOrTip = (InstallLayoutOrTipProc)GetProcAddress(inputDll, "InstallLayoutOrTip");
		if (!installLayoutOrTip) {
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME InstallLayoutOrTip not found");
			FreeLibrary(inputDll);
			return false;
		}

		SetLastError(ERROR_SUCCESS);
		BOOL openKeyInstalled = installLayoutOrTip(OPENKEY_TIP, 0);
		DWORD openKeyError = GetLastError();
		bool openKeyRegistrySet = setVietnameseInputMethodTipValue(OPENKEY_TIP);
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME ensure OpenKey tip=%s api=%d error=%lu registry=%d",
			OPENKEY_TIP, openKeyInstalled ? 1 : 0, openKeyError, openKeyRegistrySet ? 1 : 0);

		std::vector<std::wstring> tips = readVietnameseInputMethodTips();
		std::vector<std::wstring> registeredTips = readRegisteredVietnameseTIPs();
		for (size_t i = 0; i < registeredTips.size(); ++i)
			addInputMethodTip(tips, registeredTips[i]);
		addInputMethodTip(tips, WINDOWS_VIETNAMESE_IME_TIP);
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME candidates count=%u", (unsigned int)tips.size());

		bool removedAny = false;
		for (size_t i = 0; i < tips.size(); ++i) {
			const std::wstring& tip = tips[i];
			if (sameInputMethodTip(tip, OPENKEY_TIP)) {
				DEBUG_LOG(L"TSF removeDefaultVietnameseIME keep OpenKey tip=%s", tip.c_str());
				continue;
			}

			bool profileDisabled = disableLanguageProfileForTip(tip);
			SetLastError(ERROR_SUCCESS);
			BOOL installed = installLayoutOrTip(tip.c_str(), ILOT_UNINSTALL);
			DWORD error = GetLastError();
			bool registryDeleted = deleteVietnameseInputMethodTipValue(tip);
			clearInputMethodOverrideIfMatches(tip);
			bool removed = profileDisabled || installed || registryDeleted;
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME remove tip=%s profile=%d api=%d error=%lu registry=%d removed=%d",
				tip.c_str(), profileDisabled ? 1 : 0, installed ? 1 : 0, error, registryDeleted ? 1 : 0, removed ? 1 : 0);
			removedAny = removedAny || removed;
		}
		// Remove Vietnamese keyboard layout from legacy Preload list.
		{
			HKEY preloadKey = NULL;
			if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &preloadKey) == ERROR_SUCCESS) {
				std::vector<std::wstring> toDelete;
				for (DWORD index = 0; ; ++index) {
					wchar_t valueName[16] = {};
					DWORD nameSize = ARRAYSIZE(valueName);
					wchar_t valueData[32] = {};
					DWORD dataSize = sizeof(valueData);
					DWORD type = 0;
					LSTATUS s = RegEnumValueW(preloadKey, index, valueName, &nameSize, NULL, &type, (LPBYTE)valueData, &dataSize);
					if (s == ERROR_NO_MORE_ITEMS)
						break;
					if (s != ERROR_SUCCESS || type != REG_SZ)
						continue;
					DWORD charCount = dataSize / sizeof(wchar_t);
					if (charCount > 0 && valueData[charCount - 1] == L'\0') charCount--;
					std::wstring data(valueData, charCount);
					if (_wcsicmp(data.c_str(), L"0000042a") == 0) {
						toDelete.push_back(std::wstring(valueName, nameSize));
					}
				}
				for (size_t i = 0; i < toDelete.size(); ++i) {
					LSTATUS del = RegDeleteValueW(preloadKey, toDelete[i].c_str());
					DEBUG_LOG(L"TSF removeDefaultVietnameseIME preload delete name=%s status=%lu", toDelete[i].c_str(), del);
				}
				RegCloseKey(preloadKey);
			}
		}

		broadcastInputProfileChange();

		// Write the CTF Assemblies entry for Vietnamese pointing to OpenKey as the default.
		// This tells Windows CTF to use OpenKey as the default Vietnamese input method assembly,
		// suppressing system Vietnamese TIPs from appearing in the input indicator.
		setVietnameseAssemblyToOpenKey();

		FreeLibrary(inputDll);
		std::vector<std::wstring> remainingTips = readVietnameseInputMethodTips();
		bool defaultStillPresent = false;
		for (size_t i = 0; i < remainingTips.size(); ++i) {
			bool isOpenKey = sameInputMethodTip(remainingTips[i], OPENKEY_TIP);
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME remaining tip=%s openkey=%d", remainingTips[i].c_str(), isOpenKey ? 1 : 0);
			defaultStillPresent = defaultStillPresent || (!isOpenKey && isVietnameseInputMethodTip(remainingTips[i]));
		}

		std::vector<std::wstring> registeredRemainingTips = readRegisteredVietnameseTIPs();
		for (size_t i = 0; i < registeredRemainingTips.size(); ++i) {
			bool isOpenKey = sameInputMethodTip(registeredRemainingTips[i], OPENKEY_TIP);
			DEBUG_LOG(L"TSF removeDefaultVietnameseIME registered tip=%s openkey=%d", registeredRemainingTips[i].c_str(), isOpenKey ? 1 : 0);
		}

		bool success = (openKeyInstalled || openKeyRegistrySet) && !defaultStillPresent;
		DEBUG_LOG(L"TSF removeDefaultVietnameseIME result removedAny=%d defaultStillPresent=%d success=%d",
			removedAny ? 1 : 0, defaultStillPresent ? 1 : 0, success ? 1 : 0);
		return success;
	}

	bool deactivateTIP() {
		DEBUG_LOG(L"TSF deactivateTIP begin");
		HRESULT hrCo = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		bool shouldUninitialize = SUCCEEDED(hrCo);
		if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE) {
			DEBUG_LOG(L"TSF deactivateTIP CoInitializeEx failed hr=0x%08X", (unsigned int)hrCo);
			return false;
		}

		bool success = false;
		ITfInputProcessorProfiles* profiles = NULL;
		HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
			IID_ITfInputProcessorProfiles, (void**)&profiles);
		if (SUCCEEDED(hr) && profiles) {
			ITfInputProcessorProfileMgr* profileMgr = NULL;
			HRESULT hrMgr = profiles->QueryInterface(IID_ITfInputProcessorProfileMgr, (void**)&profileMgr);
			if (SUCCEEDED(hrMgr) && profileMgr) {
				HRESULT hrDeactivate = profileMgr->DeactivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
					TIP_LANGID,
					TIP_CLSID_VALUE,
					TIP_PROFILE_GUID,
					NULL,
					TF_IPPMF_FORSESSION);
				HRESULT hrDeactivateDisable = profileMgr->DeactivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
					TIP_LANGID,
					TIP_CLSID_VALUE,
					TIP_PROFILE_GUID,
					NULL,
					TF_IPPMF_FORSESSION | TF_IPPMF_DISABLEPROFILE);
				DEBUG_LOG(L"TSF deactivateTIP DeactivateProfile hr=0x%08X disable=0x%08X", (unsigned int)hrDeactivate, (unsigned int)hrDeactivateDisable);
				success = SUCCEEDED(hrDeactivate) || SUCCEEDED(hrDeactivateDisable);
				profileMgr->Release();
			}
			else {
				DEBUG_LOG(L"TSF deactivateTIP QueryInterface ITfInputProcessorProfileMgr failed hr=0x%08X", (unsigned int)hrMgr);
			}

			HRESULT hrEnable = profiles->EnableLanguageProfile(TIP_CLSID_VALUE, TIP_LANGID, TIP_PROFILE_GUID, FALSE);
			HRESULT hrDefault = profiles->EnableLanguageProfileByDefault(TIP_CLSID_VALUE, TIP_LANGID, TIP_PROFILE_GUID, FALSE);
			DEBUG_LOG(L"TSF deactivateTIP disable hr=0x%08X default=0x%08X", (unsigned int)hrEnable, (unsigned int)hrDefault);
			success = success || SUCCEEDED(hrEnable) || SUCCEEDED(hrDefault);
			profiles->Release();
		}
		else {
			DEBUG_LOG(L"TSF deactivateTIP CoCreateInstance failed hr=0x%08X", (unsigned int)hr);
		}

		if (shouldUninitialize)
			CoUninitialize();
		DEBUG_LOG(L"TSF deactivateTIP result=%d", success ? 1 : 0);
		return success;
	}

	bool unregisterTIP(bool elevated) {
		UNREFERENCED_PARAMETER(elevated);
		DEBUG_LOG(L"TSF unregisterTIP elevated=%d", elevated ? 1 : 0);
		bool deactivated = deactivateTIP();
		DEBUG_LOG(L"TSF unregisterTIP deactivate result=%d", deactivated ? 1 : 0);
		std::wstring registeredPath = readRegisteredTIPDllPath();
		if (registeredPath.empty())
			registeredPath = getTIPDllPath();
		bool unregistered = callRegistrationExport("DllUnregisterServer", registeredPath);
		if (vForceUnloadTSFDLL) {
			// TSF deactivation is dispatched asynchronously to host processes via their message
			// loops. Wait before force-unloading so those threads can process the deactivation
			// and release TIP COM objects. Without this delay, FreeLibrary fires while vtables
			// still point into the DLL, crashing the host.
			Sleep(1500);
			forceUnloadTIPFromAllProcesses();
		} else {
			DEBUG_LOG(L"TSF force unload skipped: disabled by user setting");
		}
		return unregistered;
	}

	bool forceUnloadTIPFromAllProcesses() {
		// WARNING: FreeLibrary injected into a process while COM/TSF objects from the DLL are
		// still alive can crash that process. Call deactivateTIP() first and allow some time
		// for TSF to release TIP objects before calling this.
		std::wstring tipPath = getTIPDllPath();
		if (tipPath.empty()) return false;

		size_t slash = tipPath.rfind(L'\\');
		std::wstring tipFileName = (slash != std::wstring::npos) ? tipPath.substr(slash + 1) : tipPath;
		if (tipFileName.empty()) return false;

		DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: dll=%s", tipFileName.c_str());

		DWORD currentPid = GetCurrentProcessId();

		HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
		LPTHREAD_START_ROUTINE pFreeLibrary = hKernel32
			? (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary")
			: NULL;
		if (!pFreeLibrary) {
			DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: FreeLibrary not found");
			return false;
		}

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: snapshot failed err=%lu", GetLastError());
			return false;
		}

		bool anySuccess = false;
		PROCESSENTRY32W pe = {};
		pe.dwSize = sizeof(pe);
		if (Process32FirstW(hSnapshot, &pe)) {
			do {
				DWORD pid = pe.th32ProcessID;
				if (pid == currentPid || pid == 0 || pid == 4) continue;

				HANDLE hProcess = OpenProcess(
					PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
					FALSE, pid);
				if (!hProcess) continue;

				HMODULE hMods[1024];
				DWORD cbNeeded = 0;
				HMODULE hTipMod = NULL;
				if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
					DWORD count = cbNeeded / sizeof(HMODULE);
					for (DWORD i = 0; i < count && !hTipMod; i++) {
						wchar_t modName[MAX_PATH] = {};
						if (GetModuleFileNameExW(hProcess, hMods[i], modName, MAX_PATH)) {
							std::wstring modPath = modName;
							size_t s = modPath.rfind(L'\\');
							std::wstring modFileName = (s != std::wstring::npos) ? modPath.substr(s + 1) : modPath;
							if (_wcsicmp(modFileName.c_str(), tipFileName.c_str()) == 0) {
								hTipMod = hMods[i];
							}
						}
					}
				}

				if (hTipMod) {
					DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: found in pid=%lu, injecting FreeLibrary", pid);
					HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pFreeLibrary, hTipMod, 0, NULL);
					if (hThread) {
						WaitForSingleObject(hThread, 5000);
						CloseHandle(hThread);
						anySuccess = true;
						DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: unloaded from pid=%lu", pid);
					} else {
						DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: CreateRemoteThread failed pid=%lu err=%lu", pid, GetLastError());
					}
				}

				CloseHandle(hProcess);
			} while (Process32NextW(hSnapshot, &pe));
		}

		CloseHandle(hSnapshot);
		DEBUG_LOG(L"TSF forceUnloadTIPFromAllProcesses: done anySuccess=%d", anySuccess ? 1 : 0);
		return anySuccess;
	}

	bool suppressNonOpenKeyVietnameseTIPs() {
		std::vector<std::wstring> tips = readRegisteredVietnameseTIPs();
		bool anyWritten = false;
		for (size_t i = 0; i < tips.size(); ++i) {
			if (!sameInputMethodTip(tips[i], OPENKEY_TIP)) {
				bool written = writeLanguageProfileEnableState(tips[i], 0);
				DEBUG_LOG(L"TSF suppressNonOpenKeyVietnameseTIPs tip=%s written=%d", tips[i].c_str(), written ? 1 : 0);
				anyWritten = anyWritten || written;
			}
		}
		return anyWritten;
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
