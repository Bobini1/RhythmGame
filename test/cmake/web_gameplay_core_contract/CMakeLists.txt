cmake_minimum_required(VERSION 3.21)
project(WebGameplayCoreContractTest VERSION 1.3.13 LANGUAGES CXX)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

set(CMAKE_AUTOMOC ON)
if (NOT DEFINED RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT)
    set(RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT ON)
endif ()

find_package(Qt6 REQUIRED COMPONENTS Core Qml)
find_package(Boost REQUIRED COMPONENTS headers)
find_package(spdlog REQUIRED)
find_package(zstd REQUIRED)
find_package(lexy REQUIRED)
find_package(magic_enum REQUIRED)

get_filename_component(
        RHYTHMGAME_ROOT
        "${CMAKE_CURRENT_LIST_DIR}/../../.."
        ABSOLUTE
)

if (CONTRACT_CASE STREQUAL "nested_forbidden_link")
    add_library(RhythmGame_contract_iconv_model INTERFACE)
    add_library(Iconv::Iconv ALIAS RhythmGame_contract_iconv_model)
    add_library(RhythmGame_contract_hidden_helper INTERFACE)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(
            RhythmGame_contract_hidden_helper INTERFACE SQLiteCpp)
    target_link_libraries(
            RhythmGame_contract_iconv_model
            INTERFACE RhythmGame_contract_hidden_helper)
endif ()

include("${RHYTHMGAME_ROOT}/cmake/WebGameplayCore.cmake")
rhythmgame_add_web_gameplay_core(RhythmGame_web_gameplay_core)

if (CONTRACT_CASE STREQUAL "forbidden_header")
    target_compile_options(RhythmGame_web_gameplay_core PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/FI${RHYTHMGAME_ROOT}/src/resource_managers/Vars.h>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${RHYTHMGAME_ROOT}/src/resource_managers/Vars.h>"
    )
elseif (CONTRACT_CASE STREQUAL "nested_forbidden_link")
    rhythmgame_verify_web_gameplay_core_target(
            RhythmGame_web_gameplay_core)
elseif (CONTRACT_CASE STREQUAL "direct_forbidden_import")
    add_library(SQLiteCpp INTERFACE IMPORTED)
    target_link_libraries(
            RhythmGame_web_gameplay_core PUBLIC SQLiteCpp)
    rhythmgame_web_gameplay_core_check_link_closure(
            RhythmGame_web_gameplay_core)
elseif (CONTRACT_CASE STREQUAL "imported_profiler_genex")
    add_library(
            RhythmGame_contract_authenticated_package INTERFACE IMPORTED)
    set_property(
            TARGET RhythmGame_contract_authenticated_package
            PROPERTY INTERFACE_LINK_LIBRARIES
            "$<$<BOOL:0>:RhythmGameContractProfilerPlugin>")
    set_property(TARGET Boost::headers APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            RhythmGame_contract_authenticated_package
    )
    rhythmgame_verify_web_gameplay_core_target(
            RhythmGame_web_gameplay_core)
elseif (CONTRACT_CASE STREQUAL "clean")
    # The unmodified target is built by the positive contract test.
else ()
    message(FATAL_ERROR
            "CONTRACT_CASE must be clean, forbidden_header, "
            "nested_forbidden_link, direct_forbidden_import, or "
            "imported_profiler_genex")
endif ()
