@echo off
setlocal enabledelayedexpansion

:menu
echo ========================================
echo HGFloater (HellGates Series) Build Menu
echo ========================================
echo 1. Debug Build   (Symbols, No Optimization)
echo 2. Release Build (Optimized, Stripped)
echo 3. Exit
echo.

set /p choice="Select build mode (1, 2, or 3): "

if "%choice%"=="1" (
    set FLAGS=-g -O0 -DDEBUG
    set MODE=Debug
) else if "%choice%"=="2" (
    set FLAGS=-O2 -s
    set MODE=Release
) else if "%choice%"=="3" (
    echo Exiting...
    exit /b 0
) else (
    echo [Error] Invalid choice.
    goto menu
)

echo.
echo Closing existing HGFloater instance if running...
taskkill /f /im hgfloater.exe >nul 2>&1

echo Building HGFloater [%MODE%]...
gcc hgfloater.c -o hgfloater.exe %FLAGS% -lgdi32 -luser32 -lcomctl32 -ldwmapi -mwindows -lshell32 -static

if %errorlevel% equ 0 (
    echo.
    echo [Success] hgfloater.exe [%MODE%] has been created!
    echo Starting HGFloater...
    start hgfloater.exe
) else (
    echo.
    echo [Error] Build failed.
)

goto menu
