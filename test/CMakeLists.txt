# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)


# ---- Tests ----

add_executable(RhythmGame_test charts/BmsChartReader.test.cpp db/SqliteCppDb.test.cpp sounds/OpenAlSound.test.cpp findTestAssetsFolder.h charts/BmsChart.test.cpp resource_managers/loadBmsSounds.test.cpp)
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
    find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
    find_program(TOOL_WINDEPLOYQT_DEBUG NAMES windeployqt.debug.bat)

    add_custom_command(TARGET RhythmGame_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/bin/qml" "${PROJECT_BINARY_DIR}/bin"
    )
endif()

catch_discover_tests(RhythmGame_test)
# ---- End-of-file commands ----

add_folders(Test)
