#pragma once

#include "proro/ports/my_port.h"

#include <filesystem>

namespace myprj::proro::adapters {

// --- Filesystem adapter (implements output port) ----------------------------
// Reads/writes entities to the local filesystem.
// Lives outside the hexagon; depends on std::filesystem.
class FsAdapter : public IMyRepository {
public:
    explicit FsAdapter(std::filesystem::path data_dir);

    myprj::Result save(const MyEntity& entity) override;
    std::vector<MyEntity> load_all() override;

private:
    std::filesystem::path data_dir_;
};

}  // namespace myprj::proro::adapters
