#include "composition.h"

#include <oleauto.h>

#ifndef TF_INVALID_GUIDATOM
#define TF_INVALID_GUIDATOM 0
#endif

static HRESULT ValidateTextArgs(const wchar_t *text, LONG cch)
{
    if (cch < 0)
    {
        return E_INVALIDARG;
    }

    if (text == NULL && cch > 0)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

CCompositionManager::CCompositionManager(ITfCompositionSink *sink) :
    _cRef(1),
    _composition(NULL),
    _context(NULL),
    _sink(sink),
    _displayAttributeAtom(TF_INVALID_GUIDATOM)
{
    if (_sink != NULL)
    {
        _sink->AddRef();
    }
}

CCompositionManager::~CCompositionManager()
{
    ClearComposition();

    if (_sink != NULL)
    {
        _sink->Release();
        _sink = NULL;
    }
}

bool CCompositionManager::HasComposition() const
{
    return _composition != NULL;
}

ULONG CCompositionManager::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

ULONG CCompositionManager::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return (ULONG)cRef;
}

void CCompositionManager::SetDisplayAttributeAtom(TfGuidAtom atom)
{
    _displayAttributeAtom = atom;
}

void CCompositionManager::ApplyDisplayAttribute(ITfContext *context, TfEditCookie ecWrite, ITfRange *range)
{
    if (context == NULL || range == NULL || _displayAttributeAtom == TF_INVALID_GUIDATOM)
    {
        return;
    }

    ITfProperty *property = NULL;
    HRESULT hr = context->GetProperty(GUID_PROP_ATTRIBUTE, &property);
    if (SUCCEEDED(hr) && property != NULL)
    {
        VARIANT value;
        VariantInit(&value);
        value.vt = VT_I4;
        value.lVal = (LONG)_displayAttributeAtom;
        property->SetValue(ecWrite, range, &value);
        property->Release();
    }
}

bool CCompositionManager::IsSameContext(ITfContext *context) const
{
    if (_context == NULL || context == NULL)
    {
        return _context == context;
    }

    IUnknown *left = NULL;
    IUnknown *right = NULL;
    HRESULT hrLeft = _context->QueryInterface(IID_IUnknown, (void **)&left);
    HRESULT hrRight = context->QueryInterface(IID_IUnknown, (void **)&right);
    bool same = SUCCEEDED(hrLeft) && SUCCEEDED(hrRight) && left == right;
    if (left != NULL)
    {
        left->Release();
    }
    if (right != NULL)
    {
        right->Release();
    }

    return same;
}

HRESULT CCompositionManager::StartOrUpdate(ITfContext *context, TfEditCookie ecWrite, const wchar_t *text, LONG cch)
{
    HRESULT hr = ValidateTextArgs(text, cch);
    if (FAILED(hr))
    {
        return hr;
    }

    if (context == NULL || _sink == NULL)
    {
        return E_INVALIDARG;
    }

    if (_composition != NULL)
    {
        if (!IsSameContext(context))
        {
            return E_FAIL;
        }

        ITfRange *range = NULL;
        hr = _composition->GetRange(&range);
        if (SUCCEEDED(hr) && range != NULL)
        {
            hr = range->SetText(ecWrite, 0, text, cch);
            if (SUCCEEDED(hr))
            {
                ApplyDisplayAttribute(context, ecWrite, range);
            }
            range->Release();
        }
        return hr;
    }

    ITfInsertAtSelection *insertAtSelection = NULL;
    hr = context->QueryInterface(IID_ITfInsertAtSelection, (void **)&insertAtSelection);
    if (FAILED(hr))
    {
        return hr;
    }

    ITfRange *range = NULL;
    hr = insertAtSelection->InsertTextAtSelection(ecWrite, 0, L"", 0, &range);
    insertAtSelection->Release();
    if (FAILED(hr))
    {
        return hr;
    }

    if (range == NULL)
    {
        return E_FAIL;
    }

    ITfContextComposition *contextComposition = NULL;
    hr = context->QueryInterface(IID_ITfContextComposition, (void **)&contextComposition);
    if (SUCCEEDED(hr))
    {
        ITfComposition *composition = NULL;
        hr = contextComposition->StartComposition(ecWrite, range, _sink, &composition);
        if (SUCCEEDED(hr) && composition != NULL)
        {
            _composition = composition;
            _context = context;
            _context->AddRef();
            hr = range->SetText(ecWrite, 0, text, cch);
            if (SUCCEEDED(hr))
            {
                ApplyDisplayAttribute(context, ecWrite, range);
            }
            if (FAILED(hr))
            {
                HRESULT endHr = _composition->EndComposition(ecWrite);
                if (SUCCEEDED(endHr))
                {
                    ClearComposition();
                }
            }
        }
        contextComposition->Release();
    }

    range->Release();
    return hr;
}

HRESULT CCompositionManager::Commit(ITfContext *context, TfEditCookie ecWrite, const wchar_t *text, LONG cch)
{
    HRESULT hr = ValidateTextArgs(text, cch);
    if (FAILED(hr))
    {
        return hr;
    }

    if (context == NULL)
    {
        return E_INVALIDARG;
    }

    if (_composition != NULL)
    {
        if (!IsSameContext(context))
        {
            return E_FAIL;
        }

        ITfRange *range = NULL;
        hr = _composition->GetRange(&range);
        if (SUCCEEDED(hr) && range != NULL)
        {
            hr = range->SetText(ecWrite, 0, text, cch);
            if (SUCCEEDED(hr))
            {
                ApplyDisplayAttribute(context, ecWrite, range);
            }
            range->Release();
        }

        if (SUCCEEDED(hr))
        {
            hr = _composition->EndComposition(ecWrite);
        }

        if (SUCCEEDED(hr))
        {
            ClearComposition();
        }
        return hr;
    }

    ITfInsertAtSelection *insertAtSelection = NULL;
    hr = context->QueryInterface(IID_ITfInsertAtSelection, (void **)&insertAtSelection);
    if (FAILED(hr))
    {
        return hr;
    }

    ITfRange *range = NULL;
    hr = insertAtSelection->InsertTextAtSelection(ecWrite, 0, text, cch, &range);
    insertAtSelection->Release();
    if (SUCCEEDED(hr) && range != NULL)
    {
        if (SUCCEEDED(range->Collapse(ecWrite, TF_ANCHOR_END)))
        {
            TF_SELECTION selection;
            selection.range = range;
            selection.style.ase = TF_AE_NONE;
            selection.style.fInterimChar = FALSE;
            context->SetSelection(ecWrite, 1, &selection);
        }
        range->Release();
    }

    return hr;
}

HRESULT CCompositionManager::ReplaceLeftText(ITfContext *context, TfEditCookie ecWrite, LONG cchDelete, const wchar_t *text, LONG cch)
{
    HRESULT hr = ValidateTextArgs(text, cch);
    if (FAILED(hr))
    {
        return hr;
    }

    if (context == NULL || cchDelete < 0)
    {
        return E_INVALIDARG;
    }

    if (_composition != NULL)
    {
        if (!IsSameContext(context))
        {
            return E_FAIL;
        }

        ITfRange *range = NULL;
        hr = _composition->GetRange(&range);
        if (SUCCEEDED(hr) && range != NULL)
        {
            hr = range->SetText(ecWrite, 0, text, cch);
            if (SUCCEEDED(hr))
            {
                ApplyDisplayAttribute(context, ecWrite, range);
            }
            range->Release();
        }

        if (SUCCEEDED(hr))
        {
            hr = _composition->EndComposition(ecWrite);
        }
        if (SUCCEEDED(hr))
        {
            ClearComposition();
        }
        return hr;
    }

    TF_SELECTION selection = {};
    ULONG fetched = 0;
    hr = context->GetSelection(ecWrite, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
    if (FAILED(hr))
    {
        return hr;
    }
    if (fetched != 1 || selection.range == NULL)
    {
        return E_FAIL;
    }

    ITfRange *range = NULL;
    hr = selection.range->Clone(&range);
    selection.range->Release();
    if (FAILED(hr) || range == NULL)
    {
        return FAILED(hr) ? hr : E_FAIL;
    }

    BOOL selectionEmpty = TRUE;
    hr = range->IsEmpty(ecWrite, &selectionEmpty);
    if (SUCCEEDED(hr) && selectionEmpty)
    {
        hr = range->Collapse(ecWrite, TF_ANCHOR_END);
    }
    if (SUCCEEDED(hr) && selectionEmpty && cchDelete > 0)
    {
        LONG shifted = 0;
        hr = range->ShiftStart(ecWrite, -cchDelete, &shifted, NULL);
        if (SUCCEEDED(hr) && shifted != -cchDelete)
        {
            hr = E_FAIL;
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = range->SetText(ecWrite, 0, text, cch);
    }
    if (SUCCEEDED(hr))
    {
        hr = range->Collapse(ecWrite, TF_ANCHOR_END);
    }
    if (SUCCEEDED(hr))
    {
        TF_SELECTION newSelection = {};
        newSelection.range = range;
        newSelection.style.ase = TF_AE_END;
        newSelection.style.fInterimChar = FALSE;
        hr = context->SetSelection(ecWrite, 1, &newSelection);
    }

    range->Release();
    return hr;
}

HRESULT CCompositionManager::EndComposition(TfEditCookie ecWrite)
{
    if (_composition == NULL)
    {
        return S_OK;
    }

    HRESULT hr = _composition->EndComposition(ecWrite);
    if (SUCCEEDED(hr))
    {
        ClearComposition();
    }

    return hr;
}

void CCompositionManager::ClearComposition(ITfComposition *composition)
{
    if (composition != NULL && _composition != NULL)
    {
        IUnknown *left = NULL;
        IUnknown *right = NULL;
        HRESULT hrLeft = composition->QueryInterface(IID_IUnknown, (void **)&left);
        HRESULT hrRight = _composition->QueryInterface(IID_IUnknown, (void **)&right);
        bool same = SUCCEEDED(hrLeft) && SUCCEEDED(hrRight) && left == right;
        if (left != NULL)
        {
            left->Release();
        }
        if (right != NULL)
        {
            right->Release();
        }
        if (!same)
        {
            return;
        }
    }

    if (_composition != NULL)
    {
        _composition->Release();
        _composition = NULL;
    }
    if (_context != NULL)
    {
        _context->Release();
        _context = NULL;
    }
}
