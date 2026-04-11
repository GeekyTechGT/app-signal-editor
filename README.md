# Signal Editor

<p align="center">
  <strong>Desktop waveform editing for engineering workflows, built with C++23, Qt 6, and a clean hexagonal architecture.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-v0.1.0-0f172a?style=for-the-badge" alt="Version 0.1.0">
  <img src="https://img.shields.io/badge/C%2B%2B-23-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++23">
  <img src="https://img.shields.io/badge/Qt-6-41CD52?style=for-the-badge&logo=qt&logoColor=white" alt="Qt 6">
  <img src="https://img.shields.io/badge/CMake-3.25%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white" alt="CMake 3.25+">
  <img src="https://img.shields.io/badge/Architecture-Hexagonal-E11D48?style=for-the-badge" alt="Hexagonal Architecture">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Formats-CSV%20%7C%20TSV%20%7C%20JSON%20%7C%20SpreadsheetML-7C3AED?style=flat-square" alt="Supported formats">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-0EA5E9?style=flat-square" alt="Platforms">
  <img src="https://img.shields.io/badge/UI-Plot%20%2B%20Table%20Workflows-F97316?style=flat-square" alt="UI workflows">
  <img src="https://img.shields.io/badge/Signals-Numeric%20%2B%20Enumerated-059669?style=flat-square" alt="Signal types">
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
- Interactive plot editing for numeric signals
- Table-first editing with explicit sample control
- Enumerated signal support with label/value mappings
- Shared interpolation control across plot and table workflows
- Signal generation for constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated patterns
- Import/export support for CSV, TSV/TXT, JSON, and SpreadsheetML XML
- Hexagonal architecture with isolated domain, use cases, ports, and adapters

## Why This Project Exists

Signal Editor closes a practical gap between quick scripting and heavyweight engineering suites.

Spreadsheets are flexible but fragile for waveform manipulation. Ad hoc scripts are powerful but difficult to operationalize for non-developers. Large calibration and simulation tools are often too expensive in time and cognitive overhead for fast inspection-and-correction loops.

This project focuses on a narrower, more disciplined tool: one that feels fast in daily use, preserves data semantics, and stays architecturally clean as features evolve.

## Quick Start

### Windows GUI build

```bash
cmake --preset windows-gcc-debug
cmake --build --preset windows-gcc-debug
```

Release:

```bash
cmake --preset windows-gcc-release
cmake --build --preset windows-gcc-release
```

### Linux core/test build

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
```

### Docker build

```bash
cmake --preset docker-gcc-debug
cmake --build --preset docker-gcc-debug
```

## Build Matrix

The repository exposes a full preset matrix so local and CI workflows can share the same vocabulary.

| Family | Presets |
|--------|---------|
| Windows GCC | `windows-gcc-debug`, `windows-gcc-release` |
| Windows Clang | `windows-clang-debug`, `windows-clang-release` |
| Windows MSVC | `windows-MSVC-debug`, `windows-MSVC-release` |
| Linux GCC | `linux-gcc-debug`, `linux-gcc-release` |
| Linux Clang | `linux-clang-debug`, `linux-clang-release` |
| Docker GCC | `docker-gcc-debug`, `docker-gcc-release` |
| Docker Clang | `docker-clang-debug`, `docker-clang-release` |

## Supported Formats

Signal Editor persists a shared internal signal model across multiple interchange formats.

| Format | Status | Notes |
|--------|--------|-------|
| CSV | Supported | First column is time, remaining columns are signals |
| TSV / TXT | Supported | Delimited tabular import/export |
| JSON | Supported | Structured representation for richer tool interoperability |
| SpreadsheetML XML | Supported | Excel-friendly XML import/export path |

Enumerated signals are supported across the workflow. When labels are available, the plot can render string states on the Y axis instead of raw numeric values.

## Enumerated Signals

Signal Editor supports signals whose stored values are numeric but whose user-facing meaning is textual.

Examples:

- `FALSE -> 0`, `TRUE -> 1`
- `OFF -> 0`, `ON -> 1`, `ERROR -> 2`
- `IDLE -> 0`, `RUN -> 1`, `FAULT -> 7`

The repository currently supports enumerated behavior across:

- CSV import/export with explicit enumeration metadata
- inline CSV bootstrap tokens such as `TRUE:1`
- JSON import/export with enumeration definitions
- new-signal creation in the UI
- label-aware table editing
- plot rendering with state labels on the Y axis

## Architecture

Signal Editor follows a ports-and-adapters design.

```text
apps/gui -> api -> core/usecases -> ports <- adapters
                      |
                      -> core/domain
```

This separation keeps responsibilities clear:

- `core/domain/` owns waveform invariants, interpolation semantics, and editing primitives
- `core/usecases/` orchestrates application behavior around repositories and user actions
- `ports/` defines the abstractions consumed by the use cases
- `adapters/filesystem/` implements persistence and format mapping
- `adapters/qt/` implements the workspace shell and user interaction layer
- `api/` exposes a clean composition-facing facade for executables

## Repository Layout

```text
signal-editor/
├── apps/signal_editor/gui/              # GUI entry point and composition root
├── cmake/                               # Shared CMake helpers and project options
├── deploy/config/                       # Deployment metadata and packaging descriptors
├── docs/                                # Product, architecture, specification, and guidelines
├── include/myprj/                       # Generated/public version headers
├── scripts/                             # Build, packaging, deployment, and workflow helpers
├── src/common/                          # Shared infrastructure-neutral support types
├── src/signal_editor/                   # Domain, use cases, ports, adapters, and public API
├── tests/                               # Unit tests and reusable fixtures
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

The project uses GoogleTest for unit-level validation and keeps the waveform logic isolated from Qt wherever practical.

Typical test flow:

```bash
ctest --preset linux-gcc-debug --output-on-failure
```

Windows users can also drive build, test, deploy, and packaging flows through `project_manager.bat`.

## Current Technical Baseline

- Language standard: C++23
- UI framework: Qt 6
- Build system: CMake 3.25+
- JSON library: nlohmann/json
- Unit test framework: GoogleTest
- Primary architectural style: Hexagonal architecture

## Contributing

Contributions should preserve the current architectural direction: domain logic stays framework-independent, persistence logic stays adapter-bound, and documentation stays synchronized with code changes.

Start here:

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CLAUDE.md](CLAUDE.md)

## License

No license file is currently published in the repository. Add one before distributing the project externally under a defined legal model.
