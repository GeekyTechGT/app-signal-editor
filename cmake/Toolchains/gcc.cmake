# gcc.cmake - GCC toolchain (Linux / Docker build path)
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/gcc.cmake ...
#
# This mirrors the compiler selection already embedded in CMakePresets.json's
# linux-gcc-base / docker-gcc-base presets. It is provided as an alternative,
# preset-independent entry point; the presets themselves are unchanged and
# remain the supported way to build this project.

set(CMAKE_C_COMPILER   gcc)
set(CMAKE_CXX_COMPILER g++)
