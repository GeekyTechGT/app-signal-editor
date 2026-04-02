@echo off
REM ============================================================================
REM  pm_helpers.bat
REM ============================================================================
REM  Reusable helper routines for project_manager.bat.
REM
REM  Usage pattern from another batch file:
REM    call "scripts\pm_helpers.bat" :label arg1 arg2 ...
REM
REM  Implemented helpers:
REM    :resolve_python_cmd <out_var_name>
REM    :resolve_customer_id_from_profile <profile_json> <out_var_name>
REM    :ensure_docker_ready
REM    :clean_and_create_dir <dir_path>
REM    :ensure_directory_exists <dir_path> <error_message>
REM ============================================================================

if "%~1"=="" (
    echo [ERROR] Missing helper label.
    exit /b 1
)
goto %~1

:resolve_python_cmd
REM Resolve a Python launcher available in PATH and write it to the requested variable.
set "_PMH_OUT_VAR=%~2"
if "%_PMH_OUT_VAR%"=="" exit /b 1

set "_PMH_PYTHON_CMD="
where python >nul 2>&1
if not errorlevel 1 (
    set "_PMH_PYTHON_CMD=python"
)

if not defined _PMH_PYTHON_CMD (
    where py >nul 2>&1
    if not errorlevel 1 (
        set "_PMH_PYTHON_CMD=py -3"
    )
)

if not defined _PMH_PYTHON_CMD exit /b 1
set "%_PMH_OUT_VAR%=%_PMH_PYTHON_CMD%"
exit /b 0

:resolve_customer_id_from_profile
REM Read customer_id from JSON profile using PowerShell ConvertFrom-Json.
REM Falls back to profile file name if customer_id is missing/empty.
set "_PMH_PROFILE_PATH=%~2"
set "_PMH_OUT_VAR=%~3"
if "%_PMH_PROFILE_PATH%"=="" exit /b 1
if "%_PMH_OUT_VAR%"=="" exit /b 1
if not exist "%_PMH_PROFILE_PATH%" exit /b 1

set "_PMH_CUSTOMER_ID="
for /f "usebackq delims=" %%C in (`powershell -NoProfile -Command "$p = Get-Content -Raw '%_PMH_PROFILE_PATH%' ^| ConvertFrom-Json; $p.customer_id"`) do set "_PMH_CUSTOMER_ID=%%C"

if not defined _PMH_CUSTOMER_ID (
    set "_PMH_CUSTOMER_ID=%~n2"
)

if not defined _PMH_CUSTOMER_ID exit /b 1
set "%_PMH_OUT_VAR%=%_PMH_CUSTOMER_ID%"
exit /b 0

:ensure_docker_ready
REM Validate both Docker CLI availability and daemon state.
docker --version >nul 2>&1
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Docker is not installed or not in PATH.
    exit /b 1
)
docker info >nul 2>&1
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Docker daemon is not running.
    exit /b 1
)
exit /b 0

:clean_and_create_dir
REM Recreate a directory from scratch.
set "_PMH_TARGET_DIR=%~2"
if "%_PMH_TARGET_DIR%"=="" exit /b 1

if exist "%_PMH_TARGET_DIR%" (
    rmdir /s /q "%_PMH_TARGET_DIR%" >nul 2>&1
)
mkdir "%_PMH_TARGET_DIR%" >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0

:ensure_directory_exists
REM Generic guard to validate required folder existence.
set "_PMH_REQUIRED_DIR=%~2"
set "_PMH_ERROR_TEXT=%~3"
if "%_PMH_REQUIRED_DIR%"=="" exit /b 1
if exist "%_PMH_REQUIRED_DIR%" exit /b 0

if not "%_PMH_ERROR_TEXT%"=="" (
    echo !RED![ERROR]!RESET! %_PMH_ERROR_TEXT%
) else (
    echo !RED![ERROR]!RESET! Required directory not found: %_PMH_REQUIRED_DIR%
)
exit /b 1
