#include "fallback.h"

#include <windows.h>
#include <new>
#include <vector>

static const ULONG_PTR OPENKEY_TIP_FALLBACK_EXTRA_INFO = 0x4F4B545346424BULL;

bool IsTransitoryContext(ITfContext *context)
{
    if (context == NULL)
    {
        return false;
    }

    try
    {
        TF_STATUS status = {};
        if (FAILED(context->GetStatus(&status)))
        {
            return false;
        }
        return (status.dwStaticFlags & TF_SS_TRANSITORY) != 0;
    }
    catch (...)
    {
        return false;
    }
}

bool IsFallbackInputMessage()
{
    return (ULONG_PTR)GetMessageExtraInfo() == OPENKEY_TIP_FALLBACK_EXTRA_INFO;
}

static void PushKey(std::vector<INPUT> *inputs, WORD vk, WORD scan, DWORD flags)
{
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = scan;
    input.ki.dwFlags = flags;
    input.ki.dwExtraInfo = OPENKEY_TIP_FALLBACK_EXTRA_INFO;
    inputs->push_back(input);
}

static void PushUnicodeChar(std::vector<INPUT> *inputs, WORD ch)
{
    PushKey(inputs, 0, ch, KEYEVENTF_UNICODE);
    PushKey(inputs, 0, ch, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP);
}

static HRESULT SendInputBatch(std::vector<INPUT> *inputs)
{
    if (inputs->empty())
    {
        return S_OK;
    }

    SetLastError(ERROR_GEN_FAILURE);
    UINT sent = SendInput((UINT)inputs->size(), inputs->data(), sizeof(INPUT));
    if (sent != inputs->size())
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    inputs->clear();
    return S_OK;
}

HRESULT FallbackSendOutput(const std::wstring &text, BYTE backspaceCount, FallbackBackspaceMode backspaceMode)
{
    try
    {
        if (backspaceCount == 0 && text.empty())
        {
            return S_OK;
        }

        std::vector<INPUT> inputs;
        inputs.reserve(64);

        for (BYTE i = 0; i < backspaceCount; ++i)
        {
            if (backspaceMode == FallbackBackspaceUnicode)
            {
                // Some transitory hosts ignore synthetic VK_BACK but accept
                // Unicode packets, including U+0008 backspace.
                PushUnicodeChar(&inputs, VK_BACK);
            }
            else
            {
                WORD scan = (WORD)MapVirtualKeyW(VK_BACK, MAPVK_VK_TO_VSC);
                PushKey(&inputs, VK_BACK, scan, 0);
                PushKey(&inputs, VK_BACK, scan, KEYEVENTF_KEYUP);
            }
            if (inputs.size() >= 64)
            {
                HRESULT hr = SendInputBatch(&inputs);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
        }

        for (size_t i = 0; i < text.size(); ++i)
        {
            PushUnicodeChar(&inputs, (WORD)text[i]);
            if (inputs.size() >= 64)
            {
                HRESULT hr = SendInputBatch(&inputs);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
        }

        return SendInputBatch(&inputs);
    }
    catch (const std::bad_alloc &)
    {
        return E_OUTOFMEMORY;
    }
}
