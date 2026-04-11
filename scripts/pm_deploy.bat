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

REM Non-interactive shortcuts (handy for CI / scripting):
REM   pm_deploy.bat windows debug
REM   pm_deploy.bat windows release
REM   pm_deploy.bat linux ubuntu debug
REM   pm_deploy.bat linux alpine release
if /i "%~1"=="windows" (
    call :do_deploy_windows "%~2"
    exit /b %errorlevel%
)
if /i "%~1"=="linux" (
    call :do_deploy_linux "%~2" "%~3"
    exit /b %errorlevel%
)

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

REM ------------------------------------------------------------
REM  Run windeployqt on every executable in the deploy folder so
REM  the Qt runtime DLLs and the required plugin folders end up
REM  alongside the binary. This is the only place windeployqt is
REM  invoked - it is intentionally NOT a CMake POST_BUILD step.
REM ------------------------------------------------------------
call :locate_windeployqt WINDEPLOYQT_EXE
if not defined WINDEPLOYQT_EXE goto :windeployqt_not_found
echo !YELLOW![INFO]!RESET! Using windeployqt: !WINDEPLOYQT_EXE!

for %%P in ("!WINDEPLOYQT_EXE!") do set "WINDEPLOYQT_DIR=%%~dpP"
REM Strip trailing backslash from WINDEPLOYQT_DIR for path concatenation.
if "!WINDEPLOYQT_DIR:~-1!"=="\" set "WINDEPLOYQT_DIR=!WINDEPLOYQT_DIR:~0,-1!"

REM windeployqt picks debug- or release-variant plugins based on the
REM flag we pass. Some Qt installs ship only release plugins (no
REM qwindowsd.dll), in which case "--debug" aborts with "Unable to find
REM the platform plugin." Force "--release" whenever the debug platform
REM plugin is missing, regardless of which build the .exe came from.
set "DEPLOY_FLAG=--release"
if /i "%WIN_BUILD_TYPE%"=="debug" (
    if exist "!WINDEPLOYQT_DIR!\..\plugins\platforms\qwindowsd.dll" (
        set "DEPLOY_FLAG=--debug"
    )
)
echo !YELLOW![INFO]!RESET! windeployqt variant: !DEPLOY_FLAG!

REM Prepend Qt bin to PATH so windeployqt can locate Qt6Core.dll and the
REM qwindows platform plugin it needs while introspecting the target.
set "PATH=!WINDEPLOYQT_DIR!;%PATH%"

REM Skip test executables (they are not deployable artifacts and contain
REM no Qt dependencies, so windeployqt would just bail on them).
for %%E in ("%DEPLOY_TARGET%\*.exe") do (
    set "_EXE_NAME=%%~nxE"
    if /i "!_EXE_NAME:~0,5!"=="test_" (
        echo !YELLOW![INFO]!RESET! Removing non-deployable test binary: !_EXE_NAME!
        del "%%E" >nul 2>&1
    ) else (
        echo !YELLOW![INFO]!RESET! Deploying Qt runtime for !_EXE_NAME!...
        "!WINDEPLOYQT_EXE!" !DEPLOY_FLAG! --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime --verbose 1 "%%E"
        if errorlevel 1 (
            echo !YELLOW![WARN]!RESET! windeployqt did not deploy !_EXE_NAME! ^(probably not a Qt executable^)
        )
    )
)

echo !GREEN![OK]!RESET! Windows %WIN_BUILD_TYPE% deployment completed: %DEPLOY_TARGET%
dir /b "%DEPLOY_TARGET%"
exit /b 0

:windeployqt_not_found
echo !RED![ERROR]!RESET! windeployqt6.exe not found.
echo         Set the QT_DIR environment variable to your Qt install
echo         e.g. C:\eng_apps\Qt\6.10.1\mingw_64
echo         or add windeployqt6 to PATH and retry.
exit /b 1

REM ============================================================
REM  Locate windeployqt6.exe
REM  Search order: PATH, %QT_DIR%\bin, common C:\eng_apps\Qt installs.
REM  Parameters: %1=output variable name
REM ============================================================
:locate_windeployqt
set "_LWD_OUT=%~1"
if "%_LWD_OUT%"=="" exit /b 1
set "%_LWD_OUT%="

where windeployqt6 >nul 2>&1
if not errorlevel 1 (
    for /f "delims=" %%P in ('where windeployqt6') do (
        set "%_LWD_OUT%=%%P"
        goto :locate_windeployqt_done
    )
)

if defined QT_DIR (
    if exist "%QT_DIR%\bin\windeployqt6.exe" (
        set "%_LWD_OUT%=%QT_DIR%\bin\windeployqt6.exe"
        goto :locate_windeployqt_done
    )
)

for /d %%V in ("C:\eng_apps\Qt\*") do (
    if exist "%%V\mingw_64\bin\windeployqt6.exe" (
        set "%_LWD_OUT%=%%V\mingw_64\bin\windeployqt6.exe"
    )
)

:locate_windeployqt_done
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
