# Architecture Overview

## 1. Purpose

This document explains the current architecture of Signal Editor, the responsibilities of its major subsystems, and the design constraints that contributors are expected to preserve.

It is written for developers, reviewers, and maintainers who need to understand not only where code lives, but why it is structured that way.

## 2. Architectural Style

Signal Editor uses a hexagonal architecture with a thin desktop application shell.

```text
apps/gui -> api -> core/usecases -> ports <- adapters
                      |
                      -> core/domain
```

The governing rule is simple:

- the domain and use cases must not depend on Qt
- the domain and use cases must not depend on filesystem details
- concrete format parsing and GUI behavior must remain outside the core

## 3. Primary Building Blocks

| Layer | Location | Responsibility |
|-------|----------|----------------|
| Composition root | `apps/signal_editor/gui/` | Creates the Qt application and wires service plus adapters |
| API | `src/signal_editor/api/` | Exposes a compact module-facing surface for executables |
| Use cases | `src/signal_editor/core/usecases/` | Coordinates load, save, create, replace, rename, remove, and interpolation workflows |
| Domain | `src/signal_editor/core/domain/` | Owns waveform entities, invariants, interpolation rules, and enumerated-state semantics |
| Ports | `src/signal_editor/ports/` | Defines repository abstractions consumed by the use-case layer |
| Filesystem adapters | `src/signal_editor/adapters/filesystem/` | Implements one repository adapter per format plus a dispatcher and shared row-mapping support for tabular formats |
| Qt adapters | `src/signal_editor/adapters/qt/` | Implements workspace shell, tabs, plot, table, dialogs, and interaction logic |
| Shared support | `src/signal_editor/core/domain/result.h` | Provides the lightweight result contract used by service and repository boundaries |

## 4. Why This Structure Matters

The architecture exists to preserve practical engineering benefits:

- waveform rules remain testable without launching the GUI
- persistence behavior can evolve without rewriting editing logic
- UI refactors do not require domain rewrites
- format-specific behavior stays explicit and auditable while the dispatcher remains thin
- future adapters can reuse the same core workflows

## 5. Key Runtime Flows

### 5.1 Load Flow

1. The main window receives a load request from the menu or drag-and-drop.
2. `SignalEditorService` delegates the path to the configured repository port.
3. `SignalFileRepository` dispatches by file extension to dedicated CSV, TSV/TXT, JSON, and SpreadsheetML XML adapters.
   `DelimitedSignalRepository` covers the TSV/TXT family because those formats differ only by delimiter semantics, not by domain mapping rules.
4. The resulting `SignalLibrary` is bound into the workspace document model.
5. The active signal is exposed to both the plot and table adapters.

### 5.2 Edit Flow

1. The user edits the selected signal through either the plot tab or the table tab.
2. The main window snapshots undo state for the active document.
3. Domain entities apply edits while preserving ordering, interpolation, and enumerated-state invariants.
4. The workspace refreshes plot, table, status, and signal summary state.
5. Shared controls such as interpolation are synchronized from the active signal rather than being tied to a single editing surface.

### 5.3 Save Flow

1. The active document is synchronized into the service layer.
2. The service delegates persistence to the selected repository implementation.
3. The adapter exports a union time axis plus any supported metadata.
4. Interpolation and enumerated-state semantics are preserved where the format allows them.
5. Workspace dirty state is cleared after a successful save.

## 6. Current UI Architecture Notes

The Qt shell intentionally separates workspace concerns from editing surfaces.

Key points:

- file selection and signal selection remain in dedicated side panels
- the center workspace is tab-driven rather than split vertically
- plot and table are treated as alternative views over the same active signal
- interpolation control is presented at workspace scope because it applies to the signal itself, not to the table only
- enumerated signals constrain the UI by disabling inappropriate interpolation changes and exposing label-based values

## 7. Quality Attributes

The architecture is currently optimized for:

- maintainability
- testability
- deterministic persistence semantics
- explicit format behavior
- safe handling of enumerated signals
- UI clarity for day-to-day engineering usage

## 8. Constraints and Known Limits

- Windows MinGW64 remains the primary GUI delivery path
- Linux currently focuses on core and test workflows
- native `.xlsx` and `.xls` parsing is intentionally not present
- the repository does not yet include collaborative or remote-backed persistence

## 9. Architectural Change Rules

If a change affects any of the following, the architecture documentation should be updated in the same change set:

- dependency direction
- supported runtime flows
- adapter responsibilities
- workspace navigation model
- interpolation or enumerated-state behavior that changes the boundaries between core and UI
