# Emscripten Gate 1A Vcpkg Qt Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a reproducible, vcpkg-managed static Qt 6.11.1 WebAssembly
build with Emscripten 4.0.7, native Qt host tools, the required thread/native
exception/JSPI configuration, and a minimal Qt Quick build whose toolchain and
flags are mechanically audited.

**Architecture:** Gate 1A is an isolated build probe under
`tools/wasm-probe`; it does not compile the RhythmGame application or mutate the
native root dependency graph. Vcpkg owns both native host Qt tools and static
Wasm target Qt packages. A repository-owned target triplet forwards uniform
Emscripten flags, while a complete baseline-derived `qtbase` overlay enables
Qt's Wasm features and restores its application-side Emscripten version check.

**Tech Stack:** CMake/Ninja, vcpkg manifest mode, Qt 6.11.1, Emscripten 4.0.7,
PowerShell 7, Python 3.12 `unittest`, Qt Quick/QML, Qt ShaderTools.

## Global Constraints

- Keep one RhythmGame source codebase; Gate 1A is an isolated risk probe, not a
  second application implementation.
- Chromium desktop is the first browser target and Windows 11 is the first
  qualification operating system.
- Qt is exactly `6.11.1`.
- Emscripten is exactly `4.0.7`.
- The emsdk Git commit is exactly
  `c69d433d8509c5c64564c2f0d054bf102a5cf67e`.
- The vcpkg checkout and builtin baseline are exactly
  `a0400024711b283056538ac19ced80b91a83c24c`.
- The first qualification lane fails closed unless CMake is exactly `4.2.3`
  and Ninja is exactly `1.13.2`; both observed versions enter the evidence.
- Target Qt and every target dependency are static; Qt WebAssembly dynamic
  linking is forbidden.
- Native Qt host tools use a native host triplet and the same Qt version as the
  target packages. The qualified set includes `moc`, `qmlcachegen`,
  `qmltyperegistrar`, `qsb`, `lrelease`, and `lupdate`.
- The target manifest includes Qt Base, Declarative, ShaderTools,
  ImageFormats, SVG, WebSockets, and the Wasm Multimedia QML backend.
- Target C compilation uses `-pthread`; target C++ compilation and executable
  linking use `-pthread -fwasm-exceptions`.
- Qt target configuration enables `FEATURE_thread`,
  `FEATURE_wasm_exceptions`, and `FEATURE_wasm_jspi`; SIMD remains disabled in
  Gate 1A.
- The probe executable links `-sJSPI`, `-sAUDIO_WORKLET=1`,
  `-sWASM_WORKERS=1`, `-sPTHREAD_POOL_SIZE=4`,
  `-sPTHREAD_POOL_SIZE_STRICT=2`, and
  `-sALLOW_BLOCKING_ON_MAIN_THREAD=0`.
- Do not substitute legacy JavaScript exceptions, `-fexceptions`, Asyncify, a
  non-threaded artifact, ScriptProcessor, shared Qt, or an external Qt SDK.
- Preserve the existing `qtdeclarative` overlay patch byte-for-byte; its
  SHA-256 remains
  `2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50`.
- Do not use `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH`, an external `Qt6_DIR`,
  or target Qt tools in place of native host tools.
- The unresolved Qt commercial/GPLv3 decision prevents broad application
  porting and public deployment, but it does not prevent this reproducibility
  probe.
- Gate 1A passing does not mean Gate 1 passes. Gate 1B must still prove the
  combined Qt Quick, JSPI, pthread, AudioWorklet, network, media, OPFS, File
  System Access, memory-growth, teardown, and Chromium header/runtime behavior.

---

## File Structure

Tracked files created or modified by this plan:

```text
.gitattributes
.gitignore
cmake/toolchains/vcpkg-emscripten.cmake
vcpkgTriplets/wasm32-emscripten-rg.cmake
vcpkgTriplets/x64-windows-rg-host-release.cmake
vcpkgOverlayPortsWasm/qtbase/**                  # exact baseline port copy
vcpkgOverlayPortsWasm/qtbase/restore-wasm-version-check.patch
tools/wasm-probe/CMakeLists.txt
tools/wasm-probe/CMakePresets.json
tools/wasm-probe/vcpkg.json
tools/wasm-probe/toolchain-lock.json
tools/wasm-probe/cmake/verify_exact_toolchain.cmake
tools/wasm-probe/scripts/Bootstrap-Toolchains.ps1
tools/wasm-probe/scripts/Invoke-WithToolchains.ps1
tools/wasm-probe/src/ExceptionBoundary.h
tools/wasm-probe/src/ExceptionBoundary.cpp
tools/wasm-probe/src/ProbeState.h
tools/wasm-probe/src/ProbeState.cpp
tools/wasm-probe/src/main.cpp
tools/wasm-probe/qml/Main.qml
tools/wasm-probe/qml/pulse.frag
tools/wasm-probe/tests/_toolchain_process_double.py
tools/wasm-probe/tests/test_toolchain_contract.py
tools/wasm-probe/tests/test_toolchain_scripts.py
tools/wasm-probe/tests/test_probe_source_contract.py
tools/wasm-probe/tests/verify_build.py
docs/superpowers/evidence/emscripten-gate-1a.json
```

Generated, ignored directories:

```text
.toolchains/emsdk-4.0.7
.toolchains/vcpkg-a0400024
.toolchains/cmake-4.2.3-windows-x86_64
.toolchains/ninja-1.13.2-win
.toolchains/downloads
.wasm-vcpkg/buildtrees
.wasm-vcpkg/downloads
.wasm-vcpkg/packages
.wasm-vcpkg/installed
.wasm-vcpkg/bincache
tools/wasm-probe/build/wasm-release
```

The responsibilities are:

- `vcpkg-emscripten.cmake` includes the exact Emscripten platform toolchain and
  forwards vcpkg's C, C++, configuration, and linker flags.
- `wasm32-emscripten-rg.cmake` defines one static target ABI and fails unless
  Emscripten 4.0.7 is active.
- `x64-windows-rg-host-release.cmake` builds release-only native Qt tools on the
  first qualification lane.
- `vcpkgOverlayPortsWasm/qtbase` is a reviewable copy of the exact baseline
  port; it owns the three Qt Wasm features and version-check repair.
- `tools/wasm-probe` is a separate CMake project and manifest, so native
  RhythmGame configuration remains unchanged.
- Python contract tests reject pin drift, native dependency leakage, shared Qt,
  suppressed version checks, missing flags, and incomplete generated evidence.

---

### Task 1: Add the pinned vcpkg/Emscripten contract

**Files:**

- Modify: `.gitignore`
- Modify: `.gitattributes`
- Create: `cmake/toolchains/vcpkg-emscripten.cmake`
- Create: `vcpkgTriplets/wasm32-emscripten-rg.cmake`
- Create: `vcpkgTriplets/x64-windows-rg-host-release.cmake`
- Create: `vcpkgOverlayPortsWasm/qtbase/**`
- Create:
  `vcpkgOverlayPortsWasm/qtbase/restore-wasm-version-check.patch`
- Create: `tools/wasm-probe/vcpkg.json`
- Create: `tools/wasm-probe/toolchain-lock.json`
- Create: `tools/wasm-probe/tests/test_toolchain_contract.py`

**Interfaces:**

- Consumes: vcpkg port tree at commit
  `a0400024711b283056538ac19ced80b91a83c24c`; existing
  `vcpkgOverlayPorts/qtdeclarative`.
- Produces: target triplet `wasm32-emscripten-rg`, host triplet
  `x64-windows-rg-host-release`, and an isolated Qt-only vcpkg manifest.

- [ ] **Step 1: Write the failing toolchain contract test**

Create `tools/wasm-probe/tests/test_toolchain_contract.py`:

```python
import hashlib
import json
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PROBE = REPO / "tools" / "wasm-probe"
BASELINE = "a0400024711b283056538ac19ced80b91a83c24c"
EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
FULL_QT_SHA256 = (
    "252acef8c5ae68074d91cadba2ee4a83465051bbb970dd26e8f0daa0f3904e03"
)
QTBASE_SHA256 = (
    "d9594a31228aa23ad6b531719a29b45f0f3989fe6c136d45767ea179f233c1ac"
)
QTBASE_SHA512 = (
    "b5608b6cefd483ecdc5e4fa3536acfc31116c8dfb698257f945180a8e412ee44"
    "4dc670d754d4f6145649170f7d55637a70820188337a6c6b79193fbfbcd6a3fc"
)
QTBASE_VCPKG_TREE = "29a7f9f115d568b271a3b99fabeac886ec248f9f"
QTBASE_WASM_PATCH_SHA256 = (
    "d9ed64da369eeb3aedc6830a2649925bcf8e8742d1c5fc86c21adf5bb168d5a0"
)
QTDECLARATIVE_PATCH_SHA256 = (
    "2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50"
)
CMAKE_WINDOWS_X64_SHA256 = (
    "eb4ebf5155dbb05436d675706b2a08189430df58904257ae5e91bcba4c86933c"
)
NINJA_WINDOWS_SHA256 = (
    "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65"
)


class ToolchainContractTest(unittest.TestCase):
    def test_lock_file_has_exact_production_pins(self) -> None:
        lock = json.loads((PROBE / "toolchain-lock.json").read_text("utf-8"))
        self.assertEqual(lock["qt"]["version"], "6.11.1")
        self.assertEqual(lock["emscripten"]["version"], "4.0.7")
        self.assertEqual(lock["emscripten"]["emsdkCommit"], EMSDK_COMMIT)
        self.assertEqual(lock["vcpkg"]["baseline"], BASELINE)
        self.assertEqual(lock["qt"]["fullQtSourceSha256"], FULL_QT_SHA256)
        self.assertEqual(lock["qt"]["qtbaseSourceSha256"], QTBASE_SHA256)
        self.assertEqual(lock["qt"]["qtbaseVcpkgSha512"], QTBASE_SHA512)
        self.assertEqual(lock["qt"]["qtbaseVcpkgTree"], QTBASE_VCPKG_TREE)
        self.assertEqual(
            lock["qt"]["qtbaseWasmPatchSha256"],
            QTBASE_WASM_PATCH_SHA256,
        )
        self.assertEqual(
            lock["buildTools"]["cmake"],
            {
                "version": "4.2.3",
                "url": (
                    "https://cmake.org/files/v4.2/"
                    "cmake-4.2.3-windows-x86_64.zip"
                ),
                "sha256": CMAKE_WINDOWS_X64_SHA256,
                "directory": "cmake-4.2.3-windows-x86_64",
            },
        )
        self.assertEqual(
            lock["buildTools"]["ninja"],
            {
                "version": "1.13.2",
                "url": (
                    "https://github.com/ninja-build/ninja/releases/"
                    "download/v1.13.2/ninja-win.zip"
                ),
                "sha256": NINJA_WINDOWS_SHA256,
                "directory": "ninja-1.13.2-win",
            },
        )
        self.assertEqual(
            lock["qt"]["qtdeclarativePatchSha256"],
            QTDECLARATIVE_PATCH_SHA256,
        )

    def test_manifest_is_isolated_and_has_no_native_only_features(self) -> None:
        manifest = json.loads((PROBE / "vcpkg.json").read_text("utf-8"))
        self.assertEqual(manifest["builtin-baseline"], BASELINE)
        encoded = json.dumps(manifest, sort_keys=True)
        for forbidden in (
            "ffmpeg",
            "qtkeychain",
            "qtinterfaceframework",
            "openimageio",
            "sdl2-image",
            "vulkan",
            "openssl",
            "dnslookup",
            "llfio",
            "mimalloc",
        ):
            self.assertNotIn(forbidden, encoded.lower())
        self.assertIn("qtbase", encoded)
        self.assertIn("qtdeclarative", encoded)
        self.assertIn("qtimageformats", encoded)
        self.assertIn("qtmultimedia", encoded)
        self.assertIn("qtwebsockets", encoded)
        qttools = next(
            dependency
            for dependency in manifest["dependencies"]
            if isinstance(dependency, dict)
            and dependency.get("name") == "qttools"
        )
        self.assertTrue(qttools["host"])
        self.assertFalse(qttools["default-features"])
        self.assertEqual(qttools["features"], ["linguist"])

    def test_target_triplet_is_static_and_uniform(self) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        self.assertIn("set(VCPKG_TARGET_ARCHITECTURE wasm32)", triplet)
        self.assertIn("set(VCPKG_LIBRARY_LINKAGE static)", triplet)
        self.assertIn("set(VCPKG_BUILD_TYPE release)", triplet)
        self.assertIn("set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)", triplet)
        self.assertIn("EXPECTED_EMSCRIPTEN_VERSION \"4.0.7\"", triplet)
        self.assertIn('set(VCPKG_C_FLAGS "-pthread")', triplet)
        self.assertIn(
            'set(VCPKG_CXX_FLAGS "-pthread -fwasm-exceptions")',
            triplet,
        )
        self.assertIn(
            'set(VCPKG_LINKER_FLAGS "-pthread -fwasm-exceptions")',
            triplet,
        )
        self.assertNotIn(
            "set(VCPKG_LIBRARY_LINKAGE dynamic)",
            triplet,
        )
        self.assertNotIn("if(PORT MATCHES", triplet)

    def test_wasm_wrapper_forwards_all_vcpkg_flags(self) -> None:
        wrapper = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        for variable in (
            "VCPKG_C_FLAGS",
            "VCPKG_CXX_FLAGS",
            "VCPKG_C_FLAGS_DEBUG",
            "VCPKG_CXX_FLAGS_DEBUG",
            "VCPKG_C_FLAGS_RELEASE",
            "VCPKG_CXX_FLAGS_RELEASE",
            "VCPKG_LINKER_FLAGS",
            "VCPKG_LINKER_FLAGS_DEBUG",
            "VCPKG_LINKER_FLAGS_RELEASE",
        ):
            self.assertIn(variable, wrapper)
        self.assertIn("Emscripten.cmake", wrapper)

    def test_qtbase_overlay_enables_features_and_keeps_version_check(self) -> None:
        overlay = REPO / "vcpkgOverlayPortsWasm" / "qtbase"
        portfile = (overlay / "portfile.cmake").read_text("utf-8")
        self.assertIn("restore-wasm-version-check.patch", portfile)
        self.assertIn("-DFEATURE_thread:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_exceptions:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_jspi:BOOL=ON", portfile)
        self.assertIn("-DFEATURE_wasm_simd128:BOOL=OFF", portfile)
        self.assertNotIn(
            '"_qt_test_emscripten_version()" ""',
            portfile,
        )
        patch = (overlay / "restore-wasm-version-check.patch").read_text(
            "utf-8"
        )
        attributes = (REPO / ".gitattributes").read_text("utf-8")
        self.assertIn(
            "vcpkgOverlayPortsWasm/qtbase/** -text -whitespace",
            attributes,
        )
        self.assertIn("include(QtPublicWasmToolchainHelpers)", patch)
        self.assertEqual(
            hashlib.sha256(
                (overlay / "restore-wasm-version-check.patch").read_bytes()
            ).hexdigest(),
            QTBASE_WASM_PATCH_SHA256,
        )

    def test_existing_qtdeclarative_patch_bytes_are_unchanged(self) -> None:
        patch = (
            REPO
            / "vcpkgOverlayPorts"
            / "qtdeclarative"
            / "24205cd-qquickwindow-child-window-stacking.patch"
        )
        digest = hashlib.sha256(patch.read_bytes()).hexdigest().upper()
        self.assertEqual(digest, QTDECLARATIVE_PATCH_SHA256)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the contract test and verify it fails**

Run:

```powershell
python -m unittest tools/wasm-probe/tests/test_toolchain_contract.py -v
```

Expected: `ERROR`/`FAIL` because the lock, manifest, triplets, wrapper, and
Wasm qtbase overlay do not exist.

- [ ] **Step 3: Ignore generated state and preserve patch bytes**

Append these exact entries to `.gitignore`:

```gitignore
.toolchains/
.wasm-vcpkg/
.vcpkg-buildtrees/
.vcpkg-cache/
.vcpkg-packages/
```

Do not remove or rewrite the existing ignored paths.

Append this exact line to `.gitattributes`. The baseline port contains
deliberate mixed line endings and upstream whitespace, so the whole copied
port must bypass Windows `core.autocrlf` and Git whitespace diagnostics; this
also protects the reviewed patch payload. Checks remain enabled everywhere
outside this byte-exact upstream snapshot:

```gitattributes
vcpkgOverlayPortsWasm/qtbase/** -text -whitespace
```

- [ ] **Step 4: Add the machine-readable lock**

Create `tools/wasm-probe/toolchain-lock.json`:

```json
{
  "qt": {
    "version": "6.11.1",
    "fullQtSourceSha256": "252acef8c5ae68074d91cadba2ee4a83465051bbb970dd26e8f0daa0f3904e03",
    "qtbaseSourceSha256": "d9594a31228aa23ad6b531719a29b45f0f3989fe6c136d45767ea179f233c1ac",
    "qtbaseVcpkgSha512": "b5608b6cefd483ecdc5e4fa3536acfc31116c8dfb698257f945180a8e412ee444dc670d754d4f6145649170f7d55637a70820188337a6c6b79193fbfbcd6a3fc",
    "qtbaseVcpkgTree": "29a7f9f115d568b271a3b99fabeac886ec248f9f",
    "qtbaseWasmPatchSha256": "d9ed64da369eeb3aedc6830a2649925bcf8e8742d1c5fc86c21adf5bb168d5a0",
    "qtdeclarativePatchSha256": "2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50"
  },
  "emscripten": {
    "version": "4.0.7",
    "emsdkCommit": "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
  },
  "vcpkg": {
    "baseline": "a0400024711b283056538ac19ced80b91a83c24c",
    "emscriptenWrapperSourceCommit": "5b698638c97b4610044a254347d571f823e56557"
  },
  "buildTools": {
    "cmake": {
      "version": "4.2.3",
      "url": "https://cmake.org/files/v4.2/cmake-4.2.3-windows-x86_64.zip",
      "sha256": "eb4ebf5155dbb05436d675706b2a08189430df58904257ae5e91bcba4c86933c",
      "directory": "cmake-4.2.3-windows-x86_64"
    },
    "ninja": {
      "version": "1.13.2",
      "url": "https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip",
      "sha256": "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65",
      "directory": "ninja-1.13.2-win"
    }
  }
}
```

- [ ] **Step 5: Add the vcpkg-aware Emscripten chainload wrapper**

Create `cmake/toolchains/vcpkg-emscripten.cmake` with the exact forwarding
logic below. It is the repository-pinned form of vcpkg commit
`5b698638c97b4610044a254347d571f823e56557`:

```cmake
if(NOT _RHYTHMGAME_VCPKG_EMSCRIPTEN_TOOLCHAIN)
    set(_RHYTHMGAME_VCPKG_EMSCRIPTEN_TOOLCHAIN 1)

    list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        VCPKG_CRT_LINKAGE
        VCPKG_TARGET_ARCHITECTURE
        VCPKG_C_FLAGS
        VCPKG_CXX_FLAGS
        VCPKG_C_FLAGS_DEBUG
        VCPKG_CXX_FLAGS_DEBUG
        VCPKG_C_FLAGS_RELEASE
        VCPKG_CXX_FLAGS_RELEASE
        VCPKG_LINKER_FLAGS
        VCPKG_LINKER_FLAGS_RELEASE
        VCPKG_LINKER_FLAGS_DEBUG
    )

    if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
        message(FATAL_ERROR "EMSCRIPTEN_ROOT must name the pinned 4.0.7 tree")
    endif()
    set(EMSCRIPTEN_ROOT "$ENV{EMSCRIPTEN_ROOT}")
    set(_emscripten_toolchain
        "${EMSCRIPTEN_ROOT}/cmake/Modules/Platform/Emscripten.cmake")
    if(NOT EXISTS "${_emscripten_toolchain}")
        message(FATAL_ERROR
            "Pinned Emscripten toolchain not found: ${_emscripten_toolchain}")
    endif()

    include("${_emscripten_toolchain}")

    string(APPEND CMAKE_C_FLAGS_INIT " ${VCPKG_C_FLAGS} ")
    string(APPEND CMAKE_CXX_FLAGS_INIT " ${VCPKG_CXX_FLAGS} ")
    string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " ${VCPKG_C_FLAGS_DEBUG} ")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " ${VCPKG_CXX_FLAGS_DEBUG} ")
    string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " ${VCPKG_C_FLAGS_RELEASE} ")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT
        " ${VCPKG_CXX_FLAGS_RELEASE} ")

    string(APPEND CMAKE_MODULE_LINKER_FLAGS_INIT
        " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT
        " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT
        " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT
        " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT
        " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT
        " ${VCPKG_LINKER_FLAGS_RELEASE} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT
        " ${VCPKG_LINKER_FLAGS_RELEASE} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT
        " ${VCPKG_LINKER_FLAGS_RELEASE} ")
endif()
```

- [ ] **Step 6: Add exact target and host triplets**

Create `vcpkgTriplets/wasm32-emscripten-rg.cmake`:

```cmake
set(VCPKG_ENV_PASSTHROUGH EMSCRIPTEN_ROOT EMSCRIPTEN_VERSION)

set(EXPECTED_EMSCRIPTEN_VERSION "4.0.7")
if(NOT DEFINED ENV{EMSCRIPTEN_ROOT})
    message(FATAL_ERROR "EMSCRIPTEN_ROOT is required")
endif()
if(NOT "$ENV{EMSCRIPTEN_VERSION}" STREQUAL
       "${EXPECTED_EMSCRIPTEN_VERSION}")
    message(FATAL_ERROR
        "Expected Emscripten ${EXPECTED_EMSCRIPTEN_VERSION}, got "
        "'$ENV{EMSCRIPTEN_VERSION}'")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
set(VCPKG_CMAKE_SYSTEM_NAME Emscripten)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/../cmake/toolchains/vcpkg-emscripten.cmake")

set(VCPKG_C_FLAGS "-pthread")
set(VCPKG_CXX_FLAGS "-pthread -fwasm-exceptions")
set(VCPKG_LINKER_FLAGS "-pthread -fwasm-exceptions")

list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS
    "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON")
```

Create `vcpkgTriplets/x64-windows-rg-host-release.cmake`:

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_BUILD_TYPE release)
```

- [ ] **Step 7: Copy the exact baseline qtbase port**

Resolve the baseline from the available vcpkg Git checkout, verify it, and copy
only `ports/qtbase`:

```powershell
$baseline = 'a0400024711b283056538ac19ced80b91a83c24c'
$seed = (Resolve-Path $env:VCPKG_ROOT).Path
if ((git -C $seed rev-parse "$baseline^{commit}") -ne $baseline) {
    throw "The vcpkg seed does not contain $baseline"
}
$staging = Join-Path $env:TEMP 'rhythmgame-qtbase-overlay'
if (Test-Path -LiteralPath $staging) {
    throw "Refusing to replace existing staging directory: $staging"
}
New-Item -ItemType Directory -Path $staging | Out-Null
git -C $seed archive $baseline ports/qtbase |
    tar -xf - -C $staging
New-Item -ItemType Directory -Path vcpkgOverlayPortsWasm -Force |
    Out-Null
Copy-Item -LiteralPath (Join-Path $staging 'ports\qtbase') `
    -Destination 'vcpkgOverlayPortsWasm\qtbase' -Recurse
```

The implementer must verify the copied file list with:

```powershell
$prefix = 'ports/qtbase/'
$expectedBlobs = @{}
git -C $env:VCPKG_ROOT ls-tree -r `
    a0400024711b283056538ac19ced80b91a83c24c ports/qtbase |
    ForEach-Object {
        if ($_ -notmatch '^100644 blob ([0-9a-f]{40})\s+(.+)$') {
            throw "Unexpected qtbase tree entry: $_"
        }
        $expectedBlobs[$Matches[2].Substring($prefix.Length)] = $Matches[1]
    }
$expectedFiles = @($expectedBlobs.Keys | Sort-Object)
$overlayRoot = (Resolve-Path vcpkgOverlayPortsWasm/qtbase).Path
$actualFiles = @(
    Get-ChildItem $overlayRoot -Recurse -File |
    ForEach-Object {
        $_.FullName.Substring($overlayRoot.Length + 1).Replace('\', '/')
    } |
        Sort-Object
)
$difference = Compare-Object $expectedFiles $actualFiles
if ($difference) {
    $difference | Format-Table | Out-String | Write-Error
    throw 'The qtbase overlay is not an exact baseline port copy'
}
foreach ($relativePath in $expectedFiles) {
    $actualBlob = git hash-object --no-filters (
        Join-Path $overlayRoot $relativePath.Replace('/', '\')
    )
    if ($LASTEXITCODE -ne 0 -or
        $actualBlob -ne $expectedBlobs[$relativePath]) {
        throw "Baseline byte mismatch: $relativePath"
    }
}
```

The two relative lists and every raw blob must match before adding the new
Wasm patch. `--no-filters` is required because Windows Git otherwise applies
`core.autocrlf` inconsistently when `hash-object` receives an absolute path.

- [ ] **Step 8: Patch the overlay for Qt Wasm features and version checking**

Create
`vcpkgOverlayPortsWasm/qtbase/restore-wasm-version-check.patch`:

```diff
diff --git a/src/corelib/Qt6WasmMacros.cmake b/src/corelib/Qt6WasmMacros.cmake
--- a/src/corelib/Qt6WasmMacros.cmake
+++ b/src/corelib/Qt6WasmMacros.cmake
@@ -4 +4,3 @@
+include(QtPublicWasmToolchainHelpers)
+
 # Copy in Qt HTML/JS launch files for apps.
```

Modify the overlay `portfile.cmake` in three exact ways:

1. Add `restore-wasm-version-check.patch` to `${PORT}_PATCHES`.
2. Immediately before `qt_install_submodule(...)`, append:

```cmake
if(VCPKG_TARGET_IS_EMSCRIPTEN)
    list(APPEND FEATURE_OPTIONS
        -DFEATURE_thread:BOOL=ON
        -DFEATURE_wasm_exceptions:BOOL=ON
        -DFEATURE_wasm_jspi:BOOL=ON
        -DFEATURE_wasm_simd128:BOOL=OFF
    )
endif()
```

3. Remove this exact baseline workaround:

```cmake
if(VCPKG_TARGET_IS_EMSCRIPTEN)
  vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6Core/Qt6WasmMacros.cmake" "_qt_test_emscripten_version()" "") # this is missing a include(QtPublicWasmToolchainHelpers)
endif()
```

Set `"port-version": 1` in the overlay `vcpkg.json` without changing its
version or dependency graph.

Download the exact archive named by the baseline port, verify its SHA-512, and
check the patch against an independently extracted unmodified source file:

```powershell
$archive = Join-Path $env:TEMP 'qtbase-everywhere-src-6.11.1.tar.xz'
if (-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest `
        'https://download.qt.io/archive/qt/6.11/6.11.1/submodules/qtbase-everywhere-src-6.11.1.tar.xz' `
        -OutFile $archive
}
$expectedSha512 = 'b5608b6cefd483ecdc5e4fa3536acfc31116c8dfb698257f945180a8e412ee444dc670d754d4f6145649170f7d55637a70820188337a6c6b79193fbfbcd6a3fc'
$actualSha512 = (Get-FileHash $archive -Algorithm SHA512).Hash.ToLower()
if ($actualSha512 -ne $expectedSha512) {
    throw "Qt archive SHA-512 mismatch: $actualSha512"
}
$expectedSha256 = 'd9594a31228aa23ad6b531719a29b45f0f3989fe6c136d45767ea179f233c1ac'
$actualSha256 = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLower()
if ($actualSha256 -ne $expectedSha256) {
    throw "Qt archive SHA-256 mismatch: $actualSha256"
}

$verifyRoot = Join-Path $env:TEMP (
    'rhythmgame-qtbase-patch-' + [guid]::NewGuid().ToString('N')
)
New-Item -ItemType Directory -Path $verifyRoot | Out-Null
$member = 'qtbase-everywhere-src-6.11.1/src/corelib/Qt6WasmMacros.cmake'
tar -xf $archive -C $verifyRoot $member
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to extract the pristine Qt6WasmMacros.cmake'
}
$source = Join-Path $verifyRoot 'qtbase-everywhere-src-6.11.1'
$patch = (
    Resolve-Path `
        vcpkgOverlayPortsWasm/qtbase/restore-wasm-version-check.patch
).Path
git -C $source apply --check $patch
if ($LASTEXITCODE -ne 0) {
    throw 'qtbase Wasm version-check patch does not apply cleanly'
}
```

- [ ] **Step 9: Add the isolated Gate 1 manifest**

Create `tools/wasm-probe/vcpkg.json`:

```json
{
  "name": "rhythmgame-wasm-probe",
  "version-semver": "0.1.0",
  "builtin-baseline": "a0400024711b283056538ac19ced80b91a83c24c",
  "dependencies": [
    {
      "name": "qtbase",
      "default-features": false,
      "features": [
        "concurrent",
        "future",
        "gles2",
        "gui",
        "harfbuzz",
        "jpeg",
        "network",
        "png",
        "testlib",
        "thread"
      ]
    },
    "qtdeclarative",
    {
      "name": "qtimageformats",
      "default-features": false
    },
    {
      "name": "qtmultimedia",
      "default-features": false,
      "features": [
        "qml"
      ]
    },
    "qtshadertools",
    "qtsvg",
    {
      "name": "qttools",
      "host": true,
      "default-features": false,
      "features": [
        "linguist"
      ]
    },
    "qtwebsockets"
  ]
}
```

- [ ] **Step 10: Run the contract test and commit**

Run:

```powershell
python -m unittest tools/wasm-probe/tests/test_toolchain_contract.py -v
git add .gitattributes .gitignore cmake/toolchains vcpkgTriplets `
    vcpkgOverlayPortsWasm tools/wasm-probe/vcpkg.json `
    tools/wasm-probe/toolchain-lock.json tools/wasm-probe/tests
git diff --cached --check
```

Expected: six tests pass; `git diff --cached --check` exits zero. The
`-whitespace` attribute suppresses diagnostics only for the byte-exact
baseline QtBase snapshot.

Commit:

```powershell
git commit -m "build: add pinned Wasm Qt toolchain"
```

---

### Task 2: Bootstrap exact isolated toolchains

**Files:**

- Modify: `tools/wasm-probe/toolchain-lock.json`
- Create: `tools/wasm-probe/scripts/Bootstrap-Toolchains.ps1`
- Create: `tools/wasm-probe/scripts/Invoke-WithToolchains.ps1`
- Modify: `tools/wasm-probe/tests/test_toolchain_contract.py`
- Create: `tools/wasm-probe/tests/_toolchain_process_double.py`
- Create: `tools/wasm-probe/tests/test_toolchain_scripts.py`

**Interfaces:**

- Consumes: the pins in `toolchain-lock.json`.
- Produces: `.toolchains/emsdk-4.0.7`,
  `.toolchains/vcpkg-a0400024`,
  `.toolchains/cmake-4.2.3-windows-x86_64`,
  `.toolchains/ninja-1.13.2-win`, and a command wrapper that exports
  `EMSCRIPTEN_ROOT`, `EMSCRIPTEN_VERSION`, and `VCPKG_ROOT` without changing
  the user's globally active emsdk.

- [ ] **Step 1: Extend the failing contract test for bootstrap scripts**

Add this test method:

```python
    def test_bootstrap_is_local_and_fails_on_pin_drift(self) -> None:
        bootstrap = (
            PROBE / "scripts" / "Bootstrap-Toolchains.ps1"
        ).read_text("utf-8")
        invoke = (
            PROBE / "scripts" / "Invoke-WithToolchains.ps1"
        ).read_text("utf-8")
        self.assertIn(EMSDK_COMMIT, bootstrap)
        self.assertIn(BASELINE, bootstrap)
        self.assertIn("activate 4.0.7", bootstrap)
        self.assertNotIn("--permanent", bootstrap)
        self.assertNotIn("--system", bootstrap)
        self.assertIn(EMSDK_COMMIT, invoke)
        self.assertIn(BASELINE, invoke)
        self.assertIn("rev-parse HEAD", invoke)
        self.assertIn("Expected CMake 4.2.3", invoke)
        self.assertIn("1.13.2", invoke)
        self.assertIn("EMSCRIPTEN_ROOT", invoke)
        self.assertIn("EMSCRIPTEN_VERSION", invoke)
        self.assertIn("VCPKG_ROOT", invoke)
        self.assertIn(
            '$env:Path = "$vcpkg$([IO.Path]::PathSeparator)$env:Path"',
            invoke,
        )
        self.assertIn("4.0.7", invoke)
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_toolchain_contract.py -v
```

Expected: only
`test_bootstrap_is_local_and_fails_on_pin_drift` errors because the scripts
do not exist.

- [ ] **Step 3: Implement isolated bootstrap**

Create `tools/wasm-probe/scripts/Bootstrap-Toolchains.ps1`:

```powershell
[CmdletBinding()]
param(
    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
$emsdkCommit = 'c69d433d8509c5c64564c2f0d054bf102a5cf67e'
$vcpkgCommit = 'a0400024711b283056538ac19ced80b91a83c24c'
$emsdk = Join-Path $ToolchainRoot 'emsdk-4.0.7'
$vcpkg = Join-Path $ToolchainRoot 'vcpkg-a0400024'

New-Item -ItemType Directory -Path $ToolchainRoot -Force | Out-Null

function Assert-Commit {
    param([string]$Repository, [string]$Expected)
    $actual = git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "Expected $Repository at $Expected, got $actual"
    }
}

if (-not (Test-Path -LiteralPath $emsdk)) {
    git clone --filter=blob:none `
        https://github.com/emscripten-core/emsdk.git $emsdk
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to clone emsdk'
    }
    git -C $emsdk checkout --detach $emsdkCommit
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to check out pinned emsdk'
    }
}
Assert-Commit -Repository $emsdk -Expected $emsdkCommit

if (-not (Test-Path -LiteralPath $vcpkg)) {
    git clone --filter=blob:none `
        https://github.com/microsoft/vcpkg.git $vcpkg
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to clone vcpkg'
    }
    git -C $vcpkg checkout --detach $vcpkgCommit
    if ($LASTEXITCODE -ne 0) {
        throw 'Failed to check out pinned vcpkg'
    }
}
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit

& (Join-Path $emsdk 'emsdk.bat') install 4.0.7
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to install Emscripten 4.0.7'
}
& (Join-Path $emsdk 'emsdk.bat') activate 4.0.7
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to activate Emscripten 4.0.7 in the isolated emsdk tree'
}

& (Join-Path $vcpkg 'bootstrap-vcpkg.bat') -disableMetrics
if ($LASTEXITCODE -ne 0) {
    throw 'Failed to bootstrap pinned vcpkg'
}

Write-Output "EMSDK=$emsdk"
Write-Output "VCPKG_ROOT=$vcpkg"
```

Create `tools/wasm-probe/scripts/Invoke-WithToolchains.ps1`:

```powershell
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Executable,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments,

    [string]$ToolchainRoot = (
        Join-Path $PSScriptRoot '..\..\..\.toolchains'
    )
)

$ErrorActionPreference = 'Stop'
$emsdkCommit = 'c69d433d8509c5c64564c2f0d054bf102a5cf67e'
$vcpkgCommit = 'a0400024711b283056538ac19ced80b91a83c24c'
$emsdk = (Resolve-Path (
    Join-Path $ToolchainRoot 'emsdk-4.0.7'
)).Path
$vcpkg = (Resolve-Path (
    Join-Path $ToolchainRoot 'vcpkg-a0400024'
)).Path

function Assert-Commit {
    param([string]$Repository, [string]$Expected)
    $actual = git -C $Repository rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $Expected) {
        throw "Expected $Repository at $Expected, got $actual"
    }
}

Assert-Commit -Repository $emsdk -Expected $emsdkCommit
Assert-Commit -Repository $vcpkg -Expected $vcpkgCommit
$activationFile = Join-Path $emsdk '.emscripten'
if (-not (Test-Path -LiteralPath $activationFile)) {
    throw 'Pinned emsdk is installed but not locally activated'
}
$activation = Get-Content -LiteralPath $activationFile -Raw
if (-not $activation.Contains(
    "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'"
)) {
    throw 'Pinned emsdk activation does not select upstream/emscripten'
}

. (Join-Path $emsdk 'emsdk_env.ps1')
$activeEmsdk = (Resolve-Path $env:EMSDK).Path
if ($activeEmsdk -ne $emsdk) {
    throw "Expected EMSDK=$emsdk, got $activeEmsdk"
}
$env:EMSDK = $emsdk
$env:EMSCRIPTEN_ROOT = Join-Path $emsdk 'upstream\emscripten'
$env:EMSCRIPTEN_VERSION = '4.0.7'
$env:VCPKG_ROOT = $vcpkg
$env:Path = "$vcpkg$([IO.Path]::PathSeparator)$env:Path"
$env:VCPKG_DISABLE_METRICS = '1'
$env:VCPKG_DEFAULT_BINARY_CACHE = (
    Join-Path $PSScriptRoot '..\..\..\.wasm-vcpkg\bincache'
)
New-Item -ItemType Directory `
    -Path $env:VCPKG_DEFAULT_BINARY_CACHE -Force | Out-Null

$emxx = Join-Path $env:EMSCRIPTEN_ROOT 'em++.bat'
$emxxVersion = & $emxx --version
$emxxVersionText = $emxxVersion -join "`n"
if ($LASTEXITCODE -ne 0 -or $emxxVersionText -notmatch '\b4\.0\.7\b') {
    throw "Expected em++ 4.0.7, got: $($emxxVersion -join ' ')"
}
$cmakeVersion = (& cmake --version) -join "`n"
if ($LASTEXITCODE -ne 0 -or
    $cmakeVersion -notmatch '(?m)^cmake version 4\.2\.3$') {
    throw "Expected CMake 4.2.3, got: $cmakeVersion"
}
$ninjaVersion = (& ninja --version) -join "`n"
if ($LASTEXITCODE -ne 0 -or $ninjaVersion.Trim() -ne '1.13.2') {
    throw "Expected Ninja 1.13.2, got: $ninjaVersion"
}

& $Executable @Arguments
exit $LASTEXITCODE
```

- [ ] **Step 4: Run static tests**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_toolchain_contract.py -v
```

Expected: all tests pass.

- [ ] **Step 5: Bootstrap without changing the global emsdk**

Run:

```powershell
pwsh -File tools/wasm-probe/scripts/Bootstrap-Toolchains.ps1
```

Expected: the exact emsdk, vcpkg, CMake, and Ninja paths are printed and all
installs exit zero. A long download/install is normal.

- [ ] **Step 6: Verify exact versions through the wrapper**

Run:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- em++ --version
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- vcpkg version
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- git -C .toolchains/vcpkg-a0400024 rev-parse HEAD
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --version
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- ninja --version
```

Expected:

- `em++` reports `4.0.7`;
- vcpkg runs successfully;
- Git prints `a0400024711b283056538ac19ced80b91a83c24c`.
- CMake reports exactly `4.2.3`;
- Ninja reports exactly `1.13.2`.

- [ ] **Step 7: Apply the mandatory adversarial hardening**

The initial scripts above are a deliberately small RED/GREEN skeleton. Do not
accept or commit them until all of the following review findings are closed.

Extend `toolchain-lock.json` with the exact CMake and Ninja artifact objects
shown in Task 1. `Bootstrap-Toolchains.ps1` must read the version, URL,
SHA-256, and destination directory from that lock rather than relying on
mutable host tools.

Provision build tools beneath the private toolchain root:

```text
.toolchains/
  cmake-4.2.3-windows-x86_64/bin/cmake.exe
  ninja-1.13.2-win/ninja.exe
```

Use these exact official archives:

```text
https://cmake.org/files/v4.2/cmake-4.2.3-windows-x86_64.zip
SHA-256 eb4ebf5155dbb05436d675706b2a08189430df58904257ae5e91bcba4c86933c

https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-win.zip
SHA-256 07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65
```

For each archive:

1. Download to a script-owned `*.download-tmp` file under
   `.toolchains/downloads`.
2. Verify SHA-256 before renaming or extracting.
3. Extract into an exact script-owned sibling ending in `.bootstrap-tmp`.
4. Verify the expected executable exists.
5. Atomically rename the completed directory to its canonical name.
6. On a retry, remove only the known temporary file/directory. Never replace
   an existing canonical installation silently; validate it and fail closed
   if its version or layout is wrong.

CMake's ZIP already contains the
`cmake-4.2.3-windows-x86_64` top-level directory. Ninja's ZIP is flat and
contains `ninja.exe`, so extract it into `ninja-1.13.2-win`.

Treat every lock-derived directory as untrusted input before filesystem use.
Reject empty, rooted, `.`/`..`, separator-containing, or otherwise non-leaf
artifact directory fields. Compute full canonical, `.bootstrap-tmp`, download,
and executable paths, then prove each is a strict descendant of its intended
private root before any creation, recursive removal, extraction, resolution,
or execution. Apply the same descendant checks to the emsdk/vcpkg canonical
and temporary paths. `Invoke-WithToolchains.ps1` must repeat the canonical
containment checks before accepting a directory as a provenance root.

Replace both direct repository clones with one shared atomic-clone helper:

1. If the canonical repository exists, check its exact `HEAD` and do not
   mutate it.
2. Otherwise delete only the reserved sibling
   `<canonical>.bootstrap-tmp`, clone into that sibling, detach at the pinned
   commit, and verify `HEAD`.
3. Rename the verified sibling to the canonical directory.
4. Use `try`/`finally` to remove the reserved sibling after ordinary failure.
   A stale sibling left by process termination must be recoverable on the next
   invocation.
5. Never delete similarly named user paths or repair a drifted canonical
   checkout.

After dot-sourcing `emsdk_env.ps1`, prepend all four pinned command
directories to `PATH`: vcpkg, CMake `bin`, Ninja, and Emscripten. Resolve
`em++`, `vcpkg`, `cmake`, and `ninja` as application commands, and reject any
resolved source that is not a descendant of the expected private directory.
Run the version probes through those resolved commands. Tests may use `.cmd`
or `.bat` application shims for the fixed internal version probes, while
production installs resolve to the pinned `.exe`/`.bat` files.

Give `Invoke-WithToolchains.ps1` an optional `-BinaryCache` parameter whose
default remains `.wasm-vcpkg/bincache`. Hermetic tests pass a temporary cache;
using a custom `-ToolchainRoot` must not write test state into the repository.
Keep executable/remaining-argument forwarding transparent and return the
child's exact exit code.

Do not use an advanced PowerShell `param` binder for the child command. The
public CLI is:

```text
Invoke-WithToolchains.ps1 [-ToolchainRoot PATH] [-BinaryCache PATH] -- \
    EXECUTABLE [ARGUMENT...]
```

Parse raw `$args` manually. Only recognize the two wrapper options before the
first mandatory `--`; reject unknown/missing wrapper options and a missing
executable. Everything after the executable is child data, including literal
`--`, `-Verbose`, `-ToolchainRoot`, `-BinaryCache`, option-like strings, and
empty arguments. All plan commands use the delimiter, for example:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --version
```

Do not construct a `cmd.exe` command string for the child. Arbitrary batch
files do not expose a lossless Windows `argv[]` boundary: a target can reparse
`%*`, use `call`, or enable delayed expansion. Remove
`ConvertTo-CmdArgument` and the `cmd.exe call` branch.

Resolve the requested child once. Reject a generic `.cmd` or `.bat` child
before execution with a clear error. Preserve the required Emscripten commands
by recognizing only `em++`/`emcc` aliases or path-equivalent launchers beneath
the pinned Emscripten root, then dispatching:

```text
<pinned EMSDK_PYTHON> -E <pinned em++.py or emcc.py> [ARGUMENT...]
```

Validate the Python executable beneath the pinned emsdk and the driver beneath
the pinned Emscripten root before launch. Use
`System.Diagnostics.ProcessStartInfo` with `UseShellExecute = $false` and add
each unescaped argument through `ArgumentList`; inherit standard handles, wait,
and propagate `ExitCode`. Use that same native launcher for all non-batch
children. Do not use `Start-Process -ArgumentList`, a joined command string,
`cmd.exe`, or `call`.

- [ ] **Step 8: Add hermetic behavioral tests**

Create `tools/wasm-probe/tests/_toolchain_process_double.py` and
`tools/wasm-probe/tests/test_toolchain_scripts.py`. The tests are Windows-only,
use absolute `pwsh.exe`, run with `-NoLogo -NoProfile -NonInteractive`, impose
a 20-second timeout, and never access the network or global toolchain
directories.

The process double must:

- be reached through generated `.cmd`/`.bat` launchers;
- log tool name, exact argument array, working directory, and source to an
  `events.jsonl` file;
- model Git clone/checkout/`rev-parse HEAD` using a `.fixture-head` file;
- recognize only the two expected repository URLs and destinations inside the
  temporary sandbox;
- support a fail-once clone mode with a distinctive exit code;
- create the tiny fake emsdk/vcpkg layouts needed by the real scripts; and
- fail every unknown command.

Every test uses a `TemporaryDirectory` whose `ToolchainRoot` contains spaces.
Set invalid loopback HTTP/HTTPS proxies as a network backstop.

Implement these five behavioral cases:

1. `test_wrong_heads_fail_closed`: for both emsdk and vcpkg, verify bootstrap
   and wrapper entry points reject a wrong canonical `HEAD`, report expected
   and actual SHAs, do not mutate the repository, and run no later
   activation/bootstrap/version/child process.
2. `test_bootstrap_recovers_owned_partial_clone`: seed stale exact
   `.bootstrap-tmp` siblings and an unrelated similarly named sentinel. Make
   the first fake clone fail, then rerun. Verify clones target only the
   reserved siblings, canonical directories appear only after verified
   checkout, the second run succeeds, temporary siblings disappear, the
   unrelated sentinel survives, and the only SDK commands are
   `install 4.0.7`, `activate 4.0.7`, and vcpkg `-disableMetrics`.
3. `test_wrapper_is_hermetic_transparent_and_propagates_exit`: poison caller
   variables and place version-compatible global shims first on the inherited
   `PATH`. Supply local build-tool shims beneath the private root. Invoke the
   committed wrapper directly with `subprocess.run([pwsh, ..., "-File",
   wrapper, ...])`; do not use `pwsh -Command` or a preconstructed PowerShell
   array. Use a native capture child, not a batch proxy. Pass ordinary, spaced,
   empty, literal `--`, `-Verbose`, wrapper-option names, option-like,
   `-DNAME=a b`, terminal backslashes, embedded quotes, `%PATH%`, carets,
   `&|<>()`, Unicode, and mixed-metacharacter arguments; make the child exit
   `37`. Assert byte-for-byte argument preservation, exit `37`, local exported
   paths/cache, local command provenance for all four tools, no poison-shim
   events or side-effect commands/files, and an unchanged parent environment.
4. `test_lock_directories_cannot_escape_toolchain_root`: run copied scripts
   against a copied malicious lock containing rooted and `..` artifact
   directories. For both bootstrap and wrapper, assert failure occurs before
   Git, download, cleanup, environment loading, version probes, or child
   execution; an outside sentinel and similarly named paths remain byte
   unchanged. Also cover a valid leaf name to prevent a test that rejects all
   lock values.
5. `test_batch_children_fail_closed_and_emscripten_uses_native_driver`: verify
   explicit and PATH-resolved unknown `.cmd`/`.bat` children fail before their
   first event. Verify pinned `em++` and `emcc` aliases/path-equivalents map to
   the pinned Python drivers, preserve the same hostile argument vector, and
   propagate the native process exit. A missing/outside Python or driver must
   fail closed. Assert the wrapper contains no `cmd.exe`, `ComSpec`, `call`, or
   batch command-string construction.

Remove the shallow
`test_bootstrap_is_local_and_fails_on_pin_drift` source-substring test once
these behavioral tests are in place. Keep the static lock assertions for every
repository commit, artifact URL, version, and SHA-256.

- [ ] **Step 9: Re-run bootstrap, behavioral verification, and commit**

Run:

```powershell
python -m unittest tools/wasm-probe/tests/test_toolchain_contract.py -v
pwsh -File tools/wasm-probe/scripts/Bootstrap-Toolchains.ps1
python -m unittest tools/wasm-probe/tests/test_toolchain_scripts.py -v

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --version
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- ninja --version

git diff --check
git add tools/wasm-probe/toolchain-lock.json `
    tools/wasm-probe/scripts tools/wasm-probe/tests
git commit -m "build: harden pinned Wasm tools"
```

Expected: all static and behavioral tests pass; CMake and Ninja resolve beneath
`.toolchains` and report `4.2.3`/`1.13.2`; no poison/global tool is invoked;
the bootstrap rerun is successful; and the tracked diff is clean.

---

### Task 3: Add and build the minimal Qt Quick probe

**Files:**

- Create: `tools/wasm-probe/CMakeLists.txt`
- Create: `tools/wasm-probe/CMakePresets.json`
- Create: `tools/wasm-probe/cmake/verify_exact_toolchain.cmake`
- Create: `tools/wasm-probe/src/ExceptionBoundary.h`
- Create: `tools/wasm-probe/src/ExceptionBoundary.cpp`
- Create: `tools/wasm-probe/src/ProbeState.h`
- Create: `tools/wasm-probe/src/ProbeState.cpp`
- Create: `tools/wasm-probe/src/main.cpp`
- Create: `tools/wasm-probe/qml/Main.qml`
- Create: `tools/wasm-probe/qml/pulse.frag`
- Create: `tools/wasm-probe/tests/test_probe_source_contract.py`

**Interfaces:**

- Consumes: Task 1 triplets/manifest/overlays and Task 2 command wrapper.
- Produces: CMake target `RhythmGameWasmProbe`, QML module
  `RhythmGame.WasmProbe`, static library `WasmProbeExceptionBoundary`, and
  `build/wasm-release/compile_commands.json`.

Qt/Emscripten's Windows platform module selects `emcc.bat`/`em++.bat`
internally. The real configure and compile below are therefore the required
end-to-end compiler-argument proof. If that boundary corrupts a real argument,
do not add generic cmd escaping: introduce a small native launcher (or a
reviewed compiler-list override) that invokes the pinned Python driver, then
pin and test it.

- [ ] **Step 1: Write the failing source contract test**

Create `tools/wasm-probe/tests/test_probe_source_contract.py`:

```python
import json
import unittest
from pathlib import Path


PROBE = Path(__file__).resolve().parents[1]


class ProbeSourceContractTest(unittest.TestCase):
    def test_preset_uses_only_pinned_vcpkg_target_and_host(self) -> None:
        presets = json.loads((PROBE / "CMakePresets.json").read_text("utf-8"))
        encoded = json.dumps(presets, sort_keys=True)
        self.assertIn("wasm32-emscripten-rg", encoded)
        self.assertIn("x64-windows-rg-host-release", encoded)
        self.assertIn("vcpkgOverlayPortsWasm", encoded)
        self.assertNotIn("Qt6_DIR", encoded)
        self.assertNotIn("CMAKE_FIND_ROOT_PATH_MODE_PACKAGE\": \"BOTH", encoded)

    def test_executable_has_all_gate_1a_link_settings(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        for setting in (
            "-pthread",
            "-fwasm-exceptions",
            "-sJSPI",
            "-sAUDIO_WORKLET=1",
            "-sWASM_WORKERS=1",
            "-sPTHREAD_POOL_SIZE=4",
            "-sPTHREAD_POOL_SIZE_STRICT=2",
            "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
        ):
            self.assertIn(setting, cmake)
        self.assertIn("WasmProbeExceptionBoundary", cmake)
        self.assertIn("qt_add_qml_module", cmake)
        self.assertIn("qt_add_shaders", cmake)
        self.assertIn('BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"', cmake)

    def test_main_returns_early_only_for_emscripten(self) -> None:
        main = (PROBE / "src" / "main.cpp").read_text("utf-8")
        self.assertIn("#ifdef __EMSCRIPTEN__", main)
        self.assertIn("return 0;", main)
        self.assertIn("return app->exec();", main)
        self.assertIn("new QGuiApplication", main)

    def test_probe_exercises_required_compile_time_surfaces(self) -> None:
        state = (PROBE / "src" / "ProbeState.cpp").read_text("utf-8")
        qml = (PROBE / "qml" / "Main.qml").read_text("utf-8")
        self.assertIn("QtConcurrent::run", state)
        self.assertIn("crossStaticLibraryBoundary", state)
        self.assertIn("QNetworkAccessManager", state)
        self.assertIn("QWebSocket", state)
        self.assertIn("ShaderEffect", qml)
        self.assertIn("MediaPlayer", qml)


if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the source test and verify it fails**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_probe_source_contract.py -v
```

Expected: four errors because the probe source and presets do not exist.

- [ ] **Step 3: Add an exact toolchain configure guard**

Create `tools/wasm-probe/cmake/verify_exact_toolchain.cmake`:

```cmake
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
```

The environment and triplet checks are authoritative. The bootstrap script
independently pins and verifies the emsdk Git commit that supplies this
Emscripten release.

- [ ] **Step 4: Add CMake presets**

Create `tools/wasm-probe/CMakePresets.json`:

```json
{
  "version": 6,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 25,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "wasm-release",
      "displayName": "Wasm Gate 1A Release",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/wasm-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "CMAKE_FIND_ROOT_PATH_MODE_PACKAGE": "ONLY",
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_HOST_TRIPLET": "x64-windows-rg-host-release",
        "VCPKG_INSTALL_OPTIONS": "--x-buildtrees-root=${sourceDir}/../../.wasm-vcpkg/buildtrees;--x-packages-root=${sourceDir}/../../.wasm-vcpkg/packages;--downloads-root=${sourceDir}/../../.wasm-vcpkg/downloads",
        "VCPKG_INSTALLED_DIR": "${sourceDir}/../../.wasm-vcpkg/installed",
        "VCPKG_MANIFEST_DIR": "${sourceDir}",
        "VCPKG_OVERLAY_PORTS": "${sourceDir}/../../vcpkgOverlayPorts;${sourceDir}/../../vcpkgOverlayPortsWasm",
        "VCPKG_OVERLAY_TRIPLETS": "${sourceDir}/../../vcpkgTriplets",
        "VCPKG_TARGET_TRIPLET": "wasm32-emscripten-rg"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "wasm-release",
      "configurePreset": "wasm-release",
      "configuration": "Release",
      "verbose": true
    }
  ]
}
```

- [ ] **Step 5: Add the static exception boundary**

Create `tools/wasm-probe/src/ExceptionBoundary.h`:

```cpp
#pragma once

int crossStaticLibraryBoundary();
```

Create `tools/wasm-probe/src/ExceptionBoundary.cpp`:

```cpp
#include "ExceptionBoundary.h"

#include <stdexcept>

int crossStaticLibraryBoundary()
{
    throw std::runtime_error{"wasm-native-exception"};
}
```

- [ ] **Step 6: Add the compile-time/runtime state surface**

Create `tools/wasm-probe/src/ProbeState.h`:

```cpp
#pragma once

#include <QObject>

class QNetworkAccessManager;
class QWebSocket;

class ProbeState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool exceptionPassed READ exceptionPassed NOTIFY exceptionPassedChanged)
    Q_PROPERTY(bool threadPassed READ threadPassed NOTIFY threadPassedChanged)

public:
    explicit ProbeState(QObject* parent = nullptr);

    [[nodiscard]] bool exceptionPassed() const;
    [[nodiscard]] bool threadPassed() const;

signals:
    void exceptionPassedChanged();
    void threadPassedChanged();

private:
    bool m_exceptionPassed = false;
    bool m_threadPassed = false;
    QNetworkAccessManager* m_network = nullptr;
    QWebSocket* m_webSocket = nullptr;
};
```

Create `tools/wasm-probe/src/ProbeState.cpp`:

```cpp
#include "ProbeState.h"

#include "ExceptionBoundary.h"

#include <QFutureWatcher>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QtConcurrent>

#include <stdexcept>
#include <string_view>

ProbeState::ProbeState(QObject* parent)
    : QObject{parent}
    , m_network{new QNetworkAccessManager{this}}
    , m_webSocket{new QWebSocket{QString{}, QWebSocketProtocol::VersionLatest,
                                this}}
{
    try {
        static_cast<void>(crossStaticLibraryBoundary());
    } catch (const std::runtime_error& error) {
        m_exceptionPassed =
            std::string_view{error.what()} == "wasm-native-exception";
        emit exceptionPassedChanged();
    }

    auto* watcher = new QFutureWatcher<int>{this};
    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher] {
        m_threadPassed = watcher->result() == 42;
        emit threadPassedChanged();
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([] { return 42; }));
}

bool ProbeState::exceptionPassed() const
{
    return m_exceptionPassed;
}

bool ProbeState::threadPassed() const
{
    return m_threadPassed;
}
```

- [ ] **Step 7: Add the early-return entry point**

Create `tools/wasm-probe/src/main.cpp`:

```cpp
#include "ProbeState.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char* argv[])
{
    auto* app = new QGuiApplication{argc, argv};
    auto* state = new ProbeState{app};
    auto* engine = new QQmlApplicationEngine{app};
    engine->rootContext()->setContextProperty("probeState", state);
    engine->loadFromModule("RhythmGame.WasmProbe", "Main");
    if (engine->rootObjects().isEmpty()) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    return 0;
#else
    return app->exec();
#endif
}
```

- [ ] **Step 8: Add QML animation, Multimedia type, and QSB shader**

Create `tools/wasm-probe/qml/Main.qml`:

```qml
import QtQuick
import QtMultimedia

Window {
    id: root
    width: 640
    height: 360
    visible: true
    color: "#101216"
    title: "RhythmGame Wasm Gate 1A"

    MediaPlayer {
        id: mediaProbe
    }

    ShaderEffect {
        id: shader
        anchors.fill: parent
        property real phase: 0.0
        fragmentShader: "qrc:/qt/qml/RhythmGame/WasmProbe/shaders/pulse.frag.qsb"

        NumberAnimation on phase {
            from: 0.0
            to: 1.0
            duration: 1000
            loops: Animation.Infinite
        }
    }

    Text {
        anchors.centerIn: parent
        color: "white"
        text: probeState.exceptionPassed && probeState.threadPassed
              ? "gate-1a-ready"
              : "gate-1a-starting"
    }
}
```

Create `tools/wasm-probe/qml/pulse.frag`:

```glsl
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
};

void main()
{
    float glow = 0.2 + 0.15 * sin(phase * 6.28318530718);
    fragColor = vec4(glow, glow * 1.4, glow * 2.0, 1.0) * qt_Opacity;
}
```

- [ ] **Step 9: Add the isolated CMake project**

Create `tools/wasm-probe/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES CXX)

include(cmake/verify_exact_toolchain.cmake)

find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS
    Concurrent
    Multimedia
    Network
    Qml
    Quick
    ShaderTools
    WebSockets
)

qt_standard_project_setup(REQUIRES 6.11)

add_library(WasmProbeExceptionBoundary STATIC
    src/ExceptionBoundary.cpp
    src/ExceptionBoundary.h
)
target_compile_features(WasmProbeExceptionBoundary PRIVATE cxx_std_20)

qt_add_executable(RhythmGameWasmProbe
    src/main.cpp
    src/ProbeState.cpp
    src/ProbeState.h
)
target_compile_features(RhythmGameWasmProbe PRIVATE cxx_std_20)

qt_add_qml_module(RhythmGameWasmProbe
    URI RhythmGame.WasmProbe
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
)

qt_add_shaders(RhythmGameWasmProbe "wasm_probe_shaders"
    PREFIX "/qt/qml/RhythmGame/WasmProbe/shaders"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"
    FILES
        qml/pulse.frag
)

target_link_libraries(RhythmGameWasmProbe PRIVATE
    WasmProbeExceptionBoundary
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::Network
    Qt6::Quick
    Qt6::WebSockets
)

target_link_options(RhythmGameWasmProbe PRIVATE
    -pthread
    -fwasm-exceptions
    "SHELL:-sJSPI"
    "SHELL:-sAUDIO_WORKLET=1"
    "SHELL:-sWASM_WORKERS=1"
    "SHELL:-sPTHREAD_POOL_SIZE=4"
    "SHELL:-sPTHREAD_POOL_SIZE_STRICT=2"
    "SHELL:-sALLOW_BLOCKING_ON_MAIN_THREAD=0"
)
```

- [ ] **Step 10: Run source contracts**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_toolchain_contract.py `
    tools/wasm-probe/tests/test_probe_source_contract.py -v
```

Expected: all tests pass.

- [ ] **Step 11: Configure and allow vcpkg to finish**

Run:

```powershell
Push-Location tools/wasm-probe
try {
    pwsh -File scripts/Invoke-WithToolchains.ps1 `
        -- cmake --preset wasm-release
} finally {
    Pop-Location
}
```

Expected: the command may run for a long time while native host Qt and target
Qt build. It exits zero, selects `wasm32-emscripten-rg` for target packages,
selects `x64-windows-rg-host-release` for tools, and reports Qt 6.11.1.

If configuration fails, retain the exact port log and diagnose that boundary;
do not switch to the native root manifest, external Qt, shared Qt, Asyncify, or
Emscripten 3.1.70.

- [ ] **Step 12: Build verbosely**

Run:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --build `
    tools/wasm-probe/build/wasm-release --verbose
```

Expected: exit zero and generated `.wasm`, `.js`, `.html`, Qt loader, pthread
worker, Wasm Worker, and AudioWorklet bootstrap artifacts.

- [ ] **Step 13: Commit the probe**

Run:

```powershell
git diff --check
git add tools/wasm-probe/CMakeLists.txt `
    tools/wasm-probe/CMakePresets.json tools/wasm-probe/cmake `
    tools/wasm-probe/src tools/wasm-probe/qml `
    tools/wasm-probe/tests/test_probe_source_contract.py
git commit -m "test: add Wasm Qt capability probe"
```

---

### Task 4: Audit the generated Qt/toolchain evidence

**Files:**

- Create: `tools/wasm-probe/tests/verify_build.py`
- Create: `docs/superpowers/evidence/emscripten-gate-1a.json`

**Interfaces:**

- Consumes: Task 3 build tree and `.wasm-vcpkg` target installation/buildtrees.
- Produces: machine-readable evidence with tool versions, target/host triplets,
  Qt feature state, static-package result, compile/link flag counts, generated
  artifact hashes, and an explicit `gate1aPassed` boolean.

- [ ] **Step 1: Write the build verifier**

Create `tools/wasm-probe/tests/verify_build.py`:

```python
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


EXPECTED_EMSCRIPTEN = "4.0.7"
EXPECTED_QT = "6.11.1"
EXPECTED_EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
EXPECTED_VCPKG_COMMIT = "a0400024711b283056538ac19ced80b91a83c24c"
EXPECTED_QTBASE_TREE = "29a7f9f115d568b271a3b99fabeac886ec248f9f"
EXPECTED_QTBASE_WASM_PATCH = (
    "d9ed64da369eeb3aedc6830a2649925bcf8e8742d1c5fc86c21adf5bb168d5a0"
)
EXPECTED_QTDECLARATIVE_PATCH = (
    "2a015242af462be117a2924d4d8db2c753b29891921e714c23bf1ab4355c4c50"
)
TARGET_TRIPLET = "wasm32-emscripten-rg"
HOST_TRIPLET = "x64-windows-rg-host-release"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def run(*command: str | Path) -> str:
    result = subprocess.run(
        [str(part) for part in command],
        check=True,
        capture_output=True,
        text=True,
    )
    return "\n".join(
        part for part in (result.stdout.strip(), result.stderr.strip()) if part
    )


def git(repository: Path, *arguments: str) -> str:
    return run("git", "-C", repository, *arguments)


def require_qt_version(root: Path, label: str) -> str:
    files = list(root.rglob("Qt6ConfigVersion.cmake"))
    require(len(files) == 1, f"{label}: expected one Qt6ConfigVersion.cmake")
    implementation = files[0].with_name("Qt6ConfigVersionImpl.cmake")
    require(implementation.is_file(), f"{label}: version implementation missing")
    text = implementation.read_text("utf-8", errors="replace")
    match = re.search(r'set\(PACKAGE_VERSION "([^"]+)"\)', text)
    require(match is not None, f"{label}: PACKAGE_VERSION missing")
    observed = match.group(1)
    require(observed == EXPECTED_QT, f"{label}: Qt {observed}")
    return observed


def require_exact_overlay(
    repo: Path,
    vcpkg: Path,
    port: str,
    modified: set[str],
    additions: set[str],
) -> tuple[dict[str, str], str]:
    prefix = f"ports/{port}/"
    entries = git(
        vcpkg,
        "ls-tree",
        "-r",
        EXPECTED_VCPKG_COMMIT,
        f"ports/{port}",
    ).splitlines()
    baseline_blobs: dict[str, str] = {}
    for entry in entries:
        match = re.fullmatch(r"\d+ blob ([0-9a-f]{40})\s+(.+)", entry)
        require(match is not None, f"unexpected {port} tree entry: {entry}")
        baseline_blobs[match.group(2)[len(prefix):]] = match.group(1)

    overlay = repo / "vcpkgOverlayPorts" / port
    if port == "qtbase":
        overlay = repo / "vcpkgOverlayPortsWasm" / port
    actual = {
        path.relative_to(overlay).as_posix()
        for path in overlay.rglob("*")
        if path.is_file()
    }
    require(
        actual == set(baseline_blobs) | additions,
        f"{port}: unexpected overlay files: "
        f"{sorted(actual ^ (set(baseline_blobs) | additions))}",
    )
    for relative, expected_blob in baseline_blobs.items():
        if relative in modified:
            continue
        actual_blob = run(
            "git",
            "hash-object",
            "--no-filters",
            overlay / relative,
        )
        require(
            actual_blob == expected_blob,
            f"{port}: baseline byte drift in {relative}",
        )
    tree = git(
        vcpkg,
        "rev-parse",
        f"{EXPECTED_VCPKG_COMMIT}:ports/{port}",
    )
    return {
        relative: sha256(overlay / relative)
        for relative in sorted(actual)
    }, tree


def verify_overlays(repo: Path, vcpkg: Path) -> dict[str, object]:
    qtbase_hashes, qtbase_tree = require_exact_overlay(
        repo,
        vcpkg,
        "qtbase",
        {"portfile.cmake", "vcpkg.json"},
        {"restore-wasm-version-check.patch"},
    )
    require(qtbase_tree == EXPECTED_QTBASE_TREE, qtbase_tree)

    baseline_portfile = git(
        vcpkg,
        "show",
        f"{EXPECTED_VCPKG_COMMIT}:ports/qtbase/portfile.cmake",
    ) + "\n"
    expected_portfile = baseline_portfile.replace(
        "set(${PORT}_PATCHES\n",
        "set(${PORT}_PATCHES\n"
        "        restore-wasm-version-check.patch\n",
        1,
    )
    feature_block = """if(VCPKG_TARGET_IS_EMSCRIPTEN)
    list(APPEND FEATURE_OPTIONS
        -DFEATURE_thread:BOOL=ON
        -DFEATURE_wasm_exceptions:BOOL=ON
        -DFEATURE_wasm_jspi:BOOL=ON
        -DFEATURE_wasm_simd128:BOOL=OFF
    )
endif()

"""
    expected_portfile = expected_portfile.replace(
        "qt_install_submodule(",
        feature_block + "qt_install_submodule(",
        1,
    )
    workaround = (
        "if(VCPKG_TARGET_IS_EMSCRIPTEN)\n"
        '  vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6Core/'
        'Qt6WasmMacros.cmake" "_qt_test_emscripten_version()" "") '
        "# this is missing a include(QtPublicWasmToolchainHelpers)\n"
        "endif()\n\n"
    )
    expected_portfile = expected_portfile.replace(workaround, "", 1)
    qtbase = repo / "vcpkgOverlayPortsWasm" / "qtbase"
    require(
        sha256(qtbase / "restore-wasm-version-check.patch")
        == EXPECTED_QTBASE_WASM_PATCH,
        "qtbase Wasm patch payload drift",
    )
    require(
        (qtbase / "portfile.cmake").read_text("utf-8") == expected_portfile,
        "qtbase portfile has changes outside the reviewed Wasm delta",
    )
    baseline_json = json.loads(
        git(
            vcpkg,
            "show",
            f"{EXPECTED_VCPKG_COMMIT}:ports/qtbase/vcpkg.json",
        )
    )
    baseline_json["port-version"] = 1
    require(
        json.loads((qtbase / "vcpkg.json").read_text("utf-8"))
        == baseline_json,
        "qtbase vcpkg.json has changes beyond port-version 1",
    )

    declarative_hashes, declarative_tree = require_exact_overlay(
        repo,
        vcpkg,
        "qtdeclarative",
        {"portfile.cmake", "vcpkg.json"},
        {"24205cd-qquickwindow-child-window-stacking.patch"},
    )
    declarative = repo / "vcpkgOverlayPorts" / "qtdeclarative"
    baseline_declarative_portfile = git(
        vcpkg,
        "show",
        f"{EXPECTED_VCPKG_COMMIT}:ports/qtdeclarative/portfile.cmake",
    ) + "\n"
    expected_declarative_portfile = baseline_declarative_portfile.replace(
        'set(${PORT}_PATCHES "")',
        "set(${PORT}_PATCHES\n"
        "    24205cd-qquickwindow-child-window-stacking.patch\n"
        ")",
        1,
    )
    require(
        (declarative / "portfile.cmake").read_text("utf-8")
        == expected_declarative_portfile,
        "qtdeclarative portfile has changes outside the reviewed patch delta",
    )
    declarative_json = json.loads(
        git(
            vcpkg,
            "show",
            f"{EXPECTED_VCPKG_COMMIT}:ports/qtdeclarative/vcpkg.json",
        )
    )
    declarative_json["port-version"] = 1
    require(
        json.loads((declarative / "vcpkg.json").read_text("utf-8"))
        == declarative_json,
        "qtdeclarative vcpkg.json has changes beyond port-version 1",
    )
    require(
        sha256(
            declarative
            / "24205cd-qquickwindow-child-window-stacking.patch"
        )
        == EXPECTED_QTDECLARATIVE_PATCH,
        "qtdeclarative patch payload drift",
    )
    return {
        "qtbaseBaselineTree": qtbase_tree,
        "qtbaseFiles": qtbase_hashes,
        "qtdeclarativeBaselineTree": declarative_tree,
        "qtdeclarativeFiles": declarative_hashes,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--emsdk", type=Path, required=True)
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    repo = args.repo.resolve()
    emsdk = args.emsdk.resolve()
    vcpkg = args.vcpkg.resolve()
    build = repo / "tools" / "wasm-probe" / "build" / "wasm-release"
    installed = repo / ".wasm-vcpkg" / "installed"
    target = installed / TARGET_TRIPLET
    host = installed / HOST_TRIPLET
    buildtrees = repo / ".wasm-vcpkg" / "buildtrees"

    emsdk_head = git(emsdk, "rev-parse", "HEAD")
    vcpkg_head = git(vcpkg, "rev-parse", "HEAD")
    require(emsdk_head == EXPECTED_EMSDK_COMMIT, emsdk_head)
    require(vcpkg_head == EXPECTED_VCPKG_COMMIT, vcpkg_head)
    emxx_version = run(
        emsdk / "upstream" / "emscripten" / "em++.bat",
        "--version",
    )
    require(EXPECTED_EMSCRIPTEN in emxx_version, emxx_version)
    vcpkg_version = run(vcpkg / "vcpkg.exe", "version")
    cmake_version = run("cmake", "--version")
    ninja_version = run("ninja", "--version")
    require("cmake version 4.2.3" in cmake_version, cmake_version)
    require(ninja_version.strip() == "1.13.2", ninja_version)
    require(target.is_dir(), f"missing target installation: {target}")
    require(host.is_dir(), f"missing host installation: {host}")

    target_qt_version = require_qt_version(target, "target")
    host_qt_version = require_qt_version(host, "host")

    config_headers = list(target.rglob("qconfig*.h"))
    config_text = "\n".join(
        path.read_text("utf-8", errors="replace")
        for path in config_headers
    )
    for feature in (
        "QT_FEATURE_thread 1",
        "QT_FEATURE_wasm_exceptions 1",
        "QT_FEATURE_wasm_jspi 1",
    ):
        require(feature in config_text, f"missing Qt feature: {feature}")
    require(
        "QT_FEATURE_wasm_simd128 -1" in config_text
        or "QT_FEATURE_wasm_simd128 0" in config_text,
        "Gate 1A SIMD must be disabled",
    )
    require(
        '#define QT_EMCC_VERSION "4.0.7"' in config_text,
        "target Qt was not configured with Emscripten 4.0.7",
    )

    wasm_macros = target / "share" / "Qt6Core" / "Qt6WasmMacros.cmake"
    macro_text = wasm_macros.read_text("utf-8")
    require(
        "include(QtPublicWasmToolchainHelpers)" in macro_text,
        "Emscripten version helper include missing",
    )
    require(
        "_qt_test_emscripten_version()" in macro_text,
        "Emscripten version check was suppressed",
    )

    target_shared = [
        path
        for suffix in ("*.dll", "*.so", "*.dylib")
        for path in target.rglob(suffix)
    ]
    require(not target_shared, f"target shared libraries: {target_shared}")
    target_archives = list(target.rglob("*.a"))
    require(target_archives, "no target static libraries were installed")
    host_core_dlls = list((host / "bin").glob("Qt6Core*.dll"))
    require(host_core_dlls, "host Qt is not a dynamic Windows build")

    host_tool_specs = {
        "moc.exe": ("--version",),
        "qmlcachegen.exe": ("--version",),
        "qmltyperegistrar.exe": ("--version",),
        "qsb.exe": ("--version",),
        "lrelease.exe": ("-version",),
        "lupdate.exe": ("-version",),
    }
    host_tools: dict[str, str] = {}
    for name, options in host_tool_specs.items():
        candidates = list(host.rglob(name))
        require(len(candidates) == 1, f"expected one host {name}: {candidates}")
        output = run(candidates[0], *options)
        require(EXPECTED_QT in output, f"{name}: {output}")
        host_tools[name] = output.splitlines()[0]

    compile_databases = [build / "compile_commands.json"]
    compile_databases.extend(buildtrees.rglob("compile_commands.json"))
    cpp_commands = []
    c_commands = []
    target_port_cpp_counts: dict[str, int] = {}
    for database in compile_databases:
        if not database.is_file():
            continue
        for entry in json.loads(database.read_text("utf-8")):
            command = entry.get("command", "")
            if "em++" in command or "clang++" in command:
                cpp_commands.append(command)
                port = "probe"
                if database.is_relative_to(buildtrees):
                    port = database.relative_to(buildtrees).parts[0]
                target_port_cpp_counts[port] = (
                    target_port_cpp_counts.get(port, 0) + 1
                )
            elif "emcc" in command or "clang" in command:
                c_commands.append(command)

    require(cpp_commands, "no Emscripten C++ compile commands found")
    for command in cpp_commands:
        require("-pthread" in command, f"missing -pthread: {command}")
        require(
            "-fwasm-exceptions" in command,
            f"missing -fwasm-exceptions: {command}",
        )
    for command in c_commands:
        require("-pthread" in command, f"missing C -pthread: {command}")
    for port in (
        "qtbase",
        "qtdeclarative",
        "qtimageformats",
        "qtmultimedia",
        "qtshadertools",
        "qtsvg",
        "qtwebsockets",
    ):
        require(
            target_port_cpp_counts.get(port, 0) > 0,
            f"no audited target C++ commands for {port}",
        )

    target_commands = run(
        "ninja",
        "-C",
        build,
        "-t",
        "commands",
        "RhythmGameWasmProbe",
    )
    application_links = [
        line
        for line in target_commands.splitlines()
        if "RhythmGameWasmProbe.js" in line
        and ("em++" in line or "clang++" in line)
    ]
    require(
        len(application_links) == 1,
        f"expected one application link command: {application_links}",
    )
    application_link = application_links[0]
    for flag in (
        "-pthread",
        "-fwasm-exceptions",
        "-sJSPI",
        "-sAUDIO_WORKLET=1",
        "-sWASM_WORKERS=1",
        "-sPTHREAD_POOL_SIZE=4",
        "-sPTHREAD_POOL_SIZE_STRICT=2",
        "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
    ):
        require(flag in application_link, f"missing app link setting: {flag}")

    expected_artifacts = {
        "RhythmGameWasmProbe.html",
        "RhythmGameWasmProbe.js",
        "RhythmGameWasmProbe.wasm",
        "RhythmGameWasmProbe.worker.js",
        "RhythmGameWasmProbe.ww.js",
        "RhythmGameWasmProbe.aw.js",
        "qtloader.js",
        "qtlogo.svg",
    }
    artifacts = [build / name for name in sorted(expected_artifacts)]
    for artifact in artifacts:
        require(artifact.is_file(), f"missing deployment artifact: {artifact}")
    generated_workers = {
        path.name
        for path in build.iterdir()
        if path.is_file()
        and path.name.endswith((".worker.js", ".ww.js", ".aw.js"))
    }
    require(
        generated_workers
        == {
            "RhythmGameWasmProbe.worker.js",
            "RhythmGameWasmProbe.ww.js",
            "RhythmGameWasmProbe.aw.js",
        },
        f"unexpected worker deployment set: {generated_workers}",
    )
    html = (build / "RhythmGameWasmProbe.html").read_text(
        "utf-8", errors="replace"
    )
    require(
        re.search(r'<script[^>]+src=["\']qtloader\.js["\']', html)
        is not None,
        "HTML does not load qtloader.js",
    )
    require(
        re.search(
            r'<script[^>]+src=["\']RhythmGameWasmProbe\.js["\']',
            html,
        )
        is not None,
        "HTML does not load RhythmGameWasmProbe.js",
    )

    shader_resource_mentions: list[str] = []
    for generated in build.rglob("*"):
        if (
            generated.is_file()
            and generated.suffix in {".qrc", ".cpp", ".cmake"}
            and generated.stat().st_size < 8 * 1024 * 1024
        ):
            text = generated.read_text("utf-8", errors="replace")
            if "pulse.frag.qsb" in text:
                shader_resource_mentions.append(text)
    shader_resource_text = "\n".join(shader_resource_mentions)
    require(
        "shaders/pulse.frag.qsb" in shader_resource_text,
        "generated QSB resource alias is missing",
    )
    require(
        "shaders/qml/pulse.frag.qsb" not in shader_resource_text,
        "generated QSB resource retained the unwanted qml/ segment",
    )

    overlays = verify_overlays(repo, vcpkg)

    evidence = {
        "gate": "1A",
        "technicalProbePassed": True,
        "gate1aPassed": True,
        "gate0Satisfied": False,
        "formalGate1EntryAuthorized": False,
        "gate1Passed": False,
        "emsdkCommit": emsdk_head,
        "vcpkgCommit": vcpkg_head,
        "emscriptenVersionOutput": emxx_version.splitlines()[0],
        "vcpkgVersionOutput": vcpkg_version.splitlines()[0],
        "cmakeVersionOutput": cmake_version.splitlines()[0],
        "ninjaVersionOutput": ninja_version,
        "targetQtVersion": target_qt_version,
        "hostQtVersion": host_qt_version,
        "targetTriplet": TARGET_TRIPLET,
        "hostTriplet": HOST_TRIPLET,
        "hostTools": host_tools,
        "hostQtCoreDllCount": len(host_core_dlls),
        "targetStaticArchiveCount": len(target_archives),
        "cppCommandCount": len(cpp_commands),
        "cCommandCount": len(c_commands),
        "targetPortCppCommandCounts": target_port_cpp_counts,
        "applicationLinkCommandSha256": hashlib.sha256(
            application_link.encode("utf-8")
        ).hexdigest(),
        "overlays": overlays,
        "artifacts": {
            path.name: {
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for path in artifacts
        },
        "unprovenUntilGate1B": [
            "Chromium runtime",
            "cross-origin isolation",
            "JSPI nested event loop",
            "QML and QSB rendering",
            "static-library exception execution",
            "QtConcurrent thread execution",
            "pthread lifecycle",
            "AudioWorklet audio callback",
            "shared-memory growth",
            "network and WebSocket behavior",
            "served video",
            "OPFS and File System Access",
            "1000-cycle teardown stress"
        ]
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(evidence, indent=2) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Run the verifier**

Run through the pinned environment:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- python tools/wasm-probe/tests/verify_build.py `
    --repo . `
    --emsdk .toolchains/emsdk-4.0.7 `
    --vcpkg .toolchains/vcpkg-a0400024 `
    --output docs/superpowers/evidence/emscripten-gate-1a.json
```

Expected: exit zero and generated evidence contains
`"technicalProbePassed": true`, `"gate0Satisfied": false`,
`"formalGate1EntryAuthorized": false`, and `"gate1Passed": false`.

- [ ] **Step 3: Independently inspect host/target and Qt feature caches**

Run:

```powershell
rg -n "QT_HOST_PATH|FEATURE_(thread|wasm_exceptions|wasm_jspi|wasm_simd128)" `
    .wasm-vcpkg/buildtrees/qtbase
rg -n -- "-pthread|-fwasm-exceptions|-sJSPI|-sAUDIO_WORKLET=1|-sWASM_WORKERS=1" `
    tools/wasm-probe/build/wasm-release/build.ninja `
    .wasm-vcpkg/buildtrees
Get-ChildItem .wasm-vcpkg/installed/wasm32-emscripten-rg `
    -Recurse -Include *.dll,*.so,*.dylib
```

Expected:

- host path resolves under
  `.wasm-vcpkg/installed/x64-windows-rg-host-release`;
- all three required Qt features are enabled and SIMD is disabled;
- target compile/link logs contain the uniform flags;
- the target shared-library query prints nothing.

- [ ] **Step 4: Re-run all static contracts and the verbose build**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_toolchain_contract.py `
    tools/wasm-probe/tests/test_probe_source_contract.py -v
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --build tools/wasm-probe/build/wasm-release --verbose
git diff --check
```

Expected: all Python tests pass; the incremental build exits zero;
`git diff --check` exits zero.

- [ ] **Step 5: Commit the verifier and evidence**

Run:

```powershell
git add tools/wasm-probe/tests/verify_build.py `
    docs/superpowers/evidence/emscripten-gate-1a.json
git commit -m "test: verify Wasm Qt build contract"
```

---

### Task 5: Review Gate 1A and hand off Gate 1B

**Files:**

- Review:
  `docs/superpowers/specs/2026-07-23-emscripten-web-port-design.md`
- Review: `docs/superpowers/evidence/emscripten-gate-1a.json`
- Review: every file introduced by Tasks 1-4

**Interfaces:**

- Consumes: reviewed commits and fresh evidence.
- Produces: an explicit Gate 1A pass/fail decision and the fixed interfaces for
  the separate Gate 1B browser-runtime plan.

- [ ] **Step 1: Verify repository scope and generated-state hygiene**

Run:

```powershell
git status --short
$base = git merge-base HEAD 57f59f1cc
git diff --check "$base..HEAD"
git log --oneline 57f59f1cc..HEAD
git ls-files .toolchains .wasm-vcpkg `
    tools/wasm-probe/build .vcpkg-buildtrees .vcpkg-cache .vcpkg-packages
```

Expected:

- no generated toolchain, package, cache, or build artifact is tracked;
- only design, plan, toolchain configuration, overlay, probe, tests, and
  evidence commits are present.

- [ ] **Step 2: Re-run the complete Gate 1A verification**

Run:

```powershell
python -m unittest `
    tools/wasm-probe/tests/test_toolchain_contract.py `
    tools/wasm-probe/tests/test_probe_source_contract.py -v
Push-Location tools/wasm-probe
try {
    pwsh -File scripts/Invoke-WithToolchains.ps1 `
        -- cmake --preset wasm-release
} finally {
    Pop-Location
}
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- cmake --build tools/wasm-probe/build/wasm-release --verbose
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 `
    -- python tools/wasm-probe/tests/verify_build.py `
    --repo . `
    --emsdk .toolchains/emsdk-4.0.7 `
    --vcpkg .toolchains/vcpkg-a0400024 `
    --output docs/superpowers/evidence/emscripten-gate-1a.json
```

Expected: every command exits zero and the regenerated evidence is unchanged.

- [ ] **Step 3: Perform adversarial task and whole-slice review**

The reviewer must attempt to falsify:

- exact pin enforcement;
- static target/dynamic host separation;
- application-side Emscripten version-check restoration;
- propagation of `-pthread` and `-fwasm-exceptions` into Qt and probe C++;
- absence of Asyncify, legacy exceptions, shared Qt, native manifest leakage,
  external `Qt6_DIR`, and `CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH`;
- evidence that claims Gate 1A only, not Chromium runtime success.

Critical and Important findings are fixed and re-reviewed before Gate 1B.

- [ ] **Step 4: Freeze the Gate 1B inputs**

The next plan consumes these exact outputs:

```text
target triplet: wasm32-emscripten-rg
host triplet: x64-windows-rg-host-release
target Qt: static 6.11.1
Emscripten: 4.0.7
probe target: RhythmGameWasmProbe
generated deployment set: HTML, JS, Wasm, qtloader, pthread worker,
                          .ww.js, .aw.js
```

Gate 1B then adds the production-header server, Playwright Chromium, QML/QSB
runtime assertions, static-boundary exception execution, QtConcurrent/pthread
execution, JSPI nested event loop, same-origin QNAM, main-thread WSS, served
video, early-return lifetime, real AudioWorklet callback/shared ring,
memory-growth/teardown/version-skew tests, OPFS/File System Access probes, and
the 1,000-cycle adversarial lifecycle run.
