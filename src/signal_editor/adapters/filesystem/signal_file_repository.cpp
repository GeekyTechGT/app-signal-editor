#include "signal_editor/adapters/filesystem/signal_file_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_codec.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace myprj::signal_editor::adapters {

namespace {
using nlohmann::json;

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
            const std::size_t value_end = attributes.find('\"', value_begin);
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
            row.push_back(tabular::trim_copy(decode_xml_entities(raw_data)));
            ++current_column;
        }
        if (!row.empty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}

SignalLibrary load_spreadsheet_xml(const std::filesystem::path& source) {
    return tabular::rows_to_library(spreadsheet_xml_to_rows(read_text_file(source)));
}

myprj::Result save_spreadsheet_xml(const std::filesystem::path& destination,
                                   const SignalLibrary& library) {
    try {
        const auto rows = tabular::library_to_rows(library);
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
                const bool numeric = tabular::try_parse_double(cell).has_value();
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
        const auto interpolation = tabular::parse_interpolation_mode(signal_node.value("interpolation", "linear"));

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
                const auto resolved = tabular::resolve_enumerated_cell(sample_node.at("y").get<std::string>(), mapping);
                if (!resolved.has_value()) {
                    throw std::runtime_error("Unknown JSON enumerated value for signal " + name);
                }
                enumeration = std::move(mapping);
                y = *resolved;
            } else if (sample_node.contains("label")) {
                auto mapping = enumeration;
                const auto resolved = tabular::resolve_enumerated_cell(sample_node.at("label").get<std::string>(), mapping);
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
            signal_node["interpolation"] = tabular::interpolation_mode_to_text(signal.interpolation());
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
    return tabular::to_lower_copy(path.extension().string());
}

}  // namespace

SignalLibrary SignalFileRepository::load(const std::filesystem::path& source) {
    const std::string extension = extension_lowercase(source);
    if (extension == ".csv") {
        return tabular::load_delimited_file(source, ',');
    }
    if (extension == ".tsv" || extension == ".txt") {
        return tabular::load_delimited_file(source, '\t');
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
        return tabular::save_delimited_file(destination, library, ',');
    }
    if (extension == ".tsv" || extension == ".txt") {
        return tabular::save_delimited_file(destination, library, '\t');
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
