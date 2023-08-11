cmake_minimum_required(VERSION 3.21)

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

# <------------ SQLiteCpp (db) --------------->

find_package(SQLiteCpp REQUIRED)

# <------------ lexy (parsing) --------------->

find_package(lexy REQUIRED)

# <------------ fmt (text formatting) --------------->

find_package(fmt REQUIRED)

# <------------ spdlog (logging) --------------->

find_package(spdlog REQUIRED)

# <------------ boost --------------->

find_package(Boost REQUIRED)

# <------------ sfml (game engine) --------------->

find_package(SFML REQUIRED)

# <------------ FFmpeg (audio decoding) --------------->

find_package(FFmpeg REQUIRED)

# <------------ OpenAL (audio playback) --------------->

find_package(OpenAL REQUIRED)

# <------------ Qt (gui) --------------->

find_package(Qt6 COMPONENTS Quick Gui Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

# ---- Load library ----

qt_add_library(RhythmGame_lib OBJECT src/db/SqliteCppDb.cpp src/db/SqliteCppDb.h src/charts/chart_readers/BmsChartReader.cpp src/charts/parser_models/ParsedBmsChart.cpp src/charts/parser_models/ParsedBmsChart.h src/resource_managers/QmlScriptFinder.cpp src/resource_managers/QmlScriptFinder.h src/db/DatabaseAccessPoint.cpp src/db/DatabaseAccessPoint.h src/support/EnableSharedFromBase.cpp src/support/EnableSharedFromBase.h src/support/Capitalize.cpp src/support/Capitalize.h src/resource_managers/AssetsFolderFinder.cpp src/resource_managers/AssetsFolderFinder.h src/resource_managers/FindAssetsFolderBoost.cpp src/resource_managers/FindAssetsFolderBoost.h src/resource_managers/QmlScriptFinderImpl.cpp src/resource_managers/QmlScriptFinderImpl.h src/resource_managers/LoadConfig.cpp src/resource_managers/LoadConfig.h src/sounds/OpenAlSound.cpp src/sounds/OpenAlSound.h src/sounds/OpenAlSoundBuffer.h src/sounds/OpenAlSoundBuffer.cpp src/charts/gameplay_models/BmsChart.cpp src/charts/gameplay_models/BmsChart.h src/charts/gameplay_models/Snap.cpp src/charts/gameplay_models/Snap.h src/charts/helper_functions/loadBmsSounds.cpp src/charts/helper_functions/loadBmsSounds.h RhythmGameQml/SceneSwitcher.cpp RhythmGameQml/SceneSwitcher.h)

find_package(Threads REQUIRED)

find_package(Microsoft.GSL REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC Qt6::Gui Qt6::Quick Qt6::OpenGL Threads::Threads Boost::headers Boost::serialization Boost::dynamic_linking Boost::filesystem SQLiteCpp sfml::sfml Microsoft.GSL::GSL spdlog::spdlog foonathan::lexy ffmpeg::ffmpeg OpenAL::OpenAL)

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

# ---- Declare QML module ----
add_subdirectory(RhythmGameQml)

# ---- Declare executable ----

qt_add_executable(RhythmGame_exe src/main.cpp)
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


#add_custom_command(TARGET RhythmGame_exe POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:RhythmGame_exe> $<TARGET_FILE_DIR:RhythmGame_exe>
#        COMMAND_EXPAND_LISTS
#        )

target_compile_features(RhythmGame_exe PRIVATE cxx_std_20)

target_link_libraries(RhythmGame_exe PRIVATE RhythmGame_lib RhythmGame_qmlplugin)

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
