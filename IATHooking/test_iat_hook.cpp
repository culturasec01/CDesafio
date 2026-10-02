#include "iat_hook.h"
#include <Windows.h>
#include <iostream>

// Original MessageBoxW function pointer (will be set by hooking)
typedef int (WINAPI *MessageBoxWFunc)(HWND, LPCWSTR, LPCWSTR, UINT);
MessageBoxWFunc OriginalMessageBoxW = nullptr;

// Our hooked version
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    std::wcout << L"[HOOK] MessageBoxW interceptado!" << std::endl;
    std::wcout << L"       Original caption: " << (lpCaption ? lpCaption : L"(null)") << std::endl;
    std::wcout << L"       Original text: " << (lpText ? lpText : L"(null)") << std::endl;

    // Call original function with modified message
    return OriginalMessageBoxW(hWnd,
                              L"[HACKED] Esta mensagem foi interceptada!\n\nMensagem original foi modificada via IAT Hook!",
                              L"[IAT HOOK DEMO]",
                              uType);
}

int main() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  IAT HOOKING DEMONSTRATION" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;

    // Create IAT hook instance
    IATHook hooker;

    // Install the hook
    std::cout << "[*] Installing IAT hook for MessageBoxW..." << std::endl;
    if (!hooker.HookFunction("user32.dll", "MessageBoxW", (void*)HookedMessageBoxW)) {
        std::cerr << "[-] Failed to install hook" << std::endl;
        return 1;
    }

    // Get original function address for our hooked function to call
    OriginalMessageBoxW = (MessageBoxWFunc)hooker.GetOriginalFunction("user32.dll", "MessageBoxW");
    if (!OriginalMessageBoxW) {
        std::cerr << "[-] Failed to get original function address" << std::endl;
        return 1;
    }

    std::cout << "\n[*] Calling MessageBoxW (will be intercepted)...\n" << std::endl;

    // Call MessageBoxW - this will now go through our hook
    MessageBoxW(nullptr,
               L"This is the original message",
               L"Original Caption",
               MB_OK | MB_ICONINFORMATION);

    std::cout << "\n[+] MessageBoxW returned (via hook)" << std::endl;

    // Test 2: Call MessageBoxW again
    std::cout << "\n[*] Calling MessageBoxW again...\n" << std::endl;
    MessageBoxW(nullptr,
               L"Second message test",
               L"Test 2",
               MB_YESNO | MB_ICONQUESTION);

    // Unhook
    std::cout << "\n[*] Removing hook..." << std::endl;
    if (!hooker.UnhookFunction("user32.dll", "MessageBoxW")) {
        std::cerr << "[-] Failed to remove hook" << std::endl;
        return 1;
    }

    // Call MessageBoxW after unhooking - should use original
    std::cout << "\n[*] Calling MessageBoxW (after unhooking - original function)...\n" << std::endl;
    MessageBoxW(nullptr,
               L"This message goes through the ORIGINAL function",
               L"After Unhook",
               MB_OK | MB_ICONINFORMATION);

    std::cout << "\n[+] Test completed successfully!" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;

    return 0;
}
