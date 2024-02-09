# ---- Declare documentation target ----

find_package(Doxygen)

if(NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen not found, docs target won't be available")
    return()
endif ()

set(
    DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated Doxygen documentation"
)

set(working_dir "${PROJECT_BINARY_DIR}/docs")

configure_file("docs/Doxyfile.in" "${working_dir}/Doxyfile" @ONLY)

add_custom_target(
    docs
    COMMAND ${DOXYGEN_EXECUTABLE} "${working_dir}/Doxyfile"
    WORKING_DIRECTORY ${working_dir}
    COMMENT "Generating Doxygen documentation"
    VERBATIM
)
