#include <Windows.h>
#include <TlHelp32.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Advapi32.lib")

#define RvaToPointer(type, baseAddress, offset) \
    reinterpret_cast<type>( \
        reinterpret_cast<DWORD_PTR>(baseAddress) + offset)

using LoadLibraryAPtr = HMODULE(__stdcall*)(LPCSTR);
using GetProcAddressPtr = FARPROC(__stdcall*)(HMODULE, LPCSTR);

typedef struct {
    void* remoteDllBaseAddress;
    LoadLibraryAPtr remoteLoadLibraryA;
    GetProcAddressPtr remoteGetProcAddress;
} RelocationStubParameters;

// Relocation stub - must be position independent!
void __declspec(noinline) RelocationStub(RelocationStubParameters* parameters)
{
    const auto dosHeader = RvaToPointer(IMAGE_DOS_HEADER*, parameters->remoteDllBaseAddress, 0);
    const auto ntHeader = RvaToPointer(IMAGE_NT_HEADERS*, parameters->remoteDllBaseAddress, dosHeader->e_lfanew);

    const auto relocationOffset = reinterpret_cast<DWORD_PTR>(parameters->remoteDllBaseAddress) - ntHeader->OptionalHeader.ImageBase;

    typedef struct {
        WORD offset : 12;
        WORD type : 4;
    } RELOCATION_INFO;

    // Base relocation
    const auto baseRelocationDirectoryEntry = RvaToPointer(IMAGE_BASE_RELOCATION*,
        parameters->remoteDllBaseAddress,
        ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

    IMAGE_BASE_RELOCATION* reloc = baseRelocationDirectoryEntry;
    while (reloc->VirtualAddress != 0) {
        const auto relocationCount = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(RELOCATION_INFO);
        const auto relocationInfo = RvaToPointer(RELOCATION_INFO*, reloc, sizeof(IMAGE_BASE_RELOCATION));

        for (size_t i = 0; i < relocationCount; i++) {
            if (relocationInfo[i].type == IMAGE_REL_BASED_DIR64) {
                const auto relocAddress = RvaToPointer(DWORD_PTR*,
                    parameters->remoteDllBaseAddress,
                    reloc->VirtualAddress + relocationInfo[i].offset);
                *relocAddress += relocationOffset;
            }
        }

        reloc = RvaToPointer(IMAGE_BASE_RELOCATION*, reloc, reloc->SizeOfBlock);
    }

    // Import fixing
    const auto baseImportsDirectory = RvaToPointer(IMAGE_IMPORT_DESCRIPTOR*,
        parameters->remoteDllBaseAddress,
        ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (size_t index = 0; baseImportsDirectory[index].Characteristics != 0; index++) {
        const auto moduleName = RvaToPointer(const char*,
            parameters->remoteDllBaseAddress,
            baseImportsDirectory[index].Name);

        const auto loadedModule = parameters->remoteLoadLibraryA(moduleName);

        auto addressTableEntry = RvaToPointer(IMAGE_THUNK_DATA*,
            parameters->remoteDllBaseAddress,
            baseImportsDirectory[index].FirstThunk);

        const auto nameTableEntry = RvaToPointer(IMAGE_THUNK_DATA*,
            parameters->remoteDllBaseAddress,
            baseImportsDirectory[index].OriginalFirstThunk);

        auto currentNameEntry = nameTableEntry ? nameTableEntry : addressTableEntry;

        while (currentNameEntry->u1.Function != 0) {
            if (currentNameEntry->u1.Ordinal & IMAGE_ORDINAL_FLAG) {
                addressTableEntry->u1.Function = reinterpret_cast<ULONGLONG>(
                    parameters->remoteGetProcAddress(loadedModule,
                        MAKEINTRESOURCEA(IMAGE_ORDINAL(currentNameEntry->u1.Ordinal))));
            } else {
                const auto importByName = RvaToPointer(IMAGE_IMPORT_BY_NAME*,
                    parameters->remoteDllBaseAddress,
                    currentNameEntry->u1.AddressOfData);

                addressTableEntry->u1.Function = reinterpret_cast<ULONGLONG>(
                    parameters->remoteGetProcAddress(loadedModule, (LPCSTR)importByName->Name));
            }

            currentNameEntry++;
            addressTableEntry++;
        }
    }

    // TLS callbacks
    if (ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > 0) {
        const auto tlsDirectory = RvaToPointer(IMAGE_TLS_DIRECTORY*,
            parameters->remoteDllBaseAddress,
            ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);

        auto tlsCallback = RvaToPointer(PIMAGE_TLS_CALLBACK*,
            parameters->remoteDllBaseAddress,
            (DWORD_PTR)tlsDirectory->AddressOfCallBacks - (DWORD_PTR)parameters->remoteDllBaseAddress);

        while (*tlsCallback != nullptr) {
            (*tlsCallback)(parameters->remoteDllBaseAddress, DLL_PROCESS_ATTACH, nullptr);
            tlsCallback++;
        }
    }

    // Call DllMain
    using DllMainPtr = BOOL(__stdcall*)(HINSTANCE, DWORD, LPVOID);
    const auto dllMain = RvaToPointer(DllMainPtr,
        parameters->remoteDllBaseAddress,
        ntHeader->OptionalHeader.AddressOfEntryPoint);

    dllMain(reinterpret_cast<HINSTANCE>(parameters->remoteDllBaseAddress), DLL_PROCESS_ATTACH, nullptr);
}

std::vector<char> ReadDllBytes(const wchar_t* dllPath)
{
    std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::wcerr << L"Falha ao abrir DLL: " << dllPath << std::endl;
        return {};
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> bytes(size);
    file.read(bytes.data(), size);
    return bytes;
}

void* WriteDllToProcess(HANDLE proc, const std::vector<char>& dllBytes)
{
    std::wcout << L"Escrevendo DLL na memoria..." << std::endl;

    const auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(dllBytes.data());
    const auto ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS*>(dllBytes.data() + dosHeader->e_lfanew);

    const size_t imageSize = ntHeader->OptionalHeader.SizeOfImage;
    const void* remoteBase = VirtualAllocEx(proc, nullptr, imageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    if (remoteBase == nullptr) {
        std::wcerr << L"VirtualAllocEx falhou!" << std::endl;
        return nullptr;
    }

    std::wcout << L"  Base remota: 0x" << std::hex << (DWORD_PTR)remoteBase << std::dec << std::endl;

    // Write sections
    const auto section = IMAGE_FIRST_SECTION(ntHeader);
    for (size_t i = 0; i < ntHeader->FileHeader.NumberOfSections; i++) {
        const auto targetAddr = RvaToPointer(char*, remoteBase, section[i].VirtualAddress);
        const auto sourceAddr = dllBytes.data() + section[i].PointerToRawData;

        SIZE_T written = 0;
        if (!WriteProcessMemory(proc, targetAddr, sourceAddr, section[i].SizeOfRawData, &written)) {
            std::wcerr << L"WriteProcessMemory (section) falhou!" << std::endl;
            return nullptr;
        }
    }

    // Write PE header
    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, (void*)remoteBase, dllBytes.data(), 0x1000, &written)) {
        std::wcerr << L"WriteProcessMemory (header) falhou!" << std::endl;
        return nullptr;
    }

    std::wcout << L"DLL escrito com sucesso." << std::endl;
    return (void*)remoteBase;
}

bool EnableDebugPrivilege()
{
    HANDLE tokenHandle = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle)) {
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid)) {
        CloseHandle(tokenHandle);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        CloseHandle(tokenHandle);
        return false;
    }

    CloseHandle(tokenHandle);
    return true;
}

const wchar_t* GetFileName(const wchar_t* path)
{
    const wchar_t* fn = wcsrchr(path, L'\\');
    if (fn) return fn + 1;
    fn = wcsrchr(path, L'/');
    if (fn) return fn + 1;
    return path;
}

DWORD FindProcessId(const wchar_t* procName)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (!Process32FirstW(snapshot, &pe)) {
        CloseHandle(snapshot);
        return 0;
    }

    const wchar_t* targetName = GetFileName(procName);

    do {
        if (_wcsicmp(pe.szExeFile, targetName) == 0) {
            DWORD pid = pe.th32ProcessID;
            CloseHandle(snapshot);
            return pid;
        }
    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);
    return 0;
}

int main()
{
    const wchar_t* targetProc = L"notepad.exe";
    const wchar_t* dllPath = L"C:\\Users\\Max\\source\\repos\\VltChallengeDll\\x64\\Debug\\VltChallengeDll.dll";

    std::wcout << L"=== MANUAL MAPPING DLL INJECTOR ===" << std::endl;

    std::wcout << L"\n[1] Debug privileges..." << std::endl;
    EnableDebugPrivilege();

    std::wcout << L"[2] Procurando " << targetProc << std::endl;
    DWORD pid = FindProcessId(targetProc);
    if (pid == 0) {
        std::wcerr << L"Processo nao encontrado!" << std::endl;
        return 1;
    }
    std::wcout << L"    PID: " << pid << std::endl;

    std::wcout << L"[3] Abrindo processo..." << std::endl;
    HANDLE proc = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (proc == nullptr) {
        std::wcerr << L"OpenProcess falhou!" << std::endl;
        return 1;
    }

    std::wcout << L"[4] Lendo DLL..." << std::endl;
    auto dllBytes = ReadDllBytes(dllPath);
    if (dllBytes.empty()) {
        CloseHandle(proc);
        return 1;
    }
    std::wcout << L"    Tamanho: " << dllBytes.size() << L" bytes" << std::endl;

    std::wcout << L"[5] Escrevendo DLL..." << std::endl;
    void* remoteDllBase = WriteDllToProcess(proc, dllBytes);
    if (remoteDllBase == nullptr) {
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"[6] Preparando stub..." << std::endl;

    // Get kernel32 functions
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(k32, "LoadLibraryA");
    FARPROC getProcAddr = GetProcAddress(k32, "GetProcAddress");

    RelocationStubParameters params{
        .remoteDllBaseAddress = remoteDllBase,
        .remoteLoadLibraryA = (LoadLibraryAPtr)loadLibAddr,
        .remoteGetProcAddress = (GetProcAddressPtr)getProcAddr
    };

    // Allocate stub parameters
    void* remoteParams = VirtualAllocEx(proc, nullptr, sizeof(RelocationStubParameters), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (remoteParams == nullptr) {
        std::wcerr << L"VirtualAllocEx (params) falhou!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, remoteParams, &params, sizeof(params), &written)) {
        std::wcerr << L"WriteProcessMemory (params) falhou!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"    Params: 0x" << std::hex << (DWORD_PTR)remoteParams << std::dec << std::endl;

    // Allocate relocation stub
    void* remoteStub = VirtualAllocEx(proc, nullptr, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (remoteStub == nullptr) {
        std::wcerr << L"VirtualAllocEx (stub) falhou!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    if (!WriteProcessMemory(proc, remoteStub, (void*)RelocationStub, 0x10000, &written)) {
        std::wcerr << L"WriteProcessMemory (stub) falhou!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"    Stub: 0x" << std::hex << (DWORD_PTR)remoteStub << std::dec << std::endl;

    std::wcout << L"[7] Executando relocation stub..." << std::endl;
    HANDLE remoteThread = CreateRemoteThread(
        proc,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)remoteStub,
        remoteParams,
        0,
        nullptr);

    if (remoteThread == nullptr) {
        std::wcerr << L"CreateRemoteThread falhou!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"    Thread criada!" << std::endl;

    WaitForSingleObject(remoteThread, INFINITE);
    CloseHandle(remoteThread);

    std::wcout << L"\n[OK] Manual mapping concluido!" << std::endl;
    std::wcout << L"[INFO] DLL nao aparecera em Module listings!" << std::endl;

    CloseHandle(proc);
    return 0;
}
