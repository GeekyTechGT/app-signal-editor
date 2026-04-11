#include "signal_editor/adapters/filesystem/signal_file_repository.h"

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

TEST(SignalFileRepositoryTest, LoadJsonWithEnumeratedLabels) {
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

    SignalFileRepository repository;
    auto library = repository.load(path);

    ASSERT_EQ(library.size(), 1u);
    const auto& signal = library.at(0);
    EXPECT_TRUE(signal.is_enumerated());
    EXPECT_EQ(signal.label_for_value(signal.samples()[0].y), "FALSE");
    EXPECT_EQ(signal.label_for_value(signal.samples()[1].y), "TRUE");
    std::filesystem::remove(path);
}

TEST(SignalFileRepositoryTest, SaveAndReloadTsvPreservesMappings) {
    auto path = make_temp_path("se_test_signals.tsv");

    Signal signal = Signal::create_uniform("state", 0.0, 1.0, 2, 0.0, Signal::InterpolationMode::Step);
    signal.set_enumeration({{"IDLE", 0.0}, {"RUN", 1.0}});
    signal.set_sample_value(1, 1.0);

    SignalLibrary library;
    library.add(signal);

    SignalFileRepository repository;
    ASSERT_TRUE(repository.save(path, library).is_ok());
    auto restored = repository.load(path);

    ASSERT_EQ(restored.size(), 1u);
    EXPECT_TRUE(restored.at(0).is_enumerated());
    EXPECT_EQ(restored.at(0).label_for_value(restored.at(0).samples()[1].y), "RUN");
    std::filesystem::remove(path);
}

TEST(SignalFileRepositoryTest, LoadSpreadsheetXmlTable) {
    auto path = make_temp_path("se_test_signals.xml");
    write_file(path, R"(<?xml version="1.0"?>
<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet" xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">
  <Worksheet ss:Name="Signals">
    <Table>
      <Row><Cell><Data ss:Type="String"># interpolation</Data></Cell><Cell><Data ss:Type="String">step</Data></Cell></Row>
      <Row><Cell><Data ss:Type="String"># enum_map</Data></Cell><Cell><Data ss:Type="String">OFF:0|ON:1</Data></Cell></Row>
      <Row><Cell><Data ss:Type="String">time</Data></Cell><Cell><Data ss:Type="String">mode</Data></Cell></Row>
      <Row><Cell><Data ss:Type="Number">0</Data></Cell><Cell><Data ss:Type="String">OFF</Data></Cell></Row>
      <Row><Cell><Data ss:Type="Number">1</Data></Cell><Cell><Data ss:Type="String">ON</Data></Cell></Row>
    </Table>
  </Worksheet>
</Workbook>)");

    SignalFileRepository repository;
    auto library = repository.load(path);

    ASSERT_EQ(library.size(), 1u);
    EXPECT_TRUE(library.at(0).is_enumerated());
    EXPECT_EQ(library.at(0).label_for_value(library.at(0).samples()[1].y), "ON");
    std::filesystem::remove(path);
}
