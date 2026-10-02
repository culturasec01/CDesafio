# 🚀 Setup Completo - Visual Studio Community

Guia passo-a-passo para configurar e rodar o projeto VltChallengeDll no Visual Studio Community.

---

## 📋 Pré-requisitos

- Windows 10/11 x64
- Visual Studio Community 2022 (com C++ Tools)
- Privilégios de Administrador

---

## ✅ Passo 1: Instalar Visual Studio Community

### 1.1 Download

Acesse: https://visualstudio.microsoft.com/downloads/

Clique em **"Visual Studio Community 2022"** → **Download**

### 1.2 Executar Instalador

Quando o arquivo baixar (`vs_community*.exe`), execute-o.

### 1.3 Selecionar Componentes

Na tela "Workloads":

```
☑ Desktop development with C++
```

Deixe marcado apenas isso. Clique em **Install**.

### 1.4 Aguardar Instalação

Leva 15-30 minutos. **NÃO interrompa.**

Quando terminar, pode pedir restart - faça o restart.

---

## 🎯 Passo 2: Abrir o Projeto no VS

### 2.1 Abrir Visual Studio

Procure por **"Visual Studio 2022"** no menu Iniciar e abra.

### 2.2 Abrir Solução

```
File → Open → Project/Solution
```

Navegue até:
```
C:\Users\Max\source\repos\VltChallengeDll\VltChallengeDll.sln
```

Clique em **Open**.

### 2.3 Esperar Carregamento

O VS vai carregar todos os projetos. Isso pode levar 1-2 minutos na primeira vez.

---

## 🔨 Passo 3: Compilar a Solução

### 3.1 Compilar Tudo

```
Build → Rebuild Solution
```

Ou pressione: **`Ctrl + Alt + F7`**

### 3.2 Esperar Compilação

Veja na janela de Output:

```
Build: 3 succeeded, 0 failed
========== Build complete ==========
```

**Importante:** Se houver erros, veja a seção "Troubleshooting" abaixo.

---

## 💉 Passo 4: Rodar a Injeção

### 4.1 Verificar Executáveis

Depois de compilar, você deve ter:

```
x64\Debug\VltChallengeDll.dll
x64\Debug\Injector.exe
```

### 4.2 Abrir Command Prompt como Admin

```
Windows + R
cmd
Ctrl + Shift + Enter (para Admin)
```

### 4.3 Navegar e Executar

```batch
cd C:\Users\Max\source\repos\VltChallengeDll
notepad.exe
REM (Deixe notepad aberto em outro terminal)
REM Em outro terminal:
x64\Debug\Injector.exe
```

### 4.4 Ver MessageBoxes

Você deve ver **3 MessageBoxes** no Notepad:

1. **"[+] DLL INJECTADA COM SUCESSO!"**
2. **"[+] IAT HOOK INSTALADO!"**
3. **"[HACKED by IAT Hook]"** (Hello World modificado)

---

## 🔧 Estrutura do Projeto

```
VltChallengeDll/
├── VltChallengeDll/          ← DLL a ser injetada
│   ├── dllmain.cpp           ← Código principal com IAT Hook
│   ├── VltChallengeDll.vcxproj
│   └── ...
├── Injector/                 ← Ferramenta que injeta a DLL
│   ├── Injector.cpp          ← Context Hijacking
│   ├── Injector.vcxproj
│   └── ...
├── IATHooking/               ← Módulo de IAT Hooking (educacional)
│   ├── iat_hook.h
│   ├── iat_hook.cpp
│   ├── test_iat_hook.cpp
│   └── README.md
├── VltChallengeDll.sln       ← Solução (abre TUDO)
└── ...
```

---

## 📊 O Que Cada Projeto Faz

### **VltChallengeDll (DLL)**

```cpp
// dllmain.cpp
- DLL_PROCESS_ATTACH:
  1. MessageBox: "DLL Injectada!"
  2. InstallIATHook()
  3. MessageBox: "IAT Hook instalado!"
  4. HelloWorld() → INTERCEPTADA por hook
```

**Compilação:** DLL
**Saída:** `x64/Debug/VltChallengeDll.dll`

### **Injector (Aplicação)**

```cpp
// Injector.cpp
- EnableDebugPrivilege()
- FindProcessId("notepad.exe")
- EnumerateThreads()
- SuspendThread()
- GenerateStub() → assembly x64
- HijackThread() → modifica contexto
- ResumeThread() → executa DLL
```

**Compilação:** Executável  
**Saída:** `x64/Debug/Injector.exe`

### **IATHooking (Educacional)**

Módulo reutilizável para IAT Hooking.
- `test_iat_hook.cpp` - Teste standalone
- `iat_hook.h/cpp` - Classe reutilizável

---

## 🚨 Troubleshooting

### Erro: "Cannot open source file 'Windows.h'"

**Solução:**

1. `Tools → Get Tools and Features`
2. Clique em **Modify**
3. Marque: **Desktop development with C++**
4. Clique em **Modify** (novamente)
5. Feche VS durante instalação
6. Quando terminar, reabra o projeto

### Erro: "Linker: unresolved external symbol"

**Solução:**

1. Clique direito em cada projeto
2. `Properties → Linker → Input`
3. Certifique-se que tem:
   - `kernel32.lib`
   - `user32.lib`
   - `advapi32.lib`

### Erro: "The build order of project dependencies is incorrect"

**Solução:** Feche e reabra a solução:
```
File → Close Solution
File → Recent Projects → VltChallengeDll.sln
```

### MessageBox Não Aparece

**Verificar:**

1. ✅ Compilou sem erros?
2. ✅ Notepad.exe está aberto?
3. ✅ Executou Injector.exe com privilégios Admin?
4. ✅ MessageBox pode estar atrás (Alt+Tab)

**Debug:**

```batch
# Ver mensagens de debug
Ctrl + Alt + O  (em VS, abre Output)
Debug → Windows → Output
```

---

## 🎓 Workflow Recomendado

### Modificar dllmain.cpp

1. Edite `VltChallengeDll/dllmain.cpp`
2. `Build → Rebuild Solution` (ou `Ctrl+Alt+F7`)
3. Feche notepad anterior
4. Abra novo notepad
5. Rode: `x64\Debug\Injector.exe`

### Modificar Injector.cpp

1. Edite `Injector/Injector.cpp`
2. `Build → Rebuild Solution`
3. Rode: `x64\Debug\Injector.exe`

### Testar IAT Hooking Isoladamente

```batch
cd IATHooking
cl test_iat_hook.cpp iat_hook.cpp /link User32.lib Kernel32.lib
test_iat_hook.exe
```

---

## 📚 Próximos Passos

Depois que tudo estiver funcionando:

### 1. Estude o Código

- `Injector.cpp` - Context Hijacking
- `dllmain.cpp` - IAT Hook
- `IATHooking/iat_hook.h` - Parsing PE

### 2. Experimente

- Mude a mensagem no IAT Hook
- Hook outras funções (CreateFileW, etc)
- Crie sua própria DLL

### 3. Aprenda Mais

- Leia: `IATHooking/README.md` (1400+ linhas)
- Leia: `Artigo/artigo_principal.md` (documentação completa)

---

## ✨ Dicas Úteis

### Limpar Build Cache

```
Build → Clean Solution
```

Depois rebuild.

### Debug com Breakpoints

1. Clique na linha em `dllmain.cpp`
2. Pressione `F9` (toggle breakpoint)
3. `Debug → Start Debugging` (F5)

### Ver Estrutura do Projeto

```
View → Solution Explorer  (Ctrl + Alt + L)
```

---

## 📞 Problemas Persistentes?

Se ainda não funcionar:

1. **Deletar completamente:**
   ```
   Delete: x64/, VltChallengeDll/x64/, etc
   ```

2. **Fechar VS completamente**

3. **Reabrir a solução**

4. **Rebuild novamente**

---

## ✅ Checklist de Sucesso

- [ ] Visual Studio Community instalado
- [ ] Solução abre sem erros
- [ ] Rebuild sem erros
- [ ] `x64/Debug/VltChallengeDll.dll` existe
- [ ] `x64/Debug/Injector.exe` existe
- [ ] Notepad abre
- [ ] Injector executa
- [ ] **VER 3 MessageBoxes no Notepad**
- [ ] MessageBox 1: "DLL INJECTADA"
- [ ] MessageBox 2: "IAT HOOK INSTALADO"
- [ ] MessageBox 3: "HACKED by IAT Hook"

**Parabéns! Você domina Context Hijacking + IAT Hooking!** 🏆

---

**Data:** 02 de outubro de 2026  
**Autor:** Maximiliano Tarigo  
**Status:** Pronto para produção educacional
