add_library(myprj_common ${MYPRJ_LIBRARY_TYPE}
    core/domain/common_types.cpp
    # adapters/filesystem/file_utils.cpp
)

target_include_directories(myprj_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(myprj_common
    PUBLIC
        nlohmann_json::nlohmann_json
)

myprj_configure_target(myprj_common)
