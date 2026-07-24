cmake_minimum_required(VERSION 3.24)

project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES CXX)

include(cmake/verify_exact_toolchain.cmake)

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

add_library(WasmProbeExceptionBoundary STATIC
    src/ExceptionBoundary.cpp
    src/ExceptionBoundary.h
)
target_compile_features(WasmProbeExceptionBoundary PRIVATE cxx_std_20)

qt_add_executable(RhythmGameWasmProbe
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
