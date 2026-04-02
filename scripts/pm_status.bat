@echo off
REM ============================================================
REM  pm_status.bat - Git Hooks Status
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_status.bat
REM ============================================================

setlocal enabledelayedexpansion

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

cls
echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Git Hooks Status - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.

git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Not a git repository.
    echo.
    pause
    exit /b 1
)

for /f "tokens=*" %%a in ('git config --get core.hooksPath 2^>nul') do set "HOOKS_PATH=%%a"

if "!HOOKS_PATH!"=="" (
    echo   Hooks Status:    !RED!DISABLED!RESET! (using default .git/hooks)
    echo   Hooks Path:      !YELLOW!(not set)!RESET!
) else (
    if "!HOOKS_PATH!"==".githooks/hooks" (
        echo   Hooks Status:    !GREEN!ENABLED!RESET!
        echo   Hooks Path:      !HOOKS_PATH!
    ) else (
        echo   Hooks Status:    !YELLOW!CUSTOM!RESET!
        echo   Hooks Path:      !HOOKS_PATH!
    )
)

echo.
echo   !BLUE!Available Hooks:!RESET!
if exist ".githooks\hooks\pre-commit"         (echo     !GREEN![+]!RESET! pre-commit) else (echo     !RED![-]!RESET! pre-commit (not found))
if exist ".githooks\hooks\commit-msg"         (echo     !GREEN![+]!RESET! commit-msg) else (echo     !RED![-]!RESET! commit-msg (not found))
if exist ".githooks\hooks\prepare-commit-msg" (echo     !GREEN![+]!RESET! prepare-commit-msg) else (echo     !RED![-]!RESET! prepare-commit-msg (not found))
if exist ".githooks\hooks\pre-push"           (echo     !GREEN![+]!RESET! pre-push) else (echo     !RED![-]!RESET! pre-push (not found))

echo.
for /f "tokens=*" %%a in ('git config --get commit.template 2^>nul') do set "COMMIT_TEMPLATE=%%a"
if "!COMMIT_TEMPLATE!"=="" (
    echo   Commit Template: !YELLOW!(not set)!RESET!
) else (
    echo   Commit Template: !COMMIT_TEMPLATE!
)

echo.
echo !CYAN!====================================================!RESET!
echo.
pause
exit /b 0
