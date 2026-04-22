# Product Requirements Document

## Document Control

| Field | Value |
|-------|-------|
| Product | Signal Editor |
| Version | 0.1.0 |
| Status | Active working document |
| Primary audience | Product, Engineering, QA |

## Executive Summary

Signal Editor is a desktop engineering application for waveform inspection, authoring, correction, and export. The product is intended for users who need a faster and more reliable alternative to spreadsheet-based editing while avoiding the complexity of larger toolchains when only bounded signal manipulation is required.

The current product emphasizes:

- a multi-document workspace
- explicit file selection versus file opening semantics
- interactive plot editing
- exact table editing
- explicit interpolation control
- enumerated signal support
- reliable multi-format persistence
- workbook-aware XML/XLSX handling

## Background and Motivation

Real-world engineering workflows routinely exchange waveform data as CSV-like tables, spreadsheet exports, and tool-oriented JSON structures. Those datasets often need small but high-confidence modifications such as:

- reshaping a reference trace
- fixing outliers or timing offsets
- generating a synthetic waveform for testing
- converting or preserving enumerated state channels
- saving results in a downstream-consumable interchange format

Traditional approaches are usually unsatisfactory:

- spreadsheets are convenient but semantically weak
- scripts are efficient but opaque to many users
- large simulation tools are often too heavyweight for targeted edits

Signal Editor exists to occupy this middle ground.

## Product Scope

### In scope for the current product line

- loading one or more supported signal files into a shared workspace
- selecting one or more files in the workspace and opening one of them explicitly
- switching among active workspace documents
- switching among worksheets inside workbook formats
- selecting an active signal from the current document
- editing that signal through either a plot view or a table view
- creating new signals from predefined templates and enumerated state definitions
- modifying interpolation behavior for non-enumerated signals
- exporting the current document to supported file formats while preserving supported metadata
- maintaining undo history for the active document during the current session
- persisting UI settings per application version so releases do not trample each other's preferences

### Explicitly out of scope for now

- collaborative editing
- remote storage backends
- broad scientific analysis features such as FFT, filtering suites, or statistics dashboards
- project/session management beyond currently loaded documents

## Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| FR-01 | The product shall load CSV, TSV/TXT, JSON, SpreadsheetML XML, and XLSX signal files. | High | Core capability |
| FR-02 | The product shall support a multi-document workspace with an active document concept. | High | Fundamental navigation model |
| FR-03 | The product shall allow file selection independently from file opening in the workspace file list. | High | Prevents accidental context switching during batch selection |
| FR-04 | The product shall present a signal list for the opened document. | High | Required for signal selection |
| FR-05 | The product shall support worksheet selection for workbook formats. | High | Required for XML/XLSX multi-sheet workflows |
| FR-06 | The product shall provide a dedicated plot workspace tab for visual waveform editing. | High | Primary interaction path |
| FR-07 | The product shall provide a dedicated table workspace tab for exact sample editing. | High | Precision editing path |
| FR-08 | The product shall support drag, insert, remove, and Gaussian brushing operations in the plot for numeric signals. | High | Differentiating editing capability |
| FR-09 | The product shall support row-based sample editing, insertion, and removal in the table. | High | Complements plot editing |
| FR-10 | The product shall expose interpolation control at workspace level so it is available from both plot and table workflows. | High | Signal-level property should not be hidden behind one editing mode |
| FR-11 | The product shall allow creating new signals from waveform templates including constant, sine, cosine, pulse, sawtooth, triangle, ramp, and enumerated states. | High | Covers authoring workflow |
| FR-12 | The product shall preserve interpolation metadata when supported by the persistence format. | High | Prevents semantic loss |
| FR-13 | The product shall preserve enumerated label/value mappings when supported by the persistence format. | High | Required for readable state channels |
| FR-14 | The product shall render enumerated values as human-readable labels in both plot and table contexts. | High | Usability requirement |
| FR-15 | The product shall support active-document undo. | High | Required for safe interactive editing |
| FR-16 | The product shall support reload-from-disk for one workspace file without discarding the rest of the workspace. | High | Important recovery feature |
| FR-17 | The product shall support drag-and-drop file opening in the GUI. | Medium | Important convenience feature |
| FR-18 | The product shall show document state, signal context, and interaction hints in the workspace shell. | Medium | UX clarity requirement |
| FR-19 | The product shall persist UI settings in a version-scoped store so each application line can restore its own preferences independently. | Medium | Prevents cross-version settings collisions |

## Non-Functional Requirements

| ID | Requirement | Priority |
|----|-------------|----------|
| NFR-01 | Domain and use-case logic shall remain independent from Qt. | High |
| NFR-02 | Persistence shall be abstracted behind repository ports. | High |
| NFR-03 | The repository shall remain buildable on Windows MinGW64 for GUI workflows. | High |
| NFR-04 | The repository shall remain buildable on Linux GCC for core and test workflows. | High |
| NFR-05 | The repository shall remain aligned with a GCC/MinGW-only supported toolchain matrix unless governance explicitly changes that policy. | High |
| NFR-06 | User-visible behavior changes shall be documented in repository docs and changelog entries. | High |
| NFR-07 | Public module boundaries should remain documented with Doxygen comments. | Medium |
| NFR-08 | UI changes should improve clarity and space efficiency rather than only changing surface styling. | Medium |

## Acceptance Criteria

A meaningful product increment should satisfy all applicable criteria:

- the feature or fix is implemented coherently
- format semantics are preserved or explicitly updated in docs
- the workspace remains understandable in both plot and table modes
- enumerated-state behavior remains readable and deterministic
- automated tests cover the relevant logic where practical
- documentation reflects the final behavior

## Risks and Open Questions

- Whether native `.xlsx` support is worth the dependency and maintenance cost
- Whether future releases should support multi-signal overlay editing in the plot
- Whether Linux GUI delivery should become a first-class supported runtime target
- Whether future document/session persistence should extend beyond direct file edits
