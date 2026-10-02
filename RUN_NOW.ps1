# Script simples para recompilar e injetar AGORA
# Execute como Administrador no PowerShell

Write-Host "`n[*] Limpando arquivos antigos..." -ForegroundColor Cyan

# Delete build cache
Remove-Item -Path "VltChallengeDll\*.obj" -Force -ErrorAction SilentlyContinue
Remove-Item -Path "x64\Debug\VltChallengeDll.dll" -Force -ErrorAction SilentlyContinue
Remove-Item -Path "x64\Debug\VltChallengeDll.lib" -Force -ErrorAction SilentlyContinue

Write-Host "[+] Arquivos limpos" -ForegroundColor Green

Write-Host "`n[*] Recompilando em Visual Studio..." -ForegroundColor Cyan

# Abrir Visual Studio e compilar
Start-Process "VltChallengeDll.sln"

Write-Host "`n[!] ACAO REQUERIDA:" -ForegroundColor Yellow
Write-Host "    1. No Visual Studio, clique em: Build → Rebuild Solution" -ForegroundColor Yellow
Write-Host "    2. Aguarde a compilacao completar" -ForegroundColor Yellow
Write-Host "    3. Depois VOLTE AQUI e pressione Enter" -ForegroundColor Yellow

Read-Host "`nPressione Enter apos compilar no Visual Studio"

Write-Host "`n[*] Abrindo notepad..." -ForegroundColor Cyan
Start-Process notepad
Start-Sleep -Seconds 2

Write-Host "`n[*] Executando Injector..." -ForegroundColor Cyan
cd x64\Debug
.\Injector.exe
cd ..

Write-Host "`n[+] Pronto! Verifique o notepad para as MessageBoxes" -ForegroundColor Green
Read-Host "Pressione Enter para sair"
