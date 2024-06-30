cmake_minimum_required(VERSION 3.21)

include(cmake/prelude.cmake)
project(
        RhythmGame
        VERSION 0.0.0
        DESCRIPTION "A performant and portable rhythm game engine"
        HOMEPAGE_URL "https://github.com/Bobini1"
        LANGUAGES C CXX
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
target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_WCHAR_FILENAMES)


# <------------ boost --------------->

find_package(Boost REQUIRED COMPONENTS locale serialization filesystem)

# <------------ OpenAL (audio playback) --------------->

find_package(OpenAL REQUIRED)

# <------------ Qt (gui) --------------->

find_package(Qt6 COMPONENTS ShaderTools Quick Gui Widgets Core5Compat Concurrent Multimedia Svg REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(cryptopp REQUIRED)
find_package(magic_enum REQUIRED)

# <------------ zstd (compression) --------------->

find_package(zstd REQUIRED)

# ---- Load library ----

qt_add_library(RhythmGame_lib OBJECT src/db/SqliteCppDb.cpp src/db/SqliteCppDb.h src/charts/chart_readers/BmsChartReader.cpp src/charts/parser_models/ParsedBmsChart.cpp src/charts/parser_models/ParsedBmsChart.h src/support/EnableSharedFromBase.cpp src/support/EnableSharedFromBase.h src/support/Capitalize.cpp src/support/Capitalize.h src/resource_managers/FindAssetsFolderBoost.cpp src/resource_managers/FindAssetsFolderBoost.h src/sounds/OpenAlSound.cpp src/sounds/OpenAlSound.h src/sounds/OpenAlSoundBuffer.h src/sounds/OpenAlSoundBuffer.cpp src/charts/gameplay_models/BmsNotesData.cpp src/charts/gameplay_models/BmsNotesData.h src/charts/gameplay_models/Snap.cpp src/charts/gameplay_models/Snap.h src/charts/helper_functions/loadBmsSounds.cpp src/charts/helper_functions/loadBmsSounds.h
        src/gameplay_logic/BmsPoints.cpp src/gameplay_logic/BmsPoints.h src/gameplay_logic/rules/StandardBmsHitRules.cpp src/gameplay_logic/rules/StandardBmsHitRules.h src/gameplay_logic/Judgement.cpp src/gameplay_logic/Judgement.h src/gameplay_logic/TimePoint.cpp src/gameplay_logic/TimePoint.h src/support/Sha256.cpp src/support/Sha256.h
        src/input/BmsKeys.cpp
        src/gameplay_logic/BmsScore.h
        src/gameplay_logic/BmsScore.cpp
        src/gameplay_logic/BmsGameReferee.cpp
        src/gameplay_logic/BmsGameReferee.h
        src/gameplay_logic/ChartData.cpp
        src/gameplay_logic/ChartData.h
        src/gameplay_logic/Chart.cpp
        src/gameplay_logic/Chart.h
        src/gameplay_logic/HitEvent.cpp
        src/gameplay_logic/HitEvent.h
        src/resource_managers/ChartDataFactory.cpp
        src/resource_managers/ChartDataFactory.h
        src/gameplay_logic/BmsNotes.cpp
        src/gameplay_logic/BmsNotes.h
        src/resource_managers/ChartFactory.cpp
        src/resource_managers/ChartFactory.h
        src/resource_managers/IniImageProvider.cpp
        src/resource_managers/IniImageProvider.h
        src/gameplay_logic/rules/Lr2TimingWindows.cpp
        src/gameplay_logic/rules/Lr2TimingWindows.h
        src/gameplay_logic/rules/BmsRanks.h
        src/gameplay_logic/rules/BmsHitRules.cpp
        src/gameplay_logic/rules/BmsHitRules.h
        src/gameplay_logic/rules/TimingWindows.h
        src/gameplay_logic/rules/Lr2HitValues.cpp
        src/gameplay_logic/rules/Lr2HitValues.h
        src/gameplay_logic/rules/BmsGauge.cpp
        src/gameplay_logic/rules/BmsGauge.h
        src/gameplay_logic/rules/Lr2Gauge.cpp
        src/gameplay_logic/rules/Lr2Gauge.h
        src/resource_managers/SongDbScanner.cpp
        src/resource_managers/SongDbScanner.h
        src/support/PathToQString.cpp
        src/support/PathToQString.h
        src/support/QStringToPath.cpp
        src/support/QStringToPath.h
        src/support/PathToUtfString.cpp
        src/support/PathToUtfString.h
        src/support/UtfStringToPath.cpp
        src/support/UtfStringToPath.h
        src/qml_components/SongFolderFactory.cpp
        src/qml_components/SongFolderFactory.h
        src/qml_components/ChartLoader.cpp
        src/qml_components/ChartLoader.h
        src/qml_components/ProgramSettings.cpp
        src/qml_components/ProgramSettings.h
        src/qml_components/Logger.cpp
        src/qml_components/Logger.h
        src/qml_components/RootSongFoldersConfig.cpp
        src/qml_components/RootSongFoldersConfig.h
        src/support/toLower.cpp
        src/support/toLower.h
        src/resource_managers/Profile.cpp
        src/resource_managers/Profile.h
        src/gameplay_logic/BmsResult.cpp
        src/gameplay_logic/BmsResult.h
        src/gameplay_logic/BmsReplayData.cpp
        src/gameplay_logic/BmsReplayData.h
        src/gameplay_logic/BmsGaugeHistory.cpp
        src/gameplay_logic/BmsGaugeHistory.h
        src/support/Compress.cpp
        src/support/Compress.h
        src/gameplay_logic/BmsScoreAftermath.cpp
        src/gameplay_logic/BmsScoreAftermath.h
        src/qml_components/ProfileList.cpp
        src/qml_components/ProfileList.h
        src/qml_components/PreviewFilePathFetcher.cpp
        src/qml_components/PreviewFilePathFetcher.h
        src/qml_components/ScoreDb.cpp
        src/qml_components/ScoreDb.h
        src/qml_components/FileValidator.cpp
        src/qml_components/FileValidator.h
        src/qml_components/Bga.cpp
        src/qml_components/Bga.h
        src/gameplay_logic/MineHit.cpp
        src/gameplay_logic/MineHit.h
        src/gameplay_logic/rules/CnHitRules.cpp
        src/gameplay_logic/rules/CnHitRules.h
        src/qml_components/ThemeFamily.cpp
        src/qml_components/ThemeFamily.h
        src/resource_managers/ScanThemes.cpp
        src/resource_managers/ScanThemes.h
        src/qml_components/Themes.cpp
        src/qml_components/Themes.h
        src/resource_managers/SerializeConfig.cpp
        src/resource_managers/SerializeConfig.h
        src/input/GamepadManager.cpp
        src/input/GamepadManager.h
        src/input/InputTranslator.cpp
        src/input/InputTranslator.h
        src/resource_managers/DefineDb.cpp
        src/resource_managers/DefineDb.h)

find_package(Threads REQUIRED)

find_package(SndFile REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC SDL2::SDL2 Qt6::Gui Qt6::Quick Qt6::OpenGL Qt6::Widgets Qt6::Concurrent Qt6::Core5Compat Qt6::Multimedia Threads::Threads Boost::headers Boost::locale Boost::serialization Boost::dynamic_linking Boost::filesystem SQLiteCpp spdlog::spdlog foonathan::lexy OpenAL::OpenAL cryptopp::cryptopp magic_enum::magic_enum zstd::libzstd_static SndFile::sndfile)

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
qt_import_qml_plugins(RhythmGame_qmlplugin)

# ---- Declare executable ----

qt_add_executable(RhythmGame_exe src/main.cpp)
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)

qt_import_qml_plugins(RhythmGame_exe)


add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets assets)

if (WIN32)
    set(alsoft_ext "ini")
else ()
    set(alsoft_ext "conf")
endif ()

# copy alsoft.ini
add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/alsoft.ini
        ${CMAKE_BINARY_DIR}/bin/$<0:>/alsoft.${alsoft_ext})

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
