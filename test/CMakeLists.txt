# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)


# ---- Tests ----

add_executable(RhythmGame_test setupState.h charts/chart_readers/BmsChartReader.test.cpp charts/chart_readers/ToChars.test.cpp charts/behaviour/SongDataWriter.test.cpp charts/models/BmsChart.test.cpp db/SqliteCppDb.test.cpp state_transitions/state_transitions.test.cpp drawing/splashWindow.test.cpp state_transitions/Game.test.cpp drawing/actors/VBox.test.cpp drawing/actors/Hbox.test.cpp lua/LuaVBox.test.cpp drawing/actors/Parent.test.cpp drawing/actors/Actor.test.cpp drawing/actors/Padding.test.cpp drawing/actors/Align.test.cpp drawing/actors/Layers.test.cpp lua/LuaHBox.test.cpp lua/LuaAlign.test.cpp lua/LuaQuad.test.cpp lua/LuaPadding.test.cpp lua/LuaLayers.test.cpp lua/LuaSprite.cpp lua/LuaText.test.cpp events/GlobalEvent.test.cpp drawing/animations/Animation.test.cpp drawing/animations/Linear.test.cpp drawing/animations/AnimationSequence.test.cpp lua/LuaLinear.test.cpp lua/LuaAnimation.test.cpp lua/LuaAnimationSequence.cpp drawing/animations/AnimationPlayerImpl.test.cpp events/MouseClickEvent.test.cpp events/MouseHoverEvents.test.cpp resource_managers/LuaScriptFinder.test.cpp)
target_link_libraries(
        RhythmGame_test PRIVATE
        RhythmGame_lib
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_test PRIVATE cxx_std_20)
set_target_properties(
        RhythmGame_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)
include(Catch)

# boolean cache variable "sanitize"
option(DISABLE_WINDOW_TESTS "Disable tests that launch windows and fail at the sanitizer")
if (DISABLE_WINDOW_TESTS)
    message("Disabling window tests")
    target_compile_definitions(RhythmGame_test PRIVATE DISABLE_WINDOW_TESTS)
endif ()

# MacOS luajit linker flags
if (APPLE)
    set_target_properties(RhythmGame_test PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif ()

add_custom_command(TARGET RhythmGame_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets assets)

catch_discover_tests(RhythmGame_test)
# ---- End-of-file commands ----

add_folders(Test)
