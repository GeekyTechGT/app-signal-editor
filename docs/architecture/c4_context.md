# C4 Model — Level 1: System Context

## System Context Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                         System Boundary                          │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                      MyProject                          │    │
│  │   C++ Qt application with hexagonal architecture        │    │
│  └─────────────────────────────────────────────────────────┘    │
│           ↑                          ↑                           │
└───────────┼──────────────────────────┼───────────────────────────┘
            │                          │
   ┌────────┴────────┐        ┌────────┴────────┐
   │  Developer      │        │  Configuration  │
   │  (CLI / GUI)    │        │  Files (JSON)   │
   └─────────────────┘        └─────────────────┘
```

## Actors and External Systems

| Actor / System     | Role                                                      |
|--------------------|-----------------------------------------------------------|
| Developer          | Primary user — invokes CLI or GUI to process artifacts    |
| JSON config files  | External configuration consumed by adapters/json          |
| Filesystem         | Source/destination for input and output files             |
| Qt runtime         | GUI framework (optional, Windows and Linux)               |
| Build system       | CMake + Ninja — builds and packages the system            |
| Test framework     | GoogleTest — validates domain and adapter behavior        |

## System Purpose

MyProject provides:
- TODO: describe primary capability 1
- TODO: describe primary capability 2

## Constraints

- Runs on Windows (MinGW64) and Linux (Ubuntu, Alpine)
- GUI available only on Windows (and Linux if Qt is installed)
- No network access in core domain logic
