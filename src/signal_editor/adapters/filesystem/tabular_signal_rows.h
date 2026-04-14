#pragma once

#include "signal_editor/core/domain/result.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace myprj::signal_editor::adapters::tabular_rows {

std::string trim_copy(std::string_view text);
std::optional<double> try_parse_double(const std::string& text);
std::string to_lower_copy(std::string text);
Signal::InterpolationMode parse_interpolation_mode(const std::string& raw);
const char* interpolation_mode_to_text(Signal::InterpolationMode interpolation);
std::optional<double> resolve_enumerated_cell(const std::string& cell,
                                              std::vector<Signal::EnumerationEntry>& mapping);

SignalLibrary rows_to_library(const std::vector<std::vector<std::string>>& rows);
std::vector<std::vector<std::string>> library_to_rows(const SignalLibrary& library);

}  // namespace myprj::signal_editor::adapters::tabular_rows
