#include "my_entity.h"

#include <stdexcept>

namespace myprj::my_module {

MyEntity MyEntity::create(std::string id, std::string name) {
    if (id.empty()) throw std::invalid_argument("id must not be empty");
    if (name.empty()) throw std::invalid_argument("name must not be empty");
    return {std::move(id), std::move(name)};
}

}  // namespace myprj::my_module
