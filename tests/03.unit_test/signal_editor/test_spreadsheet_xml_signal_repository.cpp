#include "signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h"

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

TEST(SpreadsheetXmlSignalRepositoryTest, LoadSpreadsheetXmlTable) {
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

    SpreadsheetXmlSignalRepository repository;
    auto library = repository.load(path);

    ASSERT_EQ(library.size(), 1u);
    EXPECT_TRUE(library.at(0).is_enumerated());
    EXPECT_EQ(library.at(0).label_for_value(library.at(0).samples()[1].y), "ON");
    std::filesystem::remove(path);
}
