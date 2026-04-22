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
│          ├──────────────> adapters/filesystem/CsvSignalRepository         │
│          ├──────────────> adapters/filesystem/DelimitedSignalRepository   │
│          ├──────────────> adapters/filesystem/JsonSignalRepository        │
│          ├──────────────> adapters/filesystem/SpreadsheetXmlSignalRepository│
│          └──────────────> adapters/filesystem/XlsxSignalRepository       │
│                                                                            │
│      adapters/filesystem/*Tabular*Repository                              │
│          └──────────────> adapters/filesystem/TabularSignalRows           │
│                                                                            │
│      adapters/filesystem/WorkbookModel                                    │
│          └──────────────> sheet-level persistence payload                 │
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
| `SignalFileRepository` | `adapters/filesystem/signal_file_repository.*` | Thin dispatcher that selects the concrete repository adapter from the file extension |
| `CsvSignalRepository` | `adapters/filesystem/csv_signal_repository.*` | CSV-specific parsing and export semantics |
| `DelimitedSignalRepository` | `adapters/filesystem/delimited_signal_repository.*` | TSV/TXT parsing and export semantics for delimiter-based tabular files |
| `JsonSignalRepository` | `adapters/filesystem/json_signal_repository.*` | JSON-specific parsing and export semantics |
| `SpreadsheetXmlSignalRepository` | `adapters/filesystem/spreadsheet_xml_signal_repository.*` | SpreadsheetML XML parsing and export semantics |
| `XlsxSignalRepository` | `adapters/filesystem/xlsx_signal_repository.*` | Native XLSX workbook parsing and export semantics including dedicated metadata worksheet handling |
| `TabularSignalRows` | `adapters/filesystem/tabular_signal_rows.*` | Shared row-model mapping used by tabular adapters after raw format decoding |
| `WorkbookModel` | `adapters/filesystem/workbook_model.h` | Workbook-level payload that allows one file to carry multiple sheet-local libraries |
| `signal_editor_api.h` | `api/signal_editor_api.h` | Public facade consumed by the application shell |

## Notes

The GUI layer depends on these components indirectly through the service/API surface. The domain remains unaware of Qt widgets, dialogs, rendering concerns, and concrete file-format parsing details.
