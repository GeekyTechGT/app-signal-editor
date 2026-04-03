#pragma once

// Public API — the only entry point that apps/ should use.
// Apps wire adapters to ports here, then call this facade.

#include "sssss/core/usecases/my_usecase.h"
#include "sssss/ports/my_port.h"
#include "common/core/domain/common_types.h"

#include <filesystem>

namespace myprj::sssss::api {

// Convenience entry point: creates all internal objects, runs the use case.
myprj::Result run(IMyRepository& repository, std::string id, std::string name);

}  // namespace myprj::sssss::api
