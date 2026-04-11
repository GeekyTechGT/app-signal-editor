# Product Vision

## Problem

Engineers often need to make small but precise adjustments to time-series signals without reopening a heavyweight modelling toolchain. Existing workflows are frequently slow, script-heavy, or tightly coupled to proprietary ecosystems.

## Vision

Signal Editor provides a focused desktop workspace for loading, inspecting, editing, and exporting waveform data with minimal friction. It aims to feel fast enough for quick correction loops while being structured enough for long-term maintainability and testability.

## Primary Users

| User | Goal | Pain Point |
|------|------|------------|
| Controls engineer | Tweak reference or validation traces quickly | Full simulation tools are slower than the task warrants |
| Test engineer | Clean and reshape measured CSV data before replay | Spreadsheet editing is error-prone and not waveform-aware |
| Tooling developer | Extend the editor without coupling to Qt internals | Many desktop tools mix domain logic directly into UI code |

## Product Goals

1. Make waveform inspection and editing fast for day-to-day engineering tasks.
2. Keep the core editing logic testable and independent from the GUI framework.
3. Preserve round-trip fidelity when loading, editing, and exporting CSV signal sets.
4. Provide a UI that communicates state clearly even for multi-file editing sessions.

## Non-Goals

- Replace full simulation authoring suites
- Provide collaborative or cloud-native workflows
- Act as a generic spreadsheet or data science notebook
- Support every proprietary waveform format in the first release

## Success Signals

- Common CSV edit loops can be completed without leaving the application
- Engineers can understand the active workspace state at a glance
- Core editing behavior stays covered by unit tests as features evolve
- New adapters or UI improvements can be introduced without refactoring domain logic
