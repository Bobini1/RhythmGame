{
  lib,
  stdenv,
  fetchFromGitHub,
  wrapQtAppsHook,
  cmake,
  qtwayland,
  qtshadertools,
  zstd,
  magic-enum,
  SDL2,
  boost,
  sqlitecpp,
  fmt,
  libxml2,
  qtdeclarative,
  qtmultimedia,
  qtsvg,
  qtinterfaceframework,
  ned14-llfio,
  lexy,
  mimalloc,
  spdlog,
  qttools,
  libsndfile,
  stb,
  pkg-config,
  tbb,
  miniaudio,
  flac,
  libogg,
  libvorbis,
  libopus,
  libmpg123,
}:
stdenv.mkDerivation rec {
  pname = "RhythmGame";
  version = "unstable-2025-11-17";

  src = ./../..; # Points to repository root

  nativeBuildInputs = [
    cmake
    wrapQtAppsHook
    pkg-config
  ];

  buildInputs = [
    qtwayland
    qtshadertools
    qttools
    zstd
    magic-enum
    SDL2
    boost
    sqlitecpp
    fmt
    libxml2
    qtdeclarative
    qtmultimedia
    qtsvg
    qtinterfaceframework
    ned14-llfio
    lexy
    mimalloc
    spdlog
    libsndfile
    stb
    tbb
    miniaudio
    flac
    libogg
    libvorbis
    libopus
    libmpg123
  ];

  cmakeFlags = [
    "-DCMAKE_CXX_STANDARD=23"
    "-DUSE_SYSTEM_LIBRARIES=ON"
    "-Wno-dev"
  ];

  meta = with lib; {
    description = "A customizable BMS player for Windows and Linux";
    homepage = "https://github.com/Bobini1/RhythmGame";
    license = licenses.mit;
    platforms = platforms.linux;
    maintainers = [maintainers.Bobini1];
    mainProgram = "RhythmGame";
  };
}
