# Install.cmake - installation rules

function(myprj_install_targets)
    include(GNUInstallDirs)

    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/signal_editor
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    if(SIGNAL_EDITOR_BUILD_GUI AND SIGNAL_EDITOR_DISCOVERED_GUI_APPS)
        set(_gui_install_targets)
        foreach(app_name ${SIGNAL_EDITOR_DISCOVERED_GUI_APPS})
            if(TARGET ${app_name}_gui)
                list(APPEND _gui_install_targets ${app_name}_gui)
            endif()
        endforeach()
        if(_gui_install_targets)
            install(TARGETS ${_gui_install_targets}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endif()
endfunction()
