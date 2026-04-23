# Workspace and Selection Model

## Purpose

This document explains how document state, visible signals, and the active
signal are managed in the desktop application.

## Why This Needs Its Own Document

Several regressions in the project have come from confusing four related but
different concepts:

- one or more files selected in the file list
- the file currently opened into the editors
- a row selected in the signal list widget
- a signal visible in the plot/table
- the active signal used for editing

They often move together, but they are not the same thing.

## Source of Truth

The source of truth for workspace state is `MainWindow::LoadedDocument`.

Important fields:

- `sheets`
- `active_sheet_index`
- `visible_signal_indices` inside each sheet
- `active_signal_index` inside each sheet
- `undo_stack`
- `dirty`

This means widgets are not authoritative. They are views over that document
state.

Large file loading is also coordinated by `MainWindow`. The parser work runs in
a background thread, but loaded documents are only applied back to the workspace
on the Qt thread.

## File-Level Model

Each loaded input file gets one `LoadedDocument`. That document keeps its own:

- one or more sheet-local waveform libraries
- active sheet
- active signal
- set of visible signals
- undo history

This is why switching between files can preserve user context cleanly.

## Visibility Rules

`visible_signal_indices` describes which signals are currently plotted and shown
as value columns in the table.

Consequences:

- plot visibility is document-local
- table column layout is document-local
- interpolation changes now target visible signals, not only the active one
- LOD cache content is derived from the currently bound library and visible
  signal set, so cache invalidation belongs to the binding/refresh flow

## Active Signal Rules

`active_signal_index` describes which signal is editable.

Rules that should remain true:

- the active signal may be `-1` when nothing should be editable
- clicking a signal row should make it checked and then active
- unchecking the active signal should make another visible signal active, or
  `-1` if none remain

The active signal must not be inferred only from `QListWidget::currentRow()`
because widget selection can change temporarily during checkbox toggles and
refreshes.

## Undo Model

Undo is tracked per document, not globally.

That is why `UndoState` stores:

- a full `SignalLibrary` snapshot
- the active signal index associated with that snapshot

Restoring only waveform data would not be enough to restore a coherent editing
context.

## Reload Model

Reload from disk replaces the library snapshot of one document. The reload flow
must then realign:

- the reloaded library
- the reloaded sheet set
- visible signals
- active signal
- list widget state
- plot and table bindings

If a bug appears only after reload, inspect `MainWindow::onFileReloadRequested`
first.

## File List Semantics

The file list now distinguishes selection from opening.

Rules that should remain true:

- single-click selects a file row
- `Ctrl+click` extends file selection for batch deletion
- double-click or context-menu `Open file` opens one file
- the opened file, not the merely selected file, drives the signal list, plot, table, and workspace title
- when several files are selected, the file-list context menu only exposes batch deletion

This split exists so users can batch-select files without accidentally changing
the signal context shown in the main editors.

## Large Signal Notes

Large signals affect workspace behavior in two places:

- the plot uses min/max LOD rendering to keep zoomed-out views responsive
- the table materializes a bounded preview of rows instead of every sample

Because of the table preview, table insertion is intentionally local to the
current selection. Inserting at the real end of a huge signal could succeed but
appear invisible because the end is outside the materialized preview.
