# Level-of-Detail Rendering

## Purpose

Signal Editor can load signals with a very large number of samples. A common
stress case is a long acquisition with a very small sampling time, for example:

- duration: 30 minutes
- sampling time: 0.0001 seconds
- sample count: about 18 million points per signal

Drawing every point every time the user pans, zooms, changes tabs, or moves the
mouse is not viable. Even if the CPU can iterate over the points, the GUI thread
would still have to build and draw millions of primitives. That blocks the UI.

The plot solves this with level-of-detail rendering, usually abbreviated as
LOD. LOD means the application keeps more than one representation of the same
signal and chooses the cheapest representation that is still visually correct
for the current zoom level.

## The Basic Problem

A screen has a fixed number of horizontal pixels. A plot area might be 1,000 to
3,000 pixels wide. If the visible time range contains 5,000,000 samples, many
samples necessarily land on the same pixel column.

When multiple samples fall inside one pixel column, drawing all of them does not
add useful horizontal detail. The screen cannot show more than one column there.
What still matters is the vertical range inside that column:

- the minimum value
- the maximum value

This is why Signal Editor does not use an average-only downsample. An average
can hide narrow spikes, glitches, and impulses. A min/max envelope preserves
them because a spike remains visible as a vertical excursion.

## Min/Max Buckets

The LOD system groups neighboring samples into buckets. Each bucket stores only
the lowest and highest value observed in that group.

Example with eight raw samples:

```text
raw:    1.0  1.1  0.9  8.0  1.0  1.2  0.8  1.1
bucket: [1.0,1.1] [0.9,8.0] [1.0,1.2] [0.8,1.1]
```

If this signal is zoomed out, the bucket containing `8.0` still exposes that
peak through its max value. A mean would reduce it and might make it disappear.

## The Pyramid

Signal Editor uses a min/max pyramid. Each level halves the number of buckets
from the previous level.

```text
raw samples   : 1 sample per sample
LOD level 0   : 2 raw samples per bucket
LOD level 1   : 4 raw samples per bucket
LOD level 2   : 8 raw samples per bucket
LOD level 3   : 16 raw samples per bucket
...
```

The rule is:

```text
samplesPerBucket = 2^(level + 1)
```

The first explicit level represents two raw samples per bucket. Raw samples are
kept in the `Signal` domain object and are treated as the implicit full-detail
level.

## Why A Pyramid Is Efficient

The first LOD level costs about half as many buckets as the raw sample count.
The next costs half of that, and so on. Across all levels, the total number of
buckets is bounded by roughly the raw sample count.

For a 1,000,000-sample signal:

```text
level 0: 500,000 buckets
level 1: 250,000 buckets
level 2: 125,000 buckets
...
```

The memory is acceptable because a bucket stores two floats. The plot never
copies the raw samples into the LOD cache.

## Module Layout

The LOD implementation lives in:

- `src/signal_editor/adapters/qt/signal_lod_pyramid.h`
- `src/signal_editor/adapters/qt/signal_lod_pyramid.cpp`

The plot widget uses that module from:

- `src/signal_editor/adapters/qt/signal_plot_widget.cpp`

The separation is intentional:

- `SignalLodCache` owns and invalidates pyramids.
- `SignalPlotWidget` maps data to pixels and draws Qt primitives.
- The domain `Signal` remains the owner of the exact original samples.

## Main Data Types

`MinMaxBucket` stores the vertical envelope for one bucket:

```cpp
struct MinMaxBucket {
    float min_value;
    float max_value;
};
```

`LodLevel` stores buckets with the same compression ratio:

```cpp
struct LodLevel {
    std::size_t samples_per_bucket;
    std::vector<MinMaxBucket> buckets;
};
```

`SignalLodPyramid` stores all levels for one signal:

```cpp
struct SignalLodPyramid {
    const Signal* signal;
    std::size_t sample_count;
    double first_t;
    double last_t;
    std::vector<LodLevel> levels;
};
```

The cache key is the signal address plus simple structural metadata. When the
plot edits a signal, it invalidates that signal in the cache.

## Choosing A Level

The plot first determines how many samples are visible:

```text
visibleSamples = samples inside [view_t_min, view_t_max]
```

Then it estimates samples per pixel:

```text
samplesPerPixel = visibleSamples / plotWidthInPixels
```

If the visible range is small enough, the plot uses raw samples and draws a
normal polyline. This is the high-detail mode.

If the visible range is large, the plot chooses a LOD level whose
`samples_per_bucket` is close to the visible samples per pixel. This keeps the
number of drawn primitives near the width of the viewport instead of the number
of samples.

## Rendering Modes

### Raw Mode

Raw mode is used when the visible sample count is small.

The plot:

1. Finds the visible sample range by binary search.
2. Builds a `QPainterPath` only for that range.
3. Draws the exact polyline.
4. Draws handles only when the visible sample count is small enough.

This keeps close zoom interactive and exact.

### LOD Mode

LOD mode is used when too many samples are visible.

The plot:

1. Selects a min/max LOD level.
2. Iterates only the buckets that intersect the visible time window.
3. Draws a vertical line from bucket min to bucket max.
4. Skips per-sample handles because they would not be meaningful at that zoom.

This keeps zoomed-out views responsive and preserves spikes.

## Y-Axis Range Calculation

Rendering is not the only expensive operation. Autoscaling the Y axis can also
freeze the UI if it scans millions of samples on every zoom.

For that reason the Y range calculation also uses the LOD module. When the
visible range is large, it selects a range-oriented LOD level and accumulates
min/max values from buckets instead of scanning every raw sample.

The result is still conservative: the visible Y range includes the bucket
envelope, so spikes are preserved.

## Interaction Safeguards

Large signals can freeze the UI outside paint code too. The plot limits these
paths:

- handle hit-testing searches only a narrow time range near the cursor
- segment hit-testing is disabled when too many segments are visible
- sample handles are hidden when too many samples are visible
- the table panel materializes only a bounded preview for very large signals

These safeguards keep mouse movement, zooming, and tab switching from falling
back to O(number of samples) behavior.

## Cache Invalidation

The cache is cleared or invalidated in these cases:

- a different library or signal is bound to the plot
- the plot refreshes after external edits
- a sample is moved, inserted, removed, or offset
- a segment move or Gaussian brush changes sample values
- a file-wide offset changes all plotted samples

Correct invalidation is important. A stale LOD pyramid would draw old min/max
values even though the raw signal changed.

## Example Dataset

A reproducible dense CSV generator is available at:

```text
scripts/generate_lod_sample.py
```

Default output:

```text
tests/01.data/generated/sample_lod_dense_120s_dt_100us.csv
```

Default parameters:

- duration: 120 seconds
- sampling time: 0.0001 seconds
- samples per signal: 1,200,001
- signals: one dense waveform and one spike-heavy waveform

Run it with:

```bash
python3 scripts/generate_lod_sample.py
```

The generated CSV is intentionally ignored by Git because it is large. It is for
manual stress testing, not for source control.

## Automated Tests

LOD behavior is covered by:

```text
tests/03.unit_test/signal_editor/test_signal_lod_pyramid.cpp
```

The tests verify that:

- pyramid levels use power-of-two sample grouping
- the coarsest level still preserves narrow positive and negative spikes
- raw, medium, and coarse rendering levels are selected as zoom changes
- large visible range calculations choose a bounded aggregation level

Run only the LOD tests with:

```bat
build\windows-mingw64-debug\bin\test_signal_editor.exe --gtest_filter=SignalLodPyramidTest.*
```

When running from Windows, ensure the MinGW runtime is on `PATH`.
