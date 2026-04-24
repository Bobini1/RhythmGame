qt_add_library(RhythmGame_qml STATIC)
target_link_libraries(RhythmGame_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
qt_policy(SET QTP0001 NEW)
qt_add_qml_module(RhythmGame_qml URI RhythmGameQml VERSION 1.0 SOURCES Rg.h Rg.cpp
        QML_FILES ContentFrame.qml Log.qml ColorChanger.qml Lr2SkinScreenWrapper.qml Lr2SpriteRenderer.qml Lr2FastSprite.qml Lr2BgaRenderer.qml Lr2TextRenderer.qml Lr2BitmapFontText.qml Lr2SystemFontText.qml Lr2SelectContext.qml Lr2PlayContext.qml Lr2PlayNoteField.qml Lr2GrooveGaugeRenderer.qml Lr2NumberRenderer.qml Lr2BarSpriteRenderer.qml Lr2BarTextRenderer.qml Lr2BarNumberRenderer.qml Lr2BarGraphRenderer.qml
        RESOURCES Lr2Timeline.js)
qt6_add_shaders(RhythmGame_qml "shaders"
        PREFIX
        "/"
        FILES
        "ColorChanger.frag"
        "Lr2AddBlend.frag"
        "Lr2SpriteAtlas.frag"
        "Lr2InvertBlend.frag"
        "Lr2Tint.frag"
)
