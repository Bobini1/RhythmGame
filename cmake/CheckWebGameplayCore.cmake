if (NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif ()
if (NOT DEFINED SOURCE_MANIFEST)
    message(FATAL_ERROR "SOURCE_MANIFEST is required")
endif ()
if (NOT EXISTS "${SOURCE_MANIFEST}")
    message(FATAL_ERROR
            "Portable gameplay source manifest is missing: ${SOURCE_MANIFEST}")
endif ()

file(STRINGS "${SOURCE_MANIFEST}" core_sources)
list(FILTER core_sources EXCLUDE REGEX "^[ \t]*$")
list(LENGTH core_sources source_count)
if (source_count LESS 2)
    message(FATAL_ERROR
            "Portable gameplay source closure is unexpectedly empty")
endif ()

set(unique_sources ${core_sources})
list(REMOVE_DUPLICATES unique_sources)
list(LENGTH unique_sources unique_source_count)
if (NOT source_count EQUAL unique_source_count)
    message(FATAL_ERROR
            "Portable gameplay source closure contains duplicate entries")
endif ()

set(required_sources
        src/gameplay_logic/SinglePlayerGameplayCore.cpp
        src/gameplay_logic/GameplayTrace.cpp
        src/gameplay_logic/SinglePlayerChartBuilder.cpp
        src/gameplay_logic/BmsGameReferee.cpp
        src/gameplay_logic/BmsLiveScore.cpp
        src/resource_managers/ChartDataFactory.cpp
        src/charts/ReadBmsFile.cpp
)
foreach (required_source IN LISTS required_sources)
    if (NOT required_source IN_LIST core_sources)
        message(FATAL_ERROR
                "Portable gameplay source closure omits ${required_source}")
    endif ()
endforeach ()

set(forbidden_source_pattern
        "sqlite|vars|audioengine|miniaudio|profile|chartrunner|bga|llfio|wil"
)
foreach (relative_source IN LISTS core_sources)
    cmake_path(NORMAL_PATH relative_source)
    if (IS_ABSOLUTE "${relative_source}" OR
            NOT relative_source MATCHES "^src/")
        message(FATAL_ERROR
                "Portable gameplay source must be repo-relative under src/: "
                "${relative_source}")
    endif ()
    string(TOLOWER "${relative_source}" normalized_source)
    if (normalized_source MATCHES "${forbidden_source_pattern}")
        message(FATAL_ERROR
                "Portable gameplay source closure contains forbidden source: "
                "${relative_source}")
    endif ()
    if (NOT EXISTS "${SOURCE_ROOT}/${relative_source}")
        message(FATAL_ERROR
                "Portable gameplay source does not exist: ${relative_source}")
    endif ()
endforeach ()

file(READ
        "${SOURCE_ROOT}/src/resource_managers/ChartDataFactory.cpp"
        chart_data_factory)
if (NOT chart_data_factory MATCHES
        "defined\\(__EMSCRIPTEN__\\)[ \t]*\\|\\|[ \t]*defined\\(RHYTHMGAME_PORTABLE_GAMEPLAY\\)")
    message(FATAL_ERROR
            "ChartDataFactory must select its QFile seam for the portable "
            "target while preserving native mapped-file implementations")
endif ()

message(STATUS
        "Portable gameplay source closure passed (${source_count} files)")
