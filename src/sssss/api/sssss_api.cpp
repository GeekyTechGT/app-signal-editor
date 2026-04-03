#include "sssss_api.h"

#include "sssss/core/usecases/my_usecase.h"

namespace myprj::sssss::api {

myprj::Result run(IMyRepository& repository, std::string id, std::string name) {
    MyUseCase uc(repository);
    return uc.execute(std::move(id), std::move(name));
}

}  // namespace myprj::sssss::api
