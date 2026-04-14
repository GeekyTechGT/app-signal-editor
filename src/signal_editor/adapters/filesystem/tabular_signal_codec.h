#pragma once

#include "signal_editor/core/domain/result.h"
#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace myprj::signal_editor::adapters::tabular {

// Shared tabular codec used by filesystem adapters that persist row/column data.
std::string trim_copy(std::string_view text);
std::string to_lower_copy(std::string text);
std::optional<double> try_parse_double(const std::string& text);
Signal::InterpolationMode parse_interpolation_mode(const std::string& raw);
const char* interpolation_mode_to_text(Signal::InterpolationMode interpolation);
std::optional<double> resolve_enumerated_cell(const std::string& cell,
                                              std::vector<Signal::EnumerationEntry>& mapping);

SignalLibrary rows_to_library(const std::vector<std::vector<std::string>>& rows);
std::vector<std::vector<std::string>> library_to_rows(const SignalLibrary& library);
std::vector<std::string> split_delimited_line(const std::string& line, char delimiter);
std::string quote_delimited_cell(const std::string& cell, char delimiter);

SignalLibrary load_delimited_file(const std::filesystem::path& source, char delimiter);
myprj::Result save_delimited_file(const std::filesystem::path& destination,
                                  const SignalLibrary& library,
                                  char delimiter);

}  // namespace myprj::signal_editor::adapters::tabular
