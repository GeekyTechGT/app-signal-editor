#pragma once

namespace signal_editor {

/**
 * @brief Immutable-style value object that represents a single sample.
 *
 * `SamplePoint` is intentionally tiny and framework-free so that it can be
 * copied, compared, and transported between the domain layer and adapters
 * without conversion overhead.
 */
struct SamplePoint {
    double t{0.0};
    double y{0.0};

    /**
     * @brief Compares two samples for exact coordinate equality.
     * @param a Left-hand sample.
     * @param b Right-hand sample.
     * @return `true` when both time and value match exactly.
     */
    friend constexpr bool operator==(const SamplePoint& a, const SamplePoint& b) noexcept {
        return a.t == b.t && a.y == b.y;
    }
};

}  // namespace signal_editor
