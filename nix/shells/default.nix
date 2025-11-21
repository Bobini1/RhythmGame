{
  mkShell,
  cmake,
  zstd,
  magic-enum,
  SDL2,
  boost,
  sqlitecpp,
  fmt,
  libxml2,
  qtwayland,
  qtshadertools,
  qtdeclarative,
  qtmultimedia,
  qtsvg,
  qtinterfaceframework,
  kirigami,
  kirigami-addons,
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
  appstream-qt,
  makeWrapper,
  bashInteractive,
  wrapQtAppsHook,
  gammaray,
}:
mkShell {
  buildInputs = [
    pkg-config
    catch2_3
    qttools
    zstd
    magic-enum
    SDL2
    boost
    sqlitecpp
    fmt
    libxml2
    qtwayland
    qtshadertools
    qtdeclarative
    qtmultimedia
    qtsvg
    qtinterfaceframework
    kirigami
    kirigami-addons
    appstream-qt
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
    bashInteractive
    cmake
    (gdb.override
      {
        safePaths = ["/"];
      })
    clang-tools
    gammaray
    lldb
  ];

  nativeBuildInputs = [
    wrapQtAppsHook
    makeWrapper
  ];

  shellHook = ''
    setQtEnvironment=$(mktemp --suffix .setQtEnvironment.sh)
    echo "shellHook: setQtEnvironment = $setQtEnvironment"
    makeWrapper "/bin/sh" "$setQtEnvironment" "''${qtWrapperArgs[@]}"
    sed "/^exec/d" -i "$setQtEnvironment"
    source "$setQtEnvironment"
    echo "RhythmGame development environment."
    echo "Before you start coding, create a CMakeUserPresets.json"
    echo "based on the template found in DEV_ENGINE.md."
    echo "Remove all references to vcpkg (unnecessary) and clang-tidy (broken)."
    echo
    echo "Afterwards, use the following commands:"
    echo "  cmake --preset dev         - Configure CMake"
    echo "  cmake --build --preset dev - Build"
  '';
}
