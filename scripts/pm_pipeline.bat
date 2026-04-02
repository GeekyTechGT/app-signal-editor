@echo off
REM ============================================================
REM  pm_pipeline.bat - Run Test Pipeline
REM ============================================================
REM  Sub-script for project_manager.bat
REM  Usage: call scripts\pm_pipeline.bat
REM ============================================================

setlocal enabledelayedexpansion

for /f %%A in ('"prompt $E & for %%B in (1) do rem"') do set "ESC=%%A"
set "GREEN=!ESC![32m"
set "YELLOW=!ESC![33m"
set "RED=!ESC![31m"
set "CYAN=!ESC![36m"
set "BLUE=!ESC![34m"
set "RESET=!ESC![0m"

set "PM_HELPERS_SCRIPT=scripts\pm_helpers.bat"
set "TEST_PIPELINES_DIR=%~dp0..\tests\05.pipeline_test"

:pipeline_menu
cls
set "PIPELINE_COUNT="
call :discover_test_pipelines

echo.
echo !CYAN!====================================================!RESET!
echo !CYAN!  Run Test Pipeline - MyProject!RESET!
echo !CYAN!====================================================!RESET!
echo.

if not defined PIPELINE_COUNT (
    echo !YELLOW![INFO]!RESET! No pipeline launchers found under %TEST_PIPELINES_DIR%.
    echo         Expected files: run_pipeline.py, run_pipeline.bat, run_pipeline.cmd, or run_pipeline.ps1
    echo         inside a pipeline folder.
    echo.
    pause
    goto done
)

echo   !BLUE!Available Pipelines:!RESET!
for /L %%I in (1,1,!PIPELINE_COUNT!) do (
    call echo   !GREEN![%%I]!RESET! %%PIPELINE_NAME_%%I%% ^(%%PIPELINE_LAUNCHER_%%I%%^)
)
echo   !GREEN![0]!RESET! Back to main menu
echo.
set /p "pipeline_choice=  Select pipeline [0-!PIPELINE_COUNT!]: "

if "%pipeline_choice%"=="0" goto done
if not defined PIPELINE_FILE_%pipeline_choice% (
    echo !RED![ERROR]!RESET! Invalid choice.
    timeout /t 2 >nul
    goto pipeline_menu
)

call set "SELECTED_PIPELINE=%%PIPELINE_FILE_%pipeline_choice%%%"
call set "SELECTED_PIPELINE_NAME=%%PIPELINE_NAME_%pipeline_choice%%%"
call :run_test_pipeline "!SELECTED_PIPELINE!"
echo.
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Pipeline execution failed.
) else (
    echo !GREEN![OK]!RESET! Pipeline execution completed.
)
echo.
pause
goto pipeline_menu

:discover_test_pipelines
set "PIPELINE_COUNT="
if not exist "%TEST_PIPELINES_DIR%" goto :eof

for %%E in (py bat cmd ps1) do (
    for /r "%TEST_PIPELINES_DIR%" %%F in (run_pipeline.%%E) do (
        if exist "%%~fF" (
            set /a PIPELINE_COUNT+=1
            for %%D in ("%%~dpF.") do (
                call set "PIPELINE_FILE_%%PIPELINE_COUNT%%=%%~fF"
                call set "PIPELINE_NAME_%%PIPELINE_COUNT%%=%%~nxD"
                call set "PIPELINE_LAUNCHER_%%PIPELINE_COUNT%%=%%~nxF"
            )
        )
    )
)
goto :eof

:run_test_pipeline
set "PIPELINE_PATH=%~1"
set "PIPELINE_EXT=%~x1"

if "%PIPELINE_PATH%"=="" ( echo !RED![ERROR]!RESET! Missing pipeline path. & exit /b 1 )
if not exist "%PIPELINE_PATH%" ( echo !RED![ERROR]!RESET! Pipeline not found: %PIPELINE_PATH% & exit /b 1 )

echo.
echo !YELLOW![INFO]!RESET! Running pipeline: "%PIPELINE_PATH%"

if /I "%PIPELINE_EXT%"==".py" (
    set "PYTHON_CMD="
    call "%PM_HELPERS_SCRIPT%" :resolve_python_cmd PYTHON_CMD
    if errorlevel 1 ( echo !RED![ERROR]!RESET! Python not found in PATH. & exit /b 1 )
    !PYTHON_CMD! -u "%PIPELINE_PATH%"
    exit /b !errorlevel!
)
if /I "%PIPELINE_EXT%"==".bat" ( call "%PIPELINE_PATH%" & exit /b !errorlevel! )
if /I "%PIPELINE_EXT%"==".cmd" ( call "%PIPELINE_PATH%" & exit /b !errorlevel! )
if /I "%PIPELINE_EXT%"==".ps1" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%PIPELINE_PATH%"
    exit /b !errorlevel!
)

echo !RED![ERROR]!RESET! Unsupported pipeline launcher type: %PIPELINE_EXT%
exit /b 1

:done
exit /b 0
