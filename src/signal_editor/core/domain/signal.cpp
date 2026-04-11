#include "signal_editor/core/domain/signal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace myprj::signal_editor {

namespace {
constexpr double kTimeEpsilon = 1e-12;

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
    const double dt =
        (t_end - t_start) / static_cast<double>(num_samples - 1);
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
    double m = samples_.front().y;
    for (const auto& s : samples_) {
        m = std::min(m, s.y);
    }
    return m;
}

double Signal::y_max() const noexcept {
    if (samples_.empty()) {
        return 0.0;
    }
    double m = samples_.front().y;
    for (const auto& s : samples_) {
        m = std::max(m, s.y);
    }
    return m;
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
                               [](double v, const SamplePoint& s) { return v < s.t; });
    const auto& hi = *it;
    const auto& lo = *std::prev(it);

    if (interpolation_ == InterpolationMode::Step) {
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
    interpolation_ = interpolation;
}

void Signal::set_sample_value(std::size_t index, double new_y) {
    if (index >= samples_.size()) {
        throw std::out_of_range("sample index out of range");
    }
    samples_[index].y = new_y;
}

std::size_t Signal::move_sample(std::size_t index, double new_t, double new_y) {
    if (index >= samples_.size()) {
        throw std::out_of_range("sample index out of range");
    }

    samples_.erase(samples_.begin() + static_cast<std::ptrdiff_t>(index));
    return insert_sample(new_t, new_y);
}

std::size_t Signal::insert_sample(double t, double y) {
    auto it = std::lower_bound(samples_.begin(), samples_.end(), t,
                               [](const SamplePoint& s, double v) { return s.t < v; });
    if (it != samples_.end() && std::fabs(it->t - t) < kTimeEpsilon) {
        it->y = y;
        return static_cast<std::size_t>(std::distance(samples_.begin(), it));
    }
    auto inserted = samples_.insert(it, SamplePoint{t, y});
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
    const double two_sigma_sq = 2.0 * sigma * sigma;
    for (auto& s : samples_) {
        const double dt = s.t - t_center;
        s.y += delta_y * std::exp(-(dt * dt) / two_sigma_sq);
    }
}

std::size_t Signal::nearest_index(double t_query) const noexcept {
    if (samples_.empty()) {
        return std::numeric_limits<std::size_t>::max();
    }
    auto it = std::lower_bound(samples_.begin(), samples_.end(), t_query,
                               [](const SamplePoint& s, double v) { return s.t < v; });
    if (it == samples_.end()) {
        return samples_.size() - 1;
    }
    if (it == samples_.begin()) {
        return 0;
    }
    auto prev = std::prev(it);
    const double d_prev = std::fabs(prev->t - t_query);
    const double d_curr = std::fabs(it->t - t_query);
    return d_prev <= d_curr
               ? static_cast<std::size_t>(std::distance(samples_.begin(), prev))
               : static_cast<std::size_t>(std::distance(samples_.begin(), it));
}

}  // namespace myprj::signal_editor
