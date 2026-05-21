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
#pragma once
#include "stdafx.h"

namespace TSFRegistrationHelper {
	bool isTIPRegistered();
	bool registerTIP(bool elevated);
	bool deactivateTIP();
	bool unregisterTIP(bool elevated);
	bool activateTIP();
	bool forceUnloadTIPFromAllProcesses();
	std::wstring getTIPDllPath();
}
