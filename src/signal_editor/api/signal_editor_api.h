#pragma once

// Public façade — the only header `apps/` should include from the core.
//
// Re-exports the use case orchestrator and the port that adapters must
// implement so that wiring code stays as small as possible.

#include "signal_editor/core/domain/sample_point.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/core/usecases/signal_editor_service.h"
#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::api {

// Convenience alias so consumers can write `api::Service`.
using Service = SignalEditorService;

}  // namespace myprj::signal_editor::api
