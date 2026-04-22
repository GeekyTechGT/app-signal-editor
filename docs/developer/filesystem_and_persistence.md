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

## Load Flow

1. `MainWindow` requests a load.
2. `SignalEditorService` calls the repository.
3. The repository resolves the concrete adapter for the file type.
4. The adapter builds a `SignalLibrary`.
5. `MainWindow` stores that library inside one `LoadedDocument`.

## Save Flow

1. The active document is synchronized into the service.
2. The service delegates to the repository.
3. The concrete adapter exports the current library.
4. On success the document dirty flag is cleared.

## Reload Flow

Reload is not just a load. It replaces one existing document from disk while
trying to keep the rest of the workspace stable.

The reload path therefore has to restore:

- the reloaded library
- the visible-signal set
- the active signal where still meaningful
- file panel and signal list state

Reload is especially important because it provides a fast recovery path when the
user removes or edits signals by mistake and wants to recover the on-disk file
content instead of stepping backward through undo.

## Why Persistence Lives Outside the Core

Parsing format-specific files inside the core would make the domain layer depend
on representation details. The repository port avoids that by keeping:

- file I/O
- delimiter rules
- JSON layout
- SpreadsheetML-specific parsing

outside the domain and use-case layers.
