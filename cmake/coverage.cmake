# ---- Variables ----

message(STATUS "raw coverage status location: ${PROJECT_BINARY_DIR}/default.profraw")

set(
        SHOW_DIRS
        find ${PROJECT_BINARY_DIR} -name *.profraw
)

set(
        COVERAGE_TRACE_COMMAND
        "${PROJECT_BINARY_DIR}/test/RhythmGame_test"
)

set(
        COVERAGE_MERGE_COMMAND
        llvm-profdata merge -sparse "${PROJECT_BINARY_DIR}/default.profraw" -o "${PROJECT_BINARY_DIR}/test/test.profdata"
        CACHE STRING
        "; separated command to generate a trace for the 'coverage' target"
)

set(
        COVERAGE_SAVE_COMMAND
        llvm-cov show "${PROJECT_BINARY_DIR}/test/RhythmGame_test" -instr-profile="${PROJECT_BINARY_DIR}/test/test.profdata" > coverage.info
        CACHE STRING
        "; separated command to generate an HTML report for the 'coverage' target"
)

# ---- Coverage target ----

add_custom_target(
        coverage
        COMMAND ${COVERAGE_TRACE_COMMAND}
        COMMAND ${SHOW_DIRS}
        COMMAND ${COVERAGE_MERGE_COMMAND}
        COMMAND ${COVERAGE_SAVE_COMMAND}
        COMMENT "Generating coverage report"
        VERBATIM
)
