# Guia de Uso - IAT Hooking com VltChallengeDll

## 🎯 Objetivo

Demonstrar como interceptar e modificar chamadas de funções Windows usando **IAT Hooking**. Quando você executa uma aplicação (como notepad), a DLL injetada instala hooks que modificam o comportamento de funções como `MessageBoxW`.

## 📋 Pré-requisitos

- Visual Studio 2019+ com C++
- Windows 10/11 x64
- Projeto VltChallengeDll compilado
- Privilégios de administrador (para injeção)

## 🚀 Workflow Rápido

### Método 1: Testar Localmente (Sem Injeção)

**Mais seguro e simples para aprendizado.**

```bash
# 1. Compile o teste:
cd IATHooking
cl test_iat_hook.cpp iat_hook.cpp /link User32.lib Kernel32.lib

# 2. Execute:
test_iat_hook.exe

# 3. Observações:
# - Primeira MessageBox será interceptada (texto modificado)
# - Segunda MessageBox será interceptada
# - Terceira será original (após unhook)
```

### Método 2: Injetar em Notepad (Com Injeção)

**Mais realista, demonstra a técnica completa.**

#### Passo 1: Compilar a DLL Hooked

```bash
# Opção A: Usar dllmain_with_iat_hook.cpp
cd VltChallengeDll
# Adicione dllmain_with_iat_hook.cpp ao projeto
# Compile como DLL (VltChallengeDll.dll)

# OU Opção B: Compilar como DLL separada
cd IATHooking
cl /LD hooked_dll.cpp /link User32.lib Kernel32.lib
# Gera: hooked_dll.dll
```

#### Passo 2: Configurar o Injector

Edite `Injector/Injector.cpp`:

```cpp
// Linhas 214-215
const wchar_t* targetProc = L"notepad.exe";  // Alvo
const wchar_t* dllPath = L"C:\\caminho\\para\\VltChallengeDll.dll";  // DLL hooked

// OU para compilação customizada:
const wchar_t* dllPath = L"C:\\caminho\\para\\hooked_dll.dll";
```

#### Passo 3: Compilar o Injector

```bash
cd Injector
cl Injector.cpp /link Kernel32.lib Advapi32.lib User32.lib
```

#### Passo 4: Executar o Attack

```bash
# Terminal 1: Abra notepad (ou mantenha aberto)
notepad.exe

# Terminal 2: Execute com privilégios de admin
Injector.exe

# Esperado na saída:
# [1] Debug privileges...
# [2] Procurando notepad.exe
#     PID: 12345
# [3] Abrindo processo...
# [4] Enumerando threads...
#     Thread encontrada: TID 67890
# [5] Suspendendo threads...
# [6] Preparando hijacking...
# [7] Gerando e alocando stub...
# [8] Hijacking...
# [9] Retomando threads...
# [OK] Injecao concluida!
```

#### Passo 5: Ver o Resultado

```
1. Uma MessageBox aparecerá no notepad:
   [VltChallenge]
   "IAT Hooks Instalados!
    Proximas chamadas de MessageBox serao interceptadas."

2. Em seguida, o MessageBox original do dllmain:
   "Hello da funcao HelloWorld!"

3. Agora, qualquer MessageBox no notepad mostrará:
   [HACKED - IAT Hook]
   "[VLT_CHALLENGE]
    Sua aplicacao foi comprometida!
    (IAT Hook Demo)
    Mensagem original interceptada via IAT Hooking!"
```

## 📊 Fluxograma Completo

```
┌─────────────────────────┐
│ notepad.exe em execução │
└────────────┬────────────┘
             │
┌────────────▼────────────────────────┐
│ Injector.exe inicia                 │
│ 1. EnableDebugPrivilege             │
│ 2. FindProcessId("notepad.exe")     │
│ 3. OpenProcess com permissões VM_*  │
└────────────┬─────────────────────────┘
             │
┌────────────▼──────────────────────────────┐
│ Context Hijacking                         │
│ 1. EnumerateThreads                       │
│ 2. SuspendThread (pausar threads)         │
│ 3. GenerateStub (código assembly)         │
│ 4. WriteProcessMemory (injetar código)    │
│ 5. SetThreadContext (modificar RIP)       │
│ 6. ResumeThread (retomar threads)         │
└────────────┬──────────────────────────────┘
             │
┌────────────▼──────────────────────────────┐
│ VltChallengeDll.dll carregada              │
│ DllMain é executado                       │
│ ├─ InstallIATHooks()                      │
│ │  ├─ Parse PE Header                     │
│ │  ├─ Localiza import descriptor          │
│ │  └─ Altera IAT entries                  │
│ └─ HelloWorld() é executado               │
└────────────┬──────────────────────────────┘
             │
┌────────────▼────────────────────────────────┐
│ Hooks Ativados no Notepad                    │
│                                              │
│ Quando notepad chama MessageBoxW:            │
│   call [IAT + offset]                        │
│       └─> aponta para HookedMessageBoxW      │
│           └─> modifica mensagem              │
│               └─> chama OriginalMessageBoxW  │
└──────────────────────────────────────────────┘
```

## 🔍 Teste em Detalhes

### Test Case 1: MessageBoxW Simples

```cpp
// No notepad (após injeção):
MessageBoxW(nullptr, L"Hello", L"Test", MB_OK);

// Resultado:
// Mostra: "[HACKED - IAT Hook]\n[VLT_CHALLENGE]\n..."
// Em vez de: "Hello"
```

### Test Case 2: Múltiplas MessageBox

```cpp
// Primeira chamada
MessageBoxW(nullptr, L"Message 1", L"Caption 1", MB_OK);  // Hooked!

// Segunda chamada
MessageBoxW(nullptr, L"Message 2", L"Caption 2", MB_OK);  // Hooked!

// Ambas serão interceptadas
```

### Test Case 3: Verificar Sem Injeção

```bash
# Abra notepad normalmente (sem injeção)
notepad.exe

# Comportamento normal - MessageBoxW não é interceptada
```

## 🛠️ Troubleshooting

### Problema: Injector falha com "Processo não encontrado"

**Solução:**
```bash
# Certifique-se que notepad está aberto primeiro
notepad.exe

# Aguarde 2 segundos
# Execute o Injector
```

### Problema: "Access Denied" ao abrir processo

**Solução:**
```bash
# Execute como Administrador
# Clique direito em cmd.exe → "Executar como administrador"
# Execute novamente
```

### Problema: Hook não funciona / MessageBox normal aparece

**Verificar:**
1. DLL foi compilada corretamente?
2. DLL path em Injector.cpp está correto?
3. Notepad ainda está rodando?
4. Verifique Debug Output (OutputDebugStringW)

```cpp
// Use DebugView (Sysinternals) para ver debug output:
// https://docs.microsoft.com/en-us/sysinternals/downloads/debugview

// No DebugView:
// [1234] [VltChallenge] DLL carregada no processo!
// [1234] [+] Procurando por imports de user32.dll...
// [1234] [+] user32.dll encontrado!
// [1234] [+] Encontrado MessageBoxW, instalando hook...
```

### Problema: Notepad crash após injeção

**Causas possíveis:**
1. IAT malformado - verificar parsing de PE
2. Endereço hook incorreto - verificar thunk allocation
3. Proteção de memória - verificar VirtualProtect

**Solução:**
```bash
# Use máquina virtual para testes
# Use debugger (WinDbg) para inspeção pós-crash
```

## 📚 Estruturas Importantes

### IMAGE_IMPORT_DESCRIPTOR

```cpp
// Define qual módulo é importado (ex: user32.dll)
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    DWORD OriginalFirstThunk;  // RVA para INT (informações de importação)
    DWORD TimeDateStamp;
    DWORD ForwarderChain;
    DWORD Name;                // RVA para nome do módulo (ex: "user32.dll")
    DWORD FirstThunk;          // RVA para IAT (tabela de endereços)
} IMAGE_IMPORT_DESCRIPTOR;
```

### IMAGE_THUNK_DATA

```cpp
// Cada entrada na IAT é uma THUNK_DATA
// Ele aponta para o endereço real da função
typedef union _IMAGE_THUNK_DATA64 {
    ULONGLONG ForwarderString;
    ULONGLONG Function;        // <-- Alvo do hook
    ULONGLONG Ordinal;
    ULONGLONG AddressOfData;
} IMAGE_THUNK_DATA64;

// Modificamos Function para apontar para nossa função
```

## 🔐 Segurança

### O que este código faz

✅ **Legítimo:**
- IAT hooking educacional
- Demonstração de Windows Internals
- Pesquisa de segurança
- Ambiente de laboratório controlado

❌ **Não use para:**
- Malware
- Espionagem
- Alterar software sem permissão
- Evasão de antivírus

### Defesa

Para proteger-se contra IAT Hooking:

1. **Monitoramento de Kernel:**
   ```cpp
   // Usar ETW (Event Tracing for Windows) para detectar VirtualProtect
   ```

2. **Importação Dinâmica:**
   ```cpp
   // Em vez de:
   MessageBoxW(...);  // Via IAT
   
   // Usar:
   auto func = (MessageBoxWFunc)GetProcAddress(
       GetModuleHandleW(L"user32.dll"), "MessageBoxW");
   func(...);
   ```

3. **Code Integrity Checking:**
   ```cpp
   // Verificar periodicamente se a IAT foi modificada
   ```

## 📖 Referências

- [Windows Internals Part 1 - 7th Edition](https://docs.microsoft.com/en-us/sysinternals/learn/windows-internals)
- [Microsoft PE Format](https://docs.microsoft.com/en-us/windows/win32/debug/pe-format)
- [Rootkit Arsenal](https://www.amazon.com/Rootkit-Arsenal-Escape-Detection-Evasion/dp/1598220616)

## ✅ Checklist de Sucesso

- [ ] Compilou IATHook.cpp sem erros
- [ ] Test_iat_hook.exe funciona localmente
- [ ] Injector.cpp configurado com caminho correto
- [ ] Notepad aberto antes de executar Injector
- [ ] Injector.exe executado com privilégios de admin
- [ ] MessageBox do notepad foi modificada
- [ ] Viu mensagem "[HACKED - IAT Hook]"
- [ ] Entendeu o fluxo completo PE → IAT → Hook

---

**Parabéns!** Se chegou aqui, domina um dos conceitos mais avançados de Windows Internals! 🎓

