# Packaging.cmake - optional CPack wiring, gated by SIGNAL_EDITOR_ENABLE_CPACK
#
# The real Windows installer path is Inno Setup (see scripts/project_manager.py
# "Create Installer" action + cmake/Templates/installer.iss.in). CPack is kept
# here as an inert, opt-in alternative for other packaging formats.

option(SIGNAL_EDITOR_ENABLE_CPACK "Enable CPack packaging targets" OFF)

if(SIGNAL_EDITOR_ENABLE_CPACK)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VENDOR "Signal Editor Team")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
    include(CPack)
endif()
