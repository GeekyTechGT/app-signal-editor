#pragma once

// Public API — the only entry point that apps/ should use.
// Apps wire adapters to ports here, then call this facade.

#include "my_module/core/usecases/my_usecase.h"
#include "my_module/ports/my_port.h"
#include "common/core/domain/common_types.h"

#include <filesystem>

namespace myprj::my_module::api {

// Convenience entry point: creates all internal objects, runs the use case.
myprj::Result run(IMyRepository& repository, std::string id, std::string name);

}  // namespace myprj::my_module::api
