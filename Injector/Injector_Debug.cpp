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
        std::wcerr << L"OpenProcessToken falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid)) {
        std::wcerr << L"LookupPrivilegeValue falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        CloseHandle(tokenHandle);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        std::wcerr << L"AdjustTokenPrivileges falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        CloseHandle(tokenHandle);
        return false;
    }

    CloseHandle(tokenHandle);
    std::wcout << L"[OK] Debug privileges habilitados." << std::endl;
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
    std::wcout << L"[PROC] Procurando por: " << procName << std::endl;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"[ERRO] CreateToolhelp32Snapshot(PROCESS) falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        return 0;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (!Process32FirstW(snapshot, &pe)) {
        std::wcerr << L"[ERRO] Process32FirstW falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        CloseHandle(snapshot);
        return 0;
    }

    const wchar_t* targetName = GetFileName(procName);
    int count = 0;

    do {
        count++;
        if (count <= 10 || _wcsicmp(pe.szExeFile, targetName) == 0) {
            std::wcout << L"  Proc #" << count << ": " << pe.szExeFile << L" (PID=" << pe.th32ProcessID << L")" << std::endl;
        }

        if (_wcsicmp(pe.szExeFile, targetName) == 0) {
            DWORD pid = pe.th32ProcessID;
            std::wcout << L"[OK] Encontrado! PID=" << pid << std::endl;
            CloseHandle(snapshot);
            return pid;
        }
    } while (Process32NextW(snapshot, &pe));

    std::wcerr << L"[ERRO] Processo nao encontrado." << std::endl;
    CloseHandle(snapshot);
    return 0;
}

void DiagnoseThreadEnumeration(DWORD targetPid)
{
    std::wcout << L"\n=== DIAGNOSTICO DE THREADS ===" << std::endl;
    std::wcout << L"Target PID: " << targetPid << std::endl;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"[ERRO] CreateToolhelp32Snapshot(THREAD) falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        return;
    }

    std::wcout << L"[OK] Snapshot de threads criado." << std::endl;

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (!Thread32First(snapshot, &te)) {
        std::wcerr << L"[ERRO] Thread32First falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        CloseHandle(snapshot);
        return;
    }

    std::wcout << L"\nEnumerando threads do sistema inteiro..." << std::endl;

    int totalThreads = 0;
    int openableThreads = 0;
    int targetProcessThreads = 0;
    int suspendableThreads = 0;
    int contextAccessThreads = 0;

    do {
        totalThreads++;

        // Try to open thread with GET_CONTEXT access
        HANDLE th = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (th != nullptr) {
            openableThreads++;

            // Try to get context
            CONTEXT ctx{};
            ctx.ContextFlags = CONTEXT_ALL;
            if (GetThreadContext(th, &ctx)) {
                contextAccessThreads++;

                // Try to suspend (indicator of target process)
                DWORD suspendResult = SuspendThread(th);
                if (suspendResult != (DWORD)-1) {
                    suspendableThreads++;
                    ResumeThread(th);
                    targetProcessThreads++;

                    if (targetProcessThreads <= 5) {
                        std::wcout << L"  Thread " << targetProcessThreads << ": TID=" << te.th32ThreadID
                                  << L" IP=0x" << std::hex;
#if defined(_M_X64)
                        std::wcout << ctx.Rip;
#else
                        std::wcout << ctx.Eip;
#endif
                        std::wcout << std::dec << std::endl;
                    }
                }
            }

            CloseHandle(th);
        }

        if (totalThreads % 100 == 0) {
            std::wcout << L"  ... processadas " << totalThreads << L" threads ..." << std::endl;
        }

    } while (Thread32Next(snapshot, &te));

    CloseHandle(snapshot);

    std::wcout << L"\n=== RESUMO ===" << std::endl;
    std::wcout << L"Total de threads no sistema: " << totalThreads << std::endl;
    std::wcout << L"Threads openable (QUERY_INFORMATION): " << openableThreads << std::endl;
    std::wcout << L"Threads com context access: " << contextAccessThreads << std::endl;
    std::wcout << L"Threads suspendaveis: " << suspendableThreads << std::endl;
    std::wcout << L"Threads do target process (suspensao OK): " << targetProcessThreads << std::endl;

    if (targetProcessThreads == 0) {
        std::wcout << L"\n[AVISO] Nenhuma thread do target encontrada!" << std::endl;
        std::wcout << L"Possíveis causas:" << std::endl;
        std::wcout << L"  1. Notepad é um processo UWP (requer downgrade para classic)" << std::endl;
        std::wcout << L"  2. Insuficientes privilégios (execute como Admin)" << std::endl;
        std::wcout << L"  3. Notepad está bloqueado ou congelado" << std::endl;
    }
}

int main()
{
    std::wcout << L"=== DIAGNOSTICO DE INJECAO ===" << std::endl;

    std::wcout << L"\n[PASSO 1] Habilitando debug privileges..." << std::endl;
    if (!EnableDebugPrivilege()) {
        std::wcerr << L"Falha ao habilitar debug privileges!" << std::endl;
        return 1;
    }

    std::wcout << L"\n[PASSO 2] Encontrando notepad..." << std::endl;
    DWORD pid = FindProcessId(L"notepad.exe");
    if (pid == 0) {
        std::wcerr << L"Notepad nao encontrado! Certifique-se que está aberto." << std::endl;
        return 1;
    }

    std::wcout << L"\n[PASSO 3] Abrindo processo..." << std::endl;
    HANDLE proc = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME,
        FALSE, pid);

    if (proc == nullptr) {
        std::wcerr << L"[ERRO] OpenProcess falhou: 0x" << std::hex << GetLastError() << std::dec << std::endl;
        return 1;
    }

    std::wcout << L"[OK] Processo aberto." << std::endl;

    std::wcout << L"\n[PASSO 4] Diagnosticando threads..." << std::endl;
    DiagnoseThreadEnumeration(pid);

    CloseHandle(proc);

    std::wcout << L"\n=== DIAGNOSTICO COMPLETO ===" << std::endl;
    return 0;
}
