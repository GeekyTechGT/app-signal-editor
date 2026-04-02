add_executable(myprj_my_module_cli
    main.cpp
)

target_link_libraries(myprj_my_module_cli
    PRIVATE
        myprj_my_module_core
)

set_target_properties(myprj_my_module_cli PROPERTIES
    OUTPUT_NAME "my_module"
)

myprj_configure_target(myprj_my_module_cli)
