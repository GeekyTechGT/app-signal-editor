# C4 Model — Level 1: System Context

## System Context Diagram

```text
┌──────────────────────────────────────────────────────────────┐
│                    Signal Editor System                     │
│  Desktop waveform editing tool for CSV-based signals        │
└───────────────┬───────────────────────────────┬──────────────┘
                │                               │
        ┌───────v────────┐              ┌───────v────────┐
        │ Engineer User  │              │ Local CSV Data │
        │ edits signals  │              │ inputs/outputs │
        └────────────────┘              └────────────────┘
                │
        ┌───────v────────┐
        │ Build & Test   │
        │ CMake + GTest  │
        └────────────────┘
```

## Actors and External Systems

| Actor / System | Role |
|----------------|------|
| Engineer user | Loads, edits, creates, and exports waveform data |
| Local filesystem | Provides CSV inputs and receives exports |
| Qt runtime | Hosts the graphical desktop interface |
| Build and test toolchain | Builds the application and verifies behavior |

## System Purpose

Signal Editor provides:

- a focused desktop workflow for CSV-based waveform editing
- direct manipulation and precise tabular editing of signal samples
- a maintainable code structure that separates editing logic from the GUI

## Constraints

- Works primarily as a local desktop application
- Depends on Qt 6 for the GUI build
- Keeps network concerns out of the core editing model
