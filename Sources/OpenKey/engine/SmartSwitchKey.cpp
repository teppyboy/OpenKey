//
//  SmartSwitchKey.cpp
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "SmartSwitchKey.h"
#include <map>
#include <iostream>
#include <memory.h>

#define INPUT_METHOD_STATE_MASK 0x0F
#define APP_INPUT_MODE_SHIFT 4
#define APP_INPUT_MODE_MASK 0xF0

//main data, i use `map` because it has O(Log(n))
static map<string, Int8> _smartSwitchKeyData;
static string _cacheKey = ""; //use cache for faster
static Int8 _cacheData = 0; //use cache for faster

static Int8 getInputMethodState(const Int8& data) {
    return data & INPUT_METHOD_STATE_MASK;
}

static Int8 getAppInputModeState(const Int8& data) {
    return (data & APP_INPUT_MODE_MASK) >> APP_INPUT_MODE_SHIFT;
}

int makeAppInputMethodStatus(const int& language, const int& codeTable, const int& appInputMode) {
    return (language & 0x01) | ((codeTable & 0x07) << 1) | ((appInputMode & 0x0F) << APP_INPUT_MODE_SHIFT);
}

void initSmartSwitchKey(const Byte* pData, const int& size) {
    _smartSwitchKeyData.clear();
    _cacheKey.clear();
    _cacheData = 0;
    if (pData == NULL || size < 2) return;
    Uint16 count = 0;
    Uint32 cursor = 0;
    memcpy(&count, pData + cursor, 2);
    cursor+=2;

    for (int i = 0; i < count; i++) {
        if (cursor >= (Uint32)size) break;
        Uint8 bundleIdSize = pData[cursor++];
        if (cursor + bundleIdSize >= (Uint32)size) break;
        string bundleId((char*)pData + cursor, bundleIdSize);
        cursor += bundleIdSize;
        Uint8 value = pData[cursor++];
        _smartSwitchKeyData[bundleId] = value;
    }
}

void getSmartSwitchKeySaveData(vector<Byte>& outData) {
    outData.clear();
    outData.push_back(0);
    outData.push_back(0);
    Uint16 count = 0;
    
    for (std::map<string, Int8>::iterator it = _smartSwitchKeyData.begin(); it != _smartSwitchKeyData.end(); ++it) {
        if (it->first.length() > 255 || count == 0xFFFF) continue;
        outData.push_back((Byte)it->first.length());
        for (int j = 0; j < it->first.length(); j++) {
            outData.push_back(it->first[j]);
        }
        outData.push_back(it->second);
        count++;
    }

    outData[0] = (Byte)count;
    outData[1] = (Byte)(count>>8);
}

int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    if (_cacheKey.compare(bundleId) == 0) {
        return getInputMethodState(_cacheData);
    }
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        _cacheKey = bundleId;
        _cacheData = _smartSwitchKeyData[bundleId];
        return getInputMethodState(_cacheData);
    }
    _cacheKey = bundleId;
    _cacheData = currentInputMethod;
    _smartSwitchKeyData[bundleId] = _cacheData;
    return -1;
}

int getAppInputMode(const string& bundleId) {
    if (_cacheKey.compare(bundleId) == 0) {
        return getAppInputModeState(_cacheData);
    }
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        _cacheKey = bundleId;
        _cacheData = _smartSwitchKeyData[bundleId];
        return getAppInputModeState(_cacheData);
    }
    return vAppInputModeDefault;
}

void setAppInputMethodStatus(const string& bundleId, const int& language) {
    Int8 appInputMode = vAppInputModeDefault;
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        appInputMode = getAppInputModeState(_smartSwitchKeyData[bundleId]);
    }
    Int8 data = getInputMethodState((Int8)language) | (appInputMode << APP_INPUT_MODE_SHIFT);
    _smartSwitchKeyData[bundleId] = data;
    _cacheKey = bundleId;
    _cacheData = data;
}

void setAppInputMode(const string& bundleId, const int& appInputMode) {
    Int8 inputMethodState = 0;
    if (_smartSwitchKeyData.find(bundleId) != _smartSwitchKeyData.end()) {
        inputMethodState = getInputMethodState(_smartSwitchKeyData[bundleId]);
    }
    Int8 data = inputMethodState | ((appInputMode & 0x0F) << APP_INPUT_MODE_SHIFT);
    _smartSwitchKeyData[bundleId] = data;
    _cacheKey = bundleId;
    _cacheData = data;
}
