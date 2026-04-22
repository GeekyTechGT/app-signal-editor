# Workbook and XLSX Notes

## Purpose

This document explains how workbook-oriented formats are represented in Signal
Editor and what contributors need to preserve when changing XLSX or
worksheet-aware workflows.

It complements:

- [Filesystem and Persistence](filesystem_and_persistence.md)
- [Workspace and Selection Model](workspace_and_selection.md)
- [Architecture Overview](../architecture/architecture_overview.md)

## Supported Workbook Formats

The current implementation treats these formats as workbook-aware:

- SpreadsheetML XML (`.xml`)
- XLSX (`.xlsx`)

These formats may contain multiple worksheets. In the UI, one loaded file may
therefore expose multiple sheet-local signal sets even though editing is always
performed against one active `SignalLibrary` at a time.

The following formats remain single-sheet:

- CSV
- TSV/TXT
- JSON

That distinction matters in both the UI and the persistence layer. Do not add
sheet-related UI affordances to single-sheet formats unless the format contract
really changes.

## Main Types Involved

### Filesystem layer

- `SignalFileRepository`
  Dispatches by extension to the correct concrete adapter.
- `SpreadsheetXmlSignalRepository`
  Loads and saves workbook-style XML documents.
- `XlsxSignalRepository`
  Loads and saves native `.xlsx` workbooks.
- `WorkbookModel`
  Carries workbook-level persistence payloads between adapters and the rest of
  the application.

### Qt layer

- `MainWindow`
  Owns loaded documents, active file, active sheet, visible signals, and active
  signal.
- file options dialog flow in `MainWindow`
  Lets the user inspect workbook sheets and switch the active sheet when the
  format supports it.

## Mental Model

Think in terms of three levels of state:

1. File
   One workspace document loaded from disk.
2. Sheet
   One worksheet-local `SignalLibrary` inside that file.
3. Signal
   One waveform inside the active sheet.

The service layer still edits one active `SignalLibrary`. `MainWindow` is the
layer that decides which sheet-local library is currently bound into that
service context.

## Loading Flow

For workbook formats the loading flow is:

1. The concrete repository reads the workbook container.
2. Each relevant worksheet is converted into one sheet-local `SignalLibrary`.
3. `MainWindow` stores those libraries inside one `LoadedDocument`.
4. One `active_sheet_index` determines which sheet is currently bound to:
   - the signal list
   - the plot
   - the table
   - the interpolation controls

This is why a bug in sheet switching usually belongs to `MainWindow`
coordination rather than to the domain layer.

## Sheet Switching Rules

When the user changes sheet in file options:

- the currently active sheet may need to be synchronized back into the document
  snapshot first
- only one synchronization pass should happen before rebinding
- the target sheet must not be overwritten by stale state from the previously
  active sheet
- signal list, plot, table, visible signals, and active signal must all be
  rebound against the new sheet

If a contributor sees a symptom where switching to `Sheet B` still shows the
signals of `Sheet A`, inspect the `activate_document(...)` and
sheet-application flow first.

## XLSX Data Layout

Saved XLSX files intentionally separate user-facing signal data from
application-specific enum metadata.

### Data worksheets

Each data worksheet is saved as a plain tabular sheet:

- first row is the header row
- first header cell is `time`
- remaining header cells are signal names
- following rows are numeric sample values aligned on the exported time axis

No enum labels are embedded in the data sheet cells.

### `METADATA` worksheet

Enumerated mappings are stored in a dedicated worksheet named `METADATA`.

Its columns are:

- `sheet`
- `signal_name`
- `enum_label`
- `enum_value`

This design keeps the user-facing data sheets Excel-friendly while still
preserving enum semantics during round-trip persistence.

## Why Metadata Is Scoped by Sheet and Signal Name

The current implementation supports this real-world case:

- worksheet `INPUT_1` contains signal `mode`
- worksheet `INPUT_2` also contains signal `mode`
- both are enumerated
- their numeric mappings differ

The metadata key is therefore effectively:

- worksheet name
- signal name

This is sufficient for the current persistence contract and avoids storing
extra technical columns in the metadata worksheet.

## Important Constraints

Contributors should preserve these constraints:

- CSV must stay single-sheet
- workbook sheet controls must only appear for workbook-capable formats
- saved XLSX data sheets must remain plain tables
- `METADATA` must remain small, readable, and stable unless the persistence
  contract is intentionally revised
- reload and sheet switching must clear and rebind UI state rather than trying
  to patch widgets incrementally from stale pointers

## Debugging Tips

When workbook behavior breaks, separate the problem by layer:

- If Excel cannot open the saved file, inspect the XLSX writer and ZIP/XML
  packaging first.
- If the wrong sheet is shown after switching, inspect `MainWindow` sheet
  synchronization and rebinding.
- If enum labels disappear after save/load, inspect `METADATA` generation and
  re-application.
- If the UI shows sheet controls for CSV, inspect the file-options branching
  logic rather than the repositories.
