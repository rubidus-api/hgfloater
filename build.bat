@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: Calculate Version String based on current date (vYY.MM.DD)
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set ver_y=!datetime:~2,2!
set ver_m=!datetime:~4,2!
set ver_d=!datetime:~6,2!
set VERSION_STRING=v!ver_y!.!ver_m!.!ver_d!

:: MSYS2 Build Script for HGFloater
:: Requirements: MSYS2 with mingw-w64-x86_64-gcc installed

:: Common Warning Flags for robust error detection
set WARNING_FLAGS=-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion -Wconversion -Wlogical-op -Wno-unused-parameter

:menu
echo ========================================
echo HGFloater (HellGates Series) MSYS2 Build
echo ========================================
echo 1. Debug Build   (Warnings, Symbols, No Opt)
echo 2. Release Build (Warnings, Optimized, Stripped)
echo 3. Exit
echo.

set /p choice="Select build mode (1, 2, or 3): "

if "%choice%"=="1" (
    set FLAGS=%WARNING_FLAGS% -g -O0 -DDEBUG
    set MODE=Debug
) else if "%choice%"=="2" (
    set FLAGS=%WARNING_FLAGS% -O2 -s
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

echo Building HGFloater [%MODE%] [Version !VERSION_STRING!] using GCC...
:: Using standard MinGW-w64 GCC command
gcc hgfloater.c -o hgfloater.exe %FLAGS% -DHG_VERSION_W=L\"!VERSION_STRING!\" -lgdi32 -luser32 -lcomctl32 -ldwmapi -mwindows -lshell32 -lpsapi -static

if %errorlevel% equ 0 (
    echo.
    echo [Success] hgfloater.exe [%MODE%] has been created!
    echo Starting HGFloater...
    start hgfloater.exe
) else (
    echo.
    echo [Error] Build failed. Make sure GCC is in your PATH ^(MSYS2 MinGW64^).
    pause
)

pause
goto menu
