include_guard(GLOBAL)

get_filename_component(
        RHYTHMGAME_WEB_AUDIO_CORE_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/.."
        ABSOLUTE
)

set(RHYTHMGAME_WEB_AUDIO_CORE_RELATIVE_SOURCES
        src/resource_managers/BmsAssetResolver.cpp
        src/resource_managers/BmsAssetResolver.h
        src/sounds/Sound.h
        src/web_playtest/audio/AudioCommand.h
        src/web_playtest/audio/PcmSoundBank.cpp
        src/web_playtest/audio/PcmSoundBank.h
        src/web_playtest/audio/RealtimeMixer.cpp
        src/web_playtest/audio/RealtimeMixer.h
        src/web_playtest/audio/ScheduledPcmSound.cpp
        src/web_playtest/audio/ScheduledPcmSound.h
        src/web_playtest/audio/SpscRing.h
)

set(RHYTHMGAME_WEB_AUDIO_CORE_SOURCES)
foreach (relative_source IN LISTS RHYTHMGAME_WEB_AUDIO_CORE_RELATIVE_SOURCES)
    list(APPEND RHYTHMGAME_WEB_AUDIO_CORE_SOURCES
            "${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/${relative_source}")
endforeach ()

function(rhythmgame_add_web_audio_core target)
    if (TARGET "${target}")
        return()
    endif ()
    add_library("${target}" STATIC ${RHYTHMGAME_WEB_AUDIO_CORE_SOURCES})
    target_compile_features("${target}" PUBLIC cxx_std_23)
    target_compile_definitions("${target}" PUBLIC NOMINMAX)
    target_include_directories("${target}" PUBLIC
            "$<BUILD_INTERFACE:${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src>")
    target_link_libraries("${target}" PUBLIC Qt6::Core)

    get_target_property(audio_links "${target}" LINK_LIBRARIES)
    string(TOLOWER "${audio_links}" normalized_links)
    if (normalized_links MATCHES
            "audioengine|miniaudio|sndfile|sqlite|multimedia|rhythmgame_lib")
        message(FATAL_ERROR
                "${target} acquired a forbidden native dependency: ${audio_links}")
    endif ()

    set(source_manifest
            "${CMAKE_CURRENT_BINARY_DIR}/${target}-sources.txt")
    string(REPLACE ";" "\n" manifest_contents
            "${RHYTHMGAME_WEB_AUDIO_CORE_RELATIVE_SOURCES}")
    file(GENERATE OUTPUT "${source_manifest}"
            CONTENT "${manifest_contents}\n")
    add_custom_target("${target}_source_contract"
            COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ROOT=${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}"
            "-DSOURCE_MANIFEST=${source_manifest}"
            -P "${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/cmake/CheckWebAudioCore.cmake"
            VERBATIM)
    add_dependencies("${target}" "${target}_source_contract")
endfunction()
