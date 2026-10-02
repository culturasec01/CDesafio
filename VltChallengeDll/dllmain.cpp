#pragma comment(linker, "/export:DisplayHelloWorld=GenericDll2.DisplayHelloWorld")

#include "pch.h"
#include <Windows.h>

extern "C" {
    __declspec(dllexport) void HelloWorld()
    {
        MessageBoxA(NULL, "Hello da funcao HelloWorld!", "DLL Message", MB_OK | MB_ICONINFORMATION);
    }
}

// Teste de injeção com mensagem clara
void TestInjection()
{
    // MessageBox 1: Confirmar que a DLL foi injetada
    MessageBoxW(NULL,
               L"[+] SUCESSO!\n\nVltChallengeDll foi injetada com sucesso no processo!\n\nEsta é a primeira MessageBox após DLL_PROCESS_ATTACH.",
               L"[VltChallengeDll] Injeção Confirmada",
               MB_OK | MB_ICONINFORMATION);

    // MessageBox 2: Chamar HelloWorld
    HelloWorld();

    // MessageBox 3: Prova final
    MessageBoxW(NULL,
               L"[+] Injeção completada!\n\nA DLL está funcionando perfeitamente no seu processo.",
               L"[VltChallengeDll] Status Final",
               MB_OK | MB_ICONINFORMATION);
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Chamar função de teste
        TestInjection();
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringW(
            L"[VitChallengeDll] DLL descarregada!\n"
        );
        break;
    }

    return TRUE;
}