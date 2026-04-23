#include "signal_editor/adapters/qt/signal_lod_pyramid.h"
#include "signal_editor/core/domain/signal.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

signal_editor::Signal make_dense_signal(std::size_t sample_count, double dt) {
    std::vector<signal_editor::SamplePoint> samples;
    samples.reserve(sample_count);
    for (std::size_t index = 0; index < sample_count; ++index) {
        const double t = static_cast<double>(index) * dt;
        double y = std::sin(2.0 * 3.14159265358979323846 * 2.0 * t);
        if (index == sample_count / 3U) {
            y = 25.0;
        } else if (index == sample_count / 2U) {
            y = -18.0;
        }
        samples.push_back({t, y});
    }
    return signal_editor::Signal("dense", std::move(samples));
}

}  // namespace

TEST(SignalLodPyramidTest, BuildsPowerOfTwoMinMaxLevels) {
    const signal_editor::Signal signal = make_dense_signal(1'048'576U, 0.0001);
    signal_editor::adapters::qt::SignalLodCache cache;

    const auto& pyramid = cache.pyramid_for(signal);

    ASSERT_FALSE(pyramid.levels.empty());
    EXPECT_EQ(pyramid.levels.front().samples_per_bucket, 2U);
    for (std::size_t level = 1; level < pyramid.levels.size(); ++level) {
        EXPECT_EQ(pyramid.levels[level].samples_per_bucket,
                  pyramid.levels[level - 1U].samples_per_bucket * 2U);
        EXPECT_LE(pyramid.levels[level].buckets.size(),
                  (pyramid.levels[level - 1U].buckets.size() + 1U) / 2U);
    }
    EXPECT_EQ(pyramid.levels.back().buckets.size(), 1U);
}

TEST(SignalLodPyramidTest, PreservesNarrowSpikesInAggregatedBuckets) {
    const signal_editor::Signal signal = make_dense_signal(262'144U, 0.0001);
    signal_editor::adapters::qt::SignalLodCache cache;

    const auto& pyramid = cache.pyramid_for(signal);
    ASSERT_FALSE(pyramid.levels.empty());

    const auto& coarsest = pyramid.levels.back();
    ASSERT_EQ(coarsest.buckets.size(), 1U);
    EXPECT_FLOAT_EQ(coarsest.buckets.front().max_value, 25.0F);
    EXPECT_FLOAT_EQ(coarsest.buckets.front().min_value, -18.0F);
}

TEST(SignalLodPyramidTest, SelectsRawMediumAndCoarseRenderingLevels) {
    const signal_editor::Signal signal = make_dense_signal(1'048'576U, 0.0001);
    signal_editor::adapters::qt::SignalLodCache cache;

    const auto& pyramid = cache.pyramid_for(signal);

    const auto* raw = signal_editor::adapters::qt::choose_lod_level(
        pyramid,
        2'000U,
        1000.0,
        12'000U);
    EXPECT_EQ(raw, nullptr);

    const auto* medium = signal_editor::adapters::qt::choose_lod_level(
        pyramid,
        300'000U,
        1000.0,
        12'000U);
    ASSERT_NE(medium, nullptr);
    EXPECT_LE(medium->samples_per_bucket, 300U);

    const auto* coarse = signal_editor::adapters::qt::choose_lod_level(
        pyramid,
        signal.size(),
        1000.0,
        12'000U);
    ASSERT_NE(coarse, nullptr);
    EXPECT_GT(coarse->samples_per_bucket, medium->samples_per_bucket);
}

TEST(SignalLodPyramidTest, KeepsVisibleRangeQueriesBounded) {
    const signal_editor::Signal signal = make_dense_signal(1'048'576U, 0.0001);
    signal_editor::adapters::qt::SignalLodCache cache;

    const auto visible = signal_editor::adapters::qt::visible_sample_count(signal, 10.0, 20.0);
    EXPECT_GT(visible, 90'000U);
    EXPECT_LT(visible, 110'100U);

    const auto& pyramid = cache.pyramid_for(signal);
    const auto* range_level = signal_editor::adapters::qt::choose_range_lod_level(
        pyramid,
        visible,
        12'000U,
        2048.0);
    ASSERT_NE(range_level, nullptr);
    EXPECT_LE(static_cast<double>(visible) / static_cast<double>(range_level->samples_per_bucket),
              2048.0);
}
