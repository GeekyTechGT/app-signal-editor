# ProjectOptions.cmake - Compiler warnings, sanitizer settings, and runtime helpers

# =============================================================================
# MinGW Runtime DLLs - Copy to bin folder (when not statically linked)
# =============================================================================
if(WIN32 AND MINGW)
    get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
    set(MYPRJ_MINGW_BIN_DIR "${MINGW_BIN_DIR}" CACHE INTERNAL "MinGW bin directory")
endif()

function(myprj_setup_mingw_runtime)
    if(WIN32 AND MINGW AND DEFINED MYPRJ_MINGW_BIN_DIR)
        set(MINGW_RUNTIME_DLLS
            "libstdc++-6.dll"
            "libgcc_s_seh-1.dll"
            "libwinpthread-1.dll"
            "libssp-0.dll"
        )
        set(MINGW_DLL_COPY_COMMANDS)
        foreach(dll ${MINGW_RUNTIME_DLLS})
            if(EXISTS "${MYPRJ_MINGW_BIN_DIR}/${dll}")
                list(APPEND MINGW_DLL_COPY_COMMANDS
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MYPRJ_MINGW_BIN_DIR}/${dll}"
                        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dll}"
                )
                message(STATUS "  Will copy: ${dll}")
            endif()
        endforeach()
        if(MINGW_DLL_COPY_COMMANDS)
            add_custom_target(copy_mingw_runtime
                ${MINGW_DLL_COPY_COMMANDS}
                COMMENT "Copying MinGW runtime DLLs to bin folder"
            )
        endif()
    endif()
endfunction()

# =============================================================================
# Copy required DLLs to executable directory (Windows)
# =============================================================================
function(myprj_copy_runtime_dlls target)
    if(WIN32 AND MINGW AND DEFINED MYPRJ_MINGW_BIN_DIR)
        set(_mingw_runtime_dlls
            "libstdc++-6.dll"
            "libgcc_s_seh-1.dll"
            "libwinpthread-1.dll"
            "libssp-0.dll"
        )
        foreach(dll IN LISTS _mingw_runtime_dlls)
            if(EXISTS "${MYPRJ_MINGW_BIN_DIR}/${dll}")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MYPRJ_MINGW_BIN_DIR}/${dll}"
                        $<TARGET_FILE_DIR:${target}>
                )
            endif()
        endforeach()
    endif()
endfunction()

function(myprj_copy_mingw_extra_dlls target)
    if(WIN32 AND MINGW AND DEFINED MYPRJ_MINGW_BIN_DIR)
        foreach(dll IN LISTS ARGN)
            if(EXISTS "${MYPRJ_MINGW_BIN_DIR}/${dll}")
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${MYPRJ_MINGW_BIN_DIR}/${dll}"
                        $<TARGET_FILE_DIR:${target}>
                )
                install(FILES "${MYPRJ_MINGW_BIN_DIR}/${dll}"
                    DESTINATION ${CMAKE_INSTALL_BINDIR}
                )
            else()
                message(WARNING "MinGW extra DLL not found: ${MYPRJ_MINGW_BIN_DIR}/${dll}")
            endif()
        endforeach()
    endif()
endfunction()

# =============================================================================
# Static C++ runtime on Windows (Qt remains dynamic)
# =============================================================================
function(myprj_enable_static_cpp_runtime target)
    if(WIN32 AND MYPRJ_STATIC_CPP_RUNTIME)
        if(MSVC)
            set_property(TARGET ${target} PROPERTY
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
            )
        elseif(MINGW)
            target_link_options(${target} PRIVATE
                -static-libstdc++
                -static-libgcc
                -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic
            )
        endif()
    endif()
endfunction()

# =============================================================================
# Compiler warnings
# =============================================================================
function(myprj_set_warnings target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            -Wimplicit-fallthrough
        )
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
            )
        endif()
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    endif()
endfunction()

# =============================================================================
# Sanitizers (for debug builds)
# =============================================================================
function(myprj_enable_sanitizers target)
    if(MYPRJ_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()

# =============================================================================
# Combined function to apply all project options to a target
# =============================================================================
function(myprj_configure_target target)
    myprj_set_warnings(${target})
    myprj_enable_sanitizers(${target})
    myprj_enable_static_cpp_runtime(${target})
    myprj_copy_runtime_dlls(${target})
endfunction()

# =============================================================================
# Qt6 helpers
# =============================================================================
function(myprj_link_optional_qt_modules target)
    foreach(module IN LISTS ARGN)
        if(TARGET Qt6::${module})
            target_link_libraries(${target} PRIVATE Qt6::${module})
        endif()
    endforeach()
endfunction()

function(myprj_copy_qt_module_dlls target)
    if(NOT CMAKE_BUILD_TYPE)
        return()
    endif()
    foreach(module IN LISTS ARGN)
        if(TARGET Qt6::${module})
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Qt6::${module}>
                    $<TARGET_FILE_DIR:${target}>
            )
        endif()
    endforeach()
endfunction()

# =============================================================================
# Windows Qt deploy helper (windeployqt6 / windeployqt)
# =============================================================================
function(myprj_find_windeployqt out_var)
    if(DEFINED MYPRJ_WINDEPLOYQT_EXECUTABLE)
        set(${out_var} "${MYPRJ_WINDEPLOYQT_EXECUTABLE}" PARENT_SCOPE)
        return()
    endif()

    set(_hints)
    if(DEFINED Qt6_DIR)
        get_filename_component(_qt6_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
        list(APPEND _hints "${_qt6_prefix}/bin")
    endif()

    find_program(_windeployqt_exe
        NAMES windeployqt6 windeployqt
        HINTS ${_hints}
    )

    if(_windeployqt_exe)
        set(MYPRJ_WINDEPLOYQT_EXECUTABLE "${_windeployqt_exe}" CACHE INTERNAL "windeployqt executable")
        set(${out_var} "${_windeployqt_exe}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

function(myprj_enable_windeployqt_post_build target)
    # Manual deployment: copy the Qt6 runtime DLLs and the minimum set of
    # platform / styles / image plugins next to the executable. We do this
    # ourselves because windeployqt6 in this Qt install fails to discover its
    # own platform plugin and aborts.
    if(NOT WIN32)
        return()
    endif()
    if(NOT DEFINED Qt6_DIR)
        return()
    endif()

    get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    set(_qt_bin     "${_qt_prefix}/bin")
    set(_qt_plugins "${_qt_prefix}/plugins")

    # Pick debug or release variant; fall back to release if no debug build.
    set(_qt_modules Core Gui Widgets)
    foreach(_mod IN LISTS _qt_modules)
        set(_chosen "")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND
           EXISTS "${_qt_bin}/Qt6${_mod}d.dll")
            set(_chosen "Qt6${_mod}d.dll")
        elseif(EXISTS "${_qt_bin}/Qt6${_mod}.dll")
            set(_chosen "Qt6${_mod}.dll")
        endif()
        if(_chosen)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_qt_bin}/${_chosen}" "$<TARGET_FILE_DIR:${target}>/${_chosen}"
            )
        endif()
    endforeach()

    set(_plugin_groups platforms styles imageformats iconengines)
    foreach(_group IN LISTS _plugin_groups)
        set(_src_dir "${_qt_plugins}/${_group}")
        if(EXISTS "${_src_dir}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target}>/${_group}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_src_dir}"
                    "$<TARGET_FILE_DIR:${target}>/${_group}"
            )
        endif()
    endforeach()
endfunction()
