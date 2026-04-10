cmake_minimum_required(VERSION 3.25)

project(SignalEditor
    VERSION 0.1.0
    DESCRIPTION "C++23 / Qt6 reimplementation of the MATLAB Signal Editor (hexagonal architecture)"
    LANGUAGES CXX
)

# =============================================================================
# Module versions (independent from project version)
# =============================================================================
set(MYPRJ_SIGNAL_EDITOR_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(MYPRJ_BUILD_TESTS       "Build unit tests"                                    ON)
option(MYPRJ_BUILD_GUI         "Build GUI applications (requires Qt6)"               OFF)
option(MYPRJ_BUILD_SHARED      "Build shared libraries"                              ON)
option(MYPRJ_ENABLE_SANITIZERS "Enable sanitizers in debug builds"                   OFF)
option(MYPRJ_STATIC_CPP_RUNTIME "Link C++ runtime statically on Windows"             OFF)

# Include project options (warnings, sanitizers, MinGW helpers, etc.)
include(cmake/ProjectOptions.cmake)

# Set library type based on option
if(MYPRJ_BUILD_SHARED)
    set(MYPRJ_LIBRARY_TYPE SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(MYPRJ_LIBRARY_TYPE STATIC)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific: put DLLs next to executables
if(WIN32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/generated)

# Generate version header from template
configure_file(
    ${CMAKE_SOURCE_DIR}/include/myprj/version.h.in
    ${CMAKE_BINARY_DIR}/generated/myprj/version.h
    @ONLY
)

# =============================================================================
# Fetch dependencies
# =============================================================================
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# GoogleTest (if tests enabled)
if(MYPRJ_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    include(GoogleTest)
endif()

# Qt6 (if GUI enabled)
if(MYPRJ_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    find_package(Qt6 QUIET COMPONENTS Svg SvgWidgets Pdf PdfWidgets Network)
    qt_standard_project_setup()
endif()

# =============================================================================
# Subdirectories — libraries
# =============================================================================
add_subdirectory(src/common)

file(GLOB MYPRJ_MODULE_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} src/*/CMakeLists.txt)
set(MYPRJ_DISCOVERED_MODULES)
foreach(module_cmakelists ${MYPRJ_MODULE_CMAKELISTS})
    get_filename_component(module_dir ${module_cmakelists} DIRECTORY)
    get_filename_component(module_name ${module_dir} NAME)
    if(module_name STREQUAL "common")
        continue()
    endif()
    add_subdirectory(${module_dir})
    list(APPEND MYPRJ_DISCOVERED_MODULES ${module_name})
endforeach()

# =============================================================================
# Subdirectories — applications
# =============================================================================
file(GLOB MYPRJ_CLI_APP_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} apps/*/cli/CMakeLists.txt)
set(MYPRJ_DISCOVERED_CLI_APPS)
foreach(app_cmakelists ${MYPRJ_CLI_APP_CMAKELISTS})
    get_filename_component(app_dir ${app_cmakelists} DIRECTORY)
    get_filename_component(app_root ${app_dir} DIRECTORY)
    get_filename_component(app_name ${app_root} NAME)
    add_subdirectory(${app_dir})
    list(APPEND MYPRJ_DISCOVERED_CLI_APPS ${app_name})
endforeach()

if(MYPRJ_BUILD_GUI)
    if(EXISTS ${CMAKE_SOURCE_DIR}/apps/common/gui/CMakeLists.txt)
        add_subdirectory(apps/common/gui)
    endif()

    file(GLOB MYPRJ_GUI_APP_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} apps/*/gui/CMakeLists.txt)
    set(MYPRJ_DISCOVERED_GUI_APPS)
    foreach(app_cmakelists ${MYPRJ_GUI_APP_CMAKELISTS})
        get_filename_component(app_dir ${app_cmakelists} DIRECTORY)
        get_filename_component(app_root ${app_dir} DIRECTORY)
        get_filename_component(app_name ${app_root} NAME)
        if(app_name STREQUAL "common")
            continue()
        endif()
        add_subdirectory(${app_dir})
        list(APPEND MYPRJ_DISCOVERED_GUI_APPS ${app_name})
    endforeach()
endif()

# =============================================================================
# Tests
# =============================================================================
if(MYPRJ_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# =============================================================================
# Installation
# =============================================================================
include(GNUInstallDirs)

install(DIRECTORY include/myprj
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(MYPRJ_BUILD_GUI AND MYPRJ_DISCOVERED_GUI_APPS)
    set(MYPRJ_GUI_INSTALL_TARGETS)
    foreach(app_name ${MYPRJ_DISCOVERED_GUI_APPS})
        list(APPEND MYPRJ_GUI_INSTALL_TARGETS myprj_${app_name}_gui)
    endforeach()
    install(TARGETS ${MYPRJ_GUI_INSTALL_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
