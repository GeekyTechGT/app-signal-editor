# StaticAnalyzers.cmake - optional clang-tidy / cppcheck integration
#
# NOTE: clang-tidy runs as a *static analysis tool* alongside the GCC/MinGW
# compile — it does not compile the project with Clang. This does not
# conflict with the project's GCC/MinGW-only compiler policy (see CLAUDE.md).
#
# Both are off by default; enable explicitly with:
#   -DSIGNAL_EDITOR_ENABLE_CLANG_TIDY=ON
#   -DSIGNAL_EDITOR_ENABLE_CPPCHECK=ON

option(SIGNAL_EDITOR_ENABLE_CLANG_TIDY "Run clang-tidy as a static analyzer alongside the build" OFF)
option(SIGNAL_EDITOR_ENABLE_CPPCHECK   "Run cppcheck as a static analyzer alongside the build"   OFF)

if(SIGNAL_EDITOR_ENABLE_CLANG_TIDY)
    find_program(SIGNAL_EDITOR_CLANG_TIDY_EXE NAMES clang-tidy)
    if(SIGNAL_EDITOR_CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${SIGNAL_EDITOR_CLANG_TIDY_EXE}")
    else()
        message(WARNING "SIGNAL_EDITOR_ENABLE_CLANG_TIDY is ON but clang-tidy was not found in PATH.")
    endif()
endif()

if(SIGNAL_EDITOR_ENABLE_CPPCHECK)
    find_program(SIGNAL_EDITOR_CPPCHECK_EXE NAMES cppcheck)
    if(SIGNAL_EDITOR_CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK "${SIGNAL_EDITOR_CPPCHECK_EXE}" "--enable=warning,performance,portability")
    else()
        message(WARNING "SIGNAL_EDITOR_ENABLE_CPPCHECK is ON but cppcheck was not found in PATH.")
    endif()
endif()
