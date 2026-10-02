# Diagnóstico de ferramentas de compilação

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "DIAGNÓSTICO DE COMPILADORES" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Procurar Visual Studio
Write-Host "[1] Procurando Visual Studio..." -ForegroundColor Yellow
$vsPath = Get-ChildItem "C:\Program Files*" -Filter "*Visual Studio*" -Directory -ErrorAction SilentlyContinue

if ($vsPath) {
    Write-Host "[+] Visual Studio encontrado:" -ForegroundColor Green
    foreach ($vs in $vsPath) {
        Write-Host "    - $($vs.FullName)" -ForegroundColor Green
    }
} else {
    Write-Host "[-] Visual Studio NÃO encontrado" -ForegroundColor Red
}

# Procurar cl.exe
Write-Host "`n[2] Procurando compilador cl.exe..." -ForegroundColor Yellow
$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "[+] cl.exe encontrado:" -ForegroundColor Green
    Write-Host "    - $($cl.Source)" -ForegroundColor Green
} else {
    Write-Host "[-] cl.exe NÃO encontrado" -ForegroundColor Red
}

# Procurar link.exe
Write-Host "`n[3] Procurando linker link.exe..." -ForegroundColor Yellow
$link = Get-Command link.exe -ErrorAction SilentlyContinue
if ($link) {
    Write-Host "[+] link.exe encontrado:" -ForegroundColor Green
    Write-Host "    - $($link.Source)" -ForegroundColor Green
} else {
    Write-Host "[-] link.exe NÃO encontrado" -ForegroundColor Red
}

# Procurar MSBuild
Write-Host "`n[4] Procurando MSBuild..." -ForegroundColor Yellow
$msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if ($msbuild) {
    Write-Host "[+] msbuild.exe encontrado:" -ForegroundColor Green
    Write-Host "    - $($msbuild.Source)" -ForegroundColor Green
} else {
    Write-Host "[-] msbuild.exe NÃO encontrado" -ForegroundColor Red
}

# Procurar Windows SDK
Write-Host "`n[5] Procurando Windows SDK..." -ForegroundColor Yellow
$sdk = Get-ChildItem "C:\Program Files*" -Filter "*Windows Kits*" -Directory -ErrorAction SilentlyContinue

if ($sdk) {
    Write-Host "[+] Windows SDK encontrado:" -ForegroundColor Green
    foreach ($s in $sdk) {
        Write-Host "    - $($s.FullName)" -ForegroundColor Green
    }
} else {
    Write-Host "[-] Windows SDK NÃO encontrado" -ForegroundColor Red
}

# Summary
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "RESUMO" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if ($cl -or $msbuild) {
    Write-Host "[+] Você TEM ferramentas de compilação!" -ForegroundColor Green
    Write-Host "`n[*] Tente:" -ForegroundColor Yellow
    Write-Host "    .\FIND_AND_COMPILE.ps1" -ForegroundColor Yellow
} else {
    Write-Host "[-] Você NÃO tem ferramentas de compilação!" -ForegroundColor Red
    Write-Host "`n[!] INSTRUÇÕES DE INSTALAÇÃO:" -ForegroundColor Yellow
    Write-Host "    1. Abra: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    Write-Host "    2. Download: 'Visual Studio Community 2022'" -ForegroundColor Yellow
    Write-Host "    3. Execute o instalador" -ForegroundColor Yellow
    Write-Host "    4. Selecione: 'Desktop development with C++'" -ForegroundColor Yellow
    Write-Host "    5. Clique em 'Install'" -ForegroundColor Yellow
    Write-Host "    6. Aguarde (pode levar 30+ minutos)" -ForegroundColor Yellow
    Write-Host "    7. Tente este script novamente" -ForegroundColor Yellow
}

Write-Host "`n========================================`n" -ForegroundColor Cyan

Read-Host "Pressione Enter para sair"
