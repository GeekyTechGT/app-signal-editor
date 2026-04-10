#pragma once

#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::adapters {

// --- CSV adapter (driven adapter) -------------------------------------------
// Reads / writes a `SignalLibrary` to disk in CSV format:
//
//   time,sig1,sig2,sig3
//   0.00,0.10,0.50,0.90
//   0.01,0.20,0.60,0.80
//   ...
//
// * The first column is always the time vector (shared by all signals).
// * The header row is optional. If the first cell of the first row is not a
//   number, the row is treated as the header and used as signal names.
//   Otherwise signals are named `signal_1 … signal_N`.
//
// On `save`, the adapter merges all signals onto the union of their time
// vectors and linearly interpolates each signal so that every row has a
// value for every column. This guarantees that a load -> edit -> save -> load
// round-trip preserves the data shown in the GUI.
class CsvSignalRepository : public ISignalRepository {
public:
    SignalLibrary load(const std::filesystem::path& source) override;

    myprj::Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;
};

}  // namespace myprj::signal_editor::adapters
