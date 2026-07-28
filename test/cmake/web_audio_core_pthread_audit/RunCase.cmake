foreach (required_variable
        SOURCE_ROOT
        CASE_ROOT
        NINJA_PROGRAM
        AUDIT_CASE)
    if (NOT DEFINED "${required_variable}" OR
            "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif ()
endforeach ()

cmake_path(NORMAL_PATH CASE_ROOT)
string(REPLACE "\\" "/" normalized_case_root "${CASE_ROOT}")
if (NOT normalized_case_root MATCHES
        "/web-audio-core-pthread-audit-tests/[^/]+$")
    message(FATAL_ERROR
            "CASE_ROOT must be a direct child of "
            "web-audio-core-pthread-audit-tests")
endif ()

set(BINARY_DIR "${CASE_ROOT}/build")
set(TARGET_NAME audio)
set(relative_target_objects
        CMakeFiles/audio.dir/a.cpp.o
        CMakeFiles/audio.dir/b.cpp.o)
set(TARGET_OBJECTS)
foreach (relative_object IN LISTS relative_target_objects)
    cmake_path(
            ABSOLUTE_PATH relative_object
            BASE_DIRECTORY "${BINARY_DIR}"
            NORMALIZE
            OUTPUT_VARIABLE absolute_object)
    list(APPEND TARGET_OBJECTS "${absolute_object}")
endforeach ()
list(LENGTH TARGET_OBJECTS TARGET_OBJECT_COUNT)

file(MAKE_DIRECTORY "${BINARY_DIR}")

if (AUDIT_CASE STREQUAL "invalid_json")
    set(RAW_COMPDB_JSON "{not-json")
    set(EXPANDED_COMPDB_JSON "[]")
elseif (AUDIT_CASE STREQUAL "schema_drift")
    string(CONCAT RAW_COMPDB_JSON
            "[{\"directory\":\"${BINARY_DIR}\","
            "\"file\":\"a.cpp\","
            "\"output\":\"CMakeFiles/audio.dir/a.cpp.o\"}]")
    set(EXPANDED_COMPDB_JSON "[]")
elseif (AUDIT_CASE STREQUAL "malformed_response")
    string(CONCAT RAW_COMPDB_JSON
            "[{\"directory\":\"${BINARY_DIR}\","
            "\"command\":\"fake++ @ -o "
            "CMakeFiles/audio.dir/a.cpp.o -c a.cpp\","
            "\"file\":\"a.cpp\","
            "\"output\":\"CMakeFiles/audio.dir/a.cpp.o\"},"
            "{\"directory\":\"${BINARY_DIR}\","
            "\"command\":\"fake++ -pthread -o "
            "CMakeFiles/audio.dir/b.cpp.o -c b.cpp\","
            "\"file\":\"b.cpp\","
            "\"output\":\"CMakeFiles/audio.dir/b.cpp.o\"}]")
    string(CONCAT EXPANDED_COMPDB_JSON
            "[{\"directory\":\"${BINARY_DIR}\","
            "\"command\":\"fake++ -pthread -o "
            "CMakeFiles/audio.dir/a.cpp.o -c a.cpp\","
            "\"file\":\"a.cpp\","
            "\"output\":\"CMakeFiles/audio.dir/a.cpp.o\"},"
            "{\"directory\":\"${BINARY_DIR}\","
            "\"command\":\"fake++ -pthread -o "
            "CMakeFiles/audio.dir/b.cpp.o -c b.cpp\","
            "\"file\":\"b.cpp\","
            "\"output\":\"CMakeFiles/audio.dir/b.cpp.o\"}]")
else ()
    if (AUDIT_CASE STREQUAL "missing_pthread")
        set(B_FLAGS "\"-DNOTE=unrelated -pthread text\"")
    elseif (AUDIT_CASE STREQUAL "consumed_pthread")
        set(B_FLAGS "-MT -pthread")
    else ()
        set(B_FLAGS "-DSECOND_OBJECT=1 -pthread")
    endif ()
    if (AUDIT_CASE STREQUAL "outside_response")
        set(A_RSPFILE "../outside-a.rsp")
    elseif (AUDIT_CASE STREQUAL "response_parent_symlink")
        set(A_RSPFILE "rsp-link/missing.rsp")
    else ()
        set(A_RSPFILE "CMakeFiles/audio.dir/a.cpp.o.rsp")
    endif ()

    set(ninja_template [=[
ninja_required_version = 1.13

rule response_compile
  command = fake++ @$RSP_FILE -MD -MT $out -o $out -c $in
  rspfile = $RSP_FILE
  rspfile_content = $flags

rule direct_compile
  command = fake++ $flags -MD -MT $out -o $out -c $in

rule unrelated
  command = fake-note $flags

build CMakeFiles/audio.dir/a.cpp.o: response_compile a.cpp
  RSP_FILE = @A_RSPFILE@
  flags = "-DQUOTED=value with spaces" -DESCAPED=\"value\" -pthread

build CMakeFiles/audio.dir/b.cpp.o: direct_compile b.cpp
  flags = @B_FLAGS@

build unrelated.stamp: unrelated note.txt
  flags = -pthread

build audio: phony CMakeFiles/audio.dir/a.cpp.o CMakeFiles/audio.dir/b.cpp.o unrelated.stamp
]=])
    string(CONFIGURE "${ninja_template}" configured_ninja @ONLY)
    file(WRITE "${BINARY_DIR}/build.ninja" "${configured_ninja}")

    execute_process(
            COMMAND
            "${NINJA_PROGRAM}" -C "${BINARY_DIR}"
            -t compdb-targets "${TARGET_NAME}"
            RESULT_VARIABLE raw_result
            OUTPUT_VARIABLE RAW_COMPDB_JSON
            ERROR_VARIABLE raw_error
            ENCODING UTF-8
    )
    if (NOT raw_result EQUAL 0)
        message(FATAL_ERROR
                "Could not read raw fixture compilation database: "
                "${raw_error}")
    endif ()
    execute_process(
            COMMAND
            "${NINJA_PROGRAM}" -C "${BINARY_DIR}"
            -t compdb-targets -x "${TARGET_NAME}"
            RESULT_VARIABLE expanded_result
            OUTPUT_VARIABLE EXPANDED_COMPDB_JSON
            ERROR_VARIABLE expanded_error
            ENCODING UTF-8
    )
    if (NOT expanded_result EQUAL 0)
        message(FATAL_ERROR
                "Could not read expanded fixture compilation database: "
                "${expanded_error}")
    endif ()

    if (AUDIT_CASE STREQUAL "response_directory")
        file(MAKE_DIRECTORY
                "${BINARY_DIR}/CMakeFiles/audio.dir/a.cpp.o.rsp")
    elseif (AUDIT_CASE STREQUAL "response_symlink")
        set(response_directory
                "${BINARY_DIR}/CMakeFiles/audio.dir")
        set(response_target
                "${response_directory}/real-response.rsp")
        set(response_link
                "${response_directory}/a.cpp.o.rsp")
        file(MAKE_DIRECTORY "${response_directory}")
        file(WRITE "${response_target}" "-pthread\n")
        file(CREATE_LINK
                "${response_target}"
                "${response_link}"
                SYMBOLIC
                RESULT response_link_result)
        if (NOT response_link_result STREQUAL "0")
            message(STATUS
                    "SKIP response_symlink: ${response_link_result}")
            return()
        endif ()
    elseif (AUDIT_CASE STREQUAL "response_parent_symlink")
        set(outside_response_directory
                "${CASE_ROOT}/outside-response-directory")
        set(response_parent_link "${BINARY_DIR}/rsp-link")
        file(MAKE_DIRECTORY "${outside_response_directory}")
        file(CREATE_LINK
                "${outside_response_directory}"
                "${response_parent_link}"
                SYMBOLIC
                RESULT response_parent_link_result)
        if (NOT response_parent_link_result STREQUAL "0")
            message(STATUS
                    "SKIP response_parent_symlink: "
                    "${response_parent_link_result}")
            return()
        endif ()
    endif ()
endif ()

include("${SOURCE_ROOT}/cmake/AuditWebAudioCorePthreadCompdb.cmake")
