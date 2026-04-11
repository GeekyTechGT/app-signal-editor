@echo off
REM ============================================================
REM  pm_deploy.bat - Deploy / Generate Installer
REM ============================================================

setlocal enabledelayedexpansion
set "DOCKER_BIN=C:\Program Files\Docker\Docker\resources\bin"
set "PATH=%DOCKER_BIN%;%PATH%"
set "BUILD_DIR=build"
set "DEPLOY_DIR=deploy"
set "PM_HELPERS_SCRIPT=scripts\pm_helpers.bat"

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

if /i "%~1"=="installer" goto installer
if /i "%~1"=="windows" (
    call :do_deploy_windows "%~2" "%~3"
    exit /b %errorlevel%
)
if /i "%~1"=="linux" (
    call :do_deploy_linux "%~2" "%~3"
    exit /b %errorlevel%
)
if /i "%~1"=="docker" (
    call :do_deploy_docker "%~2" "%~3"
    exit /b %errorlevel%
)

goto menu

:menu
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Deploy - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Windows Targets!RESET!
echo   !GREEN![1]!RESET! windows-gcc-debug
echo   !GREEN![2]!RESET! windows-gcc-release
echo   !GREEN![3]!RESET! windows-clang-debug
echo   !GREEN![4]!RESET! windows-clang-release
echo   !GREEN![5]!RESET! windows-MSVC-debug
echo   !GREEN![6]!RESET! windows-MSVC-release
echo.
echo   !BLUE!Linux Targets!RESET!
echo   !GREEN![7]!RESET! linux-gcc-debug
echo   !GREEN![8]!RESET! linux-gcc-release
echo   !GREEN![9]!RESET! linux-clang-debug
echo   !GREEN![10]!RESET! linux-clang-release
echo.
echo   !BLUE!Docker Targets!RESET!
echo   !GREEN![11]!RESET! docker-gcc-debug
echo   !GREEN![12]!RESET! docker-gcc-release
echo   !GREEN![13]!RESET! docker-clang-debug
echo   !GREEN![14]!RESET! docker-clang-release
echo   !GREEN![0]!RESET! Back
echo.
set /p "deploy_choice=  Select target [0-14]: "
if "%deploy_choice%"=="0" goto done
if "%deploy_choice%"=="1" call :do_deploy_windows gcc debug & goto finish
if "%deploy_choice%"=="2" call :do_deploy_windows gcc release & goto finish
if "%deploy_choice%"=="3" call :do_deploy_windows clang debug & goto finish
if "%deploy_choice%"=="4" call :do_deploy_windows clang release & goto finish
if "%deploy_choice%"=="5" call :do_deploy_windows MSVC debug & goto finish
if "%deploy_choice%"=="6" call :do_deploy_windows MSVC release & goto finish
if "%deploy_choice%"=="7" call :do_deploy_linux gcc debug & goto finish
if "%deploy_choice%"=="8" call :do_deploy_linux gcc release & goto finish
if "%deploy_choice%"=="9" call :do_deploy_linux clang debug & goto finish
if "%deploy_choice%"=="10" call :do_deploy_linux clang release & goto finish
if "%deploy_choice%"=="11" call :do_deploy_docker gcc debug & goto finish
if "%deploy_choice%"=="12" call :do_deploy_docker gcc release & goto finish
if "%deploy_choice%"=="13" call :do_deploy_docker clang debug & goto finish
if "%deploy_choice%"=="14" call :do_deploy_docker clang release & goto finish
goto done

:finish
pause
goto done

:installer
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Generate Installer - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo !YELLOW![INFO]!RESET! Installer generation placeholder.
echo         Integrate your packaging tooling here.
pause
goto done

:do_deploy_windows
set "COMPILER=%~1"
set "BUILD_TYPE=%~2"
set "DEPLOY_PRESET=windows-%COMPILER%-%BUILD_TYPE%"
set "BUILD_BIN_DIR=%BUILD_DIR%\%DEPLOY_PRESET%\bin"
set "DEPLOY_TARGET=%DEPLOY_DIR%\windows\%COMPILER%\%BUILD_TYPE%"
call "%PM_HELPERS_SCRIPT%" :ensure_directory_exists "%BUILD_BIN_DIR%" "Build directory not found: %BUILD_BIN_DIR%"
if errorlevel 1 exit /b 1
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DEPLOY_TARGET%"
if errorlevel 1 exit /b 1
copy "%BUILD_BIN_DIR%\*.exe" "%DEPLOY_TARGET%\" >nul 2>&1
copy "%BUILD_BIN_DIR%\*.dll" "%DEPLOY_TARGET%\" >nul 2>&1
call :locate_windeployqt WINDEPLOYQT_EXE
if not defined WINDEPLOYQT_EXE goto windeployqt_not_found
for %%E in ("%DEPLOY_TARGET%\*.exe") do (
    set "_EXE_NAME=%%~nxE"
    if /i not "!_EXE_NAME:~0,5!"=="test_" (
        "!WINDEPLOYQT_EXE!" --release --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime --verbose 1 "%%E"
    ) else (
        del "%%E" >nul 2>&1
    )
)
echo !GREEN![OK]!RESET! Windows deployment completed: %DEPLOY_TARGET%
exit /b 0

:do_deploy_linux
set "COMPILER=%~1"
set "BUILD_TYPE=%~2"
set "DEPLOY_PRESET=linux-%COMPILER%-%BUILD_TYPE%"
set "SOURCE_DIR=%BUILD_DIR%\%DEPLOY_PRESET%\bin"
set "LIB_DIR=%BUILD_DIR%\%DEPLOY_PRESET%\lib"
set "DEPLOY_TARGET=%DEPLOY_DIR%\linux\%COMPILER%\%BUILD_TYPE%"
call "%PM_HELPERS_SCRIPT%" :ensure_directory_exists "%SOURCE_DIR%" "Build directory not found: %SOURCE_DIR%"
if errorlevel 1 exit /b 1
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DEPLOY_TARGET%"
if errorlevel 1 exit /b 1
copy "%SOURCE_DIR%\*" "%DEPLOY_TARGET%\" >nul 2>&1
if exist "%LIB_DIR%" copy "%LIB_DIR%\*" "%DEPLOY_TARGET%\" >nul 2>&1
echo !GREEN![OK]!RESET! Linux deployment completed: %DEPLOY_TARGET%
exit /b 0

:do_deploy_docker
set "COMPILER=%~1"
set "BUILD_TYPE=%~2"
set "SOURCE_DIR=%BUILD_DIR%\docker-%COMPILER%-%BUILD_TYPE%\artifacts"
set "DEPLOY_TARGET=%DEPLOY_DIR%\docker\%COMPILER%\%BUILD_TYPE%"
call "%PM_HELPERS_SCRIPT%" :ensure_directory_exists "%SOURCE_DIR%" "Docker artifacts not found: %SOURCE_DIR%"
if errorlevel 1 exit /b 1
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DEPLOY_TARGET%"
if errorlevel 1 exit /b 1
copy "%SOURCE_DIR%\*" "%DEPLOY_TARGET%\" >nul 2>&1
echo !GREEN![OK]!RESET! Docker deployment completed: %DEPLOY_TARGET%
exit /b 0

:windeployqt_not_found
echo !RED![ERROR]!RESET! windeployqt6.exe not found.
exit /b 1

:locate_windeployqt
set "%~1="
for %%Q in (
    "C:\eng_apps\Qt\6.10.1\mingw_64\bin\windeployqt6.exe"
    "C:\eng_apps\Qt\6.10.1\msvc2022_64\bin\windeployqt6.exe"
) do (
    if exist %%~Q (
        set "%~1=%%~Q"
        goto :eof
    )
)
goto :eof

:done
exit /b 0
