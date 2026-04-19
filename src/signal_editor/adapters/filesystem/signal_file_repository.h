#pragma once

#include "signal_editor/ports/signal_repository.h"

namespace signal_editor::adapters {

/**
 * @brief Dispatcher over format-specific filesystem repositories.
 *
 * This adapter owns only format selection based on file extension. Concrete
 * parsing and serialisation live in dedicated per-format adapters so each class
 * has a single persistence responsibility.
 *
 * can load and save engineering data in multiple interchange formats:
 * - `.csv` for the native shared time-axis layout;
 * - `.tsv` / `.txt` for Excel-friendly tab-delimited tables;
 * - `.json` for structured integrations and automation;
 * - `.xml` for SpreadsheetML 2003 workbooks exported by Excel.
 *
 * Native `.xlsx` and `.xls` workbook binaries are intentionally out of scope
 * for this adapter because they require additional archive/spreadsheet
 * dependencies that are not part of the current project baseline.
 */
class SignalFileRepository : public ISignalRepository {
public:
    /**
     * @brief Loads a signal library from any supported file extension.
     * @param source Filesystem path to the source document.
     * @return Parsed signal library.
     */
    SignalLibrary load(const std::filesystem::path& source) override;

    /**
     * @brief Saves a signal library using the format implied by the extension.
     * @param destination Filesystem path that will receive the export.
     * @param library Library to serialise.
     * @return Success or failure information for workflow code.
     */
    Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;
};

}  // namespace signal_editor::adapters
