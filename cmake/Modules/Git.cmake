# Git.cmake - best-effort current commit hash, exposed as SIGNAL_EDITOR_GIT_COMMIT_HASH
#
# Non-fatal if git is unavailable or the source tree isn't a git checkout
# (e.g. an extracted source archive).

set(SIGNAL_EDITOR_GIT_COMMIT_HASH "unknown")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE _SIGNAL_EDITOR_GIT_HASH_OUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _SIGNAL_EDITOR_GIT_HASH_RESULT
    )
    if(_SIGNAL_EDITOR_GIT_HASH_RESULT EQUAL 0 AND _SIGNAL_EDITOR_GIT_HASH_OUT)
        set(SIGNAL_EDITOR_GIT_COMMIT_HASH "${_SIGNAL_EDITOR_GIT_HASH_OUT}")
    endif()
endif()
