#pragma once

#include <string>
#include <vector>

namespace myprj {

// Shared result type used across all modules
enum class Status { Ok, Error };

struct Result {
    Status status{Status::Ok};
    std::string message;

    static Result ok() { return {Status::Ok, {}}; }
    static Result error(std::string msg) { return {Status::Error, std::move(msg)}; }

    bool is_ok() const { return status == Status::Ok; }
};

}  // namespace myprj
