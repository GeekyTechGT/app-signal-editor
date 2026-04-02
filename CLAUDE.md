# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Purpose

This repository is a **reusable project scaffold** for C++ Qt applications following hexagonal (ports & adapters) architecture. The goal is a ready-to-use template with all boilerplate pre-configured so new projects can start structured and consistent from day one.

**Reference implementation:** `D:\GeekyTechRepos\bin-tools` — a mature production project that defines all the patterns and conventions this scaffold should replicate. When in doubt about structure or conventions, read from there.

## Build Commands

These are the commands used across projects built from this scaffold (targeting the generated output):

```bash
# Configure (Windows MinGW64)
cmake --preset windows-mingw64-debug
cmake --preset windows-mingw64-release

# Configure (Linux GCC)
cmake --preset linux-gcc-debug
cmake --preset linux-gcc-release

# Build
cmake --build --preset windows-mingw64-debug
cmake --build --preset linux-gcc-debug

# Run tests
ctest --preset linux-gcc-debug --output-on-failure

# Run a single test by name filter
ctest --preset linux-gcc-debug -R "test_name" --output-on-failure

# Enable sanitizers (debug builds only)
cmake --preset linux-gcc-debug -DBINPT_ENABLE_SANITIZERS=ON
```

**Windows interactive workflow:** `project_manager.bat` — launches a menu-driven shell for build, test, deploy, clean, and status tasks. All individual operations are delegated to `scripts/pm_*.bat` helpers.

## Scaffold Structure to Generate

The scaffold must produce the following layout for a named module (replace `<module>` and `<prefix>` with project-specific values):

```
<project>/
├── CMakeLists.txt                   # Top-level CMake, version, options, subdirs
├── CMakePresets.json                # Presets for Windows MinGW64 and Linux GCC
├── .gitignore
├── .clang-format
├── VERSION                          # Plain text semver string
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── SECURITY.md
├── LICENSE / NOTICE
├── project_manager.bat              # Windows interactive entry point
├── Dockerfile.ubuntu
├── Dockerfile.alpine
├── cmake/
│   ├── ProjectOptions.cmake         # Compiler warnings, sanitizer flags, MinGW DLL copy
│   └── ExternalLibraries.cmake      # FetchContent for nlohmann/json, GoogleTest, OpenSSL
├── scripts/
│   ├── pm_build.bat
│   ├── pm_test.bat
│   ├── pm_deploy.bat
│   ├── pm_clean.bat
│   ├── pm_pipeline.bat
│   ├── pm_status.bat
│   ├── pm_helpers.bat
│   ├── init_submodules.bat
│   ├── report_generate_html.bat
│   ├── package_customer.py
│   └── package_deploy_linux.sh
├── include/
│   └── <prefix>/                    # Public headers (namespace prefix)
├── src/
│   └── <module>/
│       ├── core/
│       │   ├── domain/              # Entities, value objects (no framework deps)
│       │   └── usecases/            # Business logic orchestration
│       ├── ports/                   # Abstract interfaces (pure virtual)
│       ├── adapters/
│       │   ├── cli/                 # CLI argument parsing & dispatch
│       │   ├── json/                # JSON config readers
│       │   ├── gui/                 # Qt widget adapters
│       │   └── filesystem/          # File I/O
│       ├── api/                     # Public module entry points
│       └── schema/                  # JSON schemas for config validation
├── apps/
│   └── <module>/
│       ├── cli/                     # CLI executable wrapper
│       ├── gui/                     # Qt GUI executable wrapper
│       └── docs/                    # Module-level docs (ABOUT, CHANGELOG, API ref, FAQ)
├── tests/
│   ├── 01.data/                     # Shared test input files
│   ├── 02.config/                   # JSON fixture configs
│   ├── 03.unit_test/<module>/       # GoogleTest unit tests per module
│   ├── 04.e2e_test/                 # Executable-level end-to-end tests
│   ├── 05.pipeline_test/            # Workflow automation scenarios
│   ├── 06.results/                  # Archived HTML reports
│   ├── test_results/                # Live CTest JUnit XML output
│   └── tmp/                         # Temporary test working directory
├── docs/
│   ├── architecture/
│   │   ├── architecture_overview.md
│   │   ├── c4_context.md
│   │   ├── c4_container.md
│   │   ├── c4_component.md
│   │   └── hexagonal_skeleton_structure.md
│   ├── product/
│   │   ├── vision.md
│   │   └── prd.md
│   └── guidelines/
│       ├── naming_conventions.md
│       └── semantic_versioning.md
├── deploy/
│   └── config/
│       ├── module_catalog.json
│       └── customer_profile.schema.json
└── compliance/
    └── (license attributions)
```

## Architecture Pattern

All projects generated from this scaffold use **Hexagonal Architecture (Ports & Adapters)**:

- **`core/domain/`** — entities and domain models, zero external dependencies
- **`core/usecases/`** — orchestrates domain logic, depends only on ports
- **`ports/`** — abstract interfaces that usecases depend on (dependency inversion)
- **`adapters/`** — concrete implementations of ports (CLI, JSON, GUI, filesystem)
- **`api/`** — thin public facade over usecases, the only entry point from `apps/`
- **`apps/`** — executables: they wire adapters to ports and call the api

The dependency rule: `apps → api → usecases → ports ← adapters`. The domain and usecases never depend on adapters or frameworks.

## Tech Stack

| Concern | Choice |
|---------|--------|
| Language | C++23 |
| Build system | CMake 3.25+ with Ninja |
| GUI | Qt 6.x (optional, enable via `BUILD_GUI=ON`) |
| Testing | GoogleTest 1.14+ via FetchContent |
| JSON | nlohmann/json 3.11+ via FetchContent |
| Crypto | OpenSSL 3.x (system-preferred, source fallback) |
| Formatting | clang-format |
| Windows compiler | MSYS2 MinGW64 |
| Linux compiler | GCC |

**Expected tool paths (Windows):**
- GCC: `C:/eng_apps/msys64/mingw64/`
- Qt: `C:/eng_apps/Qt/<version>/mingw_64/`

## Key Conventions

- Namespace prefix `binpt_` is used in the reference project — new projects should define their own consistent prefix throughout CMake targets, header guards, and namespaces.
- Each module produces at least one `*_core` static library, a CLI executable, and optionally a GUI executable.
- Test results output to `tests/test_results/` as JUnit XML; HTML reports generated separately by `report_generate_html.bat`.
- Build artifacts go to `build/<preset-name>/bin/` and `build/<preset-name>/lib/`.
- Auto-generated headers (e.g., `version.h`) are placed in `build/<preset-name>/generated/`.
