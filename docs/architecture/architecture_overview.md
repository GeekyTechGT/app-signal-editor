# Architecture Overview

## 1. Purpose

This document explains the architecture of Signal Editor and the rationale behind the current design choices. It is intended to help developers extend the application without breaking editing behavior, UI responsiveness, or module boundaries.

## 2. Architectural Style

Signal Editor uses a hexagonal architecture with a narrow application shell.

```text
apps/gui -> api -> core/usecases -> ports <- adapters
                      |
                      -> core/domain
```

The key rule is that the domain and use cases do not depend on Qt, filesystem code, or concrete persistence details.

## 3. Main Building Blocks

| Layer | Location | Responsibility |
|-------|----------|----------------|
| Composition root | `apps/signal_editor/gui/` | Creates the Qt application and wires dependencies |
| API | `src/signal_editor/api/` | Re-exports the module surface used by apps |
| Use cases | `src/signal_editor/core/usecases/` | Orchestrates load/save/edit workflows |
| Domain | `src/signal_editor/core/domain/` | Owns waveform entities, invariants, and editing primitives |
| Ports | `src/signal_editor/ports/` | Defines persistence contracts |
| Adapters | `src/signal_editor/adapters/` | Implements Qt UI and CSV persistence |
| Shared utilities | `src/common/` | Result types and reusable support code |

## 4. Why This Structure Matters

This architecture keeps the project extensible in practical ways:

- the CSV adapter can evolve without touching waveform invariants
- the GUI can become richer without pulling Qt types into the core
- automated tests can exercise editing behavior without starting the UI
- future adapters can reuse the same service and domain model

## 5. Key Runtime Flows

### 5.1 Load

1. The main window asks `SignalEditorService` to load a path.
2. The service delegates parsing to `ISignalRepository`.
3. The CSV adapter returns a `SignalLibrary`.
4. The window binds the active document into the list, plot, and table widgets.

### 5.2 Edit

1. The user edits either through the plot or the table.
2. The window snapshots undo state at the document level.
3. The bound `Signal` is mutated through domain operations or service calls.
4. The plot, table, and workspace status are refreshed.

### 5.3 Save

1. The active document is synchronized back into the service.
2. The service delegates persistence to the CSV adapter.
3. The adapter exports the union time axis and interpolation metadata.
4. The window clears dirty state and refreshes workspace summaries.

## 6. Quality Attributes

The current design optimizes for:

- maintainability through clear boundaries
- testability of the core editing model
- deterministic CSV round-trips
- UI responsiveness for day-to-day engineering work

## 7. Known Constraints

- GUI delivery is currently centered on the Windows MinGW64 preset.
- Linux presets focus on core and test workflows.
- Persistence is currently limited to CSV-backed workspaces.
