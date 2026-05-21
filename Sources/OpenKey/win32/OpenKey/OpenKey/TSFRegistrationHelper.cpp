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
		// TSF deactivation is dispatched asynchronously to host processes via their message
		// loops. Wait before force-unloading so those threads can process the deactivation
		// and release TIP COM objects. Without this delay, FreeLibrary fires while vtables
		// still point into the DLL, crashing the host.
		Sleep(1500);
		forceUnloadTIPFromAllProcesses();
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
