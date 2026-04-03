#include "json_adapter.h"

#include <fstream>
#include <stdexcept>

namespace myprj::proro::adapters {

ProroConfig ProroConfig::from_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path.string());
    return from_json(nlohmann::json::parse(file));
}

ProroConfig ProroConfig::from_json(const nlohmann::json& j) {
    ProroConfig cfg;
    cfg.data_dir = j.value("data_dir", ".");
    return cfg;
}

}  // namespace myprj::proro::adapters
