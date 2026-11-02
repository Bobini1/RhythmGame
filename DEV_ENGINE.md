# Engine Development

This guide covers local builds, tests and documentation generation.

If you plan to contribute, please read the [CONTRIBUTING](CONTRIBUTING.md)
guide.

## Developer mode

Build system targets that are only useful for developers of this project are
hidden if the `RhythmGame_DEVELOPER_MODE` option is disabled. Enabling this
option makes tests and other developer targets and options available.
`BUILD_TESTING` controls whether the test target is included.

Developer mode is always set to on in CI workflows.

### Presets

This project makes use of [presets][1] to simplify the process of configuring
the project. Install a current [CMake version][2] that supports your compiler
and generator. The project requires at least CMake 3.21; the Windows preset
uses Visual Studio 18 2026 and needs a CMake version that supports it.

You have a few options to pass `RhythmGame_DEVELOPER_MODE` to the configure
command, but this project prefers to use presets.

As a developer, you should create a `CMakeUserPresets.json` file at the root of
the project. This Windows example uses the repository's shared-Qt vcpkg
triplet. The table below gives the corresponding Linux and macOS presets.

```json
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev-common",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "inherits": [
        "ci-win64",
        "vcpkg-win64-sharedqt",
        "dev-mode",
        "vcpkg"
      ],
      "cacheVariables": {
        "CMAKE_INSTALL_PREFIX": "${sourceDir}/install/${presetName}"
      },
      "environment": {
        "UseMultiToolTask": "true",
        "EnforceProcessCountAcrossBuilds": "true"
      }
    },
    {
      "name": "dev",
      "inherits": "dev-common",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "dev-rel",
      "inherits": "dev-common",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "dev-relwithdebinfo",
      "inherits": "dev-common",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "jobs": 4
    },
    {
      "name": "dev-rel",
      "configurePreset": "dev-rel",
      "configuration": "Release",
      "jobs": 4
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "jobs": 4,
        "noTestsAction": "error"
      }
    },
    {
      "name": "dev-rel",
      "configurePreset": "dev-rel",
      "configuration": "Release",
      "output": {
        "outputOnFailure": true
      },
      "execution": {
        "jobs": 4,
        "noTestsAction": "error"
      }
    }
  ]
}
```

Choose the first two inherited presets for your platform:

| Platform | Compiler and generator preset | vcpkg preset |
|----------|-------------------------------|--------------|
| Windows x64 | `ci-win64` | `vcpkg-win64-sharedqt` |
| Linux x64 | `ci-linux` | `vcpkg-linux-sharedqt` |
| macOS ARM64 | `ci-darwin` | `vcpkg-osx-sharedqt` |

These are defined in [`CMakePresets.json`](https://github.com/Bobini1/RhythmGame/blob/master/CMakePresets.json). Add `clang-tidy`
to `inherits` if you want analysis during compilation and have the tool installed.

`CMakeUserPresets.json` is ignored by Git. Use it for local paths, build options
and other settings that should not affect other contributors.

> **Note**
> Check which CMake profiles your editor enables before opening the project.
> Configuring several profiles at once can start several dependency builds.

### Dependency manager

The above preset will make use of the [vcpkg][vcpkg] dependency manager. After
installing it, make sure the `VCPKG_ROOT` environment variable is pointing at
the directory where the vcpkg executable is. You will also want
to choose the matching vcpkg preset from the table. These triplets use shared
Qt and mostly static dependencies. FFmpeg is also shared on Windows and Linux;
mimalloc is shared on Windows. See [vcpkgTriplets](https://github.com/Bobini1/RhythmGame/tree/master/vcpkgTriplets) for the exact
configuration.

[vcpkg]: https://github.com/microsoft/vcpkg

### Configure, build and test

With the example presets above, run these commands from the project root for
a Release build and tests:

```sh
cmake --preset=dev-rel
cmake --build --preset=dev-rel --target RhythmGame_exe RhythmGame_test
ctest --preset=dev-rel
```

Use `dev` instead of `dev-rel` for Debug. Reconfigure after changing CMake
files, dependencies or configure options; ordinary source edits need only a
build. Editors with CMake preset support can use the same presets.

Build and test commands accept `-j` to override the job count. Choose a value
that fits both your CPU and available memory. The example starts with four.

On Windows, the game executable is `build/dev-rel/bin/RhythmGame.exe` and the
test executable is `build/dev-rel/test/bin/RhythmGame_test.exe`. Close the game
before rebuilding it so Windows can replace the executable. Rebuild the test
target before using CTest after adding or renaming tests.

### Developer mode targets

These are targets you may invoke using the build command from above, with an
additional `-t <target>` flag:

#### `coverage`

Available if `ENABLE_COVERAGE` is enabled. This target processes the output of
the previously run tests when built with coverage configuration. The commands
this target runs can be found in the `COVERAGE_TRACE_COMMAND` and
`COVERAGE_HTML_COMMAND` cache variables. The trace command produces an info
file by default, which can be submitted to services with CI integration. The
HTML command uses the trace command's output to generate an HTML document to
`<binary-dir>/coverage_html` by default.

#### `docs`

Available if `BUILD_DOCS` is enabled in developer mode. It builds one site with
Doxygen for C++ and Markdown, and QDoc for QML. It also copies and packages the
installable tutorial themes. Open `<binary-dir>/docs/html/index.html`; change
`DOXYGEN_OUTPUT_DIRECTORY` to choose another output directory.

The target needs Doxygen and QDoc. With vcpkg, add the `docs` manifest feature
alongside `test` to install QDoc. For the example preset:

```sh
cmake --preset=dev-rel -DBUILD_DOCS=ON "-DVCPKG_MANIFEST_FEATURES=test;docs"
cmake --build --preset=dev-rel --target docs
```

QDoc's vcpkg feature builds LLVM, so the initial dependency build can take time.
Documentation edits need only the `docs` target, not a full game build or test run.

#### QML lint

`RhythmGame_qml_qmllint` checks the main QML module, and
`RhythmGame_lr2_qml_qmllint` checks the LR2 module. `all_qmllint` checks all QML
modules in the build. For example:

```sh
cmake --build --preset=dev-rel --target RhythmGame_qml_qmllint
```

Run these targets separately from test builds because they generate some of
the same QML metadata. Read the warnings as well as the exit status; unresolved
runtime types can still produce warnings in a successful lint run.

#### `format-check` and `format-fix`

These targets run clang-format to check formatting and apply fixes,
respectively. `FORMAT_PATTERNS` selects the files and `FORMAT_COMMAND` selects
the formatter. They do not check or format QML.

#### `run-exe`

Runs the executable target `RhythmGame_exe`.

#### `spell-check` and `spell-fix`

These targets run the codespell tool on the codebase to check errors and to fix
them respectively. Customization available using the `SPELL_COMMAND` cache
variable.

#### `update_translations` and `release_translations`

Update Default's `.ts` catalogs and compile them to `.qm` files, respectively.
See [DEV_LANG.md](DEV_LANG.md) for the translation workflow.

### Backbeat

`RhythmGame_USE_BACKBEAT` defaults to `ON`. It selects the separate `backbeat`
vcpkg manifest feature and links the GPL-3.0 Backbeat SDK. To build without the
SDK or its integration code, configure with `-DRhythmGame_USE_BACKBEAT=OFF`.
Other manifest features, such as `test` and `docs`, are preserved. Without
vcpkg, an enabled build requires an installed `Backbeat::Backbeat` CMake target.

Building the SDK from source requires [Rustup](https://rust-lang.github.io/rustup/)
and the pinned Rust 1.96.0 toolchain. Install it before configuring the game:

```sh
rustup toolchain install 1.96.0 --profile minimal --no-self-update
```

The overlay uses that toolchain explicitly, so your default Rust version can
stay unchanged. CI installs it before configuring each vcpkg build.

When enabled, the game opens the user's normal Backbeat store, indexes new installed BMS and
BMSON bundles, and exposes installed packs, tables, and courses in song select.
The C++ catalog checks the store revision every five seconds, independently of
the active screen or theme, and indexes changes asynchronously.

Themes don't need to adapt to Backbeat in any special way.

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
