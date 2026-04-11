#include "signal_editor/core/domain/signal.h"
#include "signal_editor/core/domain/signal_library.h"
#include "signal_editor/core/usecases/signal_editor_service.h"
#include "signal_editor/ports/signal_repository.h"

#include <gtest/gtest.h>

using namespace myprj;
using namespace myprj::signal_editor;

namespace {

// Test double - captures the last load/save call.
class StubRepository : public ISignalRepository {
public:
    SignalLibrary load(const std::filesystem::path& source) override {
        last_loaded = source;
        SignalLibrary lib;
        lib.add(Signal::from_vectors("loaded", {0.0, 1.0, 2.0}, {0.0, 0.5, 1.0}));
        return lib;
    }
    Result save(const std::filesystem::path& destination,
                const SignalLibrary& library) override {
        last_saved   = destination;
        last_size    = library.size();
        return Result::ok();
    }

    std::filesystem::path last_loaded;
    std::filesystem::path last_saved;
    std::size_t           last_size{0};
};

}  // namespace

TEST(SignalEditorServiceTest, LoadFromUsesRepository) {
    StubRepository repo;
    SignalEditorService svc(repo);
    const auto r = svc.load_from("/tmp/foo.csv");
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(svc.library().size(), 1u);
    EXPECT_EQ(repo.last_loaded.string(), "/tmp/foo.csv");
}

TEST(SignalEditorServiceTest, SaveToForwardsToRepository) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 5);
    svc.create_signal("b", 0.0, 1.0, 5);
    const auto r = svc.save_to("/tmp/bar.csv");
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(repo.last_saved.string(), "/tmp/bar.csv");
    EXPECT_EQ(repo.last_size, 2u);
}

TEST(SignalEditorServiceTest, CreateSignalAddsToLibrary) {
    StubRepository repo;
    SignalEditorService svc(repo);
    EXPECT_TRUE(svc.create_signal("u", 0.0, 1.0, 11, 0.5).is_ok());
    EXPECT_EQ(svc.library().size(), 1u);
    EXPECT_EQ(svc.library().at(0).name(), "u");
}

TEST(SignalEditorServiceTest, CreateSignalReportsErrors) {
    StubRepository repo;
    SignalEditorService svc(repo);
    auto r = svc.create_signal("", 0.0, 1.0, 11);
    EXPECT_FALSE(r.is_ok());
    EXPECT_FALSE(r.message.empty());
}

TEST(SignalEditorServiceTest, AddSignalAcceptsCustomWaveform) {
    StubRepository repo;
    SignalEditorService svc(repo);

    const Signal waveform = Signal::from_vectors("sine_like", {0.0, 0.5, 1.0}, {0.0, 1.0, 0.0});
    EXPECT_TRUE(svc.add_signal(waveform).is_ok());
    ASSERT_EQ(svc.library().size(), 1u);
    EXPECT_EQ(svc.library().at(0).name(), "sine_like");
    EXPECT_DOUBLE_EQ(svc.library().at(0).samples()[1].y, 1.0);
}

TEST(SignalEditorServiceTest, RenameSignal) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 5);
    EXPECT_TRUE(svc.rename_signal(0, "renamed").is_ok());
    EXPECT_EQ(svc.library().at(0).name(), "renamed");
    EXPECT_FALSE(svc.rename_signal(0, "").is_ok());
    EXPECT_FALSE(svc.rename_signal(7, "x").is_ok());
}

TEST(SignalEditorServiceTest, SetSignalInterpolation) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 3, 0.0);
    EXPECT_TRUE(svc.set_signal_interpolation(0, Signal::InterpolationMode::Step).is_ok());
    EXPECT_EQ(svc.library().at(0).interpolation(), Signal::InterpolationMode::Step);
    EXPECT_FALSE(svc.set_signal_interpolation(7, Signal::InterpolationMode::Linear).is_ok());
}

TEST(SignalEditorServiceTest, RemoveSignal) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 5);
    svc.create_signal("b", 0.0, 1.0, 5);
    EXPECT_TRUE(svc.remove_signal(0).is_ok());
    EXPECT_EQ(svc.library().size(), 1u);
    EXPECT_EQ(svc.library().at(0).name(), "b");
}

TEST(SignalEditorServiceTest, ReplaceSignalUpdatesSamples) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 3, 0.0);

    const Signal replacement = Signal::from_vectors("a", {0.0, 0.25, 1.0}, {1.0, 2.0, 3.0});
    EXPECT_TRUE(svc.replace_signal(0, replacement).is_ok());
    ASSERT_EQ(svc.library().at(0).size(), 3u);
    EXPECT_DOUBLE_EQ(svc.library().at(0).samples()[1].t, 0.25);
    EXPECT_DOUBLE_EQ(svc.library().at(0).samples()[1].y, 2.0);
}

TEST(SignalEditorServiceTest, MoveAndInsertAndRemoveSamples) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 3, 0.0);  // 3 samples at t=0,0.5,1.0

    EXPECT_TRUE(svc.move_sample(0, 1, 9.0).is_ok());
    EXPECT_DOUBLE_EQ(svc.library().at(0).samples()[1].y, 9.0);

    std::size_t new_idx = 0;
    EXPECT_TRUE(svc.insert_sample(0, 0.25, 5.0, &new_idx).is_ok());
    EXPECT_EQ(new_idx, 1u);
    EXPECT_EQ(svc.library().at(0).size(), 4u);

    EXPECT_TRUE(svc.remove_sample(0, 0).is_ok());
    EXPECT_EQ(svc.library().at(0).size(), 3u);
    EXPECT_DOUBLE_EQ(svc.library().at(0).samples()[0].t, 0.25);
}

TEST(SignalEditorServiceTest, GaussianBrushReportsErrors) {
    StubRepository repo;
    SignalEditorService svc(repo);
    svc.create_signal("a", 0.0, 1.0, 11, 0.0);
    EXPECT_TRUE(svc.apply_gaussian_brush(0, 0.5, 1.0, 0.1).is_ok());
    EXPECT_FALSE(svc.apply_gaussian_brush(0, 0.5, 1.0, 0.0).is_ok());
    EXPECT_FALSE(svc.apply_gaussian_brush(7, 0.5, 1.0, 0.1).is_ok());
}
