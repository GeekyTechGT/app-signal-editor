# Software Requirements Specification — Signal Editor

**Version:** 0.1.0
**Date:** 2026-04-11
**Status:** Working draft
**Audience:** Developers, QA, Product

## 1. Introduction

### 1.1 Purpose

This specification describes the functional and non-functional requirements for Signal Editor, a C++23 desktop application with a Qt 6 GUI used to inspect, edit, generate, and export waveform data stored in CSV files.

### 1.2 Scope

The current product scope includes:

- loading one or more CSV files into a workspace
- switching between loaded documents without closing the application
- viewing and editing signals through both a plot and a sample table
- creating new signals from common waveform templates
- renaming and removing signals from the active document
- changing interpolation mode between linear and step behavior
- saving edited data back to CSV with interpolation metadata preserved
- undoing edits within the active document session

Out of scope for this release:

- collaborative editing
- FFT/spectral analysis
- native MATLAB or proprietary signal file formats
- project/session files beyond direct CSV inputs
- multi-user or network-backed persistence

## 2. Product Description

Signal Editor is a local desktop engineering tool. The core editing model is framework-independent and the GUI is an adapter layered on top of it.

### 2.1 User Classes

- **Controls engineer**: adjusts reference traces and validation signals quickly
- **Test engineer**: cleans and reshapes measured data before replay or reporting
- **Tooling engineer**: extends adapters and workflows while preserving core architecture

### 2.2 Operating Environment

- Windows 10/11 with MinGW64 and Qt 6 for GUI workflows
- Linux with GCC for core library and automated test workflows
- Local filesystem access to CSV inputs and outputs

## 3. Functional Requirements

| ID | Requirement |
|----|-------------|
| FR-1 | The system shall load CSV files whose first column is time and whose remaining columns represent signals. |
| FR-2 | The system shall accept both header-based signal names and generated fallback names when headers are absent. |
| FR-3 | The system shall support opening files through the menu and drag-and-drop. |
| FR-4 | The system shall maintain a workspace containing multiple loaded CSV documents. |
| FR-5 | The system shall let the user activate a different document without losing in-memory edits in the others. |
| FR-6 | The system shall display all signals of the active document in a dedicated list panel. |
| FR-7 | The system shall render the selected signal on an interactive plot with auto-fitted axes. |
| FR-8 | The system shall allow the user to drag existing waypoints to new positions on the plot. |
| FR-9 | The system shall allow the user to insert a waypoint by double-clicking on the plot. |
| FR-10 | The system shall allow the user to remove a waypoint from the plot context menu. |
| FR-11 | The system shall allow the user to apply Gaussian brushing by using `Shift` while dragging on the plot. |
| FR-12 | The system shall allow sample editing in tabular form. |
| FR-13 | The system shall allow adding and removing samples in the table. |
| FR-14 | The system shall allow creating a new signal from waveform templates such as constant, sine, cosine, pulse, sawtooth, triangle, and ramp. |
| FR-15 | The system shall allow renaming and removing signals in the active document. |
| FR-16 | The system shall support per-signal interpolation modes of `linear` and `step`. |
| FR-17 | The system shall save interpolation metadata when exporting CSV files. |
| FR-18 | The system shall provide undo for edits made to the active document. |
| FR-19 | The system shall surface cursor coordinates and workspace state in the UI. |

## 4. Data Requirements

### 4.1 CSV Input Format

- Delimiter: comma
- Decimal separator: dot
- First column: strictly increasing time values
- Additional columns: signal values aligned to the row time
- Optional first metadata row: interpolation descriptors
- Optional header row: signal names

### 4.2 Export Rules

- Exports shall contain the union of all timestamps across signals.
- Signal values shall be interpolated according to the signal's interpolation mode.
- Empty libraries shall not be exported successfully.

## 5. Domain Invariants

- Signal names shall be non-empty.
- Samples within a signal shall remain ordered by increasing time.
- Duplicate timestamps shall collapse into a single sample entry.
- The library shall expose bounds-checked access for indexed operations.

## 6. Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-1 | The core library shall remain independent from Qt. |
| NFR-2 | Persistence shall be abstracted behind a repository port. |
| NFR-3 | The codebase shall keep build support for Windows GUI workflows and Linux core/test workflows. |
| NFR-4 | Public module interfaces shall be documented using Doxygen-style comments. |
| NFR-5 | The UI shall make active document state and edit affordances easy to understand. |
| NFR-6 | Unit tests shall cover domain logic, service orchestration, and CSV repository behavior. |

## 7. Verification Strategy

- Unit tests validate `Signal`, `SignalLibrary`, `SignalEditorService`, and `CsvSignalRepository`.
- Manual GUI verification covers workspace switching, plot interactions, and save/load round-trips.
- Documentation review verifies alignment between implementation and stated architecture.
