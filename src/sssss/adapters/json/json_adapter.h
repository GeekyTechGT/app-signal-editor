#pragma once

#include "sssss/core/domain/my_entity.h"

#include <nlohmann/json.hpp>
#include <filesystem>

namespace myprj::sssss::adapters {

// --- JSON config adapter ----------------------------------------------------
// Reads module configuration from a JSON file.
struct SssssConfig {
    std::filesystem::path data_dir;
    // TODO: add more config fields

    static SssssConfig from_file(const std::filesystem::path& path);
    static SssssConfig from_json(const nlohmann::json& j);
};

}  // namespace myprj::sssss::adapters
