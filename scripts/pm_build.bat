@echo off
REM ============================================================
REM  pm_build.bat - Build Signal Editor
REM ============================================================

setlocal enabledelayedexpansion
set "DOCKER_BIN=C:\Program Files\Docker\Docker\resources\bin"
set "PATH=%DOCKER_BIN%;%PATH%"
set "PM_HELPERS_SCRIPT=scripts\pm_helpers.bat"
set "BUILD_DIR=build"

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

:main
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Build Project - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Target Family:!RESET!
echo   !GREEN![1]!RESET! Windows local presets
echo   !GREEN![2]!RESET! Linux host presets ^(command helper^)
echo   !GREEN![3]!RESET! Docker Linux presets
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "platform_choice=  Select target family [0-3]: "
if "%platform_choice%"=="0" goto done
if "%platform_choice%"=="1" goto windows_menu
if "%platform_choice%"=="2" goto linux_menu
if "%platform_choice%"=="3" goto docker_menu
echo !RED![ERROR]!RESET! Invalid choice.
timeout /t 2 >nul
goto main

:windows_menu
echo.
echo   !BLUE!Select Windows Compiler:!RESET!
echo   !GREEN![1]!RESET! GCC
echo   !GREEN![2]!RESET! Clang
echo   !GREEN![3]!RESET! MSVC
echo   !GREEN![0]!RESET! Back
echo.
set /p "compiler_choice=  Select compiler [0-3]: "
if "%compiler_choice%"=="0" goto done
if "%compiler_choice%"=="1" set "COMPILER=gcc"
if "%compiler_choice%"=="2" set "COMPILER=clang"
if "%compiler_choice%"=="3" set "COMPILER=MSVC"
if not defined COMPILER (
    echo !RED![ERROR]!RESET! Invalid compiler choice.
    timeout /t 2 >nul
    goto main
)
call :ask_build_type
if errorlevel 1 goto done
set "PRESET_NAME=windows-%COMPILER%-%BUILD_TYPE%"
call :configure_and_build "%PRESET_NAME%"
pause
goto done

:linux_menu
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
if not defined LINUX_COMPILER (
    echo !RED![ERROR]!RESET! Invalid compiler choice.
    timeout /t 2 >nul
    goto main
)
call :ask_build_type
if errorlevel 1 goto done
set "PRESET_NAME=linux-%LINUX_COMPILER%-%BUILD_TYPE%"
call :print_linux_commands "%PRESET_NAME%"
pause
goto done

:docker_menu
echo.
echo   !BLUE!Select Docker Compiler:!RESET!
echo   !GREEN![1]!RESET! GCC
echo   !GREEN![2]!RESET! Clang
echo   !GREEN![0]!RESET! Back
echo.
set /p "docker_choice=  Select compiler [0-2]: "
if "%docker_choice%"=="0" goto done
if "%docker_choice%"=="1" set "DOCKER_COMPILER=gcc"
if "%docker_choice%"=="2" set "DOCKER_COMPILER=clang"
if not defined DOCKER_COMPILER (
    echo !RED![ERROR]!RESET! Invalid compiler choice.
    timeout /t 2 >nul
    goto main
)
call :ask_build_type
if errorlevel 1 goto done
call :do_build_docker "%DOCKER_COMPILER%" "%BUILD_TYPE%"
pause
goto done

:ask_build_type
set "BUILD_TYPE="
echo.
echo   !BLUE!Select Build Type:!RESET!
echo   !GREEN![1]!RESET! Debug
echo   !GREEN![2]!RESET! Release
echo   !GREEN![0]!RESET! Back
echo.
set /p "build_type_choice=  Select build type [0-2]: "
if "%build_type_choice%"=="0" exit /b 1
if "%build_type_choice%"=="1" set "BUILD_TYPE=debug"
if "%build_type_choice%"=="2" set "BUILD_TYPE=release"
if not defined BUILD_TYPE exit /b 1
exit /b 0

:configure_and_build
set "PRESET_NAME=%~1"
echo.
echo !YELLOW![INFO]!RESET! Configuring preset: %PRESET_NAME%
cmake --preset %PRESET_NAME%
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Configuration failed.
    exit /b 1
)
echo.
echo !YELLOW![INFO]!RESET! Building preset: %PRESET_NAME%
cmake --build --preset %PRESET_NAME%
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Build failed.
    exit /b 1
)
echo.
echo !GREEN![OK]!RESET! Build completed successfully.
exit /b 0

:print_linux_commands
set "PRESET_NAME=%~1"
echo.
echo !YELLOW![INFO]!RESET! Linux host presets must be executed from a Linux shell.
echo !YELLOW![INFO]!RESET! Run the following commands on Linux or WSL:
echo.
echo     cmake --preset %PRESET_NAME%
echo     cmake --build --preset %PRESET_NAME%
echo.
exit /b 0

:do_build_docker
set "DOCKER_COMPILER=%~1"
set "DOCKER_BUILD_TYPE=%~2"
set "PRESET_NAME=docker-%DOCKER_COMPILER%-%DOCKER_BUILD_TYPE%"
set "DOCKER_IMAGE_NAME=signal-editor-%DOCKER_COMPILER%"
set "DOCKER_BUILD_DIR=%BUILD_DIR%\%PRESET_NAME%"
set "DOCKER_BUILD_LOG=%DOCKER_BUILD_DIR%\docker-build.log"
set "DOCKER_ARTIFACTS_DIR=%DOCKER_BUILD_DIR%\artifacts"

call "%PM_HELPERS_SCRIPT%" :ensure_docker_ready
if errorlevel 1 exit /b 1
if not exist "%DOCKER_BUILD_DIR%" mkdir "%DOCKER_BUILD_DIR%"
call "%PM_HELPERS_SCRIPT%" :clean_and_create_dir "%DOCKER_ARTIFACTS_DIR%"
if errorlevel 1 exit /b 1

if /i "%DOCKER_BUILD_TYPE%"=="debug" (
    set "CMAKE_BUILD_TYPE=Debug"
) else (
    set "CMAKE_BUILD_TYPE=Release"
)

echo.
echo !YELLOW![INFO]!RESET! Building Docker preset: %PRESET_NAME%
echo !YELLOW![INFO]!RESET! Compiler: %DOCKER_COMPILER%
echo !YELLOW![INFO]!RESET! Build type: %DOCKER_BUILD_TYPE%

powershell -NoProfile -Command ^
    "docker build -f 'Dockerfile.ubuntu' --target builder --build-arg COMPILER=%DOCKER_COMPILER% --build-arg BUILD_TYPE=%CMAKE_BUILD_TYPE% --build-arg BUILD_TESTS=ON --build-arg BUILD_GUI=OFF --build-arg BUILD_SHARED=OFF -t %DOCKER_IMAGE_NAME%-%DOCKER_BUILD_TYPE%:latest . 2>&1 | Tee-Object -FilePath '%DOCKER_BUILD_LOG%'; exit $LASTEXITCODE"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Docker build failed.
    echo !YELLOW![INFO]!RESET! Log: %DOCKER_BUILD_LOG%
    exit /b 1
)

docker run --rm %DOCKER_IMAGE_NAME%-%DOCKER_BUILD_TYPE%:latest sh -c "cd /app/build && tar -chf - bin lib" > "%DOCKER_ARTIFACTS_DIR%\build-output.tar"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to extract Docker artifacts.
    exit /b 1
)

pushd "%DOCKER_ARTIFACTS_DIR%"
tar -xf build-output.tar
del /f build-output.tar 2>nul
if exist "bin" ( move bin\* . >nul 2>&1 & rmdir bin 2>nul )
if exist "lib" ( move lib\* . >nul 2>&1 & rmdir lib 2>nul )
popd

echo !GREEN![OK]!RESET! Docker build completed.
echo !YELLOW![INFO]!RESET! Artifacts: %DOCKER_ARTIFACTS_DIR%
exit /b 0

:done
exit /b 0
