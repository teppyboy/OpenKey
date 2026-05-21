#pragma once

#include "composition.h"
#include "editsession.h"
#include "enginebridge.h"

#include <msctf.h>
#include <string>

class COpenKeyTIP : public ITfTextInputProcessorEx,
                    public ITfCompositionSink,
                    public ITfKeyEventSink,
                    public ITfThreadMgrEventSink,
                    public ITfDisplayAttributeProvider
{
public:
    COpenKeyTIP();
    virtual ~COpenKeyTIP();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // ITfTextInputProcessor
    STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    STDMETHODIMP Deactivate();

    // ITfTextInputProcessorEx
    STDMETHODIMP ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD dwFlags);

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten);

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *pdim);
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *pdim);
    STDMETHODIMP OnSetFocus(ITfDocumentMgr *pdimFocus, ITfDocumentMgr *pdimPrevFocus);
    STDMETHODIMP OnPushContext(ITfContext *pic);
    STDMETHODIMP OnPopContext(ITfContext *pic);

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo **ppInfo);

private:
    HRESULT RequestEditSession(ITfContext *context, CEditSession::Operation operation, const std::wstring &text);
    HRESULT RequestEditSession(ITfContext *context, CEditSession::Operation operation, const std::wstring &text, LONG cchDelete);
    HRESULT ReplaceLeftTextOrFallback(ITfContext *context, const std::wstring &text, LONG cchDelete, BOOL *pfEaten);
    bool IsRuntimeEnabled();
    void ResetSessionState(ITfContext *context);
    void ApplySmartSwitchForForegroundApp();

    LONG _cRef;
    ITfThreadMgr *_pThreadMgr;
    ITfKeystrokeMgr *_pKeystrokeMgr;
    ITfSource *_pThreadMgrEventSource;
    TfClientId _tfClientId;
    TfGuidAtom _displayAttributeAtom;
    DWORD _dwThreadMgrEventSinkCookie;
    CCompositionManager *_composition;
    COpenKeyEngineBridge _engine;
    BOOL _fKeyEventSinkAdvised;
    BOOL _fThreadMgrEventSinkAdvised;
    BOOL _fProcessingKey;
    BOOL _fDisabledForApp;
};
