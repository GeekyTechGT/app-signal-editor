# Hexagonal Architecture — Skeleton Structure

## Objective

This document defines the canonical folder structure for modules in this repository. Every module follows the same pattern so developers can navigate any module with the same mental model.

## Folder Layout (per module)

```
src/<module>/
├── core/
│   ├── domain/        ← Pure entities and value objects (no I/O, no frameworks)
│   └── usecases/      ← Business logic orchestration (depends only on ports)
├── ports/             ← Abstract interfaces (pure virtual C++ classes)
├── adapters/
│   ├── cli/           ← argv parsing, exit-code mapping
│   ├── filesystem/    ← File read/write implementations of ports
│   ├── json/          ← JSON configuration reading
│   └── gui/           ← Qt widget logic (no business logic)
├── api/               ← Public module facade used by apps/
└── schema/            ← JSON schemas for config validation

apps/<module>/
├── cli/
│   ├── CMakeLists.txt
│   └── main.cpp       ← Composition root: wire adapters → ports → api
├── gui/
│   ├── CMakeLists.txt
│   └── main.cpp       ← Composition root for Qt app
└── docs/
    └── ABOUT.md       ← User-facing module documentation
```

## Placement Rules

### What belongs in `core/` (inside the hexagon)

✅ Entities, value objects, domain invariants
✅ Use case orchestration
✅ Error model and domain-specific exceptions
✅ Pure algorithmic logic with no I/O

❌ Qt, filesystem, JSON library dependencies
❌ OS-specific code
❌ `main()`

### What belongs in `adapters/` (outside the hexagon)

✅ Filesystem read/write
✅ GUI: Qt widgets, event handling, binding
✅ CLI: argv parsing, help text, exit codes
✅ JSON: loading and validating config files

### What belongs in `apps/`

✅ ONLY the composition root:
- `main.cpp`
- wiring: create adapters, inject into use cases via ports
- zero business logic

> Rule: `apps/*/main.cpp` should be short. If it grows beyond 50 lines of logic, extract a factory or builder.

## Dependency Rule

```
apps/ → api/ → usecases/ → ports/ ← adapters/
                ↓
            domain/
```

Arrows point from dependents to dependencies. The domain and use cases have **no outbound arrows** to adapters or frameworks.

## Adding a New Module

1. Create `src/<new_module>/` with the above sub-folders.
2. Add `CMakeLists.txt` for the library target.
3. Create `apps/<new_module>/cli/` and/or `apps/<new_module>/gui/` with `main.cpp`.
4. Add `tests/03.unit_test/<new_module>/CMakeLists.txt`.
5. Register subdirectories in the root `CMakeLists.txt`.
6. Update `README.md` module inventory table.
