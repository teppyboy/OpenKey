#pragma once

#include <msctf.h>

class CCompositionManager
{
public:
    explicit CCompositionManager(ITfCompositionSink *sink);
    ~CCompositionManager();

    bool HasComposition() const;
    ULONG AddRef();
    ULONG Release();
    void SetDisplayAttributeAtom(TfGuidAtom atom);
    HRESULT StartOrUpdate(ITfContext *context, TfEditCookie ecWrite, const wchar_t *text, LONG cch);
    HRESULT Commit(ITfContext *context, TfEditCookie ecWrite, const wchar_t *text, LONG cch);
    HRESULT ReplaceLeftText(ITfContext *context, TfEditCookie ecWrite, LONG cchDelete, const wchar_t *text, LONG cch);
    HRESULT EndComposition(TfEditCookie ecWrite);
    void ClearComposition(ITfComposition *composition = NULL);

private:
    bool IsSameContext(ITfContext *context) const;
    void ApplyDisplayAttribute(ITfContext *context, TfEditCookie ecWrite, ITfRange *range);

    LONG _cRef;
    ITfComposition *_composition;
    ITfContext *_context;
    ITfCompositionSink *_sink;
    TfGuidAtom _displayAttributeAtom;
};
