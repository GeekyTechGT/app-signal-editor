# C4 Model — Level 2: Container

## Container Diagram

```text
┌──────────────────────────────────────────────────────────────────────┐
│                           Signal Editor                             │
│                                                                      │
│  ┌──────────────────────────────┐                                    │
│  │ myprj_signal_editor_gui      │                                    │
│  │ Qt application / shell       │                                    │
│  └───────────────┬──────────────┘                                    │
│                  │                                                   │
│  ┌───────────────v────────────────────────────────────────────────┐   │
│  │ myprj_signal_editor_qt                                        │   │
│  │ Qt widgets, dialogs, workspace controller, plot interactions  │   │
│  └───────────────┬────────────────────────────────────────────────┘   │
│                  │                                                   │
│  ┌───────────────v────────────────────────────────────────────────┐   │
│  │ myprj_signal_editor_core                                      │   │
│  │ Domain, use cases, ports, CSV adapter, public API facade      │   │
│  └───────────────┬────────────────────────────────────────────────┘   │
│                  │                                                   │
│  ┌───────────────v──────────────┐                                    │
│  │ myprj_common                 │                                    │
│  │ Shared result/support types  │                                    │
│  └──────────────────────────────┘                                    │
└──────────────────────────────────────────────────────────────────────┘
```

## Containers

| Container | Technology | Purpose |
|-----------|------------|---------|
| `myprj_signal_editor_gui` | C++23, Qt6 | GUI executable and composition root |
| `myprj_signal_editor_qt` | C++23, Qt6 | Workspace UI, plot widget, dialogs, and panel logic |
| `myprj_signal_editor_core` | C++23 | Domain model, use cases, CSV adapter, and API surface |
| `myprj_common` | C++23 | Shared result and support types |

## Key Dependencies

- Qt 6 Widgets for GUI delivery
- GoogleTest for automated verification
- nlohmann/json as an available project dependency
