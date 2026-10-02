# Script que encontra MSBuild em qualquer lugar e compila
# Run as Administrator

Write-Host "`n[*] Procurando por MSBuild..." -ForegroundColor Cyan

# Possíveis locais do MSBuild
$msbuildPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuild = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path $path) {
        $msbuild = $path
        break
    }
}

if (-not $msbuild) {
    Write-Host "[-] MSBuild nao encontrado em nenhum local!" -ForegroundColor Red
    Write-Host "`n[!] Voce precisa instalar Visual Studio C++ tools:" -ForegroundColor Yellow
    Write-Host "    1. Abra: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    Write-Host "    2. Download: Visual Studio Community" -ForegroundColor Yellow
    Write-Host "    3. Instale: Desktop development with C++" -ForegroundColor Yellow
    Write-Host "    4. Tente novamente" -ForegroundColor Yellow
    exit 1
}

Write-Host "[+] MSBuild encontrado: $msbuild`n" -ForegroundColor Green

# Alternativa: Usar cl.exe diretamente
$vcPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
$clPath = $null

if (Test-Path $vcPath) {
    $msvcVersion = (Get-ChildItem $vcPath | Sort-Object -Descending)[0].Name
    $clPath = "$vcPath\$msvcVersion\bin\Hostx64\x64\cl.exe"
    Write-Host "[+] Compilador C++ encontrado: $clPath`n" -ForegroundColor Green
}

if (-not (Test-Path $clPath)) {
    Write-Host "[-] Compilador C++ nao encontrado!" -ForegroundColor Red
    exit 1
}

# Clean
Write-Host "[1/3] Limpando build artifacts..." -ForegroundColor Yellow
Get-ChildItem -Path "VltChallengeDll" -Filter "*.obj" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path "." -Filter "*.pdb" -Recurse -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

if (Test-Path "x64") {
    Remove-Item "x64" -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "[+] Limpeza completa`n" -ForegroundColor Green

# Compile dllmain.cpp directly
Write-Host "[2/3] Compilando dllmain.cpp..." -ForegroundColor Yellow

Push-Location VltChallengeDll

# Compile
& $clPath /c dllmain.cpp /Fo:dllmain.obj 2>&1 | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Erro ao compilar dllmain.cpp!" -ForegroundColor Red
    & $clPath /c dllmain.cpp
    Pop-Location
    exit 1
}

Write-Host "[+] Compilacao OK" -ForegroundColor Green

# Link to DLL
Write-Host "[*] Linkando para DLL..." -ForegroundColor Cyan

$linkPath = $clPath.Replace("cl.exe", "link.exe")

& $linkPath /DLL /OUT:..\x64\Debug\VltChallengeDll.dll dllmain.obj kernel32.lib user32.lib 2>&1 | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Erro ao linkar!" -ForegroundColor Red
    & $linkPath /DLL /OUT:..\x64\Debug\VltChallengeDll.dll dllmain.obj kernel32.lib user32.lib
    Pop-Location
    exit 1
}

Write-Host "[+] DLL criada com sucesso!`n" -ForegroundColor Green

Pop-Location

# Verify DLL
Write-Host "[3/3] Verificando DLL..." -ForegroundColor Yellow
$dllPath = "x64\Debug\VltChallengeDll.dll"

if (Test-Path $dllPath) {
    $size = (Get-Item $dllPath).Length
    $time = (Get-Item $dllPath).LastWriteTime
    Write-Host "[+] DLL pronta para injetar!" -ForegroundColor Green
    Write-Host "    Path: $dllPath" -ForegroundColor Green
    Write-Host "    Size: $size bytes" -ForegroundColor Green
    Write-Host "    Time: $time`n" -ForegroundColor Green
} else {
    Write-Host "[-] DLL nao encontrada!" -ForegroundColor Red
    exit 1
}

# Inject
Write-Host "[4/4] Abrindo notepad e executando injector..." -ForegroundColor Cyan

# Kill existing notepad
taskkill /im notepad.exe /F 2>$null | Out-Null

# Open new notepad
Write-Host "[*] Abrindo Notepad..." -ForegroundColor Yellow
Start-Process notepad
Start-Sleep -Seconds 2

# Run injector
Write-Host "`n================================================" -ForegroundColor Cyan
Write-Host "[*] EXECUTANDO INJECTOR COM IAT HOOK..." -ForegroundColor Cyan
Write-Host "================================================`n" -ForegroundColor Cyan

Push-Location x64\Debug

if (Test-Path ".\Injector.exe") {
    & .\Injector.exe
} else {
    Write-Host "[-] Injector.exe nao encontrado!" -ForegroundColor Red
    Write-Host "[!] Compile o projeto Injector antes de rodar este script" -ForegroundColor Yellow
}

Pop-Location

Write-Host "`n================================================" -ForegroundColor Cyan
Write-Host "[SUCCESS!] Verifique o Notepad para as MessageBoxes!" -ForegroundColor Green
Write-Host "`nVoce deveria ver:" -ForegroundColor Yellow
Write-Host "  1. '[+] DLL INJECTADA COM SUCESSO!'" -ForegroundColor Yellow
Write-Host "  2. '[+] IAT HOOK INSTALADO!'" -ForegroundColor Yellow
Write-Host "  3. '[HACKED by IAT Hook]' com Hello World modificado" -ForegroundColor Yellow
Write-Host "================================================`n" -ForegroundColor Cyan

Read-Host "Pressione Enter para sair"
