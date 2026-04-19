# Signal Editor

<p align="center">
  <strong>Desktop waveform editing for engineering workflows, built with C++23, Qt 6, and a clean hexagonal architecture.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-v0.1.0-0f172a?style=for-the-badge" alt="Version 0.1.0">
  <img src="https://img.shields.io/badge/C%2B%2B-23-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++23">
  <img src="https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6">
  <img src="https://img.shields.io/badge/CMake-3.25%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.25+">
  <img src="https://img.shields.io/badge/Python-3.9%2B-3776AB?style=for-the-badge&logo=python&logoColor=white" alt="Python 3.9+">
  <img src="https://img.shields.io/badge/Architecture-Hexagonal-E11D48?style=for-the-badge" alt="Hexagonal Architecture">
</p>

<p align="center">
  <img src="https://github.com/GeekyTechGT/signal-editor/actions/workflows/ci.yml/badge.svg?branch=master" alt="Build status">
  <img src="https://img.shields.io/github/last-commit/GeekyTechGT/signal-editor/master?style=flat-square" alt="Last commit">
  <img src="https://img.shields.io/badge/License-Proprietary%20%7C%20Internal-C2410C?style=flat-square" alt="Proprietary license">
  <img src="https://img.shields.io/badge/Formats-CSV%20%7C%20TSV%20%7C%20JSON%20%7C%20SpreadsheetML-7C3AED?style=flat-square" alt="Supported formats">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-0EA5E9?style=flat-square" alt="Platforms">
  <img src="https://img.shields.io/badge/UI-Plot%20%2B%20Table%20Workflows-F97316?style=flat-square" alt="UI workflows">
  <img src="https://img.shields.io/badge/Signals-Numeric%20%2B%20Enumerated-059669?style=flat-square" alt="Signal types">
  <img src="https://img.shields.io/badge/i18n-English%20%7C%20Italian-6366F1?style=flat-square" alt="Languages">
  <img src="https://img.shields.io/badge/Theme-Dark%20%7C%20Light%20%7C%20System-1E293B?style=flat-square" alt="Themes">
</p>

## Overview

Signal Editor is a desktop application for loading, inspecting, editing, generating, and exporting time-series signals without pushing engineering users back into spreadsheets or forcing waveform rules into UI code.

It is designed for workflows where engineers need to:

- open one or more signal files and switch context quickly
- edit a signal visually in a plot-driven workflow
- refine samples precisely in a table-driven workflow
- preserve interpolation behavior and enumerated mappings during round-trip persistence
- generate synthetic signals from common templates instead of scripting them from scratch

The project intentionally keeps the domain model independent from Qt so waveform rules remain testable and maintainable over time.

## Highlights

- Multi-document workspace with active file switching
- Interactive plot editing: drag waypoints, Shift+drag for Gaussian brushing
- Plot navigation toolbar with zoom in/out, fit view, pan mode, and rectangle zoom mode
- Table-first editing with explicit sample control and guided row insertion
- Enumerated signal support with label/value mappings
- Shared interpolation control (linear / step) across plot and table workflows
- Signal generation for constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated patterns
- Import/export support for CSV, TSV/TXT, JSON, and SpreadsheetML XML
- Dark, light, and system theme support powered by QSS
- Application settings panel for theme, font, and language preferences
- Italian and English UI with runtime language switching
- Tooltip and status-hint coverage across the main workspace, plot controls, and editing panels
- Hexagonal architecture with isolated domain, use cases, ports, and adapters
- Python-driven build manager with dependency validation and Rich TUI

## Why This Project Exists

Signal Editor closes a practical gap between quick scripting and heavyweight engineering suites.

Spreadsheets are flexible but fragile for waveform manipulation. Ad hoc scripts are powerful but difficult to operationalize for non-developers. Large calibration and simulation tools are often too expensive in time and cognitive overhead for fast inspection-and-correction loops.

This project focuses on a narrower, more disciplined tool: one that feels fast in daily use, preserves data semantics, and stays architecturally clean as features evolve.

## Setup

### Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | 3.25+ | must be on `PATH` |
| GCC / MinGW-w64 | C++23-capable | see Build Matrix |
| Qt 6 | 6.7+ | including Qt LinguistTools |
| Python | 3.9+ | for the project manager |
| Git | any recent | submodule support required |

### Clone with submodules

```bash
git clone --recurse-submodules <repo-url>
# or, on an existing clone:
git submodule update --init --recursive
```

The external themes library lives at `external/res-qt-themes` and is populated automatically by the above command.

### Python environment

```bash
cd scripts
python -m venv .venv

# Windows
.venv\Scripts\activate
# Linux / macOS
source .venv/bin/activate

pip install -r ../requirements.txt
```

### Running the project manager

```bash
python project_manager.py
```

The interactive TUI lets you select a preset, configure, build, run tests, and deploy the application. It validates shared-library dependencies before invoking CMake and reports which variants are available if the required one is missing.

## Build Matrix

All supported presets are defined in `CMakePresets.json`. Clang-dependent presets and Docker flows have been removed from the repository. The supported toolchain baseline is now intentionally narrower and easier to validate:

- Windows GUI delivery: MinGW-w64 via `windows-mingw64-*`
- Linux host validation: GCC via `linux-gcc-*`
- Containerized core/test validation: GCC via `docker-gcc-*`

The project manager maps each preset to the corresponding `_shared_lib` variant automatically.

| Family | Presets | Purpose |
|--------|---------|---------|
| Windows MinGW64 | `windows-mingw64-debug`, `windows-mingw64-release` | Primary desktop GUI build and deployment path |
| Linux GCC | `linux-gcc-debug`, `linux-gcc-release` | Core library and unit/integration test workflows |
| Docker GCC | `docker-gcc-debug`, `docker-gcc-release` | Reproducible non-GUI CI-style validation |

### Manual CMake invocation

```bash
# Windows GUI
cmake --preset windows-mingw64-debug
cmake --build --preset windows-mingw64-debug

# Linux core + tests
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure

# Docker
cmake --preset docker-gcc-debug
cmake --build --preset docker-gcc-debug
```

> When building the GUI manually, pass `-DSIGNAL_EDITOR_DEPS_VARIANT=<variant>` to point CMake at the correct `_shared_lib` directory. The project manager handles this automatically.

## Dependencies

Signal Editor resolves shared workspace libraries from a central `_shared_lib` directory. The layout expected for each dependency is:

```text
_shared_lib/<name>/<version>/<variant>/
    include/
    lib/
    bin/        (Windows, DLL)
```

Dependencies declared in `project.json`:

| Library | Version | Purpose |
|---------|---------|---------|
| `lib-qt-utils` | 1.0.0 | Persistent app state, last-used paths, settings storage, window geometry/state |
| `lib-qt-custom-widgets` | 1.0.0 | Settings panel dialog, AppSettings |

The build fails with a verbose diagnostic listing all available variants if the required variant is not found. Build the dependency for the matching variant first, or adjust the `preset_to_shared_lib_variant` map in `project.json`.

## External Submodules

| Submodule | Path | Purpose |
|-----------|------|---------|
| `res-qt-themes` | `external/res-qt-themes` | Base `dark.qss` and `light.qss` stylesheets |

Themes are embedded into the binary as Qt resources and loaded at runtime. The application merges the base QSS with a small set of app-specific widget overrides.

## Supported Formats

Signal Editor persists a shared internal signal model across multiple interchange formats.

| Format | Extension | Notes |
|--------|-----------|-------|
| CSV | `.csv` | First column is time, remaining columns are signals |
| Tab-delimited | `.tsv`, `.txt` | Delimited tabular import/export |
| JSON | `.json` | Structured representation for richer tool interoperability |
| SpreadsheetML XML | `.xml` | Excel-compatible XML import/export path |

Enumerated signals are supported across all formats. When labels are present, the plot renders string state names on the Y axis instead of raw numeric values.

## Enumerated Signals

Signal Editor supports signals whose stored values are numeric but whose user-facing meaning is textual.

Examples:

- `FALSE -> 0`, `TRUE -> 1`
- `OFF -> 0`, `ON -> 1`, `ERROR -> 2`
- `IDLE -> 0`, `RUN -> 1`, `FAULT -> 7`

Enumerated behavior is supported across:

- CSV import/export with explicit enumeration metadata
- inline CSV bootstrap tokens such as `TRUE:1`
- JSON import/export with enumeration definitions
- new-signal creation dialog in the UI
- label-aware table editing
- plot rendering with state labels on the Y axis

## Settings

The application exposes a settings panel (requires `lib-qt-custom-widgets`) accessible from the button in the bottom-left corner of the status bar.

Available settings:

| Setting | Options |
|---------|---------|
| Theme | Dark, Light, System |
| Primary color | User-selected accent color |
| Accessibility | High contrast mode |
| Font | Family and size |
| Density | Compact, Normal, Spacious |
| Language | English, Italian |
| Behavior | Animation duration |

Theme changes apply immediately to the running application via the Qt style system. Language changes reload the active QTranslator and retranslate all visible strings without restarting.

Language switching now works in both directions:

- the bootstrap path applies the persisted language before splash and main-window construction
- changing the language from the settings panel retranslates the currently visible UI immediately

Application settings, last-used folders, and window geometry/state are persisted through `lib-qt-utils` (`QtUtils::AppState`).

### Settings Persistence Model

The application now persists the settings actually used by Signal Editor in a version-scoped store so each release line can keep its own settings set.

- App identifier source: `project.json` → `name`
- Current application id: `signal-editor`
- Current settings namespace: `v1.0.0`
- Windows storage root used by the application: `HKEY_CURRENT_USER\\Software\\signal-editor\\v1.0.0`

This is distinct from the older unversioned settings path. The startup flow now:

1. creates `QtUtils::AppState("signal-editor", "v1.0.0")`
2. reads persisted UI keys from that versioned store
3. force-applies theme, font, density, behavior, and language on startup
4. saves back to the same versioned namespace only when the user presses `Save`

Persisted UI keys:

- `ui/theme`
- `ui/language`
- `ui/font_family`
- `ui/font_size`
- `ui/high_contrast`
- `ui/widget_density`
- `ui/animation_duration_ms`
- `ui/primary_color`

This versioned layout lets users upgrade without automatically overwriting settings from a different application line. It also makes troubleshooting easier because the persistence scope is explicit and deterministic.

When `lib-qt-custom-widgets` is not linked, the settings button opens an informational message instead.

## Internationalization

The UI is fully internationalized using the Qt translation framework.

| Locale | File | Status |
|--------|------|--------|
| English | `resources/translations/signal_editor_en.ts` | Complete |
| Italian | `resources/translations/signal_editor_it.ts` | Complete |

Translation files are compiled to `.qm` format at build time and installed to `<output-dir>/translations/`. The application loads the translation matching the system locale on startup, and respects the language override from application settings at runtime.

The startup path installs the persisted translator before constructing the splash screen and `MainWindow`, so launch-time UI text and runtime settings-panel language changes use the same translation assets.

The `%n`-based plural strings are handled with native Qt plural forms, so English and Italian `.qm` outputs should report the same number of finished translations when generated from the same source tree.

## Deployment

The Python project manager deploys the Windows GUI to `deploy/app/<preset>/` by default.

The deploy step:

- copies the application executable
- copies every DLL declared in `project.json` dependencies
- runs `windeployqt --compiler-runtime` to bundle Qt and MinGW runtime dependencies
- copies the application `.qm` translation files into `translations/`

To update translations after adding new translatable strings:

```bash
lupdate src/ apps/ -ts resources/translations/signal_editor_en.ts resources/translations/signal_editor_it.ts
lrelease resources/translations/signal_editor_en.ts resources/translations/signal_editor_it.ts
```

## Table Editing Behavior

The table workflow is designed to keep sample insertion predictable rather than behaving like a generic spreadsheet.

- **+ Sample** appends a new row at the end of the active signal

## Plot Navigation Behavior

The plot toolbar separates navigation actions from direct waveform editing.

- **Zoom in / Zoom out** shrink or expand the visible time range while keeping the current focus centered
- **Fit view** restores the full visible range of the selected signal
- **Pan mode** turns left-drag into horizontal viewport panning without editing samples
- **Rect mode** turns left-drag into rectangle zoom focused on a selected time region
- the mouse wheel also performs plot zooming
- visible range edits in `t start` / `t end` keep the plot synchronized with the toolbar state

Plot rendering is clipped to the internal content rect so samples, handles, and fill areas stay inside plot bounds during normal view, zoom, and drag interactions.
- the default time value is always greater than the last sample time
- the default `y(t)` value copies the last sample value
- the new row is selected immediately and opened in edit mode so the user can proceed without extra clicks

## Architecture

Signal Editor follows a ports-and-adapters (hexagonal) design. The dependency rule flows strictly inward.

```text
apps/gui → api → core/usecases → ports ← adapters
                      │
                      └→ core/domain
```

Layer responsibilities:

| Layer | Path | Owns |
|-------|------|------|
| Domain | `src/signal_editor/core/domain/` | Signal, SignalLibrary, interpolation semantics, editing primitives, enumerated-state invariants |
| Use Cases | `src/signal_editor/core/usecases/` | Load/save orchestration, signal replacement and removal, interpolation change coordination |
| Ports | `src/signal_editor/core/ports/` | Repository abstractions consumed by use cases |
| Filesystem adapters | `src/signal_editor/adapters/filesystem/` | Multi-format parsing, CSV metadata, JSON mapping, SpreadsheetML XML |
| Qt adapters | `src/signal_editor/adapters/qt/` | Workspace shell, plot/table widgets, theme engine, dialogs |
| API | `src/signal_editor/api/` | Composition-facing facade for executables |

Qt and filesystem dependencies never cross into the domain or use-case layers.

## Repository Layout

```text
signal-editor/
├── apps/signal_editor/gui/              # GUI entry point and Qt resource compilation
├── cmake/                               # Shared CMake helpers and project options
├── docs/                                # Product, architecture, specification, and guidelines
├── external/                            # Git submodules
│   └── res-qt-themes/                   # Base QSS theme files (dark.qss, light.qss)
├── include/signal_editor/                       # Generated/public version headers
├── resources/                           # Runtime-facing static assets and translation sources
│   ├── img/                             # Application images, logos, splash assets
│   └── translations/                    # Qt .ts source translation files
├── scripts/                             # Build manager and workflow helpers
│   ├── project_manager.py               # Interactive build TUI (Python + Rich)
│   └── .venv/                           # Python virtual environment (not committed)
├── src/signal_editor/                   # Domain, use cases, ports, adapters, and public API
├── tests/                               # Unit tests and reusable fixtures
├── project.json                         # Dependency and variant configuration
├── requirements.txt                     # Python dependency manifest
├── CHANGELOG.md                         # Release-facing change history
├── CONTRIBUTING.md                      # Engineering contribution guide
├── GOVERNANCE.md                        # Repository governance and decision model
├── SECURITY.md                          # Security policy and reporting guidance
└── README.md                            # Project overview and onboarding entry point
```

## Documentation Map

- [CONTRIBUTING.md](CONTRIBUTING.md): contribution workflow and engineering expectations
- [GOVERNANCE.md](GOVERNANCE.md): ownership, change discipline, and decision model
- [SECURITY.md](SECURITY.md): security reporting and maintenance policy
- [CHANGELOG.md](CHANGELOG.md): release-facing history
- [docs/product/vision.md](docs/product/vision.md): product direction and positioning
- [docs/product/prd.md](docs/product/prd.md): product requirements
- [docs/specs/srs.md](docs/specs/srs.md): software requirements specification
- [docs/architecture/architecture_overview.md](docs/architecture/architecture_overview.md): architectural narrative
- [docs/architecture/c4_context.md](docs/architecture/c4_context.md): system context view
- [docs/architecture/c4_container.md](docs/architecture/c4_container.md): container-level breakdown
- [docs/architecture/c4_component.md](docs/architecture/c4_component.md): component-level view

## Testing

The project uses GoogleTest for unit-level validation and keeps waveform logic isolated from Qt wherever practical.

```bash
ctest --preset linux-gcc-debug --output-on-failure
```

The repository includes GitHub Actions CI for Linux and Windows validation, GCC-based coverage artifact generation for debug builds, and a tag-driven packaging workflow for release candidates. Clang is no longer part of the supported validation matrix, so any future reintroduction would need an explicit build-policy update plus matching documentation changes.

## Technical Baseline

| Aspect | Baseline |
|--------|---------|
| Language standard | C++23 |
| UI framework | Qt 6 (Widgets, LinguistTools) |
| Build system | CMake 3.25+ |
| JSON library | nlohmann/json |
| Unit test framework | GoogleTest |
| CI | GitHub Actions (Linux + Windows) |
| Coverage | gcovr on Linux GCC debug builds |
| Delivery | Tag-driven release packaging with archived artifacts |
| Architecture | Hexagonal (ports and adapters) |
| Theme engine | QSS from embedded Qt resources |
| i18n | Qt Linguist (.ts → .qm), runtime switching |
| Build manager | Python 3.9+ with Rich TUI |

## Contributing

Contributions should preserve the current architectural direction: domain logic stays framework-independent, persistence logic stays adapter-bound, and documentation stays synchronized with code changes.

Start here:

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CLAUDE.md](CLAUDE.md)

## License

This project is proprietary and intended for internal or explicitly authorized use only. See [LICENSE.md](LICENSE.md) for the repository notice and handling restrictions.

Because the repository is private and not distributed as open source, the README intentionally uses a proprietary license badge rather than an OSS license badge.
