set(LR2_SKIN_SOURCES
        Lr2/Lr2TimelineState.h
        Lr2/Lr2TimelineStateValue.h
        Lr2/Lr2TimelineState.cpp
        Lr2/Lr2AnimationFrameState.h
        Lr2/Lr2AnimationFrameState.cpp
        Lr2/Lr2GameplayFrameState.h
        Lr2/Lr2GameplayFrameState.cpp
        Lr2/Lr2BarPositionMap.h
        Lr2/Lr2BarPositionMap.cpp
        Lr2/Lr2BarBaseStateResolver.h
        Lr2/Lr2BarBaseStateResolver.cpp
        Lr2/Lr2BarPositionedItem.h
        Lr2/Lr2BarPositionedItem.cpp
        Lr2/Lr2BarTextItem.h
        Lr2/Lr2BarTextItem.cpp
        Lr2/Lr2BarInterpolatedState.h
        Lr2/Lr2BarInterpolatedState.cpp
        Lr2/Lr2SelectVisualState.h
        Lr2/Lr2SelectVisualState.cpp
        Lr2/Lr2SelectNavigationController.h
        Lr2/Lr2SelectNavigationController.cpp
        Lr2/Lr2SelectItemModel.h
        Lr2/Lr2SelectItemModel.cpp
        Lr2/Lr2SelectBarModel.h
        Lr2/Lr2SelectBarModel.cpp
        Lr2/Lr2SelectDetailState.h
        Lr2/Lr2SelectDetailState.cpp
        Lr2/Lr2ChartDataSnapshot.h
        Lr2/Lr2ChartDataSnapshot.cpp
        Lr2/Lr2SelectKeyNavigationFilter.h
        Lr2/Lr2SelectKeyNavigationFilter.cpp
        Lr2/Lr2SkinOptionRules.h
        Lr2/Lr2SkinOptionRules.cpp
        Lr2/Lr2SelectUpdateController.h
        Lr2/Lr2SelectUpdateController.cpp
        Lr2/Lr2SelectBarCell.h
        Lr2/Lr2SelectBarCell.cpp
        Lr2/Lr2SkinClock.h
        Lr2/Lr2SkinClock.cpp
        Lr2/Lr2SkinFrameDriver.h
        Lr2/Lr2SkinFrameDriver.cpp
        Lr2/Lr2SelectInfoTimerBridge.h
        Lr2/Lr2SelectInfoTimerBridge.cpp
        Lr2/Lr2SkinTimerState.h
        Lr2/Lr2SkinTimerState.cpp
        Lr2/Lr2SkinElementTimerState.h
        Lr2/Lr2SkinElementTimerState.cpp
        Lr2/Lr2SkinElementActiveOptionsState.h
        Lr2/Lr2SkinElementActiveOptionsState.cpp
        Lr2/Lr2SkinElementDescriptorValue.h
        Lr2/Lr2SkinRuntimeTypes.h
        Lr2/Lr2SkinRuntimeTypes.cpp
        Lr2/Lr2SkinRuntime.h
        Lr2/Lr2SkinRuntime.cpp
        Lr2/Lr2SkinSliderGeometry.h
        Lr2/Lr2SkinSliderGeometry.cpp
        Lr2/Lr2BarDistributionGraphItem.h
        Lr2/Lr2BarDistributionGraphItem.cpp
        Lr2/Lr2BitmapFontTexture.h
        Lr2/Lr2BitmapFontTexture.cpp
)

set(LR2_SKIN_QML_FILES
        Lr2/Lr2SkinUtils.js
        Lr2/Lr2TimelineFrame.qml
        Lr2/Lr2SkinScreenWrapper.qml
        Lr2/Lr2SkinScene.qml
        Lr2/Lr2SkinElementDelegate.qml
        Lr2/Lr2SelectPointerSurface.qml
        Lr2/Lr2SkinValueResolver.qml
        Lr2/Lr2SkinTiming.qml
        Lr2/Lr2WallClockState.qml
        Lr2/Lr2ScreenState.qml
        Lr2/Lr2ResultState.qml
        Lr2/Lr2RankingState.qml
        Lr2/Lr2SkinSettingsState.qml
        Lr2/Lr2OptionState.qml
        Lr2/Lr2SelectPanelController.qml
        Lr2/Lr2SkinSliderState.qml
        Lr2/Lr2SelectSearchState.qml
        Lr2/Lr2SelectBarGeometry.qml
        Lr2/Lr2SelectPointerController.qml
        Lr2/Lr2ReadmeState.qml
        Lr2/Lr2ReadmeTextElement.qml
        Lr2/Lr2ReadmePointerArea.qml
        Lr2/Lr2SelectHoverState.qml
        Lr2/Lr2SelectSideEffects.qml
        Lr2/Lr2RuntimeOptionBuilder.qml
        Lr2/Lr2SpriteRenderer.qml
        Lr2/Lr2FastSprite.qml
        Lr2/Lr2SkinCursor.qml
        Lr2/Lr2BgaRenderer.qml
        Lr2/Lr2TextElement.qml
        Lr2/Lr2TextRenderer.qml
        Lr2/Lr2BitmapFontText.qml
        Lr2/Lr2SystemFontText.qml
        Lr2/Lr2SelectContext.qml
        Lr2/Lr2PlayContext.qml
        Lr2/Lr2PlayNoteField.qml
        Lr2/Lr2GrooveGaugeRenderer.qml
        Lr2/Lr2NumberRenderer.qml
        Lr2/Lr2BarSpriteRenderer.qml
        Lr2/Lr2BarTextRenderer.qml
        Lr2/Lr2BarNumberRenderer.qml
        Lr2/Lr2BarGraphRenderer.qml
        Lr2/Lr2ResultChartRenderer.qml
        Lr2/Lr2NoteChartRenderer.qml
        Lr2/Lr2BpmChartRenderer.qml
)

set(LR2_SKIN_SHADERS
        Lr2/Lr2AddBlend.frag
        Lr2/Lr2SpriteAtlas.frag
        Lr2/Lr2BarDistributionGraph.vert
        Lr2/Lr2BarDistributionGraph.frag
        Lr2/Lr2InvertBlend.frag
)

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

foreach(lr2_qml_file IN LISTS LR2_SKIN_QML_FILES)
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
        ${LR2_SKIN_SOURCES}
        QML_FILES
        ${LR2_SKIN_QML_FILES}
)

qt6_add_shaders(RhythmGame_lr2_qml "lr2_shaders"
        BATCHABLE
        PREFIX
        "/"
        FILES
        ${LR2_SKIN_SHADERS}
)
