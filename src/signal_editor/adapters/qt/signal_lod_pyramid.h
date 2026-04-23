#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace signal_editor {
class Signal;
}

namespace signal_editor::adapters::qt {

/**
 * @file
 * @brief Min/max level-of-detail cache used by the Qt plot adapter.
 */

/** @brief Min/max envelope for a contiguous sample bucket. */
struct MinMaxBucket {
    float min_value{0.0F};
    float max_value{0.0F};
};

/** @brief One min/max pyramid level with a fixed original-samples-per-bucket ratio. */
struct LodLevel {
    std::size_t samples_per_bucket{1};
    std::vector<MinMaxBucket> buckets;
};

/** @brief Cached min/max pyramid for one signal instance. */
struct SignalLodPyramid {
    const Signal* signal{nullptr};
    std::size_t sample_count{0};
    double first_t{0.0};
    double last_t{0.0};
    std::vector<LodLevel> levels;
};

/**
 * @brief Lazily builds and owns LOD pyramids for signal instances.
 *
 * The cache is intentionally keyed by signal address plus cheap structural
 * metadata. Editing code must invalidate the changed signal when values mutate;
 * rebinding a document can clear the whole cache.
 */
class SignalLodCache {
public:
    /** @brief Returns an up-to-date min/max pyramid for `signal`. */
    [[nodiscard]] const SignalLodPyramid& pyramid_for(const Signal& signal) const;

    /** @brief Drops the cached pyramid for one signal instance. */
    void invalidate(const Signal* signal) const;

    /** @brief Drops every cached pyramid. */
    void clear() const;

private:
    mutable std::vector<SignalLodPyramid> pyramids_;
};

/** @brief Returns the sample half-open range that intersects a visible time window. */
[[nodiscard]] std::pair<std::size_t, std::size_t> visible_sample_range(
    const Signal& signal,
    double t_min,
    double t_max);

/** @brief Returns the number of samples intersecting a visible time window. */
[[nodiscard]] std::size_t visible_sample_count(const Signal& signal,
                                               double t_min,
                                               double t_max);

/** @brief Returns the time span covered by one LOD bucket. */
[[nodiscard]] std::pair<double, double> bucket_time_range(const Signal& signal,
                                                          const LodLevel& level,
                                                          std::size_t bucket_index);

/** @brief Selects a trace-rendering LOD level for the current zoom and viewport width. */
[[nodiscard]] const LodLevel* choose_lod_level(const SignalLodPyramid& pyramid,
                                               std::size_t visible_count,
                                               double content_width,
                                               std::size_t max_raw_trace_points);

/** @brief Selects a LOD level that keeps range aggregation bounded. */
[[nodiscard]] const LodLevel* choose_range_lod_level(const SignalLodPyramid& pyramid,
                                                     std::size_t visible_count,
                                                     std::size_t max_raw_scan_points,
                                                     double target_bucket_count);

}  // namespace signal_editor::adapters::qt
