#pragma comment(linker, "/export:DisplayHelloWorld=GenericDll2.DisplayHelloWorld")

#include "pch.h"
#include <Windows.h>
#include <string>

// Original function pointer
typedef int (WINAPI* MessageBoxWFunc)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxWFunc OriginalMessageBoxW = nullptr;

// Hooked version - Intercepta e modifica MessageBoxW
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // Modificar a mensagem
    return OriginalMessageBoxW(hWnd,
                              L"[HACKED by IAT Hook]\n\nMensagem original:\n" + std::wstring(lpText ? lpText : L"(vazia)"),
                              L"[VltChallenge - IAT HOOK]",
                              uType | MB_ICONWARNING);
}

// Instala IAT Hook em MessageBoxW
bool InstallIATHook()
{
    HMODULE hSelf = GetModuleHandleW(nullptr);
    if (!hSelf) return false;

    // Parse PE headers
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hSelf;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hSelf + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    // Get import directory
    PIMAGE_OPTIONAL_HEADER optHeader = &ntHeaders->OptionalHeader;
    IMAGE_DATA_DIRECTORY importDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0) return false;

    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hSelf + importDir.VirtualAddress);

    // Find user32.dll imports
    while (importDesc->Name)
    {
        const char* modName = (const char*)((BYTE*)hSelf + importDesc->Name);

        if (_stricmp(modName, "user32.dll") == 0)
        {
            // Found user32.dll - now find MessageBoxW
            PIMAGE_THUNK_DATA originalThunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->FirstThunk);

            while (originalThunk->u1.AddressOfData)
            {
                if (!(originalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64))
                {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hSelf + (DWORD)originalThunk->u1.AddressOfData);

                    if (strcmp(importByName->Name, "MessageBoxW") == 0)
                    {
                        // Save original function address
                        OriginalMessageBoxW = (MessageBoxWFunc)thunk->u1.Function;

                        // Change page protection to writable
                        DWORD oldProtect;
                        if (VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
                        {
                            // Patch the IAT entry to point to our hook
                            thunk->u1.Function = (ULONGLONG)&HookedMessageBoxW;

                            // Restore original protection
                            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);

                            OutputDebugStringW(L"[+] IAT Hook installed successfully!\n");
                            return true;
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

    return false;
}

extern "C" {
    __declspec(dllexport) void HelloWorld()
    {
        MessageBoxW(NULL, L"Hello World from DLL!", L"HelloWorld", MB_OK);
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
        {
            OutputDebugStringW(L"[VltChallenge] DLL carregada!\n");

            // Step 1: Show injection success
            MessageBoxW(NULL,
                       L"[+] DLL INJETADA COM SUCESSO!\n\nVltChallengeDll foi carregada via Context Hijacking!",
                       L"[VltChallenge] Step 1: Injection",
                       MB_OK | MB_ICONINFORMATION);

            // Step 2: Install IAT Hook
            if (InstallIATHook())
            {
                MessageBoxW(NULL,
                           L"[+] IAT HOOK INSTALADO!\n\nAgora todas as MessageBoxes serao interceptadas!",
                           L"[VltChallenge] Step 2: IAT Hook Ready",
                           MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(NULL,
                           L"[-] Falha ao instalar IAT Hook\n\nMas a DLL foi injetada!",
                           L"[VltChallenge] Hook Failed",
                           MB_OK | MB_ICONWARNING);
            }

            // Step 3: Test the hook by calling HelloWorld
            OutputDebugStringW(L"[VltChallenge] Calling HelloWorld() - should be intercepted...\n");
            HelloWorld();
        }
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringW(L"[VltChallenge] DLL descarregada!\n");
        break;
    }

    return TRUE;
}
