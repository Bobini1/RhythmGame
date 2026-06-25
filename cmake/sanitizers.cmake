include("${CMAKE_CURRENT_LIST_DIR}/instrumentation-targets.cmake")

set(RHYTHMGAME_SANITIZER_COMPILE_OPTIONS
        -U_FORTIFY_SOURCE
        -O2
        -g
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-common
)
set(RHYTHMGAME_SANITIZER_LINK_OPTIONS
        -fsanitize=address,undefined
)

function(rhythmgame_enable_sanitizers target)
    if (NOT TARGET ${target})
        return()
    endif ()

    target_compile_options(${target} PRIVATE ${RHYTHMGAME_SANITIZER_COMPILE_OPTIONS})

    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "EXECUTABLE"
            OR target_type STREQUAL "SHARED_LIBRARY"
            OR target_type STREQUAL "MODULE_LIBRARY")
        target_link_options(${target} PRIVATE ${RHYTHMGAME_SANITIZER_LINK_OPTIONS})
    endif ()
endfunction()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach (target IN LISTS RHYTHMGAME_INSTRUMENTED_TARGETS)
        rhythmgame_enable_sanitizers(${target})
    endforeach ()
else ()
    message(WARNING "Sanitizer instrumentation is only configured for GCC/Clang")
endif ()
