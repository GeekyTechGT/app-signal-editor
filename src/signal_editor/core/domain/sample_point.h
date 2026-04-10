#pragma once

namespace myprj::signal_editor {

// --- Domain value object -----------------------------------------------------
// A single (time, value) sample of a signal. Pure value type — comparable,
// copyable, no invariants beyond IEEE-754 finiteness which is enforced by
// callers (Signal::insert_sample). No framework dependency.
struct SamplePoint {
    double t{0.0};
    double y{0.0};

    friend constexpr bool operator==(const SamplePoint& a, const SamplePoint& b) noexcept {
        return a.t == b.t && a.y == b.y;
    }
};

}  // namespace myprj::signal_editor
