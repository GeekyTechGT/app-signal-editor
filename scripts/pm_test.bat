@echo off
REM ============================================================
REM  pm_test.bat - Run Tests with HTML Report
REM ============================================================

setlocal enabledelayedexpansion
for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"
set "PROJECT_ROOT=%CD%"
set "BUILD_DIR=build"
set "TEST_RESULTS_DIR=tests\06.results"

:main
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Run Tests with HTML Report - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Test Type:!RESET!
echo   !GREEN![1]!RESET! Run All Tests
echo   !GREEN![2]!RESET! Run Unit Tests Only
echo   !GREEN![3]!RESET! Run E2E Tests Only
echo   !GREEN![0]!RESET! Back
echo.
set /p "test_type_choice=  Select test type [0-3]: "
if "%test_type_choice%"=="0" goto done
if "%test_type_choice%"=="1" set "TEST_FILTER=" & set "TEST_TYPE_NAME=all" & set "TEST_TYPE_LABEL=All Tests"
if "%test_type_choice%"=="2" set "TEST_FILTER=-E _e2e$" & set "TEST_TYPE_NAME=unit" & set "TEST_TYPE_LABEL=Unit Tests"
if "%test_type_choice%"=="3" set "TEST_FILTER=-R _e2e$" & set "TEST_TYPE_NAME=e2e" & set "TEST_TYPE_LABEL=E2E Tests"
if not defined TEST_TYPE_NAME goto done

echo.
echo   !BLUE!Select Configuration:!RESET!
echo   !GREEN![1]!RESET! windows-gcc-debug
echo   !GREEN![2]!RESET! windows-gcc-release
echo   !GREEN![3]!RESET! windows-clang-debug
echo   !GREEN![4]!RESET! windows-clang-release
echo   !GREEN![5]!RESET! windows-MSVC-debug
echo   !GREEN![6]!RESET! windows-MSVC-release
echo   !GREEN![7]!RESET! Show Linux host test commands
echo   !GREEN![0]!RESET! Back
echo.
set /p "preset_choice=  Select configuration [0-7]: "
if "%preset_choice%"=="0" goto done
if "%preset_choice%"=="1" set "TEST_PRESET=windows-gcc-debug"
if "%preset_choice%"=="2" set "TEST_PRESET=windows-gcc-release"
if "%preset_choice%"=="3" set "TEST_PRESET=windows-clang-debug"
if "%preset_choice%"=="4" set "TEST_PRESET=windows-clang-release"
if "%preset_choice%"=="5" set "TEST_PRESET=windows-MSVC-debug"
if "%preset_choice%"=="6" set "TEST_PRESET=windows-MSVC-release"
if "%preset_choice%"=="7" goto linux_commands
if not defined TEST_PRESET goto done

if not exist "%TEST_RESULTS_DIR%" mkdir "%TEST_RESULTS_DIR%"
for /f "tokens=*" %%t in ('powershell -Command "Get-Date -Format 'yyyyMMdd_HHmmss'"') do set "TIMESTAMP=%%t"
set "XML_OUTPUT=%TEST_RESULTS_DIR%\test_results_%TEST_TYPE_NAME%_%TEST_PRESET%_%TIMESTAMP%.xml"
set "HTML_OUTPUT=%TEST_RESULTS_DIR%\test_results_%TEST_TYPE_NAME%_%TEST_PRESET%_%TIMESTAMP%.html"

if not exist "%BUILD_DIR%\%TEST_PRESET%" (
    echo !RED![ERROR]!RESET! Build directory not found: %BUILD_DIR%\%TEST_PRESET%
    pause
    goto done
)

cd /d "%PROJECT_ROOT%\%BUILD_DIR%\%TEST_PRESET%"
set "ORIGINAL_PATH=%PATH%"
echo %TEST_PRESET% | findstr /i "windows-gcc" >nul
if not errorlevel 1 set "PATH=C:\eng_apps\msys64\mingw64\bin;%PATH%"
echo %TEST_PRESET% | findstr /i "windows-clang" >nul
if not errorlevel 1 set "PATH=C:\Program Files\LLVM\bin;C:\eng_apps\msys64\mingw64\bin;%PATH%"
ctest %TEST_FILTER% --output-junit "%PROJECT_ROOT%\%XML_OUTPUT%" --output-on-failure -V
set "PATH=%ORIGINAL_PATH%"
cd /d "%PROJECT_ROOT%"

if not exist "%XML_OUTPUT%" (
    echo !RED![ERROR]!RESET! Test XML output not generated.
    pause
    goto done
)

call "%PROJECT_ROOT%\scripts\report_generate_html.bat" "%PROJECT_ROOT%\%XML_OUTPUT%" "%PROJECT_ROOT%\%HTML_OUTPUT%" "%TEST_PRESET%" "%PROJECT_ROOT%\%BUILD_DIR%\%TEST_PRESET%"
if exist "%PROJECT_ROOT%\%HTML_OUTPUT%" (
    echo !GREEN![OK]!RESET! HTML report generated: %PROJECT_ROOT%\%HTML_OUTPUT%
)
pause
goto done

:linux_commands
echo.
echo   !BLUE!Select Linux Compiler:!RESET!
echo   !GREEN![1]!RESET! GCC
echo   !GREEN![2]!RESET! Clang
echo   !GREEN![0]!RESET! Back
echo.
set /p "linux_choice=  Select compiler [0-2]: "
if "%linux_choice%"=="0" goto done
if "%linux_choice%"=="1" set "LINUX_COMPILER=gcc"
if "%linux_choice%"=="2" set "LINUX_COMPILER=clang"
if not defined LINUX_COMPILER goto done

echo.
echo   !BLUE!Select Build Type:!RESET!
echo   !GREEN![1]!RESET! Debug
echo   !GREEN![2]!RESET! Release
echo   !GREEN![0]!RESET! Back
echo.
set /p "linux_build_choice=  Select build type [0-2]: "
if "%linux_build_choice%"=="0" goto done
if "%linux_build_choice%"=="1" set "LINUX_BUILD=debug"
if "%linux_build_choice%"=="2" set "LINUX_BUILD=release"
if not defined LINUX_BUILD goto done
set "LINUX_PRESET=linux-%LINUX_COMPILER%-%LINUX_BUILD%"
echo.
echo !YELLOW![INFO]!RESET! Run Linux-host tests from a Linux shell using:
echo.
echo     ctest --preset %LINUX_PRESET% --output-on-failure
echo.
pause

:done
exit /b 0
