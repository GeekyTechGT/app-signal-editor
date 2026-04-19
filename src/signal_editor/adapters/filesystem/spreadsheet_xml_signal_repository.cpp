#include "signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_rows.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace signal_editor::adapters {

namespace {
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
            row.push_back(tabular_rows::trim_copy(decode_xml_entities(raw_data)));
            ++current_column;
        }
        if (!row.empty()) {
            rows.push_back(std::move(row));
        }
    }
    return rows;
}
}  // namespace

SignalLibrary SpreadsheetXmlSignalRepository::load(const std::filesystem::path& source) {
    return tabular_rows::rows_to_library(spreadsheet_xml_to_rows(read_text_file(source)));
}

signal_editor::Result SpreadsheetXmlSignalRepository::save(const std::filesystem::path& destination,
                                                   const SignalLibrary& library) {
    try {
        const auto rows = tabular_rows::library_to_rows(library);
        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return signal_editor::Result::error("Cannot open file for writing: " + destination.string());
        }

        out << "<?xml version=\"1.0\"?>\n";
        out << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
        out << "          xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";
        out << "  <Worksheet ss:Name=\"Signals\">\n";
        out << "    <Table>\n";
        for (const auto& row : rows) {
            out << "      <Row>\n";
            for (const auto& cell : row) {
                const bool numeric = tabular_rows::try_parse_double(cell).has_value();
                out << "        <Cell><Data ss:Type=\"" << (numeric ? "Number" : "String") << "\">"
                    << encode_xml_entities(cell) << "</Data></Cell>\n";
            }
            out << "      </Row>\n";
        }
        out << "    </Table>\n";
        out << "  </Worksheet>\n";
        out << "</Workbook>\n";
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

}  // namespace signal_editor::adapters
