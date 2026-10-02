#include "iat_hook.h"
#include <iostream>
#include <cctype>
#include <algorithm>

#pragma comment(lib, "Kernel32.lib")

IATHook::IATHook() = default;

IATHook::~IATHook() {
    // Cleanup hooks if needed
}

std::string ToLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

PIMAGE_IMPORT_DESCRIPTOR IATHook::GetImportDescriptor(const char* moduleName) {
    HMODULE selfModule = GetModuleHandleA(nullptr);
    if (!selfModule) return nullptr;

    // Get DOS header
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)selfModule;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cerr << "Invalid DOS header" << std::endl;
        return nullptr;
    }

    // Get NT headers
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)dosHeader + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        std::cerr << "Invalid NT header signature" << std::endl;
        return nullptr;
    }

    // Get optional header
    PIMAGE_OPTIONAL_HEADER optHeader = &ntHeaders->OptionalHeader;

    // Get import directory
    IMAGE_DATA_DIRECTORY importDir = optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0) {
        std::cerr << "No import directory found" << std::endl;
        return nullptr;
    }

    // Get import descriptor
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)selfModule + importDir.VirtualAddress);

    std::string targetModule = ToLowercase(moduleName);

    while (importDesc->Name) {
        const char* importModuleName = (const char*)((BYTE*)selfModule + importDesc->Name);
        std::string currentModule = ToLowercase(importModuleName);

        if (currentModule == targetModule) {
            return importDesc;
        }

        importDesc++;
    }

    std::cerr << "Module not found in imports: " << moduleName << std::endl;
    return nullptr;
}

PIMAGE_THUNK_DATA IATHook::FindIATThunk(PIMAGE_IMPORT_DESCRIPTOR importDesc,
                                         const char* functionName) {
    if (!importDesc) return nullptr;

    HMODULE selfModule = GetModuleHandleA(nullptr);
    PIMAGE_THUNK_DATA originalThunk = (PIMAGE_THUNK_DATA)((BYTE*)selfModule + importDesc->OriginalFirstThunk);
    PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)((BYTE*)selfModule + importDesc->FirstThunk);

    while (originalThunk->u1.AddressOfData) {
        if (!(originalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64)) {
            // Import by name
            PIMAGE_IMPORT_BY_NAME importByName = (PIMAGE_IMPORT_BY_NAME)((BYTE*)selfModule + (DWORD)originalThunk->u1.AddressOfData);
            if (strcmp(importByName->Name, functionName) == 0) {
                return thunk;
            }
        }

        originalThunk++;
        thunk++;
    }

    std::cerr << "Function not found in imports: " << functionName << std::endl;
    return nullptr;
}

bool IATHook::PatchIAT(PIMAGE_THUNK_DATA thunk, void* newFunction, void*& oldFunction) {
    if (!thunk) return false;

    // Save original address
    oldFunction = (void*)thunk->u1.Function;

    // Change page protection to allow writing
    DWORD oldProtect;
    if (!VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect)) {
        std::cerr << "Failed to change page protection" << std::endl;
        return false;
    }

    // Patch the IAT entry
    thunk->u1.Function = (ULONGLONG)newFunction;

    // Restore original protection
    if (!VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect)) {
        std::cerr << "Failed to restore page protection" << std::endl;
        return false;
    }

    std::cout << "[+] IAT patched successfully" << std::endl;
    std::cout << "    Original: 0x" << std::hex << (ULONGLONG)oldFunction << std::dec << std::endl;
    std::cout << "    New:      0x" << std::hex << (ULONGLONG)newFunction << std::dec << std::endl;

    return true;
}

bool IATHook::UnpatchIAT(PIMAGE_THUNK_DATA thunk, void* originalFunction) {
    if (!thunk || !originalFunction) return false;

    DWORD oldProtect;
    if (!VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect)) {
        return false;
    }

    thunk->u1.Function = (ULONGLONG)originalFunction;

    if (!VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect)) {
        return false;
    }

    return true;
}

bool IATHook::HookFunction(const std::string& moduleName,
                          const std::string& functionName,
                          void* newFunction) {
    std::cout << "[*] Attempting to hook: " << moduleName << "!" << functionName << std::endl;

    // Get import descriptor
    PIMAGE_IMPORT_DESCRIPTOR importDesc = GetImportDescriptor(moduleName.c_str());
    if (!importDesc) {
        std::cerr << "[-] Failed to get import descriptor" << std::endl;
        return false;
    }

    // Find IAT thunk
    PIMAGE_THUNK_DATA thunk = FindIATThunk(importDesc, functionName.c_str());
    if (!thunk) {
        std::cerr << "[-] Failed to find IAT thunk" << std::endl;
        return false;
    }

    // Patch IAT
    void* originalFunction;
    if (!PatchIAT(thunk, newFunction, originalFunction)) {
        std::cerr << "[-] Failed to patch IAT" << std::endl;
        return false;
    }

    // Store hook info
    std::string key = moduleName + "!" + functionName;
    hooks[key] = { originalFunction, newFunction, thunk };

    std::cout << "[+] Hook installed successfully" << std::endl;
    return true;
}

bool IATHook::UnhookFunction(const std::string& moduleName,
                            const std::string& functionName) {
    std::string key = moduleName + "!" + functionName;
    auto it = hooks.find(key);

    if (it == hooks.end()) {
        std::cerr << "[-] Hook not found: " << key << std::endl;
        return false;
    }

    HookEntry& entry = it->second;
    if (!UnpatchIAT(entry.thunkData, entry.originalAddress)) {
        std::cerr << "[-] Failed to unpatch IAT" << std::endl;
        return false;
    }

    hooks.erase(it);
    std::cout << "[+] Hook removed: " << key << std::endl;
    return true;
}

void* IATHook::GetOriginalFunction(const std::string& moduleName,
                                   const std::string& functionName) {
    std::string key = moduleName + "!" + functionName;
    auto it = hooks.find(key);

    if (it == hooks.end()) {
        return nullptr;
    }

    return it->second.originalAddress;
}
