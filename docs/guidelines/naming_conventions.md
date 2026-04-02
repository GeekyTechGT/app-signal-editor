# Naming Conventions

## C++ Naming

| Construct               | Convention              | Example                          |
|-------------------------|-------------------------|----------------------------------|
| Namespace               | `snake_case`            | `myprj::my_module`               |
| Class / Struct          | `PascalCase`            | `MyEntity`, `FsAdapter`          |
| Interface (port)        | `IPascalCase`           | `IMyRepository`                  |
| Function / Method       | `snake_case`            | `parse_args()`, `load_all()`     |
| Variable / Parameter    | `snake_case`            | `data_dir`, `entity_id`          |
| Constant / Enum value   | `UPPER_SNAKE_CASE`      | `Status::Ok`, `MAX_RETRY`        |
| Template parameter      | `PascalCase`            | `T`, `ValueType`                 |
| Private member          | `snake_case_` (trailing `_`) | `data_dir_`, `repository_`  |
| Header guard / macro    | `MYPRJ_MODULE_FILE_H`   | `MYPRJ_MY_MODULE_ENTITY_H`       |

## File Naming

| Artifact                | Convention              | Example                          |
|-------------------------|-------------------------|----------------------------------|
| Header file             | `snake_case.h`          | `my_entity.h`                    |
| Implementation file     | `snake_case.cpp`        | `my_entity.cpp`                  |
| Test file               | `test_snake_case.cpp`   | `test_my_entity.cpp`             |
| CMakeLists              | `CMakeLists.txt`        | (fixed)                          |
| JSON schema             | `snake_case.schema.json`| `my_module_config.schema.json`   |

## CMake Naming

| Construct               | Convention              | Example                            |
|-------------------------|-------------------------|------------------------------------|
| Target (library)        | `myprj_<module>_core`   | `myprj_my_module_core`             |
| Target (executable)     | `myprj_<module>_cli`    | `myprj_my_module_cli`              |
| CMake option            | `MYPRJ_<OPTION>`        | `MYPRJ_BUILD_TESTS`                |
| CMake function          | `myprj_<verb>_<noun>`   | `myprj_configure_target`           |

## Directory Naming

Directories always use `snake_case`. No hyphens in directory names under `src/` or `apps/`.
