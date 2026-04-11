# Signal Editor

Signal Editor is a desktop waveform editing application built with C++23 and Qt 6 around a hexagonal architecture. It is intended for engineering workflows where time-series data must be inspected, corrected, generated, and exported quickly without pushing GUI concerns into the domain model or falling back to spreadsheet-driven editing.

The current product focuses on a pragmatic balance between interactive editing and long-term maintainability. The user experience is centered on a multi-document workspace, a direct-manipulation plot, a precise sample table, explicit interpolation control, and reliable persistence across engineering-friendly interchange formats.

## Product Summary

Signal Editor addresses a common workflow gap:

- spreadsheets are flexible but error-prone for waveform manipulation
- heavyweight simulation or calibration suites are often too slow for small correction loops
- ad hoc scripts are powerful but usually inaccessible to non-developer users and difficult to audit

Signal Editor provides a focused local desktop tool that lets engineers:

- load one or more signal documents into a shared workspace
- switch between active files without losing in-memory state
- inspect and edit a selected signal in either a plot-first or table-first workflow
- create synthetic signals from common templates
- preserve interpolation semantics and enumerated state mappings across supported formats
- save the result back to the filesystem in a form suitable for downstream tooling

## Key Capabilities

- Multi-document workspace with active-document switching
- Interactive plot editing with drag, insert, remove, and Gaussian brushing for numeric signals
- Dedicated table editing workflow with label-aware enumerated value support
- Shared interpolation control available regardless of whether the user is in the plot or table view
- Signal creation from constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated templates
- Multi-format import and export for CSV, TSV/TXT, JSON, and SpreadsheetML XML
- Enumerated signals rendered with textual state labels in the plot and table
- Document-scoped undo support for interactive editing workflows
- Clean separation between domain, use cases, ports, and adapters

## Repository Layout

```text
signal-editor/
├── apps/signal_editor/gui/              # GUI entry point and composition root
├── cmake/                               # Shared CMake helpers and project options
├── deploy/config/                       # Deployment metadata and packaging descriptors
├── docs/                                # Product, architecture, specification, and guidelines
├── include/myprj/                       # Generated/public version headers
├── scripts/                             # Build, packaging, and workflow helpers
├── src/common/                          # Shared infrastructure-neutral support types
├── src/signal_editor/                   # Domain, use cases, ports, adapters, and public API
├── tests/                               # Unit tests and reusable fixtures
├── CHANGELOG.md                         # Release-facing change history
├── CONTRIBUTING.md                      # Engineering contribution guide
├── GOVERNANCE.md                        # Repository governance and decision model
├── SECURITY.md                          # Security policy and reporting guidance
└── README.md                            # Project overview and onboarding entry point
```

## Architecture at a Glance

Signal Editor uses a ports-and-adapters design with a deliberately narrow application shell.

```text
apps/gui -> api -> core/usecases -> ports <- adapters
                      |
                      -> core/domain
```

This yields three practical benefits:

- waveform rules remain testable without Qt
- persistence details remain replaceable without rewriting editing logic
- UI evolution can continue without polluting the domain with framework types

The main architectural layers are:

- `core/domain/`: waveform entities, interpolation semantics, enumerated-state invariants, and editing primitives
- `core/usecases/`: orchestration of load, save, create, replace, rename, remove, and interpolation change flows
- `ports/`: repository abstraction consumed by use cases
- `adapters/filesystem/`: CSV and multi-format persistence implementations
- `adapters/qt/`: workspace shell, plot widget, table widget, dialogs, and presentation logic
- `api/`: composition-facing facade used by executables

## Supported File Formats

Signal Editor supports multiple interchange formats that map into the same internal signal model.

### CSV

The default tabular format uses the first column as time and all remaining columns as signals.

```csv
time,throttle,brake,steer
0.0,0.0,0.0,0.0
0.1,0.2,0.0,0.1
0.2,0.4,0.0,0.2
```

Metadata rows may be emitted before the header to preserve interpolation and enumeration details.

```csv
# interpolation,step,linear
# enum_map,FALSE:0|TRUE:1,
time,enable,torque
0.0,FALSE,0.0
0.5,TRUE,12.0
1.0,FALSE,4.0
```

### Inline Enumerated Bootstrap

When `# enum_map` is absent, CSV import can bootstrap enumeration mappings from inline `label:value` tokens.

```csv
time,enable,gear
0.0,FALSE:0,PARK:0
0.1,TRUE:1,DRIVE:3
0.2,TRUE,DRIVE
```

### TSV / TXT

Tab-delimited files are treated as spreadsheet-friendly plain-text equivalents of the CSV model and are useful for Excel-oriented workflows.

### JSON

JSON is intended for automation and tool integration. It supports explicit signal definitions, interpolation metadata, enumeration mappings, and sample arrays.

### SpreadsheetML XML

SpreadsheetML XML (Excel 2003 XML) is supported for import and export. It is a practical interoperability path for Excel users who need a structured workbook-like format without introducing native binary Excel parsing.

### Explicitly Not Supported

Native `.xlsx` and `.xls` workbook binaries are not currently supported. Convert those files to CSV, TSV/TXT, or SpreadsheetML XML before using them with Signal Editor.

## Enumerated Signal Model

Enumerated signals are first-class citizens in the current product.

An enumerated signal associates human-readable labels with numeric values, for example:

- `FALSE -> 0`
- `TRUE -> 1`
- `RUN -> 2`

Important behavior:

- enumerated mappings require unique labels and unique numeric values
- enumerated signals always behave as `step` interpolated signals
- table editing uses labels rather than raw numeric values when a mapping exists
- plot rendering uses textual Y-axis state labels where possible
- imported and exported tabular formats preserve mappings through `# enum_map` or inline bootstrap tokens

Reusable fixtures are available in `tests/01.data/` for CSV, inline CSV, JSON, and SpreadsheetML XML enumerated examples.

## Current Workspace UX Model

The central editing workspace is organized around two dedicated tabs:

- `Plot`: optimized for fast visual reshaping and direct manipulation
- `Table`: optimized for exact sample-level edits and row-based reasoning

The interpolation selector now lives outside the table-specific view so it remains accessible in both workflows. This was a deliberate product decision: interpolation is a signal-level property, not a table-only concern.

## Build

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
```

Release:

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

## Test

```bash
ctest --preset linux-gcc-debug --output-on-failure
```

Use the matching Windows preset when validating GUI-side integration on the Windows toolchain.

## Development Expectations

A complete change should normally include every relevant artifact:

- implementation
- automated tests where behavior changed
- documentation updates where user-facing or architectural behavior changed
- specification or architecture updates when design intent changed
- changelog updates for visible functionality or workflow changes

## Documentation Index

- [Governance](GOVERNANCE.md)
- [Contributing](CONTRIBUTING.md)
- [Security](SECURITY.md)
- [Changelog](CHANGELOG.md)
- [Product vision](docs/product/vision.md)
- [Product requirements](docs/product/prd.md)
- [Software requirements specification](docs/specs/srs.md)
- [Architecture overview](docs/architecture/architecture_overview.md)
- [C4 context](docs/architecture/c4_context.md)
- [C4 container](docs/architecture/c4_container.md)
- [C4 component](docs/architecture/c4_component.md)
- [Hexagonal module structure](docs/architecture/hexagonal_skeleton_structure.md)
- [Naming conventions](docs/guidelines/naming_conventions.md)
- [Semantic versioning](docs/guidelines/semantic_versioning.md)
- [Repository customization guidance](docs/guidelines/template_customization.md)
