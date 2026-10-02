#pragma comment(linker, "/export:DisplayHelloWorld=GenericDll2.DisplayHelloWorld")

#include "pch.h"
#include <Windows.h>
#include <iostream>

// Original function pointers
typedef int (WINAPI* MessageBoxWFunc)(HWND, LPCWSTR, LPCWSTR, UINT);
typedef BOOL (WINAPI* MessageBoxAFunc)(HWND, LPCSTR, LPCSTR, UINT);

MessageBoxWFunc OriginalMessageBoxW = nullptr;
MessageBoxAFunc OriginalMessageBoxA = nullptr;

// Hooked versions
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    OutputDebugStringW(L"[VltChallenge_Hook] MessageBoxW interceptado!\n");

    // Modificar a mensagem
    return OriginalMessageBoxW(hWnd,
                              L"[VLT_CHALLENGE]\n\nSua aplicacao foi comprometida!\n\n(IAT Hook Demo)\n\nMensagem original interceptada via IAT Hooking!",
                              L"[HACKED - IAT Hook]",
                              uType | MB_ICONWARNING);
}

BOOL WINAPI HookedMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    OutputDebugStringA("[VltChallenge_Hook] MessageBoxA interceptado!\n");

    return OriginalMessageBoxA(hWnd,
                              "[VLT_CHALLENGE]\n\nSua aplicacao foi comprometida!\n\n(IAT Hook Demo)\n\nMensagem original interceptada via IAT Hooking!",
                              "[HACKED - IAT Hook]",
                              uType | MB_ICONWARNING);
}

// Função para fazer IAT hooking
bool InstallIATHooks() {
    OutputDebugStringW(L"[VltChallenge] Iniciando instalacao de IAT hooks...\n");

    // Obter a base do módulo alvo (notepad.exe ou outro processo)
    HMODULE hTargetModule = GetModuleHandleA(nullptr);
    if (!hTargetModule) {
        OutputDebugStringW(L"[-] Falha ao obter modulo target\n");
        return false;
    }

    // Parse PE headers
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hTargetModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        OutputDebugStringW(L"[-] DOS header invalido\n");
        return false;
    }

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hTargetModule + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        OutputDebugStringW(L"[-] NT header invalido\n");
        return false;
    }

    // Get optional header and import directory
    PIMAGE_OPTIONAL_HEADER optHeader = &ntHeaders->OptionalHeader;
    IMAGE_DATA_DIRECTORY importDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

    if (importDir.VirtualAddress == 0) {
        OutputDebugStringW(L"[-] Nenhum import directory encontrado\n");
        return false;
    }

    // Get import descriptors
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hTargetModule + importDir.VirtualAddress);
    int hooked = 0;

    OutputDebugStringW(L"[+] Procurando por imports de user32.dll...\n");

    // Loop through import descriptors
    while (importDesc->Name) {
        const char* moduleName = (const char*)((BYTE*)hTargetModule + importDesc->Name);

        if (_stricmp(moduleName, "user32.dll") == 0) {
            OutputDebugStringA("[+] user32.dll encontrado!\n");

            PIMAGE_THUNK_DATA originalThunk = (PIMAGE_THUNK_DATA)((BYTE*)hTargetModule + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hTargetModule + importDesc->FirstThunk);

            // Loop through imported functions
            while (originalThunk->u1.AddressOfData) {
                if (!(originalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64)) {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hTargetModule + (DWORD)originalThunk->u1.AddressOfData);

                    // Hook MessageBoxW
                    if (strcmp(importByName->Name, "MessageBoxW") == 0) {
                        OutputDebugStringA("[+] Encontrado MessageBoxW, instalando hook...\n");

                        // Save original
                        OriginalMessageBoxW = (MessageBoxWFunc)thunk->u1.Function;

                        // Change protection
                        DWORD oldProtect;
                        if (VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect)) {
                            thunk->u1.Function = (ULONGLONG)&HookedMessageBoxW;
                            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);
                            OutputDebugStringA("[+] MessageBoxW hooked com sucesso!\n");
                            hooked++;
                        }
                    }
                    // Hook MessageBoxA
                    else if (strcmp(importByName->Name, "MessageBoxA") == 0) {
                        OutputDebugStringA("[+] Encontrado MessageBoxA, instalando hook...\n");

                        // Save original
                        OriginalMessageBoxA = (MessageBoxAFunc)thunk->u1.Function;

                        // Change protection
                        DWORD oldProtect;
                        if (VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect)) {
                            thunk->u1.Function = (ULONGLONG)&HookedMessageBoxA;
                            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);
                            OutputDebugStringA("[+] MessageBoxA hooked com sucesso!\n");
                            hooked++;
                        }
                    }
                }
                originalThunk++;
                thunk++;
            }
            break;
        }
        importDesc++;
    }

    if (hooked > 0) {
        OutputDebugStringW(L"[+] IAT hooks instalados com sucesso!\n");
        return true;
    }

    OutputDebugStringW(L"[-] Nenhuma funcao foi hookeada\n");
    return false;
}

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
        OutputDebugStringW(L"[VltChallenge] DLL carregada no processo!\n");

        // Install IAT hooks
        if (InstallIATHooks()) {
            OutputDebugStringW(L"[VltChallenge] Hooks instalados!\n");

            // Show notification
            MessageBoxW(nullptr,
                       L"IAT Hooks Instalados!\n\nProximas chamadas de MessageBox serao interceptadas.",
                       L"[VltChallenge - IAT Hook]",
                       MB_OK | MB_ICONINFORMATION);
        } else {
            OutputDebugStringW(L"[VltChallenge] Falha ao instalar hooks!\n");
        }

        HelloWorld();
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringW(L"[VltChallenge] DLL descarregada!\n");
        break;
    }

    return TRUE;
}
