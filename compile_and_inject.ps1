# PowerShell script para compilar e injetar
# Execute como Administrador

Write-Host "[*] Compilando DLL simplificada..." -ForegroundColor Cyan

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

# Compilar
Write-Host "[*] Compilando dllmain_simple.cpp..." -ForegroundColor Cyan
& $clPath /c dllmain_simple.cpp /Fo:dllmain_simple.obj

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Erro ao compilar!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[+] Compilacao OK" -ForegroundColor Green

# Linkar para DLL
Write-Host "[*] Linkando para DLL..." -ForegroundColor Cyan
& $linkPath /DLL /OUT:..\x64\Debug\VltChallengeDll.dll dllmain_simple.obj kernel32.lib user32.lib

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
    Write-Host "[+] DLL: $dllPath ($size bytes)" -ForegroundColor Green
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
    Start-Sleep -Seconds 2
} else {
    Write-Host "[+] Notepad ja esta aberto (PID: $($notepad.Id))" -ForegroundColor Green
}

# Executar Injector
Write-Host "`n[*] Executando Injector..." -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor White

Push-Location x64\Debug
& .\Injector.exe
Pop-Location

Write-Host "============================================================" -ForegroundColor White
Write-Host "`n[*] Verificar se MessageBox apareceu no Notepad!" -ForegroundColor Yellow
Write-Host "[?] Se nao apareceu:" -ForegroundColor Yellow
Write-Host "    1. Verifique se notepad esta na frente" -ForegroundColor Yellow
Write-Host "    2. Pode estar atras de outras janelas" -ForegroundColor Yellow
Write-Host "    3. Procure na taskbar por notificacoes" -ForegroundColor Yellow

Read-Host "`nPressione Enter para sair"
