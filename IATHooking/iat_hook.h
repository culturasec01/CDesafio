#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>

class IATHook {
public:
    using HookCallback = void*;

    IATHook();
    ~IATHook();

    bool HookFunction(const std::string& moduleName,
                     const std::string& functionName,
                     void* newFunction);

    bool UnhookFunction(const std::string& moduleName,
                       const std::string& functionName);

    void* GetOriginalFunction(const std::string& moduleName,
                             const std::string& functionName);

private:
    struct HookEntry {
        void* originalAddress;
        void* newAddress;
        PIMAGE_THUNK_DATA thunkData;
    };

    std::unordered_map<std::string, HookEntry> hooks;

    PIMAGE_IMPORT_DESCRIPTOR GetImportDescriptor(const char* moduleName);
    PIMAGE_THUNK_DATA FindIATThunk(PIMAGE_IMPORT_DESCRIPTOR importDesc,
                                   const char* functionName);
    bool PatchIAT(PIMAGE_THUNK_DATA thunk, void* newFunction, void*& oldFunction);
    bool UnpatchIAT(PIMAGE_THUNK_DATA thunk, void* originalFunction);
};
