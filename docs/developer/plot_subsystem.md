# Plot Subsystem

## Purpose

This document explains how the plot is implemented, which classes are involved,
and what design choices matter when modifying plot behavior.

## Main Classes Involved

### `SignalPlotWidget`

File:

- [`src/signal_editor/adapters/qt/signal_plot_widget.h`](../../src/signal_editor/adapters/qt/signal_plot_widget.h)

Responsibility:

- render plotted signals
- emphasize one active editable signal
- maintain the visible time window
- convert mouse gestures into semantic editing signals

### `MainWindow`

File:

- [`src/signal_editor/adapters/qt/main_window.h`](../../src/signal_editor/adapters/qt/main_window.h)

Responsibility:

- decide which signals are visible
- decide which signal is active
- bind those choices into the plot widget
- preserve time view across rebinding when appropriate

### `SignalEditorService`

File:

- [`src/signal_editor/core/usecases/signal_editor_service.h`](../../src/signal_editor/core/usecases/signal_editor_service.h)

Responsibility:

- apply the actual edit after the plot emits a semantic request

## Rendering Model

The plot receives:

- a pointer to the active `SignalLibrary`
- an active signal index
- a list of visible signal indices

From that it renders:

- all visible non-active signals as contextual background curves
- the active signal with stronger visual emphasis
- sample handles for both active and non-active signals
- overlay badges and cursor feedback
- raw polylines when the visible range contains a manageable number of samples
- min/max LOD envelopes when zoomed out over dense signals

The plot must also handle the case where:

- there is an active signal, but it is currently not visible in the plot
- there is no active signal at all

Those are valid workspace states and should not be treated as impossible.

## Why the Plot Emits Signals Instead of Editing Directly

`SignalPlotWidget` does not write into the domain model directly. It emits
semantic requests such as:

- `sampleMoveRequested`
- `sampleInsertRequested`
- `sampleRemoveRequested`
- `segmentMoveRequested`
- `gaussianBrushRequested`

This choice was intentional for three reasons:

1. Domain operations remain testable without Qt.
2. The same edit operations can be reused by the table workflow.
3. The main window can coordinate undo, document dirty state, and refresh order
   in one place.

## Zoom and View State

The visible time range belongs conceptually to the plot widget, but preserving
it across workspace rebinding is the responsibility of `MainWindow`.

That split is intentional:

- the widget knows how to store and apply a time range
- the controller knows when preserving that range is correct

If a feature changes when zoom should be preserved, modify the rebinding flow in
`MainWindow` before modifying low-level plot code.

## Dense-Signal Rendering

Dense signal rendering is delegated to
[`signal_lod_pyramid.h/.cpp`](../../src/signal_editor/adapters/qt/signal_lod_pyramid.h).
The plot widget owns the visual decision of when to draw raw samples versus
bucket envelopes, but the pyramid module owns the aggregation data structure.

The key rule is that zoomed-out views should draw approximately viewport-sized
work, not one primitive per original sample. This preserves responsiveness for
signals with millions of samples while still showing spikes through min/max
buckets.

## Editing Assumptions

The active signal is the only signal directly editable in the plot.

Visible non-active signals exist for:

- comparison
- context
- Y-axis scaling
- multi-signal understanding while editing one signal

This keeps the editing model understandable even when several curves are
visible.

## Relevant Risks

The plot has historically been sensitive to:

- re-entrant refresh while mouse events are still being processed
- sentinel indices for hovered/selected segments
- losing zoom during rebinding
- hidden-active-signal states
- invalidating LOD caches when the library or visible signal set changes
- accidentally drawing every raw sample when many samples fall into the same pixel

When modifying the plot, always think in terms of event ordering and controller
rebinding rather than only painting.
