#include <Windows.h>

extern "C" {
    __declspec(dllexport) void HelloWorld()
    {
        MessageBoxW(NULL, L"Hello World!", L"DLL", MB_OK);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        MessageBoxW(NULL,
                   L"DLL INJETADA COM SUCESSO!\n\nVoltChallengeDll foi carregada no processo!",
                   L"[SUCCESS]",
                   MB_OK | MB_ICONINFORMATION);
    }
    return TRUE;
}
