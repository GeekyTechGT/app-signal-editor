cmake_minimum_required(VERSION 3.25)

project(SignalEditor
    VERSION 0.1.0
    DESCRIPTION "C++23 / Qt6 reimplementation of the MATLAB Signal Editor (hexagonal architecture)"
    LANGUAGES CXX
)

# =============================================================================
# Module versions (independent from project version)
# =============================================================================
set(SIGNAL_EDITOR_MODULE_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SIGNAL_EDITOR_BUILD_TESTS        "Build unit tests"                                         ON)
option(SIGNAL_EDITOR_BUILD_GUI          "Build GUI applications (requires Qt6)"                    OFF)
option(SIGNAL_EDITOR_BUILD_SHARED       "Build shared libraries"                                   ON)
option(SIGNAL_EDITOR_ENABLE_SANITIZERS  "Enable sanitizers in debug builds"                        OFF)
option(SIGNAL_EDITOR_ENABLE_COVERAGE    "Enable coverage instrumentation in supported debug builds" OFF)
option(SIGNAL_EDITOR_STATIC_CPP_RUNTIME "Link C++ runtime statically on Windows"                   OFF)
option(SIGNAL_EDITOR_USE_EXTERNAL_DEPS  "Load shared dependencies from external/ instead of shared_lib" OFF)

# Include project options (warnings, sanitizers, MinGW helpers, etc.)
include(cmake/ProjectOptions.cmake)

# Set library type based on option
if(SIGNAL_EDITOR_BUILD_SHARED)
    set(SIGNAL_EDITOR_LIBRARY_TYPE SHARED)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    set(SIGNAL_EDITOR_LIBRARY_TYPE STATIC)
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
    ${CMAKE_SOURCE_DIR}/include/signal_editor/version.h.in
    ${CMAKE_BINARY_DIR}/generated/signal_editor/version.h
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
if(SIGNAL_EDITOR_BUILD_TESTS)
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
if(SIGNAL_EDITOR_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Svg Pdf PdfWidgets)
    find_package(Qt6 QUIET COMPONENTS SvgWidgets Network)
    qt_standard_project_setup()

    set(SIGNAL_EDITOR_SETTINGS_STORAGE_VERSION "1.0.0"
        CACHE STRING "Version namespace used for persisted UI settings")

    if(EXISTS "${CMAKE_SOURCE_DIR}/project.json")
        file(READ "${CMAKE_SOURCE_DIR}/project.json" _SIGNAL_EDITOR_PROJECT_JSON)
    else()
        set(_SIGNAL_EDITOR_PROJECT_JSON "")
    endif()

    if(_SIGNAL_EDITOR_PROJECT_JSON)
        string(JSON _SIGNAL_EDITOR_UI_APP_ID
            ERROR_VARIABLE _SIGNAL_EDITOR_UI_APP_ID_ERR
            GET ${_SIGNAL_EDITOR_PROJECT_JSON} "name")
        if(_SIGNAL_EDITOR_UI_APP_ID_ERR)
            set(_SIGNAL_EDITOR_UI_APP_ID "signal-editor")
        endif()
    else()
        set(_SIGNAL_EDITOR_UI_APP_ID "signal-editor")
    endif()

    if(NOT SIGNAL_EDITOR_DEPS_VARIANT)
        if(CMAKE_PRESET_NAME AND _SIGNAL_EDITOR_PROJECT_JSON)
            string(JSON _SIGNAL_EDITOR_DEPS_VARIANT
                ERROR_VARIABLE _SIGNAL_EDITOR_MAP_ERR
                GET ${_SIGNAL_EDITOR_PROJECT_JSON}
                "preset_to_shared_lib_variant" "${CMAKE_PRESET_NAME}")
            if(_SIGNAL_EDITOR_MAP_ERR)
                set(_SIGNAL_EDITOR_DEPS_VARIANT "${CMAKE_PRESET_NAME}")
            endif()
        else()
            set(_SIGNAL_EDITOR_DEPS_VARIANT "${CMAKE_PRESET_NAME}")
        endif()
        set(SIGNAL_EDITOR_DEPS_VARIANT "${_SIGNAL_EDITOR_DEPS_VARIANT}"
            CACHE STRING "Shared-lib variant folder (e.g. windows-mingw64-debug)")
    endif()

    if(NOT SIGNAL_EDITOR_USE_EXTERNAL_DEPS)
        if(_SIGNAL_EDITOR_PROJECT_JSON)
            string(JSON _SIGNAL_EDITOR_SLB GET ${_SIGNAL_EDITOR_PROJECT_JSON} "shared_lib_base")
        else()
            set(_SIGNAL_EDITOR_SLB "")
        endif()
    endif()

    # ── Resolve lib-qt-custom-widgets root ───────────────────────────────────
    #
    # Two modes, matching the use_external_deps flag in project.json:
    #
    #   SIGNAL_EDITOR_USE_EXTERNAL_DEPS=OFF  (default)
    #     Pre-built binaries from shared_lib_base/<name>/<version>/<variant>/
    #
    #   SIGNAL_EDITOR_USE_EXTERNAL_DEPS=ON
    #     Vendored layout inside external/<name>/  (include/ lib/ bin/)
    #
    if(SIGNAL_EDITOR_USE_EXTERNAL_DEPS)
        set(_LIB_QT_CW_ROOT "${CMAKE_SOURCE_DIR}/external/lib-qt-custom-widgets")
        message(STATUS "lib-qt-custom-widgets: using external/ layout at ${_LIB_QT_CW_ROOT}")
    else()
        if(_SIGNAL_EDITOR_SLB AND SIGNAL_EDITOR_DEPS_VARIANT)
            set(_LIB_QT_CW_ROOT
                "${_SIGNAL_EDITOR_SLB}/lib-qt-custom-widgets/1.0.0/${SIGNAL_EDITOR_DEPS_VARIANT}")
            message(STATUS "lib-qt-custom-widgets: using shared_lib at ${_LIB_QT_CW_ROOT}")
        else()
            message(WARNING
                "SIGNAL_EDITOR_DEPS_VARIANT is empty — lib-qt-custom-widgets will NOT be linked.\n"
                "Set -DSIGNAL_EDITOR_DEPS_VARIANT=<variant> or run via project_manager.py.")
            set(_LIB_QT_CW_ROOT "")
        endif()
    endif()

    # ── Resolve lib-qt-utils root ────────────────────────────────────────────
    if(SIGNAL_EDITOR_USE_EXTERNAL_DEPS)
        set(_LIB_QT_UTILS_ROOT "${CMAKE_SOURCE_DIR}/external/lib-qt-utils")
        message(STATUS "lib-qt-utils: using external/ layout at ${_LIB_QT_UTILS_ROOT}")
    else()
        if(_SIGNAL_EDITOR_SLB AND SIGNAL_EDITOR_DEPS_VARIANT)
            set(_LIB_QT_UTILS_ROOT
                "${_SIGNAL_EDITOR_SLB}/lib-qt-utils/1.0.0/${SIGNAL_EDITOR_DEPS_VARIANT}")
            message(STATUS "lib-qt-utils: using shared_lib at ${_LIB_QT_UTILS_ROOT}")
        else()
            message(WARNING
                "SIGNAL_EDITOR_DEPS_VARIANT is empty — lib-qt-utils will NOT be linked.\n"
                "Set -DSIGNAL_EDITOR_DEPS_VARIANT=<variant> or run via project_manager.py.")
            set(_LIB_QT_UTILS_ROOT "")
        endif()
    endif()

    # ── Import lib-qt-custom-widgets ──────────────────────────────────────────
    if(_LIB_QT_CW_ROOT)
        if(NOT EXISTS "${_LIB_QT_CW_ROOT}")
            message(FATAL_ERROR
                "=================================================================\n"
                "  DEPENDENCY ERROR: lib-qt-custom-widgets not found!\n"
                "  Expected path : ${_LIB_QT_CW_ROOT}\n"
                "  Preset name   : ${CMAKE_PRESET_NAME}\n"
                "  Deps variant  : ${SIGNAL_EDITOR_DEPS_VARIANT}\n"
                "  use_external  : ${SIGNAL_EDITOR_USE_EXTERNAL_DEPS}\n"
                "-----------------------------------------------------------------\n"
                "  Either:\n"
                "    a) Build & deploy lib-qt-custom-widgets for the required variant\n"
                "    b) Set SIGNAL_EDITOR_USE_EXTERNAL_DEPS=ON and place the library\n"
                "       under external/lib-qt-custom-widgets/ (include/ lib/ bin/)\n"
                "    c) Run scripts/project_manager.py (validates deps automatically)\n"
                "=================================================================")
        endif()

        set(_LIB_QT_CW_INC "${_LIB_QT_CW_ROOT}/include")
        set(_LIB_QT_CW_LIB "${_LIB_QT_CW_ROOT}/lib/lib-qt-custom-widgets.a")
        set(_LIB_QT_CW_DLL "${_LIB_QT_CW_ROOT}/bin/lib-qt-custom-widgets.dll")

        if(NOT EXISTS "${_LIB_QT_CW_LIB}")
            message(FATAL_ERROR
                "=================================================================\n"
                "  DEPENDENCY ERROR: lib-qt-custom-widgets.a not found!\n"
                "  Looked at: ${_LIB_QT_CW_LIB}\n"
                "=================================================================")
        endif()

        # Collect Qt interface deps for lib-qt-custom-widgets.
        # The static archive uses QSvgWidget/QSvgRenderer, so Svg/SvgWidgets
        # must appear *after* the archive in the GCC single-pass link line.
        set(_LIB_QT_CW_IFACE "Qt6::Widgets;Qt6::Gui;Qt6::Core")
        foreach(_qt_cw_comp Qt6::Svg Qt6::SvgWidgets Qt6::Pdf Qt6::PdfWidgets)
            if(TARGET ${_qt_cw_comp})
                string(APPEND _LIB_QT_CW_IFACE ";${_qt_cw_comp}")
            endif()
        endforeach()

        add_library(lib_qt_custom_widgets STATIC IMPORTED GLOBAL)
        set_target_properties(lib_qt_custom_widgets PROPERTIES
            IMPORTED_LOCATION             "${_LIB_QT_CW_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_LIB_QT_CW_INC}"
            INTERFACE_LINK_LIBRARIES      "${_LIB_QT_CW_IFACE}"
        )

        if(WIN32 AND EXISTS "${_LIB_QT_CW_DLL}")
            add_custom_target(copy_lib_qt_custom_widgets_dll ALL
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_LIB_QT_CW_DLL}"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib-qt-custom-widgets.dll"
                COMMENT "Copying lib-qt-custom-widgets.dll"
            )
        endif()
    endif()

    # ── Import lib-qt-utils ──────────────────────────────────────────────────
    if(_LIB_QT_UTILS_ROOT)
        if(NOT EXISTS "${_LIB_QT_UTILS_ROOT}")
            message(FATAL_ERROR
                "=================================================================\n"
                "  DEPENDENCY ERROR: lib-qt-utils not found!\n"
                "  Expected path : ${_LIB_QT_UTILS_ROOT}\n"
                "  Preset name   : ${CMAKE_PRESET_NAME}\n"
                "  Deps variant  : ${SIGNAL_EDITOR_DEPS_VARIANT}\n"
                "  use_external  : ${SIGNAL_EDITOR_USE_EXTERNAL_DEPS}\n"
                "-----------------------------------------------------------------\n"
                "  Either:\n"
                "    a) Build & deploy lib-qt-utils for the required variant\n"
                "    b) Set SIGNAL_EDITOR_USE_EXTERNAL_DEPS=ON and place the library\n"
                "       under external/lib-qt-utils/ (include/ lib/ bin/)\n"
                "    c) Run scripts/project_manager.py (validates deps automatically)\n"
                "=================================================================")
        endif()

        set(_LIB_QT_UTILS_INC "${_LIB_QT_UTILS_ROOT}/include")
        set(_LIB_QT_UTILS_LIB "${_LIB_QT_UTILS_ROOT}/lib/lib-qt-utils.a")
        set(_LIB_QT_UTILS_DLL "${_LIB_QT_UTILS_ROOT}/bin/lib-qt-utils.dll")

        if(NOT EXISTS "${_LIB_QT_UTILS_LIB}")
            message(FATAL_ERROR
                "=================================================================\n"
                "  DEPENDENCY ERROR: lib-qt-utils.a not found!\n"
                "  Looked at: ${_LIB_QT_UTILS_LIB}\n"
                "=================================================================")
        endif()

        add_library(lib_qt_utils STATIC IMPORTED GLOBAL)
        set_target_properties(lib_qt_utils PROPERTIES
            IMPORTED_LOCATION             "${_LIB_QT_UTILS_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_LIB_QT_UTILS_INC}"
            INTERFACE_LINK_LIBRARIES      "Qt6::Core"
        )

        if(WIN32 AND EXISTS "${_LIB_QT_UTILS_DLL}")
            add_custom_target(copy_lib_qt_utils_dll ALL
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_LIB_QT_UTILS_DLL}"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib-qt-utils.dll"
                COMMENT "Copying lib-qt-utils.dll"
            )
        endif()
    endif()
endif()

# =============================================================================
# Subdirectories — libraries
# =============================================================================
file(GLOB SIGNAL_EDITOR_MODULE_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} src/*/CMakeLists.txt)
set(SIGNAL_EDITOR_DISCOVERED_MODULES)
foreach(module_cmakelists ${SIGNAL_EDITOR_MODULE_CMAKELISTS})
    get_filename_component(module_dir ${module_cmakelists} DIRECTORY)
    get_filename_component(module_name ${module_dir} NAME)
    if(module_name STREQUAL "common")
        continue()
    endif()
    add_subdirectory(${module_dir})
    list(APPEND SIGNAL_EDITOR_DISCOVERED_MODULES ${module_name})
endforeach()

# =============================================================================
# Subdirectories — applications
# =============================================================================
file(GLOB SIGNAL_EDITOR_CLI_APP_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} apps/*/cli/CMakeLists.txt)
set(SIGNAL_EDITOR_DISCOVERED_CLI_APPS)
foreach(app_cmakelists ${SIGNAL_EDITOR_CLI_APP_CMAKELISTS})
    get_filename_component(app_dir ${app_cmakelists} DIRECTORY)
    get_filename_component(app_root ${app_dir} DIRECTORY)
    get_filename_component(app_name ${app_root} NAME)
    add_subdirectory(${app_dir})
    list(APPEND SIGNAL_EDITOR_DISCOVERED_CLI_APPS ${app_name})
endforeach()

if(SIGNAL_EDITOR_BUILD_GUI)
    if(EXISTS ${CMAKE_SOURCE_DIR}/apps/common/gui/CMakeLists.txt)
        add_subdirectory(apps/common/gui)
    endif()

    file(GLOB SIGNAL_EDITOR_GUI_APP_CMAKELISTS RELATIVE ${CMAKE_SOURCE_DIR} apps/*/gui/CMakeLists.txt)
    set(SIGNAL_EDITOR_DISCOVERED_GUI_APPS)
    foreach(app_cmakelists ${SIGNAL_EDITOR_GUI_APP_CMAKELISTS})
        get_filename_component(app_dir ${app_cmakelists} DIRECTORY)
        get_filename_component(app_root ${app_dir} DIRECTORY)
        get_filename_component(app_name ${app_root} NAME)
        if(app_name STREQUAL "common")
            continue()
        endif()
        add_subdirectory(${app_dir})
        list(APPEND SIGNAL_EDITOR_DISCOVERED_GUI_APPS ${app_name})
    endforeach()
endif()

# =============================================================================
# Tests
# =============================================================================
if(SIGNAL_EDITOR_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# =============================================================================
# Installation
# =============================================================================
include(GNUInstallDirs)

install(DIRECTORY include/signal_editor
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(SIGNAL_EDITOR_BUILD_GUI AND SIGNAL_EDITOR_DISCOVERED_GUI_APPS)
    set(SIGNAL_EDITOR_GUI_INSTALL_TARGETS)
    foreach(app_name ${SIGNAL_EDITOR_DISCOVERED_GUI_APPS})
        list(APPEND SIGNAL_EDITOR_GUI_INSTALL_TARGETS ${app_name}_gui)
    endforeach()
    install(TARGETS ${SIGNAL_EDITOR_GUI_INSTALL_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
