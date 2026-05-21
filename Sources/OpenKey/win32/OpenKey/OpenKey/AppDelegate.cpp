/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "AppDelegate.h"
#include "TSFRegistrationHelper.h"

static AppDelegate* _instance;

//see document in Engine.h
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
#define DEFAULT_SWITCH_STATUS 0x5A00025A //default option + z
int vSwitchKeyStatus = DEFAULT_SWITCH_STATUS;
int vRestoreIfWrongSpelling = 1;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 1;
int vAutoCapsMacro = 0;
int vSendKeyStepByStep = 1;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vOtherLanguage = 1;
int vRememberCode = 1;
int vTempOffOpenKey = 0;
int vUseTSFInput = 0;
int vForceUnloadTSFDLL = 0;

int vUseGrayIcon = 0;
int vShowOnStartUp = 0;
int vRunWithWindows = 1;

int vSupportMetroApp = 1;
int vCreateDesktopShortcut = 0;
int vRunAsAdmin = 0;
int vCheckNewVersion = 0;
//beta feature
int vFixChromiumBrowser = 0; //new on version 2.0

bool AppDelegate::isDialogMsg(MSG & msg) const {
	return (mainDialog != NULL && IsDialogMessage(mainDialog->getHwnd(), &msg)) ||
		(macroDialog != NULL && IsDialogMessage(macroDialog->getHwnd(), &msg)) || 
		(convertDialog != NULL && IsDialogMessage(convertDialog->getHwnd(), &msg)) || 
		(aboutDialog != NULL && IsDialogMessage(aboutDialog->getHwnd(), &msg));
}

void AppDelegate::checkUpdate() {
	string newVersion;
	if (OpenKeyManager::checkUpdate(newVersion)) {
		WCHAR msg[256];
		wsprintf(msg,
			TEXT("OpenKey Có phiên bản mới (%s), bạn có muốn cập nhật không?"),
			utf8ToWideString(newVersion).c_str());

		int msgboxID = MessageBox(
			0,
			msg,
			_T("OpenKey Update"),
			MB_ICONEXCLAMATION | MB_YESNO
		);
		if (msgboxID == IDYES) {
			//Call OpenKeyUpdate
			WCHAR path[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, path);
			wsprintf(path, TEXT("%s\\OpenKeyUpdate.exe"), path);
			ShellExecute(0, L"", path, 0, 0, SW_SHOWNORMAL);
			AppDelegate::getInstance()->onOpenKeyExit();
		}

	}
}

AppDelegate::AppDelegate() {
	_instance = this;
}

AppDelegate * AppDelegate::getInstance() {
	return _instance;
}

int AppDelegate::run(HINSTANCE hInstance) {
	DebugLogInit();
	DEBUG_LOG(L"AppDelegate::run hInstance=0x%p", hInstance);
	this->hInstance = hInstance;

	//check app has already run or not
	HWND previousInstance = FindWindow(APP_CLASS, NULL);
	if (previousInstance) {
		DEBUG_LOG(L"existing OpenKey instance detected hwnd=0x%p", previousInstance);
		MessageBeep(MB_OK);
		SendMessage(previousInstance, WM_USER + 2019, 0, 0);
		PostQuitMessage(0);
		DebugLogShutdown();
		return 0;
	}

	//init OpenKey Engine
	OpenKeyManager::initEngine();
	DEBUG_LOG(L"OpenKey engine initialized");
	if (!vUseTSFInput) {
		DEBUG_LOG(L"TSF disabled at startup: deactivate stale profile");
		TSFRegistrationHelper::deactivateTIP();
	}

	//create system tray
	SystemTrayHelper::createSystemTrayIcon(hInstance);
	SystemTrayHelper::updateData();
	DEBUG_LOG(L"system tray created and initialized");

	//create main control
	if (vShowOnStartUp)
		createMainDialog();
	MessageBeep(MB_OK);

	//check update
	if (vCheckNewVersion)
		checkUpdate();

	MSG msg;
	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))	{
		if (msg.message == WM_KEYDOWN) {
			OpenKeyManager::_lastKeyCode = (UINT16)msg.wParam;
		}
		if (!isDialogMsg(msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DEBUG_LOG(L"AppDelegate::run message loop exited");
	shutdown();
	DebugLogShutdown();
	return 0;
}

void AppDelegate::createMainDialog() {
	if (mainDialog == NULL) {
		mainDialog = new MainControlDialog(hInstance, IDD_DIALOG_MAIN);
		mainDialog->show();
	} else {
		mainDialog->bringOnTop();
	}
}

void AppDelegate::closeDialog(BaseDialog * dialog) {
	dialog->closeDialog();
	if (mainDialog == dialog) {
		delete mainDialog;
		mainDialog = NULL;
	} else if (aboutDialog == dialog) {
		delete aboutDialog;
		aboutDialog = NULL;
	} else if (macroDialog == dialog) {
		delete macroDialog;
		macroDialog = NULL;
	} else if (convertDialog == dialog) {
		delete convertDialog;
		convertDialog = NULL;
	}
}

void AppDelegate::onInputMethodChangedFromHotKey() {
	APP_SET_DATA(vLanguage, vLanguage);
	DEBUG_LOG(L"input method changed: vLanguage=%d vCodeTable=%d", vLanguage, vCodeTable);
	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onDefaultConfig() {
	DEBUG_LOG(L"reset default config requested");
	APP_SET_DATA(vLanguage, 1);
	APP_SET_DATA(vInputType, 0);
	vFreeMark = 0;
	APP_SET_DATA(vCodeTable, 0);
	APP_SET_DATA(vCheckSpelling, 1);
	APP_SET_DATA(vUseModernOrthography, 0);
	APP_SET_DATA(vQuickTelex, 0);
	APP_SET_DATA(vSwitchKeyStatus, DEFAULT_SWITCH_STATUS);
	APP_SET_DATA(vRestoreIfWrongSpelling, 1);
	APP_SET_DATA(vFixRecommendBrowser, 1);
	APP_SET_DATA(vUseMacro, 0);
	APP_SET_DATA(vUseMacroInEnglishMode, 0);
	APP_SET_DATA(vSendKeyStepByStep, 1);
	APP_SET_DATA(vUseSmartSwitchKey, 1);
	APP_SET_DATA(vUpperCaseFirstChar, 0);
	APP_SET_DATA(vAllowConsonantZFWJ, 0);
	APP_SET_DATA(vTempOffSpelling, 0);

	APP_SET_DATA(vUseGrayIcon, 0);
	APP_SET_DATA(vShowOnStartUp, 1);
	APP_SET_DATA(vRunWithWindows, 1);

	APP_SET_DATA(vSupportMetroApp, 1);
	APP_SET_DATA(vRememberCode, 1);
	APP_SET_DATA(vOtherLanguage, 1);
	APP_SET_DATA(vTempOffOpenKey, 0);
	APP_SET_DATA(vUseTSFInput, 0);
	APP_SET_DATA(vForceUnloadTSFDLL, 0);
	APP_SET_DATA(vFixChromiumBrowser, 0);

	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onToggleVietnamese() {
	APP_SET_DATA(vLanguage, vLanguage ? 0 : 1);
	DEBUG_LOG(L"toggle Vietnamese: vLanguage=%d", vLanguage);
	if (mainDialog) {
		mainDialog->fillData();
	}
	
	if (vUseSmartSwitchKey) {
		string& exe = OpenKeyHelper::getLastAppExecuteName();
		setAppInputMethodStatus(exe, makeAppInputMethodStatus(vLanguage, vCodeTable));
		saveSmartSwitchKeyData();
	}
}

void AppDelegate::onToggleCheckSpelling() {
	APP_SET_DATA(vCheckSpelling, vCheckSpelling ? 0 : 1);
	DEBUG_LOG(L"toggle spelling: vCheckSpelling=%d", vCheckSpelling);
	if (mainDialog) {
		mainDialog->fillData();
	}
	vSetCheckSpelling();
}

void AppDelegate::onToggleUseSmartSwitchKey() {
	APP_SET_DATA(vUseSmartSwitchKey, vUseSmartSwitchKey ? 0 : 1);
	DEBUG_LOG(L"toggle smart switch: vUseSmartSwitchKey=%d", vUseSmartSwitchKey);
	if (mainDialog) {
		mainDialog->fillData();
	}
}

void AppDelegate::onToggleCurrentAppDisabled() {
	DEBUG_LOG(L"toggle current app disabled: currentMode=%d", getCurrentAppInputMode());
	setCurrentAppInputMode(getCurrentAppInputMode() == vAppInputModeDisabled ? vAppInputModeDefault : vAppInputModeDisabled);
}

void AppDelegate::onToggleUseMacro() {
	APP_SET_DATA(vUseMacro, vUseMacro ? 0 : 1);
	DEBUG_LOG(L"toggle macro: vUseMacro=%d", vUseMacro);
	if (mainDialog) {
		mainDialog->fillData();
	}
}

void AppDelegate::onToggleUseTSFInput() {
	DEBUG_LOG(L"toggle TSF requested: current=%d registered=%d", vUseTSFInput, TSFRegistrationHelper::isTIPRegistered());
	if (vUseTSFInput) {
		APP_SET_DATA(vUseTSFInput, 0);
		DEBUG_LOG(L"TSF disabling: unregister TIP");
		if (!TSFRegistrationHelper::unregisterTIP(false)) {
			DEBUG_LOG(L"TSF unregister failed while disabling");
			MessageBox(NULL,
				_T("Cannot remove OpenKey IME from Windows."),
				_T("OpenKey IME"),
				MB_OK | MB_ICONERROR);
		}
	} else {
		DEBUG_LOG(L"TSF enabling: register and activate TIP");
		if (!TSFRegistrationHelper::registerTIP(false) || !TSFRegistrationHelper::activateTIP()) {
			DEBUG_LOG(L"TSF register or activate failed; unregister rollback");
			TSFRegistrationHelper::unregisterTIP(false);
			MessageBox(NULL,
				_T("Cannot install or activate OpenKey IME in Windows."),
				_T("OpenKey IME"),
				MB_OK | MB_ICONERROR);
			if (mainDialog) {
				mainDialog->fillData();
			}
			SystemTrayHelper::updateData();
			return;
		}
		APP_SET_DATA(vUseTSFInput, 1);
		DEBUG_LOG(L"TSF enabled");
	}

	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onRegisterTIP() {
	DEBUG_LOG(L"manual TIP register requested");
	bool success = TSFRegistrationHelper::registerTIP(false);
	DEBUG_LOG(L"manual TIP register result=%d", success ? 1 : 0);
	MessageBox(NULL,
		success ? _T("OpenKey IME was installed into Windows.") : _T("Cannot install OpenKey IME into Windows."),
		_T("OpenKey IME"),
		success ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
	SystemTrayHelper::updateData();
}

void AppDelegate::onUnregisterTIP() {
	DEBUG_LOG(L"manual TIP unregister requested");
	bool success = TSFRegistrationHelper::unregisterTIP(false);
	DEBUG_LOG(L"manual TIP unregister result=%d", success ? 1 : 0);
	if (success)
		APP_SET_DATA(vUseTSFInput, 0);
	MessageBox(NULL,
		success ? _T("OpenKey IME was removed from Windows.") : _T("Cannot remove OpenKey IME from Windows."),
		_T("OpenKey IME"),
		success ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onMacroTable() {
	if (macroDialog == NULL) {
		macroDialog = new MacroDialog(hInstance, IDD_DIALOG_MACRO);
		macroDialog->show();
	} else {
		macroDialog->bringOnTop();
	}
}

void AppDelegate::onConvertTool() {
	if (convertDialog == NULL) {
		convertDialog = new ConvertToolDialog(hInstance, IDD_DIALOG_CONVERT_TOOL);
		convertDialog->show();
	} else {
		convertDialog->bringOnTop();
	}
}

void AppDelegate::onQuickConvert() {
	if (OpenKeyHelper::quickConvert()) {
		//alert when complete
		if (!convertToolDontAlertWhenCompleted) {
			TCHAR msg[256];
			LoadString(hInstance, IDS_STRING_CONVERT_COMPLETED, msg, 256);
			MessageBox(NULL, msg, _T("OpenKey"), MB_OK);
		}
	}
}

void AppDelegate::onInputType(const int & type) {
	APP_SET_DATA(vInputType, type);
	DEBUG_LOG(L"input type changed: vInputType=%d", vInputType);
	if (mainDialog) {
		mainDialog->fillData();
	}
}

void AppDelegate::onTableCode(const int & code) {
	APP_SET_DATA(vCodeTable, code);
	DEBUG_LOG(L"table code changed: vCodeTable=%d", vCodeTable);
	if (mainDialog) {
		mainDialog->fillData();
	}
	if (vRememberCode) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), makeAppInputMethodStatus(vLanguage, vCodeTable));
		saveSmartSwitchKeyData();
	}
}

void AppDelegate::onControlPanel() {
	createMainDialog();
}

void AppDelegate::onOpenKeyAbout() {
	if (aboutDialog == NULL) {
		aboutDialog = new AboutDialog(hInstance, IDD_ABOUTBOX);
		aboutDialog->show();
	} else {
		aboutDialog->bringOnTop();
	}
}

void AppDelegate::shutdown() {
	if (isShuttingDown) {
		return;
	}
	isShuttingDown = true;

	// Free engine (uninstalls keyboard hook) and tray before TSF cleanup so the system
	// input hook is not blocked during the TSF deactivation delay.
	OpenKeyManager::freeEngine();
	SystemTrayHelper::removeSystemTray();

	DEBUG_LOG(L"OpenKey exit requested: vUseTSFInput=%d registered=%d", vUseTSFInput, TSFRegistrationHelper::isTIPRegistered());
	if (vUseTSFInput || TSFRegistrationHelper::isTIPRegistered()) {
		APP_SET_DATA(vUseTSFInput, 0);
		TSFRegistrationHelper::unregisterTIP(false);
		DEBUG_LOG(L"TIP unregistered during exit");
	}
}

void AppDelegate::onOpenKeyExit() {
	shutdown();
	PostQuitMessage(0);
}
