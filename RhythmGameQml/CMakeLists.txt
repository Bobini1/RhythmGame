qt_add_library(RhythmGame_qml STATIC)
target_link_libraries(RhythmGame_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
qt_policy(SET QTP0001 NEW)
qt_add_qml_module(RhythmGame_qml URI RhythmGameQml VERSION 1.0 SOURCES Rg.h Rg.cpp
        QML_FILES ContentFrame.qml Log.qml ColorChanger.qml)
qt6_add_shaders(RhythmGame_qml "shaders"
        PREFIX
        "/"
        FILES
        "ColorChanger.frag"
)