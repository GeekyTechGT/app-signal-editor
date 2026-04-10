#pragma once

#include "common/core/domain/common_types.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/ports/signal_repository.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace myprj::signal_editor {

// --- Use case orchestrator --------------------------------------------------
// Owns a `SignalLibrary` and exposes the editing operations described in the
// SRS. Depends on the `ISignalRepository` port for persistence — never on a
// concrete adapter.
//
// All mutating operations return a `myprj::Result`. Read-only access goes
// through `library()` to keep the API small (KISS).
class SignalEditorService {
public:
    explicit SignalEditorService(ISignalRepository& repository);

    // --- Library access -------------------------------------------------
    [[nodiscard]] const SignalLibrary& library() const noexcept { return library_; }
    [[nodiscard]] SignalLibrary& library() noexcept { return library_; }

    void set_library(SignalLibrary library) noexcept;
    void clear() noexcept;

    // --- Persistence ----------------------------------------------------
    myprj::Result load_from(const std::filesystem::path& source);
    myprj::Result save_to(const std::filesystem::path& destination) const;

    // --- Library editing ------------------------------------------------
    myprj::Result create_signal(const std::string& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double initial_value = 0.0);

    myprj::Result remove_signal(std::size_t index);
    myprj::Result rename_signal(std::size_t index, const std::string& new_name);

    // --- Signal editing -------------------------------------------------
    myprj::Result move_sample(std::size_t signal_index,
                              std::size_t sample_index,
                              double new_y);

    // Returns the new sample index in `out_index`. `out_index` may be null.
    myprj::Result insert_sample(std::size_t signal_index,
                                double t,
                                double y,
                                std::size_t* out_index = nullptr);

    myprj::Result remove_sample(std::size_t signal_index, std::size_t sample_index);

    myprj::Result apply_gaussian_brush(std::size_t signal_index,
                                       double t_center,
                                       double delta_y,
                                       double sigma);

private:
    ISignalRepository& repository_;
    SignalLibrary library_;
};

}  // namespace myprj::signal_editor
