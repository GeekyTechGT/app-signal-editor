if(NOT SIGNAL_EDITOR_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Svg Pdf PdfWidgets)

qt_add_executable(signal_editor_gui
    bootstrap.h
    bootstrap.cpp
    main.cpp
    resources.qrc
)

set_target_properties(signal_editor_gui PROPERTIES
    AUTORCC ON
)

target_link_libraries(signal_editor_gui
    PRIVATE
        signal_editor_qt
        signal_editor_core
        Qt6::Pdf
        Qt6::PdfWidgets
        Qt6::Svg
        Qt6::Widgets
)

# ── lib-qt-custom-widgets ─────────────────────────────────────────────────────
if(TARGET lib_qt_custom_widgets)
    target_link_libraries(signal_editor_gui PRIVATE lib_qt_custom_widgets)
endif()

# ── Translations ──────────────────────────────────────────────────────────────
set(SIGNAL_EDITOR_TS_FILES
    "${CMAKE_SOURCE_DIR}/resources/translations/signal_editor_en.ts"
    "${CMAKE_SOURCE_DIR}/resources/translations/signal_editor_it.ts"
)

qt_add_translations(signal_editor_gui
    TS_FILES ${SIGNAL_EDITOR_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE SIGNAL_EDITOR_QM_FILES
    LUPDATE_OPTIONS "-no-obsolete"
)

# Copy runtime assets next to the executable. This is a normal build target,
# not a link post-build step, so changed .ts files refresh the deployed .qm
# files even when the executable itself does not relink.
add_custom_target(copy_signal_editor_gui_runtime_assets
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:signal_editor_gui>/translations"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SIGNAL_EDITOR_QM_FILES}
        "$<TARGET_FILE_DIR:signal_editor_gui>/translations"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:signal_editor_gui>/docs/user"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/docs/user/user_manual.pdf"
        "$<TARGET_FILE_DIR:signal_editor_gui>/docs/user/user_manual.pdf"
    DEPENDS
        ${SIGNAL_EDITOR_QM_FILES}
        "${CMAKE_SOURCE_DIR}/docs/user/user_manual.pdf"
    COMMENT "Copying Signal Editor GUI runtime assets"
    COMMAND_EXPAND_LISTS
)
add_dependencies(signal_editor_gui copy_signal_editor_gui_runtime_assets)

set_target_properties(signal_editor_gui PROPERTIES
    OUTPUT_NAME "signal-editor-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(signal_editor_gui)
# NOTE: Qt runtime deployment (windeployqt) is performed by the deploy task —
# see scripts/project_manager.py → action_deploy_app.
