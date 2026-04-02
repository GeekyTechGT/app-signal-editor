#pragma once

#include "my_module/core/domain/my_entity.h"
#include "common/core/domain/common_types.h"

#include <vector>

namespace myprj::my_module {

// --- Output port (driven port) ---------------------------------------------
// Implemented by adapters (filesystem, GUI, etc.)
// The use case depends only on this interface — never on concrete adapters.
class IMyRepository {
public:
    virtual ~IMyRepository() = default;

    virtual myprj::Result save(const MyEntity& entity) = 0;
    virtual std::vector<MyEntity> load_all() = 0;
};

}  // namespace myprj::my_module
