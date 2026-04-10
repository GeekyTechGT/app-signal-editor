# Software Requirements Specification — Signal Editor

**Version:** 0.1.0
**Date:** 2026-04-10
**Status:** Draft
**Audience:** Developers, QA, Product

---

## 1. Introduction

### 1.1 Purpose

This document describes the requirements for **Signal Editor**, a desktop
application written in C++23 with a Qt 6 GUI that replicates the core
workflow of the [MATLAB Simulink Signal Editor](https://www.mathworks.com/help/simulink/slref/signaleditortool.html).
It allows users to load, visualize, edit, create and export time-series
signals stored as CSV files.

### 1.2 Scope

The first release covers:

* Loading multi-signal CSV files (one time column + N value columns).
* Drag-and-drop file import from the OS file manager.
* Tabular browsing of all loaded signals.
* Plotting of any selected signal as an interactive curve.
* Direct manipulation of the curve via mouse (move samples, add/remove
  waypoints, drag and reshape regions).
* Adding new signals from scratch and removing existing ones.
* Exporting the modified library back to CSV.

Out of scope for v0.1.0: undo/redo, FFT/spectral views, multi-signal
overlay editing, signal generation from mathematical expressions,
project files (`.mat`/`.sigedit`).

### 1.3 Definitions

| Term      | Definition |
|-----------|------------|
| Signal    | A named, time-ordered sequence of `(t, y)` samples. |
| Waypoint  | A user-controlled sample point. In v0.1 every sample is a waypoint. |
| Library   | The in-memory collection of all currently loaded signals. |
| Repository| An adapter that can persist or retrieve a signal library. |

---

## 2. Overall Description

### 2.1 Product Perspective

Signal Editor follows a **Hexagonal (Ports & Adapters) architecture**:

```
                +-----------------------------+
                |          apps/gui           |
                |   (thin wiring, main.cpp)   |
                +--------------+--------------+
                               |
                +--------------v--------------+
                |       adapters/qt           |   <- swappable UI
                |  MainWindow, PlotWidget,    |
                |  SignalListPanel            |
                +--------------+--------------+
                               |
                +--------------v--------------+
                |             api             |   <- public facade
                |    SignalEditorService      |
                +--------------+--------------+
                               |
                +--------------v--------------+
                |          core               |   <- pure C++
                |  domain + usecases          |
                |  (no Qt, no I/O)            |
                +--------------+--------------+
                               |
                +--------------v--------------+
                |            ports            |   <- abstract IO
                |  ISignalRepository          |
                +--------------+--------------+
                               |
                +--------------v--------------+
                |   adapters/filesystem       |   <- swappable IO
                |   CsvSignalRepository       |
                +-----------------------------+
```

The dependency rule is one-directional: `gui → api → usecases → ports
← adapters`. The core never depends on Qt, the filesystem, or any
specific framework. **The GUI framework can be swapped in the future**
(e.g. ImGui, wxWidgets) by replacing only the `adapters/qt/` layer.

### 2.2 User Classes

* **Engineer / Researcher** — primary user. Loads measured or simulated
  data, inspects waveforms, performs minor edits and re-exports.

### 2.3 Operating Environment

* Windows 10/11 (MinGW64) — primary target.
* Linux (GCC) — supported for the headless `core` library and tests.
* Qt 6.5 or newer.

---

## 3. Functional Requirements

| ID    | Requirement |
|-------|-------------|
| FR-1  | The system shall load a CSV file where the first column is the time vector and each subsequent column is a separate signal. |
| FR-2  | The first row of the CSV may be a header containing the signal names. If absent, signals are auto-named `signal_1 … signal_N`. |
| FR-3  | The system shall accept files via a *File → Open* menu, a toolbar action, and via drag-and-drop onto the main window. |
| FR-4  | The system shall display all loaded signals in a left-hand list panel showing their name and sample count. |
| FR-5  | When a signal is selected, the system shall display its waveform on the central plot area with auto-scaled axes. |
| FR-6  | The user shall be able to drag any sample point with the left mouse button to change its `y` value. |
| FR-7  | The user shall be able to insert a new sample point by double-clicking on the plot at the desired `(t, y)`. |
| FR-8  | The user shall be able to remove the nearest sample point with a right-click context action. |
| FR-9  | The user shall be able to apply a smooth Gaussian deformation centered at the cursor by holding `Shift` while dragging. |
| FR-10 | The user shall be able to add a new empty signal from scratch through *Signal → New*, specifying name, time range and number of samples. |
| FR-11 | The user shall be able to remove the selected signal through *Signal → Remove* or the `Delete` key. |
| FR-12 | The user shall be able to rename a signal in-place from the list panel. |
| FR-13 | The user shall be able to export the entire library back to a CSV file via *File → Export*. |
| FR-14 | The exported CSV shall be re-loadable by the same application without information loss. |
| FR-15 | The status bar shall display the current `(t, y)` under the cursor when hovering the plot. |
| FR-16 | All editing operations shall update the plot in real time. |

### 3.1 CSV format

```
time,sig1,sig2,sig3
0.0,0.00,1.00,0.50
0.1,0.10,0.99,0.49
...
```

* Delimiter: `,` (comma).
* Decimal separator: `.` (dot).
* Header row: optional but recommended. Detected when the first cell is
  non-numeric.
* Time column must be strictly monotonic increasing.

---

## 4. Non-Functional Requirements

| ID     | Requirement |
|--------|-------------|
| NFR-1  | The application shall start in under 1 s on typical desktop hardware. |
| NFR-2  | The application shall handle CSV files containing up to 100 000 samples per signal at interactive frame rates (≥ 30 fps redraw). |
| NFR-3  | The core library shall compile with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` with zero warnings. |
| NFR-4  | The core library shall have **zero dependency on Qt or the filesystem**, enabling unit-testable, framework-agnostic logic. |
| NFR-5  | The Qt adapter shall expose a stable interface so an alternative UI framework can replace it without changing the core. |
| NFR-6  | The application UI shall use a modern, dark-themed Fusion stylesheet. |
| NFR-7  | All public APIs shall follow SOLID, KISS and DRY principles. |
| NFR-8  | Unit tests shall cover all use cases and the CSV adapter, with ≥ 80 % line coverage of `signal_editor_core`. |

---

## 5. Domain Model

```
+----------------+         +-----------------+
| SignalLibrary  |◇------> |     Signal      |
+----------------+ 0..*    +-----------------+
                           | name : string   |
                           | samples : vec   |
                           +-------+---------+
                                   |
                                   v
                           +-----------------+
                           |  SamplePoint    |
                           +-----------------+
                           | t : double      |
                           | y : double      |
                           +-----------------+
```

### 5.1 Invariants

* `Signal::samples_` is always sorted by `t` in ascending order.
* No two samples share the same `t`. Insertion of a duplicate `t` is
  treated as an *update* of `y`.
* `Signal::name_` is non-empty.
* `SignalLibrary` indices are stable for the lifetime of an object until
  `add` / `remove` is called.

---

## 6. Use Cases

| Use case               | Inputs                                | Output         | Side effects |
|------------------------|---------------------------------------|----------------|--------------|
| `LoadLibrary`          | path                                  | `SignalLibrary`| reads disk   |
| `SaveLibrary`          | path, library                         | `Result`       | writes disk  |
| `CreateSignal`         | name, t_start, t_end, n_samples, y0   | `Signal`       | none         |
| `AddSignal`            | library, signal                       | index          | mutates lib  |
| `RemoveSignal`         | library, index                        | `Result`       | mutates lib  |
| `RenameSignal`         | library, index, new_name              | `Result`       | mutates lib  |
| `MoveSample`           | signal, index, new_y                  | `Result`       | mutates sig  |
| `InsertSample`         | signal, t, y                          | new index      | mutates sig  |
| `RemoveSample`         | signal, index                         | `Result`       | mutates sig  |
| `ApplyGaussianBrush`   | signal, t_center, delta_y, sigma      | `Result`       | mutates sig  |

---

## 7. External Interfaces

### 7.1 GUI

* Menus: `File`, `Signal`, `Help`.
* Toolbar: Open, Save, New Signal, Remove Signal.
* Left panel: signal list (rename in place, delete key supported).
* Center: plot widget.
* Status bar: cursor coordinates and library summary.

### 7.2 Filesystem

* Read/write CSV via `CsvSignalRepository` (implements
  `ISignalRepository`).

---

## 8. Quality & Verification

* **Unit tests** (GoogleTest):
  * `Signal` and `SignalLibrary` invariants.
  * Use cases (move/insert/remove/rename, gaussian brush).
  * `CsvSignalRepository` round-trip (load → save → load).
* **Manual smoke test** with a sample CSV shipped under
  `tests/01.data/sample_signals.csv`.

---

## 9. Future Work

* Undo / redo stack.
* Multi-signal overlay and arithmetic operations.
* Project files (`.sigedit`) bundling library + view state.
* Alternative GUI adapter (ImGui or web).
