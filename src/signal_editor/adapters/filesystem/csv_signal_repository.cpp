#include "signal_editor/adapters/filesystem/csv_signal_repository.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace myprj::signal_editor::adapters {

namespace {

// --- string helpers ---------------------------------------------------------
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
    current.reserve(line.size());
    for (char c : line) {
        if (c == ',') {
            out.push_back(trim(current));
            current.clear();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    out.push_back(trim(current));
    return out;
}

std::optional<double> try_parse_double(const std::string& s) {
    if (s.empty()) {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        double value = std::stod(s, &consumed);
        if (consumed != s.size()) {
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

}  // namespace

SignalLibrary CsvSignalRepository::load(const std::filesystem::path& source) {
    std::ifstream in(source);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open CSV file: " + source.string());
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        rows.push_back(split_csv_line(line));
    }
    if (rows.empty()) {
        throw std::runtime_error("CSV file is empty: " + source.string());
    }

    // --- Detect header --------------------------------------------------
    bool has_header = !row_is_numeric(rows.front());
    std::vector<std::string> headers;
    std::size_t data_start = 0;
    if (has_header) {
        headers = rows.front();
        data_start = 1;
        if (rows.size() < 2) {
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

    // --- Allocate per-signal value vectors ------------------------------
    std::vector<double> time;
    time.reserve(rows.size() - data_start);
    std::vector<std::vector<double>> values(ncols - 1);
    for (auto& v : values) {
        v.reserve(rows.size() - data_start);
    }

    double previous_t = 0.0;
    bool first = true;
    for (std::size_t i = data_start; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.size() != ncols) {
            throw std::runtime_error("Inconsistent column count at row " + std::to_string(i + 1));
        }
        auto t_opt = try_parse_double(row[0]);
        if (!t_opt.has_value()) {
            throw std::runtime_error("Non-numeric time at row " + std::to_string(i + 1));
        }
        if (!first && !(*t_opt > previous_t)) {
            throw std::runtime_error("Time column is not strictly increasing at row " +
                                     std::to_string(i + 1));
        }
        previous_t = *t_opt;
        first = false;
        time.push_back(*t_opt);

        for (std::size_t c = 1; c < ncols; ++c) {
            auto v_opt = try_parse_double(row[c]);
            if (!v_opt.has_value()) {
                throw std::runtime_error("Non-numeric value at row " + std::to_string(i + 1) +
                                         " column " + std::to_string(c + 1));
            }
            values[c - 1].push_back(*v_opt);
        }
    }

    // --- Build the library ----------------------------------------------
    SignalLibrary library;
    for (std::size_t c = 0; c < values.size(); ++c) {
        std::string name = has_header ? headers[c + 1] : "signal_" + std::to_string(c + 1);
        if (name.empty()) {
            name = "signal_" + std::to_string(c + 1);
        }
        library.add(Signal::from_vectors(name, time, values[c]));
    }
    return library;
}

myprj::Result CsvSignalRepository::save(const std::filesystem::path& destination,
                                        const SignalLibrary& library) {
    try {
        if (library.empty()) {
            return myprj::Result::error("Cannot save an empty library");
        }

        // --- Build the union of every signal's time vector --------------
        std::set<double> time_union;
        for (const auto& s : library.items()) {
            for (const auto& sample : s.samples()) {
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

        // --- Header -----------------------------------------------------
        out << "time";
        for (const auto& s : library.items()) {
            out << ',' << s.name();
        }
        out << '\n';

        // --- Rows -------------------------------------------------------
        out << std::setprecision(12);
        for (double t : time_union) {
            out << t;
            for (const auto& s : library.items()) {
                out << ',' << s.interpolate(t);
            }
            out << '\n';
        }

        return myprj::Result::ok();
    } catch (const std::exception& ex) {
        return myprj::Result::error(ex.what());
    }
}

}  // namespace myprj::signal_editor::adapters
