@echo off
REM ============================================================
REM  customize_project.bat - Project customization wrapper
REM ============================================================
REM  Reads project.metadata.json and manages apply/rollback for
REM  the placeholder replacement workflow.
REM ============================================================

setlocal EnableDelayedExpansion

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

set "PROJECT_ROOT=%~dp0.."
set "PM_HELPERS_SCRIPT=%PROJECT_ROOT%\scripts\pm_helpers.bat"
set "CUSTOMIZER=%PROJECT_ROOT%\scripts\customize_project.py"
set "METADATA_FILE=%PROJECT_ROOT%\project.metadata.json"
set "VENV_PYTHON=%PROJECT_ROOT%\pyvenv\Scripts\python.exe"
set "BACKUP_ROOT=%PROJECT_ROOT%\.scaffold-backups"
set "LATEST_MANIFEST=%BACKUP_ROOT%\latest_manifest.json"
set "REPORT_FILE=%BACKUP_ROOT%\last_customize_report.json"

pushd "%PROJECT_ROOT%" >nul 2>&1 || (
    echo !RED![ERROR]!RESET! Unable to resolve project root.
    exit /b 1
)

cls
echo.
echo !CYAN!======================================================!RESET!
echo !CYAN!           Signal Editor - Customize Project!RESET!
echo !CYAN!======================================================!RESET!
echo.

if not exist "%CUSTOMIZER%" (
    echo !RED![ERROR]!RESET! Missing script: %CUSTOMIZER%
    popd >nul 2>&1
    exit /b 1
)

if exist "%VENV_PYTHON%" (
    set "PYTHON_CMD=%VENV_PYTHON%"
) else (
    if not exist "%PM_HELPERS_SCRIPT%" (
        echo !RED![ERROR]!RESET! Missing helper module: %PM_HELPERS_SCRIPT%
        popd >nul 2>&1
        exit /b 1
    )
    call "%PM_HELPERS_SCRIPT%" :resolve_python_cmd PYTHON_CMD
    if errorlevel 1 (
        echo !RED![ERROR]!RESET! Python interpreter not found. Run Initialize Project first or install Python.
        popd >nul 2>&1
        exit /b 1
    )
)

echo   !GREEN![1]!RESET! Preview and Apply Customization
echo   !GREEN![2]!RESET! Roll Back Last Customization
echo   !GREEN![0]!RESET! Cancel
echo.
set /p "action=  Select an option [0-2]: "
if "%action%"=="1" goto apply_customization
if "%action%"=="2" goto rollback_customization
if "%action%"=="0" goto cancelled

echo.
echo !RED![ERROR]!RESET! Invalid option.
popd >nul 2>&1
exit /b 1

:apply_customization
if not exist "%METADATA_FILE%" (
    echo !RED![ERROR]!RESET! Missing metadata file: %METADATA_FILE%
    popd >nul 2>&1
    exit /b 1
)

echo.
echo !BLUE![STEP 1/2]!RESET! Validating metadata and showing planned changes...
call "%PYTHON_CMD%" "%CUSTOMIZER%" --metadata "%METADATA_FILE%" --dry-run --report-json "%REPORT_FILE%"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Dry-run validation failed.
    popd >nul 2>&1
    exit /b 1
)

echo [INFO] JSON report: %REPORT_FILE%
echo.
set /p "confirm=Apply project customization now? [y/N]: "
if /I not "%confirm%"=="Y" goto cancelled

echo.
echo !BLUE![STEP 2/2]!RESET! Applying project customization...
call "%PYTHON_CMD%" "%CUSTOMIZER%" --metadata "%METADATA_FILE%" --apply --report-json "%REPORT_FILE%"
set "CUSTOMIZE_RESULT=%ERRORLEVEL%"
if not "%CUSTOMIZE_RESULT%"=="0" (
    echo !RED![ERROR]!RESET! Project customization failed.
    echo [INFO] Check report: %REPORT_FILE%
    if exist "%LATEST_MANIFEST%" echo [INFO] Latest rollback manifest: %LATEST_MANIFEST%
    popd >nul 2>&1
    exit /b %CUSTOMIZE_RESULT%
)

echo.
echo !GREEN![DONE]!RESET! Project customization completed.
echo [INFO] Report: %REPORT_FILE%
if exist "%LATEST_MANIFEST%" echo [INFO] Rollback manifest: %LATEST_MANIFEST%
popd >nul 2>&1
exit /b 0

:rollback_customization
if not exist "%LATEST_MANIFEST%" (
    echo !RED![ERROR]!RESET! No rollback manifest found at: %LATEST_MANIFEST%
    popd >nul 2>&1
    exit /b 1
)

echo.
echo !YELLOW![WARN]!RESET! This will restore the last backed up project customization state.
set /p "confirm=Proceed with rollback? [y/N]: "
if /I not "%confirm%"=="Y" goto cancelled

echo.
call "%PYTHON_CMD%" "%CUSTOMIZER%" --rollback "%LATEST_MANIFEST%" --report-json "%REPORT_FILE%"
set "ROLLBACK_RESULT=%ERRORLEVEL%"
if not "%ROLLBACK_RESULT%"=="0" (
    echo !RED![ERROR]!RESET! Rollback failed.
    echo [INFO] Check report: %REPORT_FILE%
    popd >nul 2>&1
    exit /b %ROLLBACK_RESULT%
)

echo.
echo !GREEN![DONE]!RESET! Rollback completed.
echo [INFO] Report: %REPORT_FILE%
popd >nul 2>&1
exit /b 0

:cancelled
echo.
echo !YELLOW![CANCELLED]!RESET! No changes applied.
popd >nul 2>&1
exit /b 0
