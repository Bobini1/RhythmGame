foreach (required_variable
        SOURCE_ROOT
        TEST_BINARY_ROOT
        NINJA_PROGRAM)
    if (NOT DEFINED "${required_variable}" OR
            "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif ()
endforeach ()

cmake_path(NORMAL_PATH TEST_BINARY_ROOT)
string(REPLACE "\\" "/" normalized_test_binary_root
        "${TEST_BINARY_ROOT}")
if (NOT normalized_test_binary_root MATCHES
        "/web-audio-core-pthread-audit-tests$")
    message(FATAL_ERROR
            "TEST_BINARY_ROOT must end in "
            "web-audio-core-pthread-audit-tests before recursive cleanup")
endif ()

file(REMOVE_RECURSE "${TEST_BINARY_ROOT}")
file(MAKE_DIRECTORY "${TEST_BINARY_ROOT}")

set(case_script
        "${SOURCE_ROOT}/test/cmake/web_audio_core_pthread_audit/RunCase.cmake")
set(success_cases
        response_and_direct
        authenticated_launcher)
set(failure_cases
        missing_pthread
        consumed_pthread
        launcher_pthread_only
        launcher_missing_delimiter
        launcher_multiple_driver_kind
        launcher_multiple_adapter
        launcher_missing_adapter
        launcher_posix_uppercase_adapter
        launcher_posix_uppercase_driver
        launcher_early_boundary
        launcher_ambiguous_boundary
        launcher_wrong_driver
        launcher_wrong_driver_kind
        launcher_missing_driver_kind
        launcher_compiler_terminator
        launcher_consumed_pthread
        launcher_shape_drift
        outside_response
        malformed_response
        response_directory
        invalid_json
        schema_drift)
set(optional_failure_cases
        response_symlink
        response_parent_symlink)

foreach (audit_case IN LISTS
        success_cases
        failure_cases
        optional_failure_cases)
    execute_process(
            COMMAND
            "${CMAKE_COMMAND}"
            "-DSOURCE_ROOT=${SOURCE_ROOT}"
            "-DCASE_ROOT=${TEST_BINARY_ROOT}/${audit_case}"
            "-DNINJA_PROGRAM=${NINJA_PROGRAM}"
            "-DAUDIT_CASE=${audit_case}"
            -P "${case_script}"
            RESULT_VARIABLE case_result
            OUTPUT_VARIABLE case_stdout
            ERROR_VARIABLE case_stderr
            ENCODING UTF-8
    )
    set(case_output "${case_stdout}\n${case_stderr}")
    if (audit_case IN_LIST success_cases)
        if (NOT case_result EQUAL 0)
            message(FATAL_ERROR
                    "${audit_case} unexpectedly failed:\n${case_output}")
        endif ()
        continue()
    endif ()
    if (audit_case IN_LIST optional_failure_cases AND
            case_result EQUAL 0 AND
            case_output MATCHES "SKIP ${audit_case}")
        continue()
    endif ()

    if (case_result EQUAL 0)
        message(FATAL_ERROR "${audit_case} unexpectedly passed")
    endif ()
    if (audit_case STREQUAL "missing_pthread" AND
            NOT case_output MATCHES
            "audio compile command omits effective expanded.*-pthread.*b[.]cpp[.]o")
        message(FATAL_ERROR
                "missing_pthread failed without the per-object diagnostic:\n"
                "${case_output}")
    elseif (audit_case STREQUAL "consumed_pthread" AND
            NOT case_output MATCHES
            "audio compile command omits effective expanded.*-pthread.*b[.]cpp[.]o")
        message(FATAL_ERROR
                "consumed_pthread failed without the effective-option "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_pthread_only" AND
            NOT case_output MATCHES
            "audio compile command omits effective expanded.*-pthread.*a[.]cpp[.]o")
        message(FATAL_ERROR
                "launcher_pthread_only failed without the compiler-argument "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_missing_delimiter" AND
            NOT case_output MATCHES
            "authenticated launcher must place -- immediately after --driver-kind em[+][+]")
        message(FATAL_ERROR
                "launcher_missing_delimiter failed without the boundary "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_multiple_driver_kind" AND
            NOT case_output MATCHES
            "authenticated launcher has multiple --driver-kind options")
        message(FATAL_ERROR
                "launcher_multiple_driver_kind failed without the "
                "multiplicity diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_multiple_adapter" AND
            NOT case_output MATCHES
            "authenticated launcher has multiple invoke_emscripten_driver[.]py arguments")
        message(FATAL_ERROR
                "launcher_multiple_adapter failed without the multiplicity "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_missing_adapter" AND
            NOT case_output MATCHES
            "has --driver-kind without the authenticated invoke_emscripten_driver[.]py")
        message(FATAL_ERROR
                "launcher_missing_adapter failed without the adapter "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_posix_uppercase_adapter" AND
            NOT case_output MATCHES
            "has --driver-kind without the authenticated invoke_emscripten_driver[.]py")
        message(FATAL_ERROR
                "launcher_posix_uppercase_adapter failed without the "
                "case-sensitive adapter diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_posix_uppercase_driver" AND
            NOT case_output MATCHES
            "authenticated launcher compiler is not em[+][+]")
        message(FATAL_ERROR
                "launcher_posix_uppercase_driver failed without the "
                "case-sensitive driver diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_early_boundary" AND
            NOT case_output MATCHES
            "authenticated launcher has an ambiguous compiler boundary")
        message(FATAL_ERROR
                "launcher_early_boundary failed without the boundary "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_ambiguous_boundary" AND
            NOT case_output MATCHES
            "authenticated launcher has an ambiguous compiler boundary")
        message(FATAL_ERROR
                "launcher_ambiguous_boundary failed without the boundary "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_wrong_driver" AND
            NOT case_output MATCHES
            "authenticated launcher compiler is not em[+][+]")
        message(FATAL_ERROR
                "launcher_wrong_driver failed without the driver "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_wrong_driver_kind" AND
            NOT case_output MATCHES
            "authenticated launcher --driver-kind must be em[+][+]")
        message(FATAL_ERROR
                "launcher_wrong_driver_kind failed without the kind value "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_missing_driver_kind" AND
            NOT case_output MATCHES
            "authenticated launcher is missing --driver-kind")
        message(FATAL_ERROR
                "launcher_missing_driver_kind failed without the kind "
                "diagnostic:\n${case_output}")
    elseif (audit_case MATCHES
            "^launcher_(compiler_terminator|consumed_pthread)$" AND
            NOT case_output MATCHES
            "audio compile command omits effective expanded.*-pthread.*b[.]cpp[.]o")
        message(FATAL_ERROR
                "${audit_case} failed without the effective compiler-option "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "launcher_shape_drift" AND
            NOT case_output MATCHES
            "raw/expanded authenticated-launcher shape differs")
        message(FATAL_ERROR
                "launcher_shape_drift failed without the database-shape "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "outside_response" AND
            NOT case_output MATCHES
            "response file is not a strict descendant")
        message(FATAL_ERROR
                "outside_response failed without the containment "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "malformed_response" AND
            NOT case_output MATCHES "malformed response-file argument")
        message(FATAL_ERROR
                "malformed_response failed without the response syntax "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "response_directory" AND
            NOT case_output MATCHES
            "response path exists but is not a regular file")
        message(FATAL_ERROR
                "response_directory failed without the file-type "
                "diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "response_symlink" AND
            NOT case_output MATCHES
            "response path (exists but is not a regular file|resolves through a symlink or reparse point)")
        message(FATAL_ERROR
                "response_symlink failed without the reparse diagnostic:\n"
                "${case_output}")
    elseif (audit_case STREQUAL "response_parent_symlink" AND
            NOT case_output MATCHES
            "response file resolves through a symlink or reparse point")
        message(FATAL_ERROR
                "response_parent_symlink failed without the ancestor "
                "reparse diagnostic:\n${case_output}")
    elseif (audit_case STREQUAL "invalid_json" AND
            NOT case_output MATCHES "raw compilation database is invalid")
        message(FATAL_ERROR
                "invalid_json failed without the JSON diagnostic:\n"
                "${case_output}")
    elseif (audit_case STREQUAL "schema_drift" AND
            NOT case_output MATCHES
            "raw compilation database record 0 lacks string field command")
        message(FATAL_ERROR
                "schema_drift failed without the schema diagnostic:\n"
                "${case_output}")
    endif ()
endforeach ()

message(STATUS
        "Portable audio pthread compilation-database contracts passed")
