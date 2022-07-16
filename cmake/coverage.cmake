# ---- Variables ----

message(STATUS "raw coverage status location: ${PROJECT_BINARY_DIR}/test.profraw")

set(
        SHOW_DIRS
        ls -R
)

set(
        COVERAGE_TRACE_COMMAND
        LLVM_PROFILE_FILE="${PROJECT_BINARY_DIR}/test.profraw" "${PROJECT_BINARY_DIR}/RhythmGame_test"
)


set(
        COVERAGE_MERGE_COMMAND
        llvm-profdata merge -sparse "${PROJECT_BINARY_DIR}/test.profraw" -o "${PROJECT_BINARY_DIR}/test.profdata"
        CACHE STRING
        "; separated command to generate a trace for the 'coverage' target"
)

set(
        COVERAGE_SAVE_COMMAND
        llvm-cov show "${PROJECT_BINARY_DIR}/RhythmGame_test" -instr-profile="${PROJECT_BINARY_DIR}/test.profdata" > coverage.info
        CACHE STRING
        "; separated command to generate an HTML report for the 'coverage' target"
)

# ---- Coverage target ----

add_custom_target(
        coverage
        COMMAND ${SHOW_DIRS}
        COMMAND ${COVERAGE_TRACE_COMMAND}
        COMMAND ${COVERAGE_MERGE_COMMAND}
        COMMAND ${COVERAGE_SAVE_COMMAND}
        COMMENT "Generating coverage report"
        VERBATIM
)
