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
find_package(lexy REQUIRED)

# <------------ fmt (text formatting) --------------->

find_package(fmt REQUIRED)

# <------------ spdlog (logging) --------------->

find_package(spdlog REQUIRED)

# <------------ boost --------------->

find_package(Boost REQUIRED)

# <------------ type_safe (utils) --------------->

find_package(type_safe REQUIRED)

# <------------ sfml (game engine) --------------->

find_package(SFML REQUIRED)

# ---- Load library ----

add_library(RhythmGame_lib OBJECT src/db/SqliteCppDb.cpp src/db/SqliteCppDb.h src/charts/chart_readers/BmsChartReader.cpp src/charts/chart_readers/ToChars.h src/charts/models/BmsChart.cpp src/charts/models/BmsChart.h src/drawing/MainWindow.cpp src/drawing/MainWindow.h src/drawing/actors/Actor.cpp src/drawing/actors/Actor.h src/drawing/SplashWindow.cpp src/drawing/SplashWindow.h src/drawing/Scene.cpp src/drawing/Scene.h src/drawing/SplashScene.cpp src/drawing/SplashScene.h src/resource_managers/LuaScriptFinder.cpp src/resource_managers/LuaScriptFinder.h src/state_transitions/SceneStateMachine.cpp src/state_transitions/SceneStateMachine.h src/state_transitions/SceneStateMachineImpl.cpp src/state_transitions/SceneStateMachineImpl.h src/drawing/Window.cpp src/drawing/Window.h src/lua/Bootstrapper.cpp src/lua/Bootstrapper.h src/state_transitions/Game.cpp src/state_transitions/Game.h src/db/DatabaseAccessPoint.cpp src/db/DatabaseAccessPoint.h src/drawing/actors/Quad.cpp src/drawing/actors/Quad.h src/drawing/actors/VBox.cpp src/drawing/actors/VBox.h src/drawing/actors/AbstractVectorCollection.cpp src/drawing/actors/AbstractVectorCollection.h src/drawing/actors/Parent.cpp src/drawing/actors/Parent.h src/support/EnableSharedFromBase.cpp src/support/EnableSharedFromBase.h src/drawing/actors/HBox.cpp src/drawing/actors/HBox.h src/drawing/actors/Text.cpp src/drawing/actors/Text.h src/drawing/actors/Sprite.cpp src/drawing/actors/Sprite.h src/resource_managers/TextureLoaderImpl.cpp src/resource_managers/TextureLoaderImpl.h src/resource_managers/TextureLoader.cpp src/resource_managers/TextureLoader.h src/resource_managers/TextureLoaderImpl.cpp src/resource_managers/TextureLoaderImpl.h src/resource_managers/FontLoader.cpp src/resource_managers/FontLoader.h src/resource_managers/FontLoaderImpl.cpp src/resource_managers/FontLoaderImpl.h src/drawing/actors/Padding.cpp src/drawing/actors/Padding.h src/drawing/actors/Align.cpp src/drawing/actors/Align.h src/drawing/actors/AbstractBox.cpp src/drawing/actors/AbstractBox.h src/drawing/actors/AbstractRectLeaf.cpp src/drawing/actors/AbstractRectLeaf.h src/drawing/actors/Layers.cpp src/drawing/actors/Layers.h src/events/GlobalEvent.cpp src/events/GlobalEvent.h src/drawing/animations/Animation.cpp src/drawing/animations/Animation.h src/drawing/animations/Linear.cpp src/drawing/animations/Linear.h src/drawing/animations/AnimationPlayerImpl.cpp src/drawing/animations/AnimationPlayerImpl.h src/drawing/animations/AnimationPlayer.cpp src/drawing/animations/AnimationPlayer.h src/drawing/animations/AbstractBasicAnimation.cpp src/drawing/animations/AbstractBasicAnimation.h src/drawing/animations/AnimationSequence.cpp src/drawing/animations/AnimationSequence.h src/lua/EventAttacher.cpp src/lua/EventAttacher.h src/events/MouseClickEvent.cpp src/events/MouseClickEvent.h src/events/MouseHoverEvents.cpp src/events/MouseHoverEvents.h src/events/Event.cpp src/events/Event.h src/support/Capitalize.cpp src/support/Capitalize.h src/lua/Lua.h src/resource_managers/AssetsFolderFinder.cpp src/resource_managers/AssetsFolderFinder.h src/resource_managers/FindAssetsFolderBoost.cpp src/resource_managers/FindAssetsFolderBoost.h src/resource_managers/LuaScriptFinderImpl.cpp src/resource_managers/LuaScriptFinderImpl.h src/resource_managers/LoadConfig.cpp src/resource_managers/LoadConfig.h)

find_package(Threads REQUIRED)

find_package(Microsoft.GSL REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC Threads::Threads taocpp::pegtl Boost::headers Boost::dynamic_linking Boost::filesystem type_safe::type_safe luajit::luajit sol2::sol2 SQLiteCpp sfml::sfml Microsoft.GSL::GSL spdlog::spdlog foonathan::lexy)

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
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)

add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets assets)

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

# ---- Install rules ----

if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()

# Some object files are too big for MSVC in Debug mode
if (MSVC)
    target_compile_options(RhythmGame_lib PRIVATE /bigobj)
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
