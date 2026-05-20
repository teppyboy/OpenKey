#include "editsession.h"

#include "composition.h"

CEditSession::CEditSession(CCompositionManager *manager, ITfContext *context, Operation operation, const std::wstring &text) :
    CEditSession(manager, context, operation, text, 0)
{
}

CEditSession::CEditSession(CCompositionManager *manager, ITfContext *context, Operation operation, const std::wstring &text, LONG cchDelete) :
    _cRef(1),
    _manager(manager),
    _context(context),
    _operation(operation),
    _text(text),
    _cchDelete(cchDelete)
{
    if (_context != NULL)
    {
        _context->AddRef();
    }
    if (_manager != NULL)
    {
        _manager->AddRef();
    }
}

CEditSession::~CEditSession()
{
    if (_context != NULL)
    {
        _context->Release();
        _context = NULL;
    }
    if (_manager != NULL)
    {
        _manager->Release();
        _manager = NULL;
    }
}

STDAPI CEditSession::QueryInterface(REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppv = static_cast<ITfEditSession *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) CEditSession::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CEditSession::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return (ULONG)cRef;
}

STDAPI CEditSession::DoEditSession(TfEditCookie ec)
{
    if (_manager == NULL || _context == NULL)
    {
        return E_INVALIDARG;
    }

    const wchar_t *text = _text.empty() ? L"" : _text.c_str();
    LONG cch = (LONG)_text.length();

    switch (_operation)
    {
    case OperationUpdateComposition:
        return _manager->StartOrUpdate(_context, ec, text, cch);
    case OperationCommitComposition:
        return _manager->Commit(_context, ec, text, cch);
    case OperationReplaceLeftText:
        return _manager->ReplaceLeftText(_context, ec, _cchDelete, text, cch);
    case OperationClearComposition:
        return _manager->EndComposition(ec);
    default:
        return E_INVALIDARG;
    }
}
