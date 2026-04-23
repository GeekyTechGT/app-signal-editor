# Signal Editor User Manual

## Document Purpose

This manual is intended for end users of Signal Editor.

It explains what the application does, how the main workflows work, what the
different panels mean, and how to complete common tasks without having to infer
the behavior from the interface alone.

The document is written to be suitable for later export to PDF and inclusion in
the GUI help system.

## Intended Audience

This manual is written for:

- controls engineers
- test engineers
- validation engineers
- calibration engineers
- technical users who need to inspect, create, edit, and save waveform data

It assumes the reader is comfortable with engineering data and spreadsheets,
but it does not assume knowledge of the internal codebase.

## How To Read This Manual

If you are new to the tool, read these sections first:

1. [What Signal Editor Is](#what-signal-editor-is)
2. [Main Window Overview](#main-window-overview)
3. [Opening Files](#opening-files)
4. [Working With Signals](#working-with-signals)
5. [Editing In The Plot](#editing-in-the-plot)
6. [Editing In The Table](#editing-in-the-table)
7. [Saving And Exporting](#saving-and-exporting)

If you already know the basics, go directly to the relevant workflow section.

## What Signal Editor Is

Signal Editor is a desktop application for working with time-based signals.

It is designed for workflows where you need to:

- open one or more signal files
- inspect the signals visually
- inspect the same data in tabular form
- edit one active signal precisely
- compare several signals at the same time
- work with numeric and enumerated signals
- preserve workbook structure when the file format supports worksheets
- save the result back to disk in a predictable format

Typical examples:

- correcting a numeric reference trace
- cleaning a measured signal
- aligning or shifting samples
- changing an enumerated state mapping
- creating a synthetic signal from scratch
- comparing several signals on one plot before saving the result

## What Signal Editor Is Not

Signal Editor is not intended to replace:

- a full spreadsheet suite
- a full calibration suite
- a signal-processing laboratory environment
- a collaborative cloud editor
- a scripting notebook

Its focus is practical signal editing with a clean workflow.

## Supported File Formats

Signal Editor currently supports these file formats:

- CSV
- TSV/TXT
- JSON
- SpreadsheetML XML
- XLSX

Important format behavior:

- CSV, TSV/TXT, and JSON are treated as single-sheet formats
- SpreadsheetML XML and XLSX can contain multiple worksheets
- XLSX worksheets used for signal data are saved as plain tabular sheets
- XLSX enum mappings are stored in a dedicated worksheet named `METADATA`

## Main Window Overview

The main window is organized around a workspace model.

At a high level, you work with:

- a file list
- a signal list
- a central editor area
- a plot tab
- a table tab
- shared controls such as interpolation
- status information

The important idea is that the application always revolves around the currently
opened file and the currently active signal.

Even when the interface shows several files and several visible signals at the
same time, editing is still driven by a clear context:

- one opened file
- one active sheet when the format supports worksheets
- one active signal
- zero or more additional visible signals for comparison

### Main Workspace Areas

Typical visible areas are:

- file list on one side
- signal list on one side
- plot/table editor area in the center
- toolbar and actions at the top
- status information at the bottom

Depending on your current task, the most important areas are:

- the file list when you manage multiple loaded documents
- the signal list when you choose what is visible and what is active
- the plot when you work visually
- the table when you work numerically
- the toolbar when you control navigation, saving, or export

> Screenshot placeholder: full main window after opening a sample file.
> Capture the entire application window with file list, signal list, toolbar,
> plot tab, and status bar visible.

## Main Concepts

Before using the tool, it is important to understand a few concepts clearly.

### Selected File Versus Opened File

The file list supports selecting files and opening a file.

These are not the same action.

- A selected file is only highlighted in the list.
- An opened file is the file currently shown in the signal list, plot, and
  table.

This distinction exists so you can:

- select multiple files for batch deletion
- browse the file list without accidentally changing the editing context
- keep the currently shown signals stable until you explicitly open another file

If you are unsure why the plot did not change, this distinction is usually the
reason. A selected file is not automatically the opened file.

### Active Sheet

For workbook-style formats such as SpreadsheetML XML and XLSX, one file may
contain multiple worksheets.

Only one worksheet is active at a time.

The active sheet determines:

- which signals appear in the signal list
- which signals can be shown in the plot
- which signals are shown in the table
- which sheet will receive your edits

### Visible Signals Versus Active Signal

A signal can be:

- visible in the plot and table
- active for editing
- both visible and active
- neither visible nor active

The active signal is the one currently edited by plot operations and most
signal-specific actions.

Visible signals are the ones currently displayed for comparison.

This means:

- several signals can be visible
- only one signal is active at a time
- if no signal is visible, there may be no active signal

### Numeric Signal Versus Enumerated Signal

A numeric signal is interpreted as numeric values over time.

An enumerated signal is stored numerically, but the numeric values represent
states such as:

- `OFF`
- `ON`
- `IDLE`
- `RUN`
- `FAULT`

The application can display those state labels instead of only raw numeric
codes.

## Opening Files

Files can be opened in multiple ways.

### Open From Menu

Use the standard file opening action from the GUI.

Use this workflow when:

- you want to browse the filesystem
- you want to add one or more files deliberately
- you are starting a new editing session

This is the most controlled workflow and is recommended when:

- you need to choose among several similar files
- you want to verify the directory before opening
- you are loading workbook files and want to be deliberate about the source

After loading with the file browser:

- the file is added to the workspace
- the file appears in the file list
- the workspace retains the file until you remove it

When a file is large, Signal Editor shows a loading progress window. This
window does not block normal main-window operations such as resize, minimize, or
restore. You can continue managing the application window while the import runs
in the background.

### Drag And Drop

You can drag supported files into the application window.

Use this workflow when:

- the files are already visible in your file explorer
- you want to populate the workspace quickly

This is usually the fastest workflow when:

- you already know which files you need
- you want to compare several files in one session
- you are doing ad hoc investigation

> Screenshot placeholder: drag-and-drop of one or more files into the main
> window.
> Capture the application while files are being dragged over the drop target.

### What Happens After Loading

After loading, the file is added to the workspace.

If multiple files are loaded, the workspace can keep them all at once.

For workbook formats:

- the file is loaded as a workbook-aware document
- one sheet becomes active
- the signal list is populated from that active sheet only

If loading fails, the error dialog includes the file, path, original parser
problem, and guidance for common issues. For XLSX workbooks, sheet-specific
validation errors include the sheet name when available.

After loading multiple files, remember that:

- all files can remain in memory together
- only one file is opened into the editors at a time
- the file list is the place where you control which file becomes the opened
  one

## Working With The File List

The file list is not only a navigation tool. It also controls file-level
operations.

Think of the file list as the document manager for the current workspace.

It answers these questions:

- which files are loaded?
- which files are selected?
- which file is currently opened?
- which file actions can I apply right now?

### Single Click

A single left click selects a file row.

This does not automatically open the file into the editors.

This allows you to move through the file list without changing the signal,
plot, and table context every time you touch a row.

### Double Click

A double click opens the file.

That file then becomes the opened file used by:

- signal list
- plot
- table
- context status

If you want the central editor content to change, double click is one of the
main ways to do it.

### Ctrl+Click

`Ctrl+click` extends file selection.

Use this when you want to:

- select several files
- remove several files from the workspace at once

Typical multi-selection workflow:

1. click the first file
2. hold `Ctrl`
3. click additional files
4. use the context menu to remove the selected group

### Context Menu

When a single file is selected, the context menu can show actions such as:

- `Open file`
- `Reload from disk`
- `Rename`
- `Options`
- `Delete file`

This menu is the place for explicit file-scoped actions that should not happen
accidentally during normal selection.

#### `Open file`

This command opens the selected file into the central editors.

Use it when:

- you prefer right-click workflows
- you want to be explicit about opening
- you do not want to rely on double click

#### `Reload from disk`

This command discards the current in-memory content of that file and reloads it
from disk.

Use it when:

- you made changes by mistake
- you want to restore removed signals
- the file changed externally and you want the latest on-disk content

After reload:

- the signal list is refreshed
- the plot is refreshed
- the table is refreshed

#### `Rename`

This command renames the file entry in the workspace context.

Use it when you want the workspace naming to be clearer while you are working
with multiple files.

#### `Options`

This command opens the file options dialog.

Use it to:

- inspect file-level information
- inspect workbook sheets
- change the active sheet for XML/XLSX workbooks

#### `Delete file`

This removes the file from the workspace.

In normal workspace usage, this is a workspace-management action, not a
filesystem delete operation.

When several files are selected, the context menu is intentionally reduced to
batch deletion.

In the multi-selection case, the simplified context menu is intentional. It
avoids mixing single-file operations with batch operations.

> Screenshot placeholder: file list context menu with one file selected.
> Capture the context menu showing open, reload, rename, options, and delete.

> Screenshot placeholder: file list context menu with multiple files selected.
> Capture the context menu showing only the batch delete action.

## File Options

The file options dialog is used for file-level information and workbook-specific
controls.

It is the right place when your question is about the file as a whole, not
about one particular signal.

### For Single-Sheet Formats

For formats such as CSV, TSV/TXT, and JSON:

- file options show information relevant to the file
- there is no active-sheet combo box because the format has only one sheet

This is intentional. CSV and other single-sheet formats do not have a valid
multi-sheet concept, so the dialog avoids showing misleading controls.

### For Workbook Formats

For SpreadsheetML XML and XLSX:

- the dialog can show the available sheets
- the dialog allows changing the active sheet
- the dialog can show file or sheet-related statistics

Typical information that may be useful in this dialog includes:

- sheet names
- current active sheet
- visible columns in the active sheet
- descriptive information about the selected sheet

Changing the active sheet updates:

- the signal list
- the plot
- the table
- the editing context

After you apply the change, you should expect the entire signal context to
follow the selected sheet.

> Screenshot placeholder: file options dialog for an XLSX workbook.
> Capture the dialog showing the list of sheets, the current active sheet, and
> sheet-related information.

## Working With The Signal List

The signal list is where you control which signals are shown and which one is
active.

This panel is central to the multi-plot workflow.

Use it to answer:

- which signals do I want visible right now?
- which signal should be the active editing target?

### Signal Checkbox

Each signal has a checkbox.

When a signal is checked:

- it is shown in the plot
- it is shown in the table as one value column

When a signal is unchecked:

- it is removed from the plot
- its value column is removed from the table

This means the checkbox controls display membership, not only visual
highlighting.

### Clicking A Signal Row

When you click a signal row:

- the signal becomes checked if it was not already checked
- the signal becomes the active signal

This is designed to reduce friction in the normal workflow.

In practice, clicking a row is the fastest way to say:

- show this signal
- make this the editing target

### Active Signal Highlight

The active signal is visually emphasized in the list.

You should always be able to understand at a glance:

- which signals are visible
- which one is active

If the active signal changes, the plot editing context changes with it.

The active signal matters for:

- plot editing gestures
- signal options
- most signal-specific editing operations

> Screenshot placeholder: signal list with multiple visible signals and one
> active signal highlighted.
> Capture the list with checkboxes, the active signal marker, and several
> visible signals.

### Important Edge Cases

If you uncheck the active signal:

- the application removes it from visibility
- if another visible signal exists, one of them becomes active
- if no visible signal remains, the active signal becomes undefined

In that last case:

- the plot may become empty
- the table may no longer show signal value columns
- signal-specific editing actions may be disabled

This is expected behavior, not a fault condition. It simply means that there is
no visible signal available to edit.

## Plot View

The plot is the main visual editing surface.

It is intended for:

- understanding waveform shape quickly
- comparing signals
- moving samples visually
- inserting samples
- removing samples
- applying brush-like changes to numeric signals

The plot is usually the best starting point when you first inspect a file,
because it gives shape, trend, and timing context immediately.

### What The Plot Shows

The plot can show:

- one active signal
- additional visible signals
- samples as points
- connecting lines between samples

The active signal is emphasized visually with:

- stronger color
- stronger line
- stronger point emphasis

Non-active visible signals remain fully visible for comparison.

This is important because the tool does not treat non-active signals as faint
background references only. They are still full participants in the multi-plot
view, just less emphasized than the active one.

### Y-Axis Behavior

When several signals are visible, the Y axis is scaled against the visible
signals.

This means:

- the plot does not only consider the active signal
- adding or removing visible signals can change the Y range

### Zoom And Navigation

The plot supports navigation tools such as:

- zoom in
- zoom out
- fit view
- pan mode
- rectangle zoom mode

Depending on your workflow, these controls can be used together:

- `Fit view` to regain context
- rectangle zoom to isolate a region
- pan mode to move across the same zoom level
- zoom in/out to refine the view step by step

The application preserves the current time zoom when possible, including during
common list operations such as:

- checking signals
- unchecking signals
- changing the active signal

This helps maintain editing focus while you compare signals.

Zoom preservation is especially useful when:

- you are checking and unchecking comparison signals
- you change the active signal but still want to stay in the same time region

> Screenshot placeholder: plot with multiple visible signals, one active
> signal, and zoom applied.
> Capture a meaningful zoomed region with at least three visible signals.

### Editing In The Plot

Common plot editing operations include:

- dragging a sample point
- inserting a new point
- removing a point
- applying Gaussian brushing to numeric signals

The exact available interactions depend on the signal type and current mode.

The practical mental model is:

- use the signal list to decide what is active
- use the plot to identify or manipulate the region of interest
- use the table when exact numeric follow-up is needed

### Mouse Actions Summary

Typical mouse-driven behaviors in the plot include:

- move the mouse to inspect and hover
- click a sample to target it
- drag a sample to move it
- use the current interaction mode to pan or rectangle-zoom
- use the current editing gesture to insert, remove, or reshape points

For step-interpolated signals, the tool avoids misleading hover interaction on
the imaginary straight segment between two states.

### Step Interpolation Behavior

When a signal uses `step` interpolation:

- the signal is interpreted as state-like or piecewise constant behavior
- the tool avoids misleading hover affordances that would suggest a linear
  connecting segment where that is not semantically correct

This is especially important for enumerated signals.

### Exporting A Plot Image

The application can export the plot content as an image.

Use this when:

- you need a screenshot-quality artifact for a report
- you want to share a view without sharing the source file
- you need a quick visual snapshot for review

Typical output formats include:

- PNG
- JPG/JPEG
- BMP

The saved image represents the current visual state of the plot, including the
current zoom and the currently visible signals.

> Screenshot placeholder: plot image export action and save dialog.
> Capture the export command in the GUI and the file save dialog.

## Table View

The table is the precise editing surface.

It is intended for:

- exact numeric changes
- exact time changes
- verifying values row by row
- inserting and deleting samples
- comparing multiple visible signals with aligned timestamps

The table is the place to use when exact values matter more than visual shape.

### Table Layout

The table shows:

- one time column
- one value column per visible signal

This means that the table grows or shrinks as signal visibility changes.

If multiple signals are visible, the table lets you compare them directly on
aligned rows.

This is one of the most important synchronizations in the application:

- plot visibility decides which value columns exist
- the active signal still remains the main editable focus
- the other visible columns give context and comparison

> Screenshot placeholder: table view with one time column and multiple signal
> value columns.
> Capture a case where several visible signals are shown side by side.

### Table Search

The table view includes a text search field.

Use it to find:

- a specific timestamp
- a specific numeric value
- a specific enum label, where visible in the table context

This is useful when:

- the table is long
- you want to jump to a known region quickly
- you want to verify a pattern or threshold crossing

The search field is particularly helpful on large datasets where scrolling alone
would be too slow.

> Screenshot placeholder: table search field filtering the table.
> Capture the search field filled in and a filtered table below it.

### Editing Samples

Typical table operations include:

- editing a time value
- editing a signal value
- inserting a sample
- removing a sample

`+ Sample` inserts near the selected row. This matters for very large signals:
the table may show only a bounded preview of rows to keep the interface
responsive, so inserting at the real end of the signal could succeed but appear
invisible. Inserting near the selection makes the new sample immediately visible
and editable.

When you need exact edits, the recommended workflow is often:

1. identify the region in the plot
2. switch to the table
3. perform the exact correction numerically

The active signal remains the main editable context.

Visible but non-active signals are typically there for comparison rather than
for direct simultaneous editing.

## Interpolation

Interpolation controls how values are interpreted between samples.

### Common Modes

Signal Editor supports:

- `linear`
- `step`

### Numeric Signals

Numeric signals can typically use either:

- `linear`
- `step`

Choose the mode that best represents the intended physical or logical behavior.

### Enumerated Signals

Enumerated signals are always treated as `step`.

This is intentional.

A state signal should not appear to ramp linearly from one state to another.

### Shared Interpolation Control

Interpolation is controlled from a shared workspace-level control.

When several signals are visible:

- changing interpolation applies to the visible plotted signals when that change
  is valid

This avoids duplicating the same control in multiple places.

Because the interpolation selector is shared, it should be thought of as a
workspace control for the currently visible plotted set, not as a control owned
only by the table.

> Screenshot placeholder: interpolation combo box in the main workspace.
> Capture the interpolation control and a plot where its effect is easy to see.

## Enumerated Signals

Enumerated signals require a slightly different mental model from numeric
signals.

### How They Are Imported

If a file contains textual states and no explicit numeric mapping is provided:

- the first distinct label encountered becomes value `0`
- the next distinct label becomes value `1`
- and so on

This allows the application to import common engineering tables that use string
states directly.

### How They Are Displayed

Enumerated signals are displayed using labels wherever possible.

This improves readability in:

- the plot
- the table
- signal options

### Editing Enum Mappings

The signal options dialog allows editing the enum mapping.

After applying the new mapping:

- the plot is refreshed
- the table is refreshed
- the signal remains semantically meaningful

This is useful when:

- the imported numeric mapping is not the one you want
- you need to align the file with an external convention
- you need more human-friendly state definitions

> Screenshot placeholder: signal options dialog for an enumerated signal.
> Capture the mapping editor with several labels and values.

## Signal Options

The `Options` action behaves differently depending on the signal type.

This is intentional. The dialog adapts to the nature of the signal instead of
forcing one generic and less useful layout.

### For Enumerated Signals

You can inspect and edit the label/value mapping.

This is the right place to use when the numeric codes are technically valid but
the state naming or numeric assignments need to be aligned to another tool or
team convention.

### For Numeric Signals

The dialog still provides useful information, such as:

- minimum value
- maximum value
- average value
- first and last sample
- time range
- duration
- sampling step statistics
- interpolation mode
- sample count

This makes the dialog useful even when no enum mapping exists.

For numeric signals, think of `Options` as a quick signal summary sheet.

> Screenshot placeholder: signal options dialog for a numeric signal.
> Capture the information view with min, max, duration, sample count, and
> interpolation details.

## Creating A New Signal

Signal Editor can create new signals from templates.

This feature is useful both for creating entirely new signals and for restoring
missing engineering inputs inside an existing file.

Typical templates include:

- constant
- sine
- cosine
- pulse
- sawtooth
- triangle
- ramp
- enumerated pattern

### Default Time Parameters

If the current document was loaded from disk, the tool tries to use meaningful
time defaults based on the opened document.

That means the default:

- start time
- end time
- sampling time

can align with the currently loaded signal set.

If no file context exists, the tool uses fallback defaults.

This makes signal creation faster in realistic editing sessions.

If you are building a signal meant to align with existing data, these defaults
reduce the amount of manual setup required.

> Screenshot placeholder: new signal dialog showing waveform template options
> and time parameter defaults.
> Capture the dialog with a realistic set of pre-filled time values.

## Multi-Sheet Workbook Workflows

Workbook workflows apply to:

- SpreadsheetML XML
- XLSX

### Typical Workflow

1. Open the workbook file.
2. Open file options.
3. Select the desired sheet.
4. Apply the sheet change.
5. Work only on the signals of that sheet.
6. Save the workbook.

Remember that at any given moment only one sheet is active in the central
editors, even though the workbook contains several sheets overall.

### What Gets Saved

When you save a workbook-aware file:

- the workbook structure is preserved
- sheet-local signal content is preserved
- your changes apply to the relevant sheet
- XLSX enum metadata is stored in the dedicated `METADATA` sheet

### What To Expect In Excel

For saved XLSX files:

- data worksheets remain plain tabular sheets
- headers remain simple and Excel-friendly
- enum mapping metadata is stored in `METADATA`

This design keeps the engineering data readable in Excel while still allowing
Signal Editor to restore enum mappings later.

> Screenshot placeholder: workbook open in Excel showing a normal data sheet
> and the separate `METADATA` worksheet.
> Capture one user-facing data sheet and the metadata sheet tab.

## Reloading A File From Disk

Use `Reload from disk` when you want to discard current in-memory changes for
one file and restore the on-disk content.

This is useful if:

- you removed a signal by mistake
- you changed values and want to go back to the file state
- the file on disk was updated externally and you want to refresh it

When reload happens, the application refreshes the editing context so that:

- the signal list reflects the file again
- the plot reflects the file again
- the table reflects the file again

This should be understood as a full refresh of that document.

> Screenshot placeholder: file context menu showing `Reload from disk`, then
> the refreshed workspace.
> Capture both the command entry and the resulting restored signal list.

## Undo

Undo is document-local.

This means:

- undo affects the currently opened document
- other loaded documents keep their own independent in-memory state

Undo is useful for:

- reverting recent signal edits
- backing out accidental point changes
- undoing unwanted table edits

If your goal is to restore the file exactly as it exists on disk, prefer
`Reload from disk`.

## Saving And Exporting

Saving writes the current document back to disk in the selected target format.

There are two conceptually different output workflows:

- saving the signal data file itself
- exporting a visual image of the current plot

### Save Behavior

Saving applies to the active opened document.

The result depends on the selected output format:

- single-sheet formats remain single-sheet
- workbook formats preserve workbook structure
- supported metadata is preserved where the format supports it

Before saving, make sure you are working on the correct:

- opened file
- active sheet, if applicable
- active signal and visible-signal set

### CSV And Text-Like Outputs

These are appropriate when:

- you need a simple tabular exchange format
- you do not need multi-sheet workbook structure

### XLSX Outputs

Use XLSX when:

- you need workbook structure
- you need multiple sheets
- you want Excel-friendly output
- you want enum metadata preserved cleanly

For XLSX, remember:

- data sheets remain ordinary tables
- enum metadata is stored separately in `METADATA`
- this is normal and is part of correct round-trip behavior

> Screenshot placeholder: save or save-as dialog targeting an XLSX file.
> Capture the format selection and a workbook-oriented save workflow.

## Recommended Daily Workflow

For a typical editing session:

1. Open one or more files.
2. Open the specific file you want to work on.
3. If it is a workbook, select the correct sheet in file options.
4. Check the signals you want visible.
5. Click the signal you want active.
6. Use the plot for visual changes.
7. Use the table for precise corrections.
8. Adjust interpolation if needed.
9. Use signal options if you need statistics or enum mapping changes.
10. Save the result in the target format.

For best results:

- use the plot first for orientation
- use the table for exact final corrections
- use file options only for file-level or sheet-level concerns
- use signal options only for signal-level concerns

## Troubleshooting

### I Selected A File But The Plot Did Not Change

This is expected if you only selected the file.

To actually change the editing context:

- double click the file
- or use `Open file` from the context menu

### I See Several Signals But Only One Seems Editable

This is expected.

Several signals may be visible for comparison, but only one signal is active for
editing at a time.

This is the intended multi-plot model.

### I Unchecked A Signal And It Disappeared From The Plot

This is expected.

Unchecked signals are removed from:

- plot visibility
- table value columns

The table and plot are synchronized in this behavior.

### The Plot Is Empty

Typical reasons:

- no file is currently opened
- no signal is checked
- the current document has no visible signals

### My Workbook Has Multiple Sheets But I Only See One Set Of Signals

This is normal.

Only the active sheet is shown at a time.

Use file options to switch the active sheet.

### Excel Shows A `METADATA` Sheet

This is expected for XLSX files saved by Signal Editor when enum mappings exist
or workbook metadata needs to be preserved.

The data sheets remain the main user-facing engineering sheets.

### I Want To Restore The Original File Content

Use:

- `Reload from disk` to restore the current file from disk
- `Undo` if you only want to reverse recent in-session edits

### My Enumerated Signal Values Look Different After Import

If the source file does not define an explicit mapping, Signal Editor infers the
mapping by first label appearance.

If needed:

- open the signal options
- inspect the mapping
- edit it
- apply the change

### Loading Failed Because The Time Column Is Not Strictly Increasing

This means one timestamp is not greater than the timestamp above it.

Common causes:

- two rows have the same `time`
- one row is out of order
- a copied row duplicated the previous timestamp

The error dialog reports the file path and, for XLSX workbooks, the sheet name
when available. Open the file in Excel or another spreadsheet editor, compare
the reported row with the previous data row, fix the timestamp, save, and load
the file again.

### Loading A Large File Shows A Progress Window

This is expected. Large files are loaded on a worker thread so the main window
can keep responding. The progress window reports the file currently being
loaded and can be cancelled before the next file starts.

## Frequently Asked Questions

### Can I Show Multiple Signals At The Same Time?

Yes.

Check multiple signals in the signal list.

### Can I Edit Multiple Signals At The Same Time?

You can view multiple signals at the same time, but one active signal is the
main edit target.

The other visible signals remain useful as references for comparison.

### Why Does The Plot Scale Change When I Check Another Signal?

Because the Y axis is recalculated from the visible signals.

### Can I Search Inside The Table?

Yes.

Use the table search field.

### Can I Save A Screenshot Of The Plot?

Yes.

Use the plot image export feature.

### Can CSV Contain Multiple Sheets?

No.

CSV is a single-sheet format.

### Which Formats Support Multiple Sheets?

SpreadsheetML XML and XLSX.

### Why Is The Active-Sheet Selector Not Shown For CSV?

Because CSV has only one sheet by definition.

## Screenshot Checklist For Final PDF

The following screenshots are recommended for the final illustrated version:

1. Full main workspace after opening a typical file
2. Drag-and-drop file opening
3. File context menu for a single selected file
4. File context menu for multiple selected files
5. File options dialog for XLSX with sheet selection
6. Signal list with checkboxes and active signal highlight
7. Plot with multiple visible signals and zoom applied
8. Plot export image workflow
9. Table with multiple signal columns
10. Table search field in use
11. Interpolation control in the workspace
12. Enum signal options dialog
13. Numeric signal information dialog
14. New signal creation dialog
15. Saved workbook visible in Excel with `METADATA`
16. Reload-from-disk workflow
17. Save-as dialog for XLSX

## Final Notes

Signal Editor is designed to make engineering signal editing predictable.

If you keep these four questions clear while working, most workflows become
intuitive:

- which file is selected
- which file is opened
- which sheet is active
- which signal is active

Everything else in the application is built around those editing contexts.
