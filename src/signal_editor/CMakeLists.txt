# =============================================================================
# signal_editor — core (no Qt dependency)
# =============================================================================
add_library(signal_editor_core ${SIGNAL_EDITOR_LIBRARY_TYPE}
    # --- Domain ---
    core/domain/signal.cpp
    core/domain/signal_library.cpp
    # --- Use cases ---
    core/usecases/signal_editor_service.cpp
    # --- Adapters (driven, no framework) ---
    adapters/filesystem/tabular_signal_rows.cpp
    adapters/filesystem/csv_signal_repository.cpp
    adapters/filesystem/delimited_signal_repository.cpp
    adapters/filesystem/json_signal_repository.cpp
    adapters/filesystem/spreadsheet_xml_signal_repository.cpp
    adapters/filesystem/xlsx_signal_repository.cpp
    adapters/filesystem/signal_file_repository.cpp
    # --- API ---
    api/signal_editor_api.cpp
)

target_include_directories(signal_editor_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(signal_editor_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

set(SIGNAL_EDITOR_LIBZIP_HINTS
    "C:/eng_apps/msys64/mingw64/include"
    "C:/eng_apps/msys64/mingw64/lib"
    "/mnt/c/eng_apps/msys64/mingw64/include"
    "/mnt/c/eng_apps/msys64/mingw64/lib"
)

find_path(SIGNAL_EDITOR_LIBZIP_INCLUDE_DIR zip.h
    HINTS ${SIGNAL_EDITOR_LIBZIP_HINTS}
)
find_library(SIGNAL_EDITOR_LIBZIP_LIBRARY NAMES zip libzip
    HINTS ${SIGNAL_EDITOR_LIBZIP_HINTS}
)
if(SIGNAL_EDITOR_LIBZIP_INCLUDE_DIR AND SIGNAL_EDITOR_LIBZIP_LIBRARY)
    target_include_directories(signal_editor_core PRIVATE ${SIGNAL_EDITOR_LIBZIP_INCLUDE_DIR})
    target_link_libraries(signal_editor_core PRIVATE ${SIGNAL_EDITOR_LIBZIP_LIBRARY})
    target_compile_definitions(signal_editor_core PRIVATE SIGNAL_EDITOR_HAS_LIBZIP=1)
endif()

myprj_configure_target(signal_editor_core)
set_target_properties(signal_editor_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

configure_file(
    ${CMAKE_SOURCE_DIR}/include/signal_editor/signal_editor_version.h.in
    ${CMAKE_BINARY_DIR}/generated/signal_editor/signal_editor_version.h
    @ONLY
)

# =============================================================================
# signal_editor — Qt adapter (only built when GUI is enabled)
# =============================================================================
if(SIGNAL_EDITOR_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    add_library(signal_editor_qt STATIC
        adapters/qt/file_list_panel.cpp
        adapters/qt/file_list_panel.h
        adapters/qt/main_window.cpp
        adapters/qt/main_window.h
        adapters/qt/signal_list_panel.cpp
        adapters/qt/signal_list_panel.h
        adapters/qt/signal_plot_widget.cpp
        adapters/qt/signal_plot_widget.h
        adapters/qt/signal_table_panel.cpp
        adapters/qt/signal_table_panel.h
        adapters/qt/theme.cpp
        adapters/qt/theme.h
    )

    target_include_directories(signal_editor_qt
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_BINARY_DIR}/generated
    )

    target_link_libraries(signal_editor_qt
        PUBLIC
            signal_editor_core
            Qt6::Widgets
    )

    target_compile_definitions(signal_editor_qt
        PUBLIC
            SIGNAL_EDITOR_UI_APP_ID="${_SIGNAL_EDITOR_UI_APP_ID}"
            SIGNAL_EDITOR_UI_SETTINGS_VERSION="v${SIGNAL_EDITOR_SETTINGS_STORAGE_VERSION}"
    )

    if(TARGET lib_qt_utils)
        target_link_libraries(signal_editor_qt PUBLIC lib_qt_utils)
        target_compile_definitions(signal_editor_qt PUBLIC LIB_QT_UTILS_AVAILABLE)
    endif()

    if(TARGET lib_qt_custom_widgets)
        target_link_libraries(signal_editor_qt PUBLIC lib_qt_custom_widgets)
        target_compile_definitions(signal_editor_qt PUBLIC LIB_QT_CUSTOM_WIDGETS_AVAILABLE)
    endif()

    set_target_properties(signal_editor_qt PROPERTIES
        AUTOMOC ON
        POSITION_INDEPENDENT_CODE ON
    )

    myprj_configure_target(signal_editor_qt)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(signal_editor_qt PRIVATE
            -Wno-useless-cast
            -Wno-shadow
            -Wno-conversion
            -Wno-double-promotion
        )
    endif()
endif()
