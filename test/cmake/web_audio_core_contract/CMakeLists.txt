cmake_minimum_required(VERSION 3.21)
project(WebAudioCoreContractTest VERSION 1.3.13 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

get_filename_component(
        RHYTHMGAME_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../../.."
        ABSOLUTE
)
include("${RHYTHMGAME_ROOT}/cmake/WebAudioCore.cmake")

if (CONTRACT_CASE STREQUAL "preexisting_target")
    add_library(RhythmGame_web_audio_core STATIC
            ${RHYTHMGAME_WEB_AUDIO_CORE_SOURCES})
    target_compile_features(RhythmGame_web_audio_core PUBLIC cxx_std_23)
    target_compile_definitions(RhythmGame_web_audio_core PUBLIC NOMINMAX)
    target_include_directories(RhythmGame_web_audio_core PUBLIC
            "$<BUILD_INTERFACE:${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src>")
    target_link_libraries(RhythmGame_web_audio_core PUBLIC Qt6::Core)
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
elseif (CONTRACT_CASE STREQUAL "forbidden_header")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    set_property(
            SOURCE
            "${RHYTHMGAME_WEB_AUDIO_CORE_ROOT}/src/web_playtest/audio/RealtimeMixer.cpp"
            APPEND PROPERTY COMPILE_OPTIONS
            "$<$<CXX_COMPILER_ID:MSVC>:/FI${CMAKE_CURRENT_LIST_DIR}/sounds/AudioEngine.h>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${CMAKE_CURRENT_LIST_DIR}/sounds/AudioEngine.h>"
    )
elseif (CONTRACT_CASE STREQUAL "late_source")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    target_sources(RhythmGame_web_audio_core PRIVATE LateMutation.cpp)
elseif (CONTRACT_CASE STREQUAL "late_definition")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    target_compile_definitions(RhythmGame_web_audio_core PRIVATE LATE_MUTATION)
elseif (CONTRACT_CASE STREQUAL "late_include")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    target_include_directories(RhythmGame_web_audio_core PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}")
elseif (CONTRACT_CASE STREQUAL "late_private_link")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(RhythmGame_web_audio_core PRIVATE SQLiteCpp)
elseif (CONTRACT_CASE STREQUAL "late_interface_link")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(RhythmGame_web_audio_core INTERFACE SQLiteCpp)
elseif (CONTRACT_CASE STREQUAL "late_direct_link")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    add_library(SQLiteCpp INTERFACE)
    set_property(TARGET RhythmGame_web_audio_core APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES_DIRECT SQLiteCpp)
elseif (CONTRACT_CASE STREQUAL "late_direct_exclude")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    set_property(TARGET RhythmGame_web_audio_core APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES_DIRECT_EXCLUDE Qt6::Core)
elseif (CONTRACT_CASE STREQUAL "late_created_helper")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    set_property(TARGET Qt6::Core APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            "$<$<BOOL:1>:$<LINK_ONLY:$<TARGET_NAME_IF_EXISTS:RhythmGame_contract_late_helper>>>"
    )
    add_library(RhythmGame_contract_late_helper INTERFACE)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(
            RhythmGame_contract_late_helper INTERFACE SQLiteCpp)
elseif (CONTRACT_CASE STREQUAL "clean")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    if (EMSCRIPTEN)
        foreach (pthread_property
                COMPILE_OPTIONS
                INTERFACE_COMPILE_OPTIONS
                LINK_OPTIONS
                INTERFACE_LINK_OPTIONS)
            get_target_property(
                    pthread_value
                    RhythmGame_web_audio_core
                    "${pthread_property}")
            if (NOT pthread_value STREQUAL "-pthread")
                message(FATAL_ERROR
                        "RhythmGame_web_audio_core must own exact "
                        "${pthread_property}=-pthread, got ${pthread_value}")
            endif ()
        endforeach ()
    endif ()
    # The unmodified portable target is built by the positive contract test.
else ()
    message(FATAL_ERROR
            "Unknown CONTRACT_CASE: ${CONTRACT_CASE}")
endif ()
