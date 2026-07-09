# Coverage.cmake - gcov/lcov instrumentation for debug builds, gated by SIGNAL_EDITOR_ENABLE_COVERAGE

function(myprj_enable_coverage target)
    if(SIGNAL_EDITOR_ENABLE_COVERAGE
       AND CMAKE_BUILD_TYPE STREQUAL "Debug"
       AND NOT WIN32
       AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            --coverage
            -O0
            -g
        )
        target_link_options(${target} PRIVATE
            --coverage
        )
    endif()
endfunction()
