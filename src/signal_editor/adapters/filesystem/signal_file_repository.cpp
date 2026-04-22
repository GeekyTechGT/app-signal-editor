#include "signal_editor/adapters/filesystem/signal_file_repository.h"

#include "signal_editor/adapters/filesystem/csv_signal_repository.h"
#include "signal_editor/adapters/filesystem/delimited_signal_repository.h"
#include "signal_editor/adapters/filesystem/json_signal_repository.h"
#include "signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h"
#include "signal_editor/adapters/filesystem/tabular_signal_rows.h"
#include "signal_editor/adapters/filesystem/xlsx_signal_repository.h"

#include <stdexcept>
#include <string>

namespace signal_editor::adapters {

namespace {
std::string extension_lowercase(const std::filesystem::path& path) {
    return tabular_rows::to_lower_copy(path.extension().string());
}
}  // namespace

SignalLibrary SignalFileRepository::load(const std::filesystem::path& source) {
    const std::string extension = extension_lowercase(source);
    if (extension == ".csv") {
        CsvSignalRepository repository;
        return repository.load(source);
    }
    if (extension == ".tsv" || extension == ".txt") {
        DelimitedSignalRepository repository('	');
        return repository.load(source);
    }
    if (extension == ".json") {
        JsonSignalRepository repository;
        return repository.load(source);
    }
    if (extension == ".xml") {
        SpreadsheetXmlSignalRepository repository;
        return repository.load(source);
    }
    if (extension == ".xlsx") {
        XlsxSignalRepository repository;
        return repository.load(source);
    }
    throw std::runtime_error("Unsupported import format: " + source.extension().string());
}

signal_editor::Result SignalFileRepository::save(const std::filesystem::path& destination,
                                         const SignalLibrary& library) {
    const std::string extension = extension_lowercase(destination);
    if (extension == ".csv") {
        CsvSignalRepository repository;
        return repository.save(destination, library);
    }
    if (extension == ".tsv" || extension == ".txt") {
        DelimitedSignalRepository repository('	');
        return repository.save(destination, library);
    }
    if (extension == ".json") {
        JsonSignalRepository repository;
        return repository.save(destination, library);
    }
    if (extension == ".xml") {
        SpreadsheetXmlSignalRepository repository;
        return repository.save(destination, library);
    }
    if (extension == ".xlsx") {
        XlsxSignalRepository repository;
        return repository.save(destination, library);
    }
    return signal_editor::Result::error("Unsupported export format: " + destination.extension().string());
}

}  // namespace signal_editor::adapters
