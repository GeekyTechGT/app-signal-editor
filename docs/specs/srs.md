# Software Requirements Specification — Signal Editor

**Version:** 0.1.0
**Date:** 2026-04-11
**Status:** Working draft
**Audience:** Engineering, QA, Product, Reviewers

## 1. Introduction

### 1.1 Purpose

This specification defines the functional and non-functional behavior expected from Signal Editor, a C++23 desktop waveform editing application with a Qt 6 GUI and a hexagonal internal architecture.

### 1.2 Scope

The current implementation scope includes:

- loading one or more supported signal files into a shared workspace
- switching between active documents without losing in-memory edits in other loaded documents
- selecting a signal from the active document
- editing the selected signal in a dedicated plot tab or table tab
- controlling interpolation mode from a shared workspace-level selector
- creating new signals from numeric and enumerated templates
- saving edited documents in supported formats while preserving supported metadata
- undoing modifications inside the active document context

The following are out of scope for the current product line:

- collaborative or network-backed editing
- native binary Excel parsing (`.xlsx`, `.xls`)
- spectrum analysis or full signal-processing suites
- notebook-style scripting environments
- persistent multi-document project/session files beyond currently loaded data files

## 2. Product Description

Signal Editor is a local desktop engineering tool. It combines direct waveform manipulation with exact tabular editing while keeping the domain model independent from the GUI framework.

### 2.1 User Classes

- **Controls engineer**: adjusts reference and validation traces quickly
- **Test engineer**: reshapes measured data before replay or reporting
- **Tooling engineer**: extends core behavior or adapters while preserving architecture
- **Calibration or validation engineer**: works with enumerated and boolean-like state channels

### 2.2 Operating Environment

- Windows 10/11 with Qt 6 and MinGW64 for GUI delivery and full application usage
- Linux with GCC for core library and automated test workflows
- local filesystem access to supported input and output files

## 3. Functional Requirements

| ID | Requirement |
|----|-------------|
| FR-1 | The system shall load CSV, TSV/TXT, JSON, and SpreadsheetML XML files into the shared signal model. |
| FR-2 | The system shall support both menu-based and drag-and-drop file opening. |
| FR-3 | The system shall maintain a workspace that can hold multiple loaded documents simultaneously. |
| FR-4 | The system shall let the user activate a different document without discarding in-memory state from the others. |
| FR-5 | The system shall display the signals of the active document in a dedicated list panel. |
| FR-6 | The system shall render the selected signal in a dedicated plot tab. |
| FR-7 | The system shall render the selected signal in a dedicated table tab. |
| FR-8 | The system shall allow dragging existing waypoints on the plot. |
| FR-9 | The system shall allow inserting a waypoint in the plot by direct interaction. |
| FR-10 | The system shall allow removing a waypoint from the plot interaction flow. |
| FR-11 | The system shall allow Gaussian brushing for numeric signals when the proper plot gesture is used. |
| FR-12 | The system shall allow sample editing in tabular form. |
| FR-13 | The system shall allow sample insertion and removal in the table. |
| FR-14 | The system shall provide a shared interpolation selector that remains available in both plot and table workflows. |
| FR-15 | The system shall allow creating signals from constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated templates. |
| FR-16 | The system shall allow renaming and removing signals in the active document. |
| FR-17 | The system shall support `linear` and `step` interpolation modes for non-enumerated signals. |
| FR-18 | The system shall force enumerated signals to behave as `step` signals. |
| FR-19 | The system shall persist interpolation metadata when the target format supports it. |
| FR-20 | The system shall persist enumerated mappings when the target format supports it. |
| FR-21 | The system shall import enumerated values either from `# enum_map` metadata or inline `label:value` tokens in supported tabular formats. |
| FR-22 | The system shall render enumerated signal values as labels in the plot and table. |
| FR-23 | The system shall provide undo for edits within the active document. |
| FR-24 | The system shall surface document state, active signal context, and interaction status in the workspace shell. |

## 4. Data Requirements

### 4.1 Supported Input Models

#### CSV

- comma-delimited tabular format
- first column represents time
- optional metadata rows may appear before the header
- optional header row may provide signal names

#### TSV / TXT

- tab-delimited equivalent of the CSV model
- intended for spreadsheet-friendly plain-text exchange

#### JSON

- structured representation with explicit signal arrays, interpolation metadata, enumeration definitions, and samples

#### SpreadsheetML XML

- Excel 2003 XML workbook representation used as a structured spreadsheet interchange format

### 4.2 Common Data Rules

- decimal separator shall be `.`
- time values are expected to be monotonically increasing inside a signal
- metadata rows may define interpolation and enumeration details before tabular data begins
- enumerated values may be encoded as labels when a matching mapping exists
- enumerated values may be bootstrapped inline through `label:value` tokens
- native `.xlsx` and `.xls` binaries are out of scope for the current implementation

### 4.3 Export Rules

- export shall operate on the active document
- the exported time axis shall contain the union of all signal timestamps
- output values shall respect the signal interpolation mode
- enumerated signals shall emit labels where a declared mapping exists
- exporting an empty library shall fail clearly rather than writing an ambiguous result

## 5. Domain Invariants

- signal names shall be non-empty
- signal samples shall remain ordered by increasing time
- duplicate timestamps shall collapse into a single logical sample entry
- indexed access shall remain bounds-checked where defined by the API
- enumerated labels shall be non-empty and unique
- enumerated numeric values shall be unique within the mapping
- enumerated sample edits shall snap to a legal mapped numeric value

## 6. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-1 | The core domain shall remain independent from Qt. |
| NFR-2 | Persistence shall remain abstracted behind repository ports. |
| NFR-3 | The codebase shall preserve Windows MinGW64 GUI support. |
| NFR-4 | The codebase shall preserve Linux GCC core/test support. |
| NFR-5 | Public module boundaries should remain documented with Doxygen comments. |
| NFR-6 | The UI shall clearly expose active document state and editing affordances. |
| NFR-7 | Documentation shall be updated when format semantics, workspace behavior, or architecture changes. |
| NFR-8 | Automated tests shall cover core domain, service orchestration, and repository behavior where practical. |

## 7. Verification Strategy

Verification should combine:

- unit tests for domain invariants, interpolation, and enumerated behavior
- repository tests for load/save and round-trip semantics
- manual GUI validation for workspace navigation, plot editing, and table editing
- documentation review to confirm alignment with implementation

## 8. Traceability Notes

The repository treats requirements, architecture, and implementation as linked artifacts. Changes to workspace navigation, supported formats, interpolation semantics, or enumerated-state handling should be reflected in both code and documentation as part of the same delivery flow.
