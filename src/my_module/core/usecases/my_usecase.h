#pragma once

#include "my_module/ports/my_port.h"
#include "common/core/domain/common_types.h"

#include <string>

namespace myprj::my_module {

// --- Use case ---------------------------------------------------------------
// Orchestrates domain logic. Depends on ports, never on concrete adapters.
class MyUseCase {
public:
    explicit MyUseCase(IMyRepository& repository);

    myprj::Result execute(std::string id, std::string name);

private:
    IMyRepository& repository_;
};

}  // namespace myprj::my_module
