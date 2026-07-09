# Sanitizers.cmake - ASan/UBSan for debug builds, gated by SIGNAL_EDITOR_ENABLE_SANITIZERS

function(myprj_enable_sanitizers target)
    if(SIGNAL_EDITOR_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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
