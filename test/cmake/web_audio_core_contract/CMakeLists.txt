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

if (CONTRACT_CASE STREQUAL "missing_compile_contract")
    add_library(RhythmGame_web_audio_core STATIC
            ${RHYTHMGAME_WEB_AUDIO_CORE_SOURCES})
    target_link_libraries(RhythmGame_web_audio_core PUBLIC Qt6::Core)
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
elseif (CONTRACT_CASE STREQUAL "forbidden_header")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)
    find_path(STB_INCLUDE_DIR stb_vorbis.c REQUIRED)
    target_include_directories(RhythmGame_web_audio_core PRIVATE
            "${MINIAUDIO_INCLUDE_DIR}"
            "${STB_INCLUDE_DIR}"
    )
    target_compile_options(RhythmGame_web_audio_core PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/FI${CMAKE_CURRENT_LIST_DIR}/ForcedAudioEngine.h>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${CMAKE_CURRENT_LIST_DIR}/ForcedAudioEngine.h>"
    )
elseif (CONTRACT_CASE STREQUAL "nested_forbidden_link")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    add_library(RhythmGame_contract_hidden_helper INTERFACE)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(
            RhythmGame_contract_hidden_helper INTERFACE SQLiteCpp)
    set_property(TARGET Qt6::Core APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            "$<$<BOOL:1>:$<LINK_ONLY:$<TARGET_NAME_IF_EXISTS:RhythmGame_contract_hidden_helper>>>"
    )
    rhythmgame_verify_web_audio_core_target(RhythmGame_web_audio_core)
elseif (CONTRACT_CASE STREQUAL "clean")
    rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)
    # The unmodified portable target is built by the positive contract test.
else ()
    message(FATAL_ERROR
            "CONTRACT_CASE must be clean, forbidden_header, or "
            "nested_forbidden_link, or missing_compile_contract")
endif ()
