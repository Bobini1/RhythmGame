cmake_minimum_required(VERSION 3.20)

include(cmake/prelude.cmake)
project(
        RhythmGame
        VERSION 0.0.0
        DESCRIPTION "A performant and portable rhythm game engine"
        HOMEPAGE_URL "https://github.com/Bobini1"
        LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# <------------ lua support --------------->

find_package(sol2 REQUIRED)
find_package(luajit REQUIRED)

# <------------ SQLiteCpp (db) --------------->

find_package(SQLiteCpp REQUIRED)

# <------------ pegtl (parsing) --------------->

find_package(pegtl REQUIRED)

# <------------ fmt (text formatting) --------------->

find_package(fmt REQUIRED)

# <------------ boost --------------->

find_package(Boost REQUIRED)

# <------------ type_safe (utils) --------------->

find_package(type_safe REQUIRED)

# <------------ sfml (game engine) --------------->

find_package(SFML REQUIRED)

# <----------- dependency injection --------------->

find_package(di REQUIRED)

# ---- Load library ----

add_library(RhythmGame_lib OBJECT src/db/sqlite_cpp_db/SqliteCppDb.cpp src/db/sqlite_cpp_db/SqliteCppDb.h
        src/charts/chart_readers/BmsChartReader.cpp src/charts/models/Chart.h src/charts/models/Chart.cpp src/charts/chart_readers/ToChars.h src/charts/behaviour/SongDataWriterToLua.cpp src/charts/behaviour/SongDataWriterToLua.h src/charts/models/BmsChart.cpp src/charts/models/BmsChart.h src/drawing/MainWindow.cpp src/drawing/MainWindow.h src/drawing/Actor.cpp src/drawing/Actor.h src/drawing/SplashWindow.cpp src/drawing/SplashWindow.h src/drawing/Scene.cpp src/drawing/Scene.h src/drawing/SplashScene.cpp src/drawing/SplashScene.h src/resource_locators/LuaScriptFinder.cpp src/resource_locators/LuaScriptFinder.h src/state_transitions/SceneStateMachine.cpp src/state_transitions/SceneStateMachine.h src/state_transitions/SceneStateMachineImpl.cpp src/state_transitions/SceneStateMachineImpl.h src/state_transitions/WindowStateMachine.cpp src/state_transitions/WindowStateMachine.h src/drawing/Window.cpp src/drawing/Window.h src/state_transitions/WindowStateMachineImpl.cpp src/state_transitions/WindowStateMachineImpl.h src/lua/Bootstrapper.cpp src/lua/Bootstrapper.h src/wiring/Injector.h src/state_transitions/Game.cpp src/state_transitions/Game.h src/charts/behaviour/SongDataWriter.cpp src/charts/behaviour/SongDataWriter.h)
message(STATUS "CMake DL Libs: ${CMAKE_DL_LIBS}")

find_package(Threads REQUIRED)

target_link_libraries(RhythmGame_lib PRIVATE Threads::Threads taocpp::pegtl PUBLIC Boost::headers type_safe::type_safe luajit::luajit sol2::sol2 SQLiteCpp sfml::sfml di::di ${CMAKE_DL_LIBS})

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(std::execution::par "execution" RHYTHMGAME_HAS_STD_EXECUTION)

if (RHYTHMGAME_HAS_STD_EXECUTION)
    target_compile_definitions(RhythmGame_lib PUBLIC RHYTHMGAME_HAS_STD_EXECUTION)
endif ()

# We all love MSVC.
target_compile_definitions(RhythmGame_lib PUBLIC NOMINMAX)

target_compile_features(RhythmGame_lib PRIVATE cxx_std_20)
target_compile_definitions(RhythmGame_lib PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_include_directories(
        RhythmGame_lib ${warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

# ---- Declare executable ----

add_executable(RhythmGame_exe src/main.cpp)
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
)

target_compile_features(RhythmGame_exe PRIVATE cxx_std_20)

target_link_libraries(RhythmGame_exe PRIVATE RhythmGame_lib)
# MacOS luajit linker flags
if (APPLE)
    set_target_properties(RhythmGame_exe PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif ()
# Get all properties that cmake supports
if (NOT CMAKE_PROPERTY_LIST)
    execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

    # Convert command output into a CMake list
    string(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
    string(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
endif ()

function(print_properties)
    message("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction()

function(print_target_properties target)
    if (NOT TARGET ${target})
        message(STATUS "There is no target named '${target}'")
        return()
    endif ()

    foreach (property ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" property ${property})

        # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
        if (property STREQUAL "LOCATION" OR property MATCHES "^LOCATION_" OR property MATCHES "_LOCATION$")
            continue()
        endif ()

        get_property(was_set TARGET ${target} PROPERTY ${property} SET)
        if (was_set)
            get_target_property(value ${target} ${property})
            message("${target} ${property} = ${value}")
        endif ()
    endforeach ()
endfunction()
find_package(TBB REQUIRED)
print_target_properties(TBB::tbb)
if (NOT APPLE)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        find_package(TBB REQUIRED)
        target_link_libraries(RhythmGame_lib PUBLIC TBB::tbb)
    endif ()
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
    target_compile_options(RhythmGame_lib PUBLIC "-fconcepts")
endif ()

# ---- Install rules ----

if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()

# ---- Developer mode ----

if (NOT RhythmGame_DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of RhythmGame"
    )
endif ()

include(cmake/dev-mode.cmake)
