cmake_minimum_required(VERSION 3.21)
project(WebGameplayCoreContractTest VERSION 1.3.13 LANGUAGES CXX)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

set(CMAKE_AUTOMOC ON)
set(RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT ON)

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
include("${RHYTHMGAME_ROOT}/cmake/WebGameplayCore.cmake")
rhythmgame_add_web_gameplay_core(RhythmGame_web_gameplay_core)

if (CONTRACT_CASE STREQUAL "forbidden_header")
    target_compile_options(RhythmGame_web_gameplay_core PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/FI${RHYTHMGAME_ROOT}/src/resource_managers/Vars.h>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include>"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${RHYTHMGAME_ROOT}/src/resource_managers/Vars.h>"
    )
elseif (CONTRACT_CASE STREQUAL "nested_forbidden_link")
    add_library(RhythmGame_contract_hidden_helper INTERFACE)
    add_library(SQLiteCpp INTERFACE)
    target_link_libraries(
            RhythmGame_contract_hidden_helper INTERFACE SQLiteCpp)
    set_property(TARGET Boost::headers APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES
            "$<$<BOOL:1>:$<LINK_ONLY:$<TARGET_NAME_IF_EXISTS:RhythmGame_contract_hidden_helper>>>"
    )
    rhythmgame_verify_web_gameplay_core_target(
            RhythmGame_web_gameplay_core)
elseif (CONTRACT_CASE STREQUAL "clean")
    # The unmodified target is built by the positive contract test.
else ()
    message(FATAL_ERROR
            "CONTRACT_CASE must be clean, forbidden_header, or "
            "nested_forbidden_link")
endif ()
