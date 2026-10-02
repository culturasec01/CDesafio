# IAT Hooking - Windows Internals Lab

Uma implementação educacional de **Import Address Table (IAT) Hooking** em Windows, demonstrando como interceptar e redirecionar chamadas de funções de APIs.

## 📚 Conceitos Fundamentais

### O que é IAT?

A **Import Address Table (IAT)** é uma tabela dentro da estrutura PE (Portable Executable) que armazena os endereços de funções importadas de DLLs externas.

```
Executável (meu.exe)
└── PE Header
    └── Data Directory
        └── Import Directory
            └── Import Descriptor (user32.dll)
                ├── OriginalFirstThunk (INT)
                └── FirstThunk (IAT)
                    ├── CreateFileW → 0x7FF00000
                    ├── ReadFile    → 0x7FF00004
                    └── MessageBoxW → 0x7FF00008
```

### Como Funciona IAT Hooking?

Quando o código chama `MessageBoxW()`, ele na verdade faz:

```asm
call [RIP + offset]  ; Chama endereço na IAT
```

Se alterarmos o endereço na IAT para apontar para nossa função, todas as chamadas serão desviadas:

```
Aplicação
    │
    ├─ call MessageBoxW()
    │
    └─ jmp [IAT + offset]
            │
            ▼
        Nosso Hook (HookedMessageBoxW)
            │
            └─ opcionalmente chama Original()
```

## 🏗️ Estrutura do Projeto

```
IATHooking/
├── iat_hook.h              # Header da classe IATHook
├── iat_hook.cpp            # Implementação do hooking
├── test_iat_hook.cpp       # Teste standalone
├── hooked_dll.cpp          # DLL que faz hooking
├── notepad_hooker.cpp      # Launcher para notepad
└── README.md               # Este arquivo
```

## 🔧 Componentes Principais

### 1. IATHook Class (iat_hook.h / iat_hook.cpp)

Classe reutilizável para fazer IAT hooking:

```cpp
IATHook hooker;

// Instalar hook
hooker.HookFunction("user32.dll", "MessageBoxW", (void*)HookedMessageBoxW);

// Obter função original
MessageBoxWFunc original = (MessageBoxWFunc)hooker.GetOriginalFunction("user32.dll", "MessageBoxW");

// Remover hook
hooker.UnhookFunction("user32.dll", "MessageBoxW");
```

**Métodos principais:**

- `HookFunction()` - Instala um hook
- `UnhookFunction()` - Remove um hook
- `GetOriginalFunction()` - Obtém endereço original

### 2. Test Program (test_iat_hook.cpp)

Programa de teste que demonstra o hooking funcionando:

```cpp
// Hooked function
int WINAPI HookedMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    std::wcout << "[HOOK] MessageBoxW interceptado!" << std::endl;
    // Chama original com mensagem modificada
    return OriginalMessageBoxW(hWnd, L"[HACKED] Mensagem modificada!", lpCaption, uType);
}

// No main:
IATHook hooker;
hooker.HookFunction("user32.dll", "MessageBoxW", (void*)HookedMessageBoxW);
OriginalMessageBoxW = (MessageBoxWFunc)hooker.GetOriginalFunction("user32.dll", "MessageBoxW");

MessageBoxW(nullptr, L"Original text", L"Caption", MB_OK);
// Mostra: "[HACKED] Mensagem modificada!"
```

### 3. Hooked DLL (hooked_dll.cpp)

Uma DLL que, quando injetada, instala IAT hooks no processo alvo.

Uso com Injector:

```
1. Compile hooked_dll.cpp como DLL
2. Configure Injector.cpp para apontar para hooked_dll.dll
3. Abra o processo alvo (notepad.exe)
4. Execute Injector.exe
5. Qualquer MessageBoxW será interceptada!
```

## 📋 Fluxo de Parsing PE para IAT

Para encontrar a IAT, o código segue este caminho:

```
1. GetModuleHandleA(nullptr)          // Obém a base do módulo
2. PIMAGE_DOS_HEADER                   // Lê o DOS header
3. IMAGE_DOS_HEADER.e_lfanew           // Offset para NT header
4. PIMAGE_NT_HEADERS                   // Lê o NT header
5. OptionalHeader.DataDirectory        // Array de diretórios
6. DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]  // Import directory
7. PIMAGE_IMPORT_DESCRIPTOR            // Descritor de módulo importado
8. OriginalFirstThunk + FirstThunk     // Entradas IAT
9. IMAGE_IMPORT_BY_NAME                // Nome da função
10. Patch do endereço na IAT
```

## 🔐 Mecanismo de Patch

Quando encontramos a entrada IAT correta:

```cpp
// 1. Verificar permissões atuais
DWORD oldProtect;
VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect);

// 2. Salvar endereço original
void* original = (void*)thunk->u1.Function;

// 3. Escrever novo endereço
thunk->u1.Function = (ULONGLONG)&HookedMessageBoxW;

// 4. Restaurar permissões
VirtualProtect(thunk, sizeof(PIMAGE_THUNK_DATA), oldProtect, &oldProtect);
```

## 🎯 Casos de Uso

### 1. Hooking Standalone

```cpp
IATHook hooker;
hooker.HookFunction("kernel32.dll", "CreateFileW", (void*)MyCreateFileW);
// Agora todas as chamadas a CreateFileW são interceptadas
```

### 2. Hooking via Injeção

```cpp
// Em uma DLL injetada:
BOOL APIENTRY DllMain(...) {
    case DLL_PROCESS_ATTACH:
        InstallIATHook();  // Instala hook no contexto do processo
        break;
}
```

### 3. Hooking Múltiplas Funções

```cpp
IATHook hooker;
hooker.HookFunction("user32.dll", "MessageBoxW", (void*)HookedMessageBoxW);
hooker.HookFunction("kernel32.dll", "CreateFileW", (void*)HookedCreateFileW);
hooker.HookFunction("kernel32.dll", "ReadFile", (void*)HookedReadFile);
```

## ⚠️ Limitações e Considerações

### Escopo do Hook

IAT hooking **afeta apenas um módulo**. Se o notepad importa `MessageBoxW`, alterar a IAT do notepad não afeta:

- Outras DLLs carregadas no processo
- Código que já resolveu o endereço manualmente
- Código que chama via GetProcAddress

### Arquitetura

O projeto é para **x64**. Diferenças x86:

```
x64                          x86
─────────────────────────────────────
ULONGLONG Function           DWORD Function
8 bytes                      4 bytes
IMAGE_THUNK_DATA64           IMAGE_THUNK_DATA32
```

### Proteção de Memória

A página contendo a IAT pode ter proteções:

```cpp
// Sempre verificar resultado
if (!VirtualProtect(..., PAGE_READWRITE, ...)) {
    // Falha - proteção pode estar ativada
}
```

## 🛡️ Defesa Contra IAT Hooking

### Detecção

1. **Inspeção Manual de PE:**
   ```cpp
   // Verificar se endereços na IAT apontam para suas DLLs
   ```

2. **Monitoring do Kernel:**
   - Detectar chamadas a `VirtualProtect` em páginas IAT
   - Monitorar mudanças de memória em seções read-only

3. **Code Signing:**
   - Verificar integridade da IAT periodicamente

### Prevenção

1. **Importação Dinâmica:**
   ```cpp
   // Em vez de:
   MessageBoxW(...);  // Via IAT
   
   // Usar:
   HMODULE h = LoadLibraryW(L"user32.dll");
   auto func = (MessageBoxWFunc)GetProcAddress(h, "MessageBoxW");
   func(...);  // Bypass IAT
   ```

2. **Resolução em Tempo de Execução:**
   ```cpp
   auto CreateFileW = (CreateFileWFunc)GetProcAddress(
       GetModuleHandleW(L"kernel32.dll"), "CreateFileW");
   ```

## 📖 Estruturas PE Relevantes

```cpp
IMAGE_DOS_HEADER {
    WORD e_magic;          // 'MZ'
    ...
    LONG e_lfanew;         // Offset para NT header
}

IMAGE_NT_HEADERS {
    DWORD Signature;       // 'PE\0\0'
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER OptionalHeader;
}

IMAGE_OPTIONAL_HEADER {
    IMAGE_DATA_DIRECTORY DataDirectory[16];
}

IMAGE_IMPORT_DESCRIPTOR {
    DWORD OriginalFirstThunk;  // RVA para INT
    DWORD Name;                // RVA para nome do módulo
    DWORD FirstThunk;          // RVA para IAT
}

IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;        // <-- Endereço da função (alvo do hook)
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
}

IMAGE_IMPORT_BY_NAME {
    WORD Hint;
    CHAR Name[1];  // String terminada em null
}
```

## 🚀 Compilação

### Visual Studio (CMake)

```cmake
add_library(IATHooking 
    iat_hook.cpp
    iat_hook.h
)

add_executable(TestIATHook test_iat_hook.cpp)
target_link_libraries(TestIATHook IATHooking)

add_library(HookedDLL SHARED hooked_dll.cpp)
```

### Command Line

```bash
# Compilar biblioteca
cl /c iat_hook.cpp

# Compilar teste
cl test_iat_hook.cpp iat_hook.obj /link User32.lib Kernel32.lib

# Compilar DLL
cl /LD hooked_dll.cpp /link User32.lib Kernel32.lib
```

## 📝 Exemplo Completo: Notepad Hooking

**Passo 1:** Compile `hooked_dll.cpp` como DLL:
```
cl /LD hooked_dll.cpp /link User32.lib Kernel32.lib
```

**Passo 2:** Modifique `Injector.cpp`:
```cpp
const wchar_t* dllPath = L"C:\\...\\HookedDLL.dll";  // Caminho da DLL hooked
```

**Passo 3:** Compile o Injector conforme instruções no VltChallengeDll

**Passo 4:** Execute:
```
1. Abra notepad.exe
2. Execute Injector.exe
3. MessageBoxW será interceptada!
```

## 🔬 Análise com Debugger

```cpp
// No WinDbg:
0:000> !dml_proc_modules
// Listar módulos carregados

0:000> dd [address_of_iat]
// Verificar endereços na IAT

0:000> ba e4 [iat_entry]
// Break on write à entrada IAT
```

## 📚 Referências

- Windows Internals (6th Edition) - Chapter 7: Processes, Threads and Job Objects
- Microsoft PE Format Specification
- Rootkit Arsenal by Gray Hat - Chapter on IAT Hooking
- Windows API Reference

---

**Status:** ✅ Educacional  
**Nível:** Intermediário-Avançado  
**Requisitos:** Windows x64, Visual Studio C++  
**Avisos:** Apenas para laboratório - não use em produção sem autorização
