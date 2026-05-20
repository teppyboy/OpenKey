#pragma once

#include <msctf.h>
#include <string>

bool IsTransitoryContext(ITfContext *context);
bool IsFallbackInputMessage();
HRESULT FallbackSendOutput(const std::wstring &text, BYTE backspaceCount);
