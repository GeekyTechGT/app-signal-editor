if(NOT SIGNAL_EDITOR_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(signal_editor_gui
    main.cpp
)

target_link_libraries(signal_editor_gui
    PRIVATE
        signal_editor_qt
        signal_editor_core
        Qt6::Widgets
)

set_target_properties(signal_editor_gui PROPERTIES
    OUTPUT_NAME "signal-editor-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(signal_editor_gui)
# NOTE: Qt runtime deployment (windeployqt) is intentionally NOT done as a
# POST_BUILD step. It is performed by the deploy task — see
# `scripts\pm_deploy.bat` (`:do_deploy_windows`).
