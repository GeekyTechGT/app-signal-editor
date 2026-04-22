#pragma once

#include "signal_editor/core/domain/result.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/ports/signal_repository.h"

#include <cstddef>
#include <filesystem>
#include <string>

namespace signal_editor {

/**
 * @file
 * @brief Application service that orchestrates editable signal workflows.
 */

/**
 * @brief Application service that orchestrates all editable signal workflows.
 *
 * `SignalEditorService` is the application-facing coordinator that sits
 * between adapters and the domain model. The class intentionally owns very
 * little policy itself: it validates high-level workflow intent, delegates
 * persistence to the repository port, and applies edits to the in-memory
 * `SignalLibrary` while preserving domain invariants already enforced by the
 * underlying entities.
 *
 * In practical terms this service is the boundary used by the Qt shell:
 *
 * - load and save requests flow through this class
 * - document-local editing commands are expressed in terms of service calls
 * - the active in-memory library is exposed as the authoritative snapshot
 * - future adapters such as CLI tools, scripts, or tests can reuse the exact
 *   same orchestration logic without depending on Qt
 *
 * The service is intentionally stateful because the desktop application works
 * on an editable, user-visible in-memory workspace. That statefulness is kept
 * at the application boundary rather than in UI widgets so editing rules stay
 * testable and reuse remains straightforward.
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
    signal_editor::Result load_from(const std::filesystem::path& source);

    /**
     * @brief Saves the current library.
     * @param destination Target path understood by the repository.
     * @return Success or failure information.
     */
    signal_editor::Result save_to(const std::filesystem::path& destination) const;

    /**
     * @brief Creates and appends a uniformly sampled signal.
     *
     * This helper is used by adapters that need a simple generated signal
     * without manually constructing vectors first. The resulting signal is
     * inserted into the active library only if the request passes validation.
     */
    signal_editor::Result create_signal(const std::string& name,
                                double t_start,
                                double t_end,
                                std::size_t num_samples,
                                double initial_value = 0.0);

    /**
     * @brief Appends a fully formed signal to the active library.
     *
     * Ownership is transferred into the library snapshot maintained by the
     * service. Name validation and ordering constraints are enforced by the
     * domain model before the signal becomes visible to adapters.
     */
    signal_editor::Result add_signal(Signal signal);

    /**
     * @brief Removes a signal from the active library.
     *
     * The service does not try to keep any UI selection state in sync. That is
     * intentionally left to adapters such as `MainWindow`, which can react to a
     * successful removal in a document-aware way.
     */
    signal_editor::Result remove_signal(std::size_t index);

    /**
     * @brief Replaces a signal inside the active library.
     *
     * This operation is used when an adapter already owns a fully built signal
     * snapshot and wants to swap it into the current library atomically.
     */
    signal_editor::Result replace_signal(std::size_t index, Signal signal);

    /**
     * @brief Renames a signal inside the active library.
     *
     * Renaming stays in the service layer so adapters do not need to duplicate
     * domain validation rules for legal signal names.
     */
    signal_editor::Result rename_signal(std::size_t index, const std::string& new_name);

    /**
     * @brief Updates the interpolation mode of a stored signal.
     *
     * Numeric signals accept the requested interpolation mode directly.
     * Enumerated signals remain step-based by domain rule even if adapters ask
     * for a different mode.
     */
    signal_editor::Result set_signal_interpolation(std::size_t index, Signal::InterpolationMode interpolation);

    /**
     * @brief Replaces the enumeration mapping of a stored signal.
     *
     * The service delegates the semantic remap to the domain object so labels
     * remain stable when numeric values are reassigned.
     */
    signal_editor::Result set_signal_enumeration(
        std::size_t index,
        std::vector<Signal::EnumerationEntry> enumeration);

    /**
     * @brief Updates the value of an existing sample.
     *
     * This command keeps the sample timestamp unchanged and only updates the
     * Y-value of the addressed sample.
     */
    signal_editor::Result move_sample(std::size_t signal_index,
                              std::size_t sample_index,
                              double new_y);

    /**
     * @brief Moves a shared sample timestamp and updates the active sample value.
     *
     * In the Signal Editor workspace multiple signals inside one file are
     * treated as sharing a common time axis. This command therefore propagates
     * the timestamp move coherently across sibling signals while applying the
     * provided value change to the addressed signal.
     */
    signal_editor::Result move_sample_point(std::size_t signal_index,
                                    std::size_t sample_index,
                                    double new_t,
                                    double new_y,
                                    std::size_t* out_index = nullptr);

    /**
     * @brief Inserts or merges a sample in the addressed signal.
     *
     * When the active library represents signals from one imported file, the
     * insert propagates the shared timestamp to sibling signals as well. The
     * addressed signal receives the provided value and other signals receive an
     * interpolated value where appropriate.
     */
    signal_editor::Result insert_sample(std::size_t signal_index,
                                double t,
                                double y,
                                std::size_t* out_index = nullptr);

    /**
     * @brief Removes a sample from the addressed signal.
     *
     * The removal follows the same shared-time-axis rule used by insertion and
     * sample-point moves so the library remains internally aligned.
     */
    signal_editor::Result remove_sample(std::size_t signal_index, std::size_t sample_index);

    /**
     * @brief Applies a vertical offset to every sample of every loaded signal.
     *
     * This is primarily used by plot and table context-menu actions that treat
     * the active file as a coherent editing surface rather than isolated curves.
     */
    signal_editor::Result apply_offset_to_all_samples(double delta_y);

    /**
     * @brief Applies a vertical offset to one sample of the addressed signal.
     *
     * The timestamp is preserved and only the selected sample value changes.
     */
    signal_editor::Result apply_offset_to_sample(std::size_t signal_index,
                                         std::size_t sample_index,
                                         double delta_y);

    /**
     * @brief Moves an active signal segment vertically by offsetting both endpoints.
     *
     * Segment movement is surfaced by the plot adapter as a direct manipulation
     * gesture. The service keeps the operation explicit so it can be reused by
     * non-Qt callers if needed.
     */
    signal_editor::Result move_segment(std::size_t signal_index,
                               std::size_t start_index,
                               double delta_y);

    /**
     * @brief Applies Gaussian deformation to the addressed signal.
     *
     * This operation is intentionally expressed at the application boundary
     * rather than inside the widget so the mathematical edit remains testable
     * independently from user interaction code.
     */
    signal_editor::Result apply_gaussian_brush(std::size_t signal_index,
                                       double t_center,
                                       double delta_y,
                                       double sigma);

private:
    ISignalRepository& repository_;
    SignalLibrary library_;
};

}  // namespace signal_editor
