# VltChallengeDll - Context Hijacking + IAT Hook

**Implementação educacional completa de injeção de DLL em Windows com Context Hijacking e IAT Hooking.**

---

## 🎯 O Que É Este Projeto?

Este projeto demonstra **duas técnicas avançadas de Windows Internals:**

### 1. **Context Hijacking** (Injeção)
- Modifica o contexto de uma thread suspensa
- Faz a thread executar código injetado (stub em assembly)
- Não cria novas threads (muito furtivo)

### 2. **IAT Hooking** (Interceptação)
- Modifica a Import Address Table (IAT) de um processo
- Redireciona chamadas de funções (ex: MessageBoxW)
- Permite interceptar e modificar comportamento

---

## 🚀 Quick Start (30 segundos)

### Opção 1: Visual Studio (Recomendado)

```batch
# 1. Abra VltChallengeDll.sln no Visual Studio Community
# 2. Build → Rebuild Solution
# 3. Abra notepad.exe
# 4. Execute: x64\Debug\Injector.exe (como Admin)
# 5. Veja 3 MessageBoxes no Notepad!
```

📖 **Guia Completo:** [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md)

### Opção 2: PowerShell (Alternativa)

```powershell
# PowerShell como Administrador
cd C:\Users\Max\source\repos\VltChallengeDll
.\DIAGNOSE.ps1                    # Verificar instalação
.\FIND_AND_COMPILE.ps1            # Compilar e injetar
```

---

## 📁 Estrutura do Projeto

```
VltChallengeDll/
│
├── VltChallengeDll/               ← DLL a ser injetada
│   ├── dllmain.cpp                ← IAT Hook + Context Hijacking
│   ├── dllmain_simple.cpp         ← Versão simples (sem hook)
│   ├── dllmain_with_hook.cpp      ← Versão alternativa
│   ├── main.cpp
│   ├── proxy.cpp
│   ├── pch.h / pch.cpp
│   └── VltChallengeDll.vcxproj    ← Projeto VS
│
├── Injector/                      ← Ferramenta de injeção
│   ├── Injector.cpp               ← Context Hijacking implementation
│   ├── Injector_*.cpp             ← Variações de técnicas
│   └── Injector.vcxproj           ← Projeto VS
│
├── IATHooking/                    ← Módulo educacional de IAT Hook
│   ├── iat_hook.h / iat_hook.cpp  ← Classe reutilizável
│   ├── test_iat_hook.cpp          ← Teste standalone
│   ├── hooked_dll.cpp             ← DLL com hook
│   ├── README.md                  ← Documentação técnica (1400+ linhas)
│   └── GUIA_USO.md                ← Guia prático
│
├── Target/                        ← Processo alvo de teste
│   └── Target.cpp
│
├── Artigo/                        ← Documentação completa
│   ├── artigo_principal.md        ← Artigo técnico completo
│   └── README.md
│
├── VltChallengeDll.sln            ← Solução Visual Studio
├── SETUP_VISUAL_STUDIO.md         ← 👈 LER PRIMEIRO
├── RECOMPILE_AND_TEST.md          ← Troubleshooting
├── README_PT.md                   ← Este arquivo
└── ...
```

---

## 🔍 O Que Esperar Ver

Quando executa `Injector.exe`, você verá **3 MessageBoxes:**

### MessageBox 1: Confirmação de Injeção
```
[VltChallenge] Step 1: Injection
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[+] DLL INJECTADA COM SUCESSO!

VltChallengeDll foi carregada via 
Context Hijacking!

[OK]
```

### MessageBox 2: Hook Instalado
```
[VltChallenge] Step 2: IAT Hook Ready
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[+] IAT HOOK INSTALADO!

Agora todas as MessageBoxes serao
interceptadas!

[OK]
```

### MessageBox 3: Demonstração do Hook
```
[VltChallenge - IAT HOOK]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[HACKED by IAT Hook]

Mensagem original:
Hello World from DLL!

[OK]
```

---

## 📚 Documentação

### Para Iniciantes
- [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md) - Configuração passo-a-passo
- [`IATHooking/README.md`](IATHooking/README.md) - Explicação de IAT Hooking

### Para Intermediários
- [`Artigo/artigo_principal.md`](Artigo/artigo_principal.md) - Artigo técnico completo
- [`IATHooking/GUIA_USO.md`](IATHooking/GUIA_USO.md) - Guia prático com exemplos

### Para Avançados
- `Injector/Injector.cpp` - Context Hijacking implementation
- `VltChallengeDll/dllmain.cpp` - IAT Hook implementation
- `IATHooking/iat_hook.h` - Classe reutilizável

---

## 🛠️ Como Usar

### Cenário 1: Testar a Injeção

```batch
# Terminal 1: Abra notepad
notepad.exe

# Terminal 2: Execute Injector (como Admin)
x64\Debug\Injector.exe
```

**Resultado:** 3 MessageBoxes aparecem no Notepad.

### Cenário 2: Modificar e Testar

```cpp
// Em VltChallengeDll/dllmain.cpp, mude:
MessageBoxW(NULL,
    L"Sua mensagem aqui!",  // ← mude isto
    L"Seu titulo aqui!",    // ← e isto
    MB_OK | MB_ICONINFORMATION);
```

```batch
# Compile em VS: Build → Rebuild Solution
# Execute novamente: x64\Debug\Injector.exe
```

### Cenário 3: Estudar IAT Hooking

```batch
cd IATHooking
cl test_iat_hook.cpp iat_hook.cpp /link User32.lib Kernel32.lib
test_iat_hook.exe
```

---

## 🔐 Segurança & Avisos

⚠️ **AVISOS LEGAIS:**

- ✅ **Permitido:** Estudo em laboratório próprio, pesquisa educacional
- ✅ **Permitido:** Ambiente de desenvolvimento controlado, máquinas virtuais
- ❌ **Proibido:** Uso não autorizado em máquinas de terceiros
- ❌ **Proibido:** Malware, espionagem, evasão de segurança
- ❌ **Proibido:** Distribuição sem propósito educacional

**Use responsavelmente.**

---

## 🎓 Conceitos Aprendidos

Ao completar este projeto, você entenderá:

### Windows Internals
- [ ] PE Format (Portable Executable)
- [ ] Import Address Table (IAT)
- [ ] Thread Context manipulation
- [ ] Virtual Memory protection (PAGE_READWRITE)

### Segurança
- [ ] DLL Injection via Context Hijacking
- [ ] IAT Hook implementation
- [ ] Detection & Defense mechanisms
- [ ] Malware analysis fundamentals

### Programação
- [ ] Assembly x64 (stub generation)
- [ ] Windows API (CreateProcess, OpenThread, etc)
- [ ] Memory manipulation
- [ ] PE header parsing

---

## 📊 Requisitos

| Componente | Requisito |
|-----------|-----------|
| SO | Windows 10/11 x64 |
| Visual Studio | Community 2022 (com C++) |
| Compilador | MSVC (cl.exe) |
| Privilégios | Administrador |
| Conhecimento | C++, Assembly básico |

---

## 🔧 Troubleshooting

### Problema: Nenhuma MessageBox Aparece

**Solução:**
1. Verificar se VS compilou sem erros
2. Confirmar que Notepad está aberto
3. Executar Injector como Admin
4. Ver: [`RECOMPILE_AND_TEST.md`](RECOMPILE_AND_TEST.md)

### Problema: Erro de Compilação

**Solução:**
1. Abrir: [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md)
2. Seguir seção "Troubleshooting"
3. Executar: `Build → Clean Solution` depois `Rebuild Solution`

### Problema: VS Community Não Instalado

**Solução:**
1. Download: https://visualstudio.microsoft.com/downloads/
2. Instalar: "Desktop development with C++"
3. Seguir: [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md)

---

## 📈 Progresso

### Iniciante
- [ ] Ler este README
- [ ] Executar SETUP_VISUAL_STUDIO.md
- [ ] Compilar e testar com VS

### Intermediário
- [ ] Estudar Injector.cpp (Context Hijacking)
- [ ] Estudar dllmain.cpp (IAT Hook)
- [ ] Ler artigo técnico completo

### Avançado
- [ ] Modificar IAT Hook para outras funções
- [ ] Implementar suas próprias técnicas
- [ ] Estudar defesas contra injeção

---

## 🚀 Próximas Etapas

1. **Setup Visual Studio** → [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md)
2. **Compilar e testar** → Build → Rebuild Solution
3. **Estudar código** → Injector.cpp & dllmain.cpp
4. **Ler documentação** → [`Artigo/artigo_principal.md`](Artigo/artigo_principal.md)
5. **Experimentar** → Modifique e teste seus próprios hooks

---

## 📝 Licença

CC BY-NC-SA 4.0 - Uso educacional com atribuição

---

## ✍️ Autor

**Maximiliano Tarigo** | 2026

---

## 🔗 Referências

- Windows Internals (6th Edition)
- Microsoft PE Format Specification
- Rootkit Arsenal by Gray Hat
- OWASP Security Testing Guide

---

**Status:** ✅ Pronto para Uso  
**Última Atualização:** 02 de outubro de 2026  
**Próxima Etapa:** [`SETUP_VISUAL_STUDIO.md`](SETUP_VISUAL_STUDIO.md)
