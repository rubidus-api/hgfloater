@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: Calculate Version String based on current date (vYY.MM.DD).
:: wmic is removed from current Windows 11 builds, so the date comes from PowerShell.
for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yy.MM.dd"') do set datever=%%I
set ver_y=!datever:~0,2!
set ver_m=!datever:~3,2!
set ver_d=!datever:~6,2!
set VERSION_STRING=v!ver_y!.!ver_m!.!ver_d!
:: Numeric (unpadded) components for the VERSIONINFO resource; leading zeros
:: would parse as octal in the resource compiler.
set /a VER_NUM_Y=1!ver_y! - 100
set /a VER_NUM_M=1!ver_m! - 100
set /a VER_NUM_D=1!ver_d! - 100

:: MSYS2 Build Script for hgfloater
:: Requirements: MSYS2 with mingw-w64-x86_64-gcc installed

:: Common Warning Flags for robust error detection
set WARNING_FLAGS=-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion -Wconversion -Wlogical-op -Wno-unused-parameter -Wno-overlength-strings

:: Non-interactive usage: build.bat debug ^| release ^| test
:: (skips the menu, does not auto-start the exe, and exits with the build result)
set NONINTERACTIVE=
set choice=
if /i "%~1"=="debug" set choice=1
if /i "%~1"=="release" set choice=2
if /i "%~1"=="test" set choice=3
if defined choice (
    set NONINTERACTIVE=1
    goto dispatch
)

:menu
echo ========================================
echo hgfloater (HellGates Series) MSYS2 Build
echo ========================================
echo 1. Debug Build   (Warnings, Symbols, No Opt)
echo 2. Release Build (Warnings, Optimized, Stripped)
echo 3. Run Tests
echo 4. Exit
echo.

set /p choice="Select build mode (1, 2, 3, or 4): "

:dispatch
if "%choice%"=="1" (
    set FLAGS=%WARNING_FLAGS% -g -O0 -DDEBUG
    set MODE=Debug
) else if "%choice%"=="2" (
    set FLAGS=%WARNING_FLAGS% -O3 -flto=auto -s -DNDEBUG
    set MODE=Release
) else if "%choice%"=="3" (
    goto run_tests
) else if "%choice%"=="4" (
    echo Exiting...
    exit /b 0
) else (
    echo [Error] Invalid choice.
    goto menu
)

echo.
echo Closing existing hgfloater instance if running...
taskkill /f /im hgfloater.exe >nul 2>&1

echo Building hgfloater [%MODE%] [Version !VERSION_STRING!] using GCC...

:: Generate hg_about_text.h from README.md (scripts\gen_about.py mirrors this for non-Windows hosts)
echo Generating About text from README.md...
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\gen_about.ps1

:: Using standard MinGW-w64 GCC command
windres hgfloater.rc -O coff -o hgfloater_res.o -DHG_RC_VER_MAJOR=!VER_NUM_Y! -DHG_RC_VER_MINOR=!VER_NUM_M! -DHG_RC_VER_PATCH=!VER_NUM_D! -DHG_RC_VER_MINOR_STR=!ver_m! -DHG_RC_VER_PATCH_STR=!ver_d!
gcc hgfloater.c hg_globals.c hg_utils.c hg_config.c hg_calc.c widgets\hg_floater.c widgets\hg_taskbox.c widgets\hg_toolbar.c widgets\hg_taskbox_menus.c widgets\hg_window_list.c widgets\hg_monitor.c widgets\hg_commandbox.c widgets\hg_about.c hgfloater_res.o -o hgfloater.exe %FLAGS% -Wno-overlength-strings -DHG_VERSION_W=L\"!VERSION_STRING!\" -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -mwindows -municode -lshell32 -lole32 -loleaut32 -luuid -lpsapi -lpathcch -lshlwapi -static -lshcore -lpropsys -limm32
set BUILD_RESULT=%errorlevel%
if exist hgfloater_res.o del hgfloater_res.o

if %BUILD_RESULT% equ 0 (
    echo.
    echo [Success] hgfloater.exe [%MODE%] has been created!
    if not defined NONINTERACTIVE (
        echo Starting hgfloater...
        start hgfloater.exe
    )
) else (
    echo.
    echo [Error] Build failed. Make sure GCC is in your PATH ^(MSYS2 MinGW64^).
)

if defined NONINTERACTIVE exit /b %BUILD_RESULT%
pause
goto menu

:run_tests
echo.
echo Running Tests in test\ directory...
if not exist test\bin mkdir test\bin
set TEST_FAILED=0
for %%f in (test\*.c) do (
    echo Compiling %%f...
    gcc %%f -o test\bin\%%~nf.exe %WARNING_FLAGS% -Wno-overlength-strings -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lpsapi -lpathcch -lshlwapi -lshcore -lpropsys -limm32
    if !errorlevel! neq 0 (
        echo [Error] Failed to compile %%f
        set TEST_FAILED=1
    ) else (
        echo Running test\bin\%%~nf.exe...
        test\bin\%%~nf.exe
        if !errorlevel! neq 0 (
            echo [Failed] %%f returned !errorlevel!
            set TEST_FAILED=1
        ) else (
            echo [Passed] %%f
        )
    )
)
if !TEST_FAILED! equ 0 (
    echo.
    echo [All Tests Passed]
) else (
    echo.
    echo [Some Tests Failed]
)
if defined NONINTERACTIVE exit /b !TEST_FAILED!
pause
goto menu
