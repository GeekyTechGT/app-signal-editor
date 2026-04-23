# Naming Conventions

## Goals

Naming conventions in Signal Editor are intended to optimize:

- readability
- predictability
- consistency across module boundaries
- fast navigation in a mixed domain/UI/persistence codebase

## C++ Naming

| Construct | Convention | Example |
|-----------|------------|---------|
| Namespace | `snake_case` | `signal_editor` |
| Class / Struct | `PascalCase` | `Signal`, `SignalLibrary`, `SignalEditorService` |
| Interface / Port | `IPascalCase` | `ISignalRepository` |
| Function / Method | `snake_case` | `load_from()`, `set_interpolation()` |
| Variable / Parameter | `snake_case` | `signal_index`, `active_document_index` |
| Enum type | `PascalCase` | `InterpolationMode` |
| Enum value | `PascalCase` within enum | `InterpolationMode::Linear` |
| Private member | trailing underscore | `workspace_tabs_`, `plot_`, `repository_` |

## File Naming

| Artifact | Convention | Example |
|----------|------------|---------|
| Header | `snake_case.h` | `signal_editor_service.h` |
| Source | `snake_case.cpp` | `signal_plot_widget.cpp` |
| Test | `test_<subject>.cpp` | `test_signal.cpp` |
| Markdown document | lowercase words with underscores where appropriate | `architecture_overview.md` |

When adding a source file, also consider whether
[`docs/developer/source_file_guide.md`](../developer/source_file_guide.md)
needs a short entry. That guide is intentionally file-oriented and should stay
useful for new contributors.

## CMake Naming

| Construct | Convention | Example |
|-----------|------------|---------|
| Core library target | `<module>_core` | `signal_editor_core` |
| Qt adapter target | `<module>_qt` | `signal_editor_qt` |
| GUI target | `<module>_gui` | `signal_editor_gui` |
| Options | `SIGNAL_EDITOR_<OPTION>` | `SIGNAL_EDITOR_BUILD_GUI` |

## Documentation Naming

Documentation should use names that reflect the real repository and real module responsibilities.

Rules:

- avoid generic scaffold placeholders
- use repository-relative links where practical
- prefer titles that explain the document’s real role
- keep architecture docs aligned with actual containers and components
- keep public header comments aligned with the current implementation
- keep file-by-file explanations aligned with the real source tree

## Commenting Conventions

Code comments should follow the same precision rules as identifiers.

- write comments in professional English
- prefer Doxygen format for public headers, entry points, and non-obvious helpers
- describe responsibilities, ownership, invariants, and side effects
- avoid comments that merely paraphrase the next line of code
- delete stale comments when the implementation changes

## Naming Smells to Avoid

Avoid introducing names that:

- expose implementation details from another layer unnecessarily
- use generic terms like `Manager`, `Utils`, or `Helper` when a more precise concept exists
- describe old scaffold structure rather than the current Signal Editor product
- hide domain meaning behind UI-centric or persistence-centric wording
