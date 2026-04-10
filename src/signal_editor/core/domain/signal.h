#pragma once

#include "signal_editor/core/domain/sample_point.h"

#include <cstddef>
#include <string>
#include <vector>

namespace myprj::signal_editor {

// --- Domain entity ----------------------------------------------------------
// A named, time-ordered collection of (t, y) samples.
//
// Invariants enforced by all mutators:
//   * `samples_` is sorted strictly ascending by `t`.
//   * `name_` is non-empty.
//
// No I/O, no framework dependency, no virtual functions — pure C++23.
class Signal {
public:
    // --- Construction ---------------------------------------------------
    Signal(std::string name, std::vector<SamplePoint> samples);

    static Signal from_vectors(std::string name,
                               const std::vector<double>& time,
                               const std::vector<double>& values);

    // Build a flat (constant `initial_value`) signal sampled uniformly on
    // [t_start, t_end] with `num_samples` points (>= 2).
    static Signal create_uniform(std::string name,
                                 double t_start,
                                 double t_end,
                                 std::size_t num_samples,
                                 double initial_value = 0.0);

    // --- Read access ----------------------------------------------------
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<SamplePoint>& samples() const noexcept { return samples_; }
    [[nodiscard]] std::size_t size() const noexcept { return samples_.size(); }
    [[nodiscard]] bool empty() const noexcept { return samples_.empty(); }

    [[nodiscard]] double t_min() const noexcept;
    [[nodiscard]] double t_max() const noexcept;
    [[nodiscard]] double y_min() const noexcept;
    [[nodiscard]] double y_max() const noexcept;

    // Linear interpolation; clamps outside [t_min, t_max].
    [[nodiscard]] double interpolate(double t) const noexcept;

    // --- Editing --------------------------------------------------------
    void set_name(std::string name);

    // Update the y of the sample at `index`. Throws std::out_of_range.
    void set_sample_value(std::size_t index, double new_y);

    // Insert a new sample, keeping the time order. If a sample already
    // exists at the same `t` (within tolerance), its `y` is updated and
    // the existing index is returned.
    std::size_t insert_sample(double t, double y);

    // Remove the sample at `index`. Throws std::out_of_range.
    void remove_sample(std::size_t index);

    // Apply a smooth Gaussian deformation centered at `t_center`:
    //   y_i += delta_y * exp(-((t_i - t_center)^2) / (2*sigma^2))
    // for every existing sample. `sigma` must be > 0.
    void apply_gaussian_brush(double t_center, double delta_y, double sigma);

    // Find the index of the sample whose `t` is closest to `t_query`.
    // Returns SIZE_MAX if the signal is empty.
    [[nodiscard]] std::size_t nearest_index(double t_query) const noexcept;

private:
    std::string name_;
    std::vector<SamplePoint> samples_;  // sorted by t

    static void validate_name(const std::string& name);
};

}  // namespace myprj::signal_editor
