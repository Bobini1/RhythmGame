from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PLAYTEST = REPO / "tools" / "web-playtest"


def _is_manifest_owned_web_source(path: Path) -> bool:
    relative = path.relative_to(PLAYTEST)
    return (
        path.is_file()
        and relative.parts[0] not in {"build", "tests"}
        and "__pycache__" not in relative.parts
        and path.name != "input-manifest.txt"
    )


class WebPlaytestSourceContractTest(unittest.TestCase):
    def test_required_project_files_exist(self) -> None:
        required = (
            "CMakeLists.txt",
            "CMakePresets.json",
            "vcpkg.json",
            "input-manifest.txt",
            "cmake/GenerateWebPlaytestChart.cmake",
            "cmake/WebPlaytestChartManifest.json.in",
            "cmake/WebPlaytestInputDigest.cpp.in",
            "cmake/verify_exact_toolchain.cmake",
            "browser/manual-launch.mjs",
            "browser/playtest-driver.mjs",
            "browser/test-playtest.mjs",
            "browser/web/bootstrap.mjs",
            "browser/web/playtest.css",
            "browser/web/preflight-worker.mjs",
            "qml/Main.qml",
            "scripts/generate_chart_package.py",
            "scripts/scan_artifacts_for_host_path.py",
            "src/WebPlaytestChartInstaller.cpp",
            "src/WebPlaytestChartInstaller.h",
            "src/WebPlaytestInputDigest.h",
            "src/main.cpp",
            "tests/fixtures/canonical-input.json",
        )
        for relative in required:
            with self.subTest(relative=relative):
                self.assertTrue((PLAYTEST / relative).is_file())

    def test_cmake_owns_the_isolated_portable_target_contract(self) -> None:
        cmake = (PLAYTEST / "CMakeLists.txt").read_text("utf-8")
        for marker in (
            "project(RhythmGameWebPlaytest VERSION 1.3.13",
            "RG_WEB_PLAYTEST_CHART_DIR",
            "RG_WEB_PLAYTEST_CHART_RELATIVE_PATH",
            "RG_WEB_PLAYTEST_CHART_DIR_CACHE_TYPE",
            'STREQUAL "PATH"',
            "-DRG_WEB_PLAYTEST_CHART_DIR:PATH=T:/absolute/chart/root",
            "CACHE PATH",
            'string(REPLACE "\\r\\n" "\\n"',
            "RHYTHMGAME_REQUIRE_WEB_GAMEPLAY_DEPENDENCY_CONTRACT ON",
            "RHYTHMGAME_REQUIRE_WEB_AUDIO_DEPENDENCY_CONTRACT ON",
            "include(${RG_WEB_PLAYTEST_REPO_ROOT}/cmake/WebGameplayCore.cmake)",
            "include(${RG_WEB_PLAYTEST_REPO_ROOT}/cmake/WebAudioCore.cmake)",
            "rhythmgame_add_web_gameplay_core(RhythmGame_web_gameplay_core)",
            "rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)",
            "qt_add_executable(RhythmGameWasmProbe",
            "MANUAL_FINALIZATION",
            "NO_WASM_DEFAULT_FILES TRUE",
            "QT_WASM_INITIAL_MEMORY 268435456",
            "QT_WASM_MAXIMUM_MEMORY 1073741824",
            "BIG_RESOURCES",
            'PREFIX "/web-playtest/chart"',
            "OPTIONS -no-compress",
            "PRE_LINK",
            "qrc_web_playtest_chart_tmp.cpp",
            "qrc_web_playtest_chart_manifest.cpp",
            "--generated-input",
            "qt_finalize_executable(RhythmGameWasmProbe)",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, cmake)
        self.assertNotIn("RhythmGame_lib", cmake)
        self.assertIn("RhythmGameWasmProbeRuntimePackage", cmake)
        self.assertIn("RhythmGameWasmProbeRuntimeVerify", cmake)
        self.assertIn(
            "tools/wasm-probe/scripts/package_runtime_artifacts.py",
            cmake,
        )
        self.assertIn(
            "--expected-audio-worklet-occurrences 1",
            re.sub(r"\s+", " ", cmake),
        )
        self.assertIn(
            "--expected-wasm-worker-occurrences 0",
            re.sub(r"\s+", " ", cmake),
        )
        self.assertNotRegex(cmake, r"\b(preload|embed)-file\b")
        self.assertEqual(
            cmake.count(
                "rhythmgame_add_web_gameplay_core("
                "RhythmGame_web_gameplay_core)"
            ),
            1,
        )
        self.assertEqual(
            cmake.count(
                "rhythmgame_add_web_audio_core(RhythmGame_web_audio_core)"
            ),
            1,
        )
        self.assertEqual(cmake.count("qt_add_executable(RhythmGameWasmProbe"), 1)
        self.assertIn(
            'BASE "${RG_WEB_PLAYTEST_CHART_STAGING_DIR}"', cmake
        )
        self.assertNotIn(
            'BASE "${RG_WEB_PLAYTEST_CHART_DIR}"', cmake
        )
        for package in (
            "Qt6 6.11.1 EXACT REQUIRED COMPONENTS",
            "Boost REQUIRED COMPONENTS headers",
            "fmt CONFIG REQUIRED",
            "lexy CONFIG REQUIRED",
            "magic_enum CONFIG REQUIRED",
            "spdlog CONFIG REQUIRED",
            "zstd CONFIG REQUIRED",
            "Iconv REQUIRED",
            "STB_INCLUDE_DIRS NAMES stb_vorbis.c REQUIRED",
        ):
            with self.subTest(package=package):
                self.assertIn(f"find_package({package}", cmake) if (
                    package != "STB_INCLUDE_DIRS NAMES stb_vorbis.c REQUIRED"
                ) else self.assertIn(f"find_path({package}", cmake)

    def test_wasm_flags_are_exactly_present_and_asyncify_is_absent(self) -> None:
        cmake = (PLAYTEST / "CMakeLists.txt").read_text("utf-8")
        required = (
            "-pthread",
            "-fwasm-exceptions",
            "-sSUPPORT_LONGJMP=wasm",
            "-sJSPI",
            "-sAUDIO_WORKLET=1",
            "-sWASM_WORKERS=1",
            "-sPTHREAD_POOL_SIZE=4",
            "-sPTHREAD_POOL_SIZE_STRICT=2",
            "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
            "-sDYNAMIC_EXECUTION=0",
            "-sEMBIND_AOT=1",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sMEMORY_GROWTH_GEOMETRIC_STEP=0",
        )
        for flag in required:
            with self.subTest(flag=flag):
                self.assertIn(flag, cmake)
        self.assertNotIn("ASYNCIFY", cmake)
        generated_leaves = re.findall(
            r'set\(RG_WEB_PLAYTEST_GENERATED_[A-Z_]+\s+'
            r'"\$\{CMAKE_CURRENT_BINARY_DIR\}/'
            r'RhythmGameWasmProbe\.([^"]+)"\)',
            cmake,
        )
        self.assertCountEqual(
            generated_leaves,
            ["aw.js", "js", "wasm", "ww.js"],
        )
        self.assertIn(
            "chartManifest=${RG_WEB_PLAYTEST_CHART_MANIFEST_SHA256}",
            cmake,
        )
        main = (PLAYTEST / "src" / "main.cpp").read_text("utf-8")
        self.assertIn("web_playtest::buildInputSha256()", main)
        self.assertIn("rgWebPlaytestBuildInputSha256", main)
        self.assertIn("WebPlaytestRuntime::createProcessLifetime(", main)
        self.assertIn("setContextProperty(", main)
        self.assertNotIn("application.exit(EXIT_FAILURE)", main)

    def test_manifest_and_installer_contract_is_authoritative_and_streaming(
        self,
    ) -> None:
        installer = (
            PLAYTEST / "src" / "WebPlaytestChartInstaller.cpp"
        ).read_text("utf-8")
        for marker in (
            '":/web-playtest/web-playtest-chart-manifest.json"',
            '":/web-playtest/chart"',
            '"/playtest/chart"',
            "QDirIterator",
            "QCryptographicHash::Sha256",
            "manifestFiles",
            "resourceFiles",
            "output.remove()",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, installer)
        self.assertNotIn("readAll(", installer)

    def test_manifest_is_sorted_unique_and_covers_portable_sources(self) -> None:
        lines = (PLAYTEST / "input-manifest.txt").read_text(
            "utf-8"
        ).splitlines()
        self.assertEqual(lines, sorted(lines, key=str.casefold))
        self.assertEqual(len(lines), len({line.casefold() for line in lines}))
        for line in lines:
            self.assertFalse(Path(line).is_absolute(), line)
            self.assertNotIn("\\", line)
            self.assertNotIn("Dstorv", line)
            self.assertNotIn("BMSTEST", line)

        portable_sources: set[str] = set()
        for module in (
            REPO / "cmake" / "WebGameplayCore.cmake",
            REPO / "cmake" / "WebAudioCore.cmake",
        ):
            portable_sources.update(
                re.findall(r"^\s+(src/[^\s)]+)$", module.read_text("utf-8"), re.M)
            )
        self.assertTrue(portable_sources)
        self.assertTrue(portable_sources.issubset(set(lines)))
        manifest_paths = set(lines)
        web_sources = {
            path.relative_to(REPO).as_posix()
            for path in PLAYTEST.rglob("*")
            if _is_manifest_owned_web_source(path)
        }
        self.assertTrue(web_sources.issubset(manifest_paths))
        self.assertTrue(
            {
                "tools/wasm-probe/scripts/Invoke-WithToolchains.ps1",
                "tools/wasm-probe/scripts/audit_emscripten_response_files.py",
                "tools/wasm-probe/scripts/invoke_emscripten_driver.py",
                "tools/wasm-probe/toolchain-lock.json",
            }.issubset(manifest_paths)
        )

    def test_generated_build_tree_is_not_a_digest_input(self) -> None:
        self.assertFalse(
            _is_manifest_owned_web_source(
                PLAYTEST / "build" / "wasm-release" / "CMakeCache.txt"
            )
        )
        self.assertTrue(
            _is_manifest_owned_web_source(
                PLAYTEST / "scripts" / "generate_chart_package.py"
            )
        )

    def test_web_playtest_sources_are_checkout_stable_lf_text(self) -> None:
        attributes = (REPO / ".gitattributes").read_text("utf-8")
        self.assertIn("tools/web-playtest/** text eol=lf\n", attributes)

    def test_vcpkg_manifest_is_the_exact_direct_allowlist(self) -> None:
        manifest = json.loads((PLAYTEST / "vcpkg.json").read_text("utf-8"))
        names = {
            item if isinstance(item, str) else item["name"]
            for item in manifest["dependencies"]
        }
        self.assertEqual(
            names,
            {
                "boost-headers",
                "boost-icl",
                "fmt",
                "foonathan-lexy",
                "libiconv",
                "magic-enum",
                "qtbase",
                "qtdeclarative",
                "qtshadertools",
                "spdlog",
                "stb",
                "zstd",
            },
        )
        qtbase = next(
            item
            for item in manifest["dependencies"]
            if isinstance(item, dict) and item["name"] == "qtbase"
        )
        probe_manifest = json.loads(
            (REPO / "tools" / "wasm-probe" / "vcpkg.json").read_text(
                "utf-8"
            )
        )
        probe_qtbase = next(
            item
            for item in probe_manifest["dependencies"]
            if isinstance(item, dict) and item["name"] == "qtbase"
        )
        self.assertEqual(qtbase["default-features"], False)
        self.assertEqual(qtbase["features"], probe_qtbase["features"])

    def test_vcpkg_uses_the_authenticated_source_tree_without_git_lookup(
        self,
    ) -> None:
        manifest = json.loads((PLAYTEST / "vcpkg.json").read_text("utf-8"))
        self.assertNotIn("builtin-baseline", manifest)

        presets = json.loads(
            (PLAYTEST / "CMakePresets.json").read_text("utf-8")
        )
        cache = presets["configurePresets"][0]["cacheVariables"]
        self.assertEqual(cache["VCPKG_FEATURE_FLAGS"], "-versions")
        self.assertEqual(
            cache["CMAKE_TOOLCHAIN_FILE"],
            "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        )
        self.assertNotIn("VCPKG_REGISTRIES_CACHE", cache)

    def test_preset_uses_independent_state_and_shared_binary_cache(self) -> None:
        text = (PLAYTEST / "CMakePresets.json").read_text("utf-8")
        self.assertIn(".web-playtest-vcpkg/installed", text)
        self.assertIn(".web-playtest-vcpkg/packages", text)
        self.assertIn(".web-playtest-vcpkg/downloads", text)
        self.assertIn(".web-playtest-vcpkg/buildtrees", text)
        self.assertIn("$env{VCPKG_DEFAULT_BINARY_CACHE},readwrite", text)
        self.assertNotIn(".wasm-vcpkg/installed", text)

    def test_exact_toolchain_validation_is_copied_byte_for_byte(self) -> None:
        self.assertEqual(
            (PLAYTEST / "cmake" / "verify_exact_toolchain.cmake").read_bytes(),
            (
                REPO
                / "tools"
                / "wasm-probe"
                / "cmake"
                / "verify_exact_toolchain.cmake"
            ).read_bytes(),
        )

    def test_playable_qml_shell_is_qt6_safe(self) -> None:
        qml = (PLAYTEST / "qml" / "Main.qml").read_text("utf-8")
        self.assertIn("import QtQuick\n", qml)
        self.assertIn("import QtQuick.Controls.Basic\n", qml)
        self.assertNotRegex(qml, r"^import .+ \d", re.M)
        self.assertNotIn("Canvas", qml)
        for literal in (
            "RhythmGame — Dstorv web playtest",
            "NORMAL gauge",
            "Native: A S D Space J K L",
            "LR2: Z S X D C F V",
        ):
            with self.subTest(literal=literal):
                self.assertIn(f'qsTr("{literal}")', qml)

    def test_strict_browser_bootstrap_audits_and_maps_runtime_roles(
        self,
    ) -> None:
        bootstrap = (
            PLAYTEST / "browser" / "web" / "bootstrap.mjs"
        ).read_text("utf-8")
        for marker in (
            'fetch("runtime-artifacts.json"',
            "crossOriginIsolated",
            "isSecureContext",
            "crypto.subtle.digest",
            "new Worker(",
            "new CSSStyleSheet()",
            "adoptedStyleSheets",
            "await window.qtLoad({",
            "locateFile",
            "audited.wasm.url",
            "audited.audioWorklet.url",
            "audited.wasmWorker.url",
            "window.RhythmGameWasmProbe_entry",
            "__rhythmGameWebPlaytestTestBridge",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)
        for forbidden in (
            "autoplay",
            "unsafe-inline",
            "--disable-web-security",
            "--allow-file-access-from-files",
            "ignoreHTTPSErrors",
            "setIgnoreCertificateErrors",
            "document.domain",
            "Access-Control-Allow-Origin",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, bootstrap)

    def test_browser_launchers_reuse_strict_probe_lifecycle(self) -> None:
        manual = (
            PLAYTEST / "browser" / "manual-launch.mjs"
        ).read_text("utf-8")
        automated = (
            PLAYTEST / "browser" / "test-playtest.mjs"
        ).read_text("utf-8")
        driver = (
            PLAYTEST / "browser" / "playtest-driver.mjs"
        ).read_text("utf-8")
        for source in (manual, automated):
            self.assertIn("startProbeServer", source)
            self.assertIn("launchExternalLifecycleBrowser", source)
        self.assertIn("chrome-stable", manual)
        self.assertIn("SIGINT", manual)
        self.assertIn("SIGTERM", manual)
        for marker in ("console", "pageerror", "crash", "requestfailed"):
            self.assertIn(marker, manual)
        for marker in (
            "--input-sequence",
            "--trace-output",
            "traceBytes",
            "writeFile",
            "completeForTrace: true",
        ):
            self.assertIn(marker, driver)
        for marker in (
            "crossOriginIsolated",
            "page.keyboard.down",
            "page.keyboard.up",
            "setViewportSize",
            "completeForTrace",
            "uncaughtExceptions",
            "qmlErrors",
            "droppedInputs",
            "audioWorkletFailures",
            "browserCrashes",
            "consoleWarnings",
            "failedRequests",
            "httpFailures",
            "missing-coop",
            "missing-coep",
            "path traversal",
            "unhashed",
            "missing manifest role",
            "non-HTTPS",
        ):
            self.assertIn(marker, automated)
        for forbidden in (
            "--disable-web-security",
            "--allow-file-access-from-files",
            "ignoreHTTPSErrors",
        ):
            self.assertNotIn(forbidden, manual + automated + driver)

    def test_canonical_input_fixture_is_bounded_and_code_based(self) -> None:
        fixture = json.loads(
            (
                PLAYTEST
                / "tests"
                / "fixtures"
                / "canonical-input.json"
            ).read_text("utf-8")
        )
        self.assertEqual(set(fixture), {"events", "preset", "schemaVersion"})
        self.assertEqual(fixture["schemaVersion"], 1)
        self.assertIn(fixture["preset"], {"native", "lr2"})
        self.assertGreater(len(fixture["events"]), 0)
        self.assertLessEqual(len(fixture["events"]), 256)
        previous = -1
        pressed: set[str] = set()
        for event in fixture["events"]:
            self.assertEqual(
                set(event),
                {"action", "atMilliseconds", "code"},
            )
            self.assertRegex(event["code"], r"^(?:Key[A-Z]|Space|ShiftLeft|ControlLeft)$")
            self.assertIn(event["action"], {"press", "release"})
            self.assertIsInstance(event["atMilliseconds"], int)
            self.assertGreaterEqual(event["atMilliseconds"], previous)
            previous = event["atMilliseconds"]
            if event["action"] == "press":
                self.assertNotIn(event["code"], pressed)
                pressed.add(event["code"])
            else:
                self.assertIn(event["code"], pressed)
                pressed.remove(event["code"])
        self.assertEqual(pressed, set())


if __name__ == "__main__":
    unittest.main()
