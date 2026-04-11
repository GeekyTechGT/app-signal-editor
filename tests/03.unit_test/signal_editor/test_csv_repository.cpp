#include "signal_editor/adapters/filesystem/csv_signal_repository.h"
#include "signal_editor/core/domain/signal_library.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace myprj::signal_editor;
using namespace myprj::signal_editor::adapters;

namespace {

std::filesystem::path make_temp_path(const std::string& name) {
    auto p = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(p);
    return p;
}

void write_file(const std::filesystem::path& p, const std::string& content) {
    std::ofstream out(p);
    out << content;
}

}  // namespace

TEST(CsvRepositoryTest, LoadHeaderedCsv) {
    auto p = make_temp_path("se_test_headered.csv");
    write_file(p,
               "time,sig1,sig2\n"
               "0.0,0.0,1.0\n"
               "0.5,0.5,0.5\n"
               "1.0,1.0,0.0\n");
    CsvSignalRepository repo;
    auto lib = repo.load(p);
    ASSERT_EQ(lib.size(), 2u);
    EXPECT_EQ(lib.at(0).name(), "sig1");
    EXPECT_EQ(lib.at(1).name(), "sig2");
    EXPECT_EQ(lib.at(0).size(), 3u);
    EXPECT_DOUBLE_EQ(lib.at(0).samples()[2].y, 1.0);
    EXPECT_DOUBLE_EQ(lib.at(1).samples()[0].y, 1.0);
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, LoadHeaderlessCsvAutoNamesSignals) {
    auto p = make_temp_path("se_test_headerless.csv");
    write_file(p,
               "0.0,0.0,1.0\n"
               "1.0,1.0,0.0\n");
    CsvSignalRepository repo;
    auto lib = repo.load(p);
    ASSERT_EQ(lib.size(), 2u);
    EXPECT_EQ(lib.at(0).name(), "signal_1");
    EXPECT_EQ(lib.at(1).name(), "signal_2");
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, LoadRejectsNonMonotonicTime) {
    auto p = make_temp_path("se_test_bad_time.csv");
    write_file(p,
               "time,a\n"
               "0.0,0.0\n"
               "0.5,1.0\n"
               "0.4,2.0\n");
    CsvSignalRepository repo;
    EXPECT_THROW(repo.load(p), std::runtime_error);
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, LoadRejectsTooFewColumns) {
    auto p = make_temp_path("se_test_one_col.csv");
    write_file(p, "time\n0.0\n1.0\n");
    CsvSignalRepository repo;
    EXPECT_THROW(repo.load(p), std::runtime_error);
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, LoadEnumeratedMetadataAndLabels) {
    auto p = make_temp_path("se_test_enum_metadata.csv");
    write_file(p,
               "# interpolation,step\n"
               "# enum_map,OFF:0|ON:1\n"
               "time,mode\n"
               "0.0,OFF\n"
               "1.0,ON\n"
               "2.0,OFF\n");
    CsvSignalRepository repo;
    auto lib = repo.load(p);
    ASSERT_EQ(lib.size(), 1u);
    const auto& signal = lib.at(0);
    EXPECT_TRUE(signal.is_enumerated());
    EXPECT_EQ(signal.interpolation(), Signal::InterpolationMode::Step);
    ASSERT_EQ(signal.enumeration().size(), 2u);
    EXPECT_EQ(signal.label_for_value(signal.samples()[1].y), "ON");
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, LoadInlineEnumeratedTokensWithoutMetadata) {
    auto p = make_temp_path("se_test_enum_inline.csv");
    write_file(p,
               "time,state\n"
               "0.0,FALSE:0\n"
               "1.0,TRUE:1\n"
               "2.0,FALSE\n");
    CsvSignalRepository repo;
    auto lib = repo.load(p);
    ASSERT_EQ(lib.size(), 1u);
    const auto& signal = lib.at(0);
    EXPECT_TRUE(signal.is_enumerated());
    ASSERT_EQ(signal.enumeration().size(), 2u);
    EXPECT_EQ(signal.label_for_value(signal.samples()[0].y), "FALSE");
    EXPECT_EQ(signal.label_for_value(signal.samples()[1].y), "TRUE");
    std::filesystem::remove(p);
}

TEST(CsvRepositoryTest, RoundTripPreservesData) {
    auto src = make_temp_path("se_test_roundtrip_in.csv");
    auto dst = make_temp_path("se_test_roundtrip_out.csv");
    write_file(src,
               "time,sigA,sigB\n"
               "0.0,0.0,2.0\n"
               "0.25,0.5,1.5\n"
               "0.5,1.0,1.0\n"
               "0.75,0.5,0.5\n"
               "1.0,0.0,0.0\n");
    CsvSignalRepository repo;
    auto lib = repo.load(src);
    ASSERT_TRUE(repo.save(dst, lib).is_ok());
    auto reloaded = repo.load(dst);
    ASSERT_EQ(reloaded.size(), 2u);
    EXPECT_EQ(reloaded.at(0).name(), "sigA");
    EXPECT_EQ(reloaded.at(1).name(), "sigB");
    ASSERT_EQ(reloaded.at(0).size(), 5u);
    EXPECT_NEAR(reloaded.at(0).samples()[2].y, 1.0, 1e-9);
    EXPECT_NEAR(reloaded.at(1).samples()[0].y, 2.0, 1e-9);
    std::filesystem::remove(src);
    std::filesystem::remove(dst);
}

TEST(CsvRepositoryTest, RoundTripPreservesInterpolationMetadata) {
    auto src = make_temp_path("se_test_interp_in.csv");
    auto dst = make_temp_path("se_test_interp_out.csv");
    write_file(src,
               "# interpolation,step,linear\n"
               "time,sigA,sigB\n"
               "0.0,0.0,0.0\n"
               "1.0,1.0,1.0\n"
               "2.0,2.0,0.0\n");
    CsvSignalRepository repo;
    auto lib = repo.load(src);
    ASSERT_EQ(lib.at(0).interpolation(), Signal::InterpolationMode::Step);
    ASSERT_EQ(lib.at(1).interpolation(), Signal::InterpolationMode::Linear);
    ASSERT_TRUE(repo.save(dst, lib).is_ok());
    auto reloaded = repo.load(dst);
    EXPECT_EQ(reloaded.at(0).interpolation(), Signal::InterpolationMode::Step);
    EXPECT_EQ(reloaded.at(1).interpolation(), Signal::InterpolationMode::Linear);
    std::filesystem::remove(src);
    std::filesystem::remove(dst);
}

TEST(CsvRepositoryTest, RoundTripPreservesEnumeratedSignals) {
    auto dst = make_temp_path("se_test_enum_roundtrip.csv");
    Signal signal = Signal::create_uniform("state", 0.0, 2.0, 3, 0.0, Signal::InterpolationMode::Step);
    signal.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}});
    signal.set_sample_value(1, 1.0);

    SignalLibrary library;
    library.add(signal);

    CsvSignalRepository repo;
    ASSERT_TRUE(repo.save(dst, library).is_ok());
    auto reloaded = repo.load(dst);
    ASSERT_EQ(reloaded.size(), 1u);
    const auto& restored = reloaded.at(0);
    EXPECT_TRUE(restored.is_enumerated());
    EXPECT_EQ(restored.label_for_value(restored.samples()[0].y), "FALSE");
    EXPECT_EQ(restored.label_for_value(restored.samples()[1].y), "TRUE");
    std::filesystem::remove(dst);
}

TEST(CsvRepositoryTest, SaveEmptyLibraryReturnsError) {
    SignalLibrary empty_lib;
    CsvSignalRepository repo;
    auto dst = make_temp_path("se_test_empty.csv");
    EXPECT_FALSE(repo.save(dst, empty_lib).is_ok());
}
