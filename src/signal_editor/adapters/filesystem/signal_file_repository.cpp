#include "signal_editor/adapters/filesystem/signal_file_repository.h"

#include "signal_editor/adapters/filesystem/csv_signal_repository.h"

#include <nlohmann/json.hpp>

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
using nlohmann::json;
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

std::string to_lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
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

std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(12) << value;
    return stream.str();
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
    const std::string lowered = to_lower_copy(raw);
    if (lowered.empty() || lowered == "linear") {
        return Signal::InterpolationMode::Linear;
    }
    if (lowered == "step" || lowered == "zoh" || lowered == "zero_order_hold") {
        return Signal::InterpolationMode::Step;
    }
    throw std::runtime_error("Unsupported interpolation mode: " + raw);
}

const char* interpolation_mode_to_text(Signal::InterpolationMode interpolation) {
    switch (interpolation) {
    case Signal::InterpolationMode::Linear:
        return "linear";
    case Signal::InterpolationMode::Step:
        return "step";
    }
    return "linear";
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

std::string enum_mapping_to_text(const std::vector<Signal::EnumerationEntry>& entries) {
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

std::string value_cell_to_text(const Signal& signal, double value) {
    if (!signal.is_enumerated()) {
        return format_number(value);
    }
    const auto label = signal.label_for_value(value);
    return label.empty() ? format_number(value) : label;
}

SignalLibrary rows_to_library(const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) {
        throw std::runtime_error("Input document is empty");
    }

    std::vector<Signal::InterpolationMode> interpolations;
    std::vector<std::vector<Signal::EnumerationEntry>> enumerations;
    std::size_t row_index = 0;
    while (row_index < rows.size() && is_metadata_row(rows[row_index])) {
        const auto& row = rows[row_index];
        if (is_interpolation_metadata_row(row)) {
            interpolations.clear();
            for (std::size_t i = 1; i < row.size(); ++i) {
                interpolations.push_back(parse_interpolation_mode(row[i]));
            }
        } else if (is_enum_metadata_row(row)) {
            enumerations.clear();
            for (std::size_t i = 1; i < row.size(); ++i) {
                enumerations.push_back(parse_enum_mapping_cell(row[i]));
            }
        }
        ++row_index;
    }
    if (row_index >= rows.size()) {
        throw std::runtime_error("Metadata rows contain no sample data");
    }

    const bool has_header = !row_is_numeric(rows[row_index]);
    std::vector<std::string> headers;
    std::size_t data_start = row_index;
    if (has_header) {
        headers = rows[row_index];
        data_start = row_index + 1;
        if (rows.size() <= data_start) {
            throw std::runtime_error("Header row has no sample rows");
        }
    }

    const std::size_t ncols = rows[data_start].size();
    if (ncols < 2) {
        throw std::runtime_error("A tabular import needs at least one time column and one signal column");
    }
    if (has_header && headers.size() != ncols) {
        throw std::runtime_error("Header column count does not match data column count");
    }
    if (!interpolations.empty() && interpolations.size() != ncols - 1) {
        throw std::runtime_error("Interpolation metadata column count does not match signal count");
    }
    if (!enumerations.empty() && enumerations.size() != ncols - 1) {
        throw std::runtime_error("Enumeration metadata column count does not match signal count");
    }
    if (interpolations.empty()) {
        interpolations.assign(ncols - 1, Signal::InterpolationMode::Linear);
    }
    if (enumerations.empty()) {
        enumerations.assign(ncols - 1, {});
    }

    std::vector<double> time;
    std::vector<std::vector<double>> values(ncols - 1);
    double previous_t = 0.0;
    bool first = true;
    for (std::size_t row_number = data_start; row_number < rows.size(); ++row_number) {
        const auto& row = rows[row_number];
        if (row.size() != ncols) {
            throw std::runtime_error("Inconsistent column count at row " + std::to_string(row_number + 1));
        }
        const auto t = try_parse_double(row[0]);
        if (!t.has_value()) {
            throw std::runtime_error("Non-numeric time at row " + std::to_string(row_number + 1));
        }
        if (!first && !(*t > previous_t)) {
            throw std::runtime_error("Time column is not strictly increasing at row " + std::to_string(row_number + 1));
        }
        previous_t = *t;
        first = false;
        time.push_back(*t);

        for (std::size_t column = 1; column < ncols; ++column) {
            auto& mapping = enumerations[column - 1];
            const auto value = resolve_enumerated_cell(row[column], mapping);
            if (!value.has_value()) {
                throw std::runtime_error("Unsupported value at row " + std::to_string(row_number + 1) +
                                         " column " + std::to_string(column + 1));
            }
            values[column - 1].push_back(*value);
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

std::vector<std::vector<std::string>> library_to_rows(const SignalLibrary& library) {
    if (library.empty()) {
        throw std::runtime_error("Cannot serialise an empty library");
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
        throw std::runtime_error("Cannot serialise a library with no samples");
    }

    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> interpolation_row{"# interpolation"};
    for (const auto& signal : library.items()) {
        interpolation_row.push_back(interpolation_mode_to_text(signal.interpolation()));
    }
    rows.push_back(std::move(interpolation_row));

    if (has_enumerated_signal) {
        std::vector<std::string> enum_row{"# enum_map"};
        for (const auto& signal : library.items()) {
            enum_row.push_back(enum_mapping_to_text(signal.enumeration()));
        }
        rows.push_back(std::move(enum_row));
    }

    std::vector<std::string> header{"time"};
    for (const auto& signal : library.items()) {
        header.push_back(signal.name());
    }
    rows.push_back(std::move(header));

    for (double t : time_union) {
        std::vector<std::string> row{format_number(t)};
        for (const auto& signal : library.items()) {
            row.push_back(value_cell_to_text(signal, signal.interpolate(t)));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::vector<std::string> split_delimited_line(const std::string& line, char delimiter) {
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
        if (ch == delimiter && !in_quotes) {
            out.push_back(trim(current));
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    out.push_back(trim(current));
    return out;
}

std::string quote_delimited_cell(const std::string& cell, char delimiter) {
    const bool requires_quotes = cell.find(delimiter) != std::string::npos ||
        cell.find_first_of("\"\n\r") != std::string::npos ||
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

SignalLibrary load_delimited(const std::filesystem::path& source, char delimiter) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open delimited file: " + source.string());
    }
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            rows.push_back(split_delimited_line(line, delimiter));
        }
    }
    return rows_to_library(rows);
}

myprj::Result save_delimited(const std::filesystem::path& destination,
                             const SignalLibrary& library,
                             char delimiter) {
    try {
        const auto rows = library_to_rows(library);
        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return myprj::Result::error("Cannot open file for writing: " + destination.string());
        }
        for (const auto& row : rows) {
            for (std::size_t index = 0; index < row.size(); ++index) {
                if (index != 0) {
                    out << delimiter;
                }
                out << quote_delimited_cell(row[index], delimiter);
            }
            out << '\n';
        }
        return myprj::Result::ok();
    } catch (const std::exception& ex) {
        return myprj::Result::error(ex.what());
    }
}

std::string read_text_file(const std::filesystem::path& source) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open file: " + source.string());
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string decode_xml_entities(std::string text) {
    const std::pair<const char*, const char*> replacements[] = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&apos;", "'"},
    };
    for (const auto& [from, to] : replacements) {
        std::size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos) {
            text.replace(pos, std::char_traits<char>::length(from), to);
            pos += std::char_traits<char>::length(to);
        }
    }
    return text;
}

std::string encode_xml_entities(std::string text) {
    const std::pair<const char*, const char*> replacements[] = {
        {"&", "&amp;"},
        {"<", "&lt;"},
        {">", "&gt;"},
        {"\"", "&quot;"},
        {"'", "&apos;"},
    };
    for (const auto& [from, to] : replacements) {
        std::size_t pos = 0;
        while ((pos = text.find(from, pos)) != std::string::npos) {
            text.replace(pos, std::char_traits<char>::length(from), to);
            pos += std::char_traits<char>::length(to);
        }
    }
    return text;
}

std::size_t find_tag_open(const std::string& text, const std::string& tag, std::size_t start) {
    return text.find("<" + tag, start);
}

std::string extract_tag_block(const std::string& text,
                              const std::string& tag,
                              std::size_t start,
                              std::size_t* next) {
    const std::size_t open = find_tag_open(text, tag, start);
    if (open == std::string::npos) {
        if (next != nullptr) {
            *next = std::string::npos;
        }
        return {};
    }
    const std::size_t open_end = text.find('>', open);
    const std::size_t close = text.find("</" + tag + ">", open_end);
    if (open_end == std::string::npos || close == std::string::npos) {
        throw std::runtime_error("Malformed XML: missing closing tag for " + tag);
    }
    if (next != nullptr) {
        *next = close + tag.size() + 3;
    }
    return text.substr(open, *next - open);
}

std::string extract_between(const std::string& text,
                            const std::string& open_token,
                            const std::string& close_token,
                            std::size_t start = 0) {
    const std::size_t open = text.find(open_token, start);
    if (open == std::string::npos) {
        return {};
    }
    const std::size_t content_start = text.find('>', open);
    if (content_start == std::string::npos) {
        return {};
    }
    const std::size_t close = text.find(close_token, content_start + 1);
    if (close == std::string::npos) {
        return {};
    }
    return text.substr(content_start + 1, close - content_start - 1);
}

int parse_spreadsheet_index(const std::string& cell_block) {
    const std::string attributes = cell_block.substr(0, cell_block.find('>'));
    const char* names[] = {"ss:Index=\"", "Index=\""};
    for (const char* name : names) {
        const std::size_t pos = attributes.find(name);
        if (pos != std::string::npos) {
            const std::size_t value_begin = pos + std::char_traits<char>::length(name);
            const std::size_t value_end = attributes.find('"', value_begin);
            if (value_end != std::string::npos) {
                return std::stoi(attributes.substr(value_begin, value_end - value_begin));
            }
        }
    }
    return -1;
}

std::vector<std::vector<std::string>> spreadsheet_xml_to_rows(const std::string& xml) {
    const std::size_t table_open = find_tag_open(xml, "Table", 0);
    if (table_open == std::string::npos) {
        throw std::runtime_error("Spreadsheet XML contains no Table element");
    }
    const std::size_t table_close = xml.find("</Table>", table_open);
    if (table_close == std::string::npos) {
        throw std::runtime_error("Spreadsheet XML contains an unterminated Table element");
    }
    const std::string table = xml.substr(table_open, table_close - table_open);

    std::vector<std::vector<std::string>> rows;
    std::size_t row_pos = 0;
    while (true) {
        std::size_t next_row = std::string::npos;
        const std::string row_block = extract_tag_block(table, "Row", row_pos, &next_row);
        if (next_row == std::string::npos) {
            break;
        }
        row_pos = next_row;

        std::vector<std::string> row;
        std::size_t cell_pos = 0;
        int current_column = 1;
        while (true) {
            std::size_t next_cell = std::string::npos;
            const std::string cell_block = extract_tag_block(row_block, "Cell", cell_pos, &next_cell);
            if (next_cell == std::string::npos) {
                break;
            }
            cell_pos = next_cell;

            const int indexed_column = parse_spreadsheet_index(cell_block);
            if (indexed_column > current_column) {
                row.resize(static_cast<std::size_t>(indexed_column - 1));
                current_column = indexed_column;
            }
            const std::string raw_data = extract_between(cell_block, "<Data", "</Data>");
            row.push_back(trim(decode_xml_entities(raw_data)));
            ++current_column;
        }
        if (!row.empty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

SignalLibrary load_spreadsheet_xml(const std::filesystem::path& source) {
    return rows_to_library(spreadsheet_xml_to_rows(read_text_file(source)));
}

myprj::Result save_spreadsheet_xml(const std::filesystem::path& destination,
                                   const SignalLibrary& library) {
    try {
        const auto rows = library_to_rows(library);
        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return myprj::Result::error("Cannot open file for writing: " + destination.string());
        }

        out << "<?xml version=\"1.0\"?>\n";
        out << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
        out << "          xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";
        out << "  <Worksheet ss:Name=\"Signals\">\n";
        out << "    <Table>\n";
        for (const auto& row : rows) {
            out << "      <Row>\n";
            for (const auto& cell : row) {
                const bool numeric = try_parse_double(cell).has_value();
                out << "        <Cell><Data ss:Type=\"" << (numeric ? "Number" : "String") << "\">"
                    << encode_xml_entities(cell) << "</Data></Cell>\n";
            }
            out << "      </Row>\n";
        }
        out << "    </Table>\n";
        out << "  </Worksheet>\n";
        out << "</Workbook>\n";
        return myprj::Result::ok();
    } catch (const std::exception& ex) {
        return myprj::Result::error(ex.what());
    }
}

SignalLibrary load_json_file(const std::filesystem::path& source) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open JSON file: " + source.string());
    }
    json document;
    in >> document;

    const json* signals_node = nullptr;
    if (document.is_object() && document.contains("signals")) {
        signals_node = &document.at("signals");
    } else if (document.is_array()) {
        signals_node = &document;
    } else {
        throw std::runtime_error("JSON input must be an array of signals or an object with a signals array");
    }
    if (!signals_node->is_array()) {
        throw std::runtime_error("JSON signals node must be an array");
    }

    SignalLibrary library;
    for (const auto& signal_node : *signals_node) {
        if (!signal_node.is_object()) {
            throw std::runtime_error("Each JSON signal must be an object");
        }
        const std::string name = signal_node.at("name").get<std::string>();
        const auto interpolation = parse_interpolation_mode(signal_node.value("interpolation", "linear"));

        std::vector<Signal::EnumerationEntry> enumeration;
        if (signal_node.contains("enumeration")) {
            for (const auto& entry_node : signal_node.at("enumeration")) {
                enumeration.push_back(Signal::EnumerationEntry{
                    entry_node.at("label").get<std::string>(),
                    entry_node.at("value").get<double>()
                });
            }
        }

        std::vector<SamplePoint> samples;
        for (const auto& sample_node : signal_node.at("samples")) {
            const double t = sample_node.at("t").get<double>();
            double y = 0.0;
            if (sample_node.contains("y") && sample_node.at("y").is_number()) {
                y = sample_node.at("y").get<double>();
            } else if (sample_node.contains("y") && sample_node.at("y").is_string()) {
                auto mapping = enumeration;
                const auto resolved = resolve_enumerated_cell(sample_node.at("y").get<std::string>(), mapping);
                if (!resolved.has_value()) {
                    throw std::runtime_error("Unknown JSON enumerated value for signal " + name);
                }
                enumeration = std::move(mapping);
                y = *resolved;
            } else if (sample_node.contains("label")) {
                auto mapping = enumeration;
                const auto resolved = resolve_enumerated_cell(sample_node.at("label").get<std::string>(), mapping);
                if (!resolved.has_value()) {
                    throw std::runtime_error("Unknown JSON enumerated label for signal " + name);
                }
                enumeration = std::move(mapping);
                y = *resolved;
            } else {
                throw std::runtime_error("JSON sample must define t and y/label");
            }
            samples.push_back(SamplePoint{t, y});
        }

        Signal signal(name, samples, interpolation);
        if (!enumeration.empty()) {
            signal.set_enumeration(enumeration);
        }
        library.add(std::move(signal));
    }
    return library;
}

myprj::Result save_json_file(const std::filesystem::path& destination,
                             const SignalLibrary& library) {
    try {
        if (library.empty()) {
            return myprj::Result::error("Cannot save an empty library");
        }

        json document;
        document["format"] = "signal-editor";
        document["version"] = 1;
        document["signals"] = json::array();

        for (const auto& signal : library.items()) {
            json signal_node;
            signal_node["name"] = signal.name();
            signal_node["interpolation"] = interpolation_mode_to_text(signal.interpolation());
            if (signal.is_enumerated()) {
                signal_node["enumeration"] = json::array();
                for (const auto& entry : signal.enumeration()) {
                    signal_node["enumeration"].push_back({
                        {"label", entry.label},
                        {"value", entry.value},
                    });
                }
            }
            signal_node["samples"] = json::array();
            for (const auto& sample : signal.samples()) {
                json sample_node;
                sample_node["t"] = sample.t;
                if (signal.is_enumerated()) {
                    const auto label = signal.label_for_value(sample.y);
                    sample_node["y"] = label.empty() ? json(sample.y) : json(label);
                } else {
                    sample_node["y"] = sample.y;
                }
                signal_node["samples"].push_back(std::move(sample_node));
            }
            document["signals"].push_back(std::move(signal_node));
        }

        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return myprj::Result::error("Cannot open file for writing: " + destination.string());
        }
        out << std::setw(2) << document << '\n';
        return myprj::Result::ok();
    } catch (const std::exception& ex) {
        return myprj::Result::error(ex.what());
    }
}

std::string extension_lowercase(const std::filesystem::path& path) {
    return to_lower_copy(path.extension().string());
}

}  // namespace

SignalLibrary SignalFileRepository::load(const std::filesystem::path& source) {
    const std::string extension = extension_lowercase(source);
    if (extension == ".csv") {
        CsvSignalRepository repository;
        return repository.load(source);
    }
    if (extension == ".tsv" || extension == ".txt") {
        return load_delimited(source, '\t');
    }
    if (extension == ".json") {
        return load_json_file(source);
    }
    if (extension == ".xml") {
        return load_spreadsheet_xml(source);
    }
    if (extension == ".xlsx" || extension == ".xls") {
        throw std::runtime_error("Native Excel workbook binaries are not supported yet. Export the sheet as CSV, TSV/TXT, SpreadsheetML XML, or JSON first.");
    }
    throw std::runtime_error("Unsupported import format: " + source.extension().string());
}

myprj::Result SignalFileRepository::save(const std::filesystem::path& destination,
                                         const SignalLibrary& library) {
    const std::string extension = extension_lowercase(destination);
    if (extension == ".csv") {
        CsvSignalRepository repository;
        return repository.save(destination, library);
    }
    if (extension == ".tsv" || extension == ".txt") {
        return save_delimited(destination, library, '\t');
    }
    if (extension == ".json") {
        return save_json_file(destination, library);
    }
    if (extension == ".xml") {
        return save_spreadsheet_xml(destination, library);
    }
    if (extension == ".xlsx" || extension == ".xls") {
        return myprj::Result::error(
            "Native Excel workbook binaries are not supported yet. Save as CSV, TSV/TXT, SpreadsheetML XML, or JSON.");
    }
    return myprj::Result::error("Unsupported export format: " + destination.extension().string());
}

}  // namespace myprj::signal_editor::adapters
