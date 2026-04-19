#pragma once

#include "signal_editor/core/domain/signal.h"

#include <cstddef>
#include <vector>

namespace signal_editor {

/**
 * @brief Aggregate root that owns the collection of editable signals.
 *
 * `SignalLibrary` keeps ordering stable, exposes bounds-checked accessors, and
 * centralises collection-level mutations for the rest of the module.
 */
class SignalLibrary {
public:
    SignalLibrary() = default;

    /** @brief Returns the number of stored signals. */
    [[nodiscard]] std::size_t size() const noexcept { return signals_.size(); }

    /** @brief Reports whether the library currently contains no signals. */
    [[nodiscard]] bool empty() const noexcept { return signals_.empty(); }

    /**
     * @brief Returns the underlying signal collection.
     * @return Ordered immutable view of all stored signals.
     */
    [[nodiscard]] const std::vector<Signal>& items() const noexcept { return signals_; }

    /**
     * @brief Returns a mutable signal reference.
     * @param index Zero-based signal index.
     * @return Mutable signal reference.
     * @throws std::out_of_range When `index` is invalid.
     */
    [[nodiscard]] Signal& at(std::size_t index);

    /**
     * @brief Returns an immutable signal reference.
     * @param index Zero-based signal index.
     * @return Immutable signal reference.
     * @throws std::out_of_range When `index` is invalid.
     */
    [[nodiscard]] const Signal& at(std::size_t index) const;

    /**
     * @brief Appends a signal to the end of the library.
     * @param signal Signal to take ownership of.
     * @return Index assigned to the appended signal.
     */
    std::size_t add(Signal signal);

    /**
     * @brief Removes a signal from the library.
     * @param index Zero-based signal index.
     * @throws std::out_of_range When `index` is invalid.
     */
    void remove(std::size_t index);

    /**
     * @brief Replaces an existing signal in place.
     * @param index Zero-based signal index.
     * @param signal Replacement signal.
     * @throws std::out_of_range When `index` is invalid.
     */
    void replace(std::size_t index, Signal signal);

    /**
     * @brief Removes every signal from the library.
     */
    void clear() noexcept;

private:
    std::vector<Signal> signals_;
};

}  // namespace signal_editor
