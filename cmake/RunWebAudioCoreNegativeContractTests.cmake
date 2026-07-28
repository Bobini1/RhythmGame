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
        "/web-audio-core-negative-contracts$")
    message(FATAL_ERROR
            "TEST_BINARY_ROOT must end in "
            "web-audio-core-negative-contracts before recursive cleanup")
endif ()

set(fixture_source
        "${SOURCE_ROOT}/test/cmake/web_audio_core_contract")
set(base_configure_command
        "${CMAKE_COMMAND}"
        -S "${fixture_source}"
        -G Ninja
        "-DCMAKE_MAKE_PROGRAM=${NINJA_PROGRAM}"
        -DCMAKE_BUILD_TYPE=Release
)
if (DEFINED DEPENDENCY_PREFIX AND NOT DEPENDENCY_PREFIX STREQUAL "")
    list(APPEND base_configure_command
            "-DCMAKE_PREFIX_PATH=${DEPENDENCY_PREFIX}")
endif ()
if (DEFINED NESTED_TOOLCHAIN_FILE AND
        NOT NESTED_TOOLCHAIN_FILE STREQUAL "")
    list(APPEND base_configure_command
            "-DCMAKE_TOOLCHAIN_FILE=${NESTED_TOOLCHAIN_FILE}")
    if (DEFINED VCPKG_INSTALLED_DIR AND
            NOT VCPKG_INSTALLED_DIR STREQUAL "")
        list(APPEND base_configure_command
                "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
                -DVCPKG_MANIFEST_MODE=OFF)
    endif ()
    if (DEFINED VCPKG_TARGET_TRIPLET AND
            NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        list(APPEND base_configure_command
                "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
    endif ()
    if (DEFINED VCPKG_HOST_TRIPLET AND
            NOT VCPKG_HOST_TRIPLET STREQUAL "")
        list(APPEND base_configure_command
                "-DVCPKG_HOST_TRIPLET=${VCPKG_HOST_TRIPLET}")
    endif ()
elseif (DEFINED NESTED_CXX_COMPILER AND
        NOT NESTED_CXX_COMPILER STREQUAL "")
    list(APPEND base_configure_command
            "-DCMAKE_CXX_COMPILER=${NESTED_CXX_COMPILER}")
endif ()
foreach (forwarded_variable
        VCPKG_CHAINLOAD_TOOLCHAIN_FILE
        VCPKG_OVERLAY_TRIPLETS
        VCPKG_OVERLAY_PORTS)
    if (DEFINED "${forwarded_variable}" AND
            NOT "${${forwarded_variable}}" STREQUAL "")
        string(REPLACE ";" "\\;" forwarded_value
                "${${forwarded_variable}}")
        list(APPEND base_configure_command
                "-D${forwarded_variable}=${forwarded_value}")
    endif ()
endforeach ()

file(REMOVE_RECURSE "${TEST_BINARY_ROOT}")
file(MAKE_DIRECTORY "${TEST_BINARY_ROOT}")

set(configure_failure_cases
        preexisting_target
        late_source
        late_definition
        late_include
        late_private_link
        late_interface_link
        late_direct_link
        late_direct_exclude
        late_created_helper
)
foreach (contract_case IN LISTS configure_failure_cases)
    set(contract_build "${TEST_BINARY_ROOT}/${contract_case}")
    execute_process(
            COMMAND
            ${base_configure_command}
            -B "${contract_build}"
            "-DCONTRACT_CASE=${contract_case}"
            RESULT_VARIABLE contract_result
            OUTPUT_VARIABLE contract_stdout
            ERROR_VARIABLE contract_stderr
            ENCODING UTF-8
    )
    set(contract_output "${contract_stdout}\n${contract_stderr}")
    if (contract_result EQUAL 0)
        message(FATAL_ERROR
                "${contract_case} unexpectedly passed final-state configure "
                "verification")
    endif ()
    if (NOT contract_output MATCHES "RhythmGame_web_audio_core")
        message(FATAL_ERROR
                "${contract_case} failed without a portable-target "
                "diagnostic:\n${contract_output}")
    endif ()
    if (contract_case STREQUAL "preexisting_target" AND
            NOT contract_output MATCHES "already exists")
        message(FATAL_ERROR
                "preexisting_target did not report ownership rejection:\n"
                "${contract_output}")
    endif ()
    if (contract_case STREQUAL "late_created_helper" AND
            NOT contract_output MATCHES
            "RhythmGame_contract_late_helper -> SQLiteCpp")
        message(FATAL_ERROR
                "late_created_helper did not report the transitive forbidden "
                "dependency:\n${contract_output}")
    endif ()
endforeach ()

set(header_build "${TEST_BINARY_ROOT}/forbidden-header")
execute_process(
        COMMAND
        ${base_configure_command}
        -B "${header_build}"
        -DCONTRACT_CASE=forbidden_header
        RESULT_VARIABLE header_configure_result
        OUTPUT_VARIABLE header_configure_stdout
        ERROR_VARIABLE header_configure_stderr
        ENCODING UTF-8
)
if (NOT header_configure_result EQUAL 0)
    message(FATAL_ERROR
            "Forbidden-header fixture did not reach compilation:\n"
            "${header_configure_stdout}\n${header_configure_stderr}")
endif ()

execute_process(
        COMMAND
        "${CMAKE_COMMAND}"
        --build "${header_build}"
        --target RhythmGame_web_audio_core
        --parallel
        RESULT_VARIABLE header_build_result
        OUTPUT_VARIABLE header_build_stdout
        ERROR_VARIABLE header_build_stderr
        ENCODING UTF-8
)
set(header_build_output
        "${header_build_stdout}\n${header_build_stderr}")
if (header_build_result EQUAL 0)
    message(FATAL_ERROR
            "Active forbidden-header injection unexpectedly passed the "
            "compiler-dependency contract")
endif ()
if (NOT header_build_output MATCHES
        "forbidden active[\r\n ]+include dependency:[\r\n ]+.*(AudioEngine|miniaudio)[.]h")
    message(FATAL_ERROR
            "Forbidden-header test failed without the dependency-contract "
            "diagnostic:\n${header_build_output}")
endif ()

set(clean_build "${TEST_BINARY_ROOT}/clean")
execute_process(
        COMMAND
        ${base_configure_command}
        -B "${clean_build}"
        -DCONTRACT_CASE=clean
        RESULT_VARIABLE clean_configure_result
        OUTPUT_VARIABLE clean_configure_stdout
        ERROR_VARIABLE clean_configure_stderr
        ENCODING UTF-8
)
if (NOT clean_configure_result EQUAL 0)
    message(FATAL_ERROR
            "Clean contract fixture did not configure:\n"
            "${clean_configure_stdout}\n${clean_configure_stderr}")
endif ()

execute_process(
        COMMAND
        "${CMAKE_COMMAND}"
        --build "${clean_build}"
        --target RhythmGame_web_audio_core
        --parallel
        RESULT_VARIABLE clean_build_result
        OUTPUT_VARIABLE clean_build_stdout
        ERROR_VARIABLE clean_build_stderr
        ENCODING UTF-8
)
set(clean_build_output
        "${clean_build_stdout}\n${clean_build_stderr}")
if (NOT clean_build_result EQUAL 0)
    message(FATAL_ERROR
            "Clean portable-core build failed:\n${clean_build_output}")
endif ()
if (NOT clean_build_output MATCHES
        "Portable audio active include closure passed")
    message(FATAL_ERROR
            "Clean core-only build omitted the active-include audit:\n"
            "${clean_build_output}")
endif ()
if (NOT EXISTS
        "${clean_build}/RhythmGame_web_audio_core-dependency-contract.stamp")
    message(FATAL_ERROR
            "Clean core-only build did not produce the dependency-contract "
            "success stamp")
endif ()

message(STATUS
        "Portable audio negative dependency contracts passed")
