# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)


# ---- Tests ----

add_executable(RhythmGame_test charts/chart_readers/BmsChartReader.test.cpp charts/chart_readers/ToChars.test.cpp charts/behaviour/SongDataWriter.test.cpp charts/models/BmsChart.test.cpp db/sqlite_cpp_db/SqliteCppDb.test.cpp state_transitions/state_transitions.test.cpp drawing/splashWindow.test.cpp state_transitions/Game.test.cpp)
target_link_libraries(
        RhythmGame_test PRIVATE
        RhythmGame_lib
        Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_test PRIVATE cxx_std_20)
include(Catch)

# MacOS luajit linker flags
if (APPLE)
    set_target_properties(RhythmGame_test PROPERTIES LINK_FLAGS "-pagezero_size 10000 -image_base 100000000")
endif ()

catch_discover_tests(RhythmGame_test)
# ---- End-of-file commands ----

add_folders(Test)
