# Force recompile using MSBuild directly
# Run as Administrator

Write-Host "`n[*] FORCE COMPILE - Ignorando cache do Visual Studio" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

# Find MSBuild
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

if (-not (Test-Path $msbuild)) {
    Write-Host "[-] MSBuild nao encontrado!" -ForegroundColor Red
    exit 1
}

Write-Host "[+] MSBuild encontrado: $msbuild`n" -ForegroundColor Green

# Clean everything
Write-Host "[1/3] Limpando build artifacts..." -ForegroundColor Yellow
Get-ChildItem -Path "VltChallengeDll" -Filter "*.obj" -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path "VltChallengeDll" -Filter "*.pdb" -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path "x64" -Recurse | Remove-Item -Force -ErrorAction SilentlyContinue -Force
rmdir x64 -Force -ErrorAction SilentlyContinue
Write-Host "[+] Limpeza completa`n" -ForegroundColor Green

# Rebuild DLL project only
Write-Host "[2/3] Compilando VltChallengeDll via MSBuild..." -ForegroundColor Yellow
& $msbuild VltChallengeDll.sln `
    /p:Configuration=Debug `
    /p:Platform=x64 `
    /t:VltChallengeDll:Rebuild `
    /v:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Host "[-] Compilacao falhou!" -ForegroundColor Red
    exit 1
}

Write-Host "[+] Compilacao bem-sucedida!`n" -ForegroundColor Green

# Check DLL
Write-Host "[3/3] Verificando DLL..." -ForegroundColor Yellow
$dllPath = "x64\Debug\VltChallengeDll.dll"

if (Test-Path $dllPath) {
    $size = (Get-Item $dllPath).Length
    $time = (Get-Item $dllPath).LastWriteTime
    Write-Host "[+] DLL compilada com sucesso!" -ForegroundColor Green
    Write-Host "    Path: $dllPath" -ForegroundColor Green
    Write-Host "    Size: $size bytes" -ForegroundColor Green
    Write-Host "    Time: $time`n" -ForegroundColor Green
} else {
    Write-Host "[-] DLL nao encontrada!" -ForegroundColor Red
    exit 1
}

# Now run injector
Write-Host "[4/4] Abrindo notepad e executando injector..." -ForegroundColor Cyan

# Kill existing notepad
taskkill /im notepad.exe /F 2>$null

# Open new notepad
Start-Process notepad
Start-Sleep -Seconds 2

# Run injector
Write-Host "`n================================================" -ForegroundColor Cyan
Write-Host "[*] EXECUTANDO INJECTOR..." -ForegroundColor Cyan
Write-Host "================================================`n" -ForegroundColor Cyan

Push-Location x64\Debug
& .\Injector.exe
Pop-Location

Write-Host "`n================================================" -ForegroundColor Cyan
Write-Host "[SUCCESS] Verifique o Notepad para as MessageBoxes!" -ForegroundColor Green
Write-Host "================================================`n" -ForegroundColor Cyan

Read-Host "Pressione Enter para sair"
