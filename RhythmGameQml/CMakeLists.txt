include(Lr2/lr2_skin_sources.cmake)

set(RHYTHM_GAME_QML_SOURCES
        Rg.h
        Rg.cpp
        QmlForeignTypes.h
)

set(RHYTHM_GAME_QML_FILES
        ContentFrame.qml
        Log.qml
        ColorChanger.qml
)

set(RHYTHM_GAME_QML_SHADERS
        ColorChanger.frag
)

qt_add_library(RhythmGame_qml STATIC)
target_link_libraries(RhythmGame_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_qml PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)
qt_add_qml_module(RhythmGame_qml
        URI RhythmGameQml
        VERSION 1.0
        SOURCES
        ${RHYTHM_GAME_QML_SOURCES}
        QML_FILES
        ${RHYTHM_GAME_QML_FILES}
)

qt6_add_shaders(RhythmGame_qml "shaders"
        BATCHABLE
        PREFIX
        "/"
        FILES
        ${RHYTHM_GAME_QML_SHADERS}
)

function(lr2_make_absolute_files out_var)
    set(absolute_files)
    foreach(relative_file IN LISTS ARGN)
        list(APPEND absolute_files "${CMAKE_CURRENT_SOURCE_DIR}/${relative_file}")
    endforeach()
    set(${out_var} ${absolute_files} PARENT_SCOPE)
endfunction()

lr2_make_absolute_files(LR2_SKIN_SOURCES_ABSOLUTE ${LR2_SKIN_SOURCES})
lr2_make_absolute_files(LR2_SKIN_QML_FILES_ABSOLUTE ${LR2_SKIN_QML_FILES})

foreach(lr2_qml_file IN LISTS LR2_SKIN_QML_FILES_ABSOLUTE)
    get_filename_component(lr2_qml_alias "${lr2_qml_file}" NAME)
    set_source_files_properties("${lr2_qml_file}" PROPERTIES
            QT_RESOURCE_ALIAS "${lr2_qml_alias}"
    )
endforeach()

qt_add_library(RhythmGame_lr2_qml STATIC)
target_link_libraries(RhythmGame_lr2_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_lr2_qml PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Lr2
)

qt_add_qml_module(RhythmGame_lr2_qml
        URI RhythmGameQml.Lr2
        VERSION 1.0
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Lr2"
        DEPENDENCIES
        TARGET RhythmGame_qml
        SOURCES
        ${LR2_SKIN_SOURCES_ABSOLUTE}
        QML_FILES
        ${LR2_SKIN_QML_FILES_ABSOLUTE}
)

qt6_add_shaders(RhythmGame_lr2_qml "lr2_shaders"
        BATCHABLE
        PREFIX
        "/"
        FILES
        ${LR2_SKIN_SHADERS}
)
