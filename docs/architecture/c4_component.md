# C4 Model — Level 3: Component

## Component Diagram for `signal_editor_core`

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                        signal_editor_core                                 │
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
│      adapters/filesystem/SignalFileRepository                             │
│          ├──────────────> adapters/filesystem/TabularSignalCodec          │
│      adapters/filesystem/CsvSignalRepository                              │
│          └──────────────> adapters/filesystem/TabularSignalCodec          │
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
| `SignalFileRepository` | `adapters/filesystem/signal_file_repository.*` | Primary filesystem adapter: dispatches by extension and owns JSON/XML persistence paths |
| `CsvSignalRepository` | `adapters/filesystem/csv_signal_repository.*` | Narrow CSV-only wrapper used when a caller intentionally wants CSV-constrained persistence |
| `TabularSignalCodec` | `adapters/filesystem/tabular_signal_codec.*` | Shared row/column parsing and export rules reused by CSV, TSV/TXT, and SpreadsheetML flows |
| `signal_editor_api.h` | `api/signal_editor_api.h` | Public facade consumed by the application shell |

## Notes

The GUI layer depends on these components indirectly through the service/API surface. The domain remains unaware of Qt widgets, dialogs, rendering concerns, and concrete file-format parsing details.
