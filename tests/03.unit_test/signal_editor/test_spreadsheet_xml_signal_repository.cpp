#include "signal_editor/adapters/filesystem/spreadsheet_xml_signal_repository.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace signal_editor;
using namespace signal_editor::adapters;

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

TEST(SpreadsheetXmlSignalRepositoryTest, LoadSpreadsheetXmlWithNamedTimeColumnNotFirst) {
    auto path = make_temp_path("se_test_time_not_first.xml");
    write_file(path, R"(<?xml version="1.0"?>
<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet" xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">
  <Worksheet ss:Name="Signals">
    <Table>
      <Row>
        <Cell><Data ss:Type="String">meta</Data></Cell>
        <Cell><Data ss:Type="String">time</Data></Cell>
        <Cell><Data ss:Type="String">sigA</Data></Cell>
      </Row>
      <Row>
        <Cell><Data ss:Type="String">x</Data></Cell>
        <Cell><Data ss:Type="Number">0</Data></Cell>
        <Cell><Data ss:Type="Number">1</Data></Cell>
      </Row>
      <Row>
        <Cell><Data ss:Type="String">x</Data></Cell>
        <Cell><Data ss:Type="Number">1</Data></Cell>
        <Cell><Data ss:Type="Number">2</Data></Cell>
      </Row>
    </Table>
  </Worksheet>
</Workbook>)");

    SpreadsheetXmlSignalRepository repository;
    auto library = repository.load(path);
    ASSERT_EQ(library.size(), 1u);
    EXPECT_EQ(library.at(0).name(), "sigA");
    EXPECT_DOUBLE_EQ(library.at(0).samples()[1].t, 1.0);
    std::filesystem::remove(path);
}

TEST(SpreadsheetXmlSignalRepositoryTest, RejectsSpreadsheetXmlWithoutNamedTimeColumn) {
    auto path = make_temp_path("se_test_missing_time_column.xml");
    write_file(path, R"(<?xml version="1.0"?>
<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet" xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet">
  <Worksheet ss:Name="Signals">
    <Table>
      <Row>
        <Cell><Data ss:Type="String">meta</Data></Cell>
        <Cell><Data ss:Type="String">tempo</Data></Cell>
        <Cell><Data ss:Type="String">sigA</Data></Cell>
      </Row>
      <Row>
        <Cell><Data ss:Type="String">x</Data></Cell>
        <Cell><Data ss:Type="Number">0</Data></Cell>
        <Cell><Data ss:Type="Number">1</Data></Cell>
      </Row>
    </Table>
  </Worksheet>
</Workbook>)");

    SpreadsheetXmlSignalRepository repository;
    EXPECT_THROW(repository.load(path), std::runtime_error);
    std::filesystem::remove(path);
}

TEST(SpreadsheetXmlSignalRepositoryTest, RoundTripWorkbookPreservesMultipleWorksheets) {
    auto path = make_temp_path("se_test_multi_sheet.xml");

    SignalLibrary powertrain;
    powertrain.add(Signal::from_vectors("rpm", {0.0, 1.0}, {800.0, 1200.0}));
    powertrain.add(Signal::from_vectors("speed", {0.0, 1.0}, {0.0, 15.0}));

    Signal state = Signal::from_vectors("gear_state", {0.0, 1.0}, {0.0, 1.0},
                                        Signal::InterpolationMode::Step);
    state.set_enumeration({{"P", 0.0}, {"D", 1.0}});
    SignalLibrary vehicle;
    vehicle.add(state);

    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"INPUT_1", powertrain});
    workbook.sheets.push_back(WorkbookSheet{"INPUT_2", vehicle});
    workbook.explicit_sheet_names = true;

    SpreadsheetXmlSignalRepository repository;
    ASSERT_TRUE(repository.save_workbook(path, workbook).is_ok());
    const auto restored = repository.load_workbook(path);

    ASSERT_EQ(restored.sheets.size(), 2u);
    EXPECT_EQ(restored.sheets[0].name, "INPUT_1");
    EXPECT_EQ(restored.sheets[1].name, "INPUT_2");
    EXPECT_EQ(restored.sheets[0].library.at(0).name(), "rpm");
    EXPECT_TRUE(restored.sheets[1].library.at(0).is_enumerated());
    std::filesystem::remove(path);
}
