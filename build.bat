@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: Calculate Version String based on current date (vYY.MM.DD)
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set ver_y=!datetime:~2,2!
set ver_m=!datetime:~4,2!
set ver_d=!datetime:~6,2!
set VERSION_STRING=v!ver_y!.!ver_m!.!ver_d!

:: MSYS2 Build Script for hgfloater
:: Requirements: MSYS2 with mingw-w64-x86_64-gcc installed

:: Common Warning Flags for robust error detection
set WARNING_FLAGS=-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion -Wconversion -Wlogical-op -Wno-unused-parameter -Wno-overlength-strings

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

if "%choice%"=="1" (
    set FLAGS=%WARNING_FLAGS% -g -O0 -DDEBUG
    set MODE=Debug
) else if "%choice%"=="2" (
    set FLAGS=%WARNING_FLAGS% -O2 -s
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

:: Generate hg_about_text.h from README.md
echo Generating About text from README.md...
echo $readmePath = 'README.md' > gen_about.ps1
echo $outputPath = 'hg_about_text.h' >> gen_about.ps1
echo if ^(Test-Path $readmePath^) { >> gen_about.ps1
echo     [byte[]]$bytes = [System.IO.File]::ReadAllBytes^($readmePath^) >> gen_about.ps1
echo     $enc = New-Object System.Text.UTF8Encoding^($false^) >> gen_about.ps1
echo     $readme = $enc.GetString^($bytes^) >> gen_about.ps1
echo     $lines = $readme -split '\r?\n' >> gen_about.ps1
echo     $skip = $false >> gen_about.ps1
echo     $output = @^(^) >> gen_about.ps1
echo     foreach ^($line in $lines^) { >> gen_about.ps1
echo         if ^($line.Contains^('^<^^!-- SKIP_START --^>'^)^) { $skip = $true; continue } >> gen_about.ps1
echo         if ^($line.Contains^('^<^^!-- SKIP_END --^>'^)^) { $skip = $false; continue } >> gen_about.ps1
echo         if ^($skip -or $line.Contains^('^<^^!-- SKIP --^>'^) -or $line.Trim^(^).StartsWith^('^^!['^)^) { continue } >> gen_about.ps1
echo         $escaped = $line.Replace^('\', '\\'^).Replace^('"', '\"'^) >> gen_about.ps1
echo         $output += "L`"$escaped\r\n`"" >> gen_about.ps1
echo     } >> gen_about.ps1
echo     $joined = $output -join ' ' >> gen_about.ps1
echo     $content = "#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W $joined`r`n#endif" >> gen_about.ps1
echo     [System.IO.File]::WriteAllText^($outputPath, $content, $enc^) >> gen_about.ps1
echo     Write-Host "[Success] README.md processed successfully." -ForegroundColor Green >> gen_about.ps1
echo } else { >> gen_about.ps1
echo     $content = '#ifndef HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_TEXT_H`r`n#define HG_ABOUT_README_W L"(README.md not found)"`r`n#endif' >> gen_about.ps1
echo     Set-Content -Path $outputPath -Value $content -Encoding UTF8 >> gen_about.ps1
echo     Write-Host "[Warning] README.md not found." -ForegroundColor Yellow >> gen_about.ps1
echo } >> gen_about.ps1

powershell -NoProfile -ExecutionPolicy Bypass -File gen_about.ps1
if exist gen_about.ps1 del gen_about.ps1

:: Using standard MinGW-w64 GCC command
windres hgfloater.rc -O coff -o hgfloater_res.o
gcc hgfloater.c hgfloater_res.o -o hgfloater.exe %FLAGS% -Wno-overlength-strings -DHG_VERSION_W=L\"!VERSION_STRING!\" -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -mwindows -municode -lshell32 -lole32 -loleaut32 -luuid -lpsapi -lpathcch -lshlwapi -static -lshcore -lpropsys 
if exist hgfloater_res.o del hgfloater_res.o

if %errorlevel% equ 0 (
    echo.
    echo [Success] hgfloater.exe [%MODE%] has been created!
    echo Starting hgfloater...
    start hgfloater.exe
) else (
    echo.
    echo [Error] Build failed. Make sure GCC is in your PATH ^(MSYS2 MinGW64^).
)

pause
goto menu

:run_tests
echo.
echo Running Tests in tests\ directory...
if not exist tests\bin mkdir tests\bin
set TEST_FAILED=0
for %%f in (tests\*.c) do (
    echo Compiling %%f...
    gcc %%f -o tests\bin\%%~nf.exe %WARNING_FLAGS% -Wno-overlength-strings -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -mwindows -municode -lshell32 -lole32 -loleaut32 -luuid -lpsapi -lpathcch -lshlwapi -lshcore
    if !errorlevel! neq 0 (
        echo [Error] Failed to compile %%f
        set TEST_FAILED=1
    ) else (
        echo Running tests\bin\%%~nf.exe...
        tests\bin\%%~nf.exe
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
pause
goto menu
