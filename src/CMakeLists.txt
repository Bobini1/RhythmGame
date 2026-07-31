if (WIN32)
    enable_language("RC")
    qt_add_executable(RhythmGame_exe
            ${PROJECT_SOURCE_DIR}/src/main.cpp
            ${PROJECT_SOURCE_DIR}/staticAssets/RhythmGame.rc
    )
    set_target_properties(RhythmGame_exe PROPERTIES
            WIN32_EXECUTABLE $<IF:$<CONFIG:Debug,RelWithDebInfo>,FALSE,TRUE>
            OUTPUT_NAME RhythmGame
            # use main() as entry point
            LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
else ()
    qt_add_executable(RhythmGame_exe
            ${PROJECT_SOURCE_DIR}/src/main.cpp
    )
endif ()
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

qt_add_resources(
        RhythmGame_exe
        "text_files"
        BASE
        "${PROJECT_SOURCE_DIR}"
        FILES
        ${PROJECT_SOURCE_DIR}/ATTRIBUTIONS.md
        ${PROJECT_SOURCE_DIR}/LICENSE.md
)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
        QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS
        "${PROJECT_SOURCE_DIR}/share/RhythmGame/themes/Default"
)

target_compile_features(RhythmGame_exe PRIVATE cxx_std_20)
target_link_libraries(
        RhythmGame_exe
        PRIVATE
        RhythmGame_lib
        RhythmGame_qmlplugin
        RhythmGame_lr2_qmlplugin
        tga
        cim
        dds
)

include(${PROJECT_SOURCE_DIR}/cmake/translations.cmake)

if (RhythmGame_DEVELOPER_MODE)
    if (WIN32)
        find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
        find_program(TOOL_WINDEPLOYQT_DEBUG NAMES windeployqt.debug.bat)
    endif ()
    include(${PROJECT_SOURCE_DIR}/cmake/build-dir-setup.cmake)
endif ()

qt_import_qml_plugins(RhythmGame_exe)
