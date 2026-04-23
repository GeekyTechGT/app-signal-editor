#include "signal_editor/core/domain/signal.h"

#include <gtest/gtest.h>

#include <stdexcept>

using namespace signal_editor;

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
    EXPECT_DOUBLE_EQ(s.interpolate(3.0), 20.0);
    EXPECT_DOUBLE_EQ(s.interpolate(0.5), 5.0);
    EXPECT_DOUBLE_EQ(s.interpolate(1.5), 15.0);
}

TEST(SignalTest, StepInterpolationUsesPreviousSampleValue) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0}, {0.0, 10.0, 20.0}, Signal::InterpolationMode::Step);
    EXPECT_EQ(s.interpolation(), Signal::InterpolationMode::Step);
    EXPECT_DOUBLE_EQ(s.interpolate(0.25), 0.0);
    EXPECT_DOUBLE_EQ(s.interpolate(1.75), 10.0);
    s.set_interpolation(Signal::InterpolationMode::Linear);
    EXPECT_DOUBLE_EQ(s.interpolate(1.75), 17.5);
}

TEST(SignalTest, EnumeratedSignalsForceStepInterpolationAndSnapValues) {
    auto s = Signal::from_vectors("bool", {0.0, 1.0, 2.0}, {0.2, 0.9, 0.1});
    s.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}});

    EXPECT_TRUE(s.is_enumerated());
    EXPECT_EQ(s.interpolation(), Signal::InterpolationMode::Step);
    EXPECT_DOUBLE_EQ(s.samples()[0].y, 0.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 1.0);
    EXPECT_DOUBLE_EQ(s.samples()[2].y, 0.0);

    s.set_interpolation(Signal::InterpolationMode::Linear);
    EXPECT_EQ(s.interpolation(), Signal::InterpolationMode::Step);
    EXPECT_DOUBLE_EQ(s.interpolate(1.5), 1.0);
}

TEST(SignalTest, EnumeratedSignalsValidateMappings) {
    Signal s = Signal::create_uniform("enum", 0.0, 1.0, 2, 0.0);
    EXPECT_THROW(s.set_enumeration({{"", 0.0}}), std::invalid_argument);
    EXPECT_THROW(s.set_enumeration({{"FALSE", 0.0}, {"FALSE", 1.0}}), std::invalid_argument);
    EXPECT_THROW(s.set_enumeration({{"FALSE", 0.0}, {"TRUE", 0.0}}), std::invalid_argument);
}

TEST(SignalTest, EnumeratedSignalsResolveLabelsAndSnapInsertedValues) {
    Signal s = Signal::create_uniform("enum", 0.0, 1.0, 2, 0.0);
    s.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}, {"UNKNOWN", 2.0}});

    EXPECT_EQ(s.label_for_value(1.0), "TRUE");
    EXPECT_DOUBLE_EQ(s.value_for_label("UNKNOWN"), 2.0);
    EXPECT_THROW([&]() { static_cast<void>(s.value_for_label("MISSING")); }(), std::invalid_argument);

    s.set_sample_value(0, 1.6);
    EXPECT_DOUBLE_EQ(s.samples()[0].y, 2.0);
    const std::size_t inserted = s.insert_sample(0.5, 0.4);
    EXPECT_DOUBLE_EQ(s.samples()[inserted].y, 0.0);
}

TEST(SignalTest, EnumeratedRemapPreservesLabelsAcrossNumericChanges) {
    Signal s = Signal::create_uniform("enum", 0.0, 2.0, 3, 0.0, Signal::InterpolationMode::Step);
    s.set_enumeration({{"OFF", 0.0}, {"ON", 1.0}});
    s.set_sample_value(1, 1.0);

    s.set_enumeration({{"OFF", 10.0}, {"ON", 20.0}});

    ASSERT_EQ(s.enumeration().size(), 2u);
    EXPECT_DOUBLE_EQ(s.samples()[0].y, 10.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 20.0);
    EXPECT_DOUBLE_EQ(s.samples()[2].y, 10.0);
    EXPECT_EQ(s.label_for_value(s.samples()[1].y), "ON");
}

TEST(SignalTest, ClearEnumerationRestoresLinearInterpolation) {
    Signal s = Signal::create_uniform("enum", 0.0, 1.0, 3, 0.0);
    s.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}});

    ASSERT_TRUE(s.is_enumerated());
    ASSERT_EQ(s.interpolation(), Signal::InterpolationMode::Step);

    s.clear_enumeration();

    EXPECT_FALSE(s.is_enumerated());
    EXPECT_EQ(s.interpolation(), Signal::InterpolationMode::Linear);
}

TEST(SignalTest, SetSampleValueUpdatesY) {
    auto s = Signal::from_vectors("s", {0.0, 1.0}, {0.0, 0.0});
    s.set_sample_value(1, 42.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 42.0);
    EXPECT_THROW(s.set_sample_value(7, 0.0), std::out_of_range);
}

TEST(SignalTest, FromOrderedSamplesKeepsLargeGeneratorOrder) {
    std::vector<SamplePoint> samples{{0.0, 1.0}, {0.1, 2.0}, {0.2, 3.0}};

    const Signal signal = Signal::from_ordered_samples("ordered", std::move(samples));

    ASSERT_EQ(signal.size(), 3u);
    EXPECT_DOUBLE_EQ(signal.samples()[0].t, 0.0);
    EXPECT_DOUBLE_EQ(signal.samples()[2].t, 0.2);
    EXPECT_THROW(Signal::from_ordered_samples("bad", {{0.0, 1.0}, {0.0, 2.0}}),
                 std::invalid_argument);
}

TEST(SignalTest, MoveSampleReordersAndUpdatesPoint) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0}, {0.0, 10.0, 20.0});
    const std::size_t moved_index = s.move_sample(0, 1.5, 7.5);
    EXPECT_EQ(moved_index, 1u);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s.samples()[0].t, 1.0);
    EXPECT_DOUBLE_EQ(s.samples()[1].t, 1.5);
    EXPECT_DOUBLE_EQ(s.samples()[1].y, 7.5);
    EXPECT_THROW(s.move_sample(9, 0.0, 0.0), std::out_of_range);
}

TEST(SignalTest, InsertSampleKeepsOrderAndDedupes) {
    auto s = Signal::from_vectors("s", {0.0, 1.0}, {0.0, 1.0});
    const auto idx_mid = s.insert_sample(0.5, 0.5);
    EXPECT_EQ(idx_mid, 1u);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_DOUBLE_EQ(s.samples()[1].t, 0.5);

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
    EXPECT_NEAR(s.samples()[5].y, 1.0, 1e-9);
    EXPECT_LT(s.samples()[0].y, 0.001);
    EXPECT_LT(s.samples()[10].y, 0.001);
    EXPECT_THROW(s.apply_gaussian_brush(0.5, 1.0, 0.0), std::invalid_argument);
}

TEST(SignalTest, EnumeratedSignalsRejectGaussianBrush) {
    Signal s = Signal::create_uniform("enum", 0.0, 1.0, 3, 0.0);
    s.set_enumeration({{"FALSE", 0.0}, {"TRUE", 1.0}});
    EXPECT_THROW(s.apply_gaussian_brush(0.5, 1.0, 0.1), std::invalid_argument);
}

TEST(SignalTest, NearestIndex) {
    auto s = Signal::from_vectors("s", {0.0, 1.0, 2.0, 3.0}, {0.0, 0.0, 0.0, 0.0});
    EXPECT_EQ(s.nearest_index(-5.0), 0u);
    EXPECT_EQ(s.nearest_index(0.4), 0u);
    EXPECT_EQ(s.nearest_index(1.6), 2u);
    EXPECT_EQ(s.nearest_index(99.0), 3u);
}
