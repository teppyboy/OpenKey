#include "globals.h"

HINSTANCE g_hInst = NULL;
LONG g_cDllRef = 0;

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
    }

    return TRUE;
}
