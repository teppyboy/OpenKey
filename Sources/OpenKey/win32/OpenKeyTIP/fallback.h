#pragma once

#include <msctf.h>
#include <string>

bool IsTransitoryContext(ITfContext *context);
bool IsFallbackInputMessage();

enum FallbackBackspaceMode
{
    FallbackBackspaceVirtualKey,
    FallbackBackspaceUnicode
};

HRESULT FallbackSendOutput(const std::wstring &text,
    BYTE backspaceCount,
    FallbackBackspaceMode backspaceMode = FallbackBackspaceVirtualKey);
