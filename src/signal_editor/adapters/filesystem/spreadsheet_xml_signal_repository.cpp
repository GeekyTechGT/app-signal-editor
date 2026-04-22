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

std::string extract_attribute(const std::string& tag_block,
                              const char* attribute_name) {
    const std::string header = tag_block.substr(0, tag_block.find('>'));
    const std::string token = std::string(attribute_name) + "=\"";
    const std::size_t begin = header.find(token);
    if (begin == std::string::npos) {
        return {};
    }
    const std::size_t value_begin = begin + token.size();
    const std::size_t value_end = header.find('"', value_begin);
    if (value_end == std::string::npos) {
        return {};
    }
    return header.substr(value_begin, value_end - value_begin);
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

std::vector<std::vector<std::string>> spreadsheet_xml_table_to_rows(const std::string& table) {
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

WorkbookDocument spreadsheet_xml_to_workbook(const std::string& xml) {
    WorkbookDocument workbook;
    std::size_t worksheet_pos = 0;
    std::size_t sheet_index = 0;
    while (true) {
        std::size_t next_worksheet = std::string::npos;
        const std::string worksheet_block =
            extract_tag_block(xml, "Worksheet", worksheet_pos, &next_worksheet);
        if (next_worksheet == std::string::npos) {
            break;
        }
        worksheet_pos = next_worksheet;

        const std::size_t table_open = find_tag_open(worksheet_block, "Table", 0);
        if (table_open == std::string::npos) {
            ++sheet_index;
            continue;
        }
        const std::size_t table_close = worksheet_block.find("</Table>", table_open);
        if (table_close == std::string::npos) {
            throw std::runtime_error("Spreadsheet XML contains an unterminated Table element");
        }
        const std::string table = worksheet_block.substr(table_open, table_close - table_open);
        const auto rows = spreadsheet_xml_table_to_rows(table);
        if (rows.empty()) {
            ++sheet_index;
            continue;
        }

        std::string name = decode_xml_entities(extract_attribute(worksheet_block, "ss:Name"));
        if (name.empty()) {
            name = decode_xml_entities(extract_attribute(worksheet_block, "Name"));
        }
        if (name.empty()) {
            name = "Sheet" + std::to_string(sheet_index + 1);
        }
        workbook.sheets.push_back(WorkbookSheet{name, tabular_rows::rows_to_library(rows)});
        ++sheet_index;
    }

    if (workbook.sheets.empty()) {
        throw std::runtime_error("Spreadsheet XML contains no worksheet tables");
    }
    workbook.explicit_sheet_names = true;
    return workbook;
}
}  // namespace

WorkbookDocument SpreadsheetXmlSignalRepository::load_workbook(const std::filesystem::path& source) {
    return spreadsheet_xml_to_workbook(read_text_file(source));
}

SignalLibrary SpreadsheetXmlSignalRepository::load(const std::filesystem::path& source) {
    return load_workbook(source).sheets.front().library;
}

signal_editor::Result SpreadsheetXmlSignalRepository::save_workbook(
    const std::filesystem::path& destination,
    const WorkbookDocument& workbook) {
    try {
        if (workbook.sheets.empty()) {
            throw std::runtime_error("Cannot serialise an empty workbook");
        }
        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return signal_editor::Result::error("Cannot open file for writing: " + destination.string());
        }

        out << "<?xml version=\"1.0\"?>\n";
        out << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
        out << "          xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";
        for (std::size_t sheet_index = 0; sheet_index < workbook.sheets.size(); ++sheet_index) {
            const auto& sheet = workbook.sheets[sheet_index];
            const auto rows = tabular_rows::library_to_rows(sheet.library);
            const std::string sheet_name =
                sheet.name.empty() ? "Sheet" + std::to_string(sheet_index + 1) : sheet.name;
            out << "  <Worksheet ss:Name=\"" << encode_xml_entities(sheet_name) << "\">\n";
            out << "    <Table>\n";
            for (const auto& row : rows) {
                out << "      <Row>\n";
                for (const auto& cell : row) {
                    const bool numeric = tabular_rows::try_parse_double(cell).has_value();
                    out << "        <Cell><Data ss:Type=\"" << (numeric ? "Number" : "String")
                        << "\">" << encode_xml_entities(cell) << "</Data></Cell>\n";
                }
                out << "      </Row>\n";
            }
            out << "    </Table>\n";
            out << "  </Worksheet>\n";
        }
        out << "</Workbook>\n";
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

signal_editor::Result SpreadsheetXmlSignalRepository::save(const std::filesystem::path& destination,
                                                   const SignalLibrary& library) {
    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"Signals", library});
    workbook.explicit_sheet_names = true;
    return save_workbook(destination, workbook);
}

}  // namespace signal_editor::adapters
