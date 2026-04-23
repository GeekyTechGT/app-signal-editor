# Filesystem and Persistence

## Purpose

This document explains how file loading, saving, and reload behavior are
organized.

## Main Classes Involved

### `ISignalRepository`

File:

- [`src/signal_editor/ports/signal_repository.h`](../../src/signal_editor/ports/signal_repository.h)

Role:

- abstract persistence contract used by the application service

### `SignalEditorService`

Role:

- delegates load/save requests through the repository port
- keeps the active in-memory `SignalLibrary`

### Filesystem adapters

Location:

- `src/signal_editor/adapters/filesystem/`

Role:

- implement actual format parsing and exporting

Examples:

- CSV
- delimited text
- JSON
- SpreadsheetML XML
- XLSX

## Load Flow

1. `MainWindow` requests a load.
2. For user-selected file loads, `MainWindow::open_paths()` starts a worker thread so the GUI remains responsive.
3. A non-blocking progress dialog reports progress and lets the user cancel before the next file starts.
4. `SignalEditorService` calls the repository.
5. The repository resolves the concrete adapter for the file type.
6. Single-sheet adapters build one `SignalLibrary`.
7. Workbook-aware adapters build one `WorkbookDocument` containing one or more sheet-local `SignalLibrary` instances.
8. If parsing fails, `MainWindow` formats the raw parser error into a user-facing diagnostic.
9. `MainWindow` stores the result inside one `LoadedDocument` with an active sheet index.

## Save Flow

1. The active document is synchronized into the service.
2. The service delegates to the repository.
3. The concrete adapter exports the current library or workbook snapshot.
4. For XLSX, data worksheets are written as plain tables and enumerated mappings are written to a dedicated `METADATA` worksheet.
5. On success the document dirty flag is cleared.

## Reload Flow

Reload is not just a load. It replaces one existing document from disk while
trying to keep the rest of the workspace stable.

The reload path therefore has to restore:

- the reloaded library
- the visible-signal set
- the active signal where still meaningful
- the active sheet where still meaningful
- file panel and signal list state

Reload is especially important because it provides a fast recovery path when the
user removes or edits signals by mistake and wants to recover the on-disk file
content instead of stepping backward through undo.

The current implementation deliberately treats reload as a clear-and-rebind
operation:

- signal list is detached
- plot is detached
- table is detached
- the file is reloaded from disk
- the UI is rebound to the reloaded document state

This avoids stale pointers and stale UI snapshots surviving across reload.

## XLSX-Specific Notes

Native XLSX support is workbook-aware and intentionally Excel-friendly.

Important rules:

- each worksheet is imported as one sheet-local `SignalLibrary`
- the user-facing data sheets remain plain tabular sheets
- the first row of each saved data worksheet is the header row (`time` plus signal columns)
- enumerated mappings are not saved inline in data sheets
- a dedicated worksheet named `METADATA` stores enumerated mappings using:
  - `sheet`
  - `signal_name`
  - `enum_label`
  - `enum_value`

This allows the same signal name to exist on multiple sheets with different
enumerated mappings without ambiguity.

When an XLSX worksheet fails validation, `XlsxSignalRepository` wraps the
underlying tabular parser error with the worksheet name. This is important for
user support because a workbook can contain several valid sheets and one invalid
sheet. The UI can then show an error such as `Sheet 'INPUT_1': Time column is
not strictly increasing at row 6`.

## Import Diagnostics

The raw filesystem adapters throw concise technical exceptions. The Qt shell
turns common parser failures into more helpful popups:

- duplicate or descending values in the `time` column
- non-numeric `time` values
- missing required `time` or `t` columns
- inconsistent row width compared with the header

Keep these layers separate. Adapters should report the precise parser failure;
the UI should explain what the user can do next.

## Why Persistence Lives Outside the Core

Parsing format-specific files inside the core would make the domain layer depend
on representation details. The repository port avoids that by keeping:

- file I/O
- delimiter rules
- JSON layout
- SpreadsheetML-specific parsing

outside the domain and use-case layers.
