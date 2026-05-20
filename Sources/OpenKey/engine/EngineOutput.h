//
//  EngineOutput.h
//  OpenKey
//

#pragma once

#include "DataType.h"
#include <vector>

using namespace std;

enum vEngineEditOpType {
    vEngineEditOpNone = 0,
    vEngineEditOpReplaceText,
    vEngineEditOpBreakWord,
    vEngineEditOpRestoreText,
    vEngineEditOpMacro,
    vEngineEditOpRestoreAndStartNewSession
};

struct vEngineEditOp {
    vEngineEditOpType type = vEngineEditOpNone;
    Byte backspaceCount = 0;
    vector<Uint32> text;
    vector<Uint32> macroKey;
    vector<Uint32> macroText;
    Byte extCode = 0;
};

vEngineEditOp vBuildEditOpFromHookState(const vKeyHookState* state);
bool vIsWordBreakKey(const Uint16& keyCode);
