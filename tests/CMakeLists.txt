include(GoogleTest)

# =============================================================================
# Unit tests
# =============================================================================
file(GLOB SIGNAL_EDITOR_UNIT_TEST_CMAKELISTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 03.unit_test/*/CMakeLists.txt)
foreach(unit_test_cmakelists ${SIGNAL_EDITOR_UNIT_TEST_CMAKELISTS})
    get_filename_component(unit_test_dir ${unit_test_cmakelists} DIRECTORY)
    add_subdirectory(${unit_test_dir})
endforeach()

# =============================================================================
# E2E tests (optional — add executables to PATH before running)
# =============================================================================
# file(GLOB SIGNAL_EDITOR_E2E_TEST_CMAKELISTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 04.e2e_test/*/CMakeLists.txt)
# foreach(e2e_test_cmakelists ${SIGNAL_EDITOR_E2E_TEST_CMAKELISTS})
#     get_filename_component(e2e_test_dir ${e2e_test_cmakelists} DIRECTORY)
#     add_subdirectory(${e2e_test_dir})
# endforeach()
