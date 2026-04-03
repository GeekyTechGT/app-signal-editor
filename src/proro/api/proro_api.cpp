#include "proro_api.h"

#include "proro/core/usecases/my_usecase.h"

namespace myprj::proro::api {

myprj::Result run(IMyRepository& repository, std::string id, std::string name) {
    MyUseCase uc(repository);
    return uc.execute(std::move(id), std::move(name));
}

}  // namespace myprj::proro::api
