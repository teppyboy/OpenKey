#pragma once

#include <msctf.h>
#include <string>

class CCompositionManager;

class CEditSession : public ITfEditSession
{
public:
    enum Operation
    {
        OperationUpdateComposition,
        OperationCommitComposition,
        OperationReplaceLeftText,
        OperationClearComposition
    };

    CEditSession(CCompositionManager *manager, ITfContext *context, Operation operation, const std::wstring &text);
    CEditSession(CCompositionManager *manager, ITfContext *context, Operation operation, const std::wstring &text, LONG cchDelete);
    virtual ~CEditSession();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    LONG _cRef;
    CCompositionManager *_manager;
    ITfContext *_context;
    Operation _operation;
    std::wstring _text;
    LONG _cchDelete;
};
