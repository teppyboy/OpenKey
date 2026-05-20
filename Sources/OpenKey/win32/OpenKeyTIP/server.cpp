#include "server.h"

#include "globals.h"
#include "tip.h"

#include <new>

CClassFactory::CClassFactory() : _cRef(1)
{
    InterlockedIncrement(&g_cDllRef);
}

CClassFactory::~CClassFactory()
{
    InterlockedDecrement(&g_cDllRef);
}

STDAPI CClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = static_cast<IClassFactory *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CClassFactory::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CClassFactory::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return (ULONG)cRef;
}

STDAPI CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (pUnkOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION;
    }

    COpenKeyTIP *pTip = new (std::nothrow) COpenKeyTIP();
    if (pTip == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pTip->QueryInterface(riid, ppv);
    pTip->Release();
    return hr;
}

STDAPI CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        InterlockedIncrement(&g_cDllRef);
    }
    else
    {
        InterlockedDecrement(&g_cDllRef);
    }

    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualCLSID(rclsid, CLSID_OpenKeyTIP))
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    CClassFactory *pFactory = new (std::nothrow) CClassFactory();
    if (pFactory == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_cDllRef == 0) ? S_OK : S_FALSE;
}
