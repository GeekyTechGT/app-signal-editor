add_executable(test_sssss
    test_my_entity.cpp
    test_my_usecase.cpp
)

target_link_libraries(test_sssss
    PRIVATE
        myprj_sssss_core
        GTest::gtest_main
)

target_include_directories(test_sssss
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

myprj_configure_target(test_sssss)

gtest_discover_tests(test_sssss
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_DIR        ${CMAKE_BINARY_DIR}
    XML_OUTPUT_DIR    ${CMAKE_BINARY_DIR}
    PROPERTIES        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin"
)
