#pragma once

#include "my_module/core/domain/my_entity.h"

#include <nlohmann/json.hpp>
#include <filesystem>

namespace myprj::my_module::adapters {

// --- JSON config adapter ----------------------------------------------------
// Reads module configuration from a JSON file.
struct MyModuleConfig {
    std::filesystem::path data_dir;
    // TODO: add more config fields

    static MyModuleConfig from_file(const std::filesystem::path& path);
    static MyModuleConfig from_json(const nlohmann::json& j);
};

}  // namespace myprj::my_module::adapters
