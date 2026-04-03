#pragma once

#include "proro/core/domain/my_entity.h"

#include <nlohmann/json.hpp>
#include <filesystem>

namespace myprj::proro::adapters {

// --- JSON config adapter ----------------------------------------------------
// Reads module configuration from a JSON file.
struct ProroConfig {
    std::filesystem::path data_dir;
    // TODO: add more config fields

    static ProroConfig from_file(const std::filesystem::path& path);
    static ProroConfig from_json(const nlohmann::json& j);
};

}  // namespace myprj::proro::adapters
