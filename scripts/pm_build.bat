@echo off
REM ============================================================
REM  pm_build.bat - Build Project
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_build.bat
REM ============================================================

setlocal enabledelayedexpansion

REM Docker path (set if Docker is not in PATH)
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
set "PM_HELPERS_SCRIPT=scripts\pm_helpers.bat"

:build_project
cls
set "PLATFORM="
set "LINUX_DISTRO="
set "COMPILER="
set "BUILD_TYPE="

echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Build Project - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Platform:!RESET!
echo   !GREEN![1]!RESET! Windows
echo   !GREEN![2]!RESET! Linux (Docker)
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "platform_choice=  Select platform [0-2]: "

if "%platform_choice%"=="0" goto done
if "%platform_choice%"=="1" goto build_windows
if "%platform_choice%"=="2" goto build_linux

echo !RED![ERROR]!RESET! Invalid platform choice.
timeout /t 2 >nul
goto build_project

:build_linux
echo.
echo   !BLUE!Select Linux Distribution:!RESET!
echo   !GREEN![1]!RESET! Ubuntu 22.04 (GLIBC 2.35, broader compatibility)
echo   !GREEN![2]!RESET! Alpine 3.19 (Static binaries, maximum portability)
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "distro_choice=  Select distribution [0-2]: "

if "%distro_choice%"=="0" goto done
if "%distro_choice%"=="1" set "LINUX_DISTRO=ubuntu"
if "%distro_choice%"=="2" set "LINUX_DISTRO=alpine"

if not defined LINUX_DISTRO (
    echo !RED![ERROR]!RESET! Invalid distribution choice.
    timeout /t 2 >nul
    goto build_project
)

echo.
echo   !BLUE!Select Build Type:!RESET!
echo   !GREEN![1]!RESET! Debug
echo   !GREEN![2]!RESET! Release
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "build_type_choice=  Select build type [0-2]: "

if "%build_type_choice%"=="0" goto done
if "%build_type_choice%"=="1" set "BUILD_TYPE=debug"
if "%build_type_choice%"=="2" set "BUILD_TYPE=release"

if not defined BUILD_TYPE (
    echo !RED![ERROR]!RESET! Invalid build type choice.
    timeout /t 2 >nul
    goto build_project
)

if "%LINUX_DISTRO%"=="alpine" (
    call :do_build_linux_alpine %BUILD_TYPE%
) else (
    call :do_build_linux %BUILD_TYPE%
)
pause
goto done

:build_windows
echo.
echo   !BLUE!Select Compiler:!RESET!
echo   !GREEN![1]!RESET! GCC (MinGW64)
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "compiler_choice=  Select compiler [0-1]: "

if "%compiler_choice%"=="0" goto done
if "%compiler_choice%"=="1" set "COMPILER=mingw64"

if not defined COMPILER (
    echo !RED![ERROR]!RESET! Invalid compiler choice.
    timeout /t 2 >nul
    goto build_project
)

echo.
echo   !BLUE!Select Build Type:!RESET!
echo   !GREEN![1]!RESET! Debug
echo   !GREEN![2]!RESET! Release
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "build_type_choice=  Select build type [0-2]: "

if "%build_type_choice%"=="0" goto done
if "%build_type_choice%"=="1" set "BUILD_TYPE=debug"
if "%build_type_choice%"=="2" set "BUILD_TYPE=release"

if not defined BUILD_TYPE (
    echo !RED![ERROR]!RESET! Invalid build type choice.
    timeout /t 2 >nul
    goto build_project
)

set "PRESET_NAME=windows-%COMPILER%-%BUILD_TYPE%"
echo.
echo !YELLOW![INFO]!RESET! Building with preset: %PRESET_NAME%
echo.

echo !YELLOW![INFO]!RESET! Configuring...
cmake --preset %PRESET_NAME%
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Configuration failed.
    pause
    goto done
)

echo.
echo !YELLOW![INFO]!RESET! Building...
cmake --build --preset %PRESET_NAME%
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Build failed.
    pause
    goto done
)

echo.
echo !GREEN![OK]!RESET! Build completed successfully!
echo.
pause
goto done

:do_build_linux
set "LINUX_BUILD_TYPE=%~1"
set "DOCKER_IMAGE_NAME=myproject-ubuntu"
set "LINUX_BUILD_DIR=%BUILD_DIR%\linux-ubuntu-%LINUX_BUILD_TYPE%"

echo.
echo !YELLOW![INFO]!RESET! Building Linux Ubuntu %LINUX_BUILD_TYPE% (CLI only) using Docker...
echo.

call "%PM_HELPERS_SCRIPT%" :ensure_docker_ready
if errorlevel 1 goto :eof

if not exist "%LINUX_BUILD_DIR%" mkdir "%LINUX_BUILD_DIR%"

if /i "%LINUX_BUILD_TYPE%"=="debug" (
    set "CMAKE_BUILD_TYPE=Debug"
) else (
    set "CMAKE_BUILD_TYPE=Release"
)

set "DOCKER_BUILD_LOG=%LINUX_BUILD_DIR%\docker-build-ubuntu-%LINUX_BUILD_TYPE%.log"
powershell -NoProfile -Command ^
    "docker build -f 'Dockerfile.ubuntu' --target builder --build-arg BUILD_TYPE=%CMAKE_BUILD_TYPE% --build-arg BUILD_TESTS=ON --build-arg BUILD_GUI=OFF --build-arg BUILD_SHARED=OFF -t %DOCKER_IMAGE_NAME%-%LINUX_BUILD_TYPE%:latest . 2>&1 | Tee-Object -FilePath '%DOCKER_BUILD_LOG%'; exit $LASTEXITCODE"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Docker build failed for Ubuntu %LINUX_BUILD_TYPE%.
    echo !YELLOW![INFO]!RESET! Log: %DOCKER_BUILD_LOG%
    goto :eof
)

set "LINUX_BUILD_ARTIFACTS=%LINUX_BUILD_DIR%\artifacts"
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%LINUX_BUILD_ARTIFACTS%"
if errorlevel 1 goto :eof

docker run --rm %DOCKER_IMAGE_NAME%-%LINUX_BUILD_TYPE%:latest sh -c "cd /app/build && tar -chf - bin lib" > "%LINUX_BUILD_ARTIFACTS%\build-output.tar"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to extract Ubuntu artifacts.
    goto :eof
)

pushd "%LINUX_BUILD_ARTIFACTS%"
tar -xf build-output.tar
del /f build-output.tar 2>nul
if exist "bin" ( move bin\* . >nul 2>&1 & rmdir bin 2>nul )
if exist "lib" ( move lib\* . >nul 2>&1 & rmdir lib 2>nul )
popd

echo !GREEN![OK]!RESET! Linux Ubuntu %LINUX_BUILD_TYPE% build completed.
echo !YELLOW![INFO]!RESET! Artifacts: %LINUX_BUILD_ARTIFACTS%
goto :eof

:do_build_linux_alpine
set "LINUX_BUILD_TYPE=%~1"
set "DOCKER_IMAGE_NAME=myproject-alpine"
set "LINUX_BUILD_DIR=%BUILD_DIR%\linux-alpine-%LINUX_BUILD_TYPE%"

echo.
echo !YELLOW![INFO]!RESET! Building Linux Alpine %LINUX_BUILD_TYPE% (Static, CLI only) using Docker...
echo.

call "%PM_HELPERS_SCRIPT%" :ensure_docker_ready
if errorlevel 1 goto :eof

if not exist "%LINUX_BUILD_DIR%" mkdir "%LINUX_BUILD_DIR%"

if /i "%LINUX_BUILD_TYPE%"=="debug" (
    set "CMAKE_BUILD_TYPE=Debug"
) else (
    set "CMAKE_BUILD_TYPE=Release"
)

set "DOCKER_BUILD_LOG=%LINUX_BUILD_DIR%\docker-build-alpine-%LINUX_BUILD_TYPE%.log"
powershell -NoProfile -Command ^
    "docker build -f 'Dockerfile.alpine' --target builder --build-arg BUILD_TYPE=%CMAKE_BUILD_TYPE% --build-arg BUILD_TESTS=ON --build-arg BUILD_GUI=OFF --build-arg BUILD_SHARED=OFF -t %DOCKER_IMAGE_NAME%-%LINUX_BUILD_TYPE%:latest . 2>&1 | Tee-Object -FilePath '%DOCKER_BUILD_LOG%'; exit $LASTEXITCODE"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Docker build failed for Alpine %LINUX_BUILD_TYPE%.
    goto :eof
)

set "LINUX_BUILD_ARTIFACTS=%LINUX_BUILD_DIR%\artifacts"
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%LINUX_BUILD_ARTIFACTS%"
if errorlevel 1 goto :eof

docker run --rm %DOCKER_IMAGE_NAME%-%LINUX_BUILD_TYPE%:latest sh -c "cd /app/build && tar -chf - bin" > "%LINUX_BUILD_ARTIFACTS%\build-output.tar"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to extract Alpine artifacts.
    goto :eof
)

pushd "%LINUX_BUILD_ARTIFACTS%"
tar -xf build-output.tar
del /f build-output.tar 2>nul
if exist "bin" ( move bin\* . >nul 2>&1 & rmdir bin 2>nul )
popd

echo !GREEN![OK]!RESET! Linux Alpine %LINUX_BUILD_TYPE% build completed.
echo !YELLOW![INFO]!RESET! Artifacts: %LINUX_BUILD_ARTIFACTS%
goto :eof

:done
exit /b 0
