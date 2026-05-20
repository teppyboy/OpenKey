#include "displayattr.h"

#include "globals.h"

#include <new>
#include <oleauto.h>

static void InitOpenKeyDisplayAttribute(TF_DISPLAYATTRIBUTE *attribute)
{
    attribute->crText.type = TF_CT_NONE;
    attribute->crText.nIndex = 0;
    attribute->crBk.type = TF_CT_NONE;
    attribute->crBk.nIndex = 0;
    attribute->lsStyle = TF_LS_SOLID;
    attribute->fBoldLine = FALSE;
    attribute->crLine.type = TF_CT_NONE;
    attribute->crLine.nIndex = 0;
    attribute->bAttr = TF_ATTR_INPUT;
}

COpenKeyDisplayAttributeInfo::COpenKeyDisplayAttributeInfo() :
    _cRef(1)
{
    InitOpenKeyDisplayAttribute(&_attribute);
    InterlockedIncrement(&g_cDllRef);
}

COpenKeyDisplayAttributeInfo::~COpenKeyDisplayAttributeInfo()
{
    InterlockedDecrement(&g_cDllRef);
}

STDAPI COpenKeyDisplayAttributeInfo::QueryInterface(REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfDisplayAttributeInfo))
    {
        *ppv = static_cast<ITfDisplayAttributeInfo *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) COpenKeyDisplayAttributeInfo::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) COpenKeyDisplayAttributeInfo::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return (ULONG)cRef;
}

STDAPI COpenKeyDisplayAttributeInfo::GetGUID(GUID *pguid)
{
    if (pguid == NULL)
    {
        return E_INVALIDARG;
    }

    *pguid = GUID_OpenKeyDisplayAttr;
    return S_OK;
}

STDAPI COpenKeyDisplayAttributeInfo::GetDescription(BSTR *pbstrDesc)
{
    if (pbstrDesc == NULL)
    {
        return E_INVALIDARG;
    }

    *pbstrDesc = SysAllocString(L"OpenKey composition");
    return *pbstrDesc != NULL ? S_OK : E_OUTOFMEMORY;
}

STDAPI COpenKeyDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE *pda)
{
    if (pda == NULL)
    {
        return E_INVALIDARG;
    }

    *pda = _attribute;
    return S_OK;
}

STDAPI COpenKeyDisplayAttributeInfo::SetAttributeInfo(const TF_DISPLAYATTRIBUTE *pda)
{
    if (pda == NULL)
    {
        return E_INVALIDARG;
    }

    _attribute = *pda;
    return S_OK;
}

STDAPI COpenKeyDisplayAttributeInfo::Reset()
{
    InitOpenKeyDisplayAttribute(&_attribute);
    return S_OK;
}

CEnumOpenKeyDisplayAttributeInfo::CEnumOpenKeyDisplayAttributeInfo() :
    _cRef(1),
    _fDone(false)
{
    InterlockedIncrement(&g_cDllRef);
}

CEnumOpenKeyDisplayAttributeInfo::~CEnumOpenKeyDisplayAttributeInfo()
{
    InterlockedDecrement(&g_cDllRef);
}

STDAPI CEnumOpenKeyDisplayAttributeInfo::QueryInterface(REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo))
    {
        *ppv = static_cast<IEnumTfDisplayAttributeInfo *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CEnumOpenKeyDisplayAttributeInfo::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CEnumOpenKeyDisplayAttributeInfo::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return (ULONG)cRef;
}

STDAPI CEnumOpenKeyDisplayAttributeInfo::Clone(IEnumTfDisplayAttributeInfo **ppEnum)
{
    if (ppEnum == NULL)
    {
        return E_INVALIDARG;
    }

    *ppEnum = new (std::nothrow) CEnumOpenKeyDisplayAttributeInfo();
    if (*ppEnum == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if (_fDone)
    {
        (*ppEnum)->Skip(1);
    }
    return S_OK;
}

STDAPI CEnumOpenKeyDisplayAttributeInfo::Next(ULONG ulCount, ITfDisplayAttributeInfo **rgInfo, ULONG *pcFetched)
{
    if (rgInfo == NULL || (ulCount != 1 && pcFetched == NULL))
    {
        return E_INVALIDARG;
    }

    if (pcFetched != NULL)
    {
        *pcFetched = 0;
    }
    if (ulCount == 0)
    {
        return S_OK;
    }

    rgInfo[0] = NULL;
    if (_fDone)
    {
        return S_FALSE;
    }

    COpenKeyDisplayAttributeInfo *info = new (std::nothrow) COpenKeyDisplayAttributeInfo();
    if (info == NULL)
    {
        return E_OUTOFMEMORY;
    }

    rgInfo[0] = info;
    _fDone = true;
    if (pcFetched != NULL)
    {
        *pcFetched = 1;
    }
    return ulCount == 1 ? S_OK : S_FALSE;
}

STDAPI CEnumOpenKeyDisplayAttributeInfo::Reset()
{
    _fDone = false;
    return S_OK;
}

STDAPI CEnumOpenKeyDisplayAttributeInfo::Skip(ULONG ulCount)
{
    if (ulCount == 0)
    {
        return S_OK;
    }

    if (_fDone)
    {
        return S_FALSE;
    }

    _fDone = true;
    return ulCount == 1 ? S_OK : S_FALSE;
}
