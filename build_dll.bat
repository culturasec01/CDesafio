@echo off
REM Script para compilar VltChallengeDll.dll rapidamente

echo [*] Compilando VltChallengeDll...
echo.

cd VltChallengeDll

REM Compilar com MSVC
cl.exe /c pch.cpp /Fo:pch.obj
cl.exe /c dllmain.cpp /Fo:dllmain.obj
cl.exe /c main.cpp /Fo:main.obj

REM Linkar para DLL
link.exe /DLL /OUT:..\x64\Debug\VltChallengeDll.dll pch.obj dllmain.obj main.obj kernel32.lib user32.lib

cd ..

echo.
echo [+] Compilacao concluida!
echo [*] DLL gerada em: x64\Debug\VltChallengeDll.dll
echo.
pause
