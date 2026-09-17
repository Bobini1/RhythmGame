# Engine Development

Here is some wisdom to help you build and test this project as a developer and
potential contributor.

If you plan to contribute, please read the [CONTRIBUTING](CONTRIBUTING.md)
guide.

## Developer mode

Build system targets that are only useful for developers of this project are
hidden if the `RhythmGame_DEVELOPER_MODE` option is disabled. Enabling this
option makes tests and other developer targets and options available. Not
enabling this option means that you are a consumer of this project and thus you
have no need for these targets and options.

Developer mode is always set to on in CI workflows.

### Presets

This project makes use of [presets][1] to simplify the process of configuring
the project. As a developer, you are recommended to always have the [latest
CMake version][2] installed to make use of the latest Quality-of-Life
additions.

You have a few options to pass `RhythmGame_DEVELOPER_MODE` to the configure
command, but this project prefers to use presets.

As a developer, you should create a `CMakeUserPresets.json` file at the root of
the project. Here is an example:

```json
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 14,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "dev-common",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "inherits": [
        "ci-<os>",
        "vcpkg-<os>-sharedqt",
        "dev-mode",
        "vcpkg",
        "clang-tidy"
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
      "jobs": 32
    },
    {
      "name": "dev-rel",
      "configurePreset": "dev-rel",
      "configuration": "Release",
      "jobs": 32
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
        "jobs": 32,
        "noTestsAction": "error"
      }
    }
  ]
}
```

You should replace `<os>` in your newly created presets file with the name of
the operating system you have, which may be `win64`, `linux` or `darwin`. You
can see what these correspond to in the
[`CMakePresets.json`](CMakePresets.json) file.

`CMakeUserPresets.json` is also the perfect place in which you can put all
sorts of things that you would otherwise want to pass to the configure command
in the terminal.

> **Note**
> Some editors are pretty greedy with how they open projects with presets.
> Some just randomly pick a preset and start configuring without your consent,
> which can be confusing. Make sure that your editor configures when you
> actually want it to, for example in CLion you have to make sure only the
> `dev-dev preset` has `Enable profile` ticked in
> `File > Settings... > Build, Execution, Deployment > CMake` and in Visual
> Studio you have to set the option `Never run configure step automatically`
> in `Tools > Options > CMake` **prior to opening the project**, after which
> you can manually configure using `Project > Configure Cache`.

### Dependency manager

The above preset will make use of the [vcpkg][vcpkg] dependency manager. After
installing it, make sure the `VCPKG_ROOT` environment variable is pointing at
the directory where the vcpkg executable is. You will also want
to inherit from the `vcpkg-<os>-sharedqt` preset, which will make vcpkg install
the dependencies as static libraries for all libs other than Qt, FFmpeg and mimalloc.

[vcpkg]: https://github.com/microsoft/vcpkg

### Configure, build and test

If you followed the above instructions, then you can configure, build and test
the project respectively with the following commands from the project root on
any operating system with any build system:

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

If you are using a compatible editor (e.g. VSCode) or IDE (e.g. CLion, VS), you
will also be able to select the above created user presets for automatic
integration.

Please note that both the build and test commands accept a `-j` flag to specify
the number of jobs to use, which should ideally be specified to the number of
threads your CPU has. You may also want to add that to your preset using the
`jobs` property, see the [presets documentation][1] for more details.

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

Available if `BUILD_MCSS_DOCS` is enabled. Builds to documentation using
Doxygen and m.css. The output will go to `<binary-dir>/docs` by default
(customizable using `DOXYGEN_OUTPUT_DIRECTORY`).

#### `format-check` and `format-fix`

These targets run the clang-format tool on the codebase to check errors and to
fix them respectively. Customization available using the `FORMAT_PATTERNS` and
`FORMAT_COMMAND` cache variables.

#### `run-exe`

Runs the executable target `RhythmGame_exe`.

#### `spell-check` and `spell-fix`

These targets run the codespell tool on the codebase to check errors and to fix
them respectively. Customization available using the `SPELL_COMMAND` cache
variable.

#### `update_translations` and `release_translations`

Create ts files and qm files for the default theme.
See the [DEV_LANG.md](DEV_LANG.md#strings-in-qml-files) document for more information.

### Backbeat

`RhythmGame_USE_BACKBEAT` defaults to `ON`. It selects the separate `backbeat`
vcpkg manifest feature and links the GPL-3.0 Backbeat SDK. To build without the
SDK or its integration code, configure with `-DRhythmGame_USE_BACKBEAT=OFF`.
Other manifest features, such as `test` and `docs`, are preserved. Without
vcpkg, an enabled build requires an installed `Backbeat::Backbeat` CMake target.

The game opens the user's normal Backbeat store, indexes new installed BMS and
BMSON bundles, and exposes installed packs, tables, and courses in song select.
It checks the store revision every five seconds while song select is active;
F2 also requests a refresh. Collection downloads and removal remain in the
Backbeat app. Local song folders continue to work independently.

Assets are read through the SDK as bytes or existing files. A temporary file is
created only when a consumer requires a filename, such as the video player or
an external README viewer. Packs retain exact bundle identities; tables and
courses can also find locally installed charts by MD5 or SHA-256. Course rules
outside the supported LR2 gauge and normal/mirror lane options are shown as
unavailable. A build with Backbeat disabled removes its cached chart entries
from the song database without changing profile scores or the Backbeat store.

[1]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[2]: https://cmake.org/download/
