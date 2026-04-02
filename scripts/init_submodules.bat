@echo off
setlocal enabledelayedexpansion

REM Initialize and update git submodules for MyProject
REM Configure submodules as: URL,PATH,BRANCH (branch optional)
REM Separate entries with ';' and avoid spaces.
REM Example:
REM set SUBMODULES=git@github.com:YourOrg/your-lib.git,external\your-lib,main

set SUBMODULES=

REM Navigate to project root
cd /d "%~dp0\.."

REM Ensure git is available
where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: git not found in PATH
    exit /b 1
)

echo =========================================
echo Submodule Initialization
echo =========================================

if "%SUBMODULES%"=="" (
    echo No submodules configured.
    echo Edit SUBMODULES variable in this script to add submodules.
    exit /b 0
)

for %%S in ("%SUBMODULES:;=" "%") do (
    set ITEM=%%~S
    call :handle_submodule "!ITEM!"
    if errorlevel 1 exit /b 1
)

echo Updating submodules...
git submodule update --init --recursive
if errorlevel 1 (
    echo ERROR: Submodule update failed
    exit /b 1
)

echo.
echo =========================================
echo Submodules are ready
echo =========================================
exit /b 0

:handle_submodule
set ITEM=%~1
set MODULE_URL=
set MODULE_PATH=
set MODULE_BRANCH=

for /f "tokens=1-3 delims=," %%a in ("%ITEM%") do (
    set MODULE_URL=%%a
    set MODULE_PATH=%%b
    set MODULE_BRANCH=%%c
)

if "%MODULE_URL%"=="" ( echo ERROR: Missing submodule URL & exit /b 1 )
if "%MODULE_PATH%"=="" ( echo ERROR: Missing submodule PATH & exit /b 1 )

for %%D in ("%MODULE_PATH%") do (
    if not exist "%%~dpD" mkdir "%%~dpD"
)

set MODULE_NAME=%MODULE_PATH:\=/%

call git config -f .gitmodules --get submodule.%MODULE_NAME%.path >nul 2>nul
if %errorlevel%==0 (
    echo Submodule already registered: %MODULE_PATH%
    if not "%MODULE_BRANCH%"=="" (
        git config -f .gitmodules submodule.%MODULE_NAME%.branch "%MODULE_BRANCH%"
        git submodule sync -- "%MODULE_PATH%"
    )
    goto after_add
)

echo Adding submodule: %MODULE_PATH%
if not "%MODULE_BRANCH%"=="" (
    git submodule add -b "%MODULE_BRANCH%" "%MODULE_URL%" "%MODULE_PATH%"
) else (
    git submodule add "%MODULE_URL%" "%MODULE_PATH%"
)
if errorlevel 1 ( echo ERROR: Failed to add submodule & exit /b 1 )

:after_add
git submodule sync -- "%MODULE_PATH%"
git submodule update --init --recursive --force -- "%MODULE_PATH%"
exit /b 0
