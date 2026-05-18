# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)


# ---- Tests ----

add_executable(RhythmGame_test
        charts/BmsChartReader.test.cpp
        db/SqliteCppDb.test.cpp
        sounds/OpenAlSound.test.cpp
        findTestAssetsFolder.h
        charts/BmsChart.test.cpp
        resource_managers/loadBmsSounds.test.cpp
        resource_managers/Lr2FontCache.test.cpp
        resource_managers/ScanThemes.test.cpp
        lr2_skin/Lr2SkinParser.test.cpp
        lr2_skin/Lr2SkinRuntimeTypes.test.cpp
        lr2_skin/Lr2SelectStateCache.test.cpp
        ../RhythmGameQml/Lr2SkinRuntimeTypes.cpp
        ../RhythmGameQml/Lr2SelectBarCell.cpp
        ../RhythmGameQml/Lr2SelectBarWindow.cpp
        ../RhythmGameQml/Lr2SelectVisualState.cpp
        ../RhythmGameQml/Lr2SelectStateCache.cpp
)
target_include_directories(RhythmGame_test PRIVATE
        ${CMAKE_SOURCE_DIR}/RhythmGameQml
)
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

add_custom_command(TARGET RhythmGame_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/testOnlyAssets testOnlyAssets)

if (WIN32)
    add_custom_command(TARGET RhythmGame_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:RhythmGame_test> $<TARGET_FILE_DIR:RhythmGame_test>
            COMMAND_EXPAND_LISTS
    )
endif ()

catch_discover_tests(RhythmGame_test)
# ---- End-of-file commands ----

add_folders(Test)
