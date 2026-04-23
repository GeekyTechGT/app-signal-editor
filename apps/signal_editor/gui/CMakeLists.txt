if(NOT SIGNAL_EDITOR_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools Svg)

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

# Copy compiled .qm files alongside the executable
foreach(_qm ${SIGNAL_EDITOR_QM_FILES})
    get_filename_component(_qm_name "${_qm}" NAME)
    add_custom_command(TARGET signal_editor_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:signal_editor_gui>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qm}"
            "$<TARGET_FILE_DIR:signal_editor_gui>/translations/${_qm_name}"
        COMMENT "Copying ${_qm_name}"
    )
endforeach()

set_target_properties(signal_editor_gui PROPERTIES
    OUTPUT_NAME "signal-editor-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(signal_editor_gui)
# NOTE: Qt runtime deployment (windeployqt) is performed by the deploy task —
# see scripts/project_manager.py → action_deploy_app.
