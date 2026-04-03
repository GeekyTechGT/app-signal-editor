#include "my_usecase.h"

#include "sssss/core/domain/my_entity.h"

#include <stdexcept>

namespace myprj::sssss {

MyUseCase::MyUseCase(IMyRepository& repository) : repository_(repository) {}

myprj::Result MyUseCase::execute(std::string id, std::string name) {
    try {
        auto entity = MyEntity::create(std::move(id), std::move(name));
        return repository_.save(entity);
    } catch (const std::invalid_argument& ex) {
        return myprj::Result::error(ex.what());
    }
}

}  // namespace myprj::sssss
