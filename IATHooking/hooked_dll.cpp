#include <Windows.h>
#include <iostream>
#include <string>

// Esta DLL será injetada e fará IAT hooking do notepad

typedef int (WINAPI* MessageBoxWFunc)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxWFunc OriginalMessageBoxW = nullptr;

// Função hookeada
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    OutputDebugStringW(L"[HOOKED_DLL] MessageBoxW interceptado!\n");

    // Interceptar e modificar a mensagem
    return OriginalMessageBoxW(hWnd,
                              L"[HACKED by IAT Hook]\n\nSua aplicacao foi comprometida!\nEsta e uma demonstracao educacional.",
                              L"[IAT HOOK DEMO]",
                              uType | MB_ICONWARNING);
}

// Funcao para fazer IAT hooking
bool InstallIATHook() {
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

    // Find user32.dll import
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hSelf + importDir.VirtualAddress);

    while (importDesc->Name) {
        const char* modName = (const char*)((BYTE*)hSelf + importDesc->Name);

        if (_stricmp(modName, "user32.dll") == 0) {
            // Found user32.dll, now find MessageBoxW
            PIMAGE_THUNK_DATA originalThunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->FirstThunk);

            while (originalThunk->u1.AddressOfData) {
                if (!(originalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64)) {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hSelf + (DWORD)originalThunk->u1.AddressOfData);

                    if (strcmp(importByName->Name, "MessageBoxW") == 0) {
                        // Found MessageBoxW! Save original and patch
                        OriginalMessageBoxW = (MessageBoxWFunc)thunk->u1.Function;

                        // Change page protection
                        DWORD oldProtect;
                        if (!VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect)) {
                            return false;
                        }

                        // Patch the IAT
                        thunk->u1.Function = (ULONGLONG)&HookedMessageBoxW;

                        // Restore protection
                        VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);

                        OutputDebugStringW(L"[HOOKED_DLL] IAT hook installed successfully!\n");
                        return true;
                    }
                }
                originalThunk++;
                thunk++;
            }
        }
        importDesc++;
    }

    return false;
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        {
            OutputDebugStringW(L"[HOOKED_DLL] Injetada no processo!\n");

            // Install the IAT hook
            if (InstallIATHook()) {
                OutputDebugStringW(L"[HOOKED_DLL] Hook instalado com sucesso!\n");
                MessageBoxW(nullptr,
                           L"IAT Hook Instalado!\n\nProximas mensagens serao interceptadas.",
                           L"[Hook Notification]",
                           MB_OK | MB_ICONINFORMATION);
            } else {
                OutputDebugStringW(L"[HOOKED_DLL] Falha ao instalar hook!\n");
            }
        }
        break;

    case DLL_PROCESS_DETACH:
        OutputDebugStringW(L"[HOOKED_DLL] Descarregada do processo!\n");
        break;
    }

    return TRUE;
}
