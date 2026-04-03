cmake_minimum_required(VERSION 3.25)

project(MyProject
    VERSION 0.1.0
    DESCRIPTION "Cross-platform C++ application scaffold with CLI, GUI, tests, and deployment workflows"
    LANGUAGES CXX
)

# =============================================================================
# Module versions (independent from project version)
# =============================================================================
set(MYPRJ_MY_MODULE_VERSION "0.1.0")
# Add more modules here:
# set(MYPRJ_OTHER_MODULE_VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(MYPRJ_BUILD_TESTS       "Build unit tests"                                    ON)
option(MYPRJ_BUILD_GUI         "Build GUI applications (requires Qt6)"               OFF)
option(MYPRJ_BUILD_SHARED      "Build shared libraries"                              ON)
option(MYPRJ_ENABLE_SANITIZERS "Enable sanitizers in debug builds"                   OFF)
option(MYPRJ_USE_SYSTEM_OPENSSL "Prefer OpenSSL from system packages"                ON)
option(MYPRJ_STATIC_CPP_RUNTIME "Link C++ runtime statically on Windows"             ON)

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
# OpenSSL dependency — remove this section if OpenSSL is not needed
# =============================================================================
if(MYPRJ_USE_SYSTEM_OPENSSL)
    if(MINGW)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        get_filename_component(MINGW_ROOT ${MINGW_BIN_DIR} DIRECTORY)
        list(APPEND CMAKE_PREFIX_PATH "${MINGW_ROOT}")
        set(OPENSSL_ROOT_DIR "${MINGW_ROOT}")
    endif()
    find_package(OpenSSL QUIET)
endif()

if(NOT OpenSSL_FOUND)
    message(STATUS "OpenSSL not found on system, fetching and building it...")
    include(ExternalProject)

    set(OPENSSL_VERSION "3.1.6")
    set(OPENSSL_PREFIX ${CMAKE_BINARY_DIR}/_deps/openssl-install)

    if(WIN32)
        if(MINGW)
            set(OPENSSL_CONFIGURE_TARGET "mingw64")
        else()
            set(OPENSSL_CONFIGURE_TARGET "VC-WIN64A")
        endif()
        set(OPENSSL_LIB_SUFFIX ".a")
        set(OPENSSL_MAKE_COMMAND "mingw32-make")
    else()
        set(OPENSSL_CONFIGURE_TARGET "linux-x86_64")
        set(OPENSSL_LIB_SUFFIX ".a")
        set(OPENSSL_MAKE_COMMAND "make")
    endif()

    find_program(PERL_EXECUTABLE perl REQUIRED)

    ExternalProject_Add(openssl_ep
        URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
        URL_HASH SHA256=5d2be4036b478ef3cb0a854ca9b353072c3a0e26d8a56f8f0ab9fb6ed32d38d7
        PREFIX ${CMAKE_BINARY_DIR}/_deps/openssl
        CONFIGURE_COMMAND
            ${PERL_EXECUTABLE} Configure
            ${OPENSSL_CONFIGURE_TARGET}
            no-shared
            no-tests
            --prefix=${OPENSSL_PREFIX}
        BUILD_COMMAND ${OPENSSL_MAKE_COMMAND}
        INSTALL_COMMAND ${OPENSSL_MAKE_COMMAND} install_sw
        BUILD_IN_SOURCE 1
        LOG_DOWNLOAD ON LOG_CONFIGURE ON LOG_BUILD ON LOG_INSTALL ON
    )

    add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
    add_library(OpenSSL::SSL    STATIC IMPORTED GLOBAL)
    add_dependencies(OpenSSL::Crypto openssl_ep)
    add_dependencies(OpenSSL::SSL    openssl_ep)

    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION ${OPENSSL_PREFIX}/lib/libcrypto${OPENSSL_LIB_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_PREFIX}/include
    )
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION ${OPENSSL_PREFIX}/lib/libssl${OPENSSL_LIB_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_PREFIX}/include
        INTERFACE_LINK_LIBRARIES OpenSSL::Crypto
    )

    if(WIN32)
        set_property(TARGET OpenSSL::Crypto APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES ws2_32 crypt32
        )
    endif()

    file(MAKE_DIRECTORY ${OPENSSL_PREFIX}/include)
    set(OPENSSL_FOUND TRUE)
    set(OPENSSL_INCLUDE_DIR ${OPENSSL_PREFIX}/include)
    set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_PREFIX}/lib/libcrypto${OPENSSL_LIB_SUFFIX})
    set(OPENSSL_SSL_LIBRARY    ${OPENSSL_PREFIX}/lib/libssl${OPENSSL_LIB_SUFFIX})
    message(STATUS "OpenSSL ${OPENSSL_VERSION} will be built from source")
else()
    message(STATUS "Using system OpenSSL: ${OPENSSL_VERSION}")
    myprj_copy_openssl_dlls()
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

set(MYPRJ_INSTALL_TARGETS myprj_common)
foreach(module_name ${MYPRJ_DISCOVERED_MODULES})
    list(APPEND MYPRJ_INSTALL_TARGETS myprj_${module_name}_core)
endforeach()
install(TARGETS ${MYPRJ_INSTALL_TARGETS}
    EXPORT MyProjectTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(MYPRJ_DISCOVERED_CLI_APPS)
    set(MYPRJ_CLI_INSTALL_TARGETS)
    foreach(app_name ${MYPRJ_DISCOVERED_CLI_APPS})
        list(APPEND MYPRJ_CLI_INSTALL_TARGETS myprj_${app_name}_cli)
    endforeach()
    install(TARGETS ${MYPRJ_CLI_INSTALL_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(MYPRJ_BUILD_GUI AND MYPRJ_DISCOVERED_GUI_APPS)
    set(MYPRJ_GUI_INSTALL_TARGETS)
    foreach(app_name ${MYPRJ_DISCOVERED_GUI_APPS})
        list(APPEND MYPRJ_GUI_INSTALL_TARGETS myprj_${app_name}_gui)
    endforeach()
    install(TARGETS ${MYPRJ_GUI_INSTALL_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(EXPORT MyProjectTargets
    FILE MyProjectTargets.cmake
    NAMESPACE MyPrj::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MyProject
)
