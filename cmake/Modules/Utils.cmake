# Utils.cmake - reusable CMake helpers

# Glob for CMakeLists.txt files matching `pattern`, add_subdirectory() each one found,
# and return the discovered module/app names (the directory name `DEPTH_STRIP` levels
# above the matched CMakeLists.txt).
#
#   myprj_discover_subdirectories(<glob-pattern> <out-var>
#       [DEPTH_STRIP n]        # default 0: name = basename of the matched dir itself
#                               #         1: name = basename of its parent (for apps/<name>/gui layouts)
#       [SKIP name1 name2 ...] # directory names to exclude (e.g. "common")
#   )
function(myprj_discover_subdirectories pattern out_var)
    cmake_parse_arguments(ARG "" "DEPTH_STRIP" "SKIP" ${ARGN})
    if(NOT DEFINED ARG_DEPTH_STRIP)
        set(ARG_DEPTH_STRIP 0)
    endif()

    file(GLOB _myprj_cmakelists RELATIVE ${CMAKE_SOURCE_DIR} ${pattern})
    set(_myprj_discovered)
    foreach(_cmakelists ${_myprj_cmakelists})
        get_filename_component(_dir ${_cmakelists} DIRECTORY)

        set(_name_dir "${_dir}")
        set(_strip_count ${ARG_DEPTH_STRIP})
        while(_strip_count GREATER 0)
            get_filename_component(_name_dir "${_name_dir}" DIRECTORY)
            math(EXPR _strip_count "${_strip_count} - 1")
        endwhile()
        get_filename_component(_name "${_name_dir}" NAME)

        if(ARG_SKIP AND _name IN_LIST ARG_SKIP)
            continue()
        endif()

        add_subdirectory(${_dir})
        list(APPEND _myprj_discovered ${_name})
    endforeach()

    set(${out_var} ${_myprj_discovered} PARENT_SCOPE)
endfunction()
