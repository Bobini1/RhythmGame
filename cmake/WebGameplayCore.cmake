include_guard(GLOBAL)

get_filename_component(
        RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/.."
        ABSOLUTE
)

set(RHYTHMGAME_WEB_GAMEPLAY_CORE_RELATIVE_SOURCES
        src/charts/Base62.h
        src/charts/BmsNotesData.cpp
        src/charts/BmsNotesData.h
        src/charts/ParsedBmsChart.cpp
        src/charts/ParsedBmsChart.h
        src/charts/ReadBmsFile.cpp
        src/charts/ReadBmsFile.h
        src/charts/Snap.cpp
        src/charts/Snap.h
        src/gameplay_logic/BmsGameReferee.cpp
        src/gameplay_logic/BmsGameReferee.h
        src/gameplay_logic/BmsGaugeHistory.cpp
        src/gameplay_logic/BmsGaugeHistory.h
        src/gameplay_logic/BmsLiveScore.cpp
        src/gameplay_logic/BmsLiveScore.h
        src/gameplay_logic/BmsNotes.cpp
        src/gameplay_logic/BmsNotes.h
        src/gameplay_logic/BmsPoints.cpp
        src/gameplay_logic/BmsPoints.h
        src/gameplay_logic/BmsReplayData.cpp
        src/gameplay_logic/BmsReplayData.h
        src/gameplay_logic/BmsResult.cpp
        src/gameplay_logic/BmsResult.h
        src/gameplay_logic/BmsScore.cpp
        src/gameplay_logic/BmsScore.h
        src/gameplay_logic/ChartData.cpp
        src/gameplay_logic/ChartData.h
        src/gameplay_logic/GameplayTrace.cpp
        src/gameplay_logic/GameplayTrace.h
        src/gameplay_logic/HitEvent.cpp
        src/gameplay_logic/HitEvent.h
        src/gameplay_logic/Judgement.cpp
        src/gameplay_logic/Judgement.h
        src/gameplay_logic/NoteState.cpp
        src/gameplay_logic/NoteState.h
        src/gameplay_logic/SinglePlayerChartBuilder.cpp
        src/gameplay_logic/SinglePlayerChartBuilder.h
        src/gameplay_logic/SinglePlayerGameplayCore.cpp
        src/gameplay_logic/SinglePlayerGameplayCore.h
        src/gameplay_logic/rules/BmsGauge.cpp
        src/gameplay_logic/rules/BmsGauge.h
        src/gameplay_logic/rules/BmsRanks.h
        src/gameplay_logic/rules/HitRules.cpp
        src/gameplay_logic/rules/HitRules.h
        src/gameplay_logic/rules/Lr2Gauge.cpp
        src/gameplay_logic/rules/Lr2Gauge.h
        src/gameplay_logic/rules/Lr2HitValues.cpp
        src/gameplay_logic/rules/Lr2HitValues.h
        src/gameplay_logic/rules/Lr2TimingWindows.cpp
        src/gameplay_logic/rules/Lr2TimingWindows.h
        src/gameplay_logic/rules/TimingWindows.h
        src/input/BmsKeys.cpp
        src/input/BmsKeys.h
        src/resource_managers/ChartDataFactory.cpp
        src/resource_managers/ChartDataFactory.h
        src/resource_managers/ChartPlayConfig.h
        src/resource_managers/ChartPlayOptions.h
        src/sounds/Sound.h
        src/support/Compress.h
        src/support/GeneratePermutation.cpp
        src/support/GeneratePermutation.h
        src/support/PathToQString.cpp
        src/support/PathToQString.h
        src/support/PathToUtfString.cpp
        src/support/PathToUtfString.h
        src/support/QStringToPath.cpp
        src/support/QStringToPath.h
        src/support/Sha256.cpp
        src/support/Sha256.h
        src/support/UtfStringToPath.cpp
        src/support/UtfStringToPath.h
        src/support/Version.h
)

set(RHYTHMGAME_WEB_GAMEPLAY_CORE_SOURCES)
foreach (relative_source IN LISTS
        RHYTHMGAME_WEB_GAMEPLAY_CORE_RELATIVE_SOURCES)
    list(APPEND RHYTHMGAME_WEB_GAMEPLAY_CORE_SOURCES
            "${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}/${relative_source}")
endforeach ()

set(RHYTHMGAME_WEB_GAMEPLAY_CORE_FORBIDDEN_LINK_PATTERN
        "sqlite|vars|miniaudio|audioengine|profile|chartrunner|bga|multimedia|llfio|wil|rhythmgame_lib"
)

option(
        RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT
        "Require Ninja compiler-dependency auditing for the portable gameplay target"
        OFF
)

function(rhythmgame_web_gameplay_core_extract_target_candidates
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

function(rhythmgame_web_gameplay_core_check_link_closure target)
    set(pending_targets ${target})
    set(visited_targets)

    while (pending_targets)
        list(POP_FRONT pending_targets current_target)
        if (current_target IN_LIST visited_targets OR
                NOT TARGET "${current_target}")
            continue()
        endif ()
        list(APPEND visited_targets "${current_target}")

        foreach (property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(links "${current_target}" "${property}")
            if (NOT links OR links MATCHES "-NOTFOUND$")
                continue()
            endif ()
            foreach (link IN LISTS links)
                string(TOLOWER "${link}" normalized_link)
                if (normalized_link MATCHES
                        "${RHYTHMGAME_WEB_GAMEPLAY_CORE_FORBIDDEN_LINK_PATTERN}")
                    message(FATAL_ERROR
                            "${target} has forbidden ${property} dependency "
                            "${current_target} -> ${link}")
                endif ()
                rhythmgame_web_gameplay_core_extract_target_candidates(
                        "${link}" link_target_candidates)
                list(APPEND pending_targets ${link_target_candidates})
            endforeach ()
        endforeach ()
    endwhile ()

    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_PORTABLE_VISITED_LINK_TARGETS
            "${visited_targets}")
endfunction()

function(rhythmgame_verify_web_gameplay_core_target target)
    if (NOT TARGET "${target}")
        message(FATAL_ERROR "Portable gameplay target does not exist: ${target}")
    endif ()

    get_target_property(target_type "${target}" TYPE)
    if (NOT target_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR
                "${target} must be a static library, got ${target_type}")
    endif ()

    get_target_property(actual_sources "${target}" SOURCES)
    set(expected_sources ${RHYTHMGAME_WEB_GAMEPLAY_CORE_SOURCES})
    list(SORT actual_sources)
    list(SORT expected_sources)
    if (NOT actual_sources STREQUAL expected_sources)
        message(FATAL_ERROR
                "${target} source closure differs from WebGameplayCore.cmake")
    endif ()

    get_target_property(compile_definitions
            "${target}" COMPILE_DEFINITIONS)
    if (NOT "RHYTHMGAME_PORTABLE_GAMEPLAY" IN_LIST compile_definitions)
        message(FATAL_ERROR
                "${target} must compile through portable platform seams")
    endif ()

    get_target_property(actual_links "${target}" LINK_LIBRARIES)
    set(expected_links
            Qt6::Core
            Qt6::Qml
            Threads::Threads
            Boost::headers
            spdlog::spdlog_header_only
            foonathan::lexy
            magic_enum::magic_enum
            Iconv::Iconv
    )
    if (zstd::libzstd_static IN_LIST actual_links)
        list(APPEND expected_links zstd::libzstd_static)
    elseif (zstd::libzstd IN_LIST actual_links)
        list(APPEND expected_links zstd::libzstd)
    else ()
        message(FATAL_ERROR
                "${target} must link one supported zstd target")
    endif ()
    list(SORT actual_links)
    list(SORT expected_links)
    if (NOT actual_links STREQUAL expected_links)
        message(FATAL_ERROR
                "${target} direct link closure differs from "
                "WebGameplayCore.cmake: ${actual_links}")
    endif ()

    rhythmgame_web_gameplay_core_check_link_closure("${target}")

    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_PORTABLE_SOURCE_CLOSURE
            "${RHYTHMGAME_WEB_GAMEPLAY_CORE_RELATIVE_SOURCES}")
    set_property(TARGET "${target}" PROPERTY
            RHYTHMGAME_PORTABLE_LINK_CLOSURE
            "${actual_links}")
endfunction()

function(rhythmgame_add_web_gameplay_core target)
    if (TARGET "${target}")
        rhythmgame_verify_web_gameplay_core_target("${target}")
        return()
    endif ()

    find_package(Threads REQUIRED)
    find_package(Iconv REQUIRED)

    if (TARGET zstd::libzstd_static)
        set(zstd_target zstd::libzstd_static)
    elseif (TARGET zstd::libzstd)
        set(zstd_target zstd::libzstd)
    else ()
        message(FATAL_ERROR "A zstd CMake target is required")
    endif ()

    qt_add_library("${target}" STATIC
            ${RHYTHMGAME_WEB_GAMEPLAY_CORE_SOURCES})
    target_compile_features("${target}" PUBLIC cxx_std_23)
    target_compile_definitions("${target}"
            PRIVATE RHYTHMGAME_PORTABLE_GAMEPLAY
            PUBLIC
            NOMINMAX
            RHYTHMGAME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
            RHYTHMGAME_VERSION_MINOR=${PROJECT_VERSION_MINOR}
            RHYTHMGAME_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    )
    target_include_directories("${target}" PUBLIC
            "$<BUILD_INTERFACE:${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}/src>")
    target_link_libraries("${target}" PUBLIC
            Qt6::Core
            Qt6::Qml
            Threads::Threads
            Boost::headers
            spdlog::spdlog_header_only
            foonathan::lexy
            magic_enum::magic_enum
            "${zstd_target}"
            Iconv::Iconv
    )

    rhythmgame_verify_web_gameplay_core_target("${target}")

    set(source_manifest
            "${CMAKE_CURRENT_BINARY_DIR}/${target}-sources.txt")
    string(REPLACE ";" "\n" manifest_contents
            "${RHYTHMGAME_WEB_GAMEPLAY_CORE_RELATIVE_SOURCES}")
    file(GENERATE OUTPUT "${source_manifest}"
            CONTENT "${manifest_contents}\n")
    add_custom_target("${target}_source_contract"
            COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ROOT=${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}"
            "-DSOURCE_MANIFEST=${source_manifest}"
            -P
            "${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}/cmake/CheckWebGameplayCore.cmake"
            VERBATIM
    )
    add_dependencies("${target}" "${target}_source_contract")

    if (RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT)
        if (NOT CMAKE_GENERATOR MATCHES "^Ninja")
            message(FATAL_ERROR
                    "RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT "
                    "requires a Ninja generator; ${CMAKE_GENERATOR} does not "
                    "expose compiler-ingested dependencies through "
                    "'ninja -t deps'")
        endif ()
        set(dependency_contract_script
                "${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}/cmake/CheckWebGameplayCoreDependencies.cmake")
        set(dependency_contract_stamp
                "${CMAKE_CURRENT_BINARY_DIR}/${target}-dependency-contract.stamp")
        set(dependency_contract_anchor_source
                "${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}/src/gameplay_logic/SinglePlayerGameplayCore.cpp")
        set_property(SOURCE "${dependency_contract_anchor_source}"
                APPEND PROPERTY OBJECT_DEPENDS
                "${dependency_contract_script}")
        add_custom_command(TARGET "${target}" POST_BUILD
                BYPRODUCTS "${dependency_contract_stamp}"
                COMMAND ${CMAKE_COMMAND} -E rm -f
                "${dependency_contract_stamp}"
                COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_ROOT=${RHYTHMGAME_WEB_GAMEPLAY_CORE_ROOT}"
                "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
                "-DNINJA_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                "-DTARGET_NAME=${target}"
                -P
                "${dependency_contract_script}"
                COMMAND ${CMAKE_COMMAND} -E touch
                "${dependency_contract_stamp}"
                VERBATIM
        )
    else ()
        message(STATUS
                "${target}: Ninja compiler-dependency audit is not active; "
                "set RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT=ON "
                "for browser and contract-verification builds")
    endif ()
endfunction()
