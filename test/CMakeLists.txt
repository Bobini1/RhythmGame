# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(RhythmGameTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 3 REQUIRED)


# ---- Tests ----

message("${Catch2_LIBRARIES}")

add_executable(RhythmGame_test source/RhythmGame_test.cpp)
target_link_libraries(
    RhythmGame_test PRIVATE
    RhythmGame_lib
    Catch2::Catch2WithMain
)
target_compile_features(RhythmGame_test PRIVATE cxx_std_20)
include(Catch)

catch_discover_tests(RhythmGame_test)
# ---- End-of-file commands ----t

add_folders(Test)
