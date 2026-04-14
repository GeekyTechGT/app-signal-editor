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

TEST(SignalFileRepositoryTest, LoadCsvViaExtensionDispatch) {
    auto path = make_temp_path("se_test_signals.csv");
    write_file(path,
               "# interpolation,step\n"
               "# enum_map,OFF:0|ON:1\n"
               "time,mode\n"
               "0.0,OFF\n"
               "1.0,ON\n");

    SignalFileRepository repository;
    auto library = repository.load(path);

    ASSERT_EQ(library.size(), 1u);
    EXPECT_TRUE(library.at(0).is_enumerated());
    EXPECT_EQ(library.at(0).label_for_value(library.at(0).samples()[1].y), "ON");
    std::filesystem::remove(path);
}

TEST(SignalFileRepositoryTest, SaveJsonViaExtensionDispatch) {
    auto path = make_temp_path("se_test_dispatch.json");

    Signal signal = Signal::create_uniform("enable", 0.0, 1.0, 2, 0.0, Signal::InterpolationMode::Step);
    signal.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}});
    signal.set_sample_value(1, 1.0);

    SignalLibrary library;
    library.add(signal);

    SignalFileRepository repository;
    ASSERT_TRUE(repository.save(path, library).is_ok());
    auto restored = repository.load(path);

    ASSERT_EQ(restored.size(), 1u);
    EXPECT_TRUE(restored.at(0).is_enumerated());
    EXPECT_EQ(restored.at(0).label_for_value(restored.at(0).samples()[1].y), "TRUE");
    std::filesystem::remove(path);
}
