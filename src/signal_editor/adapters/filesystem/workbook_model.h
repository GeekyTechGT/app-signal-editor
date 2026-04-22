#pragma once

#include "signal_editor/core/domain/signal_library.h"

#include <string>
#include <vector>

namespace signal_editor::adapters {

/**
 * @brief One logical worksheet stored inside a tabular workbook file.
 *
 * The project core still edits one `SignalLibrary` at a time. Workbook-aware
 * adapters therefore expose each persisted sheet as a self-contained library
 * plus a human-readable sheet name.
 */
struct WorkbookSheet {
    std::string name;
    SignalLibrary library;
};

/**
 * @brief Workbook-level persistence payload used by multi-sheet adapters.
 *
 * `explicit_sheet_names` is set when the source format materially stored sheet
 * boundaries or names. CSV uses that flag to decide whether save should emit
 * explicit `# sheet` sections or keep the legacy single-table layout.
 */
struct WorkbookDocument {
    std::vector<WorkbookSheet> sheets;
    bool explicit_sheet_names{false};
};

}  // namespace signal_editor::adapters
