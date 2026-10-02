#pragma comment(linker, "/export:DisplayHelloWorld=GenericDll2.DisplayHelloWorld")

#include "pch.h"
#include <Windows.h>

extern "C" {
    __declspec(dllexport) void HelloWorld()
    {
        MessageBoxA(NULL, "Hello da funcao HelloWorld!", "DLL Message", MB_OK | MB_ICONINFORMATION);
    }
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		HelloWorld();
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringW(
            L"[VitChallengeDll] DLL descarregada!\n"
        );
        break;
    }

    return TRUE;
}