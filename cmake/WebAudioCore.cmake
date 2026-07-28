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

set(RHYTHMGAME_WEB_AUDIO_CORE_FORBIDDEN_LINK_PATTERN
        "sqlite|vars|miniaudio|audioengine|profile|chartrunner|bga|multimedia|llfio|wil|rhythmgame_lib|sndfile"
)

option(
        RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT
        "Require Ninja compiler-dependency auditing for the portable audio target"
        OFF
)

function(rhythmgame_web_audio_core_extract_target_candidates
        link_item output_variable)
    set(target_candidates)
    if (TARGET "${link_item}")
        list(APPEND target_candidates "${link_item}")
    endif ()

    string(REGEX MATCHALL
            "[-A-Za-z0-9_.+]+(::[-A-Za-z0-9_.+]+)*"
            link_tokens
            "${link_item}")
    foreach (link_token IN LISTS link_tokens)
        if (TARGET "${link_token}")
            list(APPEND target_candidates "${link_token}")
        endif ()
    endforeach ()
    list(REMOVE_DUPLICATES target_candidates)
    set("${output_variable}" "${target_candidates}" PARENT_SCOPE)
endfunction()

function(rhythmgame_web_audio_core_get_target_property
        target property output_variable)
    get_target_property(property_value "${target}" "${property}")
    if (property_value MATCHES "-NOTFOUND$")
        set(property_value)
    endif ()
    set("${output_variable}" "${property_value}" PARENT_SCOPE)
endfunction()

function(rhythmgame_web_audio_core_check_link_closure target)
    set(pending_targets ${target})
    set(visited_targets)

    while (pending_targets)
        list(POP_FRONT pending_targets current_target)
        if (current_target IN_LIST visited_targets OR
                NOT TARGET "${current_target}")
            continue()
        endif ()
        list(APPEND visited_targets "${current_target}")

        foreach (property
                LINK_LIBRARIES
                INTERFACE_LINK_LIBRARIES
                INTERFACE_LINK_LIBRARIES_DIRECT
                INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE)
            rhythmgame_web_audio_core_get_target_property(
                    "${current_target}" "${property}" links)
            if (NOT links)
                continue()
            endif ()
            foreach (link IN LISTS links)
                string(TOLOWER "${link}" normalized_link)
                if (normalized_link MATCHES
                        "${RHYTHMGAME_WEB_AUDIO_CORE_FORBIDDEN_LINK_PATTERN}")
                    message(FATAL_ERROR
                            "${target} has forbidden ${property} dependency "
                            "${current_target} -> ${link}")
                endif ()
                rhythmgame_web_audio_core_extract_target_candidates(
                        "${link}" link_target_candidates)
                list(APPEND pending_targets ${link_target_candidates})
            endforeach ()
        endforeach ()
    endwhile ()

    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_WEB_AUDIO_VISITED_LINK_TARGETS
            "${visited_targets}")
endfunction()

function(rhythmgame_verify_web_audio_core_target target)
    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Portable audio target does not exist: ${target}")
    endif ()

    get_target_property(target_type "${target}" TYPE)
    if (NOT target_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
                "${target} must be a static library, got ${target_type}")
    endif ()

    rhythmgame_web_audio_core_get_target_property(
            "${target}" SOURCES actual_sources)
    set(expected_sources ${RHYTHMGAME_WEB_AUDIO_CORE_SOURCES})
    list(SORT actual_sources)
    list(SORT expected_sources)
    if (NOT actual_sources STREQUAL expected_sources)
        message(FATAL_ERROR
                "${target} source closure differs from WebAudioCore.cmake")
    endif ()

    rhythmgame_web_audio_core_get_target_property(
            "${target}" COMPILE_DEFINITIONS actual_compile_definitions)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_COMPILE_DEFINITIONS
            actual_interface_compile_definitions)
    set(expected_compile_definitions NOMINMAX)
    list(SORT actual_compile_definitions)
    list(SORT actual_interface_compile_definitions)
    list(SORT expected_compile_definitions)
    if (NOT actual_compile_definitions STREQUAL
            expected_compile_definitions OR
            NOT actual_interface_compile_definitions STREQUAL
            expected_compile_definitions)
        message(FATAL_ERROR
                "${target} must define and export exactly NOMINMAX")
    endif ()

    rhythmgame_web_audio_core_get_target_property(
            "${target}" COMPILE_FEATURES actual_compile_features)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_COMPILE_FEATURES
            actual_interface_compile_features)
    set(expected_compile_features cxx_std_23)
    if (NOT actual_compile_features STREQUAL expected_compile_features OR
            NOT actual_interface_compile_features STREQUAL
            expected_compile_features)
        message(FATAL_ERROR
                "${target} must require and export exactly cxx_std_23")
    endif ()

    rhythmgame_web_audio_core_get_target_property(
            "${target}" INCLUDE_DIRECTORIES actual_include_directories)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_INCLUDE_DIRECTORIES
            actual_interface_include_directories)
    set(expected_include_directories
            "$<BUILD_INTERFACE:${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src>")
    if (NOT actual_include_directories STREQUAL
            expected_include_directories OR
            NOT actual_interface_include_directories STREQUAL
            expected_include_directories)
        message(FATAL_ERROR
                "${target} must use and export exactly the portable src "
                "BUILD_INTERFACE include")
    endif ()

    rhythmgame_web_audio_core_get_target_property(
            "${target}" LINK_LIBRARIES actual_links)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_LINK_LIBRARIES actual_interface_links)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_LINK_LIBRARIES_DIRECT actual_direct_links)
    rhythmgame_web_audio_core_get_target_property(
            "${target}" INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE
            actual_direct_link_excludes)
    set(expected_links Qt6::Core)
    list(SORT actual_links)
    list(SORT actual_interface_links)
    list(SORT expected_links)
    if (NOT actual_links STREQUAL expected_links OR
            NOT actual_interface_links STREQUAL expected_links OR
            actual_direct_links OR actual_direct_link_excludes)
        message(FATAL_ERROR
                "${target} own link properties differ from WebAudioCore.cmake: "
                "LINK_LIBRARIES=${actual_links}; "
                "INTERFACE_LINK_LIBRARIES=${actual_interface_links}; "
                "INTERFACE_LINK_LIBRARIES_DIRECT=${actual_direct_links}; "
                "INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE="
                "${actual_direct_link_excludes}")
    endif ()

    set(expected_options)
    if (EMSCRIPTEN)
        set(expected_options -pthread)
    endif ()
    foreach (property
            COMPILE_OPTIONS
            INTERFACE_COMPILE_OPTIONS
            LINK_OPTIONS
            INTERFACE_LINK_OPTIONS)
        rhythmgame_web_audio_core_get_target_property(
                "${target}" "${property}" actual_options)
        if (NOT "${actual_options}" STREQUAL "${expected_options}")
            message(FATAL_ERROR
                    "${target} must own exact ${property}="
                    "${expected_options}, got ${actual_options}")
        endif ()
    endforeach ()

    rhythmgame_web_audio_core_check_link_closure("${target}")

    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_PORTABLE_SOURCE_CLOSURE
            "${RHYTHMGAME_WEB_AUDIO_CORE_RELATIVE_SOURCES}")
    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_PORTABLE_LINK_CLOSURE
            "${actual_links}")
endfunction()

function(rhythmgame_add_web_audio_core target)
    if (TARGET "${target}")
        message(FATAL_ERROR
                "${target} already exists; WebAudioCore.cmake must create and "
                "own the portable audio target")
    endif ()

    add_library("${target}" STATIC ${RHYTHMGAME_WEB_AUDIO_CORE_SOURCES})
    target_compile_features("${target}" PUBLIC cxx_std_23)
    target_compile_definitions("${target}" PUBLIC NOMINMAX)
    target_include_directories("${target}" PUBLIC
            "$<BUILD_INTERFACE:${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src>")
    target_link_libraries("${target}" PUBLIC Qt6::Core)
    if (EMSCRIPTEN)
        target_compile_options("${target}" PUBLIC -pthread)
        target_link_options("${target}" PUBLIC -pthread)
    endif ()

    rhythmgame_verify_web_audio_core_target("${target}")
    cmake_language(EVAL CODE
            "cmake_language(DEFER CALL rhythmgame_verify_web_audio_core_target [[${target}]])")

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

    if (RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT)
        if (NOT CMAKE_GENERATOR MATCHES "^Ninja")
            message(FATAL_ERROR
                    "RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT "
                    "requires a Ninja generator; ${CMAKE_GENERATOR} does not "
                    "expose compiler-ingested dependencies through "
                    "'ninja -t deps'")
        endif ()
        set(dependency_contract_script
                "${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/cmake/CheckWebAudioCoreDependencies.cmake")
        set(dependency_contract_stamp
                "${CMAKE_CURRENT_BINARY_DIR}/${target}-dependency-contract.stamp")
        set(dependency_contract_anchor_source
                "${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src/web_playtest/audio/RealtimeMixer.cpp")
        set(dependency_contract_objdump "${CMAKE_OBJDUMP}")
        if (EMSCRIPTEN AND
                (NOT EXISTS "${dependency_contract_objdump}"))
            get_filename_component(
                    emscripten_driver_directory
                    "${CMAKE_CXX_COMPILER}"
                    DIRECTORY)
            find_program(
                    dependency_contract_objdump
                    NAMES llvm-objdump llvm-objdump.exe
                    HINTS "${emscripten_driver_directory}/../bin"
                    NO_DEFAULT_PATH)
            if (NOT dependency_contract_objdump)
                message(FATAL_ERROR
                        "Could not find pinned llvm-objdump next to "
                        "${CMAKE_CXX_COMPILER}")
            endif ()
        endif ()
        set_property(SOURCE "${dependency_contract_anchor_source}"
                APPEND PROPERTY OBJECT_DEPENDS
                "${dependency_contract_script}")
        add_custom_command(TARGET "${target}" POST_BUILD
                BYPRODUCTS "${dependency_contract_stamp}"
                COMMAND ${CMAKE_COMMAND} -E rm -f
                "${dependency_contract_stamp}"
                COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_ROOT=${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}"
                "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
                "-DNINJA_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                "-DTARGET_NAME=${target}"
                "-DREQUIRE_PTHREAD=${EMSCRIPTEN}"
                "-DOBJDUMP_PROGRAM=${dependency_contract_objdump}"
                -P
                "${dependency_contract_script}"
                COMMAND ${CMAKE_COMMAND} -E touch
                "${dependency_contract_stamp}"
                VERBATIM
        )
    else ()
        message(STATUS
                "${target}: Ninja compiler-dependency audit is not active; "
                "set RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT=ON "
                "for browser and contract-verification builds")
    endif ()
endfunction()
