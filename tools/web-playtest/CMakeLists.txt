cmake_minimum_required(VERSION 3.25)

set(RG_WEB_PLAYTEST_LAUNCHER_ENVIRONMENT
    EMSDK_PYTHON
    RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER
    RHYTHMGAME_WASM_TOOLCHAIN_LOCK
    RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR
    RHYTHMGAME_EMSCRIPTEN_ROOT
    RHYTHMGAME_EM_CONFIG
    RHYTHMGAME_EM_CONFIG_SHA256
    RHYTHMGAME_EM_CACHE
)
foreach(launcher_variable IN LISTS
        RG_WEB_PLAYTEST_LAUNCHER_ENVIRONMENT)
    if(NOT DEFINED ENV{${launcher_variable}}
            OR "$ENV{${launcher_variable}}" STREQUAL "")
        message(FATAL_ERROR
            "Missing authenticated compiler-launcher environment: "
            "${launcher_variable}")
    endif()
endforeach()
set(RG_WEB_PLAYTEST_COMPILER_LAUNCHER_BASE
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
    ${RG_WEB_PLAYTEST_COMPILER_LAUNCHER_BASE}
    --driver-kind emcc --
    CACHE STRING "Authenticated Emscripten C compiler launcher" FORCE)
set(CMAKE_CXX_COMPILER_LAUNCHER
    ${RG_WEB_PLAYTEST_COMPILER_LAUNCHER_BASE}
    --driver-kind em++ --
    CACHE STRING "Authenticated Emscripten C++ compiler launcher" FORCE)
set(CMAKE_C_LINKER_LAUNCHER
    ${RG_WEB_PLAYTEST_COMPILER_LAUNCHER_BASE}
    --driver-kind emcc --
    CACHE STRING "Authenticated Emscripten C linker launcher" FORCE)
set(CMAKE_CXX_LINKER_LAUNCHER
    ${RG_WEB_PLAYTEST_COMPILER_LAUNCHER_BASE}
    --driver-kind em++ --
    CACHE STRING "Authenticated Emscripten C++ linker launcher" FORCE)

project(RhythmGameWebPlaytest VERSION 1.3.13 LANGUAGES C CXX)

include(cmake/verify_exact_toolchain.cmake)

get_filename_component(
    RG_WEB_PLAYTEST_REPO_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    ABSOLUTE
)
if(DEFINED CACHE{RG_WEB_PLAYTEST_CHART_DIR})
    get_property(RG_WEB_PLAYTEST_CHART_DIR_CACHE_TYPE
        CACHE RG_WEB_PLAYTEST_CHART_DIR PROPERTY TYPE)
    if(NOT RG_WEB_PLAYTEST_CHART_DIR_CACHE_TYPE STREQUAL "PATH")
        message(FATAL_ERROR
            "RG_WEB_PLAYTEST_CHART_DIR must use an explicitly typed, "
            "forward-slash PATH argument, for example "
            "-DRG_WEB_PLAYTEST_CHART_DIR:PATH=T:/absolute/chart/root")
    endif()
endif()
set(RG_WEB_PLAYTEST_CHART_DIR "" CACHE PATH
    "Directory of the private chart package embedded in this playtest build")
set(RG_WEB_PLAYTEST_CHART_RELATIVE_PATH "" CACHE STRING
    "BMS/BME/BML/PMS path relative to RG_WEB_PLAYTEST_CHART_DIR")
if(RG_WEB_PLAYTEST_CHART_DIR STREQUAL ""
        OR RG_WEB_PLAYTEST_CHART_RELATIVE_PATH STREQUAL "")
    message(FATAL_ERROR
        "RG_WEB_PLAYTEST_CHART_DIR and "
        "RG_WEB_PLAYTEST_CHART_RELATIVE_PATH are required")
endif()
if(RG_WEB_PLAYTEST_CHART_DIR MATCHES "\\\\"
        OR NOT IS_ABSOLUTE "${RG_WEB_PLAYTEST_CHART_DIR}")
    message(FATAL_ERROR
        "RG_WEB_PLAYTEST_CHART_DIR must be an absolute PATH using "
        "forward slashes")
endif()
get_filename_component(
    RG_WEB_PLAYTEST_CHART_DIR
    "${RG_WEB_PLAYTEST_CHART_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

file(GLOB_RECURSE RG_WEB_PLAYTEST_CHART_CONFIGURE_INPUTS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES true
    "${RG_WEB_PLAYTEST_CHART_DIR}/*"
)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${RG_WEB_PLAYTEST_CHART_DIR}"
    ${RG_WEB_PLAYTEST_CHART_CONFIGURE_INPUTS}
)

set(RG_WEB_PLAYTEST_GENERATED_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${RG_WEB_PLAYTEST_GENERATED_DIR}")
set(RG_WEB_PLAYTEST_PYTHON "$ENV{EMSDK_PYTHON}")
set(RG_WEB_PLAYTEST_CHART_GENERATOR
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_chart_package.py")
set(RG_WEB_PLAYTEST_CHART_STAGING_DIR
    "${RG_WEB_PLAYTEST_GENERATED_DIR}/chart-staging")
set(RG_WEB_PLAYTEST_CHART_MANIFEST_TEMPLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WebPlaytestChartManifest.json.in")
set(RG_WEB_PLAYTEST_CHART_MANIFEST
    "${RG_WEB_PLAYTEST_GENERATED_DIR}/web-playtest-chart-manifest.json")
set(RG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE
    "${RG_WEB_PLAYTEST_GENERATED_DIR}/chart-package.cmake")
include(cmake/GenerateWebPlaytestChart.cmake)

set(RG_WEB_PLAYTEST_INPUT_MANIFEST
    "${CMAKE_CURRENT_SOURCE_DIR}/input-manifest.txt")
file(STRINGS "${RG_WEB_PLAYTEST_INPUT_MANIFEST}"
    RG_WEB_PLAYTEST_INPUTS ENCODING UTF-8)
set(RG_WEB_PLAYTEST_SORTED_INPUTS ${RG_WEB_PLAYTEST_INPUTS})
list(SORT RG_WEB_PLAYTEST_SORTED_INPUTS
    COMPARE STRING CASE INSENSITIVE ORDER ASCENDING)
if(NOT RG_WEB_PLAYTEST_INPUTS STREQUAL
        RG_WEB_PLAYTEST_SORTED_INPUTS)
    message(FATAL_ERROR
        "input-manifest.txt must contain unique case-insensitively sorted paths")
endif()
file(READ "${RG_WEB_PLAYTEST_INPUT_MANIFEST}"
    RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT)
string(REPLACE "\r\n" "\n"
    RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT
    "${RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT}")
string(REPLACE "\r" "\n"
    RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT
    "${RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT}")
string(SHA256 RG_WEB_PLAYTEST_INPUT_MANIFEST_SHA256
    "${RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT}")
unset(RG_WEB_PLAYTEST_INPUT_MANIFEST_CONTENT)
set(RG_WEB_PLAYTEST_INPUT_DIGEST_PAYLOAD
    "manifest=${RG_WEB_PLAYTEST_INPUT_MANIFEST_SHA256}\n"
    "chartManifest=${RG_WEB_PLAYTEST_CHART_MANIFEST_SHA256}\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${RG_WEB_PLAYTEST_INPUT_MANIFEST}")
foreach(input IN LISTS RG_WEB_PLAYTEST_INPUTS)
    if(input STREQUAL ""
            OR input MATCHES "(^|/)\\.\\.(/|$)"
            OR input MATCHES "^[/\\\\]"
            OR input MATCHES "^[A-Za-z]:"
            OR input MATCHES "\\\\")
        message(FATAL_ERROR "Unsafe input-manifest path: '${input}'")
    endif()
    string(TOLOWER "${input}" input_key)
    string(SHA256 input_key_sha256 "${input_key}")
    if(DEFINED "RG_WEB_PLAYTEST_SEEN_${input_key_sha256}")
        message(FATAL_ERROR
            "Duplicate input-manifest path: '${input}'")
    endif()
    set("RG_WEB_PLAYTEST_SEEN_${input_key_sha256}" TRUE)
    set(input_path "${RG_WEB_PLAYTEST_REPO_ROOT}/${input}")
    if(NOT EXISTS "${input_path}"
            OR IS_DIRECTORY "${input_path}"
            OR IS_SYMLINK "${input_path}")
        message(FATAL_ERROR
            "Missing or unsafe input-manifest file: '${input}'")
    endif()
    file(SHA256 "${input_path}" input_sha256)
    string(APPEND RG_WEB_PLAYTEST_INPUT_DIGEST_PAYLOAD
        "${input}=${input_sha256}\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${input_path}")
endforeach()
string(SHA256 RG_WEB_PLAYTEST_BUILD_INPUT_SHA256
    "${RG_WEB_PLAYTEST_INPUT_DIGEST_PAYLOAD}")
configure_file(
    cmake/WebPlaytestInputDigest.cpp.in
    "${RG_WEB_PLAYTEST_GENERATED_DIR}/WebPlaytestInputDigest.cpp"
    @ONLY
)

find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS
    Core
    Qml
    Quick
    QuickControls2
    ShaderTools
)
find_package(Boost REQUIRED COMPONENTS headers)
find_package(fmt CONFIG REQUIRED)
find_package(lexy CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(Iconv REQUIRED)
find_path(STB_INCLUDE_DIRS NAMES stb_vorbis.c REQUIRED)

qt_standard_project_setup(REQUIRES 6.11)

set(RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT ON
    CACHE BOOL "Require the web gameplay dependency audit" FORCE)
include(${RG_WEB_PLAYTEST_REPO_ROOT}/cmake/WebGameplayCore.cmake)
rhythmgame_add_web_gameplay_core(RhythmGame_web_gameplay_core)

set(RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT ON
    CACHE BOOL "Require the web audio dependency audit" FORCE)
include(${RG_WEB_PLAYTEST_REPO_ROOT}/cmake/WebAudioCore.cmake)
rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)

qt_add_executable(RhythmGameWasmProbe
    MANUAL_FINALIZATION
    "${RG_WEB_PLAYTEST_GENERATED_DIR}/WebPlaytestInputDigest.cpp"
    src/WebPlaytestChartInstaller.cpp
    src/WebPlaytestChartInstaller.h
    src/WebPlaytestInputDigest.h
    src/main.cpp
)
target_compile_features(RhythmGameWasmProbe PRIVATE cxx_std_23)
target_compile_options(RhythmGameWasmProbe PRIVATE
    -pthread
    -fwasm-exceptions
    "SHELL:-sSUPPORT_LONGJMP=wasm"
)
target_include_directories(RhythmGameWasmProbe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(RhythmGameWasmProbe PROPERTIES
    NO_WASM_DEFAULT_FILES TRUE
    QT_WASM_INITIAL_MEMORY 268435456
    QT_WASM_MAXIMUM_MEMORY 1073741824
)

qt_add_qml_module(RhythmGameWasmProbe
    URI RhythmGame.WebPlaytest
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
)

qt_add_resources(RhythmGameWasmProbe "web_playtest_chart"
    BIG_RESOURCES
    PREFIX "/web-playtest/chart"
    BASE "${RG_WEB_PLAYTEST_CHART_STAGING_DIR}"
    FILES ${RG_WEB_PLAYTEST_STAGED_CHART_FILES}
    OPTIONS -no-compress
)
qt_add_resources(RhythmGameWasmProbe "web_playtest_chart_manifest"
    PREFIX "/web-playtest"
    BASE "${RG_WEB_PLAYTEST_GENERATED_DIR}"
    FILES "${RG_WEB_PLAYTEST_CHART_MANIFEST}"
    OPTIONS -no-compress
)

target_link_libraries(RhythmGameWasmProbe PRIVATE
    RhythmGame_web_gameplay_core
    RhythmGame_web_audio_core
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)
target_link_options(RhythmGameWasmProbe PRIVATE
    -pthread
    -fwasm-exceptions
    "SHELL:-sSUPPORT_LONGJMP=wasm"
    "SHELL:-sJSPI"
    "SHELL:-sAUDIO_WORKLET=1"
    "SHELL:-sWASM_WORKERS=1"
    "SHELL:-sPTHREAD_POOL_SIZE=4"
    "SHELL:-sPTHREAD_POOL_SIZE_STRICT=2"
    "SHELL:-sALLOW_BLOCKING_ON_MAIN_THREAD=0"
    "SHELL:-sDYNAMIC_EXECUTION=0"
    "SHELL:-sEMBIND_AOT=1"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
)

set(RG_WEB_PLAYTEST_ARTIFACT_SCANNER
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/scan_artifacts_for_host_path.py")
set_property(TARGET RhythmGameWasmProbe APPEND PROPERTY LINK_DEPENDS
    "${RG_WEB_PLAYTEST_ARTIFACT_SCANNER}")
add_custom_command(TARGET RhythmGameWasmProbe PRE_LINK
    COMMAND
        "$ENV{EMSDK_PYTHON}" -I -B
        "${RG_WEB_PLAYTEST_ARTIFACT_SCANNER}"
        --host-root "${RG_WEB_PLAYTEST_CHART_DIR}"
        --generated-input
            "${RG_WEB_PLAYTEST_GENERATED_DIR}/WebPlaytestInputDigest.cpp"
        --generated-input
            "${RG_WEB_PLAYTEST_CHART_MANIFEST}"
        --generated-input
            "${RG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE}"
        --generated-input
            "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc/web_playtest_chart.qrc"
        --generated-input
            "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc/qrc_web_playtest_chart_tmp.cpp"
        --generated-input
            "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc/web_playtest_chart_manifest.qrc"
        --generated-input
            "${CMAKE_CURRENT_BINARY_DIR}/.qt/rcc/qrc_web_playtest_chart_manifest.cpp"
    COMMENT "Scanning generated C++ and manifests for private host paths"
    VERBATIM
)
add_custom_command(TARGET RhythmGameWasmProbe POST_BUILD
    COMMAND
        "$ENV{EMSDK_PYTHON}" -I -B
        "${RG_WEB_PLAYTEST_ARTIFACT_SCANNER}"
        --host-root "${RG_WEB_PLAYTEST_CHART_DIR}"
        --artifact
            "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.js"
        --artifact
            "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.wasm"
        --artifact
            "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.aw.js"
        --artifact
            "${CMAKE_CURRENT_BINARY_DIR}/RhythmGameWasmProbe.ww.js"
    COMMENT "Scanning the four Wasm link leaves for private host paths"
    VERBATIM
)

qt_import_qml_plugins(RhythmGameWasmProbe)
qt_finalize_executable(RhythmGameWasmProbe)
