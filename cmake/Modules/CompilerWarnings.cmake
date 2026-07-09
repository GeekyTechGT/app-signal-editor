# CompilerWarnings.cmake - Warning flags applied to project targets

function(myprj_set_warnings target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
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
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    endif()
endfunction()
