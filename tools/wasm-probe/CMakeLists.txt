cmake_minimum_required(VERSION 3.24)

project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES CXX)

include(cmake/verify_exact_toolchain.cmake)

set(WASM_PROBE_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
get_filename_component(WASM_PROBE_REPO_ROOT
    "${WASM_PROBE_REPO_ROOT}" ABSOLUTE)
set(WASM_PROBE_INPUT_MANIFEST
    "${CMAKE_CURRENT_SOURCE_DIR}/input-manifest.txt")
file(STRINGS "${WASM_PROBE_INPUT_MANIFEST}" WASM_PROBE_INPUTS
    ENCODING UTF-8)
set(WASM_PROBE_SORTED_INPUTS ${WASM_PROBE_INPUTS})
list(SORT WASM_PROBE_SORTED_INPUTS
    COMPARE STRING CASE INSENSITIVE ORDER ASCENDING)
if(NOT WASM_PROBE_INPUTS STREQUAL WASM_PROBE_SORTED_INPUTS)
    message(FATAL_ERROR
        "input-manifest.txt must contain unique case-insensitively sorted paths")
endif()
file(SHA256 "${WASM_PROBE_INPUT_MANIFEST}"
    WASM_PROBE_INPUT_MANIFEST_SHA256)
set(WASM_PROBE_INPUT_DIGEST_PAYLOAD
    "manifest=${WASM_PROBE_INPUT_MANIFEST_SHA256}\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${WASM_PROBE_INPUT_MANIFEST}")
foreach(WASM_PROBE_INPUT IN LISTS WASM_PROBE_INPUTS)
    if(WASM_PROBE_INPUT STREQUAL ""
            OR WASM_PROBE_INPUT MATCHES "(^|/)\\.\\.(/|$)"
            OR WASM_PROBE_INPUT MATCHES "^[/\\\\]"
            OR WASM_PROBE_INPUT MATCHES "^[A-Za-z]:"
            OR WASM_PROBE_INPUT MATCHES "\\\\")
        message(FATAL_ERROR
            "Unsafe input-manifest path: '${WASM_PROBE_INPUT}'")
    endif()
    string(TOLOWER "${WASM_PROBE_INPUT}" WASM_PROBE_INPUT_KEY)
    string(SHA256 WASM_PROBE_INPUT_KEY_SHA256
        "${WASM_PROBE_INPUT_KEY}")
    if(DEFINED "WASM_PROBE_SEEN_${WASM_PROBE_INPUT_KEY_SHA256}")
        message(FATAL_ERROR
            "Duplicate input-manifest path: '${WASM_PROBE_INPUT}'")
    endif()
    set("WASM_PROBE_SEEN_${WASM_PROBE_INPUT_KEY_SHA256}" TRUE)
    set(WASM_PROBE_INPUT_PATH
        "${WASM_PROBE_REPO_ROOT}/${WASM_PROBE_INPUT}")
    if(NOT EXISTS "${WASM_PROBE_INPUT_PATH}"
            OR IS_DIRECTORY "${WASM_PROBE_INPUT_PATH}")
        message(FATAL_ERROR
            "Missing input-manifest file: '${WASM_PROBE_INPUT}'")
    endif()
    file(SHA256 "${WASM_PROBE_INPUT_PATH}" WASM_PROBE_INPUT_SHA256)
    string(APPEND WASM_PROBE_INPUT_DIGEST_PAYLOAD
        "${WASM_PROBE_INPUT}=${WASM_PROBE_INPUT_SHA256}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${WASM_PROBE_INPUT_PATH}")
endforeach()
string(SHA256 WASM_PROBE_INPUT_DIGEST
    "${WASM_PROBE_INPUT_DIGEST_PAYLOAD}")
set(WASM_PROBE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${WASM_PROBE_GENERATED_DIR}")
configure_file(
    cmake/ProbeInputDigest.cpp.in
    "${WASM_PROBE_GENERATED_DIR}/ProbeInputDigest.cpp"
    @ONLY
)

find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS
    Concurrent
    Multimedia
    Network
    Qml
    Quick
    ShaderTools
    WebSockets
)

qt_standard_project_setup(REQUIRES 6.11)

add_library(WasmProbeWasmCompileOptions INTERFACE)
target_compile_options(WasmProbeWasmCompileOptions INTERFACE
    -pthread
    -fwasm-exceptions
    "SHELL:-sSUPPORT_LONGJMP=wasm"
)

add_library(WasmProbeExceptionBoundary STATIC
    src/ExceptionBoundary.cpp
    src/ExceptionBoundary.h
)
target_compile_features(WasmProbeExceptionBoundary PRIVATE cxx_std_20)
target_link_libraries(WasmProbeExceptionBoundary PUBLIC
    WasmProbeWasmCompileOptions
)

qt_add_executable(RhythmGameWasmProbe
    "${WASM_PROBE_GENERATED_DIR}/ProbeInputDigest.cpp"
    src/main.cpp
    src/ProbeState.cpp
    src/ProbeState.h
)
target_compile_features(RhythmGameWasmProbe PRIVATE cxx_std_20)

qt_add_qml_module(RhythmGameWasmProbe
    URI RhythmGame.WasmProbe
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
)

qt_add_shaders(RhythmGameWasmProbe "wasm_probe_shaders"
    PREFIX "/qt/qml/RhythmGame/WasmProbe/shaders"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"
    FILES
        qml/pulse.frag
)

target_link_libraries(RhythmGameWasmProbe PRIVATE
    WasmProbeExceptionBoundary
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Network
    Qt6::Quick
    Qt6::WebSockets
)

target_link_options(RhythmGameWasmProbe PRIVATE
    -pthread
    -fwasm-exceptions
    "SHELL:-sJSPI"
    "SHELL:-sAUDIO_WORKLET=1"
    "SHELL:-sWASM_WORKERS=1"
    "SHELL:-sPTHREAD_POOL_SIZE=4"
    "SHELL:-sPTHREAD_POOL_SIZE_STRICT=2"
    "SHELL:-sALLOW_BLOCKING_ON_MAIN_THREAD=0"
)
