#pragma once

#include "signal_editor/ports/signal_repository.h"
#include "signal_editor/adapters/filesystem/workbook_model.h"

namespace signal_editor::adapters {

class SpreadsheetXmlSignalRepository : public ISignalRepository {
public:
    WorkbookDocument load_workbook(const std::filesystem::path& source);
    SignalLibrary load(const std::filesystem::path& source) override;
    Result save_workbook(const std::filesystem::path& destination,
                         const WorkbookDocument& workbook);
    Result save(const std::filesystem::path& destination,
                       const SignalLibrary& library) override;
};

}  // namespace signal_editor::adapters
