#include "signal_editor/core/domain/signal_library.h"

#include <stdexcept>
#include <utility>

namespace signal_editor {

namespace {
[[noreturn]] void throw_oor() {
    throw std::out_of_range("signal index out of range");
}
}  // namespace

Signal& SignalLibrary::at(std::size_t index) {
    if (index >= signals_.size()) {
        throw_oor();
    }
    return signals_[index];
}

const Signal& SignalLibrary::at(std::size_t index) const {
    if (index >= signals_.size()) {
        throw_oor();
    }
    return signals_[index];
}

std::size_t SignalLibrary::add(Signal signal) {
    signals_.push_back(std::move(signal));
    return signals_.size() - 1;
}

void SignalLibrary::remove(std::size_t index) {
    if (index >= signals_.size()) {
        throw_oor();
    }
    signals_.erase(signals_.begin() + static_cast<std::ptrdiff_t>(index));
}

void SignalLibrary::replace(std::size_t index, Signal signal) {
    if (index >= signals_.size()) {
        throw_oor();
    }
    signals_[index] = std::move(signal);
}

void SignalLibrary::clear() noexcept {
    signals_.clear();
}

}  // namespace signal_editor
