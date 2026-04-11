#pragma once

#include "signal_editor/core/domain/result.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/ports/signal_repository.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace myprj::signal_editor {

/**
 * @brief Application service that orchestrates all editable signal workflows.
 *
 * `SignalEditorService` is the primary entry point used by the GUI and any
 * future CLI or automation layer. It keeps the active in-memory library and
 * delegates persistence to the repository port.
 */
class SignalEditorService {
public:
    /**
     * @brief Creates the service around a persistence adapter.
     * @param repository Output port used for load/save operations.
     */
    explicit SignalEditorService(ISignalRepository& repository);

    /** @brief Returns the current library as an immutable reference. */
    [[nodiscard]] const SignalLibrary& library() const noexcept { return library_; }

    /** @brief Returns the current library as a mutable reference. */
    [[nodiscard]] SignalLibrary& library() noexcept { return library_; }

    /**
     * @brief Replaces the current in-memory library.
     * @param library New library snapshot.
     */
    void set_library(SignalLibrary library) noexcept;

    /**
     * @brief Clears the current library.
     */
    void clear() noexcept;

    /**
     * @brief Loads a library from persistent storage.
     * @param source Source path understood by the repository.
     * @return Success or failure information.
     */
    myprj::Result load_from(const std::filesystem::path& source);

    /**
     * @brief Saves the current library.
     * @param destination Target path understood by the repository.
     * @return Success or failure information.
     */
    myprj::Result save_to(const std::filesystem::path& destination) const;

    /**
     * @brief Creates and appends a uniformly sampled signal.
     */
    myprj::Result create_signal(const std::string& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double initial_value = 0.0);

    /**
     * @brief Appends a fully formed signal to the active library.
     */
    myprj::Result add_signal(Signal signal);

    /** @brief Removes a signal from the active library. */
    myprj::Result remove_signal(std::size_t index);

    /** @brief Replaces a signal inside the active library. */
    myprj::Result replace_signal(std::size_t index, Signal signal);

    /** @brief Renames a signal inside the active library. */
    myprj::Result rename_signal(std::size_t index, const std::string& new_name);

    /** @brief Updates the interpolation mode of a stored signal. */
    myprj::Result set_signal_interpolation(std::size_t index, Signal::InterpolationMode interpolation);

    /** @brief Updates the value of an existing sample. */
    myprj::Result move_sample(std::size_t signal_index,
                              std::size_t sample_index,
                              double new_y);

    /** @brief Inserts or merges a sample in the addressed signal. */
    myprj::Result insert_sample(std::size_t signal_index,
                                double t,
                                double y,
                                std::size_t* out_index = nullptr);

    /** @brief Removes a sample from the addressed signal. */
    myprj::Result remove_sample(std::size_t signal_index, std::size_t sample_index);

    /** @brief Applies Gaussian deformation to the addressed signal. */
    myprj::Result apply_gaussian_brush(std::size_t signal_index,
                                       double t_center,
                                       double delta_y,
                                       double sigma);

private:
    ISignalRepository& repository_;
    SignalLibrary library_;
};

}  // namespace myprj::signal_editor
