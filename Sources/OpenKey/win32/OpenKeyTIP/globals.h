#pragma once

#include <Windows.h>

// {A942CCFA-976D-4D60-93AD-5CEBE269751F}
static const CLSID CLSID_OpenKeyTIP =
{ 0xa942ccfa, 0x976d, 0x4d60, { 0x93, 0xad, 0x5c, 0xeb, 0xe2, 0x69, 0x75, 0x1f } };

// {8BCB2F64-9491-4B57-866A-E2F39DBB0668}
static const GUID GUID_OpenKeyProfile =
{ 0x8bcb2f64, 0x9491, 0x4b57, { 0x86, 0x6a, 0xe2, 0xf3, 0x9d, 0xbb, 0x06, 0x68 } };

// {2AB9944E-68BE-46AF-877F-084356FC4AA9}
static const GUID GUID_OpenKeyDisplayAttr =
{ 0x2ab9944e, 0x68be, 0x46af, { 0x87, 0x7f, 0x08, 0x43, 0x56, 0xfc, 0x4a, 0xa9 } };

static const LANGID LANGID_OpenKeyVietnamese = 0x042A;
static const WCHAR OpenKeyDescription[] = L"OpenKey Vietnamese";

extern HINSTANCE g_hInst;
extern LONG g_cDllRef;
