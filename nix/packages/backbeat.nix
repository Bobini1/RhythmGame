{
  lib,
  stdenv,
  rustPlatform,
  fetchFromGitHub,
  pkg-config,
  openssl,
  sqlite,
  zstd,
}:
rustPlatform.buildRustPackage rec {
  pname = "backbeat";
  version = "0.5.1";

  # Keep the SDK revision in sync with vcpkgOverlayPorts/backbeat/portfile.cmake.
  src = fetchFromGitHub {
    owner = "zkldi";
    repo = "backbeat";
    rev = "3ed0c353d912cfcb671920c6d88f5281ce85a49f";
    hash = "sha256-YJ6N3/cXBNinzcssIAdSas2YL82+8sMCJa0OpniP024=";
  };

  cargoHash = "sha256-wOH0/7DE/yi/hk6wYwfcKbCslK/P4P0BfcJJe52aHAA=";
  cargoBuildFlags = ["--package" "backbeat_c_sdk" "--lib"];
  cargoTestFlags = ["--package" "backbeat_c_sdk" "--lib"];

  nativeBuildInputs = [pkg-config rustPlatform.bindgenHook];
  propagatedBuildInputs = [openssl sqlite zstd];

  env = {
    BKB_LIBCOMMIT_HASH = src.rev;
    SQLITE3_LIB_DIR = "${lib.getLib sqlite}/lib";
    SQLITE3_INCLUDE_DIR = "${lib.getDev sqlite}/include";
    SQLITE3_DYNAMIC = "1";
    ZSTD_SYS_USE_PKG_CONFIG = "1";
    OPENSSL_NO_VENDOR = "1";
  };

  postPatch = ''
    # Use Nix's compiler wrapper instead of the upstream clang/lld override.
    substituteInPlace .cargo/config.toml \
      --replace-fail 'rustflags = ["-C", "linker=clang", "-C", "link-arg=-fuse-ld=lld"]' 'rustflags = []'
    substituteInPlace rust/backbeat_c_sdk/Cargo.toml \
      --replace-fail 'crate-type = ["staticlib", "cdylib", "rlib"]' 'crate-type = ["staticlib"]'
  '';

  preBuild = ''
    export BKB_HEADER_PATH="$TMPDIR/backbeat.h"
  '';

  installPhase = ''
    runHook preInstall
    install -Dm644 target/${stdenv.hostPlatform.rust.rustcTarget}/release/libbackbeat_c_sdk.a \
      "$out/lib/libbackbeat_c_sdk.a"
    install -Dm644 "$BKB_HEADER_PATH" "$out/include/backbeat.h"
    install -Dm644 rust/backbeat_c_sdk/cmake/BackbeatTargets.cmake \
      "$out/lib/cmake/Backbeat/BackbeatTargets.cmake"
    install -Dm644 ${./BackbeatConfig.cmake} "$out/lib/cmake/Backbeat/BackbeatConfig.cmake"
    install -Dm644 LICENSE.md "$out/share/licenses/backbeat/LICENSE.md"
    runHook postInstall
  '';

  meta = with lib; {
    description = "Backbeat C SDK for installed rhythm-game charts and collections";
    homepage = "https://github.com/zkldi/backbeat";
    license = licenses.gpl3Only;
    platforms = platforms.linux;
    maintainers = [maintainers.Bobini1];
  };
}
