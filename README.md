# MyProject

> Replace `MyProject`, `myprj`, `MYPRJ`, and `my_module` with the real project and module names before using this scaffold for production work. Use [`docs/guidelines/template_customization.md`](docs/guidelines/template_customization.md) as the mandatory rename and cleanup checklist.

## Overview

This repository is a cross-platform C++ scaffold oriented toward senior-level development workflows:

- CMake presets for Windows and Linux
- Hexagonal architecture as the default application structure
- CLI and GUI composition roots
- versioned Git hooks
- a local Python toolchain for reports and automation
- an interactive Windows project manager for day-to-day operations

## Module Inventory

The scaffold ships with one example module to demonstrate the structure. Replace it with the real module set for the project.

| Module      | Core Library           | CLI | GUI | Description |
|-------------|------------------------|-----|-----|-------------|
| `my_module` | `myprj_my_module_core` | Yes | Yes | Example module used to demonstrate the scaffold layout and build wiring. |

## Repository Structure

```text
MyProject/
├── CMakeLists.txt              # Top-level build configuration
├── CMakePresets.json           # Host-specific configure/build/test presets
├── .githooks/                  # Versioned git hooks and commit template
├── apps/                       # Executable entry points and composition roots
├── cmake/                      # Shared CMake modules and helper logic
├── deploy/                     # Deployment assets and packaging configuration
├── docs/                       # Architecture, specs, guidelines, product notes
├── include/myprj/              # Public headers exported by the project
├── requirements/               # Baseline Python tooling dependencies
├── scripts/                    # Project manager and automation scripts
├── src/                        # Implementation code by domain/module
├── tests/                      # Test data, config, unit, e2e, pipeline results
├── pyvenv/                     # Local Python virtual environment created by init
└── project_manager.bat         # Windows interactive entry point
```

## Quick Start

### First Customization Pass

Before feature development starts, complete the checklist in [docs/guidelines/template_customization.md](docs/guidelines/template_customization.md).

Minimum expected actions:

1. edit `project.metadata.json` with the real project metadata
2. run `Initialize Project` from `project_manager.bat`
3. run `Customize Project` from `project_manager.bat`
4. replace any remaining project-specific placeholder docs
5. adapt `CMakePresets.json` paths to the actual toolchain installation
6. validate build, test, and hook workflows on the intended host

### Project Metadata

The root file [`project.metadata.json`](project.metadata.json) is the single source of truth for project customization. Update it before running the customization step.

Current metadata fields:

- `project_name`: human-readable project name
- `project_slug`: filesystem-friendly project identifier for the project/repository
- `namespace_prefix`: lowercase namespace and include prefix
- `macro_prefix`: uppercase CMake/macro prefix
- `primary_module`: first functional module name
- `description`: default project description
- `vendor`: owning company or team
- `apps`: optional array of additional modules/apps to scaffold during initialization; each entry can be either a module name string or an object with `module`, `cli`, and `gui`

### Windows Bootstrap

Run:

```batch
project_manager.bat
```

Then choose `1 - Initialize Project`.

The initialization flow in [`scripts/init_project.bat`](scripts/init_project.bat) does the following:

1. Validates the tools referenced by `CMakePresets.json` for the current host only.
2. Creates or repairs the local virtual environment in `pyvenv/`.
3. Repairs bundled `pip` when needed and installs [`requirements/requirements.txt`](requirements/requirements.txt).
4. Scaffolds any additional modules/apps declared in `project.metadata.json` under `apps[]`.
5. Initializes Git automatically if the scaffold is not yet a repository.
6. Configures `core.hooksPath` to use the versioned hooks in `.githooks/hooks`.
7. Configures the shared commit template from `.githooks/commit-template.txt`.
8. Prints a concise summary of the default repository structure.

### Customize The Project

After initialization, update [`project.metadata.json`](project.metadata.json) and choose `2 - Customize Project` from `project_manager.bat`.

That step:

1. validates the metadata schema and naming conventions
2. performs a dry-run preview of planned replacements
3. writes a machine-readable JSON report for auditability
4. creates a versioned backup manifest before mutating files
5. applies placeholder substitutions across eligible text files
6. renames matching template paths such as `include/myprj` or `src/my_module`
7. validates that critical project placeholders are no longer present in managed files
8. leaves any extra apps generated during initialization intact
9. leaves `.git`, `pyvenv`, build directories, generated outputs, and project customization tooling untouched

The same wrapper also supports rollback of the latest customization backup.

Activate the local Python environment on Windows with:

```batch
pyvenv\Scripts\activate
```

### Prerequisites

**Windows**

- Git
- CMake 3.25+
- Ninja
- MSYS2 MinGW64 toolchain available at `C:/eng_apps/msys64/mingw64/`
- Qt 6.x for GUI builds available at `C:/eng_apps/Qt/<version>/mingw_64/`
- Python 3 with `venv`

**Linux**

- Git
- CMake 3.25+
- Ninja
- GCC with C++23 support
- G++ with C++23 support
- Python 3 with `venv`

Example Ubuntu/WSL setup:

```bash
sudo apt update
sudo apt install build-essential ninja-build cmake python3 python3-venv git
```

## Build

### Windows

```bash
cmake --preset windows-mingw64-debug
cmake --build --preset windows-mingw64-debug
```

Release:

```bash
cmake --preset windows-mingw64-release
cmake --build --preset windows-mingw64-release
```

### Linux

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Release:

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

## Test

### Windows

```bash
ctest --preset windows-mingw64-debug --output-on-failure
```

### Linux

```bash
ctest --preset linux-gcc-debug --output-on-failure
```

Run a specific test by filter:

```bash
ctest --preset windows-mingw64-debug -R "TestSuiteName" --output-on-failure
```

Adjust the preset to match the host you are using.

## Windows Interactive Workflow

```batch
project_manager.bat
```

The menu exposes these operations:

- `Initialize Project`
- `Customize Project`
- `Build Project`
- `Clean Build`
- `Clean All Builds`
- `Run Tests with HTML Report`
- `Run Test Pipeline`
- `Deploy`
- `Generate Installer`
- `Init Submodules`

## Git Hooks

The scaffold keeps Git hooks in version control instead of relying on ad-hoc local setup:

```bash
git config core.hooksPath .githooks/hooks
```

That configuration is applied automatically during project initialization.

### Hook Behavior

- `pre-commit`
  Validates staged content with `git diff --cached --check`. It blocks commits that contain whitespace issues or malformed patch content.
- `commit-msg`
  Checks the first line of the commit message. It rejects empty subjects and subjects longer than 72 characters.
- `prepare-commit-msg`
  Pre-populates a lightweight commit message structure when Git opens an empty message buffer.
- `pre-push`
  Blocks pushes if the working tree or index is dirty. The intent is to push only from a clean local state.

### Commit Template

The initialization step also configures [`.githooks/commit-template.txt`](.githooks/commit-template.txt) as the shared commit template.

Suggested headline format:

```text
<type>(<scope>): <summary>
```

Recommended types:

- `feat`
- `fix`
- `refactor`
- `docs`
- `test`
- `build`
- `ci`
- `chore`

## Customization Safety

The project customization flow is designed to be safer than a blind search-and-replace:

- dry-run before apply
- JSON report output in `.scaffold-backups/last_customize_report.json`
- backup manifest stored under `.scaffold-backups/`
- rollback support for the latest customization run
- post-apply validation for remaining critical placeholders

## Python Tooling

The repository uses a local Python environment for project automation and reporting. Baseline dependencies live in [requirements/requirements.txt](requirements/requirements.txt):

- `pytest`
- `pytest-html`
- `ruff`

This setup keeps Python tooling local to the repository and avoids polluting the system interpreter.

## Default Repository Layout

The scaffold starts with this intent:

- `apps/`: composition roots for CLI and GUI delivery mechanisms.
- `cmake/`: reusable CMake policies, options, warning profiles, and helpers.
- `deploy/`: packaging and deployment inputs.
- `docs/`: architecture, specifications, engineering guidance, and product context.
- `include/myprj/`: public API headers.
- `src/`: core implementation and module-specific code.
- `tests/`: test data, configs, unit tests, e2e tests, pipeline scenarios, and results.
- `scripts/`: automation entry points used by the project manager.
- `requirements/`: Python dependency pinning for local tooling.
- `.githooks/`: shared Git policy and commit hygiene.
- `pyvenv/`: local virtual environment created by initialization and ignored by Git.

## Architecture

This scaffold follows Hexagonal Architecture by default.

Dependency direction:

```text
apps -> api -> usecases -> ports <- adapters
```

The core domain must not depend on frameworks, UI, file systems, transport layers, or deployment concerns.

See [`docs/architecture/`](docs/architecture/) for the project-specific architecture once the scaffold is customized.

## Recommended Senior-Dev Conventions

To keep the scaffold effective in a real team, treat these as baseline expectations:

- keep the presets authoritative for host-specific builds
- avoid global Python package installs for repo automation
- keep Git hooks versioned and lightweight
- use commit subjects under 72 characters
- keep delivery code in `apps/` and domain logic in `src/`
- document deviations from the default structure in `docs/`
- prefer repeatable scripted workflows over manual local steps

## License

Add the project license before first release.
