# Software Requirements Specification — Signal Editor

**Version:** 1.0.0-beta
**Date:** 2026-04-22
**Status:** Working draft aligned with the current implementation
**Audience:** Engineering, QA, Product, Reviewers

## 1. Introduction

### 1.1 Purpose

This document defines the functional and non-functional behavior expected from
Signal Editor, a C++23 desktop waveform editing application with a Qt 6 GUI
and a hexagonal internal architecture.

The document is intended to stay close to the implemented product rather than
describe a speculative future scope.

### 1.2 Scope

The current implementation scope includes:

- loading one or more supported signal files into a shared workspace
- keeping multiple files loaded at the same time without forcing immediate
  context switches
- distinguishing explicitly between selected files in the file list and the
  file currently opened into the editors
- supporting workbook-style documents with multiple worksheets for
  SpreadsheetML XML and XLSX files
- letting the user select the active worksheet for workbook formats
- showing the signals of the opened worksheet in a signal list with
  visibility checkboxes
- keeping a document-local active signal for editing
- editing the active signal in a dedicated plot tab or table tab
- showing multiple visible signals in the plot and in the table while keeping
  one active editable signal
- preserving plot zoom across signal visibility and active-signal changes
- controlling interpolation from a shared workspace-level selector
- creating new numeric and enumerated signals from templates
- saving edited documents in supported formats while preserving supported
  metadata
- reloading one file from disk without discarding the rest of the workspace
- undoing modifications inside the active document context
- restoring and saving UI settings in a version-scoped persistence store

The following are out of scope for the current product line:

- collaborative or network-backed editing
- legacy binary Excel `.xls` parsing or export
- spectrum analysis or a full signal-processing suite
- notebook-style scripting environments
- persistent multi-document project/session files beyond currently loaded data
  files

## 2. Product Description

Signal Editor is a local desktop engineering tool. It combines direct waveform
manipulation with exact tabular editing while keeping the domain model
independent from the GUI framework.

### 2.1 User Classes

- **Controls engineer**: adjusts reference and validation traces quickly
- **Test engineer**: reshapes measured data before replay or reporting
- **Tooling engineer**: extends core behavior or adapters while preserving
  architecture
- **Calibration or validation engineer**: works with enumerated and boolean-like
  state channels

### 2.2 Operating Environment

- Windows 10/11 with Qt 6 and MinGW64 for GUI delivery and full application
  usage
- Linux with GCC for core library and automated test workflows
- local filesystem access to supported input and output files
- version-scoped local settings storage through Qt-backed application state

## 3. Functional Requirements

| ID | Requirement |
|----|-------------|
| FR-1 | The system shall load CSV, TSV/TXT, JSON, SpreadsheetML XML, and XLSX files into the shared signal model. |
| FR-2 | The system shall support both menu-based and drag-and-drop file opening. |
| FR-3 | The system shall maintain a workspace that can hold multiple loaded documents simultaneously. |
| FR-4 | The system shall allow file selection in the file list without implicitly opening the selected file into the editors. |
| FR-5 | The system shall allow opening a file explicitly by double-click or by the file-list context menu. |
| FR-6 | The system shall support multi-selection of files in the file list for batch removal. |
| FR-7 | The system shall display the signals of the opened document and active worksheet in a dedicated signal list panel. |
| FR-8 | The system shall allow signal visibility to be controlled through a checkbox per signal. |
| FR-9 | The system shall automatically make a clicked signal both visible and active for editing. |
| FR-10 | The system shall maintain an active signal distinct from signal visibility and distinct from transient widget selection. |
| FR-11 | The system shall render all visible signals in the plot while keeping one active editable signal. |
| FR-12 | The system shall rescale the plot Y axis against the visible plotted signals rather than against only the active signal. |
| FR-13 | The system shall preserve the current plot time zoom when the user changes signal visibility or the active signal, provided a plotted signal remains available. |
| FR-14 | The system shall render the opened document in a dedicated table tab. |
| FR-15 | The system shall render one value column per visible signal in the table in addition to the time column. |
| FR-16 | The system shall provide a text search field for filtering table rows by visible content. |
| FR-17 | The system shall allow dragging existing waypoints on the plot. |
| FR-18 | The system shall allow inserting and removing waypoints through the plot interaction flow where the signal type permits it. |
| FR-19 | The system shall allow Gaussian brushing for numeric signals when the proper plot gesture is used. |
| FR-20 | The system shall suppress hover segment affordances for step-interpolated signals where a straight-line segment would be misleading. |
| FR-21 | The system shall allow sample editing, insertion, and removal in tabular form. |
| FR-22 | The system shall provide a shared interpolation selector that remains available in both plot and table workflows. |
| FR-23 | The system shall apply interpolation changes to all visible plotted signals for which interpolation changes are valid. |
| FR-24 | The system shall allow creating signals from constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated templates. |
| FR-25 | When the user creates a new signal in a document loaded from disk, the default time start, time end, and sampling time shall be derived from the opened document context when meaningful. |
| FR-26 | The system shall allow renaming, removing, and inspecting signals in the opened document. |
| FR-27 | The system shall provide an options dialog for enumerated signals where the label/value mapping can be edited. |
| FR-28 | The system shall provide an information dialog for non-enumerated signals showing useful summary statistics. |
| FR-29 | The system shall support `linear` and `step` interpolation modes for non-enumerated signals. |
| FR-30 | The system shall force enumerated signals to behave as `step` signals. |
| FR-31 | The system shall infer enumerated mappings from textual values by first appearance when importing supported tabular sources that do not provide an explicit mapping. |
| FR-32 | The system shall refresh plot and table views after an enumerated mapping is changed by the user. |
| FR-33 | The system shall render enumerated signal values as labels in the plot and table when a mapping exists. |
| FR-34 | The system shall provide file-level options that show relevant file information, including workbook sheet information for workbook formats. |
| FR-35 | The system shall expose active-sheet selection in file options only for workbook-capable formats. |
| FR-36 | The system shall let the user select the active worksheet for SpreadsheetML XML and XLSX documents. |
| FR-37 | The system shall reload one file from disk on demand without discarding the rest of the workspace. |
| FR-38 | Reloading a file shall clear and rebind the signal list, plot, and table so the UI reflects on-disk content rather than stale in-memory views. |
| FR-39 | The system shall preserve one undo history per opened document. |
| FR-40 | The system shall support document-local undo for edits performed in the current session. |
| FR-41 | The system shall save the active document to the chosen supported format while preserving supported metadata. |
| FR-42 | For workbook formats, the system shall preserve the workbook structure when saving, including the set of worksheets required by the active document. |
| FR-43 | XLSX data worksheets shall be saved as plain tabular sheets whose first row contains the time header plus signal headers. |
| FR-44 | XLSX enumerated mappings shall be saved in a dedicated workbook worksheet named `METADATA`. |
| FR-45 | The XLSX `METADATA` worksheet shall store only `sheet`, `signal_name`, `enum_label`, and `enum_value` columns. |
| FR-46 | The system shall treat XLSX mappings as scoped by worksheet name and signal name so the same signal name may have different enumerated mappings on different sheets. |
| FR-47 | The system shall export plot screenshots to common image formats through the GUI. |
| FR-48 | The system shall surface document state, opened-file context, active signal context, and interaction status in the workspace shell. |
| FR-49 | The system shall persist UI settings in a version-scoped namespace so saved preferences are isolated per application line. |

## 4. Data Requirements

### 4.1 Supported Input Models

#### CSV

- comma-delimited tabular format
- single-sheet model only
- one header row containing `time` or `t` plus signal columns
- no worksheet concept

#### TSV / TXT

- tab-delimited equivalent of the CSV model
- single-sheet model only
- intended for spreadsheet-friendly plain-text exchange

#### JSON

- structured representation with explicit signal arrays, interpolation
  metadata, enumeration definitions, and samples
- single-document model in the current implementation

#### SpreadsheetML XML

- Excel 2003 XML workbook representation
- may contain multiple worksheets
- each relevant worksheet maps to one sheet-local signal library

#### XLSX

- zipped Open XML workbook representation
- may contain multiple worksheets
- user-facing data sheets are stored as plain tables
- enumerated mappings are stored in a dedicated `METADATA` worksheet

### 4.2 Common Data Rules

- decimal separator shall be `.`
- time values are expected to be monotonically increasing inside a signal
- table-driven sample insertion shall propose a timestamp greater than the
  current last sample timestamp
- the shared editor model assumes a common time axis across signals inside one
  edited sheet
- for tabular imports, a time column named `time` or `t` is required
- enumerated values may be encoded as plain strings in supported tabular inputs
- when explicit enumerated mappings are absent, the first distinct string shall
  map to `0`, the next distinct string to `1`, and so on in order of first
  appearance
- workbook formats may contain sheet-local signals with the same signal name
  but different enumerated mappings

### 4.3 Export Rules

- export shall operate on the active document
- output values shall respect the signal interpolation mode
- exporting an empty library shall fail clearly rather than writing an
  ambiguous result
- CSV and TSV/TXT exports shall remain single-sheet tabular outputs
- SpreadsheetML XML and XLSX exports may contain multiple worksheets
- XLSX data worksheets shall begin with a plain header row containing `time`
  plus the signal headers
- XLSX enumerated mappings shall not be written inline in the data worksheets
- XLSX enumerated mappings shall instead be written to `METADATA`

## 5. Domain Invariants

- signal names shall be non-empty
- signal samples shall remain ordered by increasing time
- duplicate timestamps shall collapse into a single logical sample entry
- table-driven insertion defaults shall be derived from the current domain
  signal state, not from transient widget state
- indexed access shall remain bounds-checked where defined by the API
- enumerated labels shall be non-empty and unique within one signal mapping
- enumerated numeric values shall be unique within one signal mapping
- enumerated sample edits shall snap to a legal mapped numeric value
- enumerated-signal remapping shall preserve logical labels where possible
  rather than treating remapping as an arbitrary numeric rewrite
- the active editable signal is workspace state and may legitimately be
  undefined when no visible signal remains

## 6. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-1 | The core domain shall remain independent from Qt. |
| NFR-2 | Persistence shall remain abstracted behind repository ports. |
| NFR-3 | The codebase shall preserve Windows MinGW64 GUI support. |
| NFR-4 | The codebase shall preserve Linux GCC core/test support. |
| NFR-5 | The supported compiler matrix shall remain GCC/MinGW-only unless governance explicitly changes that policy. |
| NFR-6 | Public module boundaries should remain documented with Doxygen comments. |
| NFR-7 | The UI shall clearly distinguish selected files, opened file, visible signals, and active signal. |
| NFR-8 | Reload and workbook-sheet switching shall avoid stale widget bindings and stale pointers. |
| NFR-9 | Documentation shall be updated when format semantics, workspace behavior, architecture, or persistence scope changes. |
| NFR-10 | Automated tests shall cover core domain, service orchestration, and repository behavior where practical, including XLSX workbook handling. |

## 7. Verification Strategy

Verification should combine:

- unit tests for domain invariants, interpolation, and enumerated behavior
- repository tests for load/save and round-trip semantics
- dedicated workbook tests for XML/XLSX sheet handling and XLSX metadata
  persistence
- manual GUI validation for workspace navigation, plot editing, table editing,
  reload, and workbook sheet switching
- documentation review to confirm alignment with implementation

## 8. Traceability Notes

The repository treats requirements, architecture, and implementation as linked
artifacts. Changes to workspace navigation, supported formats, workbook
semantics, interpolation semantics, or enumerated-state handling should be
reflected in both code and documentation as part of the same delivery flow.
