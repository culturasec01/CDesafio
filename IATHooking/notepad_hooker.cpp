#include <Windows.h>
#include <iostream>
#include <string>

// Para usar com o VltChallengeDll - este launcher abre notepad e injeta a DLL

int main() {
    std::wcout << L"\n" << std::wstring(60, L'=') << std::endl;
    std::wcout << L"  NOTEPAD IAT HOOKING LAUNCHER" << std::endl;
    std::wcout << std::wstring(60, L'=') << L"\n" << std::endl;

    // Passo 1: Iniciar notepad.exe
    std::cout << "[*] Inicializando notepad.exe..." << std::endl;

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(L"notepad.exe", nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << "[-] Failed to launch notepad" << std::endl;
        return 1;
    }

    std::cout << "[+] Notepad launched with PID: " << pi.dwProcessId << std::endl;

    // Passo 2: Aguardar um pouco para notepad inicializar completamente
    Sleep(2000);

    std::cout << "\n[*] Notepad deve estar aberto agora." << std::endl;
    std::cout << "[*] Para usar IAT hooking no notepad:" << std::endl;
    std::cout << "    1. Certifique-se de ter o Injector compilado" << std::endl;
    std::cout << "    2. Altere o alvo para 'notepad.exe' em Injector.cpp" << std::endl;
    std::cout << "    3. Compile a DLL hooked (versao que faz IAT hooking)" << std::endl;
    std::cout << "    4. Execute o Injector enquanto notepad estiver rodando" << std::endl;
    std::cout << "    5. MessageBoxW será interceptada!\n" << std::endl;

    // Manter processo vivo
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
