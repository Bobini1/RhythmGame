# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS QuickTest)


# ---- Tests ----

add_executable(RhythmGame_test
        arena/ArenaBinaryProtocol.test.cpp
        arena/ArenaAvailabilityIndex.test.cpp
        arena/ArenaInventorySource.test.cpp
        arena/ArenaProtocol.test.cpp
        arena/ArenaRoundLoader.test.cpp
        arena/ArenaGameplaySource.test.cpp
        arena/ArenaThemeVars.test.cpp
        arena/ArenaSelectIntegration.test.cpp
        arena/ArenaOverlayPolicy.test.cpp
        arena/ArenaCompetitionModels.test.cpp
        arena/ArenaPacemakerIntegration.test.cpp
        arena/ArenaModels.test.cpp
        arena/ArenaSession.test.cpp
        arena/ArenaSessionMatrix.test.cpp
        arena/ProfileListArenaPolicy.test.cpp
        arena/FakeArenaTransport.h
        arena/FakeArenaIdentityProvider.h
        arena/FakeArenaScheduler.h
        arena/FakeArenaInventorySource.h
        arena/FakeArenaGameplaySource.h
        gameplay_logic/ChartStartGate.test.cpp
        charts/BmsChartReader.test.cpp
        db/SqliteCppDb.test.cpp
        sounds/OpenAlSound.test.cpp
        findTestAssetsFolder.h
        charts/BmsChart.test.cpp
        resource_managers/loadBmsSounds.test.cpp
        resource_managers/ChartPlayConfig.test.cpp
        resource_managers/Lr2FontCache.test.cpp
        resource_managers/ScanThemes.test.cpp
        qml_components/FileQuery.test.cpp
        input/InputTranslator.test.cpp
        support/GeneratePermutation.test.cpp
        lr2_skin/Lr2BarBaseStateResolver.test.cpp
        lr2_skin/Lr2ArenaAvailability.test.cpp
        lr2_skin/Lr2ResolvedTextRegistry.test.cpp
        lr2_skin/Lr2SelectBarModel.test.cpp
        lr2_skin/Lr2SelectDetailState.test.cpp
        lr2_skin/Lr2SkinParser.test.cpp
        lr2_skin/Lr2SkinRuntimeTypes.test.cpp
)
target_link_libraries(
        RhythmGame_test PRIVATE
        RhythmGame_lib
        RhythmGame_lr2_qml
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_test PRIVATE cxx_std_20)
target_compile_definitions(
        RhythmGame_test PRIVATE
        ARENA_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/protocol-v1.json"
        ARENA_PHASE2_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase2-text-goldens.json"
        ARENA_PHASE3_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase3-text-goldens.json"
        ARENA_BINARY_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase2-binary-goldens.json"
        ARENA_QML_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)
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

qt_add_executable(RhythmGame_arena_qml_test
        qml/ArenaQmlTestMain.cpp
)
target_link_libraries(
        RhythmGame_arena_qml_test PRIVATE
        Qt6::QuickTest
        RhythmGame_qmlplugin
)
target_compile_definitions(
        RhythmGame_arena_qml_test PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
target_compile_features(RhythmGame_arena_qml_test PRIVATE cxx_std_20)
set_target_properties(
        RhythmGame_arena_qml_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)
qt_import_qml_plugins(RhythmGame_arena_qml_test)
if (WIN32)
    add_custom_command(TARGET RhythmGame_arena_qml_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:RhythmGame_arena_qml_test>
                    $<TARGET_FILE_DIR:RhythmGame_arena_qml_test>
            COMMAND_EXPAND_LISTS
    )
endif ()
add_custom_command(TARGET RhythmGame_arena_qml_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:RhythmGame_arena_qml_test>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::QMinimalIntegrationPlugin>"
                "$<TARGET_FILE_DIR:RhythmGame_arena_qml_test>/platforms/"
)
add_test(
        NAME ArenaQml
        COMMAND RhythmGame_arena_qml_test
                -input "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                -import "${CMAKE_BINARY_DIR}"
                -import "$<TARGET_FILE_DIR:Qt6::Core>/../qml"
)
set_tests_properties(ArenaQml PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=minimal;QT_QUICK_CONTROLS_STYLE=Basic"
        ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)
# ---- End-of-file commands ----

add_folders(Test)
