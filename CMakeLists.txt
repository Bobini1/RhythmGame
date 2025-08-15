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
target_compile_definitions(spdlog::spdlog INTERFACE SPDLOG_WCHAR_FILENAMES)


# <------------ boost --------------->

find_package(Boost REQUIRED COMPONENTS headers)

# <------------ OpenAL (audio playback) --------------->

find_package(OpenAL CONFIG REQUIRED)

# <------------ llfio (directory scanning) --------------->

find_package(llfio CONFIG REQUIRED)

# <------------ Qt (gui) --------------->

find_package(Qt6 COMPONENTS ShaderTools Quick Concurrent Multimedia Svg InterfaceFramework LinguistTools REQUIRED)
qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES pl jp)

# <------------ zstd (compression) --------------->

find_package(zstd REQUIRED)

# <------------ SndFile (audio file reading) --------------->

find_package(SndFile REQUIRED)

# <------------ SDL2 (gamepad input handling) --------------->

find_package(SDL2 REQUIRED)

# <------------ ICU (Charset detection) --------------->

find_package(ICU COMPONENTS uc i18n data REQUIRED)

find_package(cryptopp REQUIRED)
find_package(magic_enum REQUIRED)

if (MSVC)
    set(utf8Manifest staticAssets/utf8.manifest)
endif ()

# ---- Load library ----

qt_add_library(RhythmGame_lib OBJECT ${utf8Manifest}
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
        src/resource_managers/FindAssetsFolder.cpp
        src/resource_managers/FindAssetsFolder.h
        src/sounds/OpenAlSound.cpp
        src/sounds/OpenAlSound.h
        src/sounds/OpenAlSoundBuffer.h
        src/sounds/OpenAlSoundBuffer.cpp
        src/charts/BmsNotesData.cpp
        src/charts/BmsNotesData.h
        src/charts/Snap.cpp
        src/charts/Snap.h
        src/resource_managers/loadBmsSounds.cpp
        src/resource_managers/loadBmsSounds.h
        src/gameplay_logic/BmsPoints.cpp
        src/gameplay_logic/BmsPoints.h
        src/gameplay_logic/rules/StandardBmsHitRules.cpp
        src/gameplay_logic/rules/StandardBmsHitRules.h
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
)
# define version macros
target_compile_definitions(RhythmGame_lib PUBLIC
        RHYTHMGAME_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        RHYTHMGAME_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        RHYTHMGAME_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

find_package(Threads REQUIRED)

target_link_libraries(RhythmGame_lib PUBLIC $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>)
target_link_libraries(RhythmGame_lib PUBLIC SDL2::SDL2 Qt6::Quick Qt6::OpenGL Qt6::Concurrent Qt6::Multimedia Qt6::InterfaceFramework Threads::Threads Boost::headers SQLiteCpp spdlog::spdlog foonathan::lexy OpenAL::OpenAL cryptopp::cryptopp magic_enum::magic_enum zstd::libzstd_static SndFile::sndfile llfio::sl LibXml2::LibXml2)
target_link_libraries(RhythmGame_lib PUBLIC ICU::uc ICU::i18n ICU::data)

if (WIN32)
    target_link_libraries(RhythmGame_lib PUBLIC llfio::ntkernel-error-category_sl)
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
    set(WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/staticAssets/RhythmGame.rc)
endif ()

qt_add_executable(RhythmGame_exe src/main.cpp ${WIN32_RESOURCES})
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)

qt_import_qml_plugins(RhythmGame_exe)


file(GLOB_RECURSE QML_FILES "${CMAKE_SOURCE_DIR}/assets/themes/Default/**/*.qml")
# use QM_OUTPUT_DIRECTORY once we switch to Qt 6.9
qt_add_translations(
        RhythmGame_exe
        TS_FILE_BASE
        Default
        TS_FILE_DIR
        ${CMAKE_SOURCE_DIR}/assets/themes/Default/translations
        SOURCES
        ${QML_FILES}
        LUPDATE_TARGET
        Default_translations
        QM_FILES_OUTPUT_VARIABLE
        LRELEASE_QM_FILES
)
add_custom_target(ReleaseAndCopy
        COMMAND ${CMAKE_COMMAND} -E copy
        ${LRELEASE_QM_FILES}
        ${CMAKE_SOURCE_DIR}/assets/themes/Default/translations
        COMMENT "Copying qm files to assets/themes/Default/translations"
        VERBATIM
)
add_dependencies(ReleaseAndCopy release_translations)
add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ReleaseAndCopy
        COMMENT "Compiling translation files"
        VERBATIM
)

add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets assets)


if (WIN32)
    find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
    find_program(TOOL_WINDEPLOYQT_DEBUG NAMES windeployqt.debug.bat)

    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND $<IF:$<CONFIG:Debug,RelWithDebInfo>,${TOOL_WINDEPLOYQT_DEBUG},${TOOL_WINDEPLOYQT}> --qmldir "${CMAKE_SOURCE_DIR}/assets/themes/Default" --qmldir "${CMAKE_SOURCE_DIR}/RhythmGameQml"
            $<TARGET_FILE:RhythmGame_exe>
            COMMENT "Running windeployqt..."
            VERBATIM
    )

    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:RhythmGame_exe> $<TARGET_FILE_DIR:RhythmGame_exe>
            COMMAND_EXPAND_LISTS
    )

    # This is only for running in the build directory, before installation.
    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/staticAssets/qt.conf
            ${CMAKE_BINARY_DIR}/bin/$<0:>/qt.conf)
endif ()

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

if (WIN32)
    # Some object files are too big for MSVC in Debug mode
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
