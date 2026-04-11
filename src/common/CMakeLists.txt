add_library(signal_editor_common ${SIGNAL_EDITOR_LIBRARY_TYPE}
    core/domain/common_types.cpp
    # adapters/filesystem/file_utils.cpp
)

target_include_directories(signal_editor_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(signal_editor_common
    PUBLIC
        nlohmann_json::nlohmann_json
)

myprj_configure_target(signal_editor_common)
