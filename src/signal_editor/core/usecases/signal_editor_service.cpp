#include "signal_editor/core/usecases/signal_editor_service.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace signal_editor {

namespace {
constexpr double kTimeEpsilon = 1e-9;

template <typename Fn>
signal_editor::Result guarded(Fn&& fn) {
    try {
        std::forward<Fn>(fn)();
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

const std::vector<SamplePoint>& require_reference_samples(const SignalLibrary& library) {
    if (library.empty()) {
        throw std::invalid_argument("signal library is empty");
    }
    return library.at(0).samples();
}

bool shares_time_axis(const Signal& signal, const std::vector<SamplePoint>& reference) {
    if (signal.size() != reference.size()) {
        return false;
    }
    for (std::size_t index = 0; index < reference.size(); ++index) {
        if (std::fabs(signal.samples()[index].t - reference[index].t) >= kTimeEpsilon) {
            return false;
        }
    }
    return true;
}

void ensure_shared_time_axis(const SignalLibrary& library) {
    if (library.size() < 2) {
        return;
    }
    const auto& reference = require_reference_samples(library);
    for (std::size_t index = 1; index < library.size(); ++index) {
        if (!shares_time_axis(library.at(index), reference)) {
            throw std::invalid_argument("all signals in a file must share the same time axis");
        }
    }
}

Signal align_signal_to_reference_axis(Signal signal, const std::vector<SamplePoint>& reference) {
    if (reference.empty()) {
        return signal;
    }
    if (shares_time_axis(signal, reference)) {
        return signal;
    }

    std::vector<SamplePoint> aligned_samples;
    aligned_samples.reserve(reference.size());
    for (const auto& sample : reference) {
        aligned_samples.push_back({sample.t, signal.interpolate(sample.t)});
    }

    Signal aligned(signal.name(), std::move(aligned_samples), signal.interpolation());
    if (signal.is_enumerated()) {
        aligned.set_enumeration(signal.enumeration());
    }
    return aligned;
}

double interpolated_insert_value(const Signal& signal, double t) {
    const auto& samples = signal.samples();
    if (samples.empty()) {
        return signal.snap_to_enumeration(0.0);
    }
    if (t <= samples.front().t) {
        return samples.front().y;
    }
    if (t >= samples.back().t) {
        return samples.back().y;
    }

    auto upper = std::lower_bound(samples.begin(), samples.end(), t,
                                  [](const SamplePoint& sample, double value) { return sample.t < value; });
    if (upper != samples.end() && std::fabs(upper->t - t) < kTimeEpsilon) {
        return upper->y;
    }

    const auto lower = std::prev(upper);
    const double dt = upper->t - lower->t;
    if (dt <= 0.0) {
        return signal.snap_to_enumeration(lower->y);
    }

    const double alpha = (t - lower->t) / dt;
    return signal.snap_to_enumeration(lower->y + alpha * (upper->y - lower->y));
}
}  // namespace

SignalEditorService::SignalEditorService(ISignalRepository& repository)
    : repository_(repository) {}

void SignalEditorService::set_library(SignalLibrary library) noexcept {
    library_ = std::move(library);
}

void SignalEditorService::clear() noexcept {
    library_.clear();
}

signal_editor::Result SignalEditorService::load_from(const std::filesystem::path& source) {
    return guarded([&] {
        library_ = repository_.load(source);
        ensure_shared_time_axis(library_);
    });
}

signal_editor::Result SignalEditorService::save_to(const std::filesystem::path& destination) const {
    try {
        return repository_.save(destination, library_);
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

signal_editor::Result SignalEditorService::create_signal(const std::string& name,
                                                 double t_start,
                                                 double t_end,
                                                 std::size_t num_samples,
                                                 double initial_value) {
    return guarded([&] {
        library_.add(Signal::create_uniform(name, t_start, t_end, num_samples, initial_value));
    });
}

signal_editor::Result SignalEditorService::add_signal(Signal signal) {
    return guarded([&] {
        if (!library_.empty()) {
            signal = align_signal_to_reference_axis(std::move(signal), require_reference_samples(library_));
        }
        library_.add(std::move(signal));
    });
}

signal_editor::Result SignalEditorService::remove_signal(std::size_t index) {
    return guarded([&] { library_.remove(index); });
}

signal_editor::Result SignalEditorService::replace_signal(std::size_t index, Signal signal) {
    return guarded([&] {
        if (index >= library_.size()) {
            throw std::out_of_range("signal index out of range");
        }
        if (library_.size() > 1) {
            const auto& reference = index == 0
                ? library_.at(1).samples()
                : library_.at(0).samples();
            signal = align_signal_to_reference_axis(std::move(signal), reference);
        }
        library_.replace(index, std::move(signal));
    });
}

signal_editor::Result SignalEditorService::rename_signal(std::size_t index,
                                                 const std::string& new_name) {
    return guarded([&] { library_.at(index).set_name(new_name); });
}

signal_editor::Result SignalEditorService::set_signal_interpolation(
    std::size_t index,
    Signal::InterpolationMode interpolation) {
    return guarded([&] { library_.at(index).set_interpolation(interpolation); });
}

signal_editor::Result SignalEditorService::set_signal_enumeration(
    std::size_t index,
    std::vector<Signal::EnumerationEntry> enumeration) {
    return guarded([&] { library_.at(index).set_enumeration(std::move(enumeration)); });
}

signal_editor::Result SignalEditorService::move_sample(std::size_t signal_index,
                                               std::size_t sample_index,
                                               double new_y) {
    return guarded([&] {
        ensure_shared_time_axis(library_);
        library_.at(signal_index).set_sample_value(sample_index, new_y);
    });
}

signal_editor::Result SignalEditorService::move_sample_point(std::size_t signal_index,
                                                     std::size_t sample_index,
                                                     double new_t,
                                                     double new_y,
                                                     std::size_t* out_index) {
    return guarded([&] {
        ensure_shared_time_axis(library_);
        std::size_t resulting_index = 0;
        for (std::size_t index = 0; index < library_.size(); ++index) {
            auto& signal = library_.at(index);
            const double value = index == signal_index ? new_y : signal.samples().at(sample_index).y;
            const std::size_t moved_index = signal.move_sample(sample_index, new_t, value);
            if (index == signal_index) {
                resulting_index = moved_index;
            }
        }
        if (out_index != nullptr) {
            *out_index = resulting_index;
        }
    });
}

signal_editor::Result SignalEditorService::insert_sample(std::size_t signal_index,
                                                 double t,
                                                 double y,
                                                 std::size_t* out_index) {
    return guarded([&] {
        ensure_shared_time_axis(library_);
        std::size_t idx = 0;
        for (std::size_t index = 0; index < library_.size(); ++index) {
            auto& signal = library_.at(index);
            const double value = index == signal_index ? y : interpolated_insert_value(signal, t);
            const std::size_t inserted_index = signal.insert_sample(t, value);
            if (index == signal_index) {
                idx = inserted_index;
            }
        }
        if (out_index != nullptr) {
            *out_index = idx;
        }
    });
}

signal_editor::Result SignalEditorService::remove_sample(std::size_t signal_index,
                                                 std::size_t sample_index) {
    return guarded([&] {
        ensure_shared_time_axis(library_);
        (void)signal_index;
        for (std::size_t index = 0; index < library_.size(); ++index) {
            library_.at(index).remove_sample(sample_index);
        }
    });
}

signal_editor::Result SignalEditorService::apply_offset_to_all_samples(double delta_y) {
    return guarded([&] {
        for (std::size_t index = 0; index < library_.size(); ++index) {
            library_.at(index).apply_offset(delta_y);
        }
    });
}

signal_editor::Result SignalEditorService::apply_offset_to_sample(std::size_t signal_index,
                                                          std::size_t sample_index,
                                                          double delta_y) {
    return guarded([&] {
        library_.at(signal_index).apply_offset_to_sample(sample_index, delta_y);
    });
}

signal_editor::Result SignalEditorService::move_segment(std::size_t signal_index,
                                                std::size_t start_index,
                                                double delta_y) {
    return guarded([&] {
        library_.at(signal_index).move_segment(start_index, delta_y);
    });
}

signal_editor::Result SignalEditorService::apply_gaussian_brush(std::size_t signal_index,
                                                        double t_center,
                                                        double delta_y,
                                                        double sigma) {
    return guarded([&] {
        library_.at(signal_index).apply_gaussian_brush(t_center, delta_y, sigma);
    });
}

}  // namespace signal_editor
