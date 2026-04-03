add_library(myprj_proro_core ${MYPRJ_LIBRARY_TYPE}
    # --- Domain ---
    core/domain/my_entity.cpp
    # --- Use Cases ---
    core/usecases/my_usecase.cpp
    # --- Adapters ---
    adapters/cli/cli_adapter.cpp
    adapters/filesystem/fs_adapter.cpp
    adapters/json/json_adapter.cpp
    # --- API ---
    api/proro_api.cpp
)

target_include_directories(myprj_proro_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(myprj_proro_core
    PUBLIC
        myprj_common
        nlohmann_json::nlohmann_json
    # PRIVATE
    #     OpenSSL::Crypto  # uncomment if this module uses crypto
)

myprj_configure_target(myprj_proro_core)

# Optional: generate a version header for this module
configure_file(
    ${CMAKE_SOURCE_DIR}/include/myprj/proro_version.h.in
    ${CMAKE_BINARY_DIR}/generated/myprj/proro_version.h
    @ONLY
)
