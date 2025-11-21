qt_policy(SET QTP0001 NEW)
include(ECMQmlModule)

add_library(RhythmGame_qml)
ecm_add_qml_module(RhythmGame_qml URI RhythmGameQml VERSION 1.0
        GENERATE_PLUGIN_SOURCE)
target_sources(RhythmGame_qml PUBLIC Rg.h Rg.cpp)
ecm_target_qml_sources(RhythmGame_qml SOURCES ContentFrame.qml Log.qml ColorChanger.qml)
target_link_libraries(RhythmGame_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
ecm_finalize_qml_module(RhythmGame_qml)

qt6_add_shaders(RhythmGame_qml "shaders"
        PREFIX
        "/"
        FILES
        "ColorChanger.frag"
)