#include "cli_adapter.h"

#include <iostream>
#include <stdexcept>

namespace myprj::sssss::adapters {

CliOptions parse_args(std::span<const char* const> argv) {
    CliOptions opts;

    for (std::size_t i = 1; i < argv.size(); ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            opts.show_help = true;
        } else if ((arg == "--id") && i + 1 < argv.size()) {
            opts.id = argv[++i];
        } else if ((arg == "--name") && i + 1 < argv.size()) {
            opts.name = argv[++i];
        } else if ((arg == "--config") && i + 1 < argv.size()) {
            opts.config_path = argv[++i];
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    return opts;
}

void print_usage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n"
              << "Options:\n"
              << "  --id <id>          Entity ID\n"
              << "  --name <name>      Entity name\n"
              << "  --config <path>    Path to JSON config file\n"
              << "  --help             Show this help message\n";
}

}  // namespace myprj::sssss::adapters
