foreach (required_variable
        BINARY_DIR
        TARGET_NAME
        TARGET_OBJECTS
        TARGET_OBJECT_COUNT
        RAW_COMPDB_JSON
        EXPANDED_COMPDB_JSON)
    if (NOT DEFINED "${required_variable}" OR
            "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is required")
    endif ()
endforeach ()

set(normalized_binary_dir_path "${BINARY_DIR}")
cmake_path(
        ABSOLUTE_PATH normalized_binary_dir_path
        NORMALIZE
        OUTPUT_VARIABLE normalized_binary_dir)
string(REPLACE "\\" "/" normalized_binary_dir
        "${normalized_binary_dir}")
string(REGEX REPLACE "/+$" "" normalized_binary_dir
        "${normalized_binary_dir}")

function(rhythmgame_make_comparable_path candidate output_variable)
    string(REPLACE "\\" "/" comparable_candidate "${candidate}")
    if (NOT comparable_candidate MATCHES "^([A-Za-z]:)?/$")
        string(REGEX REPLACE "/+$" "" comparable_candidate
                "${comparable_candidate}")
    endif ()
    if (WIN32)
        string(TOLOWER "${comparable_candidate}"
                comparable_candidate)
    endif ()
    set("${output_variable}" "${comparable_candidate}" PARENT_SCOPE)
endfunction()

rhythmgame_make_comparable_path(
        "${normalized_binary_dir}" comparable_binary_dir)
set(comparable_binary_prefix "${comparable_binary_dir}/")

function(rhythmgame_require_binary_descendant candidate description)
    set(normalized_candidate_path "${candidate}")
    cmake_path(
            ABSOLUTE_PATH normalized_candidate_path
            NORMALIZE
            OUTPUT_VARIABLE normalized_candidate)
    rhythmgame_make_comparable_path(
            "${normalized_candidate}" comparable_candidate)
    string(FIND
            "${comparable_candidate}"
            "${comparable_binary_prefix}"
            prefix_index)
    if (NOT prefix_index EQUAL 0)
        message(FATAL_ERROR
                "${description} is not a strict descendant of BINARY_DIR: "
                "${normalized_candidate}")
    endif ()
endfunction()

function(rhythmgame_require_binary_location candidate description)
    set(normalized_candidate_path "${candidate}")
    cmake_path(
            ABSOLUTE_PATH normalized_candidate_path
            NORMALIZE
            OUTPUT_VARIABLE normalized_candidate)
    rhythmgame_make_comparable_path(
            "${normalized_candidate}" comparable_candidate)
    string(FIND
            "${comparable_candidate}"
            "${comparable_binary_prefix}"
            prefix_index)
    if (NOT comparable_candidate STREQUAL comparable_binary_dir AND
            NOT prefix_index EQUAL 0)
        message(FATAL_ERROR
                "${description} is outside BINARY_DIR: "
                "${normalized_candidate}")
    endif ()
endfunction()

function(rhythmgame_command_filename argument output_variable)
    string(REPLACE "\\" "/" normalized_argument "${argument}")
    string(REGEX REPLACE "^.*/" ""
            comparable_filename "${normalized_argument}")

    # Native Windows commands are case-insensitive. Keep POSIX-shaped paths
    # case-sensitive even when this script is exercised by CMake on Windows.
    if (WIN32 AND
            (NOT normalized_argument MATCHES "^/" OR
            normalized_argument MATCHES "^//"))
        string(TOLOWER "${comparable_filename}"
                comparable_filename)
    endif ()
    set("${output_variable}" "${comparable_filename}" PARENT_SCOPE)
endfunction()

function(rhythmgame_select_compiler_arguments
        arguments_variable
        output_variable
        launcher_output_variable
        command_description)
    # The adapter's delimiter is not the compiler's option terminator.
    # Authenticate that boundary first, then audit only the compiler argv.
    set(command_arguments "${${arguments_variable}}")
    set(adapter_indices)
    set(driver_kind_indices)
    list(LENGTH command_arguments command_argument_count)
    if (command_argument_count GREATER 0)
        math(EXPR last_command_argument_index
                "${command_argument_count} - 1")
        foreach (argument_index RANGE
                0 ${last_command_argument_index})
            list(GET command_arguments
                    ${argument_index} argument)
            rhythmgame_command_filename(
                    "${argument}" comparable_argument_filename)
            if (comparable_argument_filename STREQUAL
                    "invoke_emscripten_driver.py")
                list(APPEND adapter_indices ${argument_index})
            endif ()
            if (argument STREQUAL "--driver-kind")
                list(APPEND driver_kind_indices ${argument_index})
            endif ()
        endforeach ()
    endif ()

    list(LENGTH adapter_indices adapter_count)
    list(LENGTH driver_kind_indices driver_kind_count)
    if (adapter_count EQUAL 0 AND driver_kind_count EQUAL 0)
        set("${output_variable}" "${command_arguments}" PARENT_SCOPE)
        set("${launcher_output_variable}" FALSE PARENT_SCOPE)
        return()
    endif ()

    if (adapter_count EQUAL 0)
        message(FATAL_ERROR
                "${command_description} has --driver-kind without the "
                "authenticated invoke_emscripten_driver.py launcher")
    elseif (adapter_count GREATER 1)
        message(FATAL_ERROR
                "${command_description} authenticated launcher has "
                "multiple invoke_emscripten_driver.py arguments")
    endif ()
    if (driver_kind_count EQUAL 0)
        message(FATAL_ERROR
                "${command_description} authenticated launcher is "
                "missing --driver-kind")
    elseif (driver_kind_count GREATER 1)
        message(FATAL_ERROR
                "${command_description} authenticated launcher has "
                "multiple --driver-kind options")
    endif ()

    list(GET adapter_indices 0 adapter_index)
    list(GET driver_kind_indices 0 driver_kind_index)
    if (NOT adapter_index LESS driver_kind_index)
        message(FATAL_ERROR
                "${command_description} authenticated launcher places "
                "--driver-kind before invoke_emscripten_driver.py")
    endif ()

    math(EXPR driver_kind_value_index
            "${driver_kind_index} + 1")
    if (NOT driver_kind_value_index LESS command_argument_count)
        message(FATAL_ERROR
                "${command_description} authenticated launcher has no "
                "value for --driver-kind")
    endif ()
    list(GET command_arguments
            ${driver_kind_value_index} driver_kind)
    if (NOT driver_kind STREQUAL "em++")
        message(FATAL_ERROR
                "${command_description} authenticated launcher "
                "--driver-kind must be em++, not ${driver_kind}")
    endif ()

    math(EXPR launcher_boundary_index
            "${driver_kind_value_index} + 1")
    if (NOT launcher_boundary_index LESS command_argument_count)
        message(FATAL_ERROR
                "${command_description} authenticated launcher must "
                "place -- immediately after --driver-kind em++")
    endif ()
    list(GET command_arguments
            ${launcher_boundary_index} launcher_boundary)
    if (NOT launcher_boundary STREQUAL "--")
        message(FATAL_ERROR
                "${command_description} authenticated launcher must "
                "place -- immediately after --driver-kind em++")
    endif ()

    if (launcher_boundary_index GREATER 0)
        math(EXPR launcher_argument_last_index
                "${launcher_boundary_index} - 1")
        foreach (launcher_argument_index RANGE
                0 ${launcher_argument_last_index})
            list(GET command_arguments
                    ${launcher_argument_index} launcher_argument)
            if (launcher_argument STREQUAL "--")
                message(FATAL_ERROR
                        "${command_description} authenticated launcher "
                        "has an ambiguous compiler boundary")
            endif ()
        endforeach ()
    endif ()

    math(EXPR compiler_driver_index
            "${launcher_boundary_index} + 1")
    if (NOT compiler_driver_index LESS command_argument_count)
        message(FATAL_ERROR
                "${command_description} authenticated launcher has no "
                "compiler after its boundary")
    endif ()
    list(GET command_arguments
            ${compiler_driver_index} compiler_driver)
    if (compiler_driver STREQUAL "--")
        message(FATAL_ERROR
                "${command_description} authenticated launcher has an "
                "ambiguous compiler boundary")
    endif ()
    rhythmgame_command_filename(
            "${compiler_driver}" comparable_compiler_driver_filename)
    if (NOT comparable_compiler_driver_filename STREQUAL "em++" AND
            NOT comparable_compiler_driver_filename STREQUAL "em++.bat")
        message(FATAL_ERROR
                "${command_description} authenticated launcher compiler "
                "is not em++: ${compiler_driver}")
    endif ()

    list(SUBLIST command_arguments
            ${compiler_driver_index} -1 compiler_arguments)
    set("${output_variable}" "${compiler_arguments}" PARENT_SCOPE)
    set("${launcher_output_variable}" TRUE PARENT_SCOPE)
endfunction()

function(rhythmgame_has_effective_pthread
        arguments_variable output_variable)
    set(options_with_separate_value
            --sysroot
            --target
            -arch
            -B
            -D
            -dependency-file
            -gcc-toolchain
            -I
            -idirafter
            -iframework
            -iframeworkwithsysroot
            -imacros
            -include
            -iprefix
            -iquote
            -isystem
            -isysroot
            -iwithprefix
            -iwithprefixbefore
            -L
            -mllvm
            -MF
            -MJ
            -MQ
            -MT
            -o
            -resource-dir
            -s
            -target
            -U
            -Xanalyzer
            -Xassembler
            -Xclang
            -Xlinker
            -Xpreprocessor
            -x)
    set(pthread_found FALSE)
    set(after_option_terminator FALSE)
    list(LENGTH "${arguments_variable}" argument_count)
    if (argument_count GREATER 0)
        math(EXPR last_argument_index "${argument_count} - 1")
        foreach (argument_index RANGE 0 ${last_argument_index})
            list(GET "${arguments_variable}"
                    ${argument_index} argument)
            if (argument STREQUAL "--")
                set(after_option_terminator TRUE)
                continue()
            endif ()
            if (after_option_terminator OR
                    NOT argument STREQUAL "-pthread")
                continue()
            endif ()

            set(argument_is_consumed FALSE)
            if (argument_index GREATER 0)
                math(EXPR previous_argument_index
                        "${argument_index} - 1")
                list(GET "${arguments_variable}"
                        ${previous_argument_index} previous_argument)
                if (previous_argument IN_LIST
                        options_with_separate_value)
                    set(argument_is_consumed TRUE)
                endif ()
            endif ()
            if (NOT argument_is_consumed)
                set(pthread_found TRUE)
                break()
            endif ()
        endforeach ()
    endif ()
    set("${output_variable}" "${pthread_found}" PARENT_SCOPE)
endfunction()

function(rhythmgame_require_response_path_chain
        response_path description)
    cmake_path(GET response_path PARENT_PATH existing_ancestor)
    while (TRUE)
        if (IS_SYMLINK "${existing_ancestor}")
            message(FATAL_ERROR
                    "${description} resolves through a symlink or reparse "
                    "point: ${existing_ancestor}")
        endif ()
        if (EXISTS "${existing_ancestor}")
            if (NOT IS_DIRECTORY "${existing_ancestor}")
                message(FATAL_ERROR
                        "${description} has a non-directory ancestor: "
                        "${existing_ancestor}")
            endif ()
            break()
        endif ()
        cmake_path(GET existing_ancestor
                PARENT_PATH next_ancestor)
        if (next_ancestor STREQUAL existing_ancestor OR
                next_ancestor STREQUAL "")
            message(FATAL_ERROR
                    "${description} has no existing ancestor inside "
                    "BINARY_DIR: ${response_path}")
        endif ()
        set(existing_ancestor "${next_ancestor}")
    endwhile ()

    rhythmgame_require_binary_location(
            "${existing_ancestor}" "${description} ancestor")
    file(REAL_PATH "${normalized_binary_dir}" real_binary_dir)
    file(REAL_PATH "${existing_ancestor}" real_existing_ancestor)
    file(RELATIVE_PATH ancestor_relative_path
            "${normalized_binary_dir}" "${existing_ancestor}")
    set(expected_real_ancestor_path "${real_binary_dir}")
    cmake_path(
            APPEND expected_real_ancestor_path
            "${ancestor_relative_path}"
            OUTPUT_VARIABLE expected_real_ancestor)
    cmake_path(NORMAL_PATH expected_real_ancestor)
    rhythmgame_make_comparable_path(
            "${real_existing_ancestor}"
            comparable_real_existing_ancestor)
    rhythmgame_make_comparable_path(
            "${expected_real_ancestor}"
            comparable_expected_real_ancestor)
    if (NOT comparable_real_existing_ancestor STREQUAL
            comparable_expected_real_ancestor)
        message(FATAL_ERROR
                "${description} resolves through a symlink or reparse "
                "point: ${existing_ancestor}")
    endif ()
endfunction()

list(LENGTH TARGET_OBJECTS actual_target_object_count)
if (NOT actual_target_object_count EQUAL TARGET_OBJECT_COUNT)
    message(FATAL_ERROR
            "${TARGET_NAME} target object count drifted before pthread audit: "
            "${actual_target_object_count} paths vs "
            "${TARGET_OBJECT_COUNT} declared")
endif ()

set(comparable_target_objects)
foreach (target_object IN LISTS TARGET_OBJECTS)
    set(normalized_target_object_path "${target_object}")
    cmake_path(
            ABSOLUTE_PATH normalized_target_object_path
            NORMALIZE
            OUTPUT_VARIABLE normalized_target_object)
    rhythmgame_require_binary_descendant(
            "${normalized_target_object}"
            "${TARGET_NAME} target object")
    rhythmgame_make_comparable_path(
            "${normalized_target_object}" comparable_target_object)
    list(FIND comparable_target_objects
            "${comparable_target_object}" duplicate_target_index)
    if (NOT duplicate_target_index EQUAL -1)
        message(FATAL_ERROR
                "${TARGET_NAME} target object list contains a duplicate: "
                "${normalized_target_object}")
    endif ()
    list(APPEND comparable_target_objects
            "${comparable_target_object}")
endforeach ()

set(comparable_target_name "${TARGET_NAME}")
if (WIN32)
    string(TOLOWER "${comparable_target_name}"
            comparable_target_name)
endif ()
set(comparable_target_object_prefix
        "${comparable_binary_prefix}cmakefiles/${comparable_target_name}.dir/")

macro(rhythmgame_parse_pthread_compdb database_name database_variable)
    set(database_json "${${database_variable}}")
    string(JSON database_type
            ERROR_VARIABLE database_error
            TYPE "${database_json}")
    if (NOT database_error STREQUAL "NOTFOUND" OR
            NOT database_type STREQUAL "ARRAY")
        message(FATAL_ERROR
                "${database_name} compilation database is invalid: "
                "${database_error}")
    endif ()
    string(JSON database_length
            ERROR_VARIABLE database_error
            LENGTH "${database_json}")
    if (NOT database_error STREQUAL "NOTFOUND" OR
            database_length EQUAL 0)
        message(FATAL_ERROR
                "${database_name} compilation database has no records: "
                "${database_error}")
    endif ()

    math(EXPR database_last_index "${database_length} - 1")
    foreach (database_index RANGE 0 ${database_last_index})
        string(JSON record_type
                ERROR_VARIABLE record_error
                TYPE "${database_json}" ${database_index})
        if (NOT record_error STREQUAL "NOTFOUND" OR
                NOT record_type STREQUAL "OBJECT")
            message(FATAL_ERROR
                    "${database_name} compilation database record "
                    "${database_index} is not an object: ${record_error}")
        endif ()

        foreach (record_field directory command file output)
            string(JSON field_type
                    ERROR_VARIABLE field_error
                    TYPE "${database_json}"
                    ${database_index} "${record_field}")
            if (NOT field_error STREQUAL "NOTFOUND" OR
                    NOT field_type STREQUAL "STRING")
                message(FATAL_ERROR
                        "${database_name} compilation database record "
                        "${database_index} lacks string field "
                        "${record_field}: ${field_error}")
            endif ()
            string(JSON field_value
                    GET "${database_json}"
                    ${database_index} "${record_field}")
            if (field_value STREQUAL "")
                message(FATAL_ERROR
                        "${database_name} compilation database record "
                        "${database_index} has empty field ${record_field}")
            endif ()
            set("${database_name}_${database_index}_${record_field}"
                    "${field_value}")
        endforeach ()

        set(record_directory
                "${${database_name}_${database_index}_directory}")
        cmake_path(IS_ABSOLUTE record_directory
                record_directory_is_absolute)
        if (NOT record_directory_is_absolute)
            message(FATAL_ERROR
                    "${database_name} compilation database record "
                    "${database_index} has a relative directory: "
                    "${record_directory}")
        endif ()
        cmake_path(
                NORMAL_PATH record_directory
                OUTPUT_VARIABLE absolute_record_directory)
        rhythmgame_require_binary_location(
                "${absolute_record_directory}"
                "${database_name} compilation database directory")
        set("${database_name}_${database_index}_directory"
                "${absolute_record_directory}")

        set(record_output
                "${${database_name}_${database_index}_output}")
        cmake_path(
                ABSOLUTE_PATH record_output
                BASE_DIRECTORY "${absolute_record_directory}"
                NORMALIZE
                OUTPUT_VARIABLE absolute_record_output)
        rhythmgame_require_binary_descendant(
                "${absolute_record_output}"
                "${database_name} compilation database output")
        rhythmgame_make_comparable_path(
                "${absolute_record_output}" comparable_record_output)
        string(REPLACE "\\" "/" absolute_record_output
                "${absolute_record_output}")
        set("${database_name}_${database_index}_output"
                "${absolute_record_output}")
        set("${database_name}_${database_index}_comparable_output"
                "${comparable_record_output}")

        string(FIND
                "${comparable_record_output}"
                "${comparable_target_object_prefix}"
                target_prefix_index)
        if (target_prefix_index EQUAL 0 AND
                comparable_record_output MATCHES "[.](o|obj)$")
            list(FIND comparable_target_objects
                    "${comparable_record_output}"
                    known_target_object_index)
            if (known_target_object_index EQUAL -1)
                message(FATAL_ERROR
                        "${database_name} compilation database exposes "
                        "an untracked ${TARGET_NAME} object: "
                        "${absolute_record_output}")
            endif ()
        endif ()
    endforeach ()
    set("${database_name}_record_count" "${database_length}")
endmacro()

rhythmgame_parse_pthread_compdb(raw RAW_COMPDB_JSON)
rhythmgame_parse_pthread_compdb(expanded EXPANDED_COMPDB_JSON)

if (NOT raw_record_count EQUAL expanded_record_count)
    message(FATAL_ERROR
            "${TARGET_NAME} raw/expanded compilation database count "
            "differs: ${raw_record_count} vs ${expanded_record_count}")
endif ()

foreach (database_name raw expanded)
    set(seen_database_outputs)
    math(EXPR database_last_index
            "${${database_name}_record_count} - 1")
    foreach (database_index RANGE 0 ${database_last_index})
        set(comparable_record_output
                "${${database_name}_${database_index}_comparable_output}")
        list(FIND seen_database_outputs
                "${comparable_record_output}" duplicate_output_index)
        if (NOT duplicate_output_index EQUAL -1)
            message(FATAL_ERROR
                    "${database_name} compilation database repeats output: "
                    "${${database_name}_${database_index}_output}")
        endif ()
        list(APPEND seen_database_outputs
                "${comparable_record_output}")

        set(other_database_name expanded)
        if (database_name STREQUAL "expanded")
            set(other_database_name raw)
        endif ()
        set(other_output_count 0)
        math(EXPR other_database_last_index
                "${${other_database_name}_record_count} - 1")
        foreach (other_index RANGE 0 ${other_database_last_index})
            if ("${${other_database_name}_${other_index}_comparable_output}"
                    STREQUAL "${comparable_record_output}")
                math(EXPR other_output_count
                        "${other_output_count} + 1")
            endif ()
        endforeach ()
        if (NOT other_output_count EQUAL 1)
            message(FATAL_ERROR
                    "${TARGET_NAME} ${database_name} compilation database "
                    "output has ${other_output_count} peer records in "
                    "${other_database_name}: "
                    "${${database_name}_${database_index}_output}")
        endif ()
    endforeach ()
endforeach ()

set(audited_target_compile_count 0)
foreach (comparable_target_object IN LISTS comparable_target_objects)
    set(raw_target_record_count 0)
    set(expanded_target_record_count 0)
    foreach (database_name raw expanded)
        math(EXPR database_last_index
                "${${database_name}_record_count} - 1")
        foreach (database_index RANGE 0 ${database_last_index})
            if (NOT
                    "${${database_name}_${database_index}_comparable_output}"
                    STREQUAL "${comparable_target_object}")
                continue()
            endif ()
            math(EXPR "${database_name}_target_record_count"
                    "${${database_name}_target_record_count} + 1")
            set("${database_name}_target_command"
                    "${${database_name}_${database_index}_command}")
            set("${database_name}_target_directory"
                    "${${database_name}_${database_index}_directory}")
            set("${database_name}_target_output"
                    "${${database_name}_${database_index}_output}")
        endforeach ()
        if (NOT "${${database_name}_target_record_count}" EQUAL 1)
            message(FATAL_ERROR
                    "${TARGET_NAME} ${database_name} compilation database "
                    "has ${${database_name}_target_record_count} records "
                    "for target object ${comparable_target_object}")
        endif ()
    endforeach ()

    if (WIN32)
        separate_arguments(raw_arguments
                WINDOWS_COMMAND "${raw_target_command}")
        separate_arguments(expanded_arguments
                WINDOWS_COMMAND "${expanded_target_command}")
    else ()
        separate_arguments(raw_arguments
                UNIX_COMMAND "${raw_target_command}")
        separate_arguments(expanded_arguments
                UNIX_COMMAND "${expanded_target_command}")
    endif ()

    rhythmgame_select_compiler_arguments(
            raw_arguments
            raw_compiler_arguments
            raw_uses_authenticated_launcher
            "${TARGET_NAME} raw command for ${raw_target_output}")
    rhythmgame_select_compiler_arguments(
            expanded_arguments
            expanded_compiler_arguments
            expanded_uses_authenticated_launcher
            "${TARGET_NAME} expanded command for ${expanded_target_output}")
    if (NOT raw_uses_authenticated_launcher STREQUAL
            expanded_uses_authenticated_launcher)
        message(FATAL_ERROR
                "${TARGET_NAME} raw/expanded authenticated-launcher shape "
                "differs for ${expanded_target_output}")
    endif ()

    foreach (compile_arguments
            raw_compiler_arguments
            expanded_compiler_arguments)
        if (NOT "-c" IN_LIST "${compile_arguments}")
            message(FATAL_ERROR
                    "${TARGET_NAME} compilation database record is not a "
                    "compile command for ${expanded_target_output}: "
                    "${${compile_arguments}}")
        endif ()
    endforeach ()

    rhythmgame_has_effective_pthread(
            expanded_compiler_arguments expanded_pthread_found)
    if (NOT expanded_pthread_found)
        message(FATAL_ERROR
                "${TARGET_NAME} compile command omits effective expanded "
                "-pthread "
                "for ${expanded_target_output}: "
                "${expanded_target_command}")
    endif ()

    set(response_arguments)
    foreach (raw_argument IN LISTS raw_compiler_arguments)
        if (raw_argument MATCHES "^@")
            list(APPEND response_arguments "${raw_argument}")
        endif ()
    endforeach ()
    list(LENGTH response_arguments response_argument_count)
    if (response_argument_count GREATER 1)
        message(FATAL_ERROR
                "${TARGET_NAME} compile command has multiple response-file "
                "arguments for ${raw_target_output}: ${raw_target_command}")
    endif ()

    foreach (response_argument IN LISTS response_arguments)
        if (response_argument STREQUAL "@")
            message(FATAL_ERROR
                    "${TARGET_NAME} compile command has a malformed "
                    "response-file argument for ${raw_target_output}: "
                    "${response_argument}")
        endif ()
        string(SUBSTRING "${response_argument}" 1 -1 response_path)
        if (response_path MATCHES "^@" OR
                NOT response_path MATCHES "[.]rsp$")
            message(FATAL_ERROR
                    "${TARGET_NAME} compile command has a malformed "
                    "response-file argument for ${raw_target_output}: "
                    "${response_argument}")
        endif ()
        cmake_path(
                ABSOLUTE_PATH response_path
                BASE_DIRECTORY "${raw_target_directory}"
                NORMALIZE
                OUTPUT_VARIABLE absolute_response_path)
        rhythmgame_require_binary_descendant(
                "${absolute_response_path}"
                "${TARGET_NAME} response file")
        rhythmgame_require_response_path_chain(
                "${absolute_response_path}"
                "${TARGET_NAME} response file")

        # Ninja removes transient response files after a successful command.
        # The raw database authenticates their graph-owned path, while -x
        # obtains their effective arguments from Ninja's loaded build graph.
        # Only inspect a response path that still exists; the audit never
        # opens a missing response file.
        if (EXISTS "${absolute_response_path}")
            if (IS_DIRECTORY "${absolute_response_path}" OR
                    IS_SYMLINK "${absolute_response_path}")
                message(FATAL_ERROR
                        "${TARGET_NAME} response path exists but is not a "
                        "regular file: ${absolute_response_path}")
            endif ()
            file(REAL_PATH "${absolute_response_path}"
                    real_response_path)
            rhythmgame_make_comparable_path(
                    "${absolute_response_path}"
                    comparable_lexical_response)
            rhythmgame_make_comparable_path(
                    "${real_response_path}"
                    comparable_real_response)
            if (NOT comparable_lexical_response STREQUAL
                    comparable_real_response)
                message(FATAL_ERROR
                        "${TARGET_NAME} response path resolves through a "
                        "symlink or reparse point: "
                        "${absolute_response_path}")
            endif ()
        endif ()
    endforeach ()

    rhythmgame_has_effective_pthread(
            raw_compiler_arguments raw_pthread_found)
    if (response_argument_count EQUAL 0 AND
            NOT raw_pthread_found)
        message(FATAL_ERROR
                "${TARGET_NAME} raw compile command contains neither "
                "direct -pthread nor a response file for "
                "${raw_target_output}: ${raw_target_command}")
    endif ()

    math(EXPR audited_target_compile_count
            "${audited_target_compile_count} + 1")
endforeach ()

if (NOT audited_target_compile_count EQUAL TARGET_OBJECT_COUNT)
    message(FATAL_ERROR
            "${TARGET_NAME} pthread audit covered "
            "${audited_target_compile_count} commands for "
            "${TARGET_OBJECT_COUNT} target objects")
endif ()
