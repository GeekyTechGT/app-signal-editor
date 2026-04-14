#pragma once

#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::adapters {

/**
 * @brief CSV-specific implementation of the signal repository port.
 *
 * The adapter reads and writes libraries using a shared time-axis CSV layout:
 *
 * `time,sig1,sig2,sig3`
 * `0.00,0.10,0.50,0.90`
 * `0.01,0.20,0.60,0.80`
 *
 * The first column is always the time vector. A header row is optional, and an
 * interpolation metadata row can be stored ahead of the header so that linear
 * versus step rendering survives a save/load round-trip.
 */
class CsvSignalRepository : public ISignalRepository {
public:
    /**
     * @brief Loads a signal library from a CSV file.
     * @param source Filesystem path to the CSV document.
     * @return Parsed library with one signal per value column.
     */
    SignalLibrary load(const std::filesystem::path& source) override;

    /**
     * @brief Saves a signal library to a CSV file.
     * @param destination Filesystem path that will receive the export.
     * @param library Library to serialise.
     * @return Success or failure information for workflow code.
     */
    Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;
};

}  // namespace myprj::signal_editor::adapters
