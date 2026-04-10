# =============================================================================
# signal_editor — core (no Qt dependency)
# =============================================================================
add_library(myprj_signal_editor_core ${MYPRJ_LIBRARY_TYPE}
    # --- Domain ---
    core/domain/signal.cpp
    core/domain/signal_library.cpp
    # --- Use cases ---
    core/usecases/signal_editor_service.cpp
    # --- Adapters (driven, no framework) ---
    adapters/filesystem/csv_signal_repository.cpp
    # --- API ---
    api/signal_editor_api.cpp
)

target_include_directories(myprj_signal_editor_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(myprj_signal_editor_core
    PUBLIC
        myprj_common
        nlohmann_json::nlohmann_json
)

myprj_configure_target(myprj_signal_editor_core)

configure_file(
    ${CMAKE_SOURCE_DIR}/include/myprj/signal_editor_version.h.in
    ${CMAKE_BINARY_DIR}/generated/myprj/signal_editor_version.h
    @ONLY
)

# =============================================================================
# signal_editor — Qt adapter (only built when GUI is enabled)
#
# Lives in its own static library so the core remains framework-agnostic and a
# different GUI framework can replace this layer in the future.
# =============================================================================
if(MYPRJ_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_library(myprj_signal_editor_qt STATIC
        adapters/qt/main_window.cpp
        adapters/qt/main_window.h
        adapters/qt/signal_list_panel.cpp
        adapters/qt/signal_list_panel.h
        adapters/qt/signal_plot_widget.cpp
        adapters/qt/signal_plot_widget.h
        adapters/qt/theme.cpp
        adapters/qt/theme.h
    )

    target_include_directories(myprj_signal_editor_qt
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_BINARY_DIR}/generated
    )

    target_link_libraries(myprj_signal_editor_qt
        PUBLIC
            myprj_signal_editor_core
            Qt6::Widgets
    )

    set_target_properties(myprj_signal_editor_qt PROPERTIES
        AUTOMOC ON
        POSITION_INDEPENDENT_CODE ON
    )

    myprj_configure_target(myprj_signal_editor_qt)
    # Qt headers and arg() casts trip these specific warnings; silence them
    # only on this adapter so the core stays under strict warnings.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(myprj_signal_editor_qt PRIVATE
            -Wno-useless-cast
            -Wno-shadow
            -Wno-conversion
            -Wno-double-promotion
        )
    endif()
endif()
