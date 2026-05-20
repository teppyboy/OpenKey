#pragma once

#include <Unknwn.h>

class CClassFactory : public IClassFactory
{
public:
    CClassFactory();
    virtual ~CClassFactory();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
    STDMETHODIMP LockServer(BOOL fLock);

private:
    LONG _cRef;
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI DllCanUnloadNow();
