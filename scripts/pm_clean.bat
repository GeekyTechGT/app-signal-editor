@echo off
REM ============================================================
REM  pm_clean.bat - Clean Build Directories
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_clean.bat [clean_build|clean_all]
REM ============================================================

setlocal enabledelayedexpansion

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

set "BUILD_DIR=build"

if /i "%~1"=="clean_all" goto clean_all
goto clean_build

:clean_build
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Clean Build - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Configuration to Clean:!RESET!
echo   !GREEN![1]!RESET! GCC Debug
echo   !GREEN![2]!RESET! GCC Release
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "clean_choice=  Select configuration [0-2]: "

if "%clean_choice%"=="0" goto done
if "%clean_choice%"=="1" set "CLEAN_DIR=windows-mingw64-debug"
if "%clean_choice%"=="2" set "CLEAN_DIR=windows-mingw64-release"

if not defined CLEAN_DIR (
    echo !RED![ERROR]!RESET! Invalid choice.
    timeout /t 2 >nul
    goto clean_build
)

echo.
echo !YELLOW![INFO]!RESET! Cleaning %CLEAN_DIR%...

if exist "%BUILD_DIR%\%CLEAN_DIR%" (
    rmdir /s /q "%BUILD_DIR%\%CLEAN_DIR%"
    echo !GREEN![OK]!RESET! Cleaned %BUILD_DIR%\%CLEAN_DIR%
) else (
    echo !YELLOW![INFO]!RESET! Directory does not exist: %BUILD_DIR%\%CLEAN_DIR%
)

echo.
pause
goto done

:clean_all
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Clean All Builds - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo !YELLOW![WARN]!RESET! This will delete ALL build directories!
echo.
set /p "confirm=  Are you sure? (y/N): "

if /i not "%confirm%"=="y" (
    echo !YELLOW![INFO]!RESET! Operation cancelled.
    pause
    goto done
)

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo !GREEN![OK]!RESET! Cleaned %BUILD_DIR%
) else (
    echo !YELLOW![INFO]!RESET! Build directory does not exist.
)

echo.
echo !GREEN![OK]!RESET! All builds cleaned!
echo.
pause
goto done

:done
exit /b 0
