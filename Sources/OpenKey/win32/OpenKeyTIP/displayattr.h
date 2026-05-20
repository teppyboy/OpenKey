#pragma once

#include <msctf.h>

class COpenKeyDisplayAttributeInfo : public ITfDisplayAttributeInfo
{
public:
    COpenKeyDisplayAttributeInfo();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ITfDisplayAttributeInfo
    STDMETHODIMP GetGUID(GUID *pguid);
    STDMETHODIMP GetDescription(BSTR *pbstrDesc);
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE *pda);
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE *pda);
    STDMETHODIMP Reset();

private:
    ~COpenKeyDisplayAttributeInfo();

    LONG _cRef;
    TF_DISPLAYATTRIBUTE _attribute;
};

class CEnumOpenKeyDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo
{
public:
    CEnumOpenKeyDisplayAttributeInfo();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumTfDisplayAttributeInfo
    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo **rgInfo, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    ~CEnumOpenKeyDisplayAttributeInfo();

    LONG _cRef;
    bool _fDone;
};
