#pragma once

#include "common/core/domain/common_types.h"

#include <span>
#include <string>

namespace myprj::sssss::adapters {

// --- CLI adapter (driving adapter) ------------------------------------------
// Parses argv and dispatches to the use case.
// Owns the mapping from exit codes to error messages.
struct CliOptions {
    std::string id;
    std::string name;
    std::string config_path;
    bool show_help{false};
};

CliOptions parse_args(std::span<const char* const> argv);
void print_usage(const std::string& program_name);

}  // namespace myprj::sssss::adapters
