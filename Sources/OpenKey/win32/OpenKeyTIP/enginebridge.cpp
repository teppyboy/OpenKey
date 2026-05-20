#include "enginebridge.h"

#include "../../engine/Engine.h"
#include "../../engine/Macro.h"
#include "../../engine/SmartSwitchKey.h"

#include <limits>
#include <vector>

using std::vector;

static const wchar_t kOpenKeyRegistryRoot[] = L"SOFTWARE\\TuyenMai\\OpenKey";
static const DWORD kMaxMacroDataSize = 1024 * 1024;
static const DWORD kMaxSmartSwitchKeySize = 256 * 1024;

// Engine state is global in the shared engine, so TIP bridge instances share one
// process-wide engine configuration until the engine supports instance state.
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vSwitchKeyStatus = 0x7A000206;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 1;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 0;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 1;
int vOtherLanguage = 1;
int vTempOffOpenKey = 0;

static DWORD ReadRegDword(const wchar_t* name, DWORD defaultValue)
{
	DWORD value = defaultValue;
	DWORD valueSize = sizeof(value);
	DWORD type = 0;

	LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
		kOpenKeyRegistryRoot,
		name,
		RRF_RT_REG_DWORD,
		&type,
		&value,
		&valueSize);

	return status == ERROR_SUCCESS ? value : defaultValue;
}

static int ReadRegEnum(const wchar_t* name, int defaultValue, int minValue, int maxValue)
{
	DWORD value = ReadRegDword(name, static_cast<DWORD>(defaultValue));
	return value >= static_cast<DWORD>(minValue) && value <= static_cast<DWORD>(maxValue)
		? static_cast<int>(value)
		: defaultValue;
}

static int ReadRegBool(const wchar_t* name, int defaultValue)
{
	DWORD value = ReadRegDword(name, static_cast<DWORD>(defaultValue));
	return value == 0 || value == 1 ? static_cast<int>(value) : defaultValue;
}

static int ReadRegSwitchKeyStatus(const wchar_t* name, int defaultValue)
{
	const DWORD knownMask = 0xFF008FFF;
	DWORD value = ReadRegDword(name, static_cast<DWORD>(defaultValue));
	// Legacy code stores this packed Win32 hotkey bitfield in signed int;
	// high-bit key patterns are intentional and must be preserved.
	return (value & ~knownMask) == 0
		? static_cast<int>(value)
		: defaultValue;
}

static bool ReadRegBinary(const wchar_t* name, DWORD maxSize, vector<Byte>& out)
{
	out.clear();

	DWORD valueSize = 0;
	LSTATUS status = RegGetValueW(HKEY_CURRENT_USER,
		kOpenKeyRegistryRoot,
		name,
		RRF_RT_REG_BINARY,
		nullptr,
		nullptr,
		&valueSize);
	if (status != ERROR_SUCCESS || valueSize == 0) {
		return false;
	}
	if (valueSize > maxSize || valueSize > static_cast<DWORD>((std::numeric_limits<int>::max)())) {
		return false;
	}

	out.resize(valueSize);
	status = RegGetValueW(HKEY_CURRENT_USER,
		kOpenKeyRegistryRoot,
		name,
		RRF_RT_REG_BINARY,
		nullptr,
		out.data(),
		&valueSize);
	if (status != ERROR_SUCCESS) {
		out.clear();
		return false;
	}

	out.resize(valueSize);
	return true;
}

static int BinarySizeToInt(const vector<Byte>& data)
{
	return data.size() <= static_cast<size_t>((std::numeric_limits<int>::max)())
		? static_cast<int>(data.size())
		: 0;
}

static void InitMacroMapSafely(const vector<Byte>& data)
{
	try {
		initMacroMap(data.empty() ? nullptr : data.data(), BinarySizeToInt(data));
	} catch (...) {
		initMacroMap(nullptr, 0);
	}
}

static void InitSmartSwitchKeySafely(const vector<Byte>& data)
{
	try {
		initSmartSwitchKey(data.empty() ? nullptr : data.data(), BinarySizeToInt(data));
	} catch (...) {
		initSmartSwitchKey(nullptr, 0);
	}
}

COpenKeyEngineBridge::COpenKeyEngineBridge()
	: _initialized(false), _hookState(nullptr)
{
}

COpenKeyEngineBridge::~COpenKeyEngineBridge()
{
}

HRESULT COpenKeyEngineBridge::Initialize()
{
	ReloadConfig();
	_hookState = static_cast<vKeyHookState*>(vKeyInit());
	_initialized = _hookState != nullptr;
	return _initialized ? S_OK : E_FAIL;
}

void COpenKeyEngineBridge::ReloadConfig()
{
	vLanguage = ReadRegEnum(L"vLanguage", 1, 0, 1);
	vInputType = ReadRegEnum(L"vInputType", 0, 0, 3);
	vFreeMark = 0;
	vCodeTable = ReadRegEnum(L"vCodeTable", 0, 0, 4);
	vCheckSpelling = ReadRegBool(L"vCheckSpelling", 1);
	vUseModernOrthography = ReadRegBool(L"vUseModernOrthography", 0);
	vQuickTelex = ReadRegBool(L"vQuickTelex", 0);
	vSwitchKeyStatus = ReadRegSwitchKeyStatus(L"vSwitchKeyStatus", 0x7A000206);
	vRestoreIfWrongSpelling = ReadRegBool(L"vRestoreIfWrongSpelling", 1);
	vFixRecommendBrowser = ReadRegBool(L"vFixRecommendBrowser", 1);
	vUseMacro = ReadRegBool(L"vUseMacro", 1);
	vUseMacroInEnglishMode = ReadRegBool(L"vUseMacroInEnglishMode", 0);
	vAutoCapsMacro = ReadRegBool(L"vAutoCapsMacro", 0);
	vUseSmartSwitchKey = ReadRegBool(L"vUseSmartSwitchKey", 1);
	vUpperCaseFirstChar = ReadRegBool(L"vUpperCaseFirstChar", 0);
	vAllowConsonantZFWJ = ReadRegBool(L"vAllowConsonantZFWJ", 0);
	vTempOffSpelling = ReadRegBool(L"vTempOffSpelling", 0);
	vQuickStartConsonant = ReadRegBool(L"vQuickStartConsonant", 0);
	vQuickEndConsonant = ReadRegBool(L"vQuickEndConsonant", 0);
	vRememberCode = ReadRegBool(L"vRememberCode", 1);
	vOtherLanguage = ReadRegBool(L"vOtherLanguage", 1);
	vTempOffOpenKey = ReadRegBool(L"vTempOffOpenKey", 0);

	vector<Byte> data;
	ReadRegBinary(L"macroData", kMaxMacroDataSize, data);
	InitMacroMapSafely(data);

	ReadRegBinary(L"smartSwitchKey", kMaxSmartSwitchKeySize, data);
	InitSmartSwitchKeySafely(data);
}

void COpenKeyEngineBridge::Reset()
{
	if (_initialized) {
		startNewSession();
	}
}

vKeyHookState* COpenKeyEngineBridge::ProcessKey(Uint16 keyCode, Uint8 capsStatus, bool otherControlKey)
{
	if (!_initialized) {
		return nullptr;
	}

	vKeyHandleEvent(Keyboard,
		KeyDown,
		keyCode,
		capsStatus,
		otherControlKey);
	return _hookState;
}

bool COpenKeyEngineBridge::IsInitialized() const
{
	return _initialized;
}
