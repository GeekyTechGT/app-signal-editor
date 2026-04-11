@echo off
REM ============================================================
REM  pm_clean.bat - Clean Build Directories
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
goto clean_one

:clean_one
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Clean Build - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo   !BLUE!Select Configuration to Clean:!RESET!
echo   !GREEN![1]!RESET! windows-gcc-debug
echo   !GREEN![2]!RESET! windows-gcc-release
echo   !GREEN![3]!RESET! windows-clang-debug
echo   !GREEN![4]!RESET! windows-clang-release
echo   !GREEN![5]!RESET! windows-MSVC-debug
echo   !GREEN![6]!RESET! windows-MSVC-release
echo   !GREEN![7]!RESET! linux-gcc-debug
echo   !GREEN![8]!RESET! linux-gcc-release
echo   !GREEN![9]!RESET! linux-clang-debug
echo   !GREEN![10]!RESET! linux-clang-release
echo   !GREEN![11]!RESET! docker-gcc-debug
echo   !GREEN![12]!RESET! docker-gcc-release
echo   !GREEN![13]!RESET! docker-clang-debug
echo   !GREEN![14]!RESET! docker-clang-release
echo   !GREEN![0]!RESET! Back
echo.
set /p "clean_choice=  Select configuration [0-14]: "
if "%clean_choice%"=="0" goto done
if "%clean_choice%"=="1" set "CLEAN_DIR=windows-gcc-debug"
if "%clean_choice%"=="2" set "CLEAN_DIR=windows-gcc-release"
if "%clean_choice%"=="3" set "CLEAN_DIR=windows-clang-debug"
if "%clean_choice%"=="4" set "CLEAN_DIR=windows-clang-release"
if "%clean_choice%"=="5" set "CLEAN_DIR=windows-MSVC-debug"
if "%clean_choice%"=="6" set "CLEAN_DIR=windows-MSVC-release"
if "%clean_choice%"=="7" set "CLEAN_DIR=linux-gcc-debug"
if "%clean_choice%"=="8" set "CLEAN_DIR=linux-gcc-release"
if "%clean_choice%"=="9" set "CLEAN_DIR=linux-clang-debug"
if "%clean_choice%"=="10" set "CLEAN_DIR=linux-clang-release"
if "%clean_choice%"=="11" set "CLEAN_DIR=docker-gcc-debug"
if "%clean_choice%"=="12" set "CLEAN_DIR=docker-gcc-release"
if "%clean_choice%"=="13" set "CLEAN_DIR=docker-clang-debug"
if "%clean_choice%"=="14" set "CLEAN_DIR=docker-clang-release"
if not defined CLEAN_DIR goto done
if exist "%BUILD_DIR%\%CLEAN_DIR%" (
    rmdir /s /q "%BUILD_DIR%\%CLEAN_DIR%"
    echo !GREEN![OK]!RESET! Cleaned %BUILD_DIR%\%CLEAN_DIR%
) else (
    echo !YELLOW![INFO]!RESET! Directory does not exist: %BUILD_DIR%\%CLEAN_DIR%
)
pause
goto done

:clean_all
cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Clean All Builds - Signal Editor!RESET!
echo !CYAN!====================================================!RESET!
echo.
echo !YELLOW![WARN]!RESET! This will delete ALL build directories.
set /p "confirm=  Are you sure? (y/N): "
if /i not "%confirm%"=="y" goto done
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo !GREEN![OK]!RESET! Cleaned %BUILD_DIR%
) else (
    echo !YELLOW![INFO]!RESET! Build directory does not exist.
)
pause
goto done

:done
exit /b 0
