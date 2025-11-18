{
  mkShell,
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
  clang-tools,
  libllvm,
  libcxx,
  lldb,
  gdb,
  catch2_3,
  stdenv,
}:
mkShell {
  buildInputs = [
    pkg-config
    catch2_3
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

  nativeBuildInputs = [
    cmake
    gdb
    clang-tools
  ];

  shellHook = ''
    echo "RhythmGame development environment."
    echo "Before you start coding, create a CMakeUserPresets.json"
    echo "based on the template found in DEV_ENGINE.md."
    echo "Remove all references to vcpkg (unnecessary) and clang-tidy (broken)."
    echo
    echo "Afterwards, use the following commands to build:"
    echo "  cmake --preset dev         - Configure CMake"
    echo "  cmake --build --preset dev - Build"
  '';
}
