#pragma once

#include "common/core/domain/common_types.h"
#include "signal_editor/core/domain/signal_library.h"

#include <filesystem>

namespace myprj::signal_editor {

// --- Driven port -----------------------------------------------------------
// Abstraction for persisting / retrieving a SignalLibrary.
// The core depends only on this interface; concrete implementations live in
// `adapters/filesystem`, `adapters/json`, etc.
class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    // Load all signals from `source`. Throws on parse failure.
    virtual SignalLibrary load(const std::filesystem::path& source) = 0;

    // Persist `library` to `destination`. Returns ok / error result.
    virtual myprj::Result save(const std::filesystem::path& destination,
                               const SignalLibrary& library) = 0;
};

}  // namespace myprj::signal_editor
