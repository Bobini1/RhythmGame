file(GLOB_RECURSE QML_FILES "${CMAKE_SOURCE_DIR}/data/themes/Default/**/*.qml")
qt_add_translations(
        RhythmGame_exe
        TS_FILE_BASE
        Default
        TS_FILE_DIR
        ${CMAKE_SOURCE_DIR}/data/themes/Default/translations
        SOURCES
        ${QML_FILES}
        LUPDATE_TARGET
        Default_translations
        QM_FILES_OUTPUT_VARIABLE
        LRELEASE_QM_FILES
        QM_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/data/themes/Default/translations
)
option(BUILD_TRANSLATIONS "Build translation files (qm) together with the game" ON)
if (BUILD_TRANSLATIONS)
    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target release_translations
            COMMENT "Compiling translation files"
            VERBATIM
    )
endif ()
