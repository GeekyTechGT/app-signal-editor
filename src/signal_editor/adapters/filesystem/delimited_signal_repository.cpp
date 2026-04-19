#include "signal_editor/adapters/filesystem/delimited_signal_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_rows.h"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace signal_editor::adapters {

namespace {
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
            out.push_back(tabular_rows::trim_copy(current));
            current.clear();
            continue;
        }
        current.push_back(ch);
    }

    out.push_back(tabular_rows::trim_copy(current));
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
}  // namespace

DelimitedSignalRepository::DelimitedSignalRepository(char delimiter) noexcept : delimiter_(delimiter) {}

SignalLibrary DelimitedSignalRepository::load(const std::filesystem::path& source) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open delimited file: " + source.string());
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            rows.push_back(split_delimited_line(line, delimiter_));
        }
    }
    return tabular_rows::rows_to_library(rows);
}

signal_editor::Result DelimitedSignalRepository::save(const std::filesystem::path& destination,
                                              const SignalLibrary& library) {
    try {
        const auto rows = tabular_rows::library_to_rows(library);
        std::ofstream out(destination, std::ios::trunc);
        if (!out.is_open()) {
            return signal_editor::Result::error("Cannot open file for writing: " + destination.string());
        }
        for (const auto& row : rows) {
            for (std::size_t index = 0; index < row.size(); ++index) {
                if (index != 0) {
                    out << delimiter_;
                }
                out << quote_delimited_cell(row[index], delimiter_);
            }
            out << '\n';
        }
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

}  // namespace signal_editor::adapters
