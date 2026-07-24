if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "RhythmGameWasmProbe requires Emscripten")
endif()
if(NOT "$ENV{EMSCRIPTEN_VERSION}" STREQUAL "4.0.7")
    message(FATAL_ERROR
        "Expected EMSCRIPTEN_VERSION=4.0.7, got "
        "'$ENV{EMSCRIPTEN_VERSION}'")
endif()
if(NOT VCPKG_TARGET_TRIPLET STREQUAL "wasm32-emscripten-rg")
    message(FATAL_ERROR "Unexpected target triplet: ${VCPKG_TARGET_TRIPLET}")
endif()
if(NOT VCPKG_HOST_TRIPLET STREQUAL "x64-windows-rg-host-release")
    message(FATAL_ERROR "Unexpected host triplet: ${VCPKG_HOST_TRIPLET}")
endif()
