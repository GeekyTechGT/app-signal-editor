#include "proro/adapters/cli/cli_adapter.h"
#include "proro/adapters/filesystem/fs_adapter.h"
#include "proro/adapters/json/json_adapter.h"
#include "proro/api/proro_api.h"

#include <iostream>
#include <span>

int main(int argc, char* argv[]) {
    using namespace myprj::proro::adapters;

    try {
        auto opts = parse_args(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));

        if (opts.show_help) {
            print_usage(argv[0]);
            return 0;
        }

        // Wire: load config, create adapter, run use case via API
        auto cfg = opts.config_path.empty()
                       ? ProroConfig{}
                       : ProroConfig::from_file(opts.config_path);

        FsAdapter repository(cfg.data_dir);
        auto result = myprj::proro::api::run(repository, opts.id, opts.name);

        if (!result.is_ok()) {
            std::cerr << "[ERROR] " << result.message << '\n';
            return 1;
        }

        std::cout << "[OK] Done.\n";
        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }
}
