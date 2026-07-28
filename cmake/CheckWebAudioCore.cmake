if (NOT DEFINED SOURCE_ROOT OR NOT DEFINED SOURCE_MANIFEST)
    message(FATAL_ERROR "Web audio source contract requires root and manifest")
endif ()

file(STRINGS "${SOURCE_MANIFEST}" sources)
set(forbidden_pattern
        "AudioEngine|Miniaudio|miniaudio|sndfile|SQLite|QMutex|QFile|spdlog")
foreach (relative_source IN LISTS sources)
    if (relative_source MATCHES "\\.(cpp|h)$")
        file(READ "${SOURCE_ROOT}/${relative_source}" contents)
        if (contents MATCHES "${forbidden_pattern}")
            message(FATAL_ERROR
                    "Portable web audio source ${relative_source} contains "
                    "a forbidden dependency")
        endif ()
    endif ()
endforeach ()

file(READ "${SOURCE_ROOT}/src/web_playtest/audio/RealtimeMixer.cpp"
        realtime_mixer)
if (realtime_mixer MATCHES
        "make_unique|make_shared|push_back|emplace_back|reserve\\(|resize\\(|new[ \t]|mutex|lock_guard|unique_lock|condition_variable|wait\\(|spdlog|filesystem|QFile|QtConcurrent")
    message(FATAL_ERROR
            "RealtimeMixer render translation unit contains a forbidden "
            "realtime operation")
endif ()
list(LENGTH sources source_count)
message(STATUS "Portable web audio source closure passed "
        "(${source_count} files)")
