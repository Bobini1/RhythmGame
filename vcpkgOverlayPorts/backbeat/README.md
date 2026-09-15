# Backbeat overlay

This port builds the C SDK from a pinned upstream commit, with the CMake package
fix carried as a patch. No local Backbeat checkout
or unpublished fork commit is required to install it.

Install Rustup and the compiler pinned by Backbeat before building:

```console
rustup toolchain install 1.96.0 --profile minimal
```

The first build needs access to crates.io. The port builds its own SQLx CLI in
the vcpkg build tree. On Windows, vcpkg locates or downloads Clang for SQLite's
binding generator. Linux and macOS need Clang and libclang installed (for
example, `clang libclang-dev` on Debian/Ubuntu or Homebrew `llvm` on macOS).

The port supports native x64/arm64 builds on Windows MSVC, Linux GNU, and macOS.
It produces a static SDK; Windows uses the dynamic CRT. Cross-compilation is
currently unsupported because SQLx loads SQLite during compilation. Release
and Debug archives are built unless the triplet requests Release only.

Native dependency linkage follows the installed CMake targets. Static SQLite,
zstd, and OpenSSL stay outside the Rust archive using Rust's documented
`static:-bundle` modifier, and the final CMake link supplies them. Shared
dependencies remain shared. The build exposes the selected configuration's
runtime directory to SQLx's compile-time database checks.

The port uses Cargo's native-library override for zstd. For SQLite and OpenSSL,
their build scripts still perform the header/version checks; a small patch to
the pinned `pkg-config-rs` crate changes static link metadata to `static:-bundle`.
The crate is SHA512-verified and patched in vcpkg's build tree. The lockfile
changes only that crate's source to the private copy; dependency versions stay
pinned and builds use `--locked`. No Cargo registry cache is modified.

The CMake package is generated with `configure_package_config_file`, installed
through CMake, and relocated with `vcpkg_cmake_config_fixup`. It shares the
upstream SDK's target definitions.

Enable the optional `backbeat` manifest feature alongside any existing features,
for example:

```console
cmake --preset=dev-rel -DVCPKG_MANIFEST_FEATURES="test;docs;backbeat"
```

Consumer CMake code:

```cmake
find_package(Backbeat CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Backbeat::Backbeat)
```

Include `<backbeat.h>`. The target supplies SQLite, zstd, and platform libraries;
consumers do not need a separate `Backbeat::SQLite` link. The overlay uses
vcpkg's FTS5-enabled SQLite and zstd, leaving both outside the Rust archive.

The feature makes the SDK available for integration. It does not add chart-store
support to RhythmGame's scanner or gameplay code.
