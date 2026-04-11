add_executable(test_signal_editor
    test_signal.cpp
    test_signal_library.cpp
    test_signal_editor_service.cpp
    test_csv_repository.cpp
    test_signal_file_repository.cpp
)

target_link_libraries(test_signal_editor
    PRIVATE
        myprj_signal_editor_core
        GTest::gtest_main
)

target_include_directories(test_signal_editor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

myprj_configure_target(test_signal_editor)

gtest_discover_tests(test_signal_editor
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_DIR        ${CMAKE_BINARY_DIR}
    XML_OUTPUT_DIR    ${CMAKE_BINARY_DIR}
    PROPERTIES        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin"
)
