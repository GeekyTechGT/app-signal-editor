#include "signal_editor/core/usecases/signal_editor_service.h"

#include <stdexcept>
#include <utility>

namespace signal_editor {

namespace {
template <typename Fn>
signal_editor::Result guarded(Fn&& fn) {
    try {
        std::forward<Fn>(fn)();
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
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
        library_.add(std::move(signal));
    });
}

signal_editor::Result SignalEditorService::remove_signal(std::size_t index) {
    return guarded([&] { library_.remove(index); });
}

signal_editor::Result SignalEditorService::replace_signal(std::size_t index, Signal signal) {
    return guarded([&] {
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

signal_editor::Result SignalEditorService::move_sample(std::size_t signal_index,
                                               std::size_t sample_index,
                                               double new_y) {
    return guarded([&] {
        library_.at(signal_index).set_sample_value(sample_index, new_y);
    });
}

signal_editor::Result SignalEditorService::insert_sample(std::size_t signal_index,
                                                 double t,
                                                 double y,
                                                 std::size_t* out_index) {
    return guarded([&] {
        const auto idx = library_.at(signal_index).insert_sample(t, y);
        if (out_index != nullptr) {
            *out_index = idx;
        }
    });
}

signal_editor::Result SignalEditorService::remove_sample(std::size_t signal_index,
                                                 std::size_t sample_index) {
    return guarded([&] {
        library_.at(signal_index).remove_sample(sample_index);
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
