# Hexagonal Structure for Signal Editor

## Objective

This document explains how the Signal Editor module is structured inside the repository and what belongs in each area. It serves as a navigation guide and as a guardrail for future changes.

## Module Layout

```text
src/signal_editor/
├── core/
│   ├── domain/        # Pure waveform entities, invariants, and editing rules
│   └── usecases/      # Application workflows expressed against ports
├── ports/             # Repository abstractions consumed by the use-case layer
├── adapters/
│   ├── filesystem/    # per-format file persistence adapters plus shared tabular row mapping
│   └── qt/            # Qt widgets, dialogs, LOD rendering helpers, and workspace shell
├── api/               # Public facade used by the GUI application
└── schema/            # Structured validation assets when applicable

apps/signal_editor/gui/
└── main.cpp           # Composition root for the Qt application
```

## Placement Rules

### What belongs inside the hexagon

Inside `core/` you should place:

- entities and value objects
- invariants for sample ordering and signal naming
- interpolation semantics
- enumerated label/value rules
- editing primitives
- workflow orchestration that depends only on ports

### What belongs outside the hexagon

Inside `adapters/` you should place:

- Qt widgets and signal-slot wiring
- file parsing and serialization
- extension-based repository dispatch
- format-specific metadata handling
- rendering and interaction logic
- worker-thread UI orchestration for long-running desktop operations
- user-facing diagnostics derived from adapter/parser errors

### What belongs in `apps/`

The application entry point should remain small and focused on composition:

- create the Qt application
- build concrete adapters
- inject them into the core-facing API/service
- start the event loop

## Dependency Rule

```text
apps -> api -> usecases -> ports <- adapters
                |
                -> domain
```

The most important architectural prohibition is this: the domain must never depend on adapters or frameworks.

## Practical Interpretation

If you are unsure where logic belongs, use this test:

- if it talks about what a signal means, it probably belongs in the domain
- if it talks about how a signal is loaded or saved, it probably belongs in a filesystem adapter
- if it talks about how a user edits or sees a signal, it probably belongs in a Qt adapter
- if it coordinates operations between those pieces without becoming UI-specific, it probably belongs in a use case
