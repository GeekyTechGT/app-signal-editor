#pragma once

/**
 * @file signal_editor_api.h
 * @brief Public facade for the Signal Editor module.
 *
 * Applications include this header to consume the domain model, repository
 * port, and the primary use-case service without depending on internal folder
 * structure details.
 */

#include "signal_editor/core/domain/sample_point.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/core/usecases/signal_editor_service.h"
#include "signal_editor/ports/signal_repository.h"

namespace myprj::signal_editor::api {

/**
 * @brief Convenience alias that exposes the main orchestration service.
 */
using Service = SignalEditorService;

}  // namespace myprj::signal_editor::api
