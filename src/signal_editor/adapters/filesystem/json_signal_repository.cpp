#include "signal_editor/adapters/filesystem/json_signal_repository.h"

#include "signal_editor/adapters/filesystem/tabular_signal_rows.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
#include <vector>

namespace signal_editor::adapters {

namespace {
using nlohmann::json;
}  // namespace

SignalLibrary JsonSignalRepository::load(const std::filesystem::path& source) {
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
        const auto interpolation = tabular_rows::parse_interpolation_mode(signal_node.value("interpolation", "linear"));

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
                const auto resolved = tabular_rows::resolve_enumerated_cell(sample_node.at("y").get<std::string>(), mapping);
                if (!resolved.has_value()) {
                    throw std::runtime_error("Unknown JSON enumerated value for signal " + name);
                }
                enumeration = std::move(mapping);
                y = *resolved;
            } else if (sample_node.contains("label")) {
                auto mapping = enumeration;
                const auto resolved = tabular_rows::resolve_enumerated_cell(sample_node.at("label").get<std::string>(), mapping);
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

signal_editor::Result JsonSignalRepository::save(const std::filesystem::path& destination,
                                         const SignalLibrary& library) {
    try {
        if (library.empty()) {
            return signal_editor::Result::error("Cannot save an empty library");
        }

        json document;
        document["format"] = "signal-editor";
        document["version"] = 1;
        document["signals"] = json::array();

        for (const auto& signal : library.items()) {
            json signal_node;
            signal_node["name"] = signal.name();
            signal_node["interpolation"] = tabular_rows::interpolation_mode_to_text(signal.interpolation());
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
            return signal_editor::Result::error("Cannot open file for writing: " + destination.string());
        }
        out << std::setw(2) << document << '\n';
        return signal_editor::Result::ok();
    } catch (const std::exception& ex) {
        return signal_editor::Result::error(ex.what());
    }
}

}  // namespace signal_editor::adapters
