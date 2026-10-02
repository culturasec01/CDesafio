#include "pch.h"
#include <windows.h>

extern "C" {
    __declspec(dllexport) void HelloWorld()
    {
        MessageBoxA(NULL, "Hello da funcao HelloWorld!", "DLL Message", MB_OK | MB_ICONINFORMATION);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Executado automaticamente no carregamento da DLL
        MessageBoxA(NULL, "DLL carregada com sucesso no processo!", "DllMain", MB_OK | MB_ICONINFORMATION);
        break;
    case DLL_PROCESS_DETACH:
        // Executado quando a DLL é descarregada
        break;
    }
    return TRUE;
}