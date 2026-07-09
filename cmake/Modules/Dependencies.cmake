# Dependencies.cmake (formerly cmake/ExternalLibraries.cmake)
# Helpers to import pre-built external shared libraries from a local directory.
#
# Expected layout for each external library:
#   external/<lib_name>/
#     include/          (optional headers)
#     deploy/
#       Debug/          (debug DLL + import lib)
#       Release/        (release DLL + import lib)
#
# Usage example:
#   include(cmake/Modules/Dependencies.cmake)
#   myprj_collect_external_libraries("${CMAKE_SOURCE_DIR}/external"
#       EXT_TARGETS EXT_DEBUG_DLLS EXT_RELEASE_DLLS)
#   target_link_libraries(my_target PRIVATE ${EXT_TARGETS})

function(myprj_select_primary_dll ext_name dlls out_var)
    set(selected "")
    if(dlls)
        list(LENGTH dlls dll_count)
        if(dll_count EQUAL 1)
            list(GET dlls 0 selected)
        else()
            string(TOLOWER "${ext_name}" ext_name_lower)
            foreach(dll ${dlls})
                get_filename_component(dll_name "${dll}" NAME_WE)
                string(TOLOWER "${dll_name}" dll_name_lower)
                if(dll_name_lower MATCHES "${ext_name_lower}")
                    set(selected "${dll}")
                    break()
                endif()
            endforeach()
            if(selected STREQUAL "")
                list(GET dlls 0 selected)
            endif()
        endif()
    endif()
    set(${out_var} "${selected}" PARENT_SCOPE)
endfunction()

function(myprj_select_implib primary_dll implib_candidates out_var)
    set(selected "")
    if(primary_dll AND implib_candidates)
        get_filename_component(dll_name "${primary_dll}" NAME_WE)
        foreach(implib ${implib_candidates})
            get_filename_component(implib_name "${implib}" NAME_WE)
            if(implib_name STREQUAL dll_name)
                set(selected "${implib}")
                break()
            endif()
        endforeach()
    endif()
    if(selected STREQUAL "" AND implib_candidates)
        list(GET implib_candidates 0 selected)
    endif()
    set(${out_var} "${selected}" PARENT_SCOPE)
endfunction()

function(myprj_collect_external_libraries external_root out_targets out_debug_dlls out_release_dlls)
    set(targets)
    set(debug_dlls)
    set(release_dlls)

    if(EXISTS "${external_root}")
        file(GLOB ext_dirs LIST_DIRECTORIES true "${external_root}/*")
        foreach(ext_dir ${ext_dirs})
            if(NOT IS_DIRECTORY "${ext_dir}")
                continue()
            endif()

            get_filename_component(ext_name "${ext_dir}" NAME)
            set(target "External::${ext_name}")

            if(NOT TARGET ${target})
                add_library(${target} SHARED IMPORTED)
            endif()

            if(EXISTS "${ext_dir}/include")
                set_target_properties(${target} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${ext_dir}/include"
                )
            endif()

            set(debug_root "")
            set(release_root "")
            if(EXISTS "${ext_dir}/deploy/Debug")
                set(debug_root "${ext_dir}/deploy/Debug")
            elseif(EXISTS "${ext_dir}/deploy/debug")
                set(debug_root "${ext_dir}/deploy/debug")
            endif()
            if(EXISTS "${ext_dir}/deploy/Release")
                set(release_root "${ext_dir}/deploy/Release")
            elseif(EXISTS "${ext_dir}/deploy/release")
                set(release_root "${ext_dir}/deploy/release")
            endif()

            set(debug_dll_candidates)
            set(release_dll_candidates)
            if(debug_root)
                file(GLOB debug_dll_candidates "${debug_root}/*.dll")
            endif()
            if(release_root)
                file(GLOB release_dll_candidates "${release_root}/*.dll")
            endif()

            myprj_select_primary_dll("${ext_name}" "${debug_dll_candidates}"   debug_dll)
            myprj_select_primary_dll("${ext_name}" "${release_dll_candidates}" release_dll)

            if(debug_dll)
                set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_DEBUG "${debug_dll}")
                list(APPEND debug_dlls ${debug_dll_candidates})
            endif()
            if(release_dll)
                set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_RELEASE "${release_dll}")
                list(APPEND release_dlls ${release_dll_candidates})
            endif()
            if(debug_dll AND NOT release_dll)
                set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_RELEASE "${debug_dll}")
                list(APPEND release_dlls ${debug_dll_candidates})
            elseif(release_dll AND NOT debug_dll)
                set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_DEBUG "${release_dll}")
                list(APPEND debug_dlls ${release_dll_candidates})
            endif()

            set(debug_implib_candidates)
            set(release_implib_candidates)
            file(GLOB debug_implib_candidates
                "${ext_dir}/deploy/Debug/*.lib"   "${ext_dir}/deploy/Debug/*.dll.a"
                "${ext_dir}/deploy/debug/*.lib"   "${ext_dir}/deploy/debug/*.dll.a"
            )
            file(GLOB release_implib_candidates
                "${ext_dir}/deploy/Release/*.lib" "${ext_dir}/deploy/Release/*.dll.a"
                "${ext_dir}/deploy/release/*.lib" "${ext_dir}/deploy/release/*.dll.a"
            )

            myprj_select_implib("${debug_dll}"   "${debug_implib_candidates}"   debug_implib)
            myprj_select_implib("${release_dll}" "${release_implib_candidates}" release_implib)

            if(debug_implib)
                set_target_properties(${target} PROPERTIES IMPORTED_IMPLIB_DEBUG "${debug_implib}")
            endif()
            if(release_implib)
                set_target_properties(${target} PROPERTIES IMPORTED_IMPLIB_RELEASE "${release_implib}")
            endif()
            if(debug_implib AND NOT release_implib)
                set_target_properties(${target} PROPERTIES IMPORTED_IMPLIB_RELEASE "${debug_implib}")
            elseif(release_implib AND NOT debug_implib)
                set_target_properties(${target} PROPERTIES IMPORTED_IMPLIB_DEBUG "${release_implib}")
            endif()

            if((debug_dll AND debug_implib) OR (release_dll AND release_implib))
                list(APPEND targets ${target})
            else()
                if(TARGET ${target})
                    set_property(TARGET ${target} PROPERTY EXCLUDE_FROM_ALL TRUE)
                endif()
                message(WARNING "External library disabled due to missing DLL/implib: ${ext_name}")
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES debug_dlls)
    list(REMOVE_DUPLICATES release_dlls)

    set(${out_targets}      ${targets}      PARENT_SCOPE)
    set(${out_debug_dlls}   ${debug_dlls}   PARENT_SCOPE)
    set(${out_release_dlls} ${release_dlls} PARENT_SCOPE)
endfunction()
