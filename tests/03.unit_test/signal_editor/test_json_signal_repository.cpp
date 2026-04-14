#include "signal_editor/adapters/filesystem/json_signal_repository.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace myprj::signal_editor;
using namespace myprj::signal_editor::adapters;

namespace {

std::filesystem::path make_temp_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    return path;
}

void write_file(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
}

}  // namespace

TEST(JsonSignalRepositoryTest, LoadJsonWithEnumeratedLabels) {
    auto path = make_temp_path("se_test_signals.json");
    write_file(path, R"({
  "signals": [
    {
      "name": "enable",
      "interpolation": "step",
      "enumeration": [
        {"label": "FALSE", "value": 0},
        {"label": "TRUE", "value": 1}
      ],
      "samples": [
        {"t": 0.0, "y": "FALSE"},
        {"t": 1.0, "y": "TRUE"}
      ]
    }
  ]
})");

    JsonSignalRepository repository;
    auto library = repository.load(path);

    ASSERT_EQ(library.size(), 1u);
    const auto& signal = library.at(0);
    EXPECT_TRUE(signal.is_enumerated());
    EXPECT_EQ(signal.label_for_value(signal.samples()[0].y), "FALSE");
    EXPECT_EQ(signal.label_for_value(signal.samples()[1].y), "TRUE");
    std::filesystem::remove(path);
}
