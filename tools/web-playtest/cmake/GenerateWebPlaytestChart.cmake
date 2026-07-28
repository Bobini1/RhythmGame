foreach(required_variable IN ITEMS
        RG_WEB_PLAYTEST_PYTHON
        RG_WEB_PLAYTEST_CHART_GENERATOR
        RG_WEB_PLAYTEST_CHART_DIR
        RG_WEB_PLAYTEST_CHART_RELATIVE_PATH
        RG_WEB_PLAYTEST_CHART_STAGING_DIR
        RG_WEB_PLAYTEST_CHART_MANIFEST_TEMPLATE
        RG_WEB_PLAYTEST_CHART_MANIFEST
        RG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE)
    if(NOT DEFINED "${required_variable}"
            OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR
            "Missing required chart-package input: ${required_variable}")
    endif()
endforeach()

execute_process(
    COMMAND
        "${RG_WEB_PLAYTEST_PYTHON}" -I -B
        "${RG_WEB_PLAYTEST_CHART_GENERATOR}"
        --chart-root "${RG_WEB_PLAYTEST_CHART_DIR}"
        --selected-relative-path
            "${RG_WEB_PLAYTEST_CHART_RELATIVE_PATH}"
        --staging-dir "${RG_WEB_PLAYTEST_CHART_STAGING_DIR}"
        --manifest-template
            "${RG_WEB_PLAYTEST_CHART_MANIFEST_TEMPLATE}"
        --manifest-output "${RG_WEB_PLAYTEST_CHART_MANIFEST}"
        --cmake-output "${RG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE}"
    RESULT_VARIABLE chart_package_result
    OUTPUT_VARIABLE chart_package_output
    ERROR_VARIABLE chart_package_error
)
if(NOT chart_package_result EQUAL 0)
    message(FATAL_ERROR
        "Private chart package validation failed:\n${chart_package_error}")
endif()
include("${RG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE}")
