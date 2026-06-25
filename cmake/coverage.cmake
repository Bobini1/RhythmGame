
include("${CMAKE_CURRENT_LIST_DIR}/instrumentation-targets.cmake")

function(rhythmgame_enable_coverage target)
    if (NOT TARGET ${target})
        return()
    endif ()

    target_compile_options(${target} PRIVATE -Og -g --coverage -fkeep-static-functions)

    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "EXECUTABLE"
            OR target_type STREQUAL "SHARED_LIBRARY"
            OR target_type STREQUAL "MODULE_LIBRARY")
        target_link_options(${target} PRIVATE --coverage)
    endif ()
endfunction()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach (target IN LISTS RHYTHMGAME_INSTRUMENTED_TARGETS)
        rhythmgame_enable_coverage(${target})
    endforeach ()
else ()
    message(WARNING "Coverage instrumentation is only configured for GCC/Clang")
endif ()

add_custom_target(
        coverage
        COMMAND lcov -c -q -o "${PROJECT_BINARY_DIR}/coverage.info" -d "${PROJECT_BINARY_DIR}" --include "${PROJECT_SOURCE_DIR}/*" && genhtml --legend -f -q "${PROJECT_BINARY_DIR}/coverage.info" -p "${PROJECT_SOURCE_DIR}" -o "${PROJECT_BINARY_DIR}/coverage_html"
        COMMENT "Generating coverage report"
        VERBATIM
)
