# CLAUDE.md

This file provides repository-specific guidance for AI coding agents working in the Signal Editor codebase.

## Repository Identity

This repository is not a generic scaffold. It is the active `Signal Editor` project.

Canonical identifiers:

- Product name: `Signal Editor`
- Primary module: `signal_editor`
- CMake target family: `signal_editor_*`
- Namespace root: `myprj::signal_editor`

Do not reintroduce scaffold wording or template-oriented assumptions into code or documentation.

## Product Focus

Signal Editor is a local desktop engineering application for waveform inspection, editing, generation, and export.

The current product emphasis is:

- multi-document workspace behavior
- plot-driven and table-driven editing of the active signal
- enumerated signal support with label/value mappings
- consistent interpolation handling across UI and persistence layers
- multi-format import/export for CSV, TSV/TXT, JSON, and SpreadsheetML XML

## Build Commands

### Preset Matrix

- Windows GUI presets: `windows-gcc-*`, `windows-clang-*`, `windows-MSVC-*`
- Linux host presets: `linux-gcc-*`, `linux-clang-*`
- Docker presets: `docker-gcc-*`, `docker-clang-*`


### Windows GUI

```bash
cmake --preset windows-gcc-debug
cmake --build --preset windows-gcc-debug
```

Release:

```bash
cmake --preset windows-gcc-release
cmake --build --preset windows-gcc-release
```

### Linux Core and Tests

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
```

## Architectural Rules

Signal Editor follows a hexagonal architecture.

Dependency rule:

```text
apps -> api -> usecases -> ports <- adapters
                |
                -> domain
```

Hard rules:

- do not introduce Qt dependencies into `core/domain/` or `core/usecases/`
- do not move filesystem parsing logic into the domain
- keep `apps/` thin and focused on composition
- keep user-visible format semantics implemented through adapters and explicit service flows

## Repository Areas That Matter Most

### `src/signal_editor/core/domain/`

This layer owns:

- `Signal`
- `SignalLibrary`
- sample invariants
- interpolation semantics
- enumerated-state invariants
- editing primitives such as insertion, replacement, and brush behavior

### `src/signal_editor/core/usecases/`

This layer coordinates:

- load and save requests
- signal replacement and removal
- interpolation changes
- repository-facing orchestration

### `src/signal_editor/adapters/filesystem/`

This layer owns:

- CSV metadata parsing
- multi-format dispatch by file extension
- JSON and SpreadsheetML XML mapping
- export behavior and round-trip fidelity concerns

### `src/signal_editor/adapters/qt/`

This layer owns:

- the main workspace shell
- plot and table widgets
- dialogs for creation and editing flows
- visual state synchronization with the active document and active signal

## Documentation Expectations

When repository behavior changes, update the relevant documentation rather than leaving drift behind.

Typical targets:

- `README.md`
- `CHANGELOG.md`
- `GOVERNANCE.md`
- `CONTRIBUTING.md`
- `SECURITY.md`
- `docs/product/*`
- `docs/specs/srs.md`
- `docs/architecture/*`

## Change Discipline

When making changes:

- prefer focused, coherent edits
- do not mix unrelated deletions into feature work
- keep generated or deployment files untouched unless the task explicitly requires them
- avoid reverting user or maintainer changes you did not make
- preserve test fixtures that document format behavior

## Quality Bar

A high-quality change in this repository usually means:

- code is architecturally coherent
- tests cover the changed behavior where practical
- documentation reflects the change
- user-visible behavior is recorded in the changelog
- commit messages are concise, professional, and in English
