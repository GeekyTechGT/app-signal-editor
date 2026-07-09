# FindModules

Reserved location for custom `Find<Package>.cmake` modules.

This project currently has no dependency that needs a hand-written Find
module: Qt6 is located through its own CMake package config
(`find_package(Qt6 ...)`), and the remaining pre-built dependencies
(`lib-qt-utils`, `lib-qt-custom-widgets`) are resolved directly in the root
`CMakeLists.txt` from `shared_lib_base` / `preset_to_shared_lib_variant` in
`project.json`.

Add a `Find<Package>.cmake` here only when a future dependency ships neither
a CMake package config nor a `pkg-config` file, and append this directory to
`CMAKE_MODULE_PATH` at that point.
