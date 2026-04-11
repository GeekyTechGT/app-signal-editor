#include "signal_editor/adapters/filesystem/csv_signal_repository.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace myprj::signal_editor::adapters {

namespace {
constexpr double kEnumValueEpsilon = 1e-9;

std::string trim(std::string_view sv) {
    auto begin = sv.begin();
    auto end = sv.end();
    while (begin != end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> out;
    std::string current;
    bool in_quotes = false;

    for (std::size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (ch == '\r') {
            continue;
        }
        if (ch == '"') {
            if (in_quotes && index + 1 < line.size() && line[index + 1] == '"') {
                current.push_back('"');
                ++index;
            } else {
                in_quotes = !in_quotes;
            }
            continue;
        }
        if (ch == ',' && !in_quotes) {
            out.push_back(trim(current));
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    out.push_back(trim(current));
    return out;
}

std::string quote_csv_cell(const std::string& cell) {
    const bool requires_quotes = cell.find_first_of(",\"\n\r") != std::string::npos ||
        (!cell.empty() && (std::isspace(static_cast<unsigned char>(cell.front())) ||
                           std::isspace(static_cast<unsigned char>(cell.back()))));
    if (!requires_quotes) {
        return cell;
    }

    std::string escaped;
    escaped.reserve(cell.size() + 2);
    escaped.push_back('"');
    for (char ch : cell) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::optional<double> try_parse_double(const std::string& text) {
    if (text.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size()) {
            return std::nullopt;
        }
        return value;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool row_is_numeric(const std::vector<std::string>& row) {
    for (const auto& cell : row) {
        if (!try_parse_double(cell).has_value()) {
            return false;
        }
    }
    return !row.empty();
}

bool is_metadata_row(const std::vector<std::string>& row) {
    return !row.empty() && !row.front().empty() && row.front().front() == '#';
}

bool is_interpolation_metadata_row(const std::vector<std::string>& row) {
    return !row.empty() && (row.front() == "# interpolation" || row.front() == "#interpolation");
}

bool is_enum_metadata_row(const std::vector<std::string>& row) {
    return !row.empty() && (row.front() == "# enum_map" || row.front() == "#enum_map");
}

Signal::InterpolationMode parse_interpolation_mode(const std::string& raw) {
    std::string lowered;
    lowered.reserve(raw.size());
    for (char ch : raw) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    if (lowered.empty() || lowered == "linear") {
        return Signal::InterpolationMode::Linear;
    }
    if (lowered == "step" || lowered == "zoh" || lowered == "zero_order_hold") {
        return Signal::InterpolationMode::Step;
    }
    throw std::runtime_error("Unsupported interpolation mode: " + raw);
}

const char* interpolation_mode_to_csv(Signal::InterpolationMode interpolation) {
    switch (interpolation) {
    case Signal::InterpolationMode::Linear:
        return "linear";
    case Signal::InterpolationMode::Step:
        return "step";
    }
    return "linear";
}

std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(12) << value;
    return stream.str();
}

std::string escape_enum_token(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (char ch : input) {
        if (ch == '\\' || ch == '|' || ch == ':') {
            output.push_back('\\');
        }
        output.push_back(ch);
    }
    return output;
}

std::vector<std::string> split_escaped(std::string_view text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    bool escape = false;
    for (char ch : text) {
        if (escape) {
            current.push_back(ch);
            escape = false;
            continue;
        }
        if (ch == '\\') {
            escape = true;
            continue;
        }
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    if (escape) {
        current.push_back('\\');
    }
    parts.push_back(current);
    return parts;
}

Signal::EnumerationEntry parse_enum_entry(const std::string& raw) {
    const auto parts = split_escaped(raw, ':');
    if (parts.size() != 2) {
        throw std::runtime_error("Invalid enumerated mapping token: " + raw);
    }
    const auto value = try_parse_double(parts[1]);
    if (!value.has_value()) {
        throw std::runtime_error("Invalid enumerated numeric value: " + raw);
    }
    return Signal::EnumerationEntry{parts[0], *value};
}

std::vector<Signal::EnumerationEntry> parse_enum_mapping_cell(const std::string& raw) {
    if (raw.empty()) {
        return {};
    }

    std::vector<Signal::EnumerationEntry> entries;
    for (const auto& token : split_escaped(raw, '|')) {
        if (!token.empty()) {
            entries.push_back(parse_enum_entry(token));
        }
    }
    return entries;
}

std::string enum_mapping_to_csv(const std::vector<Signal::EnumerationEntry>& entries) {
    std::string out;
    bool first = true;
    for (const auto& entry : entries) {
        if (!first) {
            out.push_back('|');
        }
        first = false;
        out += escape_enum_token(entry.label);
        out.push_back(':');
        out += format_number(entry.value);
    }
    return out;
}

std::optional<Signal::EnumerationEntry> try_parse_inline_enum_value(const std::string& raw) {
    const auto parts = split_escaped(raw, ':');
    if (parts.size() != 2 || parts[0].empty()) {
        return std::nullopt;
    }
    const auto value = try_parse_double(parts[1]);
    if (!value.has_value()) {
        return std::nullopt;
    }
    return Signal::EnumerationEntry{parts[0], *value};
}

void merge_enum_entry(std::vector<Signal::EnumerationEntry>& mapping,
                      const Signal::EnumerationEntry& candidate) {
    const auto by_label = std::find_if(mapping.begin(), mapping.end(),
                                       [&](const Signal::EnumerationEntry& entry) {
                                           return entry.label == candidate.label;
                                       });
    if (by_label != mapping.end()) {
        if (std::fabs(by_label->value - candidate.value) >= kEnumValueEpsilon) {
            throw std::runtime_error("Conflicting enumerated mapping for label: " + candidate.label);
        }
        return;
    }

    const auto by_value = std::find_if(mapping.begin(), mapping.end(),
                                       [&](const Signal::EnumerationEntry& entry) {
                                           return std::fabs(entry.value - candidate.value) < kEnumValueEpsilon;
                                       });
    if (by_value != mapping.end()) {
        throw std::runtime_error("Conflicting enumerated mapping for numeric value: " +
                                 format_number(candidate.value));
    }
    mapping.push_back(candidate);
}

std::optional<double> resolve_enumerated_cell(const std::string& cell,
                                              std::vector<Signal::EnumerationEntry>& mapping) {
    if (const auto numeric = try_parse_double(cell); numeric.has_value()) {
        return numeric;
    }

    if (const auto inline_entry = try_parse_inline_enum_value(cell); inline_entry.has_value()) {
        merge_enum_entry(mapping, *inline_entry);
        return inline_entry->value;
    }

    const auto it = std::find_if(mapping.begin(), mapping.end(),
                                 [&](const Signal::EnumerationEntry& entry) {
                                     return entry.label == cell;
                                 });
    if (it != mapping.end()) {
        return it->value;
    }

    return std::nullopt;
}

std::string value_cell_to_csv(const Signal& signal, double value) {
    if (!signal.is_enumerated()) {
        return format_number(value);
    }
    const auto label = signal.label_for_value(value);
    return label.empty() ? format_number(value) : label;
}

}  // namespace

SignalLibrary CsvSignalRepository::load(const std::filesystem::path& source) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open CSV file: " + source.string());
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            rows.push_back(split_csv_line(line));
        }
    }
    if (rows.empty()) {
        throw std::runtime_error("CSV file is empty: " + source.string());
    }

    std::vector<Signal::InterpolationMode> interpolations;
    std::vector<std::vector<Signal::EnumerationEntry>> enumerations;
    std::size_t row_index = 0;
    while (row_index < rows.size() && is_metadata_row(rows[row_index])) {
        const auto& row = rows[row_index];
        if (is_interpolation_metadata_row(row)) {
            interpolations.clear();
            interpolations.reserve(row.size() > 0 ? row.size() - 1 : 0);
            for (std::size_t i = 1; i < row.size(); ++i) {
                interpolations.push_back(parse_interpolation_mode(row[i]));
            }
        } else if (is_enum_metadata_row(row)) {
            enumerations.clear();
            enumerations.reserve(row.size() > 0 ? row.size() - 1 : 0);
            for (std::size_t i = 1; i < row.size(); ++i) {
                enumerations.push_back(parse_enum_mapping_cell(row[i]));
            }
        }
        ++row_index;
    }
    if (row_index >= rows.size()) {
        throw std::runtime_error("CSV metadata has no content rows");
    }

    const bool has_header = !row_is_numeric(rows[row_index]);
    std::vector<std::string> headers;
    std::size_t data_start = row_index;
    if (has_header) {
        headers = rows[row_index];
        data_start = row_index + 1;
        if (rows.size() <= data_start) {
            throw std::runtime_error("CSV header has no data rows");
        }
    }

    const std::size_t ncols = rows[data_start].size();
    if (ncols < 2) {
        throw std::runtime_error("CSV must contain at least a time column and one signal column");
    }
    if (has_header && headers.size() != ncols) {
        throw std::runtime_error("CSV header column count does not match data column count");
    }
    if (!interpolations.empty() && interpolations.size() != ncols - 1) {
        throw std::runtime_error("Interpolation metadata column count does not match signal column count");
    }
    if (!enumerations.empty() && enumerations.size() != ncols - 1) {
        throw std::runtime_error("Enumeration metadata column count does not match signal column count");
    }
    if (interpolations.empty()) {
        interpolations.assign(ncols - 1, Signal::InterpolationMode::Linear);
    }
    if (enumerations.empty()) {
        enumerations.assign(ncols - 1, {});
    }

    std::vector<double> time;
    time.reserve(rows.size() - data_start);
    std::vector<std::vector<double>> values(ncols - 1);
    for (auto& column : values) {
        column.reserve(rows.size() - data_start);
    }

    double previous_t = 0.0;
    bool first = true;
    for (std::size_t i = data_start; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.size() != ncols) {
            throw std::runtime_error("Inconsistent column count at row " + std::to_string(i + 1));
        }
        const auto t_opt = try_parse_double(row[0]);
        if (!t_opt.has_value()) {
            throw std::runtime_error("Non-numeric time at row " + std::to_string(i + 1));
        }
        if (!first && !(*t_opt > previous_t)) {
            throw std::runtime_error("Time column is not strictly increasing at row " + std::to_string(i + 1));
        }
        previous_t = *t_opt;
        first = false;
        time.push_back(*t_opt);

        for (std::size_t column = 1; column < ncols; ++column) {
            auto& mapping = enumerations[column - 1];
            const auto resolved = resolve_enumerated_cell(row[column], mapping);
            if (!resolved.has_value()) {
                throw std::runtime_error("Unsupported value at row " + std::to_string(i + 1) +
                                         " column " + std::to_string(column + 1) +
                                         ". Use numeric values, labels declared in # enum_map, or inline label:value tokens.");
            }
            values[column - 1].push_back(*resolved);
        }
    }

    SignalLibrary library;
    for (std::size_t column = 0; column < values.size(); ++column) {
        std::string name = has_header ? headers[column + 1] : "signal_" + std::to_string(column + 1);
        if (name.empty()) {
            name = "signal_" + std::to_string(column + 1);
        }

        if (!enumerations[column].empty()) {
            interpolations[column] = Signal::InterpolationMode::Step;
        }

        Signal signal = Signal::from_vectors(name, time, values[column], interpolations[column]);
        if (!enumerations[column].empty()) {
            signal.set_enumeration(enumerations[column]);
        }
        library.add(std::move(signal));
    }
    return library;
}

myprj::Result CsvSignalRepository::save(const std::filesystem::path& destination,
                                        const SignalLibrary& library) {
    try {
        if (library.empty()) {
            return myprj::Result::error("Cannot save an empty library");
        }

        std::set<double> time_union;
        bool has_enumerated_signal = false;
        for (const auto& signal : library.items()) {
            has_enumerated_signal = has_enumerated_signal || signal.is_enumerated();
            for (const auto& sample : signal.samples()) {
                time_union.insert(sample.t);
            }
        }
        if (time_union.empty()) {
            return myprj::Result::error("Cannot save: all signals are empty");
        }

        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return myprj::Result::error("Cannot open file for writing: " + destination.string());
        }

        out << "# interpolation";
        for (const auto& signal : library.items()) {
            out << ',' << interpolation_mode_to_csv(signal.interpolation());
        }
        out << '\n';

        if (has_enumerated_signal) {
            out << "# enum_map";
            for (const auto& signal : library.items()) {
                out << ',' << quote_csv_cell(enum_mapping_to_csv(signal.enumeration()));
            }
            out << '\n';
        }

        out << "time";
        for (const auto& signal : library.items()) {
            out << ',' << quote_csv_cell(signal.name());
        }
        out << '\n';

        for (double t : time_union) {
            out << format_number(t);
            for (const auto& signal : library.items()) {
                out << ',' << quote_csv_cell(value_cell_to_csv(signal, signal.interpolate(t)));
            }
            out << '\n';
        }

        return myprj::Result::ok();
    } catch (const std::exception& ex) {
        return myprj::Result::error(ex.what());
    }
}

}  // namespace myprj::signal_editor::adapters
