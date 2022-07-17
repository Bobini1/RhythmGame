# ---- Variables ----

set(
        COVERAGE_TRACE_COMMAND
        "${PROJECT_BINARY_DIR}/test/RhythmGame_test"
        CACHE STRING
        "; tracing for the 'coverage' target"
)

set(
        COVERAGE_MERGE_COMMAND
        llvm-profdata merge -sparse "${PROJECT_BINARY_DIR}/test/default.profraw" "${PROJECT_BINARY_DIR}/default.profraw" -o "${PROJECT_BINARY_DIR}/test/test.profdata"
        CACHE STRING
        "; merging traces for the 'coverage' target"
)

set(
        COVERAGE_SAVE_COMMAND
        "llvm-cov show ${PROJECT_BINARY_DIR}/RhythmGame -instr-profile=${PROJECT_BINARY_DIR}/test/test.profdata"
        CACHE STRING
        "; save report for the 'coverage' target"
)

# ---- Coverage target ----

add_custom_target(
        coverage
        COMMAND ${COVERAGE_TRACE_COMMAND}
        COMMAND ${COVERAGE_MERGE_COMMAND}
        COMMAND ${COVERAGE_SAVE_COMMAND}
        COMMENT "Generating coverage report"
)
