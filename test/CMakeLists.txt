# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)
include(Catch)
# ---- Tests ----

add_executable(RhythmGame_test
        arena/ArenaBinaryProtocol.test.cpp
        arena/ArenaAvailabilityIndex.test.cpp
        arena/ArenaInventorySource.test.cpp
        arena/ArenaProtocol.test.cpp
        arena/ArenaRoundLoader.test.cpp
        arena/ArenaGameplaySource.test.cpp
        arena/ArenaCompetitionModels.test.cpp
        arena/ArenaModels.test.cpp
        arena/ArenaSession.test.cpp
        arena/ArenaSessionCompetition.test.cpp
        arena/ArenaSessionMatrix.test.cpp
        arena/ProfileListArenaPolicy.test.cpp
        arena/FakeArenaTransport.h
        arena/FakeArenaIdentityProvider.h
        arena/FakeArenaScheduler.h
        arena/FakeArenaInventorySource.h
        arena/FakeArenaGameplaySource.h
        gameplay_logic/ChartStartGate.test.cpp
        gameplay_logic/ScheduledSound.test.cpp
        gameplay_logic/SinglePlayerChartBuilder.test.cpp
        charts/BmsChartReader.test.cpp
        db/SqliteCppDb.test.cpp
        sounds/OpenAlSound.test.cpp
        findTestAssetsFolder.h
        charts/BmsChart.test.cpp
        resource_managers/loadBmsSounds.test.cpp
        resource_managers/ChartDataFactory.test.cpp
        resource_managers/ChartPlayConfig.test.cpp
        resource_managers/Lr2FontCache.test.cpp
        resource_managers/Languages.test.cpp
        resource_managers/ScanThemes.test.cpp
        qml_components/FileQuery.test.cpp
        qml_components/FontResolver.test.cpp
        qml_components/OnlineScores.test.cpp
        qml_components/ScoreDbAsyncApi.test.cpp
        input/InputTranslator.test.cpp
        support/GeneratePermutation.test.cpp
        support/PendingReply.test.cpp
        lr2_skin/Lr2BarBaseStateResolver.test.cpp
        lr2_skin/Lr2ResolvedTextRegistry.test.cpp
        lr2_skin/Lr2SelectBarModel.test.cpp
        lr2_skin/Lr2SelectDetailState.test.cpp
        lr2_skin/Lr2SkinParser.test.cpp
        lr2_skin/Lr2SkinRuntimeTypes.test.cpp
)

add_executable(RhythmGame_web_gameplay_core_test
        gameplay_logic/SinglePlayerGameplayCore.test.cpp
)
target_link_libraries(RhythmGame_web_gameplay_core_test PRIVATE
        RhythmGame_web_gameplay_core
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_web_gameplay_core_test PRIVATE cxx_std_23)
target_compile_definitions(
        RhythmGame_web_gameplay_core_test PRIVATE
        RHYTHMGAME_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
set_target_properties(
        RhythmGame_web_gameplay_core_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)
add_dependencies(
        RhythmGame_web_gameplay_core_test
        RhythmGame_web_gameplay_core_source_contract
)
add_executable(RhythmGame_web_audio_core_test
        web_playtest/RealtimeMixer.test.cpp
        resource_managers/BmsAssetResolver.test.cpp
)
target_link_libraries(RhythmGame_web_audio_core_test PRIVATE
        RhythmGame_web_audio_core
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_web_audio_core_test PRIVATE cxx_std_23)
target_compile_definitions(
        RhythmGame_web_audio_core_test PRIVATE
        RHYTHMGAME_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
set_target_properties(
        RhythmGame_web_audio_core_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)
add_dependencies(
        RhythmGame_web_audio_core_test
        RhythmGame_web_audio_core_source_contract
)
add_library(PortableGameplayHeaders_compile OBJECT
        resource_managers/PortableGameplayHeaders.compile.cpp
)
target_include_directories(PortableGameplayHeaders_compile BEFORE PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/portable_header_checks"
)
target_link_libraries(
        PortableGameplayHeaders_compile PRIVATE
        RhythmGame_web_gameplay_core
)
target_compile_features(PortableGameplayHeaders_compile PRIVATE cxx_std_23)
add_dependencies(
        RhythmGame_test
        PortableGameplayHeaders_compile
        RhythmGame_web_audio_core_test
)

target_link_libraries(
        RhythmGame_test PRIVATE
        RhythmGame_lib
        RhythmGame_qml
        RhythmGame_lr2_qml
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_test PRIVATE cxx_std_20)
target_compile_definitions(
        RhythmGame_test PRIVATE
        RHYTHMGAME_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        ARENA_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/protocol-v1.json"
        ARENA_PHASE2_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase2-text-goldens.json"
        ARENA_PHASE3_PROTOCOL_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase3-text-goldens.json"
        ARENA_BINARY_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/arena/fixtures/phase2-binary-goldens.json"
)
set_target_properties(
        RhythmGame_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
)

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
    add_custom_command(TARGET RhythmGame_web_audio_core_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:RhythmGame_web_audio_core_test>
            $<TARGET_FILE_DIR:RhythmGame_web_audio_core_test>
            COMMAND_EXPAND_LISTS
    )
    add_custom_command(TARGET RhythmGame_web_gameplay_core_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:RhythmGame_web_gameplay_core_test>
            $<TARGET_FILE_DIR:RhythmGame_web_gameplay_core_test>
            COMMAND_EXPAND_LISTS
    )
    add_custom_command(TARGET RhythmGame_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:RhythmGame_test> $<TARGET_FILE_DIR:RhythmGame_test>
            COMMAND_EXPAND_LISTS
    )
    add_custom_command(TARGET RhythmGame_test POST_BUILD
            COMMAND $<IF:$<CONFIG:Debug>,${TOOL_WINDEPLOYQT_DEBUG},${TOOL_WINDEPLOYQT}> $<$<CONFIG:Debug,RelWithDebInfo>:--pdb> --no-translations --no-compiler-runtime
            $<TARGET_FILE:RhythmGame_test>
            COMMENT "Running windeployqt for tests..."
            COMMAND_EXPAND_LISTS
            VERBATIM
    )
endif ()

catch_discover_tests(RhythmGame_web_gameplay_core_test)
catch_discover_tests(
        RhythmGame_web_audio_core_test
        TEST_PREFIX "RealtimeMixer.BmsAssetResolver."
)
catch_discover_tests(RhythmGame_test)
add_test(
        NAME WebAudioCore_source_contract
        COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
        "-DSOURCE_MANIFEST=${CMAKE_BINARY_DIR}/RhythmGame_web_audio_core-sources.txt"
        -P "${CMAKE_SOURCE_DIR}/cmake/CheckWebAudioCore.cmake"
)
add_test(
        NAME WebGameplayCore_source_contract
        COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
        "-DSOURCE_MANIFEST=${CMAKE_BINARY_DIR}/RhythmGame_web_gameplay_core-sources.txt"
        -P "${CMAKE_SOURCE_DIR}/cmake/CheckWebGameplayCore.cmake"
)
if (CMAKE_GENERATOR MATCHES "^Ninja")
    add_test(
            NAME WebAudioCore_negative_dependency_contracts
            COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
            "-DTEST_BINARY_ROOT=${CMAKE_BINARY_DIR}/web-audio-core-negative-contracts"
            "-DNINJA_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DDEPENDENCY_PREFIX=${CMAKE_PREFIX_PATH}"
            "-DNESTED_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DNESTED_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_INSTALLED_DIR=${_VCPKG_INSTALLED_DIR}"
            "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
            "-DVCPKG_HOST_TRIPLET=${VCPKG_HOST_TRIPLET}"
            -P
            "${CMAKE_SOURCE_DIR}/cmake/RunWebAudioCoreNegativeContractTests.cmake"
    )
    add_test(
            NAME WebGameplayCore_negative_dependency_contracts
            COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
            "-DTEST_BINARY_ROOT=${CMAKE_BINARY_DIR}/web-gameplay-core-negative-contracts"
            "-DNINJA_PROGRAM=${CMAKE_MAKE_PROGRAM}"
            "-DDEPENDENCY_PREFIX=${CMAKE_PREFIX_PATH}"
            "-DNESTED_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DNESTED_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
            "-DVCPKG_INSTALLED_DIR=${_VCPKG_INSTALLED_DIR}"
            "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
            "-DVCPKG_HOST_TRIPLET=${VCPKG_HOST_TRIPLET}"
            -P
            "${CMAKE_SOURCE_DIR}/cmake/RunWebGameplayCoreNegativeContractTests.cmake"
    )
endif ()
# ---- End-of-file commands ----

add_folders(Test)
