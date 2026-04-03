if(NOT MYPRJ_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(myprj_proro_gui
    main.cpp
    # mainwindow.cpp
    # mainwindow.h
)

target_link_libraries(myprj_proro_gui
    PRIVATE
        myprj_proro_core
        Qt6::Widgets
)

set_target_properties(myprj_proro_gui PROPERTIES
    OUTPUT_NAME "proro-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(myprj_proro_gui)
myprj_enable_windeployqt_post_build(myprj_proro_gui)
