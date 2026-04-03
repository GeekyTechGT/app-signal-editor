add_executable(test_proro
    test_my_entity.cpp
    test_my_usecase.cpp
)

target_link_libraries(test_proro
    PRIVATE
        myprj_proro_core
        GTest::gtest_main
)

target_include_directories(test_proro
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

myprj_configure_target(test_proro)

gtest_discover_tests(test_proro
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_DIR        ${CMAKE_BINARY_DIR}
    XML_OUTPUT_DIR    ${CMAKE_BINARY_DIR}
    PROPERTIES        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin"
)
