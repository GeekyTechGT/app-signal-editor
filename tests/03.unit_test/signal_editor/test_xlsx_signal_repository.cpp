#include "signal_editor/adapters/filesystem/workbook_model.h"
#include "signal_editor/adapters/filesystem/xlsx_signal_repository.h"

#include <gtest/gtest.h>

#include <filesystem>

#ifdef SIGNAL_EDITOR_HAS_LIBZIP
#include <zip.h>
#endif

using namespace signal_editor;
using namespace signal_editor::adapters;

namespace {

std::filesystem::path source_data_path(const std::string& relative_path) {
#ifdef SIGNAL_EDITOR_SOURCE_DIR
    return std::filesystem::path(SIGNAL_EDITOR_SOURCE_DIR) / relative_path;
#else
    return std::filesystem::path(relative_path);
#endif
}

std::filesystem::path make_temp_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    return path;
}

bool xlsx_supported() {
    XlsxSignalRepository repository;
    SignalLibrary library;
    library.add(Signal::from_vectors("probe", {0.0, 1.0}, {0.0, 1.0}));
    const auto probe_path = make_temp_path("se_xlsx_probe.xlsx");
    const auto probe = repository.save(probe_path, library);
    std::filesystem::remove(probe_path);
    return probe.message.find("not available") == std::string::npos;
}

}  // namespace

TEST(XlsxSignalRepositoryTest, RoundTripWorkbookPreservesMultipleSheets) {
    if (!xlsx_supported()) {
        GTEST_SKIP() << "Native XLSX support is not available in this build";
    }

    auto path = make_temp_path("se_test_workbook.xlsx");

    SignalLibrary input_1;
    input_1.add(Signal::from_vectors("engine_speed_rpm", {0.0, 0.5, 1.0}, {780.0, 1250.0, 2200.0}));
    input_1.add(Signal::from_vectors("vehicle_speed_kph", {0.0, 0.5, 1.0}, {0.0, 12.0, 42.0}));

    Signal gear_state =
        Signal::from_vectors("gear_state", {0.0, 0.5, 1.0}, {0.0, 1.0, 2.0}, Signal::InterpolationMode::Step);
    gear_state.set_enumeration({{"P", 0.0}, {"D", 1.0}, {"N", 2.0}});
    SignalLibrary input_2;
    input_2.add(Signal::from_vectors("battery_voltage_v", {0.0, 0.5, 1.0}, {12.5, 14.0, 13.2}));
    input_2.add(gear_state);

    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"INPUT_1", input_1});
    workbook.sheets.push_back(WorkbookSheet{"INPUT_2", input_2});
    workbook.explicit_sheet_names = true;

    XlsxSignalRepository repository;
    ASSERT_TRUE(repository.save_workbook(path, workbook).is_ok());

    const auto restored = repository.load_workbook(path);
    ASSERT_EQ(restored.sheets.size(), 2u);
    EXPECT_EQ(restored.sheets[0].name, "INPUT_1");
    EXPECT_EQ(restored.sheets[1].name, "INPUT_2");
    ASSERT_EQ(restored.sheets[0].library.size(), 2u);
    ASSERT_EQ(restored.sheets[1].library.size(), 2u);
    EXPECT_EQ(restored.sheets[0].library.at(0).name(), "engine_speed_rpm");
    EXPECT_EQ(restored.sheets[1].library.at(1).name(), "gear_state");
    EXPECT_TRUE(restored.sheets[1].library.at(1).is_enumerated());
    EXPECT_EQ(restored.sheets[1].library.at(1).label_for_value(restored.sheets[1].library.at(1).samples()[1].y),
              "D");

    std::filesystem::remove(path);
}

TEST(XlsxSignalRepositoryTest, SavedWorkbookContainsExpectedZipEntries) {
    if (!xlsx_supported()) {
        GTEST_SKIP() << "Native XLSX support is not available in this build";
    }

    auto path = make_temp_path("se_test_workbook_entries.xlsx");

    SignalLibrary library;
    library.add(Signal::from_vectors("rpm", {0.0, 0.5, 1.0}, {800.0, 1250.0, 2100.0}));

    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"INPUT_1", library});
    workbook.explicit_sheet_names = true;

    XlsxSignalRepository repository;
    ASSERT_TRUE(repository.save_workbook(path, workbook).is_ok());

#ifdef SIGNAL_EDITOR_HAS_LIBZIP
    int error_code = ZIP_ER_OK;
    zip_t* archive = zip_open(path.string().c_str(), ZIP_RDONLY, &error_code);
    ASSERT_NE(archive, nullptr);
    EXPECT_GE(zip_name_locate(archive, "[Content_Types].xml", ZIP_FL_ENC_GUESS), 0);
    EXPECT_GE(zip_name_locate(archive, "_rels/.rels", ZIP_FL_ENC_GUESS), 0);
    EXPECT_GE(zip_name_locate(archive, "xl/workbook.xml", ZIP_FL_ENC_GUESS), 0);
    EXPECT_GE(zip_name_locate(archive, "xl/_rels/workbook.xml.rels", ZIP_FL_ENC_GUESS), 0);
    EXPECT_GE(zip_name_locate(archive, "xl/styles.xml", ZIP_FL_ENC_GUESS), 0);
    EXPECT_GE(zip_name_locate(archive, "xl/worksheets/sheet1.xml", ZIP_FL_ENC_GUESS), 0);
    ASSERT_EQ(zip_close(archive), 0);
#endif

    const auto restored = repository.load_workbook(path);
    ASSERT_EQ(restored.sheets.size(), 1u);
    EXPECT_EQ(restored.sheets[0].name, "INPUT_1");
    EXPECT_EQ(restored.sheets[0].library.at(0).name(), "rpm");

    std::filesystem::remove(path);
}

TEST(XlsxSignalRepositoryTest, SavedWorksheetStartsWithPlainHeaderRow) {
    if (!xlsx_supported()) {
        GTEST_SKIP() << "Native XLSX support is not available in this build";
    }

    auto path = make_temp_path("se_test_plain_header.xlsx");

    Signal enum_signal =
        Signal::from_vectors("gear_state", {0.0, 0.5, 1.0}, {0.0, 1.0, 2.0}, Signal::InterpolationMode::Step);
    enum_signal.set_enumeration({{"P", 0.0}, {"D", 1.0}, {"N", 2.0}});
    SignalLibrary library;
    library.add(enum_signal);

    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"INPUT_1", library});
    workbook.explicit_sheet_names = true;

    XlsxSignalRepository repository;
    ASSERT_TRUE(repository.save_workbook(path, workbook).is_ok());

#ifdef SIGNAL_EDITOR_HAS_LIBZIP
    int error_code = ZIP_ER_OK;
    zip_t* archive = zip_open(path.string().c_str(), ZIP_RDONLY, &error_code);
    ASSERT_NE(archive, nullptr);

    zip_file_t* file = zip_fopen(archive, "xl/worksheets/sheet1.xml", ZIP_FL_ENC_GUESS);
    ASSERT_NE(file, nullptr);
    zip_stat_t stat{};
    ASSERT_EQ(zip_stat(archive, "xl/worksheets/sheet1.xml", ZIP_FL_ENC_GUESS, &stat), 0);
    std::string xml(static_cast<std::size_t>(stat.size), '\0');
    ASSERT_EQ(zip_fread(file, xml.data(), stat.size), static_cast<zip_int64_t>(stat.size));
    zip_fclose(file);
    ASSERT_EQ(zip_close(archive), 0);

    EXPECT_NE(xml.find(">time<"), std::string::npos);
    EXPECT_NE(xml.find(">gear_state<"), std::string::npos);
    EXPECT_EQ(xml.find("# interpolation"), std::string::npos);
    EXPECT_EQ(xml.find("# enum_map"), std::string::npos);
    EXPECT_EQ(xml.find(">P<"), std::string::npos);
#endif

    const auto restored = repository.load_workbook(path);
    ASSERT_EQ(restored.sheets.size(), 1u);
    ASSERT_EQ(restored.sheets[0].library.size(), 1u);
    EXPECT_TRUE(restored.sheets[0].library.at(0).is_enumerated());
    EXPECT_EQ(restored.sheets[0].library.at(0).label_for_value(1.0), "D");

    std::filesystem::remove(path);
}

TEST(XlsxSignalRepositoryTest, SavedMetadataSheetUsesMinimalEnumSchema) {
    if (!xlsx_supported()) {
        GTEST_SKIP() << "Native XLSX support is not available in this build";
    }

    auto path = make_temp_path("se_test_metadata_schema.xlsx");

    Signal gear_state =
        Signal::from_vectors("gear_state", {0.0, 0.5}, {0.0, 1.0}, Signal::InterpolationMode::Step);
    gear_state.set_enumeration({{"P", 0.0}, {"D", 1.0}});
    SignalLibrary library;
    library.add(gear_state);

    WorkbookDocument workbook;
    workbook.sheets.push_back(WorkbookSheet{"INPUT_2", library});
    workbook.explicit_sheet_names = true;

    XlsxSignalRepository repository;
    ASSERT_TRUE(repository.save_workbook(path, workbook).is_ok());

#ifdef SIGNAL_EDITOR_HAS_LIBZIP
    int error_code = ZIP_ER_OK;
    zip_t* archive = zip_open(path.string().c_str(), ZIP_RDONLY, &error_code);
    ASSERT_NE(archive, nullptr);

    zip_file_t* workbook_file = zip_fopen(archive, "xl/workbook.xml", ZIP_FL_ENC_GUESS);
    ASSERT_NE(workbook_file, nullptr);
    zip_stat_t workbook_stat{};
    ASSERT_EQ(zip_stat(archive, "xl/workbook.xml", ZIP_FL_ENC_GUESS, &workbook_stat), 0);
    std::string workbook_xml(static_cast<std::size_t>(workbook_stat.size), '\0');
    ASSERT_EQ(zip_fread(workbook_file, workbook_xml.data(), workbook_stat.size),
              static_cast<zip_int64_t>(workbook_stat.size));
    zip_fclose(workbook_file);

    EXPECT_NE(workbook_xml.find("name=\"METADATA\""), std::string::npos);

    zip_file_t* metadata_file = zip_fopen(archive, "xl/worksheets/sheet2.xml", ZIP_FL_ENC_GUESS);
    ASSERT_NE(metadata_file, nullptr);
    zip_stat_t metadata_stat{};
    ASSERT_EQ(zip_stat(archive, "xl/worksheets/sheet2.xml", ZIP_FL_ENC_GUESS, &metadata_stat), 0);
    std::string metadata_xml(static_cast<std::size_t>(metadata_stat.size), '\0');
    ASSERT_EQ(zip_fread(metadata_file, metadata_xml.data(), metadata_stat.size),
              static_cast<zip_int64_t>(metadata_stat.size));
    zip_fclose(metadata_file);
    ASSERT_EQ(zip_close(archive), 0);

    EXPECT_NE(metadata_xml.find(">sheet<"), std::string::npos);
    EXPECT_NE(metadata_xml.find(">signal_name<"), std::string::npos);
    EXPECT_NE(metadata_xml.find(">enum_label<"), std::string::npos);
    EXPECT_NE(metadata_xml.find(">enum_value<"), std::string::npos);
    EXPECT_EQ(metadata_xml.find(">signal_index<"), std::string::npos);
    EXPECT_EQ(metadata_xml.find(">interpolation<"), std::string::npos);
#endif

    const auto restored = repository.load_workbook(path);
    ASSERT_EQ(restored.sheets.size(), 1u);
    ASSERT_EQ(restored.sheets[0].library.size(), 1u);
    EXPECT_TRUE(restored.sheets[0].library.at(0).is_enumerated());
    EXPECT_EQ(restored.sheets[0].library.at(0).label_for_value(1.0), "D");

    std::filesystem::remove(path);
}

TEST(XlsxSignalRepositoryTest, LoadRealFixtureWorkbook) {
    if (!xlsx_supported()) {
        GTEST_SKIP() << "Native XLSX support is not available in this build";
    }

    XlsxSignalRepository repository;
    const auto workbook =
        repository.load_workbook(source_data_path("tests/01.data/sample_automotive_signals_2_tab.xlsx"));

    ASSERT_EQ(workbook.sheets.size(), 2u);
    EXPECT_EQ(workbook.sheets[0].name, "INPUT_1");
    EXPECT_EQ(workbook.sheets[1].name, "INPUT_2");
    EXPECT_EQ(workbook.sheets[0].library.at(0).name(), "engine_speed_rpm");
    EXPECT_EQ(workbook.sheets[1].library.at(1).name(), "gear_state");
}
