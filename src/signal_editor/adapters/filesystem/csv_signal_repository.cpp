#include "signal_editor/adapters/filesystem/csv_signal_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_codec.h"

namespace myprj::signal_editor::adapters {

SignalLibrary CsvSignalRepository::load(const std::filesystem::path& source) {
    return tabular::load_delimited_file(source, ',');
}

myprj::Result CsvSignalRepository::save(const std::filesystem::path& destination,
                                        const SignalLibrary& library) {
    return tabular::save_delimited_file(destination, library, ',');
}

}  // namespace myprj::signal_editor::adapters
