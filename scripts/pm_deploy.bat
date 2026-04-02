@echo off
REM ============================================================
REM  pm_deploy.bat - Deploy / Generate Installer
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_deploy.bat [deploy|installer]
REM ============================================================

setlocal enabledelayedexpansion

set "DOCKER_BIN=C:\Program Files\Docker\Docker\resources\bin"
set "PATH=%DOCKER_BIN%;%PATH%"

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

set "BUILD_DIR=build"
set "DEPLOY_DIR=deploy"
set "PM_HELPERS_SCRIPT=scripts\pm_helpers.bat"

if /i "%~1"=="installer" goto generate_installer
goto deploy_menu

REM ============================================================
:deploy_menu
REM ============================================================
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Deploy - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Target:!RESET!
echo   !GREEN![1]!RESET! Windows Debug
echo   !GREEN![2]!RESET! Windows Release
echo   !GREEN![3]!RESET! Linux Ubuntu Debug
echo   !GREEN![4]!RESET! Linux Ubuntu Release
echo   !GREEN![5]!RESET! Linux Alpine Debug
echo   !GREEN![6]!RESET! Linux Alpine Release
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "deploy_choice=  Select target [0-6]: "

if "%deploy_choice%"=="0" goto done
if "%deploy_choice%"=="1" call :do_deploy_windows debug   & goto deploy_done
if "%deploy_choice%"=="2" call :do_deploy_windows release & goto deploy_done
if "%deploy_choice%"=="3" call :do_deploy_linux ubuntu debug   & goto deploy_done
if "%deploy_choice%"=="4" call :do_deploy_linux ubuntu release & goto deploy_done
if "%deploy_choice%"=="5" call :do_deploy_linux alpine debug   & goto deploy_done
if "%deploy_choice%"=="6" call :do_deploy_linux alpine release & goto deploy_done

echo !RED![ERROR]!RESET! Invalid choice.
timeout /t 2 >nul
goto deploy_menu

:deploy_done
echo.
pause
goto deploy_menu

REM ============================================================
:generate_installer
REM ============================================================
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Generate Installer - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo !YELLOW![INFO]!RESET! Installer generation placeholder.
echo         Integrate Inno Setup (Windows) or package_deploy_linux.sh (Linux) here.
echo.
pause
goto done

REM ============================================================
REM  Windows Deploy Subroutine
REM  Parameters: %1=build_type (debug/release)
REM ============================================================
:do_deploy_windows
set "WIN_BUILD_TYPE=%~1"
if "%WIN_BUILD_TYPE%"=="" set "WIN_BUILD_TYPE=release"

if /i "%WIN_BUILD_TYPE%"=="debug" (
    set "DEPLOY_PRESET=windows-mingw64-debug"
) else (
    set "DEPLOY_PRESET=windows-mingw64-release"
)

set "BUILD_BIN_DIR=%BUILD_DIR%\%DEPLOY_PRESET%\bin"
set "DEPLOY_TARGET=%DEPLOY_DIR%\windows\%WIN_BUILD_TYPE%"

echo.
echo !YELLOW![INFO]!RESET! Deploying Windows %WIN_BUILD_TYPE%...
echo !YELLOW![INFO]!RESET! Source: %BUILD_BIN_DIR%
echo !YELLOW![INFO]!RESET! Target: %DEPLOY_TARGET%
echo.

call "%PM_HELPERS_SCRIPT%" :ensure_directory_exists "%BUILD_BIN_DIR%" "Build directory not found: %BUILD_BIN_DIR%"
if errorlevel 1 (
    echo         Please build the project first.
    exit /b 1
)

call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DEPLOY_TARGET%"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to prepare deploy directory.
    exit /b 1
)

copy "%BUILD_BIN_DIR%\*.exe" "%DEPLOY_TARGET%\" >nul 2>&1
copy "%BUILD_BIN_DIR%\*.dll" "%DEPLOY_TARGET%\" >nul 2>&1

echo !GREEN![OK]!RESET! Windows %WIN_BUILD_TYPE% deployment completed: %DEPLOY_TARGET%
dir /b "%DEPLOY_TARGET%"
exit /b 0

REM ============================================================
REM  Linux Deploy Subroutine
REM  Parameters: %1=distro (ubuntu/alpine), %2=build_type
REM ============================================================
:do_deploy_linux
set "DISTRO=%~1"
set "LINUX_BUILD_TYPE=%~2"
if "%DISTRO%"==""          set "DISTRO=ubuntu"
if "%LINUX_BUILD_TYPE%"=="" set "LINUX_BUILD_TYPE=release"

set "LINUX_BUILD_ARTIFACTS=%BUILD_DIR%\linux-%DISTRO%-%LINUX_BUILD_TYPE%\artifacts"
set "DEPLOY_TARGET=%DEPLOY_DIR%\linux\%DISTRO%\%LINUX_BUILD_TYPE%"

echo.
echo !YELLOW![INFO]!RESET! Deploying Linux %DISTRO% %LINUX_BUILD_TYPE%...
echo !YELLOW![INFO]!RESET! Source: %LINUX_BUILD_ARTIFACTS%
echo !YELLOW![INFO]!RESET! Target: %DEPLOY_TARGET%
echo.

call "%PM_HELPERS_SCRIPT%" :ensure_directory_exists "%LINUX_BUILD_ARTIFACTS%" "Linux build artifacts not found: %LINUX_BUILD_ARTIFACTS%"
if errorlevel 1 (
    echo         Run Build -^> Linux %DISTRO% %LINUX_BUILD_TYPE% first.
    exit /b 1
)

call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DEPLOY_TARGET%"
if errorlevel 1 exit /b 1

xcopy "%LINUX_BUILD_ARTIFACTS%\*" "%DEPLOY_TARGET%\" /E /I /Y >nul
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to copy Linux %DISTRO% artifacts.
    exit /b 1
)

echo !GREEN![OK]!RESET! Linux %DISTRO% %LINUX_BUILD_TYPE% deployment completed: %DEPLOY_TARGET%
dir /b "%DEPLOY_TARGET%"
exit /b 0

:done
exit /b 0
