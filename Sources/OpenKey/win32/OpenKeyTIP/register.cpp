#include "globals.h"

#include <msctf.h>

namespace
{
    // Keep CLSID string in sync with CLSID_OpenKeyTIP in globals.h
    const WCHAR OpenKeyClsidKey[] = L"Software\\Classes\\CLSID\\{A942CCFA-976D-4D60-93AD-5CEBE269751F}";
    const WCHAR OpenKeyInprocKey[] = L"Software\\Classes\\CLSID\\{A942CCFA-976D-4D60-93AD-5CEBE269751F}\\InProcServer32";
    const WCHAR OpenKeyTipName[] = L"OpenKey Vietnamese TIP";
    const WCHAR ThreadingModel[] = L"Apartment";

    HRESULT HResultFromWin32(LSTATUS status)
    {
        return (status == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(status);
    }

    HRESULT SetStringValue(HKEY hKey, const WCHAR *valueName, const WCHAR *value)
    {
        DWORD cbData = (DWORD)((lstrlenW(value) + 1) * sizeof(WCHAR));
        return HResultFromWin32(RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE *)value, cbData));
    }

    HRESULT WriteComRegistration(const WCHAR *dllPath)
    {
        HKEY hKey = NULL;
        LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, OpenKeyClsidKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        HRESULT hr = HResultFromWin32(status);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = SetStringValue(hKey, NULL, OpenKeyTipName);
        RegCloseKey(hKey);
        if (FAILED(hr))
        {
            return hr;
        }

        status = RegCreateKeyExW(HKEY_CURRENT_USER, OpenKeyInprocKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        hr = HResultFromWin32(status);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = SetStringValue(hKey, NULL, dllPath);
        if (SUCCEEDED(hr))
        {
            hr = SetStringValue(hKey, L"ThreadingModel", ThreadingModel);
        }

        RegCloseKey(hKey);
        return hr;
    }

    HRESULT DeleteComRegistration()
    {
        LSTATUS status = RegDeleteTreeW(HKEY_CURRENT_USER, OpenKeyClsidKey);
        if (status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND)
        {
            return S_OK;
        }

        return HResultFromWin32(status);
    }

    HRESULT InitializeCom(bool *shouldUninitialize)
    {
        *shouldUninitialize = false;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (hr == S_OK || hr == S_FALSE)
        {
            *shouldUninitialize = true;
            return S_OK;
        }

        if (hr == RPC_E_CHANGED_MODE)
        {
            return S_OK;
        }

        return hr;
    }

    void SaveFirstFailure(HRESULT hr, HRESULT *firstFailure)
    {
        if (FAILED(hr) && SUCCEEDED(*firstFailure))
        {
            *firstFailure = hr;
        }
    }

    bool IsMissingRegistrationFailure(HRESULT hr)
    {
        return hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    HRESULT IgnoreMissingRegistration(HRESULT hr)
    {
        return IsMissingRegistrationFailure(hr) ? S_OK : hr;
    }

    void DeactivateTsfProfile(ITfInputProcessorProfiles *profiles)
    {
        if (profiles == NULL)
        {
            return;
        }

        ITfInputProcessorProfileMgr *profileMgr = NULL;
        HRESULT hr = profiles->QueryInterface(IID_ITfInputProcessorProfileMgr, (void **)&profileMgr);
        if (SUCCEEDED(hr) && profileMgr != NULL)
        {
            profileMgr->DeactivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
                LANGID_OpenKeyVietnamese,
                CLSID_OpenKeyTIP,
                GUID_OpenKeyProfile,
                NULL,
                TF_IPPMF_FORSESSION);
            profileMgr->DeactivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
                LANGID_OpenKeyVietnamese,
                CLSID_OpenKeyTIP,
                GUID_OpenKeyProfile,
                NULL,
                TF_IPPMF_FORSESSION | TF_IPPMF_DISABLEPROFILE);
            profileMgr->Release();
        }
    }

    void RemoveTsfRegistration(ITfInputProcessorProfiles *profiles, ITfCategoryMgr *categoryMgr, HRESULT *firstFailure)
    {
        if (categoryMgr != NULL)
        {
            SaveFirstFailure(IgnoreMissingRegistration(categoryMgr->UnregisterCategory(CLSID_OpenKeyTIP, GUID_TFCAT_TIP_KEYBOARD, CLSID_OpenKeyTIP)), firstFailure);
            SaveFirstFailure(IgnoreMissingRegistration(categoryMgr->UnregisterCategory(CLSID_OpenKeyTIP, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, CLSID_OpenKeyTIP)), firstFailure);
        }


        if (profiles != NULL)
        {
            DeactivateTsfProfile(profiles);
            SaveFirstFailure(IgnoreMissingRegistration(profiles->EnableLanguageProfile(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile, FALSE)), firstFailure);
            SaveFirstFailure(IgnoreMissingRegistration(profiles->EnableLanguageProfileByDefault(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile, FALSE)), firstFailure);
            SaveFirstFailure(IgnoreMissingRegistration(profiles->RemoveLanguageProfile(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile)), firstFailure);
            SaveFirstFailure(IgnoreMissingRegistration(profiles->Unregister(CLSID_OpenKeyTIP)), firstFailure);
        }
    }

    void RollbackRegistration(ITfInputProcessorProfiles *profiles, ITfCategoryMgr *categoryMgr)
    {
        HRESULT ignoredFailure = S_OK;
        RemoveTsfRegistration(profiles, categoryMgr, &ignoredFailure);
        DeleteComRegistration();
    }
}

STDAPI DllRegisterServer()
{
    WCHAR dllPath[MAX_PATH];
    DWORD pathLength = GetModuleFileNameW(g_hInst, dllPath, ARRAYSIZE(dllPath));
    if (pathLength == 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (pathLength >= ARRAYSIZE(dllPath))
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    HRESULT hr = WriteComRegistration(dllPath);
    if (FAILED(hr))
    {
        DeleteComRegistration();
        return hr;
    }

    bool shouldUninitialize = false;
    hr = InitializeCom(&shouldUninitialize);
    if (FAILED(hr))
    {
        DeleteComRegistration();
        return hr;
    }

    ITfInputProcessorProfiles *profiles = NULL;
    ITfCategoryMgr *categoryMgr = NULL;
    HRESULT firstFailure = S_OK;

    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void **)&profiles);
    SaveFirstFailure(hr, &firstFailure);

    if (SUCCEEDED(firstFailure))
    {
        hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **)&categoryMgr);
        SaveFirstFailure(hr, &firstFailure);
    }

    if (SUCCEEDED(firstFailure))
    {
        HRESULT ignoredFailure = S_OK;
        RemoveTsfRegistration(profiles, categoryMgr, &ignoredFailure);

        hr = profiles->Register(CLSID_OpenKeyTIP);
        if (SUCCEEDED(hr))
        {
            hr = profiles->AddLanguageProfile(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile,
                OpenKeyDescription, (ULONG)lstrlenW(OpenKeyDescription), dllPath, (ULONG)lstrlenW(dllPath), 0);
        }
        if (SUCCEEDED(hr))
        {
            hr = profiles->EnableLanguageProfile(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile, TRUE);
        }
        if (SUCCEEDED(hr))
        {
            profiles->EnableLanguageProfileByDefault(CLSID_OpenKeyTIP, LANGID_OpenKeyVietnamese, GUID_OpenKeyProfile, TRUE);
        }
        SaveFirstFailure(hr, &firstFailure);
    }

    if (SUCCEEDED(firstFailure))
    {
        hr = categoryMgr->RegisterCategory(CLSID_OpenKeyTIP, GUID_TFCAT_TIP_KEYBOARD, CLSID_OpenKeyTIP);
        SaveFirstFailure(hr, &firstFailure);

        hr = categoryMgr->RegisterCategory(CLSID_OpenKeyTIP, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, CLSID_OpenKeyTIP);
        SaveFirstFailure(hr, &firstFailure);
    }

    if (FAILED(firstFailure))
    {
        RollbackRegistration(profiles, categoryMgr);
    }

    if (categoryMgr != NULL)
    {
        categoryMgr->Release();
    }

    if (profiles != NULL)
    {
        profiles->Release();
    }

    if (shouldUninitialize)
    {
        CoUninitialize();
    }

    return firstFailure;
}

STDAPI DllUnregisterServer()
{
    bool shouldUninitialize = false;
    HRESULT hr = InitializeCom(&shouldUninitialize);
    HRESULT firstFailure = hr;

    if (SUCCEEDED(hr))
    {
        ITfInputProcessorProfiles *profiles = NULL;
        ITfCategoryMgr *categoryMgr = NULL;

        hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void **)&profiles);
        if (FAILED(hr))
        {
            SaveFirstFailure(hr, &firstFailure);
        }

        hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void **)&categoryMgr);
        if (FAILED(hr))
        {
            SaveFirstFailure(hr, &firstFailure);
        }

        RemoveTsfRegistration(profiles, categoryMgr, &firstFailure);

        if (categoryMgr != NULL)
        {
            categoryMgr->Release();
        }

        if (profiles != NULL)
        {
            profiles->Release();
        }
    }

    SaveFirstFailure(DeleteComRegistration(), &firstFailure);

    if (shouldUninitialize)
    {
        CoUninitialize();
    }

    return firstFailure;
}
