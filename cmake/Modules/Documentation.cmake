# Documentation.cmake - documentation targets, gated by SIGNAL_EDITOR_BUILD_DOCS

option(SIGNAL_EDITOR_BUILD_DOCS "Add a 'doxygen' build target (requires Doxygen)" OFF)

include(${CMAKE_CURRENT_LIST_DIR}/Doxygen.cmake)

if(SIGNAL_EDITOR_BUILD_DOCS)
    myprj_add_doxygen_target()
endif()
