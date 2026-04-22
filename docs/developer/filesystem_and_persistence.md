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
2. `SignalEditorService` calls the repository.
3. The repository resolves the concrete adapter for the file type.
4. Single-sheet adapters build one `SignalLibrary`.
5. Workbook-aware adapters build one `WorkbookDocument` containing one or more sheet-local `SignalLibrary` instances.
6. `MainWindow` stores the result inside one `LoadedDocument` with an active sheet index.

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

## Why Persistence Lives Outside the Core

Parsing format-specific files inside the core would make the domain layer depend
on representation details. The repository port avoids that by keeping:

- file I/O
- delimiter rules
- JSON layout
- SpreadsheetML-specific parsing

outside the domain and use-case layers.
