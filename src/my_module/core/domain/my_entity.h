#pragma once

#include <string>

namespace myprj::my_module {

// --- Domain entity ---------------------------------------------------------
// Pure domain object: no I/O, no framework dependencies.
// Business invariants are enforced in the constructor.
struct MyEntity {
    std::string id;
    std::string name;

    // Factory — validates invariants before constructing.
    static MyEntity create(std::string id, std::string name);
};

}  // namespace myprj::my_module
