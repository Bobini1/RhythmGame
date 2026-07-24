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
        self.assertNotIn(
            "set(CMAKE_CXX_COMPILER",
            chainload,
        )

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
