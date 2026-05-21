#include "..\..\engine\DataType.h"
#include "..\..\engine\Engine.h"
#include "..\..\engine\EngineOutput.h"
#include "..\..\engine\Vietnamese.h"

#include <stdio.h>
#include <string>

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

static bool AppendEditText(const std::vector<Uint32>& data, std::wstring* text)
{
    for (size_t i = 0; i < data.size(); i++) {
        Uint32 ch = data[i];
        if (ch & PURE_CHARACTER_MASK) {
            text->push_back((wchar_t)(ch & CHAR_MASK));
            continue;
        }
        if (!(ch & CHAR_CODE_MASK)) {
            Uint16 plain = keyCodeToCharacter(ch);
            if (plain == 0) {
                return false;
            }
            text->push_back((wchar_t)plain);
            continue;
        }
        text->push_back((wchar_t)(ch & CHAR_MASK));
    }
    return true;
}

static bool ApplyKey(vKeyHookState* state, Uint16 keyCode, std::wstring* text)
{
    vKeyHandleEvent(Keyboard, KeyDown, keyCode, 0, false);
    vEngineEditOp op = vBuildEditOpFromHookState(state);
    if (op.type == vEngineEditOpNone) {
        Uint16 plain = keyCodeToCharacter(keyCode);
        if (plain != 0) {
            text->push_back((wchar_t)plain);
        }
        return true;
    }
    if (op.type != vEngineEditOpReplaceText &&
        op.type != vEngineEditOpRestoreText &&
        op.type != vEngineEditOpRestoreAndStartNewSession) {
        return true;
    }
    if (op.backspaceCount > text->size()) {
        return false;
    }
    text->erase(text->size() - op.backspaceCount);
    return AppendEditText(op.text, text);
}

static bool TestTelexDdoasOrder()
{
    vKeyHookState* state = static_cast<vKeyHookState*>(vKeyInit());
    if (state == nullptr) {
        return Fail("vKeyInit returned null for ddoas test");
    }
    startNewSession();

    std::wstring text;
    if (!ApplyKey(state, KEY_D, &text) ||
        !ApplyKey(state, KEY_D, &text) ||
        !ApplyKey(state, KEY_O, &text) ||
        !ApplyKey(state, KEY_A, &text) ||
        !ApplyKey(state, KEY_S, &text)) {
        return Fail("ddoas edit sequence failed");
    }

    std::wstring expected;
    expected.push_back((wchar_t)0x0111);
    expected.push_back((wchar_t)0x00f3);
    expected.push_back(L'a');
    if (text != expected) {
        return Fail("ddoas did not produce expected character order");
    }

    return true;
}

static bool TestWindowsOemPunctuationKeys()
{
    if (keyCodeToCharacter(KEY_BACKQUOTE) != L'`') {
        return Fail("KEY_BACKQUOTE did not map to backquote");
    }
    if (keyCodeToCharacter(KEY_BACKQUOTE | CAPS_MASK) != L'~') {
        return Fail("KEY_BACKQUOTE shifted did not map to tilde");
    }
    if (keyCodeToCharacter(KEY_BACK_SLASH) != L'\\') {
        return Fail("KEY_BACK_SLASH did not map to backslash");
    }
    if (keyCodeToCharacter(KEY_BACK_SLASH | CAPS_MASK) != L'|') {
        return Fail("KEY_BACK_SLASH shifted did not map to pipe");
    }
    if (keyCodeToCharacter(KEY_QUOTE) != L'\'') {
        return Fail("KEY_QUOTE did not map to quote");
    }
    if (keyCodeToCharacter(KEY_QUOTE | CAPS_MASK) != L'\"') {
        return Fail("KEY_QUOTE shifted did not map to double quote");
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
    if (!TestTelexDdoasOrder()) {
        return 1;
    }
    if (!TestWindowsOemPunctuationKeys()) {
        return 1;
    }

    printf("OpenKeyTIPTests passed\n");
    return 0;
}
