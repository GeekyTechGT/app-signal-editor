# IPO.cmake - interprocedural optimization (LTO), gated by SIGNAL_EDITOR_ENABLE_IPO
#
# Off by default: LTO meaningfully increases GCC/MinGW link times on this
# project's Qt targets, so it is opt-in rather than automatic.

option(SIGNAL_EDITOR_ENABLE_IPO "Enable interprocedural optimization (LTO) in Release builds" OFF)

if(SIGNAL_EDITOR_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT SIGNAL_EDITOR_IPO_SUPPORTED OUTPUT SIGNAL_EDITOR_IPO_ERROR)
    if(NOT SIGNAL_EDITOR_IPO_SUPPORTED)
        message(WARNING "SIGNAL_EDITOR_ENABLE_IPO is ON but IPO/LTO is not supported: ${SIGNAL_EDITOR_IPO_ERROR}")
    endif()
endif()

function(myprj_enable_ipo target)
    if(SIGNAL_EDITOR_ENABLE_IPO AND SIGNAL_EDITOR_IPO_SUPPORTED AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()
