# Version.cmake - version header generation + build metadata from project.json

set(SIGNAL_EDITOR_BUILD_NUMBER 0)
if(EXISTS "${CMAKE_SOURCE_DIR}/project.json")
    file(READ "${CMAKE_SOURCE_DIR}/project.json" _SIGNAL_EDITOR_VERSION_JSON)
    string(JSON _SIGNAL_EDITOR_BUILD_FROM_JSON
        ERROR_VARIABLE _SIGNAL_EDITOR_BUILD_ERR
        GET ${_SIGNAL_EDITOR_VERSION_JSON} "build")
    if(NOT _SIGNAL_EDITOR_BUILD_ERR)
        set(SIGNAL_EDITOR_BUILD_NUMBER "${_SIGNAL_EDITOR_BUILD_FROM_JSON}")
    endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/Git.cmake)

configure_file(
    ${CMAKE_SOURCE_DIR}/include/signal_editor/version.h.in
    ${CMAKE_BINARY_DIR}/generated/signal_editor/version.h
    @ONLY
)
