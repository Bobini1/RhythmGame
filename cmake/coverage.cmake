# ---- Variables ----

add_custom_target(
        coverage
        COMMAND llvm-profdata merge ${PROJECT_BINARY_DIR}/test/default.profraw -o ${PROJECT_BINARY_DIR}/test/test.profdata && llvm-cov show ${PROJECT_BINARY_DIR}/RhythmGame -instr-profile=${PROJECT_BINARY_DIR}/test/test.profdata > coverage.info
        COMMENT "Generating coverage report"
        VERBATIM
)
