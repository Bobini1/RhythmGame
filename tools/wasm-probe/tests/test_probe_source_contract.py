import json
import unittest
from pathlib import Path


PROBE = Path(__file__).resolve().parents[1]


class ProbeSourceContractTest(unittest.TestCase):
    def test_preset_uses_only_pinned_vcpkg_target_and_host(self) -> None:
        presets = json.loads((PROBE / "CMakePresets.json").read_text("utf-8"))
        cache = presets["configurePresets"][0]["cacheVariables"]
        encoded = json.dumps(presets, sort_keys=True)
        self.assertIn("wasm32-emscripten-rg", encoded)
        self.assertIn("x64-windows-rg-host-release", encoded)
        self.assertIn("vcpkgOverlayPortsWasm", encoded)
        self.assertEqual(
            cache["VCPKG_CHAINLOAD_TOOLCHAIN_FILE"],
            "${sourceDir}/../../cmake/toolchains/vcpkg-emscripten.cmake",
        )
        self.assertNotIn("Qt6_DIR", encoded)
        self.assertNotIn("CMAKE_FIND_ROOT_PATH_MODE_PACKAGE\": \"BOTH", encoded)

    def test_preset_shortens_only_the_vcpkg_buildtrees_root(self) -> None:
        presets = json.loads((PROBE / "CMakePresets.json").read_text("utf-8"))
        options = presets["configurePresets"][0]["cacheVariables"][
            "VCPKG_INSTALL_OPTIONS"
        ].split(";")
        self.assertIn(
            "--x-buildtrees-root=${sourceDir}/../../.wb",
            options,
        )
        self.assertIn(
            "--x-packages-root=${sourceDir}/../../.wasm-vcpkg/packages",
            options,
        )
        self.assertIn(
            "--downloads-root=${sourceDir}/../../.wasm-vcpkg/downloads",
            options,
        )
        self.assertNotIn(
            "--x-buildtrees-root=${sourceDir}/../../.wasm-vcpkg/buildtrees",
            options,
        )
        self.assertIn(".wb/", (PROBE.parents[1] / ".gitignore").read_text("utf-8"))

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
