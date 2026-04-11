# C4 Model — Level 2: Container

## Container Diagram

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                               Signal Editor                               │
│                                                                            │
│  ┌──────────────────────────────┐                                          │
│  │ signal_editor_gui            │                                          │
│  │ Qt executable / composition  │                                          │
│  └───────────────┬──────────────┘                                          │
│                  │                                                         │
│  ┌───────────────v──────────────────────────────────────────────────────┐   │
│  │ signal_editor_qt                                                   │   │
│  │ Main window, workspace tabs, plot widget, table panel, dialogs      │   │
│  └───────────────┬──────────────────────────────────────────────────────┘   │
│                  │                                                         │
│  ┌───────────────v──────────────────────────────────────────────────────┐   │
│  │ signal_editor_core                                                 │   │
│  │ Domain, use cases, ports, repositories, and public API facade       │   │
│  └───────────────┬──────────────────────────────────────────────────────┘   │
│                  │                                                         │
└────────────────────────────────────────────────────────────────────────────┘
```

## Containers

| Container | Technology | Purpose |
|-----------|------------|---------|
| `signal_editor_gui` | C++23, Qt 6 | GUI executable and composition root |
| `signal_editor_qt` | C++23, Qt 6 | Workspace shell, plot editing, table editing, dialogs, and presentation logic |
| `signal_editor_core` | C++23 | Domain model, use cases, repository abstractions, filesystem repositories, and API surface |

## Key Dependencies

- Qt 6 Widgets for GUI delivery
- GoogleTest for automated verification
- nlohmann/json for JSON persistence support

## Container Notes

Although filesystem repositories live under the Signal Editor module rather than in a separate deployable process, they are still treated architecturally as infrastructure adapters consumed through ports.
