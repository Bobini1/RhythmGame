cmake_minimum_required(VERSION 3.24)

set(WASM_PROBE_LAUNCHER_ENVIRONMENT
    EMSDK_PYTHON
    RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER
    RHYTHMGAME_WASM_TOOLCHAIN_LOCK
    RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR
    RHYTHMGAME_EMSCRIPTEN_ROOT
    RHYTHMGAME_EM_CONFIG
    RHYTHMGAME_EM_CONFIG_SHA256
    RHYTHMGAME_EM_CACHE
)
foreach(WASM_PROBE_LAUNCHER_VARIABLE
        IN LISTS WASM_PROBE_LAUNCHER_ENVIRONMENT)
    if(NOT DEFINED ENV{${WASM_PROBE_LAUNCHER_VARIABLE}}
            OR "$ENV{${WASM_PROBE_LAUNCHER_VARIABLE}}" STREQUAL "")
        message(FATAL_ERROR
            "Missing authenticated compiler-launcher environment: "
            "${WASM_PROBE_LAUNCHER_VARIABLE}")
    endif()
endforeach()
set(WASM_PROBE_COMPILER_LAUNCHER_BASE
    "$ENV{EMSDK_PYTHON}"
    -I
    -B
    "$ENV{RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER}"
    --lock
    "$ENV{RHYTHMGAME_WASM_TOOLCHAIN_LOCK}"
    --auditor
    "$ENV{RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR}"
    --emscripten-root
    "$ENV{RHYTHMGAME_EMSCRIPTEN_ROOT}"
    --em-config
    "$ENV{RHYTHMGAME_EM_CONFIG}"
    --em-config-sha256
    "$ENV{RHYTHMGAME_EM_CONFIG_SHA256}"
    --cache-root
    "$ENV{RHYTHMGAME_EM_CACHE}"
)
set(CMAKE_C_COMPILER_LAUNCHER
    ${WASM_PROBE_COMPILER_LAUNCHER_BASE}
    --driver-kind emcc --
    CACHE STRING "Authenticated Emscripten C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER
    ${WASM_PROBE_COMPILER_LAUNCHER_BASE}
    --driver-kind em++ --
    CACHE STRING "Authenticated Emscripten C++ compiler launcher" FORCE)
set(CMAKE_C_LINKER_LAUNCHER
    ${WASM_PROBE_COMPILER_LAUNCHER_BASE}
    --driver-kind emcc --
    CACHE STRING "Authenticated Emscripten C linker launcher" FORCE)
set(CMAKE_CXX_LINKER_LAUNCHER
    ${WASM_PROBE_COMPILER_LAUNCHER_BASE}
    --driver-kind em++ --
    CACHE STRING "Authenticated Emscripten C++ linker launcher" FORCE)

project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES C CXX)

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
    QuickControls2
    ShaderTools
    WebSockets
)

if(NOT DEFINED ENV{EMSDK_PYTHON})
    message(FATAL_ERROR "EMSDK_PYTHON must name the pinned SDK Python")
endif()
if(NOT DEFINED VCPKG_INSTALLED_DIR
        OR NOT DEFINED VCPKG_TARGET_TRIPLET)
    message(FATAL_ERROR
        "The dependency digest requires the vcpkg installed root/triplet")
endif()
set(WASM_PROBE_DEPENDENCY_ROOT
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
set(WASM_PROBE_DEPENDENCY_CONTRACT
    "${CMAKE_CURRENT_SOURCE_DIR}/dependency-archive-contract.json")
set(WASM_PROBE_DEPENDENCY_GENERATOR
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_dependency_digest.py")
set(WASM_PROBE_DEPENDENCY_MANIFEST
    "${WASM_PROBE_GENERATED_DIR}/dependency-archive-digest.json")
set(WASM_PROBE_DEPENDENCY_SOURCE
    "${WASM_PROBE_GENERATED_DIR}/ProbeDependencyDigest.cpp")
file(GLOB_RECURSE WASM_PROBE_DEPENDENCY_LINK_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${WASM_PROBE_DEPENDENCY_ROOT}/*.a"
    "${WASM_PROBE_DEPENDENCY_ROOT}/*.o")
if(NOT WASM_PROBE_DEPENDENCY_LINK_INPUTS)
    message(FATAL_ERROR
        "The target vcpkg installation contains no static link inputs")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${WASM_PROBE_DEPENDENCY_LINK_INPUTS}
    "${WASM_PROBE_DEPENDENCY_CONTRACT}"
    "${WASM_PROBE_DEPENDENCY_GENERATOR}")
execute_process(
    COMMAND
        "$ENV{EMSDK_PYTHON}" -I -B
        "${WASM_PROBE_DEPENDENCY_GENERATOR}"
        --contract "${WASM_PROBE_DEPENDENCY_CONTRACT}"
        --installed-root "${WASM_PROBE_DEPENDENCY_ROOT}"
        --output-cpp "${WASM_PROBE_DEPENDENCY_SOURCE}"
        --output-manifest "${WASM_PROBE_DEPENDENCY_MANIFEST}"
    RESULT_VARIABLE WASM_PROBE_DEPENDENCY_RESULT
    OUTPUT_VARIABLE WASM_PROBE_DEPENDENCY_DIGEST
    ERROR_VARIABLE WASM_PROBE_DEPENDENCY_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(LENGTH "${WASM_PROBE_DEPENDENCY_DIGEST}"
    WASM_PROBE_DEPENDENCY_DIGEST_LENGTH)
if(NOT WASM_PROBE_DEPENDENCY_RESULT EQUAL 0
        OR NOT WASM_PROBE_DEPENDENCY_DIGEST_LENGTH EQUAL 64
        OR NOT WASM_PROBE_DEPENDENCY_DIGEST
            MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR
        "Failed to bind target static archives: "
        "${WASM_PROBE_DEPENDENCY_ERROR}")
endif()

qt_standard_project_setup(REQUIRES 6.11)

add_library(WasmProbeWasmCompileOptions INTERFACE)
target_compile_definitions(WasmProbeWasmCompileOptions INTERFACE
    "RG_WASM_PROBE_COMPILE_INPUT_SHA256=${WASM_PROBE_INPUT_DIGEST}"
)
target_compile_options(WasmProbeWasmCompileOptions INTERFACE
    -pthread
    -fwasm-exceptions
    "SHELL:-sSUPPORT_LONGJMP=wasm"
)

add_executable(RhythmGameWasmCLauncherProbe
    src/LauncherProbe.c
)
target_link_libraries(RhythmGameWasmCLauncherProbe PRIVATE
    WasmProbeWasmCompileOptions
)
target_link_options(RhythmGameWasmCLauncherProbe PRIVATE
    -pthread
    -fwasm-exceptions
    "SHELL:-sSUPPORT_LONGJMP=wasm"
)
set_target_properties(RhythmGameWasmCLauncherProbe PROPERTIES
    AUTOMOC OFF
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/c-launcher-probe"
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
    "${WASM_PROBE_DEPENDENCY_SOURCE}"
    "${WASM_PROBE_GENERATED_DIR}/ProbeInputDigest.cpp"
    src/BrowserRuntimeBridge.cpp
    src/BrowserRuntimeBridge.h
    src/Gate1bReport.cpp
    src/Gate1bReport.h
    src/JspiNestedLoopProbe.cpp
    src/JspiNestedLoopProbe.h
    src/main.cpp
    src/MediaProbe.cpp
    src/MediaProbe.h
    src/NetworkProbe.cpp
    src/NetworkProbe.h
    src/ProbeState.cpp
    src/ProbeState.h
    src/RenderProbe.cpp
    src/RenderProbe.h
)
target_compile_features(RhythmGameWasmProbe PRIVATE cxx_std_20)
target_compile_definitions(RhythmGameWasmProbe PRIVATE
    _GNU_SOURCE
)
set_target_properties(RhythmGameWasmProbe PROPERTIES
    NO_WASM_DEFAULT_FILES TRUE
)

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
    Qt6::CorePrivate
    Qt6::Multimedia
    Qt6::Network
    Qt6::Quick
    Qt6::QuickControls2
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
    "SHELL:-sDYNAMIC_EXECUTION=0"
    "SHELL:-sEMBIND_AOT=1"
)

set(WASM_PROBE_RUNTIME_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/runtime")
set(WASM_PROBE_RUNTIME_STAGING_DIRECTORY
    "${CMAKE_CURRENT_BINARY_DIR}/runtime-packaging-inputs")
set(WASM_PROBE_RUNTIME_PACKAGER
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_runtime_artifacts.py")
set(WASM_PROBE_RUNTIME_HTML_TEMPLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/browser/web/RhythmGameWasmProbe.html.in")
set(WASM_PROBE_RUNTIME_CSS
    "${CMAKE_CURRENT_SOURCE_DIR}/browser/web/probe.css")
set(WASM_PROBE_RUNTIME_BOOTSTRAP
    "${CMAKE_CURRENT_SOURCE_DIR}/browser/web/bootstrap.mjs")
set(WASM_PROBE_RUNTIME_PREFLIGHT_WORKER
    "${CMAKE_CURRENT_SOURCE_DIR}/browser/web/preflight-worker.mjs")
set(WASM_PROBE_RUNTIME_MEDIA
    "${CMAKE_CURRENT_SOURCE_DIR}/browser/fixtures/probe.webm")
set(WASM_PROBE_QTLOADER
    "${WASM_PROBE_DEPENDENCY_ROOT}/plugins/platforms/qtloader.js")
set(WASM_PROBE_GENERATED_MAIN_JS
    "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.js")
set(WASM_PROBE_GENERATED_WASM
    "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.wasm")
set(WASM_PROBE_GENERATED_AUDIO_WORKLET
    "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.aw.js")
set(WASM_PROBE_GENERATED_WASM_WORKER
    "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.ww.js")
# NO_WASM_DEFAULT_FILES makes these prior Qt-generated shell files obsolete.
# Remove only these fixed leaves so the real link-directory shape audit remains
# exact without carrying Gate 1A leftovers into the Gate 1B package.
foreach(WASM_PROBE_LEGACY_SHELL IN ITEMS
        "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.html"
        "${CMAKE_CURRENT_BINARY_DIR}/qtloader.js"
        "${CMAKE_CURRENT_BINARY_DIR}/qtlogo.svg")
    if(IS_DIRECTORY "${WASM_PROBE_LEGACY_SHELL}")
        message(FATAL_ERROR
            "Legacy Wasm shell leaf is unexpectedly a directory: "
            "${WASM_PROBE_LEGACY_SHELL}")
    endif()
    if(EXISTS "${WASM_PROBE_LEGACY_SHELL}"
            OR IS_SYMLINK "${WASM_PROBE_LEGACY_SHELL}")
        file(REMOVE "${WASM_PROBE_LEGACY_SHELL}")
    endif()
endforeach()
set(WASM_PROBE_RUNTIME_MANIFEST
    "${WASM_PROBE_RUNTIME_DIRECTORY}/runtime-artifacts.json")
set(WASM_PROBE_RUNTIME_HTML
    "${WASM_PROBE_RUNTIME_DIRECTORY}/RhythmGameWasmProbe.html")
set(WASM_PROBE_STAGED_MAIN_JS
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/RhythmGameWasmProbe.js")
set(WASM_PROBE_STAGED_WASM
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/RhythmGameWasmProbe.wasm")
set(WASM_PROBE_STAGED_AUDIO_WORKLET
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/RhythmGameWasmProbe.aw.js")
set(WASM_PROBE_STAGED_WASM_WORKER
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/RhythmGameWasmProbe.ww.js")
set(WASM_PROBE_STAGED_HTML_TEMPLATE
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/RhythmGameWasmProbe.html.in")
set(WASM_PROBE_STAGED_CSS
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/probe.css")
set(WASM_PROBE_STAGED_BOOTSTRAP
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/bootstrap.mjs")
set(WASM_PROBE_STAGED_PREFLIGHT_WORKER
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/preflight-worker.mjs")
set(WASM_PROBE_STAGED_QTLOADER
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/qtloader.js")
set(WASM_PROBE_STAGED_MEDIA
    "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}/probe.webm")

foreach(WASM_PROBE_RUNTIME_INPUT IN ITEMS
        "${WASM_PROBE_RUNTIME_PACKAGER}"
        "${WASM_PROBE_RUNTIME_HTML_TEMPLATE}"
        "${WASM_PROBE_RUNTIME_CSS}"
        "${WASM_PROBE_RUNTIME_BOOTSTRAP}"
        "${WASM_PROBE_RUNTIME_PREFLIGHT_WORKER}"
        "${WASM_PROBE_RUNTIME_MEDIA}"
        "${WASM_PROBE_QTLOADER}")
    if(NOT EXISTS "${WASM_PROBE_RUNTIME_INPUT}"
            OR IS_DIRECTORY "${WASM_PROBE_RUNTIME_INPUT}")
        message(FATAL_ERROR
            "Missing Gate 1B runtime packaging input: "
            "${WASM_PROBE_RUNTIME_INPUT}")
    endif()
endforeach()

# The generated main script does not name the emitted .aw.js/.ww.js files
# before Task 5 stamping adds the guarded loader sites. Exact zero counts keep
# this Task 2 packaging step source-shape checked without guessing.
set(WASM_PROBE_AUDIO_WORKLET_LITERAL_COUNT 0)
set(WASM_PROBE_WASM_WORKER_LITERAL_COUNT 0)

set_property(TARGET RhythmGameWasmProbe APPEND PROPERTY LINK_DEPENDS
    "${WASM_PROBE_RUNTIME_PACKAGER}"
    "${WASM_PROBE_RUNTIME_HTML_TEMPLATE}"
    "${WASM_PROBE_RUNTIME_CSS}"
    "${WASM_PROBE_RUNTIME_BOOTSTRAP}"
    "${WASM_PROBE_RUNTIME_PREFLIGHT_WORKER}"
    "${WASM_PROBE_RUNTIME_MEDIA}"
    "${WASM_PROBE_QTLOADER}"
)

add_custom_command(TARGET RhythmGameWasmProbe POST_BUILD
    COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_GENERATED_MAIN_JS}"
        "${WASM_PROBE_STAGED_MAIN_JS}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_GENERATED_WASM}"
        "${WASM_PROBE_STAGED_WASM}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_GENERATED_AUDIO_WORKLET}"
        "${WASM_PROBE_STAGED_AUDIO_WORKLET}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_GENERATED_WASM_WORKER}"
        "${WASM_PROBE_STAGED_WASM_WORKER}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_RUNTIME_HTML_TEMPLATE}"
        "${WASM_PROBE_STAGED_HTML_TEMPLATE}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_RUNTIME_CSS}"
        "${WASM_PROBE_STAGED_CSS}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_RUNTIME_BOOTSTRAP}"
        "${WASM_PROBE_STAGED_BOOTSTRAP}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_RUNTIME_PREFLIGHT_WORKER}"
        "${WASM_PROBE_STAGED_PREFLIGHT_WORKER}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_QTLOADER}"
        "${WASM_PROBE_STAGED_QTLOADER}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${WASM_PROBE_RUNTIME_MEDIA}"
        "${WASM_PROBE_STAGED_MEDIA}"
    COMMAND
        "$ENV{EMSDK_PYTHON}" -I -B
        "${WASM_PROBE_RUNTIME_PACKAGER}"
        --build-id "${WASM_PROBE_INPUT_DIGEST}"
        --generated-dir "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}"
        --generated-shape-dir "${CMAKE_CURRENT_BINARY_DIR}"
        --output-dir "${WASM_PROBE_RUNTIME_DIRECTORY}"
        --html-template "${WASM_PROBE_STAGED_HTML_TEMPLATE}"
        --css "${WASM_PROBE_STAGED_CSS}"
        --bootstrap "${WASM_PROBE_STAGED_BOOTSTRAP}"
        --preflight-worker "${WASM_PROBE_STAGED_PREFLIGHT_WORKER}"
        --qtloader "${WASM_PROBE_STAGED_QTLOADER}"
        --media "${WASM_PROBE_STAGED_MEDIA}"
        --expected-audio-worklet-occurrences
            "${WASM_PROBE_AUDIO_WORKLET_LITERAL_COUNT}"
        --expected-wasm-worker-occurrences
            "${WASM_PROBE_WASM_WORKER_LITERAL_COUNT}"
    BYPRODUCTS
        "${WASM_PROBE_RUNTIME_STAGING_DIRECTORY}"
        "${WASM_PROBE_STAGED_MAIN_JS}"
        "${WASM_PROBE_STAGED_WASM}"
        "${WASM_PROBE_STAGED_AUDIO_WORKLET}"
        "${WASM_PROBE_STAGED_WASM_WORKER}"
        "${WASM_PROBE_STAGED_HTML_TEMPLATE}"
        "${WASM_PROBE_STAGED_CSS}"
        "${WASM_PROBE_STAGED_BOOTSTRAP}"
        "${WASM_PROBE_STAGED_PREFLIGHT_WORKER}"
        "${WASM_PROBE_STAGED_QTLOADER}"
        "${WASM_PROBE_STAGED_MEDIA}"
        "${WASM_PROBE_RUNTIME_DIRECTORY}"
        "${WASM_PROBE_RUNTIME_MANIFEST}"
        "${WASM_PROBE_RUNTIME_HTML}"
    COMMENT "Packaging content-addressed Gate 1B runtime artifacts"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

add_custom_target(RhythmGameWasmProbeRuntimePackage ALL
    DEPENDS
        "${WASM_PROBE_RUNTIME_PACKAGER}"
        "${WASM_PROBE_RUNTIME_HTML_TEMPLATE}"
        "${WASM_PROBE_RUNTIME_CSS}"
        "${WASM_PROBE_RUNTIME_BOOTSTRAP}"
        "${WASM_PROBE_RUNTIME_PREFLIGHT_WORKER}"
        "${WASM_PROBE_RUNTIME_MEDIA}"
        "${WASM_PROBE_QTLOADER}"
)
add_dependencies(
    RhythmGameWasmProbeRuntimePackage
    RhythmGameWasmProbe
)

# Stock CMake cannot declare post-link digest-derived leaf names as individual
# BYPRODUCTS. This non-default target therefore verifies the manifest-modeled
# exact directory inventory and every declared artifact without rewriting it.
add_custom_target(RhythmGameWasmProbeRuntimeVerify
    COMMAND
        "$ENV{EMSDK_PYTHON}" -I -B
        "${WASM_PROBE_RUNTIME_PACKAGER}"
        --verify-output-dir "${WASM_PROBE_RUNTIME_DIRECTORY}"
        --expected-build-id "${WASM_PROBE_INPUT_DIGEST}"
    DEPENDS
        "${WASM_PROBE_RUNTIME_PACKAGER}"
    COMMENT "Verifying exact Gate 1B runtime artifact inventory"
    VERBATIM
)
add_dependencies(
    RhythmGameWasmProbeRuntimeVerify
    RhythmGameWasmProbeRuntimePackage
)
