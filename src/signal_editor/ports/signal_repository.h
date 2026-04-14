#pragma once

#include "signal_editor/core/domain/result.h"
#include "signal_editor/core/domain/signal_library.h"

#include <filesystem>

namespace myprj::signal_editor {

/**
 * @brief Output port responsible for persisting and restoring signal libraries.
 *
 * The core depends only on this abstraction. Concrete implementations live in
 * adapters such as filesystem-backed CSV readers or future network/database
 * integrations.
 */
class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    /**
     * @brief Loads all signals from the given source.
     * @param source Repository-specific source path.
     * @return Fully populated signal library.
     * @throws std::exception Implementations throw when parsing or access fails.
     */
    virtual SignalLibrary load(const std::filesystem::path& source) = 0;

    /**
     * @brief Persists the supplied library to the destination.
     * @param destination Repository-specific output path.
     * @param library In-memory library to serialise.
     * @return Success or failure information for UI and workflow code.
     */
    virtual Result save(const std::filesystem::path& destination,
                               const SignalLibrary& library) = 0;
};

}  // namespace myprj::signal_editor
