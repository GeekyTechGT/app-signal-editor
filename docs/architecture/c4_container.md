# C4 Model — Level 2: Container

## Container Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│  MyProject                                                         │
│                                                                    │
│  ┌───────────────────┐    ┌───────────────────┐                   │
│  │  my_module (CLI)  │    │  my_module (GUI)  │                   │
│  │  C++ executable   │    │  Qt executable    │                   │
│  └────────┬──────────┘    └────────┬──────────┘                   │
│           │                        │                               │
│           └──────────┬─────────────┘                               │
│                      ↓                                             │
│  ┌────────────────────────────────────────────────┐               │
│  │  myprj_my_module_core  (shared library)        │               │
│  │  C++ — domain, use cases, ports, adapters, api │               │
│  └───────────────────┬────────────────────────────┘               │
│                      │                                             │
│  ┌───────────────────┴────────────────────────────┐               │
│  │  myprj_common  (shared library)                │               │
│  │  C++ — shared domain types, utilities          │               │
│  └────────────────────────────────────────────────┘               │
└────────────────────────────────────────────────────────────────────┘
```

## Containers

| Container              | Technology | Purpose                                         |
|------------------------|------------|-------------------------------------------------|
| `my_module` CLI        | C++23      | Command-line entry point; composition root       |
| `my_module-gui`        | C++23, Qt6 | Graphical entry point; composition root          |
| `myprj_my_module_core` | C++23      | Core library: domain + use cases + adapters + api|
| `myprj_common`         | C++23      | Shared types and utilities across modules        |

## Key Dependencies

- nlohmann/json — JSON configuration parsing
- OpenSSL — cryptographic operations (if needed)
- GoogleTest — unit testing (not deployed)
- Qt6 Widgets — GUI rendering (optional)
