foreach (required_variable
        SOURCE_ROOT
        BINARY_DIR
        NINJA_PROGRAM
        TARGET_NAME)
    if (NOT DEFINED "${required_variable}" OR
            "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif ()
endforeach ()

execute_process(
        COMMAND "${NINJA_PROGRAM}" -C "${BINARY_DIR}" -t deps
        RESULT_VARIABLE deps_result
        OUTPUT_VARIABLE deps_output
        ERROR_VARIABLE deps_error
        ENCODING UTF-8
)
if (NOT deps_result EQUAL 0)
    message(FATAL_ERROR
            "Could not read Ninja compiler dependencies for ${TARGET_NAME}: "
            "${deps_error}")
endif ()

string(REPLACE "\r\n" "\n" deps_output "${deps_output}")
string(REPLACE "\n" ";" deps_lines "${deps_output}")
set(in_target_object FALSE)
set(target_object_count 0)
set(target_dependency_count 0)
set(target_objects)
set(forbidden_dependency_pattern
        "/(resource_managers/(vars|profile)[.]h|gameplay_logic/chartrunner[.]h|qml_components/bga[.]h|sounds/(audioengine|miniaudiobackend)[.]h|db/sqlitecppdb[.]h|sqlitecpp/|qtmultimedia/|wil/|llfio|miniaudio[.]h|sndfile[.]h)"
)

foreach (deps_line IN LISTS deps_lines)
    if (deps_line MATCHES "^[^ \t].*: #deps ")
        set(in_target_object FALSE)
        string(REPLACE "\\" "/" object_line "${deps_line}")
        if (object_line MATCHES
                "^CMakeFiles/${TARGET_NAME}[.]dir/.*[.](obj|o): #deps ")
            set(in_target_object TRUE)
            math(EXPR target_object_count "${target_object_count} + 1")
            string(REGEX REPLACE ": #deps .*$" "" target_object
                    "${object_line}")
            cmake_path(
                    ABSOLUTE_PATH target_object
                    BASE_DIRECTORY "${BINARY_DIR}"
                    NORMALIZE
                    OUTPUT_VARIABLE absolute_target_object)
            list(APPEND target_objects "${absolute_target_object}")
        endif ()
        continue()
    endif ()
    if (NOT in_target_object OR
            NOT deps_line MATCHES "^[ \t]+(.+)$")
        continue()
    endif ()

    set(dependency_path "${CMAKE_MATCH_1}")
    cmake_path(
            ABSOLUTE_PATH dependency_path
            BASE_DIRECTORY "${BINARY_DIR}"
            NORMALIZE
            OUTPUT_VARIABLE absolute_dependency_path)
    string(REPLACE "\\" "/" normalized_dependency
            "${absolute_dependency_path}")
    string(TOLOWER "${normalized_dependency}" normalized_dependency)
    math(EXPR target_dependency_count "${target_dependency_count} + 1")

    if (normalized_dependency MATCHES "${forbidden_dependency_pattern}")
        message(FATAL_ERROR
                "Portable audio target ${TARGET_NAME} has forbidden active "
                "include dependency: ${absolute_dependency_path}")
    endif ()
endforeach ()

if (target_object_count EQUAL 0)
    message(FATAL_ERROR
            "Ninja reported no compiler dependency sections for "
            "${TARGET_NAME}; the active include contract cannot pass")
endif ()
if (target_dependency_count EQUAL 0)
    message(FATAL_ERROR
            "Ninja reported no compiler dependencies for ${TARGET_NAME}; "
            "the active include contract cannot pass")
endif ()

if (REQUIRE_PTHREAD)
    if (NOT DEFINED OBJDUMP_PROGRAM OR OBJDUMP_PROGRAM STREQUAL "" OR
            NOT EXISTS "${OBJDUMP_PROGRAM}")
        message(FATAL_ERROR
                "Pthread object verification requires CMAKE_OBJDUMP, got "
                "${OBJDUMP_PROGRAM}")
    endif ()
    execute_process(
            COMMAND
            "${NINJA_PROGRAM}" -C "${BINARY_DIR}" -t commands "${TARGET_NAME}"
            RESULT_VARIABLE commands_result
            OUTPUT_VARIABLE commands_output
            ERROR_VARIABLE commands_error
            ENCODING UTF-8
    )
    if (NOT commands_result EQUAL 0)
        message(FATAL_ERROR
                "Could not inspect Ninja commands for ${TARGET_NAME}: "
                "${commands_error}")
    endif ()
    string(REPLACE "\\" "/" normalized_commands "${commands_output}")
    string(REPLACE "\r\n" "\n" normalized_commands "${normalized_commands}")
    string(REPLACE "\n" ";" command_lines "${normalized_commands}")
    set(pthread_compile_count 0)
    foreach (command_line IN LISTS command_lines)
        if (NOT command_line MATCHES
                "CMakeFiles/${TARGET_NAME}[.]dir/.*[.](obj|o)" OR
                NOT command_line MATCHES "(^|[ \t])-c([ \t]|$)")
            continue()
        endif ()
        math(EXPR pthread_compile_count "${pthread_compile_count} + 1")
        if (NOT command_line MATCHES "(^|[ \t])-pthread([ \t]|$)")
            message(FATAL_ERROR
                    "${TARGET_NAME} compile command omits direct -pthread: "
                    "${command_line}")
        endif ()
    endforeach ()
    if (NOT pthread_compile_count EQUAL target_object_count)
        message(FATAL_ERROR
                "${TARGET_NAME} command/dependency object count differs: "
                "${pthread_compile_count} commands vs "
                "${target_object_count} dependency objects")
    endif ()

    set(atomic_instruction_found FALSE)
    foreach (target_object IN LISTS target_objects)
        if (NOT EXISTS "${target_object}")
            message(FATAL_ERROR
                    "Expected portable audio object does not exist: "
                    "${target_object}")
        endif ()
        execute_process(
                COMMAND "${OBJDUMP_PROGRAM}" -d "${target_object}"
                RESULT_VARIABLE objdump_result
                OUTPUT_VARIABLE objdump_output
                ERROR_VARIABLE objdump_error
                ENCODING UTF-8
        )
        if (NOT objdump_result EQUAL 0)
            message(FATAL_ERROR
                    "Could not disassemble ${target_object}: ${objdump_error}")
        endif ()
        if (objdump_output MATCHES "i64[.]atomic")
            set(atomic_instruction_found TRUE)
        endif ()
    endforeach ()
    if (NOT atomic_instruction_found)
        message(FATAL_ERROR
                "${TARGET_NAME} objects contain no i64.atomic instructions; "
                "the pthread atomic ABI was not proven")
    endif ()
endif ()

message(STATUS
        "Portable audio active include closure passed "
        "(${target_object_count} objects, "
        "${target_dependency_count} compiler dependencies)")
