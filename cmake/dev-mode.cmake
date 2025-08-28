include(cmake/folders.cmake)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

add_custom_target(
        run-exe
        COMMAND RhythmGame_exe
        VERBATIM
)
add_dependencies(run-exe RhythmGame_exe)

option(BUILD_DOCS "Build documentation using Doxygen" OFF)
if (BUILD_DOCS)
    include(cmake/docs.cmake)
endif ()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if (ENABLE_COVERAGE)
    include(cmake/coverage.cmake)
endif ()

option(ENABLE_INCLUDE_WHAT_YOU_USE "Enable include-what-you-use" OFF)
if (ENABLE_INCLUDE_WHAT_YOU_USE)
    include(cmake/include-what-you-use.cmake)
endif ()

include(cmake/build-dir-setup.cmake)
include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)
include(cmake/translations.cmake)

add_folders(Project)
