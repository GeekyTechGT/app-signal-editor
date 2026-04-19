#pragma once

#include "signal_editor/core/domain/sample_point.h"

#include <cstddef>
#include <string>
#include <vector>

namespace signal_editor {

/**
 * @brief Domain entity representing a named, time-ordered waveform.
 *
 * The class owns the sample collection and enforces the following invariants
 * through every mutation path:
 * - sample timestamps remain strictly ordered;
 * - duplicate timestamps collapse into a single sample;
 * - the signal name is never empty.
 */
class Signal {
public:
    /**
     * @brief Interpolation strategy used for visualisation and export.
     */
    enum class InterpolationMode { Linear = 0, Step };

    /**
     * @brief User-defined enumerated state associated with a numeric value.
     */
    struct EnumerationEntry {
        std::string label;
        double value{0.0};
    };

    /**
     * @brief Builds a signal from a name and explicit sample collection.
     * @param name Human-readable signal identifier.
     * @param samples Sample sequence to take ownership of.
     * @param interpolation Rendering/export interpolation strategy.
     */
    Signal(std::string name,
           std::vector<SamplePoint> samples,
           InterpolationMode interpolation = InterpolationMode::Linear);

    /**
     * @brief Builds a signal from parallel time and value vectors.
     * @param name Human-readable signal identifier.
     * @param time Sample timestamps.
     * @param values Sample values.
     * @param interpolation Rendering/export interpolation strategy.
     * @return Newly constructed signal.
     */
    static Signal from_vectors(std::string name,
                               const std::vector<double>& time,
                               const std::vector<double>& values,
                               InterpolationMode interpolation = InterpolationMode::Linear);

    /**
     * @brief Builds a uniformly sampled flat signal.
     * @param name Human-readable signal identifier.
     * @param t_start Inclusive first timestamp.
     * @param t_end Inclusive final timestamp.
     * @param num_samples Number of generated points, must be at least 2.
     * @param initial_value Value assigned to every generated point.
     * @param interpolation Rendering/export interpolation strategy.
     * @return Newly constructed signal.
     */
    static Signal create_uniform(std::string name,
                                 double t_start,
                                 double t_end,
                                 std::size_t num_samples,
                                 double initial_value = 0.0,
                                 InterpolationMode interpolation = InterpolationMode::Linear);

    /** @brief Returns the signal name. */
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /** @brief Returns the interpolation mode. */
    [[nodiscard]] InterpolationMode interpolation() const noexcept { return interpolation_; }

    /** @brief Returns the ordered sample collection. */
    [[nodiscard]] const std::vector<SamplePoint>& samples() const noexcept { return samples_; }

    /** @brief Returns the configured enumeration mapping. */
    [[nodiscard]] const std::vector<EnumerationEntry>& enumeration() const noexcept { return enumeration_; }

    /** @brief Returns the number of samples in the signal. */
    [[nodiscard]] std::size_t size() const noexcept { return samples_.size(); }

    /** @brief Reports whether the signal contains no samples. */
    [[nodiscard]] bool empty() const noexcept { return samples_.empty(); }

    /** @brief Reports whether the signal uses enumerated states. */
    [[nodiscard]] bool is_enumerated() const noexcept { return !enumeration_.empty(); }

    /** @brief Returns the smallest timestamp or `0.0` when empty. */
    [[nodiscard]] double t_min() const noexcept;

    /** @brief Returns the largest timestamp or `0.0` when empty. */
    [[nodiscard]] double t_max() const noexcept;

    /** @brief Returns the smallest sample value or `0.0` when empty. */
    [[nodiscard]] double y_min() const noexcept;

    /** @brief Returns the largest sample value or `0.0` when empty. */
    [[nodiscard]] double y_max() const noexcept;

    /**
     * @brief Interpolates the signal at an arbitrary timestamp.
     * @param t Query timestamp.
     * @return Interpolated value, clamped outside the sampled range.
     */
    [[nodiscard]] double interpolate(double t) const noexcept;

    /**
     * @brief Replaces the human-readable signal name.
     * @param name Non-empty new name.
     */
    void set_name(std::string name);

    /**
     * @brief Updates the interpolation mode.
     * @param interpolation New interpolation strategy.
     */
    void set_interpolation(InterpolationMode interpolation) noexcept;

    /**
     * @brief Replaces the enumeration mapping used by the signal.
     * @param enumeration Ordered label/value pairs defined by the user.
     */
    void set_enumeration(std::vector<EnumerationEntry> enumeration);

    /**
     * @brief Removes any enumeration mapping from the signal.
     */
    void clear_enumeration() noexcept;

    /**
     * @brief Resolves a numeric sample value back to its label.
     * @param y Numeric value stored in the signal.
     * @return Matching label when available, or an empty string.
     */
    [[nodiscard]] std::string label_for_value(double y) const;

    /**
     * @brief Resolves a label to its numeric value.
     * @param label User-visible enumerated state label.
     * @return Numeric value associated with the label.
     * @throws std::invalid_argument When the label is unknown.
     */
    [[nodiscard]] double value_for_label(const std::string& label) const;

    /**
     * @brief Returns the nearest valid enum value for the given numeric input.
     * @param y Raw numeric input.
     * @return The closest mapped enumeration value, or `y` for non-enum signals.
     */
    [[nodiscard]] double snap_to_enumeration(double y) const noexcept;

    /**
     * @brief Updates the value of an existing sample.
     * @param index Zero-based sample index.
     * @param new_y Replacement sample value.
     */
    void set_sample_value(std::size_t index, double new_y);

    /**
     * @brief Moves a sample and keeps time ordering intact.
     * @param index Zero-based sample index.
     * @param new_t New timestamp.
     * @param new_y New sample value.
     * @return Index assigned to the moved sample after reordering.
     */
    std::size_t move_sample(std::size_t index, double new_t, double new_y);

    /**
     * @brief Inserts or merges a sample at the given timestamp.
     * @param t Timestamp to insert.
     * @param y Sample value to insert.
     * @return Index of the inserted or updated sample.
     */
    std::size_t insert_sample(double t, double y);

    /**
     * @brief Removes a sample from the signal.
     * @param index Zero-based sample index.
     */
    void remove_sample(std::size_t index);

    /**
     * @brief Applies a smooth Gaussian deformation to all samples.
     * @param t_center Centre timestamp of the brush.
     * @param delta_y Signed amplitude applied by the brush.
     * @param sigma Standard deviation controlling brush width.
     */
    void apply_gaussian_brush(double t_center, double delta_y, double sigma);

    /**
     * @brief Finds the closest sample by timestamp.
     * @param t_query Query timestamp.
     * @return Nearest sample index or `SIZE_MAX` when the signal is empty.
     */
    [[nodiscard]] std::size_t nearest_index(double t_query) const noexcept;

private:
    std::string name_;
    InterpolationMode interpolation_{InterpolationMode::Linear};
    std::vector<SamplePoint> samples_;
    std::vector<EnumerationEntry> enumeration_;

    static void validate_name(const std::string& name);
    static void validate_enumeration(const std::vector<EnumerationEntry>& enumeration);
};

}  // namespace signal_editor
