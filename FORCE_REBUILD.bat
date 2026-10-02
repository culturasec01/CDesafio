@echo off
REM Force total rebuild - delete all build artifacts

echo [*] Limpando TODOS os arquivos de build...

REM Delete all obj files
del /S /Q VltChallengeDll\*.obj 2>nul
del /S /Q VltChallengeDll\*.pdb 2>nul
del /S /Q VltChallengeDll\*.ilk 2>nul

REM Delete all build folders
rmdir /S /Q x64 2>nul
rmdir /S /Q VltChallengeDll\x64 2>nul
rmdir /S /Q Injector\x64 2>nul
rmdir /S /Q Target\x64 2>nul
rmdir /S /Q VtlChallengeTarget\bin 2>nul
rmdir /S /Q VtlChallengeTarget\obj 2>nul

echo [+] Limpeza completa
echo.
echo [*] Abrindo Visual Studio...
start VltChallengeDll.sln

echo.
echo [!] NO VISUAL STUDIO:
echo    1. Build Menu ^ Rebuild Solution
echo    2. Aguarde completar
echo    3. Apos compilacao, feche VS
echo    4. Execute compile_with_hook.ps1
echo.
pause
