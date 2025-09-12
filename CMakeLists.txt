cmake_minimum_required(VERSION 3.21)

include(cmake/prelude.cmake)
project(
        RhythmGame
        VERSION 1.0.5
        DESCRIPTION "A customizable BMS player for Windows and Linux. "
        HOMEPAGE_URL "https://github.com/Bobini1/RhythmGame"
        LANGUAGES C CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if (supported)
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()

# <---- mimalloc (faster memory allocation) ---->

find_package(mimalloc CONFIG REQUIRED)

# <------------ LibXml2 (html parsing) --------------->

find_package(LibXml2 REQUIRED)

# <------------ SQLiteCpp (db) --------------->

find_package(SQLiteCpp REQUIRED)

# <------------ lexy (parsing) --------------->

find_package(lexy REQUIRED)

# <------------ fmt (text formatting) --------------->

find_package(fmt REQUIRED)

# <------------ spdlog (logging) --------------->

find_package(spdlog REQUIRED)

# <------------ boost --------------->

find_package(Boost REQUIRED COMPONENTS headers)

# <------------ miniaudio (audio playback) --------------->

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

# <------------ Qt (gui) --------------->

find_package(Qt6 COMPONENTS ShaderTools Quick Concurrent Multimedia Svg InterfaceFramework LinguistTools REQUIRED)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES pl jp)

# <------------ zstd (compression) --------------->

find_package(zstd REQUIRED)

# <------------ SndFile (audio file reading) --------------->

find_package(SndFile REQUIRED)

# <------------ SDL2 (gamepad input handling) --------------->

find_package(SDL2 REQUIRED)

# <------------ magic_enum (enum reflection) --------------->

find_package(magic_enum REQUIRED)

find_package(Stb REQUIRED)

# ---- Load library ----

qt_add_library(RhythmGame_lib OBJECT
        src/db/SqliteCppDb.cpp
        src/db/SqliteCppDb.h
        src/charts/ReadBmsFile.cpp
        src/charts/ReadBmsFile.h
        src/charts/ParsedBmsChart.cpp
        src/charts/ParsedBmsChart.h
        src/support/EnableSharedFromBase.cpp
        src/support/EnableSharedFromBase.h
        src/support/Capitalize.cpp
        src/support/Capitalize.h
        src/resource_managers/FindDataFolder.cpp
        src/resource_managers/FindDataFolder.h
        src/sounds/Sound.cpp
        src/sounds/Sound.h
        src/sounds/SoundBuffer.h
        src/sounds/SoundBuffer.cpp
        src/charts/BmsNotesData.cpp
        src/charts/BmsNotesData.h
        src/charts/Snap.cpp
        src/charts/Snap.h
        src/resource_managers/loadBmsSounds.cpp
        src/resource_managers/loadBmsSounds.h
        src/gameplay_logic/BmsPoints.cpp
        src/gameplay_logic/BmsPoints.h
        src/gameplay_logic/rules/HitRules.cpp
        src/gameplay_logic/rules/HitRules.h
        src/gameplay_logic/Judgement.cpp
        src/gameplay_logic/Judgement.h
        src/gameplay_logic/TimePoint.cpp
        src/gameplay_logic/TimePoint.h
        src/support/Sha256.cpp
        src/support/Sha256.h
        src/input/BmsKeys.cpp
        src/gameplay_logic/BmsLiveScore.h
        src/gameplay_logic/BmsLiveScore.cpp
        src/gameplay_logic/BmsGameReferee.cpp
        src/gameplay_logic/BmsGameReferee.h
        src/gameplay_logic/ChartData.cpp
        src/gameplay_logic/ChartData.h
        src/gameplay_logic/ChartRunner.cpp
        src/gameplay_logic/ChartRunner.h
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
        src/gameplay_logic/BmsScore.cpp
        src/gameplay_logic/BmsScore.h
        src/qml_components/ProfileList.cpp
        src/qml_components/ProfileList.h
        src/qml_components/PreviewFilePathFetcher.cpp
        src/qml_components/PreviewFilePathFetcher.h
        src/qml_components/ScoreDb.cpp
        src/qml_components/ScoreDb.h
        src/qml_components/FileQuery.cpp
        src/qml_components/FileQuery.h
        src/qml_components/Bga.cpp
        src/qml_components/Bga.h
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
        src/resource_managers/DefineDb.h
        src/resource_managers/Vars.cpp
        src/resource_managers/Vars.h
        src/support/Exception.cpp
        src/support/Exception.h
        src/qml_components/InputAttached.cpp
        src/qml_components/InputAttached.h
        src/resource_managers/GaugeFactory.cpp
        src/resource_managers/GaugeFactory.h
        src/support/GeneratePermutation.cpp
        src/support/GeneratePermutation.h
        src/gameplay_logic/NoteState.cpp
        src/gameplay_logic/NoteState.h
        src/resource_managers/Tables.cpp
        src/resource_managers/Tables.h
        src/qml_components/QmlUtils.cpp
        src/qml_components/QmlUtils.h
        src/resource_managers/Languages.cpp
        src/resource_managers/Languages.h
        src/input/CustomNotifyApp.cpp
        src/input/CustomNotifyApp.h
        src/support/Version.cpp
        src/support/Version.h
        src/gameplay_logic/CourseRunner.cpp
        src/gameplay_logic/CourseRunner.h
        src/gameplay_logic/BmsResultCourse.cpp
        src/gameplay_logic/BmsResultCourse.h
        src/gameplay_logic/BmsScoreCourse.cpp
        src/gameplay_logic/BmsScoreCourse.h
        src/support/QtSink.cpp
        src/support/QtSink.h
        src/sounds/AudioEngine.cpp
        src/sounds/AudioEngine.h
        src/sounds/MiniaudioImpl.cpp
        src/sounds/AudioPlayer.cpp
        src/sounds/AudioPlayer.h
)

if (WIN32)
    target_sources(RhythmGame_lib INTERFACE staticAssets/utf8.manifest)
endif ()
# define version macros
target_compile_definitions(RhythmGame_lib PUBLIC
        RHYTHMGAME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        RHYTHMGAME_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        RHYTHMGAME_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

set(DATA_PATH "../data" CACHE STRING "Path to the data folder, relative to the executable")
target_compile_definitions(RhythmGame_lib PUBLIC RHYTHMGAME_DATA_PATH="${DATA_PATH}")

find_package(Threads REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>)
target_link_libraries(RhythmGame_lib PUBLIC SDL2::SDL2 Qt6::Quick Qt6::OpenGL Qt6::Concurrent Qt6::Multimedia Qt6::InterfaceFramework Threads::Threads Boost::headers SQLiteCpp spdlog::spdlog foonathan::lexy magic_enum::magic_enum zstd::libzstd_static SndFile::sndfile LibXml2::LibXml2)
target_include_directories(RhythmGame_lib PUBLIC ${MINIAUDIO_INCLUDE_DIRS} ${Stb_INCLUDE_DIR})

if (WIN32)
    # <------------ wil (Windows API helpers) --------------->

    find_package(wil CONFIG REQUIRED)
    target_link_libraries(RhythmGame_lib PUBLIC WIL::WIL)
else ()
    # <------------ llfio (directory scanning) --------------->

    find_package(llfio CONFIG REQUIRED)
    target_link_libraries(RhythmGame_lib PUBLIC llfio::sl)
endif ()

if (LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(xkbcommon REQUIRED IMPORTED_TARGET xkbcommon)
    target_link_libraries(RhythmGame_lib PUBLIC PkgConfig::xkbcommon)
endif ()

# We all love MSVC.
target_compile_definitions(RhythmGame_lib PUBLIC NOMINMAX)

target_compile_features(RhythmGame_lib PRIVATE cxx_std_23)
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

if (WIN32)
    enable_language("RC")
    qt_add_executable(RhythmGame_exe
            src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/staticAssets/RhythmGame.rc
    )
    set_target_properties(RhythmGame_exe PROPERTIES
            WIN32_EXECUTABLE $<IF:$<CONFIG:Debug,RelWithDebInfo>,FALSE,TRUE>
            OUTPUT_NAME RhythmGame
    )
    # use main() as entry point
    set_target_properties(RhythmGame_exe PROPERTIES
            LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
else ()
    qt_add_executable(RhythmGame_exe
            src/main.cpp
    )
endif ()
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

qt_add_resources(
        RhythmGame_exe
        "text_files"
        BASE
        "${CMAKE_SOURCE_DIR}"
        FILES
        ${CMAKE_SOURCE_DIR}/ATTRIBUTIONS.md ${CMAKE_SOURCE_DIR}/LICENSE.md
)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)

qt_import_qml_plugins(RhythmGame_exe)

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

if (WIN32)
    # Some object files are too big for MSVC in Debug mode
    target_compile_options(RhythmGame_lib PRIVATE /bigobj)
    set_target_properties(RhythmGame_exe PROPERTIES LINK_FLAGS "/STACK:8388608")
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
