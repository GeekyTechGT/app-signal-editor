@echo off
REM ============================================================
REM  Project Manager - Signal Editor
REM ============================================================
REM  Main project management script for common operations
REM ============================================================

setlocal enabledelayedexpansion

title Signal Editor - Project Manager

set "DOCKER_BIN=C:\Program Files\Docker\Docker\resources\bin"
set "PATH=%DOCKER_BIN%;%PATH%"

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "MAGENTA=!ESC![35m"
set "RESET=!ESC![0m"

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"
set "INIT_PROJECT_SCRIPT=%PROJECT_ROOT%\scripts\init_project.bat"
set "CUSTOMIZE_PROJECT_SCRIPT=%PROJECT_ROOT%\scripts\customize_project.bat"
set "INIT_SUBMODULES_SCRIPT=%PROJECT_ROOT%\scripts\init_submodules.bat"
set "PM_HELPERS_SCRIPT=%PROJECT_ROOT%\scripts\pm_helpers.bat"
set "METADATA_FILE=%PROJECT_ROOT%\project.metadata.json"

if not exist "%PM_HELPERS_SCRIPT%" (
    echo.
    echo !RED![ERROR]!RESET! Missing helper module: %PM_HELPERS_SCRIPT%
    echo         Please restore this file before running Project Manager.
    echo.
    pause
    exit /b 1
)

:main_menu
cls
echo.
echo !CYAN!======================================================!RESET!
echo !CYAN!         Signal Editor - Project Manager!RESET!
echo !CYAN!======================================================!RESET!
echo.
echo   !BLUE!Project Setup!RESET!
echo   !GREEN![1]!RESET! Initialize Project
echo   !GREEN![2]!RESET! Customize Project
echo.
echo   !BLUE!Build Operations!RESET!
echo   !GREEN![3]!RESET! Build Project
echo   !GREEN![4]!RESET! Clean Build
echo   !GREEN![5]!RESET! Clean All Builds
echo.
echo   !BLUE!Test Operations!RESET!
echo   !GREEN![6]!RESET! Run Tests with HTML Report
echo   !GREEN![7]!RESET! Run Test Pipeline
echo.
echo   !BLUE!Deployment!RESET!
echo   !GREEN![8]!RESET! Deploy
echo   !GREEN![9]!RESET! Generate Installer
echo.
echo   !BLUE!Other!RESET!
echo   !GREEN![10]!RESET! Init Submodules
echo   !GREEN![11]!RESET! Git Hooks Status
echo   !GREEN![0]!RESET! Exit
echo.
echo !MAGENTA!Preset matrix exposed by this repository:!RESET!
echo   Windows: `windows-gcc`, `windows-clang`, `windows-MSVC` x `debug/release`
echo   Linux:   `linux-gcc`, `linux-clang` x `debug/release`
echo   Docker:  `docker-gcc`, `docker-clang` x `debug/release`
echo.
echo !CYAN!======================================================!RESET!
echo.

set /p "choice=  Select an option [0-11]: "

if "%choice%"=="1"  goto init_project
if "%choice%"=="2"  goto customize_project
if "%choice%"=="3"  goto build_project
if "%choice%"=="4"  goto clean_build
if "%choice%"=="5"  goto clean_all
if "%choice%"=="6"  goto run_tests_html
if "%choice%"=="7"  goto pipeline_menu
if "%choice%"=="8"  goto deploy_menu
if "%choice%"=="9"  goto generate_installer
if "%choice%"=="10" goto init_submodules
if "%choice%"=="11" goto status_menu
if "%choice%"=="0"  goto exit_script

echo.
echo !RED![ERROR]!RESET! Invalid option. Please try again.
timeout /t 2 >nul
goto main_menu

:init_project
cls
echo.
if exist "%INIT_PROJECT_SCRIPT%" (
    cd /d "%PROJECT_ROOT%"
    call "%INIT_PROJECT_SCRIPT%"
) else (
    echo !RED![ERROR]!RESET! Script not found: %INIT_PROJECT_SCRIPT%
)
echo.
pause
goto main_menu

:customize_project
cls
echo.
if exist "%CUSTOMIZE_PROJECT_SCRIPT%" (
    call "%CUSTOMIZE_PROJECT_SCRIPT%"
) else (
    echo !RED![ERROR]!RESET! Script not found: %CUSTOMIZE_PROJECT_SCRIPT%
)
echo.
pause
goto main_menu

:build_project
call scripts\pm_build.bat
goto main_menu

:clean_build
call scripts\pm_clean.bat clean_build
goto main_menu

:clean_all
call scripts\pm_clean.bat clean_all
goto main_menu

:run_tests_html
call scripts\pm_test.bat
goto main_menu

:deploy_menu
call scripts\pm_deploy.bat deploy
goto main_menu

:generate_installer
call scripts\pm_deploy.bat installer
goto main_menu

:init_submodules
cls
echo.
if exist "%INIT_SUBMODULES_SCRIPT%" (
    call "%INIT_SUBMODULES_SCRIPT%"
) else (
    echo !RED![ERROR]!RESET! Script not found: %INIT_SUBMODULES_SCRIPT%
)
echo.
pause
goto main_menu

:pipeline_menu
call scripts\pm_pipeline.bat
goto main_menu

:status_menu
call scripts\pm_status.bat
goto main_menu

:exit_script
echo.
echo !GREEN![DONE]!RESET! Exiting Project Manager.
exit /b 0
