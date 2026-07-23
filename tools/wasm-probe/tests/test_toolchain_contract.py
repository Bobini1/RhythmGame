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


class ToolchainContractTest(unittest.TestCase):
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
        self.assertEqual(lock["buildTools"]["cmake"], "4.2.3")
        self.assertEqual(lock["buildTools"]["ninja"], "1.13.2")
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
