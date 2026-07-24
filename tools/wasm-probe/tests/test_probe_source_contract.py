import json
import re
import unittest
from pathlib import Path


PROBE = Path(__file__).resolve().parents[1]
WASM_COMPILE_SETTINGS = (
    "-pthread",
    "-fwasm-exceptions",
    "-sSUPPORT_LONGJMP=wasm",
)


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

    def test_exception_boundary_publishes_wasm_compile_contract(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        compile_contract = re.search(
            (
                r"target_compile_options\(\s*"
                r"WasmProbeWasmCompileOptions\s+INTERFACE"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(compile_contract)
        for setting in WASM_COMPILE_SETTINGS:
            self.assertIn(setting, compile_contract.group("body"))
        self.assertRegex(
            cmake,
            (
                r"target_link_libraries\(\s*"
                r"WasmProbeExceptionBoundary\s+PUBLIC\s+"
                r"WasmProbeWasmCompileOptions\s*\)"
            ),
        )
        self.assertEqual(cmake.count("target_link_options("), 1)
        self.assertRegex(
            cmake,
            r"target_link_options\(\s*RhythmGameWasmProbe\s+PRIVATE",
        )

    def test_generated_boundary_and_consumer_share_wasm_compile_contract(
        self,
    ) -> None:
        compile_database = (
            PROBE / "build" / "wasm-release" / "compile_commands.json"
        )
        if not compile_database.exists():
            self.skipTest("requires a generated wasm-release build")
        entries = json.loads(compile_database.read_text("utf-8"))
        for source_name in ("ExceptionBoundary.cpp", "ProbeState.cpp"):
            matching = [
                entry
                for entry in entries
                if entry["file"].replace("\\", "/").endswith(
                    f"/{source_name}"
                )
            ]
            self.assertEqual(len(matching), 1)
            command = matching[0].get("command") or " ".join(
                matching[0]["arguments"]
            )
            for setting in WASM_COMPILE_SETTINGS:
                self.assertIn(setting, command)

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
