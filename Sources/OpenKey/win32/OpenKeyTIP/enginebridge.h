#pragma once

#include "globals.h"
#include "../../engine/DataType.h"

class COpenKeyEngineBridge {
public:
	COpenKeyEngineBridge();
	~COpenKeyEngineBridge();
	HRESULT Initialize();
	void ReloadConfig();
	void Reset();
	vKeyHookState* ProcessKey(Uint16 keyCode, Uint8 capsStatus, bool otherControlKey);
	bool IsTsfModeActive() const;
	bool IsInitialized() const;

private:
	bool _initialized;
	vKeyHookState* _hookState;
};
