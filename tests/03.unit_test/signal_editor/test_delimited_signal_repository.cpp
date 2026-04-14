#include "signal_editor/adapters/filesystem/delimited_signal_repository.h"
#include "signal_editor/core/domain/signal_library.h"

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

TEST(DelimitedSignalRepositoryTest, SaveAndReloadTsvPreservesMappings) {
    auto path = make_temp_path("se_test_signals.tsv");

    Signal signal = Signal::create_uniform("state", 0.0, 1.0, 2, 0.0, Signal::InterpolationMode::Step);
    signal.set_enumeration({{"IDLE", 0.0}, {"RUN", 1.0}});
    signal.set_sample_value(1, 1.0);

    SignalLibrary library;
    library.add(signal);

    DelimitedSignalRepository repository('	');
    ASSERT_TRUE(repository.save(path, library).is_ok());
    auto restored = repository.load(path);

    ASSERT_EQ(restored.size(), 1u);
    EXPECT_TRUE(restored.at(0).is_enumerated());
    EXPECT_EQ(restored.at(0).label_for_value(restored.at(0).samples()[1].y), "RUN");
    std::filesystem::remove(path);
}
