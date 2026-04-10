#pragma once

#include "signal_editor/core/domain/signal.h"

#include <cstddef>
#include <vector>

namespace myprj::signal_editor {

// --- Domain aggregate -------------------------------------------------------
// Owns a collection of `Signal` objects. Pure container with safe access.
// No I/O, no framework dependency.
class SignalLibrary {
public:
    SignalLibrary() = default;

    [[nodiscard]] std::size_t size() const noexcept { return signals_.size(); }
    [[nodiscard]] bool empty() const noexcept { return signals_.empty(); }
    // NOTE: not named `signals()` because Qt's moc redefines `signals` as a macro.
    [[nodiscard]] const std::vector<Signal>& items() const noexcept { return signals_; }

    // Throws std::out_of_range if `index` is invalid.
    [[nodiscard]] Signal& at(std::size_t index);
    [[nodiscard]] const Signal& at(std::size_t index) const;

    // Add a signal at the end. Returns its index.
    std::size_t add(Signal signal);

    // Remove the signal at `index`. Throws std::out_of_range.
    void remove(std::size_t index);

    // Replace the signal at `index`. Throws std::out_of_range.
    void replace(std::size_t index, Signal signal);

    // Remove all signals.
    void clear() noexcept;

private:
    std::vector<Signal> signals_;
};

}  // namespace myprj::signal_editor
