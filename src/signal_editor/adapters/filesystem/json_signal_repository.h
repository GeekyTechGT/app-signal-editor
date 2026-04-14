#pragma once

#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::adapters {

class JsonSignalRepository : public ISignalRepository {
public:
    SignalLibrary load(const std::filesystem::path& source) override;
    Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;
};

}  // namespace myprj::signal_editor::adapters
