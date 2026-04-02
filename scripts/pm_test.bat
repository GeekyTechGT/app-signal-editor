@echo off
REM ============================================================
REM  pm_test.bat - Run Tests with HTML Report
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_test.bat
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

:run_tests_html
cls
set "TEST_FILTER="
set "TEST_TYPE_NAME="
set "TEST_TYPE_LABEL="
set "TEST_PRESET="
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Run Tests with HTML Report - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Test Type:!RESET!
echo   !GREEN![1]!RESET! Run All Tests
echo   !GREEN![2]!RESET! Run Unit Tests Only
echo   !GREEN![3]!RESET! Run E2E Tests Only
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "test_type_html_choice=  Select test type [0-3]: "

if "%test_type_html_choice%"=="0" goto done
if "%test_type_html_choice%"=="1" set "TEST_FILTER=" & set "TEST_TYPE_NAME=all"  & set "TEST_TYPE_LABEL=All Tests"
if "%test_type_html_choice%"=="2" set "TEST_FILTER=-E _e2e$" & set "TEST_TYPE_NAME=unit" & set "TEST_TYPE_LABEL=Unit Tests"
if "%test_type_html_choice%"=="3" set "TEST_FILTER=-R _e2e$" & set "TEST_TYPE_NAME=e2e"  & set "TEST_TYPE_LABEL=E2E Tests"

if not defined TEST_TYPE_NAME (
    echo !RED![ERROR]!RESET! Invalid choice.
    timeout /t 2 >nul
    goto run_tests_html
)

echo.
echo   !BLUE!Select Configuration:!RESET!
echo   !GREEN![1]!RESET! GCC Debug
echo   !GREEN![2]!RESET! GCC Release
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "test_html_choice=  Select configuration [0-2]: "

if "%test_html_choice%"=="0" goto done
if "%test_html_choice%"=="1" set "TEST_PRESET=windows-mingw64-debug"
if "%test_html_choice%"=="2" set "TEST_PRESET=windows-mingw64-release"

if not defined TEST_PRESET (
    echo !RED![ERROR]!RESET! Invalid choice.
    timeout /t 2 >nul
    goto run_tests_html
)

if not exist "%TEST_RESULTS_DIR%" mkdir "%TEST_RESULTS_DIR%"

for /f "tokens=*" %%t in ('powershell -Command "Get-Date -Format 'yyyyMMdd_HHmmss'"') do set "TIMESTAMP=%%t"
set "XML_OUTPUT=%TEST_RESULTS_DIR%\test_results_%TEST_TYPE_NAME%_%TEST_PRESET%_%TIMESTAMP%.xml"
set "HTML_OUTPUT=%TEST_RESULTS_DIR%\test_results_%TEST_TYPE_NAME%_%TEST_PRESET%_%TIMESTAMP%.html"

echo.
echo !YELLOW![INFO]!RESET! Running %TEST_TYPE_LABEL% with preset: %TEST_PRESET%
echo !YELLOW![INFO]!RESET! XML Output: %XML_OUTPUT%
echo.

if not exist "%BUILD_DIR%\%TEST_PRESET%" (
    echo !RED![ERROR]!RESET! Build directory not found: %BUILD_DIR%\%TEST_PRESET%
    echo         Please build the project first.
    pause
    goto done
)

cd "%PROJECT_ROOT%\%BUILD_DIR%\%TEST_PRESET%"
set "ORIGINAL_PATH=%PATH%"
echo %TEST_PRESET% | findstr /i "mingw64" >nul
if not errorlevel 1 (
    set "PATH=C:\eng_apps\msys64\mingw64\bin;%PATH%"
)
ctest %TEST_FILTER% --output-junit "%PROJECT_ROOT%\%XML_OUTPUT%" --output-on-failure -V
set "PATH=%ORIGINAL_PATH%"
cd /d "%PROJECT_ROOT%"

if not exist "%XML_OUTPUT%" (
    echo !RED![ERROR]!RESET! Test XML output not generated.
    pause
    goto done
)

echo.
echo !YELLOW![INFO]!RESET! Generating HTML report...

call "%PROJECT_ROOT%\scripts\report_generate_html.bat" "%PROJECT_ROOT%\%XML_OUTPUT%" "%PROJECT_ROOT%\%HTML_OUTPUT%" "%TEST_PRESET%" "%PROJECT_ROOT%\%BUILD_DIR%\%TEST_PRESET%"

if exist "%PROJECT_ROOT%\%HTML_OUTPUT%" (
    echo !GREEN![OK]!RESET! HTML report generated: %PROJECT_ROOT%\%HTML_OUTPUT%
    echo.
    set /p "open_report=  Open report in browser? (y/N): "
    if /i "!open_report!"=="y" (
        start "" "%PROJECT_ROOT%\%HTML_OUTPUT%"
    )
) else (
    echo !YELLOW![WARN]!RESET! Could not generate HTML report.
)

echo.
pause
set "TEST_FILTER="
set "TEST_TYPE_NAME="
set "TEST_TYPE_LABEL="
set "TEST_PRESET="
goto done

:done
exit /b 0
