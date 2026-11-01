cmake_minimum_required(VERSION 3.25)
project(backbeat_vcpkg LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(cargo_profile debug)
    set(native_prefix "${BACKBEAT_INSTALLED_DIR}/debug")
else()
    set(cargo_profile release)
    set(cargo_profile_args --release)
    set(native_prefix "${BACKBEAT_INSTALLED_DIR}")
endif()
set(native_library_dir "${native_prefix}/lib")

# Respect the dependency targets, which may be shared even though the SDK is
# static. A static Rust dependency must use -bundle to defer the final C link.
get_target_property(sqlite_type unofficial::sqlite3::sqlite3 TYPE)
if(sqlite_type STREQUAL "STATIC_LIBRARY")
    set(sqlite_linkage SQLITE3_STATIC=1)
else()
    set(sqlite_linkage SQLITE3_DYNAMIC=1)
endif()
if(TARGET zstd::libzstd_shared)
    set(zstd_link_kind dylib)
else()
    set(zstd_link_kind static:-bundle)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
    if(OPENSSL_CRYPTO_LIBRARY MATCHES "\\.a$")
        set(openssl_linkage OPENSSL_STATIC=1)
    else()
        set(openssl_linkage OPENSSL_DYNAMIC=1)
    endif()
endif()
configure_file(cargo-config.toml.in cargo-config.toml @ONLY)

string(ASCII 31 rust_flag_separator)
if(WIN32)
    if(VCPKG_CRT_LINKAGE STREQUAL "static")
        set(rust_flags "-C${rust_flag_separator}target-feature=+crt-static")
    else()
        set(rust_flags "-C${rust_flag_separator}target-feature=-crt-static")
    endif()
else()
    set(rust_flags "-C${rust_flag_separator}linker=${CMAKE_C_COMPILER}")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" config_upper)
set(c_flags "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${config_upper}}")
set(backbeat_archive "${BACKBEAT_CARGO_TARGET_DIR}/${BACKBEAT_RUST_TARGET}/${cargo_profile}/${CMAKE_STATIC_LIBRARY_PREFIX}backbeat_c_sdk${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(backbeat_header "${CMAKE_CURRENT_BINARY_DIR}/backbeat.h")

add_custom_target(backbeat ALL
    COMMAND "${CMAKE_COMMAND}" -E env
        --unset=OPENSSL_DIR --unset=OPENSSL_LIB_DIR --unset=OPENSSL_INCLUDE_DIR
        --modify "PATH=path_list_prepend:${BACKBEAT_INSTALLED_DIR}/bin"
        --modify "PATH=path_list_prepend:${native_prefix}/bin"
        --modify "LD_LIBRARY_PATH=path_list_prepend:${native_library_dir}"
        --modify "DYLD_FALLBACK_LIBRARY_PATH=path_list_prepend:${native_library_dir}"
        "CARGO_TARGET_DIR=${BACKBEAT_CARGO_TARGET_DIR}"
        "CARGO_ENCODED_RUSTFLAGS=${rust_flags}"
        "CC_${BACKBEAT_RUST_TARGET}=${CMAKE_C_COMPILER}"
        "CFLAGS_${BACKBEAT_RUST_TARGET}=${c_flags}"
        "SQLITE3_INCLUDE_DIR=${BACKBEAT_INSTALLED_DIR}/include"
        "SQLITE3_LIB_DIR=${native_library_dir}"
        "${sqlite_linkage}"
        "PKG_CONFIG=${PKG_CONFIG_EXECUTABLE}"
        "PKG_CONFIG_PATH=${native_library_dir}/pkgconfig"
        "PKG_CONFIG_LIBDIR=${native_library_dir}/pkgconfig"
        ${openssl_linkage}
        "BKB_LIBCOMMIT_HASH=${BACKBEAT_REVISION}"
        "BKB_HEADER_PATH=${backbeat_header}"
        "${RUSTUP}" run 1.96.0 cargo rustc --locked
        --config "${CMAKE_CURRENT_BINARY_DIR}/cargo-config.toml"
        --target "${BACKBEAT_RUST_TARGET}" --package backbeat_c_sdk
        --lib --crate-type staticlib --jobs "${BACKBEAT_BUILD_JOBS}"
        ${cargo_profile_args} -- --print native-static-libs
    BYPRODUCTS "${backbeat_archive}" "${backbeat_header}"
    WORKING_DIRECTORY "${BACKBEAT_SOURCE_DIR}"
    VERBATIM
)

install(FILES "${backbeat_archive}" DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES "${backbeat_header}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" CONFIGURATIONS Release)
configure_package_config_file(BackbeatConfig.cmake.in BackbeatConfig.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Backbeat"
    NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BackbeatConfig.cmake"
    "${BACKBEAT_SOURCE_DIR}/rust/backbeat_c_sdk/cmake/BackbeatTargets.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Backbeat")
