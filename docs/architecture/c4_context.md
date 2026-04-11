# C4 Model — Level 1: System Context

## System Context Diagram

```text
┌─────────────────────────────────────────────────────────────────────┐
│                         Signal Editor System                       │
│  Desktop waveform editing workspace for engineering signal data    │
└───────────────┬───────────────────────────────┬────────────────────┘
                │                               │
        ┌───────v────────┐              ┌───────v──────────────────┐
        │ Engineer User  │              │ Local Signal Files       │
        │ edits signals  │              │ CSV / TSV / JSON / XML   │
        └────────────────┘              └──────────────────────────┘
                │
        ┌───────v────────┐
        │ Build & Test   │
        │ CMake + GTest  │
        └────────────────┘
```

## Actors and External Systems

| Actor / System | Role |
|----------------|------|
| Engineer user | Loads, inspects, edits, creates, and exports waveform data |
| Local filesystem | Supplies and receives supported signal files |
| Qt runtime | Hosts the desktop GUI and event loop |
| Build and test toolchain | Builds the application and verifies behavior |

## System Purpose

Signal Editor provides:

- a focused local desktop workflow for waveform editing
- visual and tabular editing paths over the same signal model
- explicit handling of interpolation and enumerated state mappings
- a maintainable implementation that separates domain logic from GUI concerns

## Constraints

- operates primarily as a local desktop application
- depends on Qt 6 for GUI delivery
- keeps network concerns out of the core editing model
- currently relies on engineering-friendly interchange formats rather than proprietary binary tool formats
