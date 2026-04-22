#pragma once

#include "signal_editor/adapters/filesystem/workbook_model.h"
#include "signal_editor/ports/signal_repository.h"

namespace signal_editor::adapters {

/**
 * @brief Native XLSX workbook repository backed by libzip.
 *
 * The adapter intentionally supports the subset needed by the editor:
 * workbook-level sheet enumeration, shared time-axis tables, numeric cells,
 * plain strings, and inline/shared strings. Styling formulas and richer Excel
 * constructs remain out of scope for the waveform editing workflow.
 */
class XlsxSignalRepository : public ISignalRepository {
public:
    WorkbookDocument load_workbook(const std::filesystem::path& source);
    SignalLibrary load(const std::filesystem::path& source) override;
    Result save_workbook(const std::filesystem::path& destination,
                         const WorkbookDocument& workbook);
    Result save(const std::filesystem::path& destination,
                const SignalLibrary& library) override;
};

}  // namespace signal_editor::adapters
