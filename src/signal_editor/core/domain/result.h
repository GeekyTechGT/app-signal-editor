#pragma once

#include <string>
#include <utility>

namespace signal_editor {

/**
 * @brief Generic success/error discriminator shared across Signal Editor workflows.
 */
enum class Status { Ok, Error };

/**
 * @brief Lightweight result container used by use cases and adapters.
 *
 * The type deliberately stays small and dependency-free so it can cross module
 * boundaries without bringing exceptions or framework-specific wrappers into
 * the public API.
 */
struct Result {
    Status status{Status::Ok};
    std::string message;

    /**
     * @brief Creates a successful result.
     * @return Result marked as `Status::Ok`.
     */
    [[nodiscard]] static Result ok() { return {Status::Ok, {}}; }

    /**
     * @brief Creates a failing result carrying a human-readable explanation.
     * @param msg Failure details intended for logs or UI feedback.
     * @return Result marked as `Status::Error`.
     */
    [[nodiscard]] static Result error(std::string msg) { return {Status::Error, std::move(msg)}; }

    /**
     * @brief Convenience predicate used by callers that only need pass/fail.
     * @return `true` when the operation succeeded.
     */
    [[nodiscard]] bool is_ok() const noexcept { return status == Status::Ok; }
};

}  // namespace signal_editor
