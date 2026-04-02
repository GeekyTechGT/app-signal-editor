add_executable(test_my_module
    test_my_entity.cpp
    test_my_usecase.cpp
)

target_link_libraries(test_my_module
    PRIVATE
        myprj_my_module_core
        GTest::gtest_main
)

target_include_directories(test_my_module
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

myprj_configure_target(test_my_module)

gtest_discover_tests(test_my_module
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_DIR        ${CMAKE_BINARY_DIR}
    XML_OUTPUT_DIR    ${CMAKE_BINARY_DIR}
    PROPERTIES        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin"
)
