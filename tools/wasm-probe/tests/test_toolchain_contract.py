import hashlib
import json
import os
import re
import subprocess
import tempfile
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
    "8aa3ed93e30f16c3c9691b35dee1f27c9608bcf424fc4b0e86009ce64709c786"
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
CMAKE_EXECUTABLE_SHA256 = (
    "daae341e73330c9c5bf391f22d10510c0f70e5f01d2b61b0fd256cd9edd28379"
)
NINJA_EXECUTABLE_SHA256 = (
    "e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8"
)
HOST_COMPILER_SHA256 = (
    "9cf613fcbebece019712511eec4b1a32d0a9c94e65249a1aeb326305c2388d6b"
)
SOURCE_DATE_EPOCH = 1782488244
REPRODUCIBLE_BUILD_DERIVATION = (
    "vcpkg-baseline-source-archive-root-entry-utc"
)


class ToolchainContractTest(unittest.TestCase):
    def test_lock_file_has_exact_production_pins(self) -> None:
        lock = json.loads((PROBE / "toolchain-lock.json").read_text("utf-8"))
        self.assertEqual(lock["qt"]["version"], "6.11.1")
        self.assertEqual(lock["emscripten"]["version"], "4.0.7")
        self.assertEqual(lock["emscripten"]["emsdkCommit"], EMSDK_COMMIT)
        self.assertEqual(
            lock["reproducibleBuild"],
            {
                "sourceDateEpoch": SOURCE_DATE_EPOCH,
                "derivation": REPRODUCIBLE_BUILD_DERIVATION,
                "vcpkgMaxConcurrency": 8,
            },
        )
        self.assertEqual(
            lock["emscripten"]["releaseHash"],
            "ef4e9cedeac3332e4738087567552063f4f250d3",
        )
        self.assertEqual(
            lock["emscripten"]["releaseManifest"]["path"],
            "emscripten-releases-tags.json",
        )
        self.assertEqual(
            lock["emscripten"]["payload"]["algorithm"],
            "sha256-path-null-digest-lf-v1",
        )
        self.assertEqual(
            lock["emscripten"]["generatedBytecode"],
            {
                "cacheDirectory": "__pycache__",
                "fileSuffix": ".pyc",
                "normalization": (
                    "authenticate-non-bytecode-delete-cache-"
                    "authenticate-full-v1"
                ),
            },
        )
        self.assertEqual(lock["emscripten"]["bootstrapScript"], "emsdk.py")
        self.assertRegex(
            lock["emscripten"]["bootstrapScriptSha256"],
            r"^[0-9a-f]{64}$",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"][
                "installationDirectory"
            ],
            "python/3.9.2-nuget_64bit",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"]["executable"],
            "python.exe",
        )
        self.assertEqual(
            lock["emscripten"]["bootstrapPython"]["payload"]["fileCount"],
            1486,
        )
        self.assertEqual(
            lock["emscripten"]["driverApi"]["pythonImportClosure"],
            {
                "algorithm": "sha256-path-null-digest-lf-v1",
                "fileCount": 225,
                "totalBytes": 3159759,
                "inventorySha256": (
                    "acdbf9111c4779b62764eaa595a184179f7ffe8f46de07ea"
                    "4303f635d1308477"
                ),
                "aggregateSha256": (
                    "4a378899a0a3ad36e19f7f5e0170641fdb647d61aaf464c"
                    "721b07d729abdf6f6"
                ),
            },
        )
        self.assertRegex(
            lock["emscripten"]["nodeExecutableSha256"],
            r"^[0-9a-f]{64}$",
        )
        self.assertEqual(
            lock["emscripten"]["sourceArchive"]["allowedRuntimePrefixes"],
            [
                "downloads",
                "node/20.18.0_64bit",
                "python/3.9.2-nuget_64bit",
                "upstream/bin",
                "upstream/emscripten",
                "upstream/lib",
            ],
        )
        self.assertEqual(
            lock["vcpkg"]["sourceArchive"]["allowedRuntimePrefixes"],
            [],
        )
        self.assertNotIn(".git", json.dumps(lock["emscripten"]["sourceArchive"]))
        self.assertNotIn(".git", json.dumps(lock["vcpkg"]["sourceArchive"]))
        gate_scripts = {
            "adapterSha256": (
                PROBE / "scripts" / "invoke_emscripten_driver.py"
            ),
            "responseAuditorSha256": (
                PROBE / "scripts" / "audit_emscripten_response_files.py"
            ),
        }
        for field, path in gate_scripts.items():
            self.assertEqual(
                lock["gateTools"][field],
                hashlib.sha256(path.read_bytes()).hexdigest(),
            )
        self.assertEqual(
            lock["emscripten"]["payload"]["excludedSegments"],
            [],
        )
        self.assertEqual(
            lock["emscripten"]["payload"]["excludedSuffixes"],
            [],
        )
        self.assertGreater(lock["emscripten"]["payload"]["fileCount"], 10_000)
        for name in ("inventorySha256", "aggregateSha256"):
            self.assertRegex(
                lock["emscripten"]["payload"][name],
                r"^[0-9a-f]{64}$",
            )
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
                "archiveFile": "cmake-4.2.3-windows-x86_64.zip",
                "executableSha256": CMAKE_EXECUTABLE_SHA256,
                "directory": "cmake-4.2.3-windows-x86_64",
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "cmake-4.2.3-windows-x86_64",
                    "fileCount": 8525,
                    "directoryCount": 148,
                    "totalBytes": 132316585,
                    "inventorySha256": (
                        "48f746fb6ca853fd693c018eaa3220da06b4251275a15a3e41a"
                        "93bc1013c3dd3"
                    ),
                    "directoryInventorySha256": (
                        "649f6d95061ef02fe899c0c7062b551e401e372a098c9a9880"
                        "cbbee4d72fc814"
                    ),
                    "aggregateSha256": (
                        "38d0389fbb638f78ec2a624592d331efeef3c3f951eb3a3aff1"
                        "eacbff16146f7"
                    ),
                },
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
                "archiveFile": "ninja-win.zip",
                "executableSha256": NINJA_EXECUTABLE_SHA256,
                "directory": "ninja-1.13.2-win",
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "",
                    "fileCount": 1,
                    "directoryCount": 0,
                    "totalBytes": 603648,
                    "inventorySha256": (
                        "014cb71e5fd86a18adb79f57e26acbd58577f6f84a52491297a"
                        "3454a3b38f477"
                    ),
                    "directoryInventorySha256": (
                        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495"
                        "991b7852b855"
                    ),
                    "aggregateSha256": (
                        "90779f6fe330259113900c836a8dbf973c87905e098e0ada0121"
                        "b5ac135d6dac"
                    ),
                },
            },
        )
        self.assertEqual(
            lock["hostCompiler"],
            {
                "basename": "cl.exe",
                "toolsetRelativePath": (
                    "VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe"
                ),
                "cmakeCompilerId": "MSVC",
                "cmakeCompilerVersion": "19.51.36244.0",
                "frontendVariant": "MSVC",
                "architecture": "x64",
                "platform": "Windows",
                "executableSha256": HOST_COMPILER_SHA256,
            },
        )
        self.assertEqual(
            lock["qt"]["qtdeclarativePatchSha256"],
            QTDECLARATIVE_PATCH_SHA256,
        )

    def test_probe_has_real_adapter_authenticated_c_compile_and_link_edges(
        self,
    ) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        manifest = (PROBE / "input-manifest.txt").read_text("utf-8")
        self.assertIn(
            "project(RhythmGameWasmProbe VERSION 0.1.0 LANGUAGES C CXX)",
            cmake,
        )
        self.assertIn("set(CMAKE_C_COMPILER_LAUNCHER", cmake)
        self.assertIn("set(CMAKE_C_LINKER_LAUNCHER", cmake)
        self.assertIn("add_executable(RhythmGameWasmCLauncherProbe", cmake)
        self.assertIn("src/LauncherProbe.c", cmake)
        self.assertRegex(
            cmake,
            (
                r"set_target_properties\(\s*"
                r"RhythmGameWasmCLauncherProbe\s+PROPERTIES"
                r"(?s:.*?)AUTOMOC OFF"
                r"(?s:.*?)LINKER_LANGUAGE C"
            ),
        )
        self.assertIn("tools/wasm-probe/src/LauncherProbe.c\n", manifest)

    def test_probe_input_checkout_byte_policy_is_bound_and_portable(
        self,
    ) -> None:
        entries = (
            PROBE / "input-manifest.txt"
        ).read_text("utf-8").splitlines()
        attributes = (
            REPO / ".gitattributes"
        ).read_text("utf-8").splitlines()
        self.assertEqual(len(entries), 87)
        self.assertEqual(entries.count(".gitattributes"), 1)
        for rule in (
            "/.gitattributes text eol=lf",
            "cmake/toolchains/vcpkg-emscripten.cmake text eol=lf",
            "tools/wasm-probe/** text eol=lf",
            "vcpkgOverlayPorts/qtdeclarative/* text eol=lf",
            "vcpkgTriplets/*.cmake text eol=lf",
            (
                "vcpkgOverlayPorts/qtdeclarative/"
                "24205cd-qquickwindow-child-window-stacking.patch -text"
            ),
            "vcpkgOverlayPortsWasm/qtbase/** -text -whitespace",
        ):
            self.assertEqual(attributes.count(rule), 1)

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
        qtbase = next(
            dependency
            for dependency in manifest["dependencies"]
            if isinstance(dependency, dict)
            and dependency.get("name") == "qtbase"
        )
        self.assertIn("freetype", qtbase["features"])
        self.assertIn("gles2", qtbase["features"])
        self.assertIn("opengl", qtbase["features"])
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
        self.assertIn("EMSDK_PYTHON", triplet)
        self.assertIn(
            'set(VCPKG_C_FLAGS "-pthread -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertIn(
            "set(VCPKG_CXX_FLAGS "
            '"-pthread -fwasm-exceptions -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertIn(
            "set(VCPKG_LINKER_FLAGS "
            '"-pthread -fwasm-exceptions -sSUPPORT_LONGJMP=wasm")',
            triplet,
        )
        self.assertNotIn(
            "set(VCPKG_LIBRARY_LINKAGE dynamic)",
            triplet,
        )
        self.assertNotIn("if(PORT MATCHES", triplet)

    def test_target_triplet_passes_canonical_emsdk_to_vcpkg_builds(
        self,
    ) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        passthrough = re.search(
            r"set\(VCPKG_ENV_PASSTHROUGH(?P<body>.*?)\)",
            triplet,
            re.DOTALL,
        )
        self.assertIsNotNone(passthrough)
        variables = passthrough.group("body").split()
        self.assertIn("EMSDK", variables)
        self.assertIn("EMSDK_PYTHON", variables)
        self.assertEqual(variables.count("EMSDK"), 1)

    def test_host_triplet_passes_reproducible_environment_to_vcpkg_builds(
        self,
    ) -> None:
        triplet = (
            REPO / "vcpkgTriplets" / "x64-windows-rg-host-release.cmake"
        ).read_text("utf-8")
        passthrough = re.search(
            r"set\(VCPKG_ENV_PASSTHROUGH(?P<body>.*?)\)",
            triplet,
            re.DOTALL,
        )
        self.assertIsNotNone(passthrough)
        variables = passthrough.group("body").split()
        self.assertEqual(
            variables,
            ["PYTHONNOUSERSITE", "SOURCE_DATE_EPOCH"],
        )

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

    def test_wasm_wrapper_forces_ninja_response_files(self) -> None:
        chainload = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        invocation_wrapper = (
            PROBE / "scripts" / "Invoke-WithToolchains.ps1"
        ).read_text("utf-8")
        triplet = (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8")
        self.assertIn(
            "$env:CMAKE_NINJA_FORCE_RESPONSE_FILE = '1'",
            invocation_wrapper,
        )
        self.assertIn(
            "CMAKE_NINJA_FORCE_RESPONSE_FILE",
            triplet,
        )
        self.assertNotIn(
            "set(CMAKE_C_COMPILER",
            chainload,
        )

    def test_wrapper_scrubs_ambient_flags_and_authenticates_before_execution(
        self,
    ) -> None:
        wrapper = (
            PROBE / "scripts" / "Invoke-WithToolchains.ps1"
        ).read_text("utf-8")
        bootstrap = (
            PROBE / "scripts" / "Bootstrap-Toolchains.ps1"
        ).read_text("utf-8")
        provenance = (
            PROBE / "scripts" / "Toolchain-Provenance.ps1"
        ).read_text("utf-8")
        chainload = (
            REPO / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        ).read_text("utf-8")
        for variable in (
            "EMCC_CFLAGS",
            "CFLAGS",
            "CXXFLAGS",
            "CPPFLAGS",
            "LDFLAGS",
            "EM_CONFIG",
            "EM_CACHE",
            "EM_PORTS",
            "EM_COMPILER_WRAPPER",
            "EM_COMPILER_WRAPPER2",
            "BASH_ENV",
            "ENV",
            "QML_",
            "QT_",
            "SOURCE_DATE_EPOCH",
            "QT_RCC_SOURCE_DATE_OVERRIDE",
            "VCPKG_MAX_CONCURRENCY",
            "RHYTHMGAME_",
            "_EMCC_CCACHE",
        ):
            self.assertIn(variable, provenance)
        self.assertIn(
            "$env:SOURCE_DATE_EPOCH = $parsedSourceDateEpoch.ToString(",
            wrapper,
        )
        self.assertIn(
            "$env:VCPKG_MAX_CONCURRENCY = "
            "$parsedVcpkgMaxConcurrency.ToString(",
            wrapper,
        )
        for script in (wrapper, bootstrap):
            self.assertIn("Clear-WasmBuildEnvironment", script)
            self.assertIn("Assert-SourceArchiveInstallation", script)
            self.assertIn("Assert-FileSha256", script)
            self.assertIn("Assert-EmscriptenInstallation", script)
            self.assertIn("Assert-BuildToolInstallation", script)
        self.assertNotIn("-fexceptions", (
            REPO / "vcpkgTriplets" / "wasm32-emscripten-rg.cmake"
        ).read_text("utf-8"))
        self.assertNotIn(
            "set(CMAKE_CXX_COMPILER",
            chainload,
        )

    def test_qtbase_overlay_enables_features_and_keeps_version_check(self) -> None:
        overlay = REPO / "vcpkgOverlayPortsWasm" / "qtbase"
        portfile = (overlay / "portfile.cmake").read_text("utf-8")
        manifest = json.loads((overlay / "vcpkg.json").read_text("utf-8"))
        self.assertEqual(manifest["port-version"], 2)
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
        self.assertIn(
            (
                '"${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}/'
                'QtCore/qconfig.h"'
            ),
            patch,
        )
        self.assertIn(
            '"${WASM_BUILD_DIR}/src/corelib/global/qconfig.h"',
            patch,
        )
        self.assertIn(
            '"${WASM_BUILD_DIR}/include/QtCore/qconfig.h"',
            patch,
        )
        self.assertEqual(
            hashlib.sha256(
                (overlay / "restore-wasm-version-check.patch").read_bytes()
            ).hexdigest(),
            QTBASE_WASM_PATCH_SHA256,
        )

    def test_qtbase_overlay_seeds_wasm_mkspec_before_project(self) -> None:
        portfile = (
            REPO / "vcpkgOverlayPortsWasm" / "qtbase" / "portfile.cmake"
        ).read_text("utf-8")
        option = "-DQT_QMAKE_TARGET_MKSPEC:STRING=wasm-emscripten"
        emscripten_options = re.search(
            (
                r"if\(VCPKG_TARGET_IS_EMSCRIPTEN\)\s*"
                r"list\(APPEND FEATURE_OPTIONS(?P<body>.*?)\)\s*"
                r"endif\(\)"
            ),
            portfile,
            re.DOTALL,
        )
        self.assertIsNotNone(emscripten_options)
        self.assertIn(option, emscripten_options.group("body"))
        self.assertEqual(portfile.count(option), 1)

    def test_installed_qt_rejects_mismatched_emscripten(self) -> None:
        cmake = (
            REPO
            / ".toolchains"
            / "cmake-4.2.3-windows-x86_64"
            / "bin"
            / "cmake.exe"
        )
        qt_prefix = (
            REPO
            / ".wasm-vcpkg"
            / "installed"
            / "wasm32-emscripten-rg"
        )
        qt_cmake = qt_prefix / "share" / "Qt6"
        qt_core_cmake = qt_prefix / "share" / "Qt6Core"
        required = (
            cmake,
            qt_cmake / "Qt6Config.cmake",
            qt_cmake / "QtInstallPaths.cmake",
            qt_cmake / "QtPublicWasmToolchainHelpers.cmake",
            qt_core_cmake / "Qt6CoreConfig.cmake",
            qt_core_cmake / "Qt6CoreConfigExtras.cmake",
        )
        if not all(path.is_file() for path in required):
            self.skipTest("requires the generated wasm-release Qt install")

        qt6_config = required[1].read_text("utf-8")
        install_paths = required[2].read_text("utf-8")
        core_config = required[4].read_text("utf-8")
        core_extras = required[5].read_text("utf-8")
        self.assertIn(
            'set(QT6_INSTALL_HEADERS "include/Qt6")',
            install_paths,
        )
        self.assertLess(
            qt6_config.index("QtInstallPaths.cmake"),
            qt6_config.index("find_package(Qt6${module}"),
        )
        self.assertIn("Qt6CoreConfigExtras.cmake", core_config)
        self.assertIn("Qt6WasmMacros.cmake", core_extras)

        with tempfile.TemporaryDirectory(
            prefix="rhythm-game-wasm-mismatch-"
        ) as temporary:
            temporary_path = Path(temporary)
            fake_sdk = temporary_path / "fake-emsdk"
            fake_emcc = (
                fake_sdk / "upstream" / "emscripten" / "emcc.bat"
            )
            fake_emcc.parent.mkdir(parents=True)
            (fake_sdk / ".emscripten").write_text(
                (
                    "emsdk_path = r'ignored-by-qt-regex'\n"
                    "EMSCRIPTEN_ROOT = "
                    "emsdk_path + '/upstream/emscripten'\n"
                ),
                encoding="utf-8",
            )
            fake_emcc.write_text(
                (
                    "@echo off\n"
                    "echo emcc (Emscripten compiler) 9.9.9\n"
                    "exit /b 0\n"
                ),
                encoding="utf-8",
            )
            check = temporary_path / "check-mismatch.cmake"
            check.write_text(
                (
                    f'include("{required[2].as_posix()}")\n'
                    "if(NOT QT6_INSTALL_HEADERS "
                    'STREQUAL "include/Qt6")\n'
                    '    message(FATAL_ERROR "missing install headers")\n'
                    "endif()\n"
                    f'include("{required[3].as_posix()}")\n'
                    "_qt_test_emscripten_version()\n"
                    'message(FATAL_ERROR "MISMATCH_SENTINEL")\n'
                ),
                encoding="utf-8",
            )
            environment = os.environ.copy()
            environment["EMSDK"] = str(fake_sdk)
            result = subprocess.run(
                [str(cmake), "-P", str(check)],
                cwd=REPO,
                env=environment,
                capture_output=True,
                text=True,
                timeout=30,
                check=False,
            )

        output = result.stdout + result.stderr
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("MISMATCH_SENTINEL", output)
        self.assertIn(
            "Qt Wasm was built with Emscripten version: 4.0.7",
            output,
        )
        self.assertIn(
            "You are using Emscripten version: 9.9.9",
            output,
        )
        self.assertIn(
            "Stopping configuration due to mismatch",
            output,
        )

    def test_qtdeclarative_disables_long_path_styles_only_for_host_tools(
        self,
    ) -> None:
        portfile = (
            REPO / "vcpkgOverlayPorts" / "qtdeclarative" / "portfile.cmake"
        ).read_text("utf-8")
        fluent_option = "-DFEATURE_quickcontrols2_fluentwinui3:BOOL=OFF"
        universal_option = "-DFEATURE_quickcontrols2_universal:BOOL=OFF"
        self.assertEqual(portfile.count(fluent_option), 1)
        self.assertEqual(portfile.count(universal_option), 1)
        self.assertNotIn("CMAKE_OBJECT_PATH_MAX", portfile)
        self.assertRegex(
            portfile,
            (
                r"(?s)if\(VCPKG_TARGET_IS_WINDOWS AND\s+"
                r'TARGET_TRIPLET STREQUAL "'
                r'x64-windows-rg-host-release"\).*?'
                + fluent_option
                + r".*?"
                + universal_option
                + r".*?endif\(\)"
            ),
        )
        self.assertNotIn("VCPKG_TARGET_TRIPLET STREQUAL", portfile)

    def test_wasm_autogen_uses_batch_safe_response_threshold(self) -> None:
        portfile = (
            REPO / "vcpkgOverlayPorts" / "qtdeclarative" / "portfile.cmake"
        ).read_text("utf-8")
        option = (
            "-DCMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX:STRING=4096"
        )
        self.assertIn(option, portfile)
        self.assertRegex(
            portfile,
            (
                r"(?s)if\(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "
                r'"Emscripten"\).*?'
                + option
                + r".*?endif\(\)"
            ),
        )
        presets = json.loads((PROBE / "CMakePresets.json").read_text("utf-8"))
        cache = presets["configurePresets"][0]["cacheVariables"]
        self.assertEqual(
            cache["CMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX"],
            "4096",
        )
        self.assertNotIn("CMAKE_AUTOMOC_COMPILER_PREDEFINES", portfile)
        self.assertNotIn(
            "CMAKE_AUTOMOC_COMPILER_PREDEFINES",
            json.dumps(presets),
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
