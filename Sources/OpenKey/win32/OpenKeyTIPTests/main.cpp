#include "..\..\engine\DataType.h"
#include "..\..\engine\Engine.h"
#include "..\..\engine\EngineOutput.h"

#include <stdio.h>

int vLanguage = 1;
int vInputType = vTelex;
int vFreeMark = 0;
int vCodeTable = 0;
int vSwitchKeyStatus = 0;
int vCheckSpelling = 0;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 0;
int vUseMacro = 0;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

static bool Fail(const char* message)
{
    printf("FAIL: %s\n", message);
    return false;
}

static bool TestNullHookState()
{
    vEngineEditOp op = vBuildEditOpFromHookState(nullptr);
    if (op.type != vEngineEditOpNone) {
        return Fail("null hook state produced edit op");
    }

    return true;
}

static bool TestWordBreakKeys()
{
    if (!vIsWordBreakKey(KEY_ENTER)) {
        return Fail("KEY_ENTER was not a word break");
    }
    if (vIsWordBreakKey(KEY_A)) {
        return Fail("KEY_A was a word break");
    }

    return true;
}

static bool TestTelexComposition()
{
    // vKeyInit returns a pointer to global engine state; do not free it.
    vKeyHookState* state = static_cast<vKeyHookState*>(vKeyInit());
    if (state == nullptr) {
        return Fail("vKeyInit returned null");
    }
    // Ensure clean engine state before injecting keystrokes.
    startNewSession();

    vKeyHandleEvent(Keyboard, KeyDown, KEY_A, 0, false);
    vKeyHandleEvent(Keyboard, KeyDown, KEY_S, 0, false);

    vEngineEditOp op = vBuildEditOpFromHookState(state);
    if (op.type != vEngineEditOpReplaceText && op.type != vEngineEditOpRestoreText) {
        return Fail("Telex a+s did not produce replace/restore edit op");
    }
    if (op.text.empty()) {
        return Fail("Telex a+s edit op had no text");
    }
    if (getCharacterCode(op.text[0]) == 0) {
        return Fail("Telex a+s converted character data was empty");
    }

    return true;
}

int main()
{
    if (!TestNullHookState()) {
        return 1;
    }
    if (!TestWordBreakKeys()) {
        return 1;
    }
    if (!TestTelexComposition()) {
        return 1;
    }

    printf("OpenKeyTIPTests passed\n");
    return 0;
}
