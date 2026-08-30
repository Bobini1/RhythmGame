cmake_minimum_required(VERSION 3.21)

foreach(var IN ITEMS PROJECT_BINARY_DIR PROJECT_SOURCE_DIR PROJECT_NAME PROJECT_VERSION)
    if(NOT DEFINED "${var}")
        message(FATAL_ERROR "${var} must be defined")
    endif()
endforeach()

if(NOT DEFINED DOXYGEN_OUTPUT_DIRECTORY)
    set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs")
endif()

find_program(DOXYGEN_EXECUTABLE NAMES doxygen REQUIRED)

set(qdoc_hints "/usr/lib/qt6/bin")
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND qdoc_hints
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin"
    )
endif()
find_program(
    QDOC_EXECUTABLE
    NAMES qdoc qdoc6 qdoc-qt6
    HINTS ${qdoc_hints}
    REQUIRED
)

set(working_dir "${PROJECT_BINARY_DIR}/docs")
set(QDOC_OUTPUT_DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}/html/qml")
file(MAKE_DIRECTORY "${working_dir}")

configure_file(
    "${PROJECT_SOURCE_DIR}/docs/Doxyfile.in"
    "${working_dir}/Doxyfile"
    @ONLY
)
configure_file(
    "${PROJECT_SOURCE_DIR}/docs/RhythmGameQml.qdocconf.in"
    "${working_dir}/RhythmGameQml.qdocconf"
    @ONLY
)
configure_file(
    "${PROJECT_SOURCE_DIR}/docs/qdoc.css"
    "${working_dir}/qdoc.css"
    COPYONLY
)

execute_process(
    COMMAND "${DOXYGEN_EXECUTABLE}" "${working_dir}/Doxyfile"
    WORKING_DIRECTORY "${working_dir}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND
        "${QDOC_EXECUTABLE}"
        -no-link-errors
        -outputdir "${QDOC_OUTPUT_DIRECTORY}"
        "${working_dir}/RhythmGameQml.qdocconf"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)
