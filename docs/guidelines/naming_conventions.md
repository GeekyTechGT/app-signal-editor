# Naming Conventions

## C++ Naming

| Construct | Convention | Example |
|-----------|------------|---------|
| Namespace | `snake_case` | `myprj::signal_editor` |
| Class / Struct | `PascalCase` | `Signal`, `SignalLibrary`, `CsvSignalRepository` |
| Interface / Port | `IPascalCase` | `ISignalRepository` |
| Function / Method | `snake_case` | `load_from()`, `apply_gaussian_brush()` |
| Variable / Parameter | `snake_case` | `signal_index`, `destination_path` |
| Enum value | `PascalCase` inside enum | `InterpolationMode::Linear` |
| Private member | trailing underscore | `repository_`, `workspace_title_label_` |

## File Naming

| Artifact | Convention | Example |
|----------|------------|---------|
| Header | `snake_case.h` | `signal_editor_service.h` |
| Implementation | `snake_case.cpp` | `signal_plot_widget.cpp` |
| Test | `test_<subject>.cpp` | `test_signal_editor_service.cpp` |

## CMake Naming

| Construct | Convention | Example |
|-----------|------------|---------|
| Core library target | `myprj_<module>_core` | `myprj_signal_editor_core` |
| GUI target | `myprj_<module>_gui` | `myprj_signal_editor_gui` |
| Qt adapter library | `myprj_<module>_qt` | `myprj_signal_editor_qt` |
| Options | `MYPRJ_<OPTION>` | `MYPRJ_BUILD_GUI` |
| Helper functions | `myprj_<verb>_<noun>` | `myprj_configure_target` |

## Documentation Naming

- Product-facing docs use clear titles and repository-relative links.
- Architecture docs describe actual containers and components, not template placeholders.
- Public headers that define module boundaries should include Doxygen comments.
