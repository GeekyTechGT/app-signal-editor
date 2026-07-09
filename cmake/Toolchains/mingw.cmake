# mingw.cmake - MinGW-w64 toolchain (Windows build path)
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/mingw.cmake ...
#
# This mirrors the compiler selection already embedded in CMakePresets.json's
# windows-mingw64-base preset. It is provided as an alternative,
# preset-independent entry point; the presets themselves are unchanged and
# remain the supported way to build this project.
#
# Adjust the paths below if your MinGW-w64 toolchain is installed elsewhere.

set(CMAKE_C_COMPILER   "C:/eng_apps/msys64/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/eng_apps/msys64/mingw64/bin/g++.exe")

if(EXISTS "C:/eng_apps/msys64/mingw64/bin/ninja.exe")
    set(CMAKE_MAKE_PROGRAM "C:/eng_apps/msys64/mingw64/bin/ninja.exe")
endif()
