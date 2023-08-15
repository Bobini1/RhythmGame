qt_add_library(RhythmGame_qml STATIC
        ChartData.cpp
        ChartData.h
        ChartLoader.cpp
        ChartLoader.h)
target_link_libraries(RhythmGame_qml PUBLIC RhythmGame_lib)
target_include_directories(RhythmGame_qml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
qt_policy(SET QTP0001 NEW)
qt_add_qml_module(RhythmGame_qml URI RhythmGameQml VERSION 1.0 SOURCES SceneUrls.cpp SceneUrls.h
        SOURCES ChartList.h ChartList.cpp QML_FILES ContentFrame.qml)

