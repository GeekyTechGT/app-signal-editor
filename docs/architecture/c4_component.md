# C4 Model — Level 3: Component

## Component Diagram for `myprj_signal_editor_core`

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                        myprj_signal_editor_core                           │
│                                                                            │
│  api/signal_editor_api.h                                                   │
│          │                                                                 │
│          v                                                                 │
│  core/usecases/SignalEditorService                                         │
│      ├──────────────> core/domain/SignalLibrary                            │
│      │                  └────────────> core/domain/Signal                  │
│      │                                         └────> SamplePoint          │
│      │                                                                    │
│      └──────────────> ports/ISignalRepository                              │
│                               ^                                            │
│                               │                                            │
│      adapters/filesystem/CsvSignalRepository                              │
│      adapters/filesystem/SignalFileRepository                             │
└────────────────────────────────────────────────────────────────────────────┘
```

## Components

| Component | Location | Responsibility |
|-----------|----------|----------------|
| `SignalEditorService` | `core/usecases/signal_editor_service.*` | Coordinates load/save/edit/create workflows and repository interaction |
| `SignalLibrary` | `core/domain/signal_library.*` | Owns the collection of active signals |
| `Signal` | `core/domain/signal.*` | Enforces waveform invariants, interpolation behavior, and enumerated-state semantics |
| `SamplePoint` | `core/domain/sample_point.h` | Value object representing time/value pairs |
| `ISignalRepository` | `ports/signal_repository.h` | Persistence abstraction consumed by the use-case layer |
| `CsvSignalRepository` | `adapters/filesystem/csv_signal_repository.*` | CSV-specific parsing and export semantics |
| `SignalFileRepository` | `adapters/filesystem/signal_file_repository.*` | Multi-format dispatch and non-CSV persistence handling |
| `signal_editor_api.h` | `api/signal_editor_api.h` | Public facade consumed by the application shell |

## Notes

The GUI layer depends on these components indirectly through the service/API surface. The domain remains unaware of Qt widgets, dialogs, or rendering concerns.
