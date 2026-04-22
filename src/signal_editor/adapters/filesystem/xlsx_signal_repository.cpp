#include "signal_editor/adapters/filesystem/xlsx_signal_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_rows.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef SIGNAL_EDITOR_HAS_LIBZIP
#include <zip.h>
#endif

namespace signal_editor::adapters {

namespace {
#ifdef SIGNAL_EDITOR_HAS_LIBZIP

constexpr const char* kMetadataSheetName = "METADATA";

struct SignalMetadataRow {
    std::string sheet_name;
    std::string signal_name;
    std::string enum_label;
    std::string enum_value;
};

std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(12) << value;
    return stream.str();
}

std::vector<SignalMetadataRow> parse_metadata_rows(const std::vector<std::vector<std::string>>& rows);
void apply_workbook_metadata(WorkbookDocument& workbook,
                             const std::vector<SignalMetadataRow>& metadata);

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

std::size_t find_element_open(const std::string& text, const std::string& tag, std::size_t start) {
    std::size_t pos = start;
    const std::string token = "<" + tag;
    while ((pos = text.find(token, pos)) != std::string::npos) {
        const std::size_t next = pos + token.size();
        if (next >= text.size()) {
            return pos;
        }
        const char trailing = text[next];
        if (std::isspace(static_cast<unsigned char>(trailing)) || trailing == '/' || trailing == '>') {
            return pos;
        }
        pos = next;
    }
    return std::string::npos;
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

std::string extract_attribute(const std::string& tag_block,
                              const std::vector<std::string>& names) {
    const std::string header = tag_block.substr(0, tag_block.find('>'));
    for (const auto& name : names) {
        const std::string token = name + "=\"";
        const std::size_t begin = header.find(token);
        if (begin == std::string::npos) {
            continue;
        }
        const std::size_t value_begin = begin + token.size();
        const std::size_t value_end = header.find('"', value_begin);
        if (value_end != std::string::npos) {
            return header.substr(value_begin, value_end - value_begin);
        }
    }
    return {};
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

std::size_t column_reference_to_index(const std::string& reference) {
    std::size_t index = 0;
    for (char ch : reference) {
        if (!std::isalpha(static_cast<unsigned char>(ch))) {
            break;
        }
        const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        index = index * 26 + static_cast<std::size_t>(upper - 'A' + 1);
    }
    return index == 0 ? 1U : index;
}

std::string column_index_to_reference(std::size_t index) {
    std::string out;
    while (index > 0) {
        --index;
        out.insert(out.begin(), static_cast<char>('A' + (index % 26)));
        index /= 26;
    }
    return out.empty() ? "A" : out;
}

std::string normalise_zip_target(std::string target) {
    while (target.rfind("../", 0) == 0) {
        target.erase(0, 3);
    }
    if (target.rfind("xl/", 0) != 0) {
        target = "xl/" + target;
    }
    return target;
}

std::string read_zip_entry(zip_t* archive, const char* entry_name) {
    zip_stat_t stat{};
    if (zip_stat(archive, entry_name, ZIP_FL_ENC_GUESS, &stat) != 0) {
        throw std::runtime_error(std::string("Missing XLSX entry: ") + entry_name);
    }

    zip_file_t* file = zip_fopen(archive, entry_name, ZIP_FL_ENC_GUESS);
    if (file == nullptr) {
        throw std::runtime_error(std::string("Cannot open XLSX entry: ") + entry_name);
    }

    std::string content;
    content.resize(static_cast<std::size_t>(stat.size));
    const zip_int64_t read = zip_fread(file, content.data(), stat.size);
    zip_fclose(file);
    if (read < 0 || static_cast<zip_uint64_t>(read) != stat.size) {
        throw std::runtime_error(std::string("Cannot read XLSX entry: ") + entry_name);
    }
    return content;
}

std::vector<std::string> parse_shared_strings(const std::string& xml) {
    std::vector<std::string> shared_strings;
    std::size_t pos = 0;
    while (true) {
        std::size_t next = std::string::npos;
        const std::string si_block = extract_tag_block(xml, "si", pos, &next);
        if (next == std::string::npos) {
            break;
        }
        pos = next;

        std::string text;
        std::size_t t_pos = 0;
        while (true) {
            std::size_t t_next = std::string::npos;
            const std::string t_block = extract_tag_block(si_block, "t", t_pos, &t_next);
            if (t_next == std::string::npos) {
                break;
            }
            t_pos = t_next;
            text += decode_xml_entities(extract_between(t_block, "<t", "</t>"));
        }
        shared_strings.push_back(text);
    }
    return shared_strings;
}

std::vector<std::pair<std::string, std::string>> parse_sheet_targets(const std::string& workbook_xml,
                                                                     const std::string& rels_xml) {
    std::vector<std::pair<std::string, std::string>> sheets;
    std::vector<std::pair<std::string, std::string>> relationships;

    std::size_t rel_pos = 0;
    while (true) {
        const std::size_t open = find_element_open(rels_xml, "Relationship", rel_pos);
        if (open == std::string::npos) {
            break;
        }
        const std::size_t open_end = rels_xml.find('>', open);
        if (open_end == std::string::npos) {
            throw std::runtime_error("Malformed XLSX workbook relationships");
        }
        const bool self_closing = open_end > open && rels_xml[open_end - 1] == '/';
        std::size_t block_end = open_end + 1;
        if (!self_closing) {
            const std::size_t close = rels_xml.find("</Relationship>", open_end);
            if (close == std::string::npos) {
                throw std::runtime_error("Malformed XLSX workbook relationships");
            }
            block_end = close + std::string("</Relationship>").size();
        }
        const std::string rel_block = rels_xml.substr(open, block_end - open);
        rel_pos = block_end;

        const std::string id = extract_attribute(rel_block, {"Id"});
        const std::string type = extract_attribute(rel_block, {"Type"});
        const std::string target = extract_attribute(rel_block, {"Target"});
        if (type.find("/worksheet") != std::string::npos) {
            relationships.emplace_back(id, normalise_zip_target(target));
        }
    }

    std::size_t pos = 0;
    while (true) {
        const std::size_t open = find_element_open(workbook_xml, "sheet", pos);
        if (open == std::string::npos) {
            break;
        }
        const std::size_t open_end = workbook_xml.find('>', open);
        if (open_end == std::string::npos) {
            throw std::runtime_error("Malformed XLSX workbook sheets");
        }
        const bool self_closing = open_end > open && workbook_xml[open_end - 1] == '/';
        std::size_t block_end = open_end + 1;
        if (!self_closing) {
            const std::size_t close = workbook_xml.find("</sheet>", open_end);
            if (close == std::string::npos) {
                throw std::runtime_error("Malformed XLSX workbook sheets");
            }
            block_end = close + std::string("</sheet>").size();
        }
        const std::string sheet_block = workbook_xml.substr(open, block_end - open);
        pos = block_end;

        const std::string name = decode_xml_entities(extract_attribute(sheet_block, {"name"}));
        const std::string rel_id = extract_attribute(sheet_block, {"r:id"});
        const auto relationship = std::find_if(
            relationships.begin(), relationships.end(),
            [&](const auto& item) { return item.first == rel_id; });
        if (relationship == relationships.end()) {
            throw std::runtime_error("XLSX workbook sheet relationship is missing");
        }
        sheets.emplace_back(name, relationship->second);
    }
    return sheets;
}

std::vector<std::vector<std::string>> worksheet_xml_to_rows(const std::string& xml,
                                                            const std::vector<std::string>& shared_strings) {
    const std::string sheet_data = extract_between(xml, "<sheetData", "</sheetData>");
    if (sheet_data.empty()) {
        throw std::runtime_error("XLSX worksheet contains no sheetData");
    }

    std::vector<std::vector<std::string>> rows;
    std::size_t row_pos = 0;
    while (true) {
        std::size_t next_row = std::string::npos;
        const std::string row_block = extract_tag_block(sheet_data, "row", row_pos, &next_row);
        if (next_row == std::string::npos) {
            break;
        }
        row_pos = next_row;

        std::vector<std::string> row;
        std::size_t cell_pos = 0;
        std::size_t current_column = 1;
        while (true) {
            std::size_t next_cell = std::string::npos;
            const std::string cell_block = extract_tag_block(row_block, "c", cell_pos, &next_cell);
            if (next_cell == std::string::npos) {
                break;
            }
            cell_pos = next_cell;

            const std::string ref = extract_attribute(cell_block, {"r"});
            const std::size_t indexed_column = ref.empty() ? current_column : column_reference_to_index(ref);
            if (indexed_column > current_column) {
                row.resize(indexed_column - 1U);
                current_column = indexed_column;
            }

            const std::string type = extract_attribute(cell_block, {"t"});
            std::string value;
            if (type == "inlineStr") {
                value = decode_xml_entities(extract_between(cell_block, "<t", "</t>"));
            } else if (type == "s") {
                const std::string raw_index = extract_between(cell_block, "<v", "</v>");
                const auto parsed_index = tabular_rows::try_parse_double(raw_index);
                if (!parsed_index.has_value()) {
                    throw std::runtime_error("XLSX shared string index is not numeric");
                }
                const std::size_t shared_index = static_cast<std::size_t>(*parsed_index);
                if (shared_index >= shared_strings.size()) {
                    throw std::runtime_error("XLSX shared string index out of range");
                }
                value = shared_strings[shared_index];
            } else {
                value = decode_xml_entities(extract_between(cell_block, "<v", "</v>"));
            }
            row.push_back(tabular_rows::trim_copy(value));
            ++current_column;
        }

        if (!row.empty()) {
            rows.push_back(std::move(row));
        }
    }

    return rows;
}

WorkbookDocument load_xlsx_workbook(const std::filesystem::path& source) {
    int error_code = ZIP_ER_OK;
    zip_t* archive = zip_open(source.string().c_str(), ZIP_RDONLY, &error_code);
    if (archive == nullptr) {
        throw std::runtime_error("Cannot open XLSX file: " + source.string());
    }

    try {
        const std::string workbook_xml = read_zip_entry(archive, "xl/workbook.xml");
        const std::string rels_xml = read_zip_entry(archive, "xl/_rels/workbook.xml.rels");

        std::vector<std::string> shared_strings;
        try {
            shared_strings = parse_shared_strings(read_zip_entry(archive, "xl/sharedStrings.xml"));
        } catch (const std::exception&) {
            shared_strings.clear();
        }

        WorkbookDocument workbook;
        std::vector<SignalMetadataRow> metadata;
        for (const auto& [name, target] : parse_sheet_targets(workbook_xml, rels_xml)) {
            const auto rows = worksheet_xml_to_rows(read_zip_entry(archive, target.c_str()), shared_strings);
            if (rows.empty()) {
                continue;
            }
            if (name == kMetadataSheetName) {
                metadata = parse_metadata_rows(rows);
                continue;
            }
            workbook.sheets.push_back(WorkbookSheet{name, tabular_rows::rows_to_library(rows)});
        }
        zip_close(archive);

        if (workbook.sheets.empty()) {
            throw std::runtime_error("XLSX workbook contains no tabular sheets");
        }
        apply_workbook_metadata(workbook, metadata);
        workbook.explicit_sheet_names = true;
        return workbook;
    } catch (...) {
        zip_close(archive);
        throw;
    }
}

void add_zip_string(zip_t* archive, const char* entry_name, const std::string& content) {
    auto* owned_buffer = new char[content.size()];
    if (!content.empty()) {
        std::memcpy(owned_buffer, content.data(), content.size());
    }

    zip_source_t* source = zip_source_buffer(archive, owned_buffer, content.size(), 1);
    if (source == nullptr) {
        delete[] owned_buffer;
        throw std::runtime_error(std::string("Cannot prepare XLSX entry: ") + entry_name);
    }
    if (zip_file_add(archive, entry_name, source, ZIP_FL_OVERWRITE) < 0) {
        zip_source_free(source);
        throw std::runtime_error(std::string("Cannot add XLSX entry: ") + entry_name);
    }
}

std::string make_content_types_xml(std::size_t sheet_count) {
    std::ostringstream out;
    out << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
        << "\n<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        << R"(  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)"
        << "\n"
        << R"(  <Default Extension="xml" ContentType="application/xml"/>)"
        << "\n"
        << R"(  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>)"
        << "\n"
        << R"(  <Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>)"
        << "\n";
    for (std::size_t index = 0; index < sheet_count; ++index) {
        out << "  <Override PartName=\"/xl/worksheets/sheet" << (index + 1)
            << ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n";
    }
    out << "</Types>\n";
    return out.str();
}

std::string make_root_rels_xml() {
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>\n"
        "</Relationships>\n";
}

std::string make_workbook_xml(const WorkbookDocument& workbook) {
    std::ostringstream out;
    out << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
        << "\n"
        << R"(<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
        << "\n  <sheets>\n";
    for (std::size_t index = 0; index < workbook.sheets.size(); ++index) {
        const std::string name =
            workbook.sheets[index].name.empty() ? "Sheet" + std::to_string(index + 1) : workbook.sheets[index].name;
        out << "    <sheet name=\"" << encode_xml_entities(name)
            << "\" sheetId=\"" << (index + 1)
            << "\" r:id=\"rId" << (index + 1) << "\"/>\n";
    }
    out << "  </sheets>\n</workbook>\n";
    return out.str();
}

std::string make_workbook_rels_xml(std::size_t sheet_count) {
    std::ostringstream out;
    out << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
        << "\n<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n";
    for (std::size_t index = 0; index < sheet_count; ++index) {
        out << "  <Relationship Id=\"rId" << (index + 1)
            << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\""
            << " Target=\"worksheets/sheet" << (index + 1) << ".xml\"/>\n";
    }
    out << "</Relationships>\n";
    return out.str();
}

std::string make_styles_xml() {
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
        "  <fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>\n"
        "  <fills count=\"1\"><fill><patternFill patternType=\"none\"/></fill></fills>\n"
        "  <borders count=\"1\"><border/></borders>\n"
        "  <cellStyleXfs count=\"1\"><xf/></cellStyleXfs>\n"
        "  <cellXfs count=\"1\"><xf xfId=\"0\"/></cellXfs>\n"
        "</styleSheet>\n";
}

std::vector<std::vector<std::string>> library_to_plain_rows(const SignalLibrary& library) {
    if (library.empty()) {
        throw std::runtime_error("Cannot serialise an empty library");
    }

    std::set<double> time_union;
    for (const auto& signal : library.items()) {
        for (const auto& sample : signal.samples()) {
            time_union.insert(sample.t);
        }
    }
    if (time_union.empty()) {
        throw std::runtime_error("Cannot serialise a library with no samples");
    }

    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> header{"time"};
    for (const auto& signal : library.items()) {
        header.push_back(signal.name());
    }
    rows.push_back(std::move(header));

    for (double t : time_union) {
        std::vector<std::string> row{format_number(t)};
        for (const auto& signal : library.items()) {
            row.push_back(format_number(signal.interpolate(t)));
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::vector<SignalMetadataRow> collect_workbook_metadata(const WorkbookDocument& workbook) {
    std::vector<SignalMetadataRow> rows;
    for (const auto& sheet : workbook.sheets) {
        for (std::size_t signal_index = 0; signal_index < sheet.library.size(); ++signal_index) {
            const auto& signal = sheet.library.at(signal_index);
            if (signal.is_enumerated()) {
                for (const auto& entry : signal.enumeration()) {
                    rows.push_back(SignalMetadataRow{
                        sheet.name,
                        signal.name(),
                        entry.label,
                        format_number(entry.value),
                    });
                }
            }
        }
    }
    return rows;
}

std::vector<std::vector<std::string>> metadata_rows_from_workbook(const WorkbookDocument& workbook) {
    const auto metadata = collect_workbook_metadata(workbook);
    if (metadata.empty()) {
        return {};
    }

    std::vector<std::vector<std::string>> rows;
    rows.push_back({"sheet", "signal_name", "enum_label", "enum_value"});
    for (const auto& row : metadata) {
        rows.push_back({
            row.sheet_name,
            row.signal_name,
            row.enum_label,
            row.enum_value,
        });
    }
    return rows;
}

std::vector<SignalMetadataRow> parse_metadata_rows(const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) {
        return {};
    }
    const auto& header = rows.front();
    if (header.size() < 4 ||
        tabular_rows::to_lower_copy(tabular_rows::trim_copy(header[0])) != "sheet" ||
        tabular_rows::to_lower_copy(tabular_rows::trim_copy(header[1])) != "signal_name") {
        throw std::runtime_error("XLSX metadata sheet has an unsupported layout");
    }

    std::vector<SignalMetadataRow> metadata;
    for (std::size_t row_index = 1; row_index < rows.size(); ++row_index) {
        const auto& row = rows[row_index];
        if (row.size() < 4) {
            continue;
        }
        if (tabular_rows::trim_copy(row[0]).empty() || tabular_rows::trim_copy(row[1]).empty()) {
            throw std::runtime_error("XLSX metadata requires sheet and signal_name");
        }
        metadata.push_back(SignalMetadataRow{
            row[0],
            row[1],
            row[2],
            row[3],
        });
    }
    return metadata;
}

void apply_workbook_metadata(WorkbookDocument& workbook,
                             const std::vector<SignalMetadataRow>& metadata) {
    if (metadata.empty()) {
        return;
    }

    std::map<std::pair<std::string, std::string>, std::vector<Signal::EnumerationEntry>> enumerations;

    for (const auto& row : metadata) {
        const auto key = std::make_pair(row.sheet_name, row.signal_name);
        if (!row.enum_label.empty()) {
            const auto parsed_value = tabular_rows::try_parse_double(row.enum_value);
            if (!parsed_value.has_value()) {
                throw std::runtime_error("XLSX metadata enum_value is invalid");
            }
            enumerations[key].push_back(Signal::EnumerationEntry{row.enum_label, *parsed_value});
        }
    }

    for (auto& sheet : workbook.sheets) {
        for (std::size_t signal_index = 0; signal_index < sheet.library.size(); ++signal_index) {
            auto& signal = sheet.library.at(signal_index);
            const auto key = std::make_pair(sheet.name, signal.name());
            if (const auto enum_it = enumerations.find(key); enum_it != enumerations.end()) {
                signal.set_enumeration(enum_it->second);
                signal.set_interpolation(Signal::InterpolationMode::Step);
            }
        }
    }
}

std::string make_sheet_xml_from_rows(const std::vector<std::vector<std::string>>& rows) {
    std::ostringstream out;
    out << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
        << "\n"
        << R"(<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">)"
        << "\n  <sheetData>\n";

    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        out << "    <row r=\"" << (row_index + 1) << "\">\n";
        const auto& row = rows[row_index];
        for (std::size_t column_index = 0; column_index < row.size(); ++column_index) {
            const std::string cell_ref =
                column_index_to_reference(column_index + 1) + std::to_string(row_index + 1);
            const std::string& cell = row[column_index];
            if (tabular_rows::try_parse_double(cell).has_value()) {
                out << "      <c r=\"" << cell_ref << "\"><v>" << encode_xml_entities(cell)
                    << "</v></c>\n";
            } else {
                out << "      <c r=\"" << cell_ref << "\" t=\"inlineStr\"><is><t>"
                    << encode_xml_entities(cell) << "</t></is></c>\n";
            }
        }
        out << "    </row>\n";
    }

    out << "  </sheetData>\n</worksheet>\n";
    return out.str();
}

std::string make_sheet_xml(const SignalLibrary& library) {
    return make_sheet_xml_from_rows(library_to_plain_rows(library));
}

signal_editor::Result save_xlsx_workbook(const std::filesystem::path& destination,
                                         const WorkbookDocument& workbook) {
    if (workbook.sheets.empty()) {
        throw std::runtime_error("Cannot serialise an empty workbook");
    }

    int error_code = ZIP_ER_OK;
    zip_t* archive = zip_open(destination.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error_code);
    if (archive == nullptr) {
        throw std::runtime_error("Cannot open XLSX file for writing: " + destination.string());
    }

    try {
        const auto metadata_rows = metadata_rows_from_workbook(workbook);
        const std::size_t persisted_sheet_count =
            workbook.sheets.size() + (metadata_rows.empty() ? 0U : 1U);

        WorkbookDocument persisted_workbook = workbook;
        if (!metadata_rows.empty()) {
            persisted_workbook.sheets.push_back(WorkbookSheet{kMetadataSheetName, SignalLibrary{}});
        }

        add_zip_string(archive, "[Content_Types].xml", make_content_types_xml(persisted_sheet_count));
        add_zip_string(archive, "_rels/.rels", make_root_rels_xml());
        add_zip_string(archive, "xl/workbook.xml", make_workbook_xml(persisted_workbook));
        add_zip_string(archive, "xl/_rels/workbook.xml.rels", make_workbook_rels_xml(persisted_sheet_count));
        add_zip_string(archive, "xl/styles.xml", make_styles_xml());
        for (std::size_t index = 0; index < workbook.sheets.size(); ++index) {
            add_zip_string(archive,
                           ("xl/worksheets/sheet" + std::to_string(index + 1) + ".xml").c_str(),
                           make_sheet_xml(workbook.sheets[index].library));
        }
        if (!metadata_rows.empty()) {
            add_zip_string(
                archive,
                ("xl/worksheets/sheet" + std::to_string(workbook.sheets.size() + 1) + ".xml").c_str(),
                make_sheet_xml_from_rows(metadata_rows));
        }

        if (zip_close(archive) != 0) {
            throw std::runtime_error("Cannot finalise XLSX archive");
        }
        return signal_editor::Result::ok();
    } catch (...) {
        zip_discard(archive);
        throw;
    }
}

#endif
}  // namespace

WorkbookDocument XlsxSignalRepository::load_workbook(const std::filesystem::path& source) {
#ifdef SIGNAL_EDITOR_HAS_LIBZIP
    return load_xlsx_workbook(source);
#else
    (void)source;
    throw std::runtime_error("Native XLSX support is not available in this build.");
#endif
}

SignalLibrary XlsxSignalRepository::load(const std::filesystem::path& source) {
    return load_workbook(source).sheets.front().library;
}

Result XlsxSignalRepository::save_workbook(const std::filesystem::path& destination,
                                           const WorkbookDocument& workbook) {
#ifdef SIGNAL_EDITOR_HAS_LIBZIP
    try {
        return save_xlsx_workbook(destination, workbook);
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
#else
    (void)destination;
    (void)workbook;
    return signal_editor::Result::error("Native XLSX support is not available in this build.");
#endif
}

Result XlsxSignalRepository::save(const std::filesystem::path& destination,
                                  const SignalLibrary& library) {
    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"Signals", library});
    workbook.explicit_sheet_names = true;
    return save_workbook(destination, workbook);
}

}  // namespace signal_editor::adapters
