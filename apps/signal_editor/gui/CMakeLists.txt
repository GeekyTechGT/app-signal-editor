if(NOT MYPRJ_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(myprj_signal_editor_gui
    main.cpp
)

target_link_libraries(myprj_signal_editor_gui
    PRIVATE
        myprj_signal_editor_qt
        myprj_signal_editor_core
        Qt6::Widgets
)

set_target_properties(myprj_signal_editor_gui PROPERTIES
    OUTPUT_NAME "signal-editor-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(myprj_signal_editor_gui)
myprj_enable_windeployqt_post_build(myprj_signal_editor_gui)
