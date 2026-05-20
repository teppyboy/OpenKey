//
//  EngineOutput.cpp
//  OpenKey
//

#include "EngineOutput.h"

static void vCopyHookText(vEngineEditOp& op, const vKeyHookState* state) {
    op.backspaceCount = state->backspaceCount;

    Byte count = state->newCharCount;
    if (count > MAX_BUFF) {
        count = MAX_BUFF;
    }

    op.text.reserve(count);
    for (Byte i = 0; i < count; i++) {
        op.text.push_back(state->charData[i]);
    }
}

vEngineEditOp vBuildEditOpFromHookState(const vKeyHookState* state) {
    vEngineEditOp op;
    op.type = vEngineEditOpNone;
    op.backspaceCount = 0;
    op.extCode = 0;

    if (state == nullptr) {
        return op;
    }

    op.extCode = state->extCode;

    switch (state->code) {
        case vDoNothing:
            break;
        case vWillProcess:
            op.type = vEngineEditOpReplaceText;
            vCopyHookText(op, state);
            break;
        case vBreakWord:
            op.type = vEngineEditOpBreakWord;
            break;
        case vRestore:
            op.type = vEngineEditOpRestoreText;
            vCopyHookText(op, state);
            break;
        case vReplaceMaro:
            op.type = vEngineEditOpMacro;
            vCopyHookText(op, state);
            op.macroKey = state->macroKey;
            op.macroText = state->macroData;
            break;
        case vRestoreAndStartNewSession:
            op.type = vEngineEditOpRestoreAndStartNewSession;
            vCopyHookText(op, state);
            break;
        default:
            break;
    }

    return op;
}

bool vIsWordBreakKey(const Uint16& keyCode) {
    return keyCode == KEY_ESC ||
           keyCode == KEY_TAB ||
           keyCode == KEY_ENTER ||
           keyCode == KEY_RETURN ||
           keyCode == KEY_LEFT ||
           keyCode == KEY_RIGHT ||
           keyCode == KEY_DOWN ||
           keyCode == KEY_UP ||
           keyCode == KEY_COMMA ||
           keyCode == KEY_DOT ||
           keyCode == KEY_SLASH ||
           keyCode == KEY_SEMICOLON ||
           keyCode == KEY_QUOTE ||
           keyCode == KEY_BACK_SLASH ||
           keyCode == KEY_MINUS ||
           keyCode == KEY_EQUALS ||
           keyCode == KEY_BACKQUOTE
#if _WIN32
           || keyCode == VK_INSERT
           || keyCode == VK_HOME
           || keyCode == VK_END
           || keyCode == VK_DELETE
           || keyCode == VK_PRIOR
           || keyCode == VK_NEXT
           || keyCode == VK_SNAPSHOT
           || keyCode == VK_PRINT
           || keyCode == VK_SELECT
           || keyCode == VK_HELP
           || keyCode == VK_EXECUTE
           || keyCode == VK_NUMLOCK
           || keyCode == VK_SCROLL
#endif
           ;
}
