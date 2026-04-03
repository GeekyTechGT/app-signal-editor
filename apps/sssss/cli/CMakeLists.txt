add_executable(myprj_sssss_cli
    main.cpp
)

target_link_libraries(myprj_sssss_cli
    PRIVATE
        myprj_sssss_core
)

set_target_properties(myprj_sssss_cli PROPERTIES
    OUTPUT_NAME "sssss"
)

myprj_configure_target(myprj_sssss_cli)
