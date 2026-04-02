if(NOT MYPRJ_BUILD_GUI)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(myprj_my_module_gui
    main.cpp
    # mainwindow.cpp
    # mainwindow.h
)

target_link_libraries(myprj_my_module_gui
    PRIVATE
        myprj_my_module_core
        Qt6::Widgets
)

set_target_properties(myprj_my_module_gui PROPERTIES
    OUTPUT_NAME "my_module-gui"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

myprj_configure_target(myprj_my_module_gui)
myprj_enable_windeployqt_post_build(myprj_my_module_gui)
