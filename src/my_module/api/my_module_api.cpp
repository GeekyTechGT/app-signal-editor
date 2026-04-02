#include "my_module_api.h"

#include "my_module/core/usecases/my_usecase.h"

namespace myprj::my_module::api {

myprj::Result run(IMyRepository& repository, std::string id, std::string name) {
    MyUseCase uc(repository);
    return uc.execute(std::move(id), std::move(name));
}

}  // namespace myprj::my_module::api
