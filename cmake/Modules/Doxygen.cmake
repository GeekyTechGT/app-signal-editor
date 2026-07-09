# Doxygen.cmake - optional `doxygen` target built from the repository root Doxyfile

function(myprj_add_doxygen_target)
    find_package(Doxygen QUIET)
    if(NOT DOXYGEN_FOUND)
        message(WARNING "SIGNAL_EDITOR_BUILD_DOCS is ON but Doxygen was not found.")
        return()
    endif()
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/Doxyfile")
        message(WARNING "SIGNAL_EDITOR_BUILD_DOCS is ON but ${CMAKE_SOURCE_DIR}/Doxyfile does not exist.")
        return()
    endif()
    add_custom_target(doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_SOURCE_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endfunction()
