add_executable(myprj_proro_cli
    main.cpp
)

target_link_libraries(myprj_proro_cli
    PRIVATE
        myprj_proro_core
)

set_target_properties(myprj_proro_cli PROPERTIES
    OUTPUT_NAME "proro"
)

myprj_configure_target(myprj_proro_cli)
