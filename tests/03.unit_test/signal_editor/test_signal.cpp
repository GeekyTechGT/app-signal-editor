#include "signal_editor/core/domain/signal.h"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace myprj::signal_editor;

TEST(SignalTest, FromVectorsBuildsSortedSamples) {
    auto s = Signal::from_vectors("sig", {0.0, 0.5, 1.0}, {0.0, 1.0, 0.0});
    ASSERT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s.samples()[0].t, 0.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].t, 0.5);
    EXPECT_DOUBLE_EQ(s.samples()[2].t, 1.0);
}

TEST(SignalTest, RejectsEmptyName) {
    EXPECT_THROW(Signal("", {{0.0, 0.0}, {1.0, 1.0}}), std::invalid_argument);
}

TEST(SignalTest, RejectsLengthMismatch) {
    EXPECT_THROW(Signal::from_vectors("sig", {0.0, 1.0}, {0.0}), std::invalid_argument);
}

TEST(SignalTest, CreateUniformIsLinearlySpaced) {
    auto s = Signal::create_uniform("uni", 0.0, 1.0, 5, 2.0);
    ASSERT_EQ(s.size(), 5u);
    EXPECT_DOUBLE_EQ(s.samples().front().t, 0.0);
    EXPECT_DOUBLE_EQ(s.samples().back().t, 1.0);
    for (const auto& p : s.samples()) {
        EXPECT_DOUBLE_EQ(p.y, 2.0);
    }
}

TEST(SignalTest, CreateUniformValidatesArguments) {
    EXPECT_THROW(Signal::create_uniform("u", 0.0, 1.0, 1), std::invalid_argument);
    EXPECT_THROW(Signal::create_uniform("u", 1.0, 0.0, 5), std::invalid_argument);
}

TEST(SignalTest, InterpolateClampsAndInterpolates) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0}, {0.0, 10.0, 20.0});
    EXPECT_DOUBLE_EQ(s.interpolate(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(s.interpolate(3.0),  20.0);
    EXPECT_DOUBLE_EQ(s.interpolate(0.5),  5.0);
    EXPECT_DOUBLE_EQ(s.interpolate(1.5),  15.0);
}

TEST(SignalTest, SetSampleValueUpdatesY) {
    auto s = Signal::from_vectors("s", {0.0, 1.0}, {0.0, 0.0});
    s.set_sample_value(1, 42.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 42.0);
    EXPECT_THROW(s.set_sample_value(7, 0.0), std::out_of_range);
}

TEST(SignalTest, InsertSampleKeepsOrderAndDedupes) {
    auto s = Signal::from_vectors("s", {0.0, 1.0}, {0.0, 1.0});
    const auto idx_mid = s.insert_sample(0.5, 0.5);
    EXPECT_EQ(idx_mid, 1u);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s.samples()[1].t, 0.5);

    // Duplicate t -> updates y, no new sample.
    const auto idx_dup = s.insert_sample(0.5, 7.0);
    EXPECT_EQ(idx_dup, 1u);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 7.0);
}

TEST(SignalTest, RemoveSample) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0}, {0.0, 1.0, 2.0});
    s.remove_sample(1);
    ASSERT_EQ(s.size(), 2u);
    EXPECT_DOUBLE_EQ(s.samples()[1].t, 2.0);
    EXPECT_THROW(s.remove_sample(7), std::out_of_range);
}

TEST(SignalTest, GaussianBrushPeaksAtCenter) {
    auto s = Signal::create_uniform("u", 0.0, 1.0, 11, 0.0);
    s.apply_gaussian_brush(0.5, 1.0, 0.1);
    // Sample at t=0.5 (index 5) should be ~1.0
    EXPECT_NEAR(s.samples()[5].y, 1.0, 1e-9);
    EXPECT_LT(s.samples()[0].y, 0.001);
    EXPECT_LT(s.samples()[10].y, 0.001);
    EXPECT_THROW(s.apply_gaussian_brush(0.5, 1.0, 0.0), std::invalid_argument);
}

TEST(SignalTest, NearestIndex) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0, 3.0}, {0.0, 0.0, 0.0, 0.0});
    EXPECT_EQ(s.nearest_index(-5.0), 0u);
    EXPECT_EQ(s.nearest_index( 0.4), 0u);
    EXPECT_EQ(s.nearest_index( 1.6), 2u);
    EXPECT_EQ(s.nearest_index(99.0), 3u);
}
