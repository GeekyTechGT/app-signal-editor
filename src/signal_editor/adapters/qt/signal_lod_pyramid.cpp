#include "signal_editor/adapters/qt/signal_lod_pyramid.h"

#include "signal_editor/core/domain/sample_point.h"
#include "signal_editor/core/domain/signal.h"

#include <algorithm>

namespace signal_editor::adapters::qt {
namespace {

auto lower_sample_at_or_after(const std::vector<SamplePoint>& samples, double t) {
    return std::lower_bound(samples.begin(), samples.end(), t,
                            [](const SamplePoint& sample, double value) {
                                return sample.t < value;
                            });
}

auto upper_sample_after(const std::vector<SamplePoint>& samples, double t) {
    return std::upper_bound(samples.begin(), samples.end(), t,
                            [](double value, const SamplePoint& sample) {
                                return value < sample.t;
                            });
}

[[nodiscard]] bool matches_signal(const SignalLodPyramid& pyramid, const Signal& signal) {
    return pyramid.signal == &signal &&
           pyramid.sample_count == signal.size() &&
           !signal.empty() &&
           pyramid.first_t == signal.samples().front().t &&
           pyramid.last_t == signal.samples().back().t;
}

[[nodiscard]] SignalLodPyramid build_pyramid(const Signal& signal) {
    SignalLodPyramid pyramid;
    pyramid.signal = &signal;
    pyramid.sample_count = signal.size();
    if (!signal.empty()) {
        pyramid.first_t = signal.samples().front().t;
        pyramid.last_t = signal.samples().back().t;
    }

    const auto& samples = signal.samples();
    if (samples.size() < 2U) {
        return pyramid;
    }

    LodLevel first_level;
    first_level.samples_per_bucket = 2U;
    first_level.buckets.reserve((samples.size() + 1U) / 2U);
    for (std::size_t index = 0; index < samples.size(); index += 2U) {
        const std::size_t last_index = std::min(index + 1U, samples.size() - 1U);
        const double a = samples[index].y;
        const double b = samples[last_index].y;
        first_level.buckets.push_back({
            static_cast<float>(std::min(a, b)),
            static_cast<float>(std::max(a, b))
        });
    }
    pyramid.levels.push_back(std::move(first_level));

    while (!pyramid.levels.empty() && pyramid.levels.back().buckets.size() > 1U) {
        const LodLevel& previous = pyramid.levels.back();
        LodLevel next;
        next.samples_per_bucket = previous.samples_per_bucket * 2U;
        next.buckets.reserve((previous.buckets.size() + 1U) / 2U);
        for (std::size_t index = 0; index < previous.buckets.size(); index += 2U) {
            const auto& a = previous.buckets[index];
            const auto& b = previous.buckets[std::min(index + 1U, previous.buckets.size() - 1U)];
            next.buckets.push_back({
                std::min(a.min_value, b.min_value),
                std::max(a.max_value, b.max_value)
            });
        }
        pyramid.levels.push_back(std::move(next));
    }

    return pyramid;
}

}  // namespace

const SignalLodPyramid& SignalLodCache::pyramid_for(const Signal& signal) const {
    const auto cached = std::find_if(pyramids_.begin(), pyramids_.end(),
                                     [&signal](const SignalLodPyramid& pyramid) {
                                         return matches_signal(pyramid, signal);
                                     });
    if (cached != pyramids_.end()) {
        return *cached;
    }

    invalidate(&signal);
    pyramids_.push_back(build_pyramid(signal));
    return pyramids_.back();
}

void SignalLodCache::invalidate(const Signal* signal) const {
    if (signal == nullptr) {
        return;
    }
    pyramids_.erase(std::remove_if(pyramids_.begin(), pyramids_.end(),
                                   [signal](const SignalLodPyramid& pyramid) {
                                       return pyramid.signal == signal;
                                   }),
                    pyramids_.end());
}

void SignalLodCache::clear() const {
    pyramids_.clear();
}

std::pair<std::size_t, std::size_t> visible_sample_range(const Signal& signal,
                                                         double t_min,
                                                         double t_max) {
    const auto& samples = signal.samples();
    if (samples.empty() || t_max < samples.front().t || t_min > samples.back().t) {
        return {0U, 0U};
    }

    auto first = lower_sample_at_or_after(samples, t_min);
    if (first != samples.begin()) {
        --first;
    }
    auto last = upper_sample_after(samples, t_max);
    if (last != samples.end()) {
        ++last;
    }

    return {
        static_cast<std::size_t>(std::distance(samples.begin(), first)),
        static_cast<std::size_t>(std::distance(samples.begin(), last))
    };
}

std::size_t visible_sample_count(const Signal& signal, double t_min, double t_max) {
    const auto [first, last] = visible_sample_range(signal, t_min, t_max);
    return last > first ? last - first : 0U;
}

std::pair<double, double> bucket_time_range(const Signal& signal,
                                            const LodLevel& level,
                                            std::size_t bucket_index) {
    const auto& samples = signal.samples();
    const std::size_t first_sample_index = bucket_index * level.samples_per_bucket;
    const std::size_t last_sample_index =
        std::min(first_sample_index + level.samples_per_bucket - 1U, samples.size() - 1U);
    return {samples[first_sample_index].t, samples[last_sample_index].t};
}

const LodLevel* choose_lod_level(const SignalLodPyramid& pyramid,
                                 std::size_t visible_count,
                                 double content_width,
                                 std::size_t max_raw_trace_points) {
    if (visible_count <= max_raw_trace_points || pyramid.levels.empty()) {
        return nullptr;
    }

    const double samples_per_pixel =
        static_cast<double>(visible_count) / std::max(1.0, content_width);
    if (samples_per_pixel <= 1.5) {
        return nullptr;
    }

    const LodLevel* selected = nullptr;
    for (const auto& level : pyramid.levels) {
        if (static_cast<double>(level.samples_per_bucket) <= samples_per_pixel) {
            selected = &level;
        } else {
            break;
        }
    }
    return selected;
}

const LodLevel* choose_range_lod_level(const SignalLodPyramid& pyramid,
                                       std::size_t visible_count,
                                       std::size_t max_raw_scan_points,
                                       double target_bucket_count) {
    if (visible_count <= max_raw_scan_points || pyramid.levels.empty()) {
        return nullptr;
    }

    const LodLevel* selected = &pyramid.levels.back();
    for (const auto& level : pyramid.levels) {
        const double estimated_visible_buckets =
            static_cast<double>(visible_count) /
            static_cast<double>(std::max<std::size_t>(1U, level.samples_per_bucket));
        selected = &level;
        if (estimated_visible_buckets <= target_bucket_count) {
            break;
        }
    }
    return selected;
}

}  // namespace signal_editor::adapters::qt
