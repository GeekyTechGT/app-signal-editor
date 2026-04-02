@echo off
REM ============================================================
REM  init_project.bat - Project bootstrap
REM ============================================================
REM  Verifies required tools from CMakePresets.json, creates the
REM  Python virtual environment, enables git hooks and prints a
REM  compact overview of the default project layout.
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
pushd "%PROJECT_ROOT%" >nul 2>&1 || (
    echo !RED![ERROR]!RESET! Unable to resolve project root.
    exit /b 1
)

set "PRESETS_FILE=%PROJECT_ROOT%\CMakePresets.json"
set "PM_HELPERS_SCRIPT=%PROJECT_ROOT%\scripts\pm_helpers.bat"
set "REQUIREMENTS_FILE=%PROJECT_ROOT%\requirements\requirements.txt"
set "VENV_DIR=%PROJECT_ROOT%\pyvenv"
set "VENV_PYTHON=%VENV_DIR%\Scripts\python.exe"
set "VENV_SITE_PACKAGES=%VENV_DIR%\Lib\site-packages"
set "HOOKS_DIR=%PROJECT_ROOT%\.githooks\hooks"
set "HOOKS_TEMPLATE=%PROJECT_ROOT%\.githooks\commit-template.txt"

set "VERIFY_ERRORS=0"
set "VENV_ERRORS=0"
set "HOOK_ERRORS=0"
set "INIT_EXIT_CODE=0"

cls
echo.
echo !CYAN!======================================================!RESET!
echo !CYAN!         MyProject - Project Initialization!RESET!
echo !CYAN!======================================================!RESET!
echo.

echo !BLUE![STEP 1/4]!RESET! Verifying tools declared in CMake presets...
if not exist "%PRESETS_FILE%" (
    echo !RED![ERROR]!RESET! Missing file: %PRESETS_FILE%
    set /a VERIFY_ERRORS+=1
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
      "$ErrorActionPreference = 'Stop';" ^
      "$presetsPath = '%PRESETS_FILE%';" ^
      "$hostSystemName = [System.Environment]::OSVersion.Platform; if ($hostSystemName -eq 'Win32NT') { $hostSystemName = 'Windows' } elseif ($hostSystemName -match 'Unix') { $hostSystemName = 'Linux' }" ^
      "$json = Get-Content -Raw $presetsPath | ConvertFrom-Json;" ^
      "$configurePresets = @($json.configurePresets);" ^
      "$presetMap = @{};" ^
      "foreach ($preset in $configurePresets) { $presetMap[$preset.name] = $preset }" ^
      "function Merge-Hashtable([hashtable]$base, [object]$extra) {" ^
      "  if ($null -eq $extra) { return $base }" ^
      "  foreach ($p in $extra.PSObject.Properties) { $base[$p.Name] = $p.Value }" ^
      "  return $base" ^
      "}" ^
      "function Test-PresetCondition([object]$condition) {" ^
      "  if ($null -eq $condition) { return $true }" ^
      "  if ($condition.type -eq 'equals' -and $condition.lhs -eq '${hostSystemName}') { return $condition.rhs -eq $hostSystemName }" ^
      "  return $true" ^
      "}" ^
      "function Get-PresetState([string]$name, [hashtable]$seen) {" ^
      "  if ($seen.ContainsKey($name)) { throw ('Circular preset inheritance detected for ' + $name) }" ^
      "  if (-not $presetMap.ContainsKey($name)) { throw ('Unknown preset: ' + $name) }" ^
      "  $seen[$name] = $true;" ^
      "  $preset = $presetMap[$name];" ^
      "  $state = @{ Cache = @{}; Environment = @{}; Generator = $null; Applicable = $true };" ^
      "  if ($preset.PSObject.Properties.Name -contains 'inherits') {" ^
      "    $parents = @($preset.inherits);" ^
      "    foreach ($parent in $parents) {" ^
      "      $parentState = Get-PresetState $parent $seen.Clone();" ^
      "      foreach ($entry in $parentState.Cache.GetEnumerator()) { $state.Cache[$entry.Key] = $entry.Value }" ^
      "      foreach ($entry in $parentState.Environment.GetEnumerator()) { $state.Environment[$entry.Key] = $entry.Value }" ^
      "      if ($parentState.Generator) { $state.Generator = $parentState.Generator }" ^
      "      if (-not $parentState.Applicable) { $state.Applicable = $false }" ^
      "    }" ^
      "  }" ^
      "  if (-not (Test-PresetCondition $preset.condition)) { $state.Applicable = $false }" ^
      "  if ($preset.PSObject.Properties.Name -contains 'cacheVariables') { [void](Merge-Hashtable $state.Cache $preset.cacheVariables) }" ^
      "  if ($preset.PSObject.Properties.Name -contains 'environment') { [void](Merge-Hashtable $state.Environment $preset.environment) }" ^
      "  if ($preset.PSObject.Properties.Name -contains 'generator' -and $preset.generator) { $state.Generator = $preset.generator }" ^
      "  return $state" ^
      "}" ^
      "$checks = New-Object System.Collections.Generic.List[object];" ^
      "$skipped = New-Object System.Collections.Generic.List[string];" ^
      "foreach ($preset in $configurePresets) {" ^
      "  $state = Get-PresetState $preset.name @{};" ^
      "  if (-not $state.Applicable) { $skipped.Add($preset.name) | Out-Null; continue }" ^
      "  $toolVars = 'CMAKE_C_COMPILER','CMAKE_CXX_COMPILER','CMAKE_MAKE_PROGRAM','CMAKE_PREFIX_PATH';" ^
      "  foreach ($varName in $toolVars) {" ^
      "    if ($state.Cache.ContainsKey($varName) -and $state.Cache[$varName]) {" ^
      "      $rawValue = [string]$state.Cache[$varName];" ^
      "      $rawValue.Split(';') | ForEach-Object {" ^
      "        $entry = $_.Trim();" ^
      "        if ($entry) { $checks.Add([pscustomobject]@{ Preset = $preset.name; Source = $varName; Value = $entry }) | Out-Null }" ^
      "      }" ^
      "    }" ^
      "  }" ^
      "  if ($state.Generator -eq 'Ninja' -and -not ($state.Cache.ContainsKey('CMAKE_MAKE_PROGRAM'))) {" ^
      "    $checks.Add([pscustomobject]@{ Preset = $preset.name; Source = 'generator'; Value = 'ninja' }) | Out-Null" ^
      "  }" ^
      "  if ($state.Environment.ContainsKey('PATH') -and $state.Environment['PATH']) {" ^
      "    ([string]$state.Environment['PATH']).Split(';') | ForEach-Object {" ^
      "      $entry = $_.Trim();" ^
      "      if ($entry -and $entry -notmatch '^\$penv\{PATH\}$') { $checks.Add([pscustomobject]@{ Preset = $preset.name; Source = 'environment.PATH'; Value = $entry }) | Out-Null }" ^
      "    }" ^
      "  }" ^
      "}" ^
      "$checks.Insert(0, [pscustomobject]@{ Preset = 'global'; Source = 'cmake'; Value = 'cmake' });" ^
      "$seenKeys = @{}; $failures = 0;" ^
      "Write-Host ('[INFO] Host system detected for preset validation: ' + $hostSystemName);" ^
      "Write-Host '[INFO] Effective preset tool checks:';" ^
      "foreach ($check in $checks) {" ^
      "  $key = '{0}|{1}|{2}' -f $check.Preset, $check.Source, $check.Value;" ^
      "  if ($seenKeys.ContainsKey($key)) { continue }" ^
      "  $seenKeys[$key] = $true;" ^
      "  $value = $check.Value;" ^
      "  $resolved = $null;" ^
      "  $exists = $false;" ^
      "  if ($value -match '^[A-Za-z]:\\|^\\\\') {" ^
      "    $exists = Test-Path -LiteralPath $value;" ^
      "    $resolved = $value" ^
      "  } elseif ($value -match '^[A-Za-z0-9_.+-]+$') {" ^
      "    $cmd = Get-Command $value -ErrorAction SilentlyContinue;" ^
      "    if ($cmd) { $exists = $true; $resolved = $cmd.Source } else { $resolved = '(not found in PATH)' }" ^
      "  } else {" ^
      "    $expanded = [Environment]::ExpandEnvironmentVariables($value);" ^
      "    $exists = Test-Path -LiteralPath $expanded;" ^
      "    $resolved = $expanded" ^
      "  }" ^
      "  if ($exists) {" ^
      "    Write-Host ('[OK] preset=' + $check.Preset + ' source=' + $check.Source + ' value=' + $value + ' resolved=' + $resolved)" ^
      "  } else {" ^
      "    Write-Host ('[FAIL] preset=' + $check.Preset + ' source=' + $check.Source + ' value=' + $value + ' resolved=' + $resolved);" ^
      "    $failures++" ^
      "  }" ^
      "}" ^
      "foreach ($name in $skipped) { Write-Host ('[SKIP] preset=' + $name + ' condition does not match current host') }" ^
      "exit $failures"

    set "VERIFY_RESULT=!ERRORLEVEL!"
    if "!VERIFY_RESULT!"=="0" (
        echo !GREEN![OK]!RESET! All referenced CMake tools were found for the current host.
    ) else (
        echo !RED![ERROR]!RESET! Missing or invalid CMake tool entries detected for the current host: !VERIFY_RESULT!
        set /a VERIFY_ERRORS=!VERIFY_RESULT!
    )
)

echo.
echo !BLUE![STEP 2/4]!RESET! Creating Python virtual environment...
if not exist "%PM_HELPERS_SCRIPT%" (
    echo !RED![ERROR]!RESET! Missing helper module: %PM_HELPERS_SCRIPT%
    set /a VENV_ERRORS+=1
    goto hooks_step
)

if not exist "%REQUIREMENTS_FILE%" (
    echo !RED![ERROR]!RESET! Missing requirements file: %REQUIREMENTS_FILE%
    set /a VENV_ERRORS+=1
    goto hooks_step
)

call "%PM_HELPERS_SCRIPT%" :resolve_python_cmd PYTHON_CMD
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Python interpreter not found. Install Python or add it to PATH.
    set /a VENV_ERRORS+=1
    goto hooks_step
)

echo [INFO] Using Python launcher: %PYTHON_CMD%
if exist "%VENV_PYTHON%" (
    echo [INFO] Reusing existing virtual environment: %VENV_DIR%
) else (
    echo [INFO] Creating virtual environment in: %VENV_DIR%
    call %PYTHON_CMD% -m venv "%VENV_DIR%"
    if errorlevel 1 (
        echo !RED![ERROR]!RESET! Failed to create the virtual environment.
        set /a VENV_ERRORS+=1
        goto hooks_step
    )
)

if exist "%VENV_SITE_PACKAGES%\~ip" (
    echo [INFO] Removing stale pip rollback directory: %VENV_SITE_PACKAGES%\~ip
    rmdir /s /q "%VENV_SITE_PACKAGES%\~ip" >nul 2>&1
)
if exist "%VENV_SITE_PACKAGES%\~ip-25.2.dist-info" (
    echo [INFO] Removing stale pip metadata: %VENV_SITE_PACKAGES%\~ip-25.2.dist-info
    rmdir /s /q "%VENV_SITE_PACKAGES%\~ip-25.2.dist-info" >nul 2>&1
)

echo [INFO] Repairing bundled pip in the virtual environment...
call "%VENV_PYTHON%" -m ensurepip --upgrade
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to repair pip inside pyvenv.
    set /a VENV_ERRORS+=1
    goto hooks_step
)

echo [INFO] Installing baseline Python requirements from %REQUIREMENTS_FILE%
call "%VENV_PYTHON%" -m pip install -r "%REQUIREMENTS_FILE%"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to install Python requirements.
    set /a VENV_ERRORS+=1
    goto hooks_step
)

echo !GREEN![OK]!RESET! Python virtual environment is ready.
echo [INFO] Activate it with: pyvenv\Scripts\activate

:hooks_step
echo.
echo !BLUE![STEP 3/4]!RESET! Enabling git hooks...
git rev-parse --show-toplevel >nul 2>&1
if errorlevel 1 (
    echo [INFO] Git repository not found. Initializing repository in %PROJECT_ROOT%
    git init >nul 2>&1
    if errorlevel 1 (
        echo !RED![ERROR]!RESET! Failed to initialize the git repository.
        set /a HOOK_ERRORS+=1
        goto summary_step
    )
    echo !GREEN![OK]!RESET! Git repository initialized.
)

if not exist "%HOOKS_DIR%" (
    echo !RED![ERROR]!RESET! Missing hooks directory: %HOOKS_DIR%
    set /a HOOK_ERRORS+=1
    goto summary_step
)

git config core.hooksPath ".githooks/hooks"
if errorlevel 1 (
    echo !RED![ERROR]!RESET! Failed to configure core.hooksPath.
    set /a HOOK_ERRORS+=1
    goto summary_step
)

if exist "%HOOKS_TEMPLATE%" (
    git config commit.template ".githooks/commit-template.txt"
    if errorlevel 1 (
        echo !YELLOW![WARN]!RESET! Hooks path enabled, but commit template setup failed.
        set /a HOOK_ERRORS+=1
    ) else (
        echo [INFO] Commit template configured: .githooks/commit-template.txt
    )
) else (
    echo !YELLOW![WARN]!RESET! Commit template not found: %HOOKS_TEMPLATE%
    set /a HOOK_ERRORS+=1
)

if "!HOOK_ERRORS!"=="0" (
    echo !GREEN![OK]!RESET! Git hooks enabled through .githooks/hooks.
)

:summary_step
echo.
echo !BLUE![STEP 4/4]!RESET! Initialization summary
echo.
echo   Tool checks:
if "!VERIFY_ERRORS!"=="0" (
    echo     !GREEN![OK]!RESET! All tools referenced by host-applicable CMake presets are available.
) else (
    echo     !YELLOW![WARN]!RESET! Missing tools detected for host-applicable presets: !VERIFY_ERRORS!
)
echo   Python environment:
if "!VENV_ERRORS!"=="0" (
    echo     !GREEN![OK]!RESET! pyvenv created and dependencies installed.
) else (
    echo     !YELLOW![WARN]!RESET! Python environment setup requires attention.
)
echo   Git hooks:
if "!HOOK_ERRORS!"=="0" (
    echo     !GREEN![OK]!RESET! Hooks path and commit template are active.
) else (
    echo     !YELLOW![WARN]!RESET! Hooks scaffold is ready, but activation was incomplete.
)
echo.
echo   Default project structure:
echo     - apps\              application entry points and packaging-facing launchers
echo     - cmake\             shared CMake modules and helper toolchains
echo     - deploy\            deployment assets and installer configuration
echo     - docs\              architecture, specs, product notes and guidelines
echo     - include\myprj\     public headers exported by the project
echo     - src\               implementation code split by module/domain
echo     - tests\             data, configs, unit, e2e, pipeline and results folders
echo     - scripts\           project automation used by Project Manager
echo     - requirements\      baseline Python dependencies for local tooling
echo     - .githooks\         versioned git hooks and commit message template

if not "!VERIFY_ERRORS!"=="0" set "INIT_EXIT_CODE=1"
if not "!VENV_ERRORS!"=="0" set "INIT_EXIT_CODE=1"
if not "!HOOK_ERRORS!"=="0" set "INIT_EXIT_CODE=1"

echo.
if "!INIT_EXIT_CODE!"=="0" (
    echo !GREEN![DONE]!RESET! Project initialization completed successfully.
) else (
    echo !YELLOW![DONE WITH WARNINGS]!RESET! Initialization finished with recoverable issues.
)

popd >nul 2>&1
exit /b !INIT_EXIT_CODE!
