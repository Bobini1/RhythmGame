vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

set(backbeat_revision 8e0ca2b441b7898c7255b756f658d69a51789509)
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO zkldi/backbeat
    REF "${backbeat_revision}"
    SHA512 273221515cbf0c253b1b968e144ba0f1ac515c1f671d6f4f57f4cce82730f18a08d6c6f3774a50e05824d3d915d42e7875eb4221293c31f9d815155d7548f754
    PATCHES cmake-package.patch cargo-native-dependencies.patch
)

# pkg-config-rs has no option for rustc's static:-bundle modifier. This small
# packaging patch keeps discovered static libraries as external dependencies.
# Cargo's registry cache and all locked dependency versions remain unchanged.
vcpkg_download_distfile(pkg_config_archive
    URLS "https://static.crates.io/crates/pkg-config/pkg-config-0.3.33.crate"
    FILENAME pkg-config-0.3.33.crate
    SHA512 af931d889e72f51e0ae41c880a1f5aa6215b93148cf7041378664e4ba013d0c83d654e560a73c9d7b301b5a008c221e48ac81b413d1123786e003c97f7fff63a
)
vcpkg_extract_source_archive(pkg_config_source
    ARCHIVE "${pkg_config_archive}"
    PATCHES pkg-config-static-linkage.patch
)
file(COPY "${pkg_config_source}/" DESTINATION "${SOURCE_PATH}/vcpkg/pkg-config")

find_program(RUSTUP NAMES rustup HINTS "$ENV{CARGO_HOME}/bin" "$ENV{USERPROFILE}/.cargo/bin" "$ENV{HOME}/.cargo/bin")
if(NOT RUSTUP)
    message(FATAL_ERROR "Building Backbeat requires Rustup and Rust 1.96.0. Install Rustup, then run: rustup toolchain install 1.96.0 --profile minimal")
endif()
vcpkg_execute_required_process(
    COMMAND "${RUSTUP}" run 1.96.0 rustc -vV
    WORKING_DIRECTORY "${SOURCE_PATH}"
    LOGNAME rust-version-${TARGET_TRIPLET}
)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(rust_arch x86_64)
else()
    set(rust_arch aarch64)
endif()
if(VCPKG_TARGET_IS_WINDOWS)
    set(rust_target "${rust_arch}-pc-windows-msvc")
elseif(VCPKG_TARGET_IS_OSX)
    set(rust_target "${rust_arch}-apple-darwin")
else()
    set(rust_target "${rust_arch}-unknown-linux-gnu")
endif()

# Triplet names can differ while describing the same native platform.
file(READ "${CURRENT_BUILDTREES_DIR}/rust-version-${TARGET_TRIPLET}-out.log" rust_version)
string(REGEX MATCH "host: ([^\r\n]+)" rust_host_line "${rust_version}")
set(rust_host "${CMAKE_MATCH_1}")
if(NOT rust_host STREQUAL rust_target)
    message(FATAL_ERROR "The Backbeat overlay requires a native build: Rust host '${rust_host}' does not match SDK target '${rust_target}'.")
endif()

# SQLx checks queries during compilation. Install its CLI privately, using
# its own lockfile and bundled host SQLite, before configuring target libraries.
set(host_tools "${CURRENT_BUILDTREES_DIR}/host-tools")
vcpkg_execute_required_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "CARGO_TARGET_DIR=${CURRENT_BUILDTREES_DIR}/sqlx-target"
        "${RUSTUP}" run 1.96.0 cargo install sqlx-cli --version 0.9.0 --locked
        --no-default-features --features sqlite --root "${host_tools}"
        --jobs "${VCPKG_CONCURRENCY}"
    WORKING_DIRECTORY "${SOURCE_PATH}"
    LOGNAME sqlx-install-${HOST_TRIPLET}
)
vcpkg_add_to_path("${host_tools}/bin")
get_filename_component(rustup_bin "${RUSTUP}" DIRECTORY)
vcpkg_add_to_path("${rustup_bin}")
vcpkg_find_acquire_program(PKGCONFIG)
vcpkg_find_acquire_program(CLANG)
get_filename_component(clang_bin "${CLANG}" DIRECTORY)
vcpkg_add_to_path("${clang_bin}")
if(VCPKG_HOST_IS_WINDOWS)
    set(ENV{LIBCLANG_PATH} "${clang_bin}")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${CURRENT_PORT_DIR}"
    OPTIONS
        "-DBACKBEAT_SOURCE_DIR=${SOURCE_PATH}"
        "-DBACKBEAT_INSTALLED_DIR=${CURRENT_INSTALLED_DIR}"
        "-DBACKBEAT_RUST_TARGET=${rust_target}"
        "-DBACKBEAT_REVISION=${backbeat_revision}"
        "-DBACKBEAT_CARGO_TARGET_DIR=${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-cargo"
        "-DBACKBEAT_BUILD_JOBS=${VCPKG_CONCURRENCY}"
        "-DRUSTUP=${RUSTUP}"
        "-DPKG_CONFIG_EXECUTABLE=${PKGCONFIG}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Backbeat CONFIG_PATH lib/cmake/Backbeat)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md"
    COMMENT "This static library includes Rust dependencies. Their versions and sources are recorded in Cargo.lock at https://github.com/zkldi/backbeat/tree/${backbeat_revision}; consult each crate's license and copyright notices as well.")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
