#include <Windows.h>
#include <iostream>

int main()
{
    std::cout << "Processo iniciado." << std::endl;
    std::cout << "PID: " << GetCurrentProcessId() << std::endl;

    std::cout << "Pressione Ctrl+C para encerrar." << std::endl;

    while (true)
    {
        Sleep(1000);
    }

    return 0;
}