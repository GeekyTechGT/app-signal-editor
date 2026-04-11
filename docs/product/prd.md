# Product Requirements Document

## Overview

| Field | Value |
|-------|-------|
| Product | Signal Editor |
| Version | 0.1.0 |
| Status | Active draft |
| Owner | Engineering team |

## Background

Signal Editor exists to cover the gap between heavyweight model-based tooling and ad-hoc CSV editing in spreadsheets. The product should let engineers manipulate waveforms confidently while keeping the implementation maintainable enough for long-term extension.

## Core User Stories

- As an engineer, I want to open one or more CSV files and switch between them inside a single workspace.
- As a test engineer, I want to edit signals directly on a plot so I can reshape traces faster than by editing raw CSV rows.
- As a tooling engineer, I want the editing logic isolated from Qt so that the core stays testable and reusable.

## Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| FR-01 | The product shall load CSV files containing a shared time column and one or more signal columns. | High | Header row optional |
| FR-02 | The product shall support a multi-file workspace with an active document concept. | High | Required for quick comparison/edit switching |
| FR-03 | The product shall allow direct manipulation of plotted samples, including drag, insert, remove, and Gaussian brushing. | High | Main UX differentiator |
| FR-04 | The product shall allow precise sample editing in tabular form. | High | Complements plot editing |
| FR-05 | The product shall allow creating new signals from template waveforms. | Medium | Supports synthetic trace authoring |
| FR-06 | The product shall preserve interpolation mode metadata when saving and reloading CSV files. | High | Prevents semantic loss |
| FR-07 | The product shall provide undo at the active document level. | High | Guards interactive edits |

## Non-Functional Requirements

| ID | Requirement | Priority |
|----|-------------|----------|
| NFR-01 | Core editing logic shall remain independent from Qt and direct filesystem calls. | High |
| NFR-02 | The GUI shall communicate active workspace state clearly, including file status and edit affordances. | High |
| NFR-03 | The repository shall build on Windows MinGW64 for the GUI and Linux GCC for core/test workflows. | High |
| NFR-04 | Public module boundaries shall be documented with Doxygen-style comments. | Medium |
| NFR-05 | CSV round-trip behavior shall be validated through automated tests. | High |

## Acceptance Criteria

- [ ] Workspace loading, editing, and export flows are stable on the supported presets
- [ ] Core use cases and CSV repository behaviors remain covered by unit tests
- [ ] Product and architecture docs reflect the actual implementation
- [ ] No scaffold placeholders remain in active project-facing documentation

## Open Decisions

- Whether to add overlay/multi-signal comparison editing in a future release
- Whether Linux GUI delivery becomes a first-class supported distribution target
