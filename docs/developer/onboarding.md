# Developer Onboarding

## Purpose

This document gives new contributors a practical reading path into the Signal
Editor codebase. It is intentionally written as an onboarding aid rather than a
formal specification.

The goal is to answer these questions quickly:

- Where does application state live?
- Which classes are responsible for UI behavior?
- Which layers are safe to change for a given feature?
- What assumptions must be preserved to avoid regressions?

## Recommended Reading Order

If you are completely new to the codebase, read the following in order:

1. [`README.md`](../../README.md) for product scope and supported workflows
2. [`docs/architecture/architecture_overview.md`](../architecture/architecture_overview.md)
3. [`src/signal_editor/core/usecases/signal_editor_service.h`](../../src/signal_editor/core/usecases/signal_editor_service.h)
4. [`src/signal_editor/adapters/qt/main_window.h`](../../src/signal_editor/adapters/qt/main_window.h)
5. [`plot_subsystem.md`](plot_subsystem.md)
6. [`workspace_and_selection.md`](workspace_and_selection.md)

## Mental Model

Signal Editor is easiest to understand if you think of it as three nested
systems:

1. The domain model
   This defines what a signal is, how interpolation works, how enumerated
   values behave, and what edits are legal.
2. The application service layer
   This turns user intent into domain changes and persistence operations.
3. The Qt workspace shell
   This manages multi-document state, workbook sheet selection, signal
   visibility, active selection, plot state, table state, and settings.

The project stays maintainable because those responsibilities are separated
rather than merged inside widgets.

## Important Directories

### `src/signal_editor/core/domain/`

Contains the waveform entities and the rules that must remain true regardless
of UI technology.

Important examples:

- `signal.h/.cpp`
- `signal_library.h/.cpp`
- `sample_point.h`

### `src/signal_editor/core/usecases/`

Contains the application service used by the UI.

Important example:

- `signal_editor_service.h/.cpp`

### `src/signal_editor/ports/`

Contains abstraction boundaries used by the use-case layer.

Important example:

- `signal_repository.h`

### `src/signal_editor/adapters/filesystem/`

Contains concrete persistence logic and shared parsing helpers for the various
formats.

### `src/signal_editor/adapters/qt/`

Contains the desktop UI and is the most important folder for day-to-day feature
work on the application shell.

Important classes:

- `MainWindow`
- `SignalPlotWidget`
- `SignalTablePanel`
- `SignalListPanel`
- `FileListPanel`

## What `MainWindow` Actually Does

New contributors often underestimate how much workspace coordination lives in
`MainWindow`. It is not just a top-level widget.

It is responsible for:

- loading and saving documents
- reloading from disk
- switching worksheets inside workbook-aware formats
- keeping one document-local undo stack per loaded file
- distinguishing selected files from the file actually opened into the editors
- tracking which signals are visible in the plot and table
- tracking which signal is currently active for editing
- preserving plot zoom while list state changes
- synchronizing widgets after edits

If a change affects more than one panel or mixes document state with widget
state, `MainWindow` is usually the correct place to inspect first.

## Key Invariants

Contributors should preserve these invariants:

- Domain code must not depend on Qt.
- File-format adapters must not encode UI rules.
- The active editable signal is workspace state, not just a visual selection.
- The opened file is workspace state, not just the currently highlighted file
  row.
- Signals imported from one file share the same time axis from the editor’s
  point of view.
- Plot and table must remain two views over the same logical workspace state.
- Enumerated signals are always step-interpolated by domain rule.
- Workbook formats are sheet-aware in the UI even though the service edits one
  active `SignalLibrary` at a time.

## Common Feature Placement Guide

Use this rule of thumb:

- Change `core/domain` if the waveform rule itself changes.
- Change `core/usecases` if the operation exposed to adapters changes.
- Change `adapters/filesystem` if import/export representation changes.
- Change `adapters/qt` if the workflow or presentation changes but domain rules
  stay the same.

## Common Risk Areas

These areas deserve extra care during review:

- selected file versus opened file behavior
- workbook sheet switching and per-sheet state
- signal visibility and active selection rebinding
- plot zoom preservation when widgets refresh
- undo and reload interactions
- shared time-axis edits across multiple plotted signals
- enumerated-signal remapping

## Good First Debugging Strategy

When debugging a UI issue:

1. Identify whether the problem is state, rendering, or persistence.
2. Verify whether `MainWindow` document state is coherent.
3. Check whether the plot/table/list widgets are only reflecting bad state or
   are producing it.
4. Only then inspect low-level widget painting or event handling.

This order avoids spending time on paint code when the actual issue is usually
selection or document-state orchestration.
