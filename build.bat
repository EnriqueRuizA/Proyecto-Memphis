@echo off
rem ============================================================
rem build.bat - Compila Medieval Conquest a rts_game.exe (raiz)
rem Toolchain: w64devkit (MinGW-w64) en C:\raylib\w64devkit
rem Log completo: build\build.log
rem ============================================================
setlocal
cd /d "%~dp0"
set PATH=C:\raylib\w64devkit\bin;%PATH%
set SYSFIND=%SystemRoot%\System32\find.exe

if not exist build mkdir build
if exist rts_game.exe del rts_game.exe

rem -O2 desde Fase A (antes -O1): +velocidad, coste de compilacion ~+5s
g++ -std=c++17 -O2 -Wall -Wextra -Isrc src\*.cpp -o rts_game.exe -L"C:\raylib\raylib\src" -lraylib -lopengl32 -lgdi32 -lwinmm 2> build\build.log

rem errores de compilacion/enlace (el find de w64devkit es GNU: usar el del sistema)
set ERRS=0
set WARNS=0
for /f %%A in ('%SYSFIND% /c ": error" ^< build\build.log') do set ERRS=%%A
for /f %%A in ('%SYSFIND% /c ": warning" ^< build\build.log') do set WARNS=%%A

if not exist rts_game.exe (
    echo BUILD FAILED - sin salida [errors=%ERRS% warnings=%WARNS%] ver build\build.log
    exit /b 1
)
if not "%ERRS%"=="0" (
    echo BUILD FAILED - [errors=%ERRS% warnings=%WARNS%] ver build\build.log
    exit /b 1
)
echo BUILD OK - rts_game.exe [errors=%ERRS% warnings=%WARNS%]
exit /b 0
