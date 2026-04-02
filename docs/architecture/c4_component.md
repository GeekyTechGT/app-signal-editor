# C4 Model — Level 3: Component

## Component Diagram for `myprj_my_module_core`

```
┌────────────────────────────────────────────────────────────────────┐
│  myprj_my_module_core                                              │
│                                                                    │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  api/MyModuleApi                                            │  │
│  │  Public facade — thin wrapper over use cases                │  │
│  └───────────────────────────┬─────────────────────────────────┘  │
│                              ↓                                     │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  core/usecases/MyUseCase                                    │  │
│  │  Orchestrates domain logic; depends only on ports           │  │
│  └─────────────┬─────────────────────────────────┬─────────────┘  │
│                ↓                                 ↓                 │
│  ┌─────────────────────────┐  ┌─────────────────────────────────┐ │
│  │  core/domain/MyEntity   │  │  ports/IMyRepository            │ │
│  │  Immutable domain type  │  │  Abstract output port           │ │
│  └─────────────────────────┘  └────────────┬────────────────────┘ │
│                                            ↑                      │
│                               ┌────────────┴────────────────────┐ │
│                               │  adapters/                       │ │
│                               │  FsAdapter (filesystem)          │ │
│                               │  JsonAdapter (config parsing)    │ │
│                               │  CliAdapter (arg parsing)        │ │
│                               │  GuiAdapter (Qt widget logic)    │ │
│                               └─────────────────────────────────┘ │
└────────────────────────────────────────────────────────────────────┘
```

## Components

| Component          | Location                              | Responsibility                              |
|--------------------|---------------------------------------|---------------------------------------------|
| MyEntity           | `core/domain/my_entity.h`             | Domain object with enforced invariants       |
| MyUseCase          | `core/usecases/my_usecase.h`          | Business orchestration                       |
| IMyRepository      | `ports/my_port.h`                     | Abstract persistence interface               |
| FsAdapter          | `adapters/filesystem/fs_adapter.h`    | Filesystem-backed repository implementation  |
| JsonAdapter        | `adapters/json/json_adapter.h`        | JSON configuration parser                    |
| CliAdapter         | `adapters/cli/cli_adapter.h`          | argv parser and usage printer                |
| MyModuleApi        | `api/my_module_api.h`                 | Public entry point used by apps              |
