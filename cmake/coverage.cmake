
add_custom_target(
        coverage
        COMMAND lcov -c -q -o "${PROJECT_BINARY_DIR}/coverage.info" -d "${PROJECT_BINARY_DIR}" --include "${PROJECT_SOURCE_DIR}/*" && genhtml --legend -f -q "${PROJECT_BINARY_DIR}/coverage.info" -p "${PROJECT_SOURCE_DIR}" -o "${PROJECT_BINARY_DIR}/coverage_html"
        COMMENT "Generating coverage report"
        VERBATIM
)