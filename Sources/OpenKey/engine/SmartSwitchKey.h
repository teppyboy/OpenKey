//
//  SmartSwitchKey.h
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef SmartSwitchKey_h
#define SmartSwitchKey_h

#include "DataType.h"
#include <string>

using namespace std;

enum vAppInputMode {
    vAppInputModeDefault = 0,
    vAppInputModeDisabled = 1,
};

int makeAppInputMethodStatus(const int& language, const int& codeTable, const int& appInputMode = vAppInputModeDefault);

void initSmartSwitchKey(const Byte* pData, const int& size);

/**
 * convert all data to save on disk
 */
void getSmartSwitchKeySaveData(vector<Byte>& outData);

/**
 * Find saved input method state for this app. The stored value currently packs
 * language in bit 0 and code table in the remaining bits.
 * If this app is missing, save @currentInputMethod for next time.
 * return:
 * -1: don't have this bundleId
 * otherwise: packed input method state
 */
int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod);

int getAppInputMode(const string& bundleId);

/**
 * Set packed input method state for this @bundleId.
 */
void setAppInputMethodStatus(const string& bundleId, const int& language);

void setAppInputMode(const string& bundleId, const int& appInputMode);

#endif /* SmartSwitchKey_h */
