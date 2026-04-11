# Signal Editor

Signal Editor is a desktop waveform editing application written in C++23 with a Qt 6 GUI and a hexagonal architecture. It is designed for engineers who need to inspect, shape, and export time-series data stored in CSV files without mixing UI concerns into the domain layer.

## Highlights

- Multi-file workspace with fast switching between loaded CSV documents
- Interactive plot editing with waypoint drag, add/remove, and Gaussian brushing
- Tabular sample editing for precise numeric adjustments
- Signal creation from common waveform templates
- Per-signal interpolation modes persisted in CSV metadata
- Undo support scoped to the active workspace document
- Clean separation between domain, use cases, ports, and adapters

## Repository Layout

```text
signal-editor/
├── apps/signal_editor/gui/              # GUI entry point and composition root
├── cmake/                               # Shared CMake helpers and build options
├── docs/                                # Product, requirements, and architecture docs
├── include/myprj/                       # Generated/public version headers
├── scripts/                             # Build, test, deploy, and project manager helpers
├── src/common/                          # Shared cross-module types
├── src/signal_editor/                   # Signal Editor domain, use cases, adapters, API
├── tests/                               # Unit tests and reusable test data
├── CMakeLists.txt                       # Top-level build configuration
└── CMakePresets.json                    # Windows and Linux configure/build/test presets
```

## Architecture Summary

The implementation follows a ports-and-adapters style:

- `core/domain/`: waveform entities and invariants
- `core/usecases/`: orchestration of load, save, edit, and generation flows
- `ports/`: persistence abstraction (`ISignalRepository`)
- `adapters/filesystem/`: CSV repository
- `adapters/qt/`: Qt widgets, dialogs, and interactive editing surface
- `api/`: compact facade for app composition roots

Dependency rule: `apps -> api -> usecases -> ports <- adapters`

## Supported CSV Format

Signal Editor works with CSV files where the first column is the shared time axis and each following column is a signal:

```csv
time,throttle,brake,steer
0.0,0.0,0.0,0.0
0.1,0.2,0.0,0.1
0.2,0.4,0.0,0.2
```

When saving, the application also emits an interpolation metadata row before the header so that `linear` versus `step` rendering survives a round-trip.

## Build

### Windows GUI

```bash
cmake --preset windows-mingw64-debug
cmake --build --preset windows-mingw64-debug
```

Release:

```bash
cmake --preset windows-mingw64-release
cmake --build --preset windows-mingw64-release
```

### Linux Core + Tests

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
```

Release:

```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

## Test

```bash
ctest --preset linux-gcc-debug --output-on-failure
```

On Windows, use the matching `windows-mingw64-*` preset.

## Runtime Notes

- GUI builds require Qt 6 Widgets and are enabled by `MYPRJ_BUILD_GUI=ON`.
- Linux presets currently target the core library and tests with GUI disabled.
- Files can be opened from the menu or via drag and drop onto the main window.

## Workflow Expectations

A complete change should include:

- implementation
- tests
- user-facing documentation updates when behavior changes
- architecture or requirements updates when design intent changes
- changelog updates for visible or workflow-relevant changes

## Related Documentation

- [Product vision](docs/product/vision.md)
- [Product requirements](docs/product/prd.md)
- [Software requirements specification](docs/specs/srs.md)
- [Architecture overview](docs/architecture/architecture_overview.md)
- [Contributing guide](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
