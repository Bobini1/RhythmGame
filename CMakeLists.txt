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

# ---- Load library ----

add_library(RhythmGame_lib OBJECT src/db/SqliteCppDb.cpp src/db/SqliteCppDb.h src/charts/chart_readers/BmsChartReader.cpp src/charts/models/Chart.h src/charts/models/Chart.cpp src/charts/chart_readers/ToChars.h src/charts/behaviour/SongDataWriterToLua.cpp src/charts/behaviour/SongDataWriterToLua.h src/charts/models/BmsChart.cpp src/charts/models/BmsChart.h src/drawing/MainWindow.cpp src/drawing/MainWindow.h src/drawing/actors/Actor.cpp src/drawing/actors/Actor.h src/drawing/SplashWindow.cpp src/drawing/SplashWindow.h src/drawing/Scene.cpp src/drawing/Scene.h src/drawing/SplashScene.cpp src/drawing/SplashScene.h src/resource_managers/LuaScriptFinder.cpp src/resource_managers/LuaScriptFinder.h src/state_transitions/SceneStateMachine.cpp src/state_transitions/SceneStateMachine.h src/state_transitions/SceneStateMachineImpl.cpp src/state_transitions/SceneStateMachineImpl.h src/state_transitions/WindowStateMachine.cpp src/state_transitions/WindowStateMachine.h src/drawing/Window.cpp src/drawing/Window.h src/state_transitions/WindowStateMachineImpl.cpp src/state_transitions/WindowStateMachineImpl.h src/lua/Bootstrapper.cpp src/lua/Bootstrapper.h src/state_transitions/Game.cpp src/state_transitions/Game.h src/charts/behaviour/SongDataWriter.cpp src/charts/behaviour/SongDataWriter.h src/db/DatabaseAccessPoint.cpp src/db/DatabaseAccessPoint.h src/drawing/actors/Quad.cpp src/drawing/actors/Quad.h src/drawing/actors/VBox.cpp src/drawing/actors/VBox.h src/drawing/actors/AbstractVectorCollection.cpp src/drawing/actors/AbstractVectorCollection.h src/drawing/actors/Parent.cpp src/drawing/actors/Parent.h src/support/EnableSharedFromBase.cpp src/support/EnableSharedFromBase.h src/drawing/actors/HBox.cpp src/drawing/actors/HBox.h src/drawing/actors/Text.cpp src/drawing/actors/Text.h src/drawing/actors/Sprite.cpp src/drawing/actors/Sprite.h src/resource_managers/TextureLoaderImpl.cpp src/resource_managers/TextureLoaderImpl.h src/resource_managers/TextureLoader.cpp src/resource_managers/TextureLoader.h src/resource_managers/TextureLoaderImpl.cpp src/resource_managers/TextureLoaderImpl.h src/resource_managers/FontLoader.cpp src/resource_managers/FontLoader.h src/resource_managers/FontLoaderImpl.cpp src/resource_managers/FontLoaderImpl.h src/drawing/actors/Padding.cpp src/drawing/actors/Padding.h src/drawing/actors/Align.cpp src/drawing/actors/Align.h src/drawing/actors/AbstractBox.cpp src/drawing/actors/AbstractBox.h src/drawing/actors/AbstractRectLeaf.cpp src/drawing/actors/AbstractRectLeaf.h)

find_package(Threads REQUIRED)

find_package(Microsoft.GSL REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC Threads::Threads taocpp::pegtl Boost::headers type_safe::type_safe luajit::luajit sol2::sol2 SQLiteCpp sfml::sfml Microsoft.GSL::GSL Boost::log)

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

if (NOT APPLE)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        find_package(TBB REQUIRED)
        target_link_libraries(RhythmGame_lib PUBLIC TBB::tbb)
    endif ()
endif ()

get_target_property(SQLITECPP_INCLUDE_DIRS SQLiteCpp INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "SQLiteCpp include dirs: ${SQLITECPP_INCLUDE_DIRS}")

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

target_precompile_headers(RhythmGame_lib PRIVATE "<memory>" "<vector>" "<chrono>")

include(cmake/dev-mode.cmake)
