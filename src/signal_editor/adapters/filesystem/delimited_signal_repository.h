#pragma once

#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::adapters {

class DelimitedSignalRepository : public ISignalRepository {
public:
    explicit DelimitedSignalRepository(char delimiter = '	') noexcept;

    SignalLibrary load(const std::filesystem::path& source) override;
    Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;

private:
    char delimiter_;
};

}  // namespace myprj::signal_editor::adapters
