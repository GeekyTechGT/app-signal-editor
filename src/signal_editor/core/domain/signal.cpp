#include "signal_editor/core/domain/signal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace myprj::signal_editor {

namespace {
constexpr double kTimeEpsilon = 1e-12;
constexpr double kEnumValueEpsilon = 1e-9;

void sort_and_dedupe(std::vector<SamplePoint>& samples) {
    std::sort(samples.begin(), samples.end(),
              [](const SamplePoint& a, const SamplePoint& b) { return a.t < b.t; });

    auto last = std::unique(samples.begin(), samples.end(),
                            [](const SamplePoint& a, const SamplePoint& b) {
                                return std::fabs(a.t - b.t) < kTimeEpsilon;
                            });
    samples.erase(last, samples.end());
}
}  // namespace

void Signal::validate_name(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Signal name must not be empty");
    }
}

void Signal::validate_enumeration(const std::vector<EnumerationEntry>& enumeration) {
    std::vector<std::string> labels;
    std::vector<double> values;
    labels.reserve(enumeration.size());
    values.reserve(enumeration.size());

    for (const auto& entry : enumeration) {
        if (entry.label.empty()) {
            throw std::invalid_argument("Enumerated labels must not be empty");
        }
        if (std::find(labels.begin(), labels.end(), entry.label) != labels.end()) {
            throw std::invalid_argument("Duplicate enumerated label: " + entry.label);
        }
        const auto value_it = std::find_if(values.begin(), values.end(),
                                           [&](double candidate) {
                                               return std::fabs(candidate - entry.value) < kEnumValueEpsilon;
                                           });
        if (value_it != values.end()) {
            throw std::invalid_argument("Duplicate enumerated numeric value");
        }
        labels.push_back(entry.label);
        values.push_back(entry.value);
    }
}

Signal::Signal(std::string name,
               std::vector<SamplePoint> samples,
               InterpolationMode interpolation)
    : name_(std::move(name)), interpolation_(interpolation), samples_(std::move(samples)) {
    validate_name(name_);
    sort_and_dedupe(samples_);
}

Signal Signal::from_vectors(std::string name,
                            const std::vector<double>& time,
                            const std::vector<double>& values,
                            InterpolationMode interpolation) {
    if (time.size() != values.size()) {
        throw std::invalid_argument("time and values must have the same length");
    }
    std::vector<SamplePoint> samples;
    samples.reserve(time.size());
    for (std::size_t i = 0; i < time.size(); ++i) {
        samples.push_back({time[i], values[i]});
    }
    return Signal(std::move(name), std::move(samples), interpolation);
}

Signal Signal::create_uniform(std::string name,
                              double t_start,
                              double t_end,
                              std::size_t num_samples,
                              double initial_value,
                              InterpolationMode interpolation) {
    if (num_samples < 2) {
        throw std::invalid_argument("num_samples must be >= 2");
    }
    if (!(t_end > t_start)) {
        throw std::invalid_argument("t_end must be greater than t_start");
    }

    std::vector<SamplePoint> samples;
    samples.reserve(num_samples);
    const double dt = (t_end - t_start) / static_cast<double>(num_samples - 1);
    for (std::size_t i = 0; i < num_samples; ++i) {
        samples.push_back({t_start + dt * static_cast<double>(i), initial_value});
    }
    return Signal(std::move(name), std::move(samples), interpolation);
}

double Signal::t_min() const noexcept {
    return samples_.empty() ? 0.0 : samples_.front().t;
}

double Signal::t_max() const noexcept {
    return samples_.empty() ? 0.0 : samples_.back().t;
}

double Signal::y_min() const noexcept {
    if (samples_.empty()) {
        return 0.0;
    }
    double minimum = samples_.front().y;
    for (const auto& sample : samples_) {
        minimum = std::min(minimum, sample.y);
    }
    return minimum;
}

double Signal::y_max() const noexcept {
    if (samples_.empty()) {
        return 0.0;
    }
    double maximum = samples_.front().y;
    for (const auto& sample : samples_) {
        maximum = std::max(maximum, sample.y);
    }
    return maximum;
}

double Signal::interpolate(double t) const noexcept {
    if (samples_.empty()) {
        return 0.0;
    }
    if (t <= samples_.front().t) {
        return samples_.front().y;
    }
    if (t >= samples_.back().t) {
        return samples_.back().y;
    }

    auto it = std::upper_bound(samples_.begin(), samples_.end(), t,
                               [](double value, const SamplePoint& sample) { return value < sample.t; });
    const auto& hi = *it;
    const auto& lo = *std::prev(it);

    if (interpolation_ == InterpolationMode::Step || is_enumerated()) {
        return lo.y;
    }

    const double dt = hi.t - lo.t;
    if (dt <= 0.0) {
        return lo.y;
    }
    const double alpha = (t - lo.t) / dt;
    return lo.y + alpha * (hi.y - lo.y);
}

void Signal::set_name(std::string name) {
    validate_name(name);
    name_ = std::move(name);
}

void Signal::set_interpolation(InterpolationMode interpolation) noexcept {
    interpolation_ = is_enumerated() ? InterpolationMode::Step : interpolation;
}

void Signal::set_enumeration(std::vector<EnumerationEntry> enumeration) {
    validate_enumeration(enumeration);
    enumeration_ = std::move(enumeration);
    interpolation_ = InterpolationMode::Step;
    for (auto& sample : samples_) {
        sample.y = snap_to_enumeration(sample.y);
    }
}

void Signal::clear_enumeration() noexcept {
    enumeration_.clear();
}

std::string Signal::label_for_value(double y) const {
    for (const auto& entry : enumeration_) {
        if (std::fabs(entry.value - y) < kEnumValueEpsilon) {
            return entry.label;
        }
    }
    return {};
}

double Signal::value_for_label(const std::string& label) const {
    const auto it = std::find_if(enumeration_.begin(), enumeration_.end(),
                                 [&](const EnumerationEntry& entry) { return entry.label == label; });
    if (it == enumeration_.end()) {
        throw std::invalid_argument("Unknown enumerated label: " + label);
    }
    return it->value;
}

double Signal::snap_to_enumeration(double y) const noexcept {
    if (!is_enumerated()) {
        return y;
    }

    const auto best = std::min_element(enumeration_.begin(), enumeration_.end(),
                                       [&](const EnumerationEntry& lhs, const EnumerationEntry& rhs) {
                                           return std::fabs(lhs.value - y) < std::fabs(rhs.value - y);
                                       });
    return best == enumeration_.end() ? y : best->value;
}

void Signal::set_sample_value(std::size_t index, double new_y) {
    if (index >= samples_.size()) {
        throw std::out_of_range("sample index out of range");
    }
    samples_[index].y = snap_to_enumeration(new_y);
}

std::size_t Signal::move_sample(std::size_t index, double new_t, double new_y) {
    if (index >= samples_.size()) {
        throw std::out_of_range("sample index out of range");
    }

    samples_.erase(samples_.begin() + static_cast<std::ptrdiff_t>(index));
    return insert_sample(new_t, new_y);
}

std::size_t Signal::insert_sample(double t, double y) {
    const double snapped_y = snap_to_enumeration(y);
    auto it = std::lower_bound(samples_.begin(), samples_.end(), t,
                               [](const SamplePoint& sample, double value) { return sample.t < value; });
    if (it != samples_.end() && std::fabs(it->t - t) < kTimeEpsilon) {
        it->y = snapped_y;
        return static_cast<std::size_t>(std::distance(samples_.begin(), it));
    }
    auto inserted = samples_.insert(it, SamplePoint{t, snapped_y});
    return static_cast<std::size_t>(std::distance(samples_.begin(), inserted));
}

void Signal::remove_sample(std::size_t index) {
    if (index >= samples_.size()) {
        throw std::out_of_range("sample index out of range");
    }
    samples_.erase(samples_.begin() + static_cast<std::ptrdiff_t>(index));
}

void Signal::apply_gaussian_brush(double t_center, double delta_y, double sigma) {
    if (!(sigma > 0.0)) {
        throw std::invalid_argument("sigma must be > 0");
    }
    if (is_enumerated()) {
        throw std::invalid_argument("Gaussian brush is not supported for enumerated signals");
    }
    const double two_sigma_sq = 2.0 * sigma * sigma;
    for (auto& sample : samples_) {
        const double dt = sample.t - t_center;
        sample.y += delta_y * std::exp(-(dt * dt) / two_sigma_sq);
    }
}

std::size_t Signal::nearest_index(double t_query) const noexcept {
    if (samples_.empty()) {
        return std::numeric_limits<std::size_t>::max();
    }
    auto it = std::lower_bound(samples_.begin(), samples_.end(), t_query,
                               [](const SamplePoint& sample, double value) { return sample.t < value; });
    if (it == samples_.end()) {
        return samples_.size() - 1;
    }
    if (it == samples_.begin()) {
        return 0;
    }
    auto prev = std::prev(it);
    const double previous_distance = std::fabs(prev->t - t_query);
    const double current_distance = std::fabs(it->t - t_query);
    return previous_distance <= current_distance
               ? static_cast<std::size_t>(std::distance(samples_.begin(), prev))
               : static_cast<std::size_t>(std::distance(samples_.begin(), it));
}

}  // namespace myprj::signal_editor
