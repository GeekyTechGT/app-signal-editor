#include "fs_adapter.h"

#include <fstream>
#include <stdexcept>

namespace myprj::sssss::adapters {

FsAdapter::FsAdapter(std::filesystem::path data_dir) : data_dir_(std::move(data_dir)) {}

myprj::Result FsAdapter::save(const MyEntity& entity) {
    // TODO: implement file-based persistence
    (void)entity;
    return myprj::Result::ok();
}

std::vector<MyEntity> FsAdapter::load_all() {
    // TODO: implement file-based loading
    return {};
}

}  // namespace myprj::sssss::adapters
