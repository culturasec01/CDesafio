# PowerShell script para compilar DLL com IAT Hook e injetar
# Execute como Administrador

Write-Host "`n" + ("=" * 60) -ForegroundColor Cyan
Write-Host "  VltChallengeDll - IAT HOOK INJECTION" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor Cyan

Write-Host "`n[*] Compilando DLL com IAT Hook..." -ForegroundColor Cyan

# Caminho para cl.exe
$vcPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
if (-not (Test-Path $vcPath)) {
    Write-Host "[-] Visual Studio C++ tools nao encontrado!" -ForegroundColor Red
    Write-Host "    Instale: Visual Studio -> Modify -> Desktop Development with C++" -ForegroundColor Yellow
    exit 1
}

# Encontrar versao mais recente do MSVC
$msvcVersion = (Get-ChildItem $vcPath | Sort-Object -Descending)[0].Name
$clPath = "$vcPath\$msvcVersion\bin\Hostx64\x64\cl.exe"
$linkPath = "$vcPath\$msvcVersion\bin\Hostx64\x64\link.exe"

Write-Host "[+] MSVC encontrado: $msvcVersion" -ForegroundColor Green

# Mudar para diretorio da DLL
Push-Location VltChallengeDll

# Compilar dllmain_with_hook.cpp
Write-Host "[*] Compilando dllmain_with_hook.cpp..." -ForegroundColor Cyan
& $clPath /c dllmain_with_hook.cpp /Fo:dllmain_with_hook.obj 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Erro ao compilar!" -ForegroundColor Red
    Write-Host "[*] Tentando compilar versao simples..." -ForegroundColor Yellow
    & $clPath /c dllmain_simple.cpp /Fo:dllmain_with_hook.obj

    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        exit 1
    }
}

Write-Host "[+] Compilacao OK" -ForegroundColor Green

# Linkar para DLL
Write-Host "[*] Linkando para DLL..." -ForegroundColor Cyan
& $linkPath /DLL /OUT:..\x64\Debug\VltChallengeDll.dll dllmain_with_hook.obj kernel32.lib user32.lib 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Erro ao linkar!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[+] DLL criada com sucesso!" -ForegroundColor Green

Pop-Location

# Verificar DLL
$dllPath = "x64\Debug\VltChallengeDll.dll"
if (Test-Path $dllPath) {
    $size = (Get-Item $dllPath).Length
    $modified = (Get-Item $dllPath).LastWriteTime
    Write-Host "[+] DLL: $dllPath" -ForegroundColor Green
    Write-Host "    Tamanho: $size bytes" -ForegroundColor Green
    Write-Host "    Modificado: $modified" -ForegroundColor Green
} else {
    Write-Host "[-] DLL nao encontrada!" -ForegroundColor Red
    exit 1
}

# Verificar se notepad ja esta aberto
Write-Host "`n[*] Verificando notepad..." -ForegroundColor Cyan
$notepad = Get-Process notepad -ErrorAction SilentlyContinue
if (-not $notepad) {
    Write-Host "[-] Notepad nao esta aberto!" -ForegroundColor Red
    Write-Host "[*] Abrindo notepad..." -ForegroundColor Yellow
    Start-Process notepad
    Start-Sleep -Seconds 3
    $notepad = Get-Process notepad -ErrorAction SilentlyContinue
}

if ($notepad) {
    Write-Host "[+] Notepad esta aberto (PID: $($notepad.Id))" -ForegroundColor Green
}

# Executar Injector
Write-Host "`n[*] Executando Injector..." -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor White

Push-Location x64\Debug
try {
    & .\Injector.exe
} catch {
    Write-Host "[-] Erro ao executar Injector!" -ForegroundColor Red
}
Pop-Location

Write-Host ("=" * 60) -ForegroundColor White

Write-Host "`n[SUCCESS] Injeção completada!" -ForegroundColor Green
Write-Host "`n[*] VERIFICAR:" -ForegroundColor Yellow
Write-Host "    1. Procure MessageBoxes no Notepad" -ForegroundColor Yellow
Write-Host "    2. Podem estar atras de outras janelas" -ForegroundColor Yellow
Write-Host "    3. Use Alt+Tab para ativar o Notepad" -ForegroundColor Yellow
Write-Host "`n[*] ESPERADO VER:" -ForegroundColor Cyan
Write-Host "    1. '[+] DLL INJETADA COM SUCESSO'" -ForegroundColor Cyan
Write-Host "    2. '[+] IAT Hook instalado'" -ForegroundColor Cyan
Write-Host "    3. 'Hello World from DLL!' (com texto modificado)" -ForegroundColor Cyan

Write-Host "`n[*] Para testar IAT Hook:" -ForegroundColor Yellow
Write-Host "    - Qualquer MessageBoxW no notepad sera modificada!" -ForegroundColor Yellow

Write-Host "`n" + ("=" * 60) -ForegroundColor Cyan

Read-Host "Pressione Enter para finalizar"
