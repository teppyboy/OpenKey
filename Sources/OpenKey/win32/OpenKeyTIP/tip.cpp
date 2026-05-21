#include "tip.h"

#include "../../engine/Engine.h"
#include "../../engine/EngineOutput.h"
#include "../../engine/SmartSwitchKey.h"
#include "../../engine/Vietnamese.h"
#include "fallback.h"
#include "displayattr.h"
#include "globals.h"

#include <new>

#ifndef TF_INVALID_GUIDATOM
#define TF_INVALID_GUIDATOM 0
#endif

static std::string GetForegroundExecutableName()
{
    HWND hwnd = GetForegroundWindow();
    if (hwnd == NULL)
    {
        return std::string();
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0)
    {
        return std::string();
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (process == NULL)
    {
        return std::string();
    }

    wchar_t path[32768];
    DWORD cchPath = ARRAYSIZE(path) - 1;
    BOOL ok = QueryFullProcessImageNameW(process, 0, path, &cchPath);
    CloseHandle(process);
    if (!ok || cchPath == 0)
    {
        return std::string();
    }
    path[cchPath] = L'\0';

    const wchar_t *name = path;
    for (DWORD i = 0; i < cchPath; ++i)
    {
        if (path[i] == L'\\' || path[i] == L'/')
        {
            name = path + i + 1;
        }
    }

    int required = WideCharToMultiByte(CP_UTF8, 0, name, -1, NULL, 0, NULL, NULL);
    if (required <= 1)
    {
        return std::string();
    }

    std::string exe(required, '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, name, -1, &exe[0], required, NULL, NULL) == 0)
    {
        return std::string();
    }
    exe.resize(required - 1);

    return exe;
}

static bool IsControlModified()
{
    return (GetKeyState(VK_CONTROL) < 0) ||
        (GetKeyState(VK_MENU) < 0) ||
        (GetKeyState(VK_LWIN) < 0) ||
        (GetKeyState(VK_RWIN) < 0);
}

static bool IsModifierKey(WPARAM wParam)
{
    return wParam == VK_CONTROL ||
        wParam == VK_LCONTROL ||
        wParam == VK_RCONTROL ||
        wParam == VK_SHIFT ||
        wParam == VK_LSHIFT ||
        wParam == VK_RSHIFT ||
        wParam == VK_MENU ||
        wParam == VK_LMENU ||
        wParam == VK_RMENU ||
        wParam == VK_LWIN ||
        wParam == VK_RWIN;
}

static Uint8 GetCapsStatus()
{
    const bool shift = GetKeyState(VK_SHIFT) < 0;
    const bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
    return (shift && caps) ? 0 : (shift ? 1 : (caps ? 2 : 0));
}

static bool TranslateKey(WPARAM wParam, Uint16 *keyCode)
{
    if (keyCode == NULL)
    {
        return false;
    }

    if ((wParam >= 'A' && wParam <= 'Z') ||
        (wParam >= '0' && wParam <= '9'))
    {
        *keyCode = (Uint16)wParam;
        return true;
    }

    switch (wParam)
    {
    case VK_BACK:
    case VK_SPACE:
    case VK_RETURN:
    case VK_TAB:
    case VK_ESCAPE:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_INSERT:
    case VK_HOME:
    case VK_END:
    case VK_DELETE:
    case VK_PRIOR:
    case VK_NEXT:
        *keyCode = (Uint16)wParam;
        return true;
    case VK_OEM_4:
        *keyCode = KEY_LEFT_BRACKET;
        return true;
    case VK_OEM_6:
        *keyCode = KEY_RIGHT_BRACKET;
        return true;
    case VK_OEM_COMMA:
        *keyCode = KEY_COMMA;
        return true;
    case VK_OEM_PERIOD:
        *keyCode = KEY_DOT;
        return true;
    case VK_OEM_2:
        *keyCode = KEY_SLASH;
        return true;
    case VK_OEM_1:
        *keyCode = KEY_SEMICOLON;
        return true;
    case VK_OEM_7:
        *keyCode = KEY_QUOTE;
        return true;
    case VK_OEM_5:
        *keyCode = KEY_BACK_SLASH;
        return true;
    case VK_OEM_MINUS:
        *keyCode = KEY_MINUS;
        return true;
    case VK_OEM_PLUS:
        *keyCode = KEY_EQUALS;
        return true;
    case VK_OEM_3:
        *keyCode = KEY_BACKQUOTE;
        return true;
    default:
        return false;
    }
}

static bool KeyCodeToLiteralText(Uint16 keyCode, std::wstring *text)
{
    if (text == NULL)
    {
        return false;
    }

    text->clear();

    const bool shift = GetKeyState(VK_SHIFT) < 0;
    const bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
    const bool isLetter = keyCode >= KEY_A && keyCode <= KEY_Z;
    Uint32 literalKey = keyCode;

    if ((isLetter && shift != caps) || (!isLetter && shift))
    {
        literalKey |= CAPS_MASK;
    }

    Uint16 ch = keyCodeToCharacter(literalKey);
    if (ch == 0 && literalKey != keyCode)
    {
        ch = keyCodeToCharacter(keyCode);
    }
    if (ch == 0)
    {
        return false;
    }

    text->push_back((wchar_t)ch);
    return true;
}

static void AppendRestoreLiteral(Uint16 keyCode, std::wstring *text)
{
    std::wstring literal;
    if (KeyCodeToLiteralText(keyCode, &literal))
    {
        text->append(literal);
    }
}

class ProcessingKeyGuard
{
public:
    explicit ProcessingKeyGuard(BOOL *processingKey) :
        _processingKey(processingKey)
    {
        *_processingKey = TRUE;
    }

    ~ProcessingKeyGuard()
    {
        *_processingKey = FALSE;
    }

private:
    BOOL *_processingKey;
};

static bool CodePointsToString(const std::vector<Uint32> &codePoints, std::wstring *text)
{
    if (text == NULL)
    {
        return false;
    }

    text->clear();
    for (size_t i = 0; i < codePoints.size(); ++i)
    {
        Uint32 data = codePoints[i];
        if (data & PURE_CHARACTER_MASK)
        {
            text->push_back((wchar_t)(data & CHAR_MASK));
            continue;
        }

        if (!(data & CHAR_CODE_MASK))
        {
            Uint16 ch = keyCodeToCharacter(data);
            if (ch == 0)
            {
                return false;
            }
            text->push_back((wchar_t)ch);
            continue;
        }

        Uint16 ch = (Uint16)(data & CHAR_MASK);
        if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4)
        {
            BYTE low = LOBYTE(ch);
            BYTE high = HIBYTE(ch);
            text->push_back((wchar_t)low);
            if (high > 32)
            {
                text->push_back((wchar_t)high);
            }
        }
        else if (vCodeTable == 3)
        {
            Uint16 markIndex = ch >> 13;
            ch &= 0x1FFF;
            text->push_back((wchar_t)ch);
            if (markIndex > 0)
            {
                text->push_back((wchar_t)_unicodeCompoundMark[markIndex - 1]);
            }
        }
        else
        {
            text->push_back((wchar_t)ch);
        }
    }
    return true;
}

COpenKeyTIP::COpenKeyTIP() :
    _cRef(1),
    _pThreadMgr(NULL),
    _pKeystrokeMgr(NULL),
    _pThreadMgrEventSource(NULL),
    _tfClientId(TF_CLIENTID_NULL),
    _displayAttributeAtom(TF_INVALID_GUIDATOM),
    _dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE),
    _composition(NULL),
    _fKeyEventSinkAdvised(FALSE),
    _fThreadMgrEventSinkAdvised(FALSE),
    _fProcessingKey(FALSE),
    _fDisabledForApp(FALSE)
{
    InterlockedIncrement(&g_cDllRef);
}

COpenKeyTIP::~COpenKeyTIP()
{
    Deactivate();
    InterlockedDecrement(&g_cDllRef);
}

STDAPI COpenKeyTIP::QueryInterface(REFIID riid, void **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    {
        *ppv = static_cast<ITfTextInputProcessorEx *>(this);
    }
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
    {
        *ppv = static_cast<ITfCompositionSink *>(this);
    }
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    {
        *ppv = static_cast<ITfKeyEventSink *>(this);
    }
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppv = static_cast<ITfThreadMgrEventSink *>(this);
    }
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    {
        *ppv = static_cast<ITfDisplayAttributeProvider *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDAPI_(ULONG) COpenKeyTIP::AddRef()
{
    return (ULONG)InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) COpenKeyTIP::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return (ULONG)cRef;
}

STDAPI COpenKeyTIP::Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId)
{
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

STDAPI COpenKeyTIP::ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD)
{
    Deactivate();

    _pThreadMgr = pThreadMgr;
    _tfClientId = tfClientId;

    if (_pThreadMgr != NULL)
    {
        _pThreadMgr->AddRef();
    }

    HRESULT hr = _engine.Initialize();
    if (FAILED(hr))
    {
        Deactivate();
        return hr;
    }

    if (_composition == NULL)
    {
        _composition = new (std::nothrow) CCompositionManager(static_cast<ITfCompositionSink *>(this));
        if (_composition == NULL)
        {
            Deactivate();
            return E_OUTOFMEMORY;
        }
    }

    ITfCategoryMgr *categoryMgr = NULL;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER,
        IID_ITfCategoryMgr, (void **)&categoryMgr);
    if (SUCCEEDED(hr) && categoryMgr != NULL)
    {
        if (FAILED(categoryMgr->RegisterGUID(GUID_OpenKeyDisplayAttr, &_displayAttributeAtom)))
        {
            _displayAttributeAtom = TF_INVALID_GUIDATOM;
        }
        categoryMgr->Release();
    }
    else
    {
        _displayAttributeAtom = TF_INVALID_GUIDATOM;
    }
    _composition->SetDisplayAttributeAtom(_displayAttributeAtom);

    if (_pThreadMgr != NULL)
    {
        hr = _pThreadMgr->QueryInterface(IID_ITfSource, (void **)&_pThreadMgrEventSource);
        if (FAILED(hr))
        {
            Deactivate();
            return hr;
        }

        hr = _pThreadMgrEventSource->AdviseSink(IID_ITfThreadMgrEventSink,
            static_cast<ITfThreadMgrEventSink *>(this),
            &_dwThreadMgrEventSinkCookie);
        if (FAILED(hr))
        {
            Deactivate();
            return hr;
        }
        _fThreadMgrEventSinkAdvised = TRUE;

        hr = _pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&_pKeystrokeMgr);
        if (FAILED(hr))
        {
            Deactivate();
            return hr;
        }

        hr = _pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, static_cast<ITfKeyEventSink *>(this), TRUE);
        if (FAILED(hr))
        {
            Deactivate();
            return hr;
        }
        _fKeyEventSinkAdvised = TRUE;
    }

    return S_OK;
}

STDAPI COpenKeyTIP::Deactivate()
{
    if (_pThreadMgrEventSource != NULL)
    {
        if (_fThreadMgrEventSinkAdvised)
        {
            _pThreadMgrEventSource->UnadviseSink(_dwThreadMgrEventSinkCookie);
            _fThreadMgrEventSinkAdvised = FALSE;
        }
        _pThreadMgrEventSource->Release();
        _pThreadMgrEventSource = NULL;
    }
    _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;

    if (_pKeystrokeMgr != NULL)
    {
        if (_fKeyEventSinkAdvised)
        {
            _pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);
            _fKeyEventSinkAdvised = FALSE;
        }
        _pKeystrokeMgr->Release();
        _pKeystrokeMgr = NULL;
    }

    if (_composition != NULL)
    {
        _composition->Release();
        _composition = NULL;
    }

    if (_pThreadMgr != NULL)
    {
        _pThreadMgr->Release();
        _pThreadMgr = NULL;
    }

    _tfClientId = TF_CLIENTID_NULL;
    _displayAttributeAtom = TF_INVALID_GUIDATOM;
    _fProcessingKey = FALSE;
    _fDisabledForApp = FALSE;
    return S_OK;
}

STDAPI COpenKeyTIP::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum)
{
    if (ppEnum == NULL)
    {
        return E_INVALIDARG;
    }

    *ppEnum = new (std::nothrow) CEnumOpenKeyDisplayAttributeInfo();
    return *ppEnum != NULL ? S_OK : E_OUTOFMEMORY;
}

STDAPI COpenKeyTIP::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo **ppInfo)
{
    if (ppInfo == NULL)
    {
        return E_INVALIDARG;
    }

    *ppInfo = NULL;
    if (!IsEqualGUID(guid, GUID_OpenKeyDisplayAttr))
    {
        return E_INVALIDARG;
    }

    *ppInfo = new (std::nothrow) COpenKeyDisplayAttributeInfo();
    return *ppInfo != NULL ? S_OK : E_OUTOFMEMORY;
}

STDAPI COpenKeyTIP::OnCompositionTerminated(TfEditCookie, ITfComposition *pComposition)
{
    if (_composition != NULL)
    {
        _composition->ClearComposition(pComposition);
    }
    return S_OK;
}

STDAPI COpenKeyTIP::OnSetFocus(BOOL fForeground)
{
    if (fForeground)
    {
        ApplySmartSwitchForForegroundApp();
    }
    return S_OK;
}

STDAPI COpenKeyTIP::OnTestKeyDown(ITfContext *, WPARAM wParam, LPARAM, BOOL *pfEaten)
{
    if (pfEaten == NULL)
    {
        return E_INVALIDARG;
    }

    *pfEaten = FALSE;
    if (_fDisabledForApp || !IsRuntimeEnabled() || IsFallbackInputMessage())
    {
        return S_OK;
    }

    if (IsControlModified())
    {
        *pfEaten = !IsModifierKey(wParam) ? TRUE : FALSE;
        return S_OK;
    }

    Uint16 keyCode = 0;
    *pfEaten = TranslateKey(wParam, &keyCode) ? TRUE : FALSE;
    return S_OK;
}

STDAPI COpenKeyTIP::OnTestKeyUp(ITfContext *, WPARAM, LPARAM, BOOL *pfEaten)
{
    if (pfEaten == NULL)
    {
        return E_INVALIDARG;
    }

    *pfEaten = FALSE;
    return S_OK;
}

STDAPI COpenKeyTIP::OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM, BOOL *pfEaten)
{
    if (pfEaten == NULL)
    {
        return E_INVALIDARG;
    }

    *pfEaten = FALSE;
    if (pic == NULL || _fDisabledForApp || !IsRuntimeEnabled() || _fProcessingKey || IsFallbackInputMessage())
    {
        return S_OK;
    }

    if (IsControlModified())
    {
        if (!IsModifierKey(wParam))
        {
            ResetSessionState(pic);
        }
        return S_OK;
    }

    Uint16 keyCode = 0;
    if (!TranslateKey(wParam, &keyCode))
    {
        return S_OK;
    }

    ProcessingKeyGuard guard(&_fProcessingKey);
    HRESULT hr = S_OK;

    try
    {
        vKeyHookState *state = _engine.ProcessKey(keyCode, GetCapsStatus(), false);
        vEngineEditOp op = vBuildEditOpFromHookState(state);
        bool isTransitory = IsTransitoryContext(pic);

        switch (op.type)
        {
        case vEngineEditOpNone:
        {
            if (keyCode == KEY_DELETE)
            {
                std::wstring text;
                if (isTransitory)
                {
                    hr = FallbackSendOutput(text, 1);
                }
                else
                {
                    hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, 1);
                }
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
                break;
            }

            std::wstring text;
            if (!KeyCodeToLiteralText(keyCode, &text))
            {
                break;
            }
            if (isTransitory)
            {
                hr = FallbackSendOutput(text, 0);
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
            }
            else
            {
                hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, 0);
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
            }
            break;
        }
        case vEngineEditOpReplaceText:
        {
            std::wstring text;
            if (!CodePointsToString(op.text, &text))
            {
                hr = E_FAIL;
                break;
            }
            if (isTransitory)
            {
                hr = FallbackSendOutput(text, op.backspaceCount);
                *pfEaten = TRUE;
            }
            else
            {
                hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, op.backspaceCount);
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
            }
            break;
        }
        case vEngineEditOpRestoreText:
        {
            std::wstring text;
            if (!CodePointsToString(op.text, &text))
            {
                hr = E_FAIL;
                break;
            }
            AppendRestoreLiteral(keyCode, &text);
            if (isTransitory)
            {
                hr = FallbackSendOutput(text, op.backspaceCount);
                *pfEaten = TRUE;
            }
            else
            {
                hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, op.backspaceCount);
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
            }
            break;
        }
        case vEngineEditOpMacro:
        {
            std::wstring text;
            if (!CodePointsToString(op.macroText, &text))
            {
                hr = E_FAIL;
                break;
            }
            if (isTransitory)
            {
                hr = FallbackSendOutput(text, op.backspaceCount);
                *pfEaten = TRUE;
            }
            else
            {
                hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, op.backspaceCount);
            }
            break;
        }
        case vEngineEditOpRestoreAndStartNewSession:
        {
            std::wstring text;
            if (!CodePointsToString(op.text, &text))
            {
                hr = E_FAIL;
                break;
            }
            AppendRestoreLiteral(keyCode, &text);
            if (isTransitory)
            {
                hr = FallbackSendOutput(text, op.backspaceCount);
                *pfEaten = TRUE;
            }
            else
            {
                hr = RequestEditSession(pic, CEditSession::OperationReplaceLeftText, text, op.backspaceCount);
                if (SUCCEEDED(hr))
                {
                    *pfEaten = TRUE;
                }
            }
            if (SUCCEEDED(hr))
            {
                _engine.Reset();
            }
            break;
        }
        case vEngineEditOpBreakWord:
            if (_composition != NULL && _composition->HasComposition())
            {
                hr = RequestEditSession(pic, CEditSession::OperationClearComposition, std::wstring());
            }
            break;
        default:
            break;
        }
    }
    catch (const std::bad_alloc &)
    {
        _engine.Reset();
        if (_composition != NULL)
        {
            _composition->ClearComposition();
        }
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        _engine.Reset();
        if (_composition != NULL)
        {
            _composition->ClearComposition();
        }
        return E_FAIL;
    }

    if (FAILED(hr))
    {
        _engine.Reset();
        if (_composition != NULL)
        {
            _composition->ClearComposition();
        }
    }

    return S_OK;
}

STDAPI COpenKeyTIP::OnKeyUp(ITfContext *, WPARAM, LPARAM, BOOL *pfEaten)
{
    if (pfEaten == NULL)
    {
        return E_INVALIDARG;
    }

    *pfEaten = FALSE;
    return S_OK;
}

STDAPI COpenKeyTIP::OnPreservedKey(ITfContext *, REFGUID, BOOL *pfEaten)
{
    if (pfEaten == NULL)
    {
        return E_INVALIDARG;
    }

    *pfEaten = FALSE;
    return S_OK;
}

STDAPI COpenKeyTIP::OnInitDocumentMgr(ITfDocumentMgr *)
{
    return S_OK;
}

STDAPI COpenKeyTIP::OnUninitDocumentMgr(ITfDocumentMgr *)
{
    return S_OK;
}

STDAPI COpenKeyTIP::OnSetFocus(ITfDocumentMgr *, ITfDocumentMgr *)
{
    ApplySmartSwitchForForegroundApp();
    return S_OK;
}

STDAPI COpenKeyTIP::OnPushContext(ITfContext *)
{
    return S_OK;
}

STDAPI COpenKeyTIP::OnPopContext(ITfContext *)
{
    return S_OK;
}

void COpenKeyTIP::ApplySmartSwitchForForegroundApp()
{
    try
    {
        _fDisabledForApp = FALSE;
        _engine.ReloadConfig();

        std::string exe = GetForegroundExecutableName();
        if (exe.empty() || lstrcmpiA(exe.c_str(), "explorer.exe") == 0)
        {
            _engine.Reset();
            return;
        }

        if (vUseSmartSwitchKey || vRememberCode)
        {
            int status = getAppInputMethodStatus(exe, makeAppInputMethodStatus(vLanguage, vCodeTable));
            if (status != -1 && vUseSmartSwitchKey)
            {
                vLanguage = status & 1;
            }
            if (status != -1 && vRememberCode)
            {
                vCodeTable = status >> 1;
            }
        }

        if (getAppInputMode(exe) == vAppInputModeDisabled)
        {
            _fDisabledForApp = TRUE;
        }

        if (_fDisabledForApp && _composition != NULL && _composition->HasComposition())
        {
            _composition->ClearComposition();
        }
        _engine.Reset();
    }
    catch (...)
    {
        _fDisabledForApp = FALSE;
        _engine.Reset();
        if (_composition != NULL && _composition->HasComposition())
        {
            _composition->ClearComposition();
        }
    }
}

HRESULT COpenKeyTIP::RequestEditSession(ITfContext *context, CEditSession::Operation operation, const std::wstring &text)
{
    return RequestEditSession(context, operation, text, 0);
}

HRESULT COpenKeyTIP::RequestEditSession(ITfContext *context, CEditSession::Operation operation, const std::wstring &text, LONG cchDelete)
{
    if (context == NULL || _composition == NULL)
    {
        return E_INVALIDARG;
    }

    CEditSession *session = new (std::nothrow) CEditSession(_composition, context, operation, text, cchDelete);
    if (session == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hrSession = E_FAIL;
    HRESULT hr = context->RequestEditSession(_tfClientId,
        session,
        TF_ES_SYNC | TF_ES_READWRITE,
        &hrSession);
    session->Release();

    return FAILED(hr) ? hr : hrSession;
}

bool COpenKeyTIP::IsRuntimeEnabled()
{
    if (_engine.IsTsfModeActive())
    {
        return true;
    }

    _engine.Reset();
    if (_composition != NULL && _composition->HasComposition())
    {
        _composition->ClearComposition();
    }
    return false;
}

void COpenKeyTIP::ResetSessionState(ITfContext *context)
{
    _engine.Reset();
    if (context != NULL && _composition != NULL && _composition->HasComposition())
    {
        RequestEditSession(context, CEditSession::OperationClearComposition, std::wstring());
    }
}
