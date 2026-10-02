#include <Windows.h>
#include <string>

// Original function pointer
typedef int (WINAPI* MessageBoxWFunc)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxWFunc OriginalMessageBoxW = nullptr;

// Hooked version
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    // Mensagem modificada
    return OriginalMessageBoxW(hWnd,
                              L"[HACKED by IAT Hook]\n\nMensagem original:\n" + std::wstring(lpText ? lpText : L"(vazia)"),
                              L"[VltChallenge - IAT HOOK]",
                              uType | MB_ICONWARNING);
}

// Funcao para instalar IAT hook
bool InstallHook()
{
    HMODULE hSelf = GetModuleHandleW(nullptr);
    if (!hSelf) return false;

    // Parse PE
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hSelf;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hSelf + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

    // Get import directory
    PIMAGE_OPTIONAL_HEADER optHeader = &ntHeaders->OptionalHeader;
    IMAGE_DATA_DIRECTORY importDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0) return false;

    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)hSelf + importDir.VirtualAddress);

    // Find user32.dll
    while (importDesc->Name)
    {
        const char* modName = (const char*)((BYTE*)hSelf + importDesc->Name);

        if (_stricmp(modName, "user32.dll") == 0)
        {
            // Found user32.dll, find MessageBoxW
            PIMAGE_THUNK_DATA originalThunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)hSelf + importDesc->FirstThunk);

            while (originalThunk->u1.AddressOfData)
            {
                if (!(originalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64))
                {
                    PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)hSelf + (DWORD)originalThunk->u1.AddressOfData);

                    if (strcmp(importByName->Name, "MessageBoxW") == 0)
                    {
                        // Save original
                        OriginalMessageBoxW = (MessageBoxWFunc)thunk->u1.Function;

                        // Change page protection
                        DWORD oldProtect;
                        if (VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
                        {
                            // Patch IAT
                            thunk->u1.Function = (ULONGLONG)&HookedMessageBoxW;

                            // Restore protection
                            VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);

                            MessageBoxW(NULL,
                                       L"[+] IAT Hook instalado com sucesso!\n\nProximas MessageBoxes serao interceptadas.",
                                       L"[VltChallenge] Hook Status",
                                       MB_OK | MB_ICONINFORMATION);
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // Mensagem 1: Confirmar injeção
        MessageBoxW(NULL,
                   L"[+] DLL INJETADA COM SUCESSO!\n\nVltChallengeDll foi carregada no processo!",
                   L"[VltChallenge] Step 1: Injection",
                   MB_OK | MB_ICONINFORMATION);

        // Mensagem 2: Instalar hook
        if (InstallHook())
        {
            MessageBoxW(NULL,
                       L"[+] IAT Hook instalado!\n\nAgora todas as MessageBoxes serao interceptadas e modificadas.",
                       L"[VltChallenge] Step 2: IAT Hook",
                       MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxW(NULL,
                       L"[-] Falha ao instalar IAT hook.\n\nMas a DLL foi injetada com sucesso!",
                       L"[VltChallenge] Hook Failed",
                       MB_OK | MB_ICONWARNING);
        }

        // Teste: chamar HelloWorld que sera interceptada
        HelloWorld();
    }
    return TRUE;
}
