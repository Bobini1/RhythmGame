# ---- Variables ----

add_custom_target(
        coverage
        COMMAND llvm-profdata-12 merge ${PROJECT_BINARY_DIR}/test/default.profraw -o ${PROJECT_BINARY_DIR}/test/test.profdata && llvm-cov-12 show ${PROJECT_BINARY_DIR}/RhythmGame --instr-profile=${PROJECT_BINARY_DIR}/test/test.profdata > coverage.info
        COMMENT "Generating coverage report"
        VERBATIM
)
