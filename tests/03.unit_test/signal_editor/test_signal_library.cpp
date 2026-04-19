#include "signal_editor/core/domain/signal_library.h"

#include <gtest/gtest.h>

using namespace signal_editor;

TEST(SignalLibraryTest, AddRemoveAndAccess) {
    SignalLibrary lib;
    EXPECT_TRUE(lib.empty());
    const auto i0 = lib.add(Signal::from_vectors("a", {0.0, 1.0}, {0.0, 1.0}));
    const auto i1 = lib.add(Signal::from_vectors("b", {0.0, 1.0}, {1.0, 0.0}));
    EXPECT_EQ(i0, 0u);
    EXPECT_EQ(i1, 1u);
    EXPECT_EQ(lib.size(), 2u);
    EXPECT_EQ(lib.at(0).name(), "a");
    EXPECT_EQ(lib.at(1).name(), "b");

    lib.remove(0);
    EXPECT_EQ(lib.size(), 1u);
    EXPECT_EQ(lib.at(0).name(), "b");

    EXPECT_THROW({ (void)lib.at(7); }, std::out_of_range);
    EXPECT_THROW(lib.remove(7), std::out_of_range);
}

TEST(SignalLibraryTest, ReplaceUpdatesSignal) {
    SignalLibrary lib;
    lib.add(Signal::from_vectors("a", {0.0, 1.0}, {0.0, 1.0}));
    lib.replace(0, Signal::from_vectors("renamed", {0.0, 1.0}, {2.0, 3.0}));
    EXPECT_EQ(lib.at(0).name(), "renamed");
    EXPECT_DOUBLE_EQ(lib.at(0).samples()[1].y, 3.0);
}

TEST(SignalLibraryTest, ClearEmptiesLibrary) {
    SignalLibrary lib;
    lib.add(Signal::from_vectors("a", {0.0, 1.0}, {0.0, 1.0}));
    lib.add(Signal::from_vectors("b", {0.0, 1.0}, {0.0, 1.0}));
    lib.clear();
    EXPECT_TRUE(lib.empty());
}
