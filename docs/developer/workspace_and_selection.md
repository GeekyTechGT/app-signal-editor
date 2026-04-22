# Workspace and Selection Model

## Purpose

This document explains how document state, visible signals, and the active
signal are managed in the desktop application.

## Why This Needs Its Own Document

Several regressions in the project have come from confusing three related but
different concepts:

- a row selected in the Qt list widget
- a signal visible in the plot/table
- the active signal used for editing

They often move together, but they are not the same thing.

## Source of Truth

The source of truth for workspace state is `MainWindow::LoadedDocument`.

Important fields:

- `library`
- `visible_signal_indices`
- `active_signal_index`
- `undo_stack`
- `dirty`

This means widgets are not authoritative. They are views over that document
state.

## File-Level Model

Each loaded input file gets one `LoadedDocument`. That document keeps its own:

- waveform library
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
- visible signals
- active signal
- list widget state
- plot and table bindings

If a bug appears only after reload, inspect `MainWindow::onFileReloadRequested`
first.
