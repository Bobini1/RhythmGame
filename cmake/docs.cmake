# ---- Declare the unified documentation target ----

set(
    DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated documentation"
)

add_custom_target(
    docs
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}"
        "-DPROJECT_NAME=${PROJECT_NAME}"
        "-DPROJECT_VERSION=${PROJECT_VERSION}"
        "-DDOXYGEN_OUTPUT_DIRECTORY=${DOXYGEN_OUTPUT_DIRECTORY}"
        "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
        "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
        -P "${PROJECT_SOURCE_DIR}/cmake/generate-docs.cmake"
    COMMENT "Generating C++ and QML documentation"
    VERBATIM
)
