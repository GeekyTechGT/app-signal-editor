# C4 Model — Level 3: Component

## Component Diagram for `myprj_signal_editor_core`

```text
┌────────────────────────────────────────────────────────────────────┐
│                    myprj_signal_editor_core                       │
│                                                                    │
│  api/signal_editor_api.h                                           │
│          │                                                         │
│          v                                                         │
│  core/usecases/SignalEditorService                                 │
│      ├──────────────> core/domain/SignalLibrary                    │
│      │                  └────────────> core/domain/Signal          │
│      │                                         └────> SamplePoint  │
│      │                                                            │
│      └──────────────> ports/ISignalRepository                      │
│                               ^                                    │
│                               │                                    │
│               adapters/filesystem/CsvSignalRepository              │
└────────────────────────────────────────────────────────────────────┘
```

## Components

| Component | Location | Responsibility |
|-----------|----------|----------------|
| `SignalEditorService` | `core/usecases/signal_editor_service.*` | Coordinates load/save/edit workflows |
| `SignalLibrary` | `core/domain/signal_library.*` | Owns the collection of signals |
| `Signal` | `core/domain/signal.*` | Enforces waveform invariants and editing primitives |
| `SamplePoint` | `core/domain/sample_point.h` | Value object for time/value samples |
| `ISignalRepository` | `ports/signal_repository.h` | Persistence abstraction consumed by the core |
| `CsvSignalRepository` | `adapters/filesystem/csv_signal_repository.*` | CSV-specific persistence implementation |
| `signal_editor_api.h` | `api/signal_editor_api.h` | Public façade consumed by apps |
