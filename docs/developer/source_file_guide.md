# Source File Guide

This document is a beginner-friendly map of the most relevant files in the Signal Editor repository. It explains what each file is for, why it exists, and what to be careful about before changing it.

Use this guide when you are new to the project and need to answer questions such as:

- Where does the application start?
- Which files contain business rules?
- Which files are Qt-only UI code?
- Where are CSV, JSON, XML, and XLSX handled?
- Where should a new feature be added?
- Which files are build system glue rather than product logic?

The repository follows a ports-and-adapters architecture. The short version is:

- `src/signal_editor/core/domain` owns pure signal rules.
- `src/signal_editor/core/usecases` owns application operations.
- `src/signal_editor/ports` defines abstract boundaries.
- `src/signal_editor/adapters/filesystem` implements file formats.
- `src/signal_editor/adapters/qt` implements the desktop UI.
- `apps/signal_editor/gui` wires the Qt application together.
- `cmake` and `CMakeLists.txt` files describe how all targets are built.

## How To Read This Guide

Start with the architecture sections if you are changing behavior. Start with the build sections if the project does not compile. Start with the UI adapter section if the problem is visible in the desktop application.

Do not treat every file as equally important. Some files are central orchestration points, while others are small helpers. The files that most often require careful reasoning are:

- `src/signal_editor/core/domain/signal.cpp`
- `src/signal_editor/core/usecases/signal_editor_service.cpp`
- `src/signal_editor/adapters/qt/main_window.cpp`
- `src/signal_editor/adapters/qt/signal_plot_widget.cpp`
- `src/signal_editor/adapters/qt/signal_table_panel.cpp`
- `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp`
- `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp`

## Top-Level Build Files

### `CMakeLists.txt`

This is the root CMake entry point. It defines project-wide options, discovers dependencies, configures output folders, includes helper modules from `cmake/`, and adds the main source and test directories.

Relevant details:

- It is responsible for high-level build shape, not individual source file ownership.
- It should stay small and declarative.
- Add new product targets in lower-level `CMakeLists.txt` files unless the change truly affects the whole repository.

### `CMakePresets.json`

This file defines named configure/build presets. Presets encode toolchain and output-directory choices so contributors can build consistently.

Relevant details:

- Windows GUI development normally uses the `windows-mingw64-*` presets.
- Linux and Docker presets are useful for core/test validation.
- If a preset fails before compiling, inspect this file and `project.json` together.

### `project.json`

This file describes project identity, dependency versions, and preset-to-shared-library variant mapping used by the Python project manager.

Relevant details:

- The application id and settings scope derive from project metadata.
- Shared dependencies such as `lib-qt-utils` and `lib-qt-custom-widgets` are resolved through this configuration.
- Changing this file can affect build, deploy, settings persistence, and documentation.

## `cmake/`

### `cmake/ExternalLibraries.cmake`

This helper imports prebuilt external shared libraries from a local layout. It creates imported CMake targets and records DLLs that must be copied for runtime.

Relevant details:

- This file is build infrastructure, not application logic.
- It deals with DLL/import-library pairing on Windows.
- If a dependency is present but not linked, inspect this file and the `_shared_lib` directory layout.

### `cmake/ProjectOptions.cmake`

This file centralizes compiler warnings, sanitizer/coverage settings, MinGW runtime copying, and common target configuration.

Relevant details:

- Warning policy is controlled here.
- MinGW runtime DLL copy behavior lives here.
- Do not hide real warnings here unless the warning is noisy for a clear third-party or toolchain reason.

## `include/signal_editor/`

### `include/signal_editor/version.h.in`

Template for a generated public version header. CMake substitutes version values during configuration.

Relevant details:

- Do not include project logic here.
- Keep generated version symbols stable because other code may include them.

### `include/signal_editor/signal_editor_version.h.in`

Template for the generated Signal Editor version header used by the core target.

Relevant details:

- CMake writes the configured header into the build directory.
- Source files should include the generated output path, not edit generated files manually.

## `src/signal_editor/CMakeLists.txt`

This file defines the core library and the Qt adapter library.

Relevant details:

- `signal_editor_core` contains domain, use case, filesystem adapters, and API facade code.
- `signal_editor_qt` is built only when GUI support is enabled.
- `signal_editor_core` must not depend on Qt widgets.
- Libzip discovery for native XLSX support is configured here.
- When adding a new `.cpp` file under `src`, remember to add it to the correct target here.

## Core Domain

The domain layer contains the rules that should stay independent from Qt, filesystems, dialogs, and UI state.

### `src/signal_editor/core/domain/sample_point.h`

Defines the smallest signal data unit: a time/value pair.

Relevant details:

- `t` is the timestamp.
- `y` is the numeric value.
- Enumerated signals still store numeric values; labels are mapping metadata on `Signal`.

### `src/signal_editor/core/domain/result.h`

Defines a small success/error result type used by use cases and adapters.

Relevant details:

- This avoids throwing exceptions through normal edit workflows.
- Use exceptions for programmer errors and parsing failures where the current code already expects them.

### `src/signal_editor/core/domain/signal.h`

Declares the `Signal` entity.

Relevant details:

- A signal owns ordered samples.
- It owns interpolation mode.
- It may own enumeration mappings.
- It exposes editing primitives such as moving, inserting, removing, and offsetting samples.
- Public methods enforce invariants, so callers should not manipulate samples directly.

### `src/signal_editor/core/domain/signal.cpp`

Implements signal invariants and editing behavior.

Relevant details:

- Timestamps must be strictly increasing for ordered construction.
- `insert_sample` uses sorted insertion and replaces an existing sample if the timestamp is effectively equal.
- Enumerated signals snap values to known enumeration entries.
- Interpolation behavior is implemented here, not in the plot widget.

Be careful when changing this file:

- Every format adapter and UI workflow depends on these invariants.
- Large-signal performance matters because this file is called by both generated signals and imported files.
- If you change timestamp rules, update import diagnostics, table behavior, plot behavior, and tests.

### `src/signal_editor/core/domain/signal_library.h`

Declares a container for multiple signals.

Relevant details:

- A document sheet is represented as a `SignalLibrary`.
- Many workflows assume signals in one library share a time axis.
- The library is the unit passed between repositories, service methods, and UI panels.

### `src/signal_editor/core/domain/signal_library.cpp`

Implements library operations such as adding, replacing, removing, and clearing signals.

Relevant details:

- Keep this file focused on collection semantics.
- Cross-signal edit orchestration belongs in `SignalEditorService`, not here.

## Core Use Cases

### `src/signal_editor/core/usecases/signal_editor_service.h`

Declares the application service used by the GUI and API facade.

Relevant details:

- This is the main non-UI interface for loading, saving, and editing signals.
- The UI should call service methods instead of mutating the domain directly.
- The service is intentionally independent of Qt.

### `src/signal_editor/core/usecases/signal_editor_service.cpp`

Implements application-level operations.

Relevant details:

- Load and save operations call repository ports.
- Sample insertion/removal is applied across the shared time axis when needed.
- Interpolated insert values are calculated for non-active signals so libraries remain aligned.
- This is the right place for operations that coordinate several domain objects.

Be careful when changing this file:

- The GUI undo stack stores whole document snapshots, so service changes affect undo behavior.
- Shared time-axis behavior is central to plot and table correctness.
- Add tests in `test_signal_editor_service.cpp` for new service-level behavior.

## Ports

### `src/signal_editor/ports/signal_repository.h`

Defines the repository interface used by the service layer.

Relevant details:

- The service depends on this abstraction rather than concrete file formats.
- Concrete adapters implement this contract for CSV, JSON, XML, XLSX, and delimited text.
- Keep this interface stable unless multiple adapters need the new operation.

## Filesystem Adapters

Filesystem adapters translate external file formats into the domain model.

### `src/signal_editor/adapters/filesystem/workbook_model.h`

Defines workbook-oriented data structures shared by XML and XLSX adapters.

Relevant details:

- `WorkbookDocument` contains one or more named sheets.
- Each sheet contains a `SignalLibrary`.
- `explicit_sheet_names` records whether sheet names came from the source format.

### `src/signal_editor/adapters/filesystem/tabular_signal_rows.h`

Declares helpers for converting generic tabular rows into a `SignalLibrary`.

Relevant details:

- CSV, TSV/TXT, SpreadsheetML XML, and XLSX all reuse this logic.
- This is where common tabular rules are centralized.

### `src/signal_editor/adapters/filesystem/tabular_signal_rows.cpp`

Implements common tabular parsing.

Relevant details:

- Requires a `time` or `t` column.
- Requires timestamps to be strictly increasing.
- Detects numeric and enumerated signal columns.
- Handles explicit enumeration metadata and inferred textual labels.
- Produces clear errors for malformed rows.

Be careful when changing this file:

- A change here affects several import formats at once.
- User-facing import diagnostics in the Qt layer assume some of these error messages.
- If you add a new validation error, consider improving `format_load_failure_message` in `MainWindow`.

### `src/signal_editor/adapters/filesystem/csv_signal_repository.h`

Declares the CSV repository adapter.

Relevant details:

- CSV is treated as a single-sheet document.
- It relies on the shared tabular parser for most semantic checks.

### `src/signal_editor/adapters/filesystem/csv_signal_repository.cpp`

Implements CSV load/save.

Relevant details:

- Handles comma-separated text files.
- Preserves enumerated metadata in the repository's supported CSV convention.
- Delegates signal construction rules to the common tabular layer and domain.

### `src/signal_editor/adapters/filesystem/delimited_signal_repository.h`

Declares the adapter for generic delimited text.

Relevant details:

- Used for TSV and TXT-style imports/exports.
- Shares most behavior with CSV but uses a different delimiter.

### `src/signal_editor/adapters/filesystem/delimited_signal_repository.cpp`

Implements tab-delimited persistence.

Relevant details:

- Useful when users need spreadsheet-friendly text without comma escaping issues.
- Should remain aligned with CSV semantics unless a format-specific reason exists.

### `src/signal_editor/adapters/filesystem/json_signal_repository.h`

Declares the JSON repository adapter.

Relevant details:

- JSON supports richer structure than tabular formats.
- It can preserve mappings and interpolation metadata explicitly.

### `src/signal_editor/adapters/filesystem/json_signal_repository.cpp`

Implements JSON import/export using `nlohmann_json`.

Relevant details:

- Good place to inspect the canonical structured persistence shape.
- Keep schema changes backward-aware because users may have saved JSON files.

### `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h`

Declares support for SpreadsheetML XML workbooks.

Relevant details:

- This format can contain multiple worksheets.
- The Qt UI treats it as workbook-capable.

### `src/signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.cpp`

Implements SpreadsheetML XML parsing and writing.

Relevant details:

- Converts worksheet tables into `WorkbookDocument`.
- Reuses common tabular conversion for each worksheet.
- Preserves workbook shape during save where possible.

### `src/signal_editor/adapters/filesystem/xlsx_signal_repository.h`

Declares native XLSX workbook support.

Relevant details:

- Native XLSX support depends on libzip being available at build time.
- The adapter exposes workbook load/save operations as well as repository behavior.

### `src/signal_editor/adapters/filesystem/xlsx_signal_repository.cpp`

Implements native XLSX parsing and writing.

Relevant details:

- Reads XLSX files as ZIP packages.
- Parses workbook metadata, worksheet XML, shared strings, and the app-specific `METADATA` sheet.
- Writes plain data worksheets plus a dedicated `METADATA` worksheet for enumerated mappings.
- When a worksheet fails tabular validation, the exception includes the sheet name so the UI can show a useful error.

Be careful when changing this file:

- XLSX files are ZIP packages with several related XML files. A small relationship or content-type mistake can make Excel reject the workbook.
- Keep data worksheets readable by humans. Do not add hidden app-only rows to user-facing data sheets.
- Add tests in `test_xlsx_signal_repository.cpp` for every workbook layout change.

### `src/signal_editor/adapters/filesystem/signal_file_repository.h`

Declares extension-based dispatch across concrete repositories.

Relevant details:

- The GUI uses this adapter when loading arbitrary user-selected files.
- It hides format-specific repository classes behind one entry point.

### `src/signal_editor/adapters/filesystem/signal_file_repository.cpp`

Implements file extension dispatch.

Relevant details:

- `.csv`, `.tsv`, `.txt`, `.json`, `.xml`, and `.xlsx` are routed here.
- Add new file formats here after implementing and testing the concrete adapter.

## API Facade

### `src/signal_editor/api/signal_editor_api.h`

Declares a small facade intended for composition code and potential future non-Qt consumers.

Relevant details:

- Keep this layer thin.
- Do not put UI behavior here.

### `src/signal_editor/api/signal_editor_api.cpp`

Implements the facade over the service and repositories.

Relevant details:

- Good place for simple examples and integration hooks.
- Avoid duplicating service logic here.

## Qt Adapter

The Qt adapter owns desktop presentation and user interaction. It should not own low-level signal rules.

### `src/signal_editor/adapters/qt/constants.hpp`

Centralizes UI constants such as app display names, settings keys, resource paths, and sizing values.

Relevant details:

- Use this file when the same UI literal is needed in multiple Qt files.
- Do not put domain constants here.

### `src/signal_editor/adapters/qt/branding.h`

Declares branding helpers.

Relevant details:

- Used to build icons and apply application identity consistently.

### `src/signal_editor/adapters/qt/branding.cpp`

Implements branding helpers.

Relevant details:

- Builds the runtime application icon from Qt resources.
- Keeps icon setup out of `main.cpp` and `MainWindow`.

### `src/signal_editor/adapters/qt/icon_theme.h`

Declares icon theme helpers used by toolbar and panel actions.

Relevant details:

- Keeps icon loading and fallback behavior in one place.

### `src/signal_editor/adapters/qt/icon_theme.cpp`

Implements icon lookup and styling behavior.

Relevant details:

- Uses resources from `resources/img` through `resources.qrc`.
- If an icon is missing in the UI, check this file and the resource file.

### `src/signal_editor/adapters/qt/theme.h`

Declares theme-related helpers and visual settings.

Relevant details:

- Bridges app settings into practical visual choices.
- The UI should prefer this helper over hard-coded colors.

### `src/signal_editor/adapters/qt/theme.cpp`

Implements theme loading and application-specific QSS augmentation.

Relevant details:

- Loads base QSS from `external/res-qt-themes`.
- Adds Signal Editor-specific selectors and polish.
- Dark/light behavior and selected-item contrast fixes belong here when they are style issues.

### `src/signal_editor/adapters/qt/main_window.h`

Declares the main desktop shell.

Relevant details:

- Defines document, sheet, undo, and async-load state structures.
- Declares slots for menu actions, file list actions, signal list actions, plot actions, table actions, settings, and language changes.
- This is the orchestration header, not a widget styling file.

### `src/signal_editor/adapters/qt/main_window.cpp`

Implements the main desktop shell.

Relevant details:

- Owns the multi-document workspace model.
- Coordinates active document, active sheet, active signal, and visible signals.
- Owns load/save/reload flows.
- Runs large file loading on a worker thread and shows non-blocking progress.
- Runs large generated-signal creation on a worker thread and shows modal progress while the signal is being created.
- Formats import failures into user-facing diagnostic popups.
- Connects plot and table signals to `SignalEditorService`.
- Applies settings, theme, language, and translation updates.

Be careful when changing this file:

- It is large because it coordinates many widgets. Prefer extracting helpers when adding new behavior.
- Do not move domain rules into this file. It should coordinate, not redefine signal semantics.
- UI operations must stay on the Qt thread. Worker threads should return data that the UI applies later.

### `src/signal_editor/adapters/qt/file_list_panel.h`

Declares the file list widget.

Relevant details:

- Exposes file selection, open, reload, rename, remove, and details requests as signals.
- Does not load files directly.

### `src/signal_editor/adapters/qt/file_list_panel.cpp`

Implements file list UI behavior.

Relevant details:

- Single-click selects files.
- Double-click opens a file.
- Context menu actions are emitted to `MainWindow`.

### `src/signal_editor/adapters/qt/signal_list_panel.h`

Declares the signal list widget.

Relevant details:

- Shows signals in the active document/sheet.
- Emits active-signal and visibility changes.

### `src/signal_editor/adapters/qt/signal_list_panel.cpp`

Implements signal list UI behavior.

Relevant details:

- Checkboxes control plot/table visibility.
- Selection controls the active editable signal.
- Styling must preserve text contrast in dark and light themes.

### `src/signal_editor/adapters/qt/signal_lod_pyramid.h`

Declares the LOD data model and helper functions for dense plot rendering.

Relevant details:

- Defines min/max buckets and pyramid levels.
- Exposes level selection and visible-range query helpers.
- This module is separate from `SignalPlotWidget` so rendering strategy is testable.

### `src/signal_editor/adapters/qt/signal_lod_pyramid.cpp`

Implements min/max LOD pyramid construction and queries.

Relevant details:

- Level 0 is represented by raw samples.
- Higher levels aggregate pairs of previous buckets.
- Min/max buckets preserve spikes better than averages.
- Tests live in `test_signal_lod_pyramid.cpp`.

### `src/signal_editor/adapters/qt/signal_plot_widget.h`

Declares the interactive plot widget.

Relevant details:

- Exposes navigation modes, zoom operations, edit signals, and time-view state.
- Does not directly call repositories or save files.

### `src/signal_editor/adapters/qt/signal_plot_widget.cpp`

Implements plotting, interaction, and dense-signal rendering.

Relevant details:

- Renders raw polylines when zoomed in.
- Renders min/max LOD envelopes when zoomed out.
- Handles sample drag, insert, remove, segment move, Gaussian brush, panning, wheel zoom, and rectangle zoom.
- Emits semantic edit requests to `MainWindow`.

Be careful when changing this file:

- Coordinate transforms and hit-testing must stay consistent.
- Large signals must not be rendered sample-by-sample when zoomed out.
- Keep rendering clipped to the plot content rectangle.

### `src/signal_editor/adapters/qt/signal_table_panel.h`

Declares the table widget for sample inspection and editing.

Relevant details:

- Shows one shared time column and one value column per visible signal.
- Only the active signal's value column is editable.
- Emits semantic edit requests to `MainWindow`.

### `src/signal_editor/adapters/qt/signal_table_panel.cpp`

Implements table rendering and editing.

Relevant details:

- Large signals are previewed with a capped number of materialized rows to keep the UI responsive.
- `+ Sample` inserts near the selected row so the new row is visible even when the table is truncated.
- Enumerated signals use label-aware editing.
- Table filtering is local to the visible rows.

Be careful when changing this file:

- Visual row numbers are not always the same as full signal size when the table is truncated.
- Keep table edits coordinated with the shared time-axis behavior in `SignalEditorService`.

## GUI Application

### `apps/signal_editor/gui/CMakeLists.txt`

Defines the Qt executable target.

Relevant details:

- Adds `bootstrap.cpp`, `main.cpp`, and `resources.qrc`.
- Compiles translation `.ts` files into `.qm` files.
- Copies compiled translations next to the executable.
- Links the GUI against `signal_editor_qt` and `signal_editor_core`.

### `apps/signal_editor/gui/main.cpp`

Contains the executable entry point.

Relevant details:

- Creates the application object.
- Sets application icon and identity.
- Installs startup translation.
- Creates the service and launches the desktop shell.

### `apps/signal_editor/gui/bootstrap.h`

Declares startup helpers.

Relevant details:

- Keeps startup orchestration out of `main.cpp`.
- Includes the safe application wrapper and launch helpers.

### `apps/signal_editor/gui/bootstrap.cpp`

Implements application bootstrap.

Relevant details:

- Reads persisted language before main-window construction.
- Installs the bootstrap translator.
- Creates and styles the splash screen.
- Launches the main desktop shell.
- Catches unexpected Qt event exceptions in the safe application wrapper.

### `apps/signal_editor/gui/resources.qrc`

Qt resource manifest.

Relevant details:

- Registers icons, images, QSS files, and translations for runtime access.
- If a resource path fails at runtime, check this file first.

## Resources

### `resources/img/*`

Application icons and UI action icons.

Relevant details:

- SVG icons are used for toolbar and panel actions.
- PNG/ICO files are used for branding and platform integration.
- Keep icon names stable because code refers to resource paths.

### `resources/translations/signal_editor_en.ts`

Qt translation source for English.

Relevant details:

- English still uses a `.ts` file so Qt can generate a complete `.qm`.
- Add new user-visible strings here when not using automated `lupdate`.

### `resources/translations/signal_editor_it.ts`

Qt translation source for Italian.

Relevant details:

- Runtime language switching depends on this file being complete.
- When adding a string in C++, add the Italian translation before shipping.

## Scripts

### `scripts/project_manager.py`

Interactive project manager for configure/build/test/deploy workflows.

Relevant details:

- Validates dependency variants.
- Runs CMake presets.
- Handles deployment helpers for the GUI.

### `scripts/generate_lod_sample.py`

Generates dense CSV fixtures for LOD testing.

Relevant details:

- Useful for creating repeatable large-signal datasets.
- Generated files may be large; decide intentionally whether to commit them.

### `scripts/customize_project.py`

Project identity customization helper.

Relevant details:

- Used when adapting the repository template.
- Be cautious because identity changes can affect docs, package names, settings keys, and generated files.

### `scripts/scaffold_apps.py`

Scaffolding helper for application directories.

Relevant details:

- Useful when adding a new app target.
- Prefer editing generated output before committing it.

### `scripts/package_customer.py`

Packaging helper for customer-facing deliverables.

Relevant details:

- Keep packaging behavior aligned with deploy output and documentation.

### `scripts/package_deploy_linux.sh`

Linux deploy packaging script.

Relevant details:

- Shell script intended for Linux packaging workflows.
- Validate it with `bash -n` after editing.

## Unit Tests

### `tests/03.unit_test/signal_editor/CMakeLists.txt`

Defines the GoogleTest executable for Signal Editor.

Relevant details:

- Add new test source files here.
- Some adapter implementation files are linked directly for focused tests.

### `tests/03.unit_test/signal_editor/test_signal.cpp`

Tests the `Signal` domain entity.

Relevant details:

- Covers construction, interpolation, sample edits, enumeration rules, and ordered-sample construction.

### `tests/03.unit_test/signal_editor/test_signal_library.cpp`

Tests library-level collection behavior.

Relevant details:

- Use this file for add/remove/replace behavior on `SignalLibrary`.

### `tests/03.unit_test/signal_editor/test_signal_editor_service.cpp`

Tests application service behavior.

Relevant details:

- Covers repository orchestration, shared sample edits, interpolation changes, offsets, and error reporting.

### `tests/03.unit_test/signal_editor/test_signal_lod_pyramid.cpp`

Tests LOD pyramid construction and selection.

Relevant details:

- Covers min/max bucket generation, spike preservation, level selection, and bounded visible-range queries.

### `tests/03.unit_test/signal_editor/test_csv_repository.cpp`

Tests CSV import/export behavior.

Relevant details:

- Covers header handling, time-column rules, enumeration metadata, and round-trip persistence.

### `tests/03.unit_test/signal_editor/test_delimited_signal_repository.cpp`

Tests TSV/TXT-style delimited persistence.

Relevant details:

- Ensures mapping and value semantics match CSV-style expectations.

### `tests/03.unit_test/signal_editor/test_json_signal_repository.cpp`

Tests JSON import behavior.

Relevant details:

- Focuses on structured enumerated data and labels.

### `tests/03.unit_test/signal_editor/test_spreadsheet_xml_signal_repository.cpp`

Tests SpreadsheetML XML workbook behavior.

Relevant details:

- Covers named time columns, multi-worksheet round trips, and rejection of invalid sheets.

### `tests/03.unit_test/signal_editor/test_xlsx_signal_repository.cpp`

Tests native XLSX behavior.

Relevant details:

- Covers workbook round trips, ZIP contents, plain header rows, metadata schema, and real fixture loading.
- Add tests here whenever XLSX layout or diagnostics change.

### `tests/03.unit_test/signal_editor/test_signal_file_repository.cpp`

Tests extension-based dispatch.

Relevant details:

- Ensures the generic repository routes file extensions to the correct adapter.

## Test Data

### `tests/01.data/`

Contains reusable input fixtures.

Relevant details:

- Small fixtures should be committed.
- Large generated fixtures should be committed only when they serve a clear regression or performance purpose.
- `sample_automotive_signals_2_tab_new.xlsx` is an edited workbook fixture that verifies loading an XLSX with an added signal and strictly increasing time values.

### `tests/01.data/generated/`

Contains generated LOD stress fixtures.

Relevant details:

- Files here exercise raw and bucket-sized LOD rendering scenarios.
- They are useful for manual UI performance checks.

## Documentation

### `docs/architecture/`

Architecture-level material: context, containers, components, and hexagonal structure.

Relevant details:

- Read this before changing module boundaries.
- Keep diagrams and prose aligned with actual source files.

### `docs/developer/`

Developer-facing implementation guides.

Relevant details:

- Read these before modifying workspace state, plot rendering, persistence, or source structure.
- This `source_file_guide.md` is the broadest map; the other developer docs go deeper into individual topics.

### `docs/product/`, `docs/specs/`, `docs/user/`, `docs/guidelines/`

Product intent, requirements, user manual, and contribution conventions.

Relevant details:

- Update these when behavior changes in a user-visible way.
- Keep user documentation free of internal implementation details unless they help troubleshooting.

## Generated And Ignored Files

Some files may appear locally but should not be edited as source:

- `build/` directories are CMake output.
- `.qm` translation files are generated from `.ts` files.
- `scripts/__pycache__/` contains Python bytecode.
- IDE temporary files and spreadsheet lock files such as `~$*.xlsx` should not be committed.

When in doubt, check `git status --short` and prefer committing only intentional source, docs, tests, and fixture changes.
