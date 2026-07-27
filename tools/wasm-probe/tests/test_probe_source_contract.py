import json
import re
import unittest
from pathlib import Path


PROBE = Path(__file__).resolve().parents[1]
REPO = PROBE.parents[1]
DESIGN = (
    REPO
    / "docs"
    / "superpowers"
    / "specs"
    / "2026-07-23-emscripten-web-port-design.md"
)
WASM_COMPILE_SETTINGS = (
    "-pthread",
    "-fwasm-exceptions",
    "-sSUPPORT_LONGJMP=wasm",
)
TASK3_SOURCE_INPUTS = (
    "tools/wasm-probe/browser/tests/gate1b.spec.mjs",
    "tools/wasm-probe/src/BrowserRuntimeBridge.cpp",
    "tools/wasm-probe/src/BrowserRuntimeBridge.h",
    "tools/wasm-probe/src/Gate1bReport.cpp",
    "tools/wasm-probe/src/Gate1bReport.h",
    "tools/wasm-probe/src/JspiNestedLoopProbe.cpp",
    "tools/wasm-probe/src/JspiNestedLoopProbe.h",
    "tools/wasm-probe/src/RenderProbe.cpp",
    "tools/wasm-probe/src/RenderProbe.h",
)


class ProbeSourceContractTest(unittest.TestCase):
    def _read_required(self, relative: str) -> str:
        path = PROBE / relative
        self.assertTrue(
            path.is_file(),
            f"Task 3 source behavior is missing: {path}",
        )
        return path.read_text("utf-8")

    def test_webmidi_is_a_mandatory_parity_and_gate_contract(self) -> None:
        design = DESIGN.read_text("utf-8")
        required_markers = (
            "| WebMIDI controllers |",
            "WebMIDI permission denied and granted",
            "WebMIDI hotplug and unplug",
            "WebMIDI timestamp-domain calibration",
            "WebMIDI duplicate-source arbitration",
            "explicitly approved WebMIDI exception",
            "blocks a full-parity claim",
            "keyboard, WebHID, WebMIDI, Gamepad",
        )
        for marker in required_markers:
            with self.subTest(marker=marker):
                self.assertIn(marker, design)

    def test_probe_build_binds_the_tracked_input_manifest(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        manifest = PROBE / "input-manifest.txt"
        template = PROBE / "cmake" / "ProbeInputDigest.cpp.in"
        self.assertTrue(manifest.is_file())
        self.assertTrue(template.is_file())
        entries = manifest.read_text("utf-8").splitlines()
        self.assertEqual(entries, sorted(set(entries), key=str.casefold))
        self.assertIn(
            "tools/wasm-probe/cmake/ProbeInputDigest.cpp.in",
            entries,
        )
        self.assertIn("RG_WASM_PROBE_INPUT_SHA256", template.read_text("utf-8"))
        self.assertIn("input-manifest.txt", cmake)

    def test_dependency_digest_uses_cmake_portable_sha256_validation(
        self,
    ) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        self.assertIn(
            'string(LENGTH "${WASM_PROBE_DEPENDENCY_DIGEST}"',
            cmake,
        )
        self.assertIn(
            "NOT WASM_PROBE_DEPENDENCY_DIGEST_LENGTH EQUAL 64",
            cmake,
        )
        self.assertNotIn('MATCHES "^[0-9a-f]{64}$"', cmake)
        self.assertIn("CMAKE_CONFIGURE_DEPENDS", cmake)
        self.assertIn("ProbeInputDigest.cpp", cmake)

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

    def test_application_link_disables_runtime_code_generation(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        link_options = re.search(
            (
                r"target_link_options\(\s*"
                r"RhythmGameWasmProbe\s+PRIVATE"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(link_options)
        body = link_options.group("body")
        self.assertEqual(body.count("SHELL:-sDYNAMIC_EXECUTION=0"), 1)
        self.assertEqual(body.count("SHELL:-sEMBIND_AOT=1"), 1)

    def test_gate1b_runtime_packaging_is_ordered_and_source_bound(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        for marker in (
            "NO_WASM_DEFAULT_FILES TRUE",
            "package_runtime_artifacts.py",
            "RhythmGameWasmProbeRuntimePackage",
            "POST_BUILD",
            "BYPRODUCTS",
            "runtime-artifacts.json",
            "RhythmGameWasmProbe.html",
            "RhythmGameWasmProbe.aw.js",
            "RhythmGameWasmProbe.ww.js",
            "plugins/platforms/qtloader.js",
            "browser/web/RhythmGameWasmProbe.html.in",
            "browser/web/probe.css",
            "browser/web/bootstrap.mjs",
            "browser/web/preflight-worker.mjs",
            "browser/fixtures/probe.webm",
            "WASM_PROBE_INPUT_DIGEST",
        ):
            self.assertIn(marker, cmake)
        self.assertRegex(
            cmake,
            (
                r"add_dependencies\(\s*"
                r"RhythmGameWasmProbeRuntimePackage\s+"
                r"RhythmGameWasmProbe"
            ),
        )

    def test_input_digest_invalidates_every_local_compile_sidecar(self) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        compile_identity = re.search(
            (
                r"target_compile_definitions\(\s*"
                r"WasmProbeWasmCompileOptions\s+INTERFACE"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(
            compile_identity,
            "the input closure must participate in every local compile command",
        )
        self.assertIn(
            "RG_WASM_PROBE_COMPILE_INPUT_SHA256",
            compile_identity.group("body"),
        )
        self.assertIn(
            "${WASM_PROBE_INPUT_DIGEST}",
            compile_identity.group("body"),
        )
        self.assertRegex(
            cmake,
            (
                r"target_link_libraries\(\s*"
                r"RhythmGameWasmCLauncherProbe\s+PRIVATE\s+"
                r"WasmProbeWasmCompileOptions"
            ),
        )
        self.assertRegex(
            cmake,
            (
                r"target_link_libraries\(\s*"
                r"WasmProbeExceptionBoundary\s+PUBLIC\s+"
                r"WasmProbeWasmCompileOptions"
            ),
        )

    def test_runtime_post_build_controller_is_qualification_locked(self) -> None:
        controls = (
            PROBE / "build-control-manifest.txt"
        ).read_text("utf-8").splitlines()
        self.assertIn(
            "CMakeFiles/RhythmGameWasmProbe.dir/post-build.bat",
            controls,
        )

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
        self.assertEqual(cmake.count("target_link_options("), 2)
        self.assertRegex(
            cmake,
            (
                r"target_link_options\(\s*"
                r"RhythmGameWasmCLauncherProbe\s+PRIVATE"
            ),
        )
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
        self.assertIn("struct ProcessLifetimeArguments", main)
        self.assertIn(
            "static ProcessLifetimeArguments processArguments",
            main,
        )
        self.assertIn("processArguments.argc", main)
        self.assertIn("processArguments.pointers.data()", main)
        argument_owner = re.search(
            (
                r"struct\s+ProcessLifetimeArguments\s*\{"
                r"(?P<body>.*?)"
                r"\n\};"
            ),
            main,
            re.DOTALL,
        )
        self.assertIsNotNone(argument_owner)
        owner_body = argument_owner.group("body")
        for marker in (
            "std::vector<QByteArray> storage;",
            "std::vector<char *> pointers;",
            "storage.reserve(",
            "storage.emplace_back(values[index] != nullptr ? values[index] : \"\")",
            "pointers.push_back(value.data())",
            "pointers.push_back(nullptr)",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, owner_body)
        storage_fill = owner_body.index("storage.emplace_back(")
        pointer_fill = owner_body.index("pointers.push_back(value.data())")
        null_sentinel = owner_body.index("pointers.push_back(nullptr)")
        self.assertLess(storage_fill, pointer_fill)
        self.assertLess(pointer_fill, null_sentinel)
        self.assertNotIn("pointers.push_back(values[index])", owner_body)
        self.assertNotIn("pointers.emplace_back(values[index])", owner_body)
        self.assertNotRegex(
            owner_body[pointer_fill:],
            r"storage\.(?:emplace_back|push_back|insert|erase|clear|resize)\s*\(",
        )
        self.assertRegex(
            main,
            (
                r"new\s+QGuiApplication\s*\{\s*"
                r"processArguments\.argc\s*,\s*"
                r"processArguments\.pointers\.data\(\)\s*\}"
            ),
        )
        self.assertNotRegex(
            main,
            r"new\s+QGuiApplication\s*\{\s*argc\s*,\s*argv\s*\}",
        )

    def test_task3_report_and_capability_shapes_are_frozen(self) -> None:
        report_header = self._read_required("src/Gate1bReport.h")
        report_source = self._read_required("src/Gate1bReport.cpp")
        bridge_header = self._read_required("src/BrowserRuntimeBridge.h")
        combined_header = report_header + bridge_header

        for marker in (
            "struct BrowserCapabilities",
            "bool secureContext;",
            "bool crossOriginIsolated;",
            "bool sharedArrayBuffer;",
            "bool jspiApi;",
            "bool webGl2Api;",
            "bool audioWorklet;",
            "bool opfs;",
            "bool fileSystemAccess;",
            "class Gate1bReport final : public QObject",
            "Q_OBJECT",
            "void append(QStringView type, QJsonObject payload = {});",
            "void pass(QStringView check, QJsonObject detail = {});",
            "void fail(QStringView code, QStringView detail);",
            "[[nodiscard]] QJsonObject snapshot() const;",
            "[[nodiscard]] BrowserCapabilities browserCapabilities();",
            "[[nodiscard]] bool publishGate1bEvent(",
            "[[nodiscard]] bool publishGate1bSnapshot(",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined_header)

        for marker in (
            "QElapsedTimer",
            "QJsonDocument::Compact",
            "publishGate1bEvent",
            "publishGate1bSnapshot",
            "sequence",
            "monotonicMicroseconds",
            "type",
            "payload",
            "phase",
            "checks",
            "capabilities",
            "failures",
            "cycleSummary",
            "authority",
            "not-started",
            "completed",
            "lastMonotonicMicroseconds",
            "lastMonotonicMicroseconds + 1",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, report_header + report_source)
        self.assertRegex(
            report_source,
            r"(?:\(std::max\)|qMax)\s*\(",
        )
        self.assertRegex(
            report_header + report_source,
            r"(?:nextSequence|m_nextSequence)\s*(?:=|\{)\s*0",
        )
        for field in (
            "gate1bTechnicalPassed",
            "gate0Satisfied",
            "formalGate1EntryAuthorized",
            "gate1Passed",
            "productionPortAuthorized",
        ):
            with self.subTest(authority=field):
                self.assertRegex(
                    report_source,
                    (
                        rf'["\']{field}["\']'
                        rf'[^\n]{{0,80}}\bfalse\b'
                    ),
                )
                self.assertNotRegex(
                    report_source,
                    (
                        rf'["\']{field}["\']'
                        rf'[^\n]{{0,80}}\btrue\b'
                    ),
                )
        for forbidden in (
            "QProcessEnvironment",
            "qEnvironmentVariable",
            "certificatePrivateKey",
            "environmentDump",
            "profilePath",
            "selectedDirectoryPath",
        ):
            self.assertNotIn(forbidden, report_header + report_source)

    def test_task3_bridge_is_the_only_fixed_json_js_boundary(self) -> None:
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        source_occurrences = {}
        for path in sorted((PROBE / "src").glob("**/*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            text = path.read_text("utf-8")
            count = len(re.findall(r"\bEM_(?:ASYNC_)?JS\s*\(", text))
            if count:
                source_occurrences[path.name] = count
        self.assertEqual(
            source_occurrences,
            {"BrowserRuntimeBridge.cpp": source_occurrences.get(
                "BrowserRuntimeBridge.cpp",
                0,
            )},
        )
        self.assertGreater(
            source_occurrences.get("BrowserRuntimeBridge.cpp", 0),
            0,
        )
        for marker in (
            "EM_JS(",
            "EM_ASYNC_JS(",
            "rgGate1bAwaitOwnedNonce",
            "QJsonDocument::fromJson",
            "QJsonDocument::Compact",
            "QJsonParseError",
            "rhythmGameGate1bCommand",
            "EMSCRIPTEN_BINDINGS",
            '"probe-ping"',
            '"set-shader-phase"',
            "QThread::currentThread",
            "commandHandlerGeneration",
            "installationGeneration",
            "navigator.userActivation?.isActive",
            "isFinite",
            "0.20",
            "0.80",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bridge)
        for forbidden in (
            "emscripten_sleep",
            "-sASYNCIFY",
            "ALLOW_BLOCKING_ON_MAIN_THREAD=1",
            "commandOwner == destroyedOwner",
            "unsafe-eval",
        ):
            self.assertNotIn(forbidden, bridge)

    def test_task3_runtime_observes_the_compiled_input_build_id(self) -> None:
        digest_template = self._read_required(
            "cmake/ProbeInputDigest.cpp.in"
        )
        state = self._read_required("src/ProbeState.cpp")
        bootstrap = self._read_required("browser/web/bootstrap.mjs")
        spec = self._read_required("browser/tests/gate1b.spec.mjs")

        self.assertIn(
            "RG_WASM_PROBE_INPUT_SHA256=@WASM_PROBE_INPUT_DIGEST@",
            digest_template,
        )
        for marker in (
            'extern "C" const char *rhythmGameWasmProbeInputDigest();',
            "inputDigestMarkerPrefix",
            '"RG_WASM_PROBE_INPUT_SHA256="',
            "compiledInputBuildId()",
            "marker.startsWith(prefix)",
            "marker.size() != prefix.size() + 64",
            'QStringLiteral("inputBuildId")',
        ):
            with self.subTest(cpp_marker=marker):
                self.assertIn(marker, state)
        for marker in (
            "const compiledIdentity = await report.command("
            '"probe-ping", {});',
            '["command", "inputBuildId"]',
            "compiledIdentity.inputBuildId !== manifest.buildId",
            '"runtime-compiled-input-digest-mismatch"',
        ):
            with self.subTest(bootstrap_marker=marker):
                self.assertIn(marker, bootstrap)
        for marker in (
            "inputBuildId: reply.inputBuildId",
            "expect(boundCommand.inputBuildId).toBe(expectedRuntimeBuildId)",
            "expect(boundCommand.inputBuildId).toBe(actualRuntimeBuildId)",
            "readyReport.events[readyPingIndex].payload.inputBuildId",
        ):
            with self.subTest(spec_marker=marker):
                self.assertIn(marker, spec)

    def test_task3_jspi_contract_requires_real_promise_resumption(
        self,
    ) -> None:
        header = self._read_required("src/JspiNestedLoopProbe.h")
        source = self._read_required("src/JspiNestedLoopProbe.cpp")
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        combined = header + source
        for marker in (
            "struct JspiNestedLoopResult",
            "bool promiseResolvedWhileExec;",
            "bool quitDelivered;",
            "bool postLoopSentinel;",
            "bool fullPumpDeferredWhilePrimary;",
            "bool nativeComposedPathsIntact;",
            "bool nativeEventIdentitiesIntact;",
            "bool nativeStackCanariesIntact;",
            "bool primaryStackCanaryIntact;",
            "bool primaryStackCanaryObservedIntact;",
            "quint32 requestedNonce;",
            "quint32 resolvedNonce;",
            "qint64 elapsedMicroseconds;",
            "QJsonArray nativeEnterOrder;",
            "QJsonArray nativeExitOrder;",
            "QList<quint32> m_nativeBoundaryNegativeHandlers;",
            "[[nodiscard]] JspiNestedLoopResult runJspiNestedLoop();",
            "QEventLoop",
            "QTimer::singleShot",
            "loop.exec()",
            "loop.quit()",
            "jspi-before-exec",
            "jspi-before-import",
            "jspi-promise-resolved",
            "jspi-quit-delivered",
            "jspi-after-exec",
            "qt-native-suspension-returned",
            "strictlyEquals",
            'call<emscripten::val>("composedPath")',
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)
        for marker in (
            "EM_ASYNC_JS(",
            "rgGate1bAwaitOwnedNonce",
            "rgDispatchNativeDepthProbeEvent",
            "rgScheduleNativeDepthProbeEvent",
            "rgDispatchNativeDepthLimitAttemptEvent",
            "rgVerifyNativeStackCanary",
            "rgAwaitUnmatchedNativeSuspension",
            "queueMicrotask",
            "new Promise(() => {})",
            "crypto.getRandomValues",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bridge)
        async_import_start = bridge.index("EM_ASYNC_JS(")
        async_import_end = bridge.index(
            "EMSCRIPTEN_BINDINGS",
            async_import_start,
        )
        async_import = bridge[async_import_start:async_import_end]
        for marker in (
            "requestedNonce",
            "new Promise",
            "Promise.race",
            "setTimeout(",
            "clearTimeout(",
            "jspiWatchdogSentinel",
            "jspi-resume-watchdog-timeout",
            "jspi-after-exec",
            "postLoopSentinel",
            "report.rejectReady",
            'document.querySelector("#screen")',
            "screen instanceof HTMLElement",
            "onTrustedPointerUp",
            "event.isTrusted !== true",
            "event.button !== 0",
            "screen.addEventListener(",
            '{ capture: true }',
            'screen.removeEventListener(',
            "removeTrustedPointerListener",
            "cancelOwnedImport",
            "Promise.resolve(report.ready).catch",
            "importCanceled",
            "resumeWatchdogTimer",
        ):
            with self.subTest(owned_async_import=marker):
                self.assertIn(marker, async_import)
        self.assertGreaterEqual(async_import.count("setTimeout("), 3)
        self.assertEqual(async_import.count("clearTimeout("), 3)
        inner_race_start = async_import.index("let successTimer")
        resume_watchdog = async_import[:inner_race_start]
        for marker in (
            "jspi-resume-watchdog-timeout",
            "jspi-after-exec",
            "postLoopSentinel",
            "report.rejectReady",
            "}, 7500);",
        ):
            with self.subTest(resume_watchdog=marker):
                self.assertIn(marker, resume_watchdog)
        self.assertNotIn("clearTimeout(", resume_watchdog)
        self.assertIn(
            "clearTimeout(successTimer)",
            async_import[inner_race_start:],
        )
        self.assertIn(
            "clearTimeout(sentinelTimer)",
            async_import[inner_race_start:],
        )
        self.assertIn(
            "clearTimeout(resumeWatchdogTimer)",
            async_import[inner_race_start:],
        )
        self.assertIn(
            "() => resolve(jspiWatchdogSentinel),\n"
            "                5000,",
            async_import[inner_race_start:],
        )
        self.assertIn("requestedNonce == resolvedNonce", source)
        self.assertIn("result.nativeEnterOrder.append(ordinal)", source)
        self.assertIn("result.nativeExitOrder.append(ordinal)", source)
        self.assertIn("fullPumpDeferredWhilePrimary", source)
        self.assertIn("primaryStackCanaryObservedIntact", source)
        self.assertIn("elapsedMicroseconds", source)
        self.assertIn("postLoopSentinel", source)
        self.assertEqual(
            source.count(
                "m_nativeBoundaryNegativeHandlers.append(handler);"
            ),
            2,
        )
        self.assertGreaterEqual(
            source.count(
                "releaseNativeBoundaryNegativeProbeHandlers();"
            ),
            3,
        )
        self.assertIn(
            "std::exchange(m_nativeBoundaryNegativeHandlers, {})",
            source,
        )
        self.assertNotIn("elapsedMicroseconds < 2'000'000", source)
        spec = (
            PROBE / "browser" / "tests" / "gate1b.spec.mjs"
        ).read_text("utf-8")
        self.assertNotIn(
            "elapsedMicroseconds).toBeLessThan(2_000_000)",
            spec,
        )
        self.assertNotIn("native-suspension-trap-bypassed", combined)
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        self.assertIn('"SHELL:-sJSPI"', cmake)
        self.assertNotIn("-sASYNCIFY", cmake)

    def test_task3_exclusive_suspend_guard_is_discriminating(
        self,
    ) -> None:
        header = self._read_required("src/JspiNestedLoopProbe.h")
        source = self._read_required("src/JspiNestedLoopProbe.cpp")
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        state = self._read_required("src/ProbeState.cpp")
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        for marker in (
            "struct ExclusiveSuspendGuardResult",
            "ExclusiveSuspendGuardCallback",
            "startExclusiveSuspendGuardProbe",
            "finishExclusiveSuspendGuardProbe",
            "QWasmSuspendResumeControl::get()",
            "registerEventHandler",
            "suspendExclusive({exclusiveHandler})",
            "ownerResumedByExclusive",
            "exclusiveDrainCount",
            "armExclusiveSuspendGuardNormalDrain()",
            "completionDrainCount",
            "completionFinalizedAfterOwnerReturn",
            "exclusiveDomDispatch",
            "normalDrainOwnedQueue",
            "earlierOrdinaryPumpOwnedQueue",
            "QTimer::singleShot(0, this",
            "m_exclusiveState",
            "state->deliveryOrder == expectedDeliveryOrder",
            "firstGuardValid && secondGuardValid && normalValid",
            "removeEventHandler",
        ):
            with self.subTest(cpp_marker=marker):
                self.assertIn(marker, header + source)
        for marker in (
            "rgScheduleExclusiveSuspendGuardProbe",
            "control.exclusiveEventHandler !== 0",
            "control.resume !== null",
            "pendingBefore: before",
            "const result = await boundPump()",
            "invoke(firstHandler >>> 0, \"first\")",
            "void guard(1).catch(recordError)",
            "invoke(secondHandler >>> 0, \"second\")",
            "void guard(2).catch(recordError)",
            "invoke(exclusiveHandler >>> 0, \"exclusive\", true)",
            'screen.addEventListener("click", capture',
            "screen.dispatchEvent(event)",
            "state.exclusiveDomDispatch = true",
            "rgArmExclusiveSuspendGuardNormalDrain",
            "state.normalArmed !== false",
            "control.exclusiveEventHandler !== 0",
            "control.resume !== null",
            "state.normalArmed = true",
            "state.timerIds.push(setTimeout(() => {",
            "void state.normalDrain();",
            "const scheduleCompletion = () => {",
            "scheduleCompletion();",
            "invoke(completionHandler >>> 0, \"completion\")",
        ):
            with self.subTest(bridge_marker=marker):
                self.assertIn(marker, bridge)
        normal_drain_start = bridge.index("const normalDrain = async () => {")
        normal_drain_end = bridge.index(
            'Object.defineProperty(state, "normalDrain"',
            normal_drain_start,
        )
        self.assertIn(
            "scheduleCompletion();",
            bridge[normal_drain_start:normal_drain_end],
        )
        self.assertNotIn(
            'invoke(completionHandler >>> 0, "completion")',
            bridge[normal_drain_start:normal_drain_end],
        )
        schedule_completion_start = bridge.index(
            "const scheduleCompletion = () => {",
        )
        schedule_completion_end = bridge.index(
            "const normalDrain = async () => {",
            schedule_completion_start,
        )
        schedule_completion = bridge[
            schedule_completion_start:schedule_completion_end
        ]
        self.assertIn("schedule(() => {", schedule_completion)
        self.assertIn(
            'invoke(completionHandler >>> 0, "completion")',
            schedule_completion,
        )
        self.assertIn("}, 0);", schedule_completion)
        exclusive_bridge_start = bridge.index(
            "rgScheduleExclusiveSuspendGuardProbe",
        )
        exclusive_bridge_end = bridge.index(
            "EM_JS(uint32_t, rgGenerateOwnedBrowserNonce",
            exclusive_bridge_start,
        )
        self.assertNotIn(
            "queueMicrotask",
            bridge[exclusive_bridge_start:exclusive_bridge_end],
        )
        self.assertNotIn("control->suspend();", source)
        self.assertNotIn("ordinaryResumeCount", source)
        self.assertIn(
            'u"qt-exclusive-suspend-guard"',
            state,
        )
        self.assertIn(
            'm_report->hasPassed(u"qt-exclusive-suspend-guard")',
            state,
        )
        self.assertIn("Qt6::CorePrivate", cmake)

    def test_task3_threads_are_nonblocking_distinct_and_overlapped(
        self,
    ) -> None:
        state_header = self._read_required("src/ProbeState.h")
        state_source = self._read_required("src/ProbeState.cpp")
        combined = state_header + state_source
        for marker in (
            "QtConcurrent::run",
            "pthread_create",
            "pthread_tryjoin_np",
            "EBUSY",
            "pthread_self()",
            "emscripten_is_main_runtime_thread()",
            "memory_order_release",
            "memory_order_acquire",
            "QFutureWatcher",
            "QTimer",
            "explicitReady",
            "releaseExplicit",
            "overlapObserved",
            "std::chrono::steady_clock",
            "explicitReadyMicroseconds",
            "explicitCompletedMicroseconds",
            "qtConcurrentStartedMicroseconds",
            "qtConcurrentObservedReadyMicroseconds",
            "qtConcurrentReleaseMicroseconds",
            "return 42;",
            "crossStaticLibraryBoundary",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        gnu_source_definition = re.search(
            (
                r"target_compile_definitions\(\s*"
                r"RhythmGameWasmProbe\s+PRIVATE"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(
            gnu_source_definition,
            "_GNU_SOURCE must be a target compile definition",
        )
        self.assertIn("_GNU_SOURCE", gnu_source_definition.group("body"))
        self.assertIsNotNone(
            re.search(r"0xA5A55A5A", combined, re.IGNORECASE),
        )
        self.assertGreaterEqual(
            combined.count("emscripten_is_main_runtime_thread()"),
            2,
        )
        self.assertRegex(
            combined,
            (
                r"pthread_tryjoin_np\s*\([^;]+;\s*"
                r"(?:if\s*\([^)]*==\s*EBUSY|"
                r"switch\s*\([^)]*\)[\s\S]*?case\s+EBUSY)"
            ),
        )
        for ownership_marker in (
            "std::unique_ptr<std::shared_ptr<ExplicitWorkerState>>",
            "new std::shared_ptr<ExplicitWorkerState>",
            "delete workerStateOwner",
            "pthread_detach(context.explicitThread)",
        ):
            with self.subTest(ownership_marker=ownership_marker):
                self.assertIn(ownership_marker, state_source)
        self.assertNotIn(
            "context.explicitState.get()",
            state_source,
        )
        destructor_start = state_source.index(
            "ProbeState::~ProbeState()",
        )
        destructor_end = state_source.index(
            "bool ProbeState::exceptionPassed() const",
            destructor_start,
        )
        destructor = state_source[destructor_start:destructor_end]
        self.assertIn("context.pollTimer->stop();", destructor)
        self.assertIn(
            "context.explicitState->releaseExplicit.store(",
            destructor,
        )
        self.assertIn("std::memory_order_release", destructor)
        self.assertIn(
            "pthread_detach(context.explicitThread)",
            destructor,
        )
        task3_sources = "\n".join(
            path.read_text("utf-8")
            for path in sorted((PROBE / "src").glob("*"))
            if path.suffix in (".cpp", ".h")
        )
        self.assertNotRegex(task3_sources, r"(?<!try)pthread_join\s*\(")
        for forbidden in (
            "waitForFinished",
            "future.waitForFinished",
            "QThread::wait",
            "emscripten_futex_wait",
        ):
            self.assertNotIn(forbidden, task3_sources)

    def test_task3_post_main_readiness_is_event_ordered(self) -> None:
        main = self._read_required("src/main.cpp")
        state = self._read_required("src/ProbeState.cpp")
        for marker in (
            "new QGuiApplication",
            "new ProbeState",
            "new QQmlApplicationEngine",
            "QQuickWindow",
            "rootObjects().size() != 1",
            "attachWindow",
            "qml-root-attached",
            "QTimer::singleShot(0",
            "post-main-tick",
            "post-main-application-state",
            "QCoreApplication::arguments()",
            "QCoreApplication::applicationFilePath()",
            "post-main-application-state-invalid",
            "argumentsMatchRetainedCopy",
            "arguments == m_expectedArguments",
            "main-returning",
            "return 0;",
            "return app->exec();",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, main + state)
        schedule = main.find("QTimer::singleShot(0")
        returning = main.find("main-returning", schedule)
        wasm_return = main.find("return 0;", returning)
        self.assertGreater(schedule, 0)
        self.assertGreater(returning, schedule)
        self.assertGreater(wasm_return, returning)
        self.assertIn("post-main-tick", main[schedule:returning])
        for completion in (
            "startThreadProbes",
            "startJspiProbe",
        ):
            self.assertIn(completion, state)
        post_main = state.find("post-main-tick")
        self.assertGreater(state.find("startThreadProbes", post_main), post_main)
        self.assertGreater(state.find("startJspiProbe", post_main), post_main)

    def test_task3_render_probe_uses_the_current_qt_webgl2_context(
        self,
    ) -> None:
        header = self._read_required("src/RenderProbe.h")
        source = self._read_required("src/RenderProbe.cpp")
        state = self._read_required("src/ProbeState.cpp")
        combined = header + source
        for marker in (
            "QQuickWindow",
            "beforeRendering",
            "Qt::DirectConnection",
            "QSGRendererInterface::graphicsApi()",
            "QSGRendererInterface::OpenGL",
            "emscripten_webgl_get_current_context()",
            "emscripten_webgl_get_context_attributes(",
            "EMSCRIPTEN_RESULT_SUCCESS",
            "attributes.majorVersion",
            "std::atomic",
            "std::deque<AtomicFrame>",
            "std::lock_guard",
            "std::mutex",
            "memory_order_release",
            "memory_order_acquire",
            "QTimer",
            "post-main",
            "scheduleNextCaptureFrame",
            "scheduledWindow",
            "scheduledCaptureKind",
            "scheduledCaptureKindValue",
            "scheduledCaptureGeneration",
            "hasQueuedFrameForCaptureLocked",
            "scheduledWindow->update();",
            "m_publishedFrames.push_back(frame);",
            "m_publishedFrames.pop_front();",
            "readNextFrame",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)
        self.assertIsNotNone(
            re.search(
                (
                    r"enum class CaptureKind : int\s*"
                    r"\{\s*"
                    r"None = 0,\s*"
                    r"PostMain = 1,\s*"
                    r"Phase = 2,\s*"
                    r"\};"
                ),
                header,
            ),
        )
        self.assertRegex(
            header,
            r"bool m_discardNextCaptureFrame = false;",
        )
        constructor_start = source.index("RenderProbe::RenderProbe(")
        constructor_end = source.index(
            "void RenderProbe::attachWindow(",
            constructor_start,
        )
        constructor = source[constructor_start:constructor_end]
        self.assertEqual(
            constructor.count(
                "m_pollTimer->setInterval(std::chrono::milliseconds{1});"
            ),
            1,
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"connect\(\s*"
                    r"m_pollTimer,\s*"
                    r"&QTimer::timeout,\s*"
                    r"this,\s*"
                    r"&RenderProbe::consumeLatestFrame\s*"
                    r"\);"
                ),
                constructor,
            ),
        )
        attach_start = source.index("void RenderProbe::attachWindow(")
        attach_end = source.index(
            "quint64 RenderProbe::beginPostMainCapture()",
            attach_start,
        )
        attach = source[attach_start:attach_end]
        self.assertNotIn("m_pollTimer->start();", attach)
        self.assertNotIn("m_pollTimer->stop();", attach)
        self.assertEqual(combined.count("m_pollTimer->start();"), 1)
        self.assertEqual(combined.count("m_pollTimer->stop();"), 2)
        self.assertNotIn("m_pollTimer->setSingleShot(true);", combined)
        wrappers_start = attach_end
        wrappers_end = source.index(
            "quint64 RenderProbe::beginCapture(",
            wrappers_start,
        )
        wrappers = source[wrappers_start:wrappers_end]
        self.assertIsNotNone(
            re.fullmatch(
                (
                    r"quint64 RenderProbe::beginPostMainCapture\(\)\s*"
                    r"\{\s*"
                    r"return beginCapture\(CaptureKind::PostMain, 0\);\s*"
                    r"\}\s*"
                    r"quint64 RenderProbe::beginPhaseCapture"
                    r"\(quint64 generation\)\s*"
                    r"\{\s*"
                    r"return beginCapture\(CaptureKind::Phase, generation\);\s*"
                    r"\}\s*"
                    r"quint64 RenderProbe::latestFrameSequence\(\) const\s*"
                    r"\{\s*"
                    r"return m_frameCounter\.load"
                    r"\(std::memory_order_acquire\);\s*"
                    r"\}\s*"
                ),
                wrappers,
            ),
        )
        consume_start = source.index(
            "void RenderProbe::consumeLatestFrame()",
        )
        consume_end = source.index(
            "void RenderProbe::captureTimedOut",
            consume_start,
        )
        consume = source[consume_start:consume_end]
        self.assertEqual(consume.count("readNextFrame(frame)"), 1)
        self.assertIsNotNone(
            re.match(
                (
                    r"void RenderProbe::consumeLatestFrame\(\)\s*"
                    r"\{\s*"
                    r"AtomicFrame frame;\s*"
                    r"if \(!readNextFrame\(frame\)\) \{\s*"
                    r"scheduleNextCaptureFrame\(\);\s*"
                    r"return;\s*"
                    r"\}\s*"
                    r"if \(m_guiCaptureKind == CaptureKind::None\s*"
                    r"\|\| frame\.captureKind != "
                    r"static_cast<int>\(m_guiCaptureKind\)\s*"
                    r"\|\| frame\.generation != "
                    r"m_guiCaptureGeneration\s*"
                    r"\|\| frame\.sequence <= m_guiCaptureBaseline\s*"
                    r"\|\| frame\.sequence <= "
                    r"m_lastPublishedFrame\) \{\s*"
                    r"return;\s*"
                    r"\}\s*"
                    r"m_lastPublishedFrame = frame\.sequence;\s*"
                    r"if \(m_discardNextCaptureFrame\) \{\s*"
                    r"m_discardNextCaptureFrame = false;\s*"
                    r"scheduleNextCaptureFrame\(\);\s*"
                    r"return;\s*"
                    r"\}"
                ),
                consume,
            ),
        )
        begin_start = source.index(
            "quint64 RenderProbe::beginCapture(",
        )
        begin_end = source.index(
            "void RenderProbe::recordRenderFrame(",
            begin_start,
        )
        begin_capture = source[begin_start:begin_end]
        self.assertIsNotNone(
            re.search(
                (
                    r"m_guiCaptureKind = kind;\s*"
                    r"m_guiCaptureGeneration = generation;\s*"
                    r"m_guiCaptureFrameCount = 0;\s*"
                    r"m_discardNextCaptureFrame = true;\s*"
                    r"m_guiCaptureBaseline = latestFrameSequence\(\);\s*"
                    r"m_lastPublishedFrame = m_guiCaptureBaseline;"
                ),
                begin_capture,
            ),
        )
        self.assertEqual(
            source.count("scheduleNextCaptureFrame();"),
            4,
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"m_activeCaptureKind\.store\(\s*"
                    r"static_cast<int>\(kind\),\s*"
                    r"std::memory_order_release\s*\);\s*"
                    r"\}\s*"
                    r"m_pollTimer->start\(\);\s*"
                    r"m_captureDeadlineTimer->start"
                    r"\(captureDeadline\);\s*"
                    r"scheduleNextCaptureFrame\(\);\s*"
                    r"return m_guiCaptureBaseline;\s*"
                    r"\}\s*$"
                ),
                begin_capture,
            ),
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"\{\s*"
                    r"const std::lock_guard lock"
                    r"\{m_publishedFramesMutex\};\s*"
                    r"m_publishedFrames\.clear\(\);\s*"
                    r"m_activeCaptureGeneration\.store\(\s*"
                    r"generation,\s*std::memory_order_release\s*\);\s*"
                    r"m_activeCaptureKind\.store\(\s*"
                    r"static_cast<int>\(kind\),\s*"
                    r"std::memory_order_release\s*\);\s*"
                    r"\}\s*"
                    r"m_pollTimer->start\(\);\s*"
                    r"m_captureDeadlineTimer->start"
                    r"\(captureDeadline\);\s*"
                    r"scheduleNextCaptureFrame\(\);\s*"
                    r"return m_guiCaptureBaseline;"
                ),
                begin_capture,
            ),
        )
        self.assertNotIn("m_window->update();", begin_capture)
        record_start = source.index(
            "void RenderProbe::recordRenderFrame(",
        )
        record_end = source.index(
            "void RenderProbe::consumeLatestFrame()",
            record_start,
        )
        record = source[record_start:record_end]
        self.assertIsNotNone(
            re.search(
                (
                    r"const quint64 frameSequence =\s*"
                    r"m_frameCounter\.fetch_add"
                    r"\(1, std::memory_order_relaxed\) \+ 1;"
                ),
                record,
            ),
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"const AtomicFrame frame\{\s*"
                    r"\.sequence = frameSequence,\s*"
                    r"\.generation = generation,\s*"
                    r"\.contextHandle = contextHandle,\s*"
                    r"\.contextAttributesResult = "
                    r"contextAttributesResult,\s*"
                    r"\.majorVersion = majorVersion,\s*"
                    r"\.graphicsApi = static_cast<int>"
                    r"\(graphicsApi\),\s*"
                    r"\.captureKind = captureKind,\s*"
                    r"\};\s*"
                    r"const std::lock_guard lock"
                    r"\{m_publishedFramesMutex\};\s*"
                    r"if \(m_activeCaptureGeneration\.load"
                    r"\(std::memory_order_acquire\)\s*"
                    r"!= generation\s*"
                    r"\|\| m_activeCaptureKind\.load"
                    r"\(std::memory_order_acquire\)\s*"
                    r"!= captureKind\) \{\s*"
                    r"return;\s*"
                    r"\}\s*"
                    r"m_publishedFrames\.push_back\(frame\);\s*"
                    r"\}\s*$"
                ),
                record,
            ),
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"if \(m_discardNextCaptureFrame\) \{\s*"
                    r"m_discardNextCaptureFrame = false;\s*"
                    r"scheduleNextCaptureFrame\(\);\s*"
                    r"return;\s*"
                    r"\}"
                ),
                consume,
            ),
        )
        self.assertEqual(
            consume.count(
                "const bool captureFinished = "
                "m_guiCaptureFrameCount >= 2;",
            ),
            1,
        )
        self.assertNotIn("m_window->update();", consume)
        active_kind_cleared = consume.rindex(
            "m_activeCaptureKind.store(",
        )
        active_generation_cleared = consume.rindex(
            "m_activeCaptureGeneration.store(",
        )
        gui_capture_cleared = consume.rindex(
            "m_guiCaptureKind = CaptureKind::None;",
        )
        milestone_published = consume.index(
            "m_milestoneCallback(milestone);",
        )
        capture_completed = consume.index(
            "m_captureCompletedCallback(milestone);",
        )
        completion_branch_match = re.search(
            (
                r"if \(!captureFinished\) \{\s*"
                r"scheduleNextCaptureFrame\(\);\s*"
                r"\} else \{(?P<body>.*?)"
                r"\n    \}\n\n    m_milestoneCallback"
            ),
            consume,
            re.DOTALL,
        )
        self.assertIsNotNone(completion_branch_match)
        completion_branch = completion_branch_match.group("body")
        branch_active_kind_match = re.search(
            (
                r"m_activeCaptureKind\.store\(\s*"
                r"static_cast<int>\(CaptureKind::None\),\s*"
                r"std::memory_order_release\s*\);"
            ),
            completion_branch,
        )
        branch_active_generation_match = re.search(
            (
                r"m_activeCaptureGeneration\.store\(\s*"
                r"0,\s*std::memory_order_release\s*\);"
            ),
            completion_branch,
        )
        self.assertIsNotNone(branch_active_kind_match)
        self.assertIsNotNone(branch_active_generation_match)
        branch_active_kind_cleared = branch_active_kind_match.start()
        branch_active_generation_cleared = (
            branch_active_generation_match.start()
        )
        branch_gui_capture_cleared = completion_branch.index(
            "m_guiCaptureKind = CaptureKind::None;",
        )
        branch_poll_timer_stopped = completion_branch.index(
            "m_pollTimer->stop();",
        )
        self.assertIsNotNone(
            re.search(
                (
                    r"\{\s*"
                    r"const std::lock_guard lock"
                    r"\{m_publishedFramesMutex\};\s*"
                    r"m_publishedFrames\.clear\(\);\s*"
                    r"m_activeCaptureKind\.store\(\s*"
                    r"static_cast<int>\(CaptureKind::None\),\s*"
                    r"std::memory_order_release\s*\);\s*"
                    r"m_activeCaptureGeneration\.store\(\s*"
                    r"0,\s*std::memory_order_release\s*\);\s*"
                    r"\}\s*"
                    r"m_guiCaptureKind = CaptureKind::None;\s*"
                    r"m_pollTimer->stop\(\);"
                ),
                completion_branch,
            ),
        )
        self.assertLess(
            branch_active_kind_cleared,
            branch_active_generation_cleared,
        )
        self.assertLess(
            branch_active_generation_cleared,
            branch_gui_capture_cleared,
        )
        self.assertLess(
            branch_gui_capture_cleared,
            branch_poll_timer_stopped,
        )
        self.assertLess(branch_poll_timer_stopped, milestone_published)
        self.assertLess(active_kind_cleared, milestone_published)
        self.assertLess(active_generation_cleared, milestone_published)
        self.assertLess(gui_capture_cleared, milestone_published)
        self.assertLess(milestone_published, capture_completed)
        schedule_start = source.index(
            "void RenderProbe::scheduleNextCaptureFrame()",
        )
        schedule_end = source.index(
            "bool RenderProbe::hasQueuedFrameForCaptureLocked(",
            schedule_start,
        )
        queued_helper_end = source.index(
            "bool RenderProbe::readNextFrame(",
            schedule_end,
        )
        schedule = source[schedule_start:schedule_end]
        queued_helper = source[schedule_end:queued_helper_end]
        read_next = source[queued_helper_end:]
        self.assertEqual(
            source.count("QTimer::singleShot("),
            0,
        )
        self.assertIsNotNone(
            re.fullmatch(
                (
                    r"void RenderProbe::scheduleNextCaptureFrame\(\)\s*"
                    r"\{\s*"
                    r"const QPointer<QQuickWindow> scheduledWindow =\s*"
                    r"m_window;\s*"
                    r"if \(scheduledWindow\.isNull\(\)\s*"
                    r"\|\| m_guiCaptureKind == CaptureKind::None\) \{\s*"
                    r"return;\s*"
                    r"\}\s*"
                    r"const CaptureKind scheduledCaptureKind =\s*"
                    r"m_guiCaptureKind;\s*"
                    r"const int scheduledCaptureKindValue =\s*"
                    r"static_cast<int>\(scheduledCaptureKind\);\s*"
                    r"const quint64 scheduledCaptureGeneration =\s*"
                    r"m_guiCaptureGeneration;\s*"
                    r"\{\s*"
                    r"const std::lock_guard lock"
                    r"\{m_publishedFramesMutex\};\s*"
                    r"if \(hasQueuedFrameForCaptureLocked\(\s*"
                    r"scheduledCaptureKindValue,\s*"
                    r"scheduledCaptureGeneration\)\s*"
                    r"\|\| m_activeCaptureKind\.load\(\s*"
                    r"std::memory_order_acquire\)\s*"
                    r"!= scheduledCaptureKindValue\s*"
                    r"\|\| m_activeCaptureGeneration\.load\(\s*"
                    r"std::memory_order_acquire\)\s*"
                    r"!= scheduledCaptureGeneration\s*"
                    r"\) \{\s*"
                    r"return;\s*"
                    r"\}\s*"
                    r"\}\s*"
                    r"(?:[ \t]*//[^\n]*\n)*\s*"
                    r"scheduledWindow->update\(\);\s*"
                    r"\}\s*"
                ),
                schedule,
            ),
        )
        self.assertIsNotNone(
            re.fullmatch(
                (
                    r"bool RenderProbe::hasQueuedFrameForCaptureLocked\(\s*"
                    r"int captureKind,\s*"
                    r"quint64 generation\) const\s*"
                    r"\{\s*"
                    r"return std::any_of\(\s*"
                    r"m_publishedFrames\.cbegin\(\),\s*"
                    r"m_publishedFrames\.cend\(\),\s*"
                    r"\[captureKind, generation\]"
                    r"\(const AtomicFrame &frame\) \{\s*"
                    r"return frame\.captureKind == captureKind\s*"
                    r"&& frame\.generation == generation;\s*"
                    r"\}\);\s*"
                    r"\}\s*"
                ),
                queued_helper,
            ),
        )
        self.assertNotIn("m_captureUpdateScheduled", combined)
        self.assertNotIn("m_captureUpdateTimer", combined)
        self.assertNotIn("CaptureWakeSerial", combined)
        self.assertNotIn("wakeSerial", combined)
        for forbidden_scheduler in (
            "QMetaObject::invokeMethod",
            "QTimer::singleShot",
            "Qt::QueuedConnection",
            "emscripten_async_call",
            "emscripten_request_animation_frame",
            "processEvents(",
        ):
            with self.subTest(
                forbidden_scheduler=forbidden_scheduler,
            ):
                self.assertNotIn(forbidden_scheduler, combined)
        self.assertEqual(
            schedule.count("scheduledWindow->update();"),
            1,
        )
        self.assertNotIn("scheduleNextCaptureFrame();", record)
        self.assertEqual(source.count("m_publishedFrames.clear();"), 3)
        self.assertEqual(
            source.count("m_publishedFrames.push_back(frame);"),
            1,
        )
        self.assertEqual(
            source.count("m_publishedFrames.pop_front();"),
            1,
        )
        self.assertIsNotNone(
            re.fullmatch(
                (
                    r"bool RenderProbe::readNextFrame"
                    r"\(AtomicFrame &frame\)\s*"
                    r"\{\s*"
                    r"const std::lock_guard lock"
                    r"\{m_publishedFramesMutex\};\s*"
                    r"if \(m_publishedFrames\.empty\(\)\) \{\s*"
                    r"return false;\s*"
                    r"\}\s*"
                    r"frame = m_publishedFrames\.front\(\);\s*"
                    r"m_publishedFrames\.pop_front\(\);\s*"
                    r"return true;\s*"
                    r"\}\s*"
                ),
                read_next,
            ),
        )
        for forbidden in (
            "m_publicationGuard",
            "m_publishedSequence",
            "m_publishedGeneration",
            "m_publishedContextHandle",
            "m_publishedContextAttributesResult",
            "m_publishedMajorVersion",
            "m_publishedGraphicsApi",
            "m_publishedCaptureKind",
            "readCoherentFrame",
        ):
            self.assertNotIn(forbidden, combined)
        command_start = state.index(
            "case BrowserRuntimeCommand::SetShaderPhase:",
        )
        command_end = state.index(
            "void ProbeState::runStaticLibraryExceptionProbe()",
            command_start,
        )
        self.assertNotIn(
            "m_window->update();",
            state[command_start:command_end],
        )
        for forbidden in (
            "QJson",
            "Gate1bReport",
            "document.createElement",
            'getContext("webgl2")',
        ):
            self.assertNotIn(forbidden, source)

    def test_task3_qml_uses_basic_button_bindings_and_exact_shader(
        self,
    ) -> None:
        qml = self._read_required("qml/Main.qml")
        shader = self._read_required("qml/pulse.frag")
        imports = [
            line.strip()
            for line in qml.splitlines()
            if line.strip().startswith("import ")
        ]
        self.assertEqual(
            imports,
            [
                "import QtQuick",
                "import QtMultimedia",
                "import QtQuick.Controls.Basic",
            ],
        )
        self.assertEqual(len(re.findall(r"(?m)^\s*Button\s*\{", qml)), 1)
        self.assertGreater(qml.find("Button {"), qml.find("ShaderEffect {"))
        for marker in (
            "x: 24",
            "y: 296",
            "width: 200",
            "height: 40",
            "onClicked: probeState.beginUserActivatedProbes()",
            'title: qsTr("RhythmGame Wasm Gate 1B")',
            'text: qsTr("Start browser probes")',
            "property real animatedPhase",
            "probeState.phaseFrozen",
            "probeState.shaderPhase",
            'property: "animatedPhase"',
            "running: root.visible && !probeState.phaseFrozen",
            "pulse.frag.qsb",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, qml)
        self.assertNotIn("NumberAnimation on phase", qml)
        self.assertNotRegex(
            qml,
            r"on[A-Za-z0-9]+Changed\s*:\s*shader\.phase\s*=",
        )
        self.assertNotRegex(qml, r'(?:title|text)\s*:\s*"[^"]*"')
        for marker in (
            "0.10 + 0.50 * phase",
            "0.20 + 0.25 * phase",
            "0.80 - 0.50 * phase",
            "1.00",
        ):
            with self.subTest(shader_formula=marker):
                self.assertIn(marker, shader)

    def test_task3_cmake_and_manifest_use_existing_quickcontrols2(
        self,
    ) -> None:
        cmake = (PROBE / "CMakeLists.txt").read_text("utf-8")
        find_package = re.search(
            r"find_package\(Qt6[^)]*COMPONENTS(?P<body>.*?)\)",
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(find_package)
        self.assertIn("QuickControls2", find_package.group("body"))
        link_libraries = re.search(
            (
                r"target_link_libraries\(\s*"
                r"RhythmGameWasmProbe\s+PRIVATE"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(link_libraries)
        self.assertIn("Qt6::QuickControls2", link_libraries.group("body"))
        executable = re.search(
            (
                r"qt_add_executable\(\s*"
                r"RhythmGameWasmProbe"
                r"(?P<body>.*?)\n\)"
            ),
            cmake,
            re.DOTALL,
        )
        self.assertIsNotNone(executable)
        for leaf in (
            "BrowserRuntimeBridge.cpp",
            "BrowserRuntimeBridge.h",
            "Gate1bReport.cpp",
            "Gate1bReport.h",
            "JspiNestedLoopProbe.cpp",
            "JspiNestedLoopProbe.h",
            "RenderProbe.cpp",
            "RenderProbe.h",
        ):
            with self.subTest(executable_input=leaf):
                self.assertIn(f"src/{leaf}", executable.group("body"))
        self.assertNotIn("JSPI_EXPORTS", cmake)
        input_entries = (
            PROBE / "input-manifest.txt"
        ).read_text("utf-8").splitlines()
        for relative in TASK3_SOURCE_INPUTS:
            with self.subTest(manifest_input=relative):
                self.assertEqual(input_entries.count(relative), 1)

    def test_probe_exercises_required_compile_time_surfaces(self) -> None:
        state = (PROBE / "src" / "ProbeState.cpp").read_text("utf-8")
        network = self._read_required("src/NetworkProbe.cpp")
        qml = (PROBE / "qml" / "Main.qml").read_text("utf-8")
        self.assertIn("QtConcurrent::run", state)
        self.assertIn("crossStaticLibraryBoundary", state)
        self.assertIn("QNetworkAccessManager", network)
        self.assertIn("QWebSocket", network)
        self.assertIn("ShaderEffect", qml)
        self.assertIn("VideoOutput", qml)

    def test_task4_network_probe_is_bounded_same_origin_and_main_thread(
        self,
    ) -> None:
        header = self._read_required("src/NetworkProbe.h")
        source = self._read_required("src/NetworkProbe.cpp")
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        for marker in (
            "QNetworkAccessManager",
            "QNetworkReply",
            "QWebSocket",
            "ManualRedirectPolicy",
            "setMaximumRedirectsAllowed(0)",
            "setTransferTimeout",
            "setReadBufferSize(maximumQnamResponseBytes + 1)",
            "remainingBytes + 1",
            "read(maximumReadableBytes)",
            "maximumQnamResponseBytes",
            "sameOriginUrl",
            "application/json",
            "heartbeat:",
            "qApp->thread()",
            "m_pendingWebSocketEvents",
            "appendWebSocketEvent",
            "flushPendingWebSocketEvents",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, header + source + bridge)
        self.assertNotIn(".ping(", source)
        self.assertNotIn("setRawHeader", source)
        self.assertNotIn("readAll()", source)

    def test_task4_media_probe_requires_frames_capture_seek_and_natural_end(
        self,
    ) -> None:
        header = self._read_required("src/MediaProbe.h")
        source = self._read_required("src/MediaProbe.cpp")
        qml = self._read_required("qml/Main.qml")
        for marker in (
            "QAudioDevice",
            "QAudioOutput",
            "QMediaDevices",
            "QMediaPlayer",
            "QVideoSink",
            "videoFrameChanged",
            "/fixtures/probe.webm?nonce=",
            "capture-ready",
            "qt-media-capture-pause-requested",
            "qt-media-resume-requested",
            "qt-media-playback-resumed",
            "QMediaPlayer::PausedState",
            "m_player->pause()",
            "m_player->play()",
            "requiredSeekPositionMilliseconds = 1000",
            "setPosition(requiredSeekPositionMilliseconds)",
            "QMediaPlayer::EndOfMedia",
            "armOwnedMediaSeekTracking",
            "finishOwnedMediaSeekTracking",
            "armOwnedMediaBackendRemoval",
            "finishOwnedMediaBackendRemoval",
            "releaseOwnedMediaBackendRemoval",
            "seekResponseTimeout",
            "minimumSeekJumpMilliseconds",
            "maximumSeekTargetErrorMilliseconds",
            "requestMonotonicMilliseconds",
            "preSeekPositionMilliseconds",
            "seekedMonotonicMilliseconds",
            "seekedPositionMilliseconds",
            "qt-media-seek-timeout",
            "qt-media-device-snapshot",
            "qt-media-device-batch-settled",
            "qt-media-device-change-overflow",
            "qt-media-device-settlement-overflow",
            "qt-media-player-output-created",
            "qt-media-post-seek-video-frame",
            "qt-media-audio-output-destroyed",
            "QCryptographicHash::Sha256",
            "maximumDeviceSnapshots = 16",
            "maximumDeviceSettlements = 16",
            "maximumPostSeekFramePositionSamples = 32",
            "requiredPostSeekAdvanceMilliseconds = 100",
            "backendRemovalPollInterval",
            "backendRemovalStabilityWindow",
            "backendRemovalResponseTimeout",
            "maximumEndPositionErrorMilliseconds = 125",
            "armDeviceObservation",
            "m_deviceSettleTimer",
            "m_postSeekFramePositionSamples",
            "m_backendRemovalPoll",
            "m_capturePaused",
            "m_resumeObserved",
            "m_resumePositionMilliseconds",
            "qt-media-resume-position",
            "m_audioInputSignalCount",
            "m_audioOutputSignalCount",
            "finishObjectTeardownIfReady",
            "&QMediaDevices::audioInputsChanged",
            "&QMediaDevices::audioOutputsChanged",
            "deleteLater",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, header + source)
        self.assertNotIn("setSource(QUrl{})", source)
        self.assertNotRegex(
            source,
            r"EndOfMedia[\s\S]{0,1200}->stop\(",
        )
        self.assertRegex(
            source,
            r"status == QMediaPlayer::EndOfMedia\)\s*\{"
            r"[\s\S]{0,180}QTimer::singleShot\("
            r"[\s\S]{0,180}&MediaProbe::handleNaturalEnd",
        )
        self.assertRegex(
            source,
            r"m_postSeekFramePositionSamples\.replace\(\s*"
            r"maximumPostSeekFramePositionSamples\s*-\s*1,\s*"
            r"position\)",
        )
        self.assertRegex(
            source,
            r"finalPostSeekFramePosition\s*"
            r"<\s*m_resumePositionMilliseconds\s*"
            r"\+\s*requiredPostSeekAdvanceMilliseconds",
        )
        self.assertIn(
            'u"qt-media-backend-removal-armed"',
            source,
        )
        self.assertIn(
            'u"qt-media-element-resource-released"',
            source,
        )
        self.assertIn(
            'u"qt-media-backend-removal-timeout"',
            source,
        )
        player_delete = source.index("player->deleteLater()")
        player_destruction_guard = source.index(
            "if (!m_playerDestructionRecorded)"
        )
        backend_removed = source.index(
            'u"qt-media-backend-removed"',
            player_destruction_guard,
        )
        backend_completion_guard = source.index(
            "|| !m_backendRemovalRecorded"
        )
        self.assertLess(player_delete, player_destruction_guard)
        self.assertLess(player_destruction_guard, backend_removed)
        self.assertLess(backend_removed, backend_completion_guard)
        pause_request = source.index(
            'u"qt-media-capture-pause-requested"'
        )
        pause_call = source.index("m_player->pause()", pause_request)
        paused_state = source.index(
            "state == QMediaPlayer::PausedState"
        )
        capture_ready = source.index(
            'u"qt-media-capture-ready"',
            paused_state,
        )
        seek_observed = source.index('u"qt-media-seek-observed"')
        resume_requested = source.index(
            'u"qt-media-resume-requested"',
            seek_observed,
        )
        resume_call = source.index("m_player->play()", resume_requested)
        resumed_state = source.index(
            'u"qt-media-playback-resumed"',
        )
        resume_position_guard = source.index(
            "maximumSeekTargetErrorMilliseconds",
            source.index(
                "m_resumePositionMilliseconds = m_player->position()"
            ),
        )
        self.assertLess(pause_request, pause_call)
        self.assertLess(paused_state, capture_ready)
        self.assertLess(seek_observed, resume_requested)
        self.assertLess(resume_requested, resume_call)
        self.assertLess(resume_position_guard, resumed_state)
        self.assertIn("VideoOutput", qml)
        self.assertIn("videoOutput.videoSink", qml)
        self.assertIn("attachMediaVideoSink", qml)
        state = self._read_required("src/ProbeState.cpp")
        observation = state.index(
            "m_mediaProbe->armDeviceObservation(m_runtimeRunNonce)"
        )
        network = state.index(
            "m_networkProbe->start(m_runtimeRunNonce)",
            observation,
        )
        activation = state.index(
            "void ProbeState::beginUserActivatedProbes()"
        )
        media_start = state.index(
            "m_mediaProbe->start(m_runtimeRunNonce)",
            activation,
        )
        self.assertLess(observation, network)
        self.assertLess(network, activation)
        self.assertLess(activation, media_start)

    def test_prebuild_runtime_commands_have_one_bounded_typed_schema(
        self,
    ) -> None:
        header = self._read_required("src/BrowserRuntimeBridge.h")
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        state_header = self._read_required("src/ProbeState.h")
        state = self._read_required("src/ProbeState.cpp")
        media = self._read_required("src/MediaProbe.cpp")

        for marker in (
            "enum class BrowserRuntimeCommand",
            "maximumRuntimeCommandNameBytes",
            "maximumRuntimeCommandPayloadBytes",
            "maximumMediaCaptureRequestIdBytes",
            "parseRuntimeCommand",
            "validateRuntimeCommandPayload",
            "BrowserRuntimeCommand command",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, header + bridge + state_header + state)

        command_bound = bridge.index(
            "commandUtf8.size() > maximumRuntimeCommandNameBytes"
        )
        command_conversion = bridge.index(
            "QString::fromUtf8",
            command_bound,
        )
        payload_bound = bridge.index(
            "payloadUtf8.size() > maximumRuntimeCommandPayloadBytes"
        )
        payload_conversion = bridge.index("QByteArray payloadBytes")
        self.assertLess(command_bound, command_conversion)
        self.assertLess(payload_bound, payload_conversion)
        self.assertIn("switch (command)", state)
        self.assertNotRegex(
            state,
            r'command\s*[!=]=\s*u"',
        )
        self.assertNotIn("requestIdPattern", media)

    def test_prebuild_report_commits_state_before_browser_reentrancy(
        self,
    ) -> None:
        header = self._read_required("src/BrowserRuntimeBridge.h")
        report_header = self._read_required("src/Gate1bReport.h")
        report = self._read_required("src/Gate1bReport.cpp")
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")

        append = report[
            report.index("void Gate1bReport::append"):
            report.index("void Gate1bReport::pass")
        ]
        self.assertLess(
            append.index("m_nextSequence++"),
            append.index("publishGate1bEvent"),
        )
        resolve = report[
            report.index("void Gate1bReport::resolveReady"):
            report.index("bool Gate1bReport::isTerminal")
        ]
        self.assertLess(
            resolve.index("m_readyResolved = true"),
            resolve.index("publishGate1bSnapshot"),
        )
        self.assertIn("maximumFailureRecords", report_header)
        self.assertIn("m_failures.size() < maximumFailureRecords", report)
        for signature in (
            "[[nodiscard]] bool publishGate1bEvent",
            "[[nodiscard]] bool publishGate1bSnapshot",
            "[[nodiscard]] bool resolveGate1bReady",
        ):
            with self.subTest(signature=signature):
                self.assertIn(signature, header)
        for function_name, next_name in (
            ("publishGate1bEvent", "publishGate1bSnapshot"),
            ("publishGate1bSnapshot", "resolveGate1bReady"),
            ("resolveGate1bReady", "rejectGate1bReady"),
        ):
            body = bridge[
                bridge.index(f"bool {function_name}"):
                bridge.index(f"bool {next_name}")
                if next_name != "rejectGate1bReady"
                else bridge.index("void rejectGate1bReady")
            ]
            self.assertNotIn("rejectGate1bReady", body)

    def test_prebuild_render_capture_is_inactive_fast_and_bounded(
        self,
    ) -> None:
        header = self._read_required("src/RenderProbe.h")
        source = self._read_required("src/RenderProbe.cpp")
        record = source[
            source.index("void RenderProbe::recordRenderFrame"):
            source.index("void RenderProbe::consumeLatestFrame")
        ]
        inactive = record.index("CaptureKind::None")
        self.assertLess(inactive, record.index("rendererInterface()"))
        self.assertLess(inactive, record.index("emscripten_webgl"))
        for marker in (
            "CaptureKind::None",
            "CaptureKind::PostMain",
            "CaptureKind::Phase",
            "captureDeadline",
            "FailureCallback",
            "captureTimedOut",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, header + source)
        for forbidden in (
            "CaptureKind::none",
            "CaptureKind::postMain",
            "CaptureKind::phase",
        ):
            self.assertNotIn(forbidden, header + source)

    def test_prebuild_network_and_media_retention_are_bounded(
        self,
    ) -> None:
        network_header = self._read_required("src/NetworkProbe.h")
        network = self._read_required("src/NetworkProbe.cpp")
        media_header = self._read_required("src/MediaProbe.h")
        media = self._read_required("src/MediaProbe.cpp")

        text_handler = network[
            network.index("void NetworkProbe::webSocketTextMessage"):
            network.index("void NetworkProbe::webSocketBinaryMessage")
        ]
        self.assertEqual(text_handler.count("message.toUtf8()"), 1)
        for marker in (
            "std::chrono::seconds",
            "sslErrors",
            "retireQnamReply",
            "maximumPendingWebSocketEvents",
            "std::move(event.payload)",
        ):
            with self.subTest(network_marker=marker):
                self.assertIn(marker, network_header + network)
        self.assertRegex(
            network,
            (
                r"m_pendingWebSocketEvents\.size\(\)\s*"
                r">=\s*maximumPendingWebSocketEvents"
            ),
        )
        for marker in (
            "maximumFramePositionSamples",
            "m_framePositionSamples.replace",
            "std::chrono::seconds",
            "std::chrono::milliseconds",
        ):
            with self.subTest(media_marker=marker):
                self.assertIn(marker, media_header + media)

    def test_prebuild_explicit_worker_uses_the_named_nonce_transform(
        self,
    ) -> None:
        state = self._read_required("src/ProbeState.cpp")
        self.assertEqual(state.count("0xA5A55A5A"), 1)
        self.assertGreaterEqual(state.count("explicitNonceXor"), 3)

    def test_residual_native_fallback_and_val_boundary_are_ordered(
        self,
    ) -> None:
        bridge = self._read_required("src/BrowserRuntimeBridge.cpp")
        jspi = self._read_required("src/JspiNestedLoopProbe.cpp")
        boundary = bridge[
            bridge.index("std::string rhythmGameGate1bCommand("):
            bridge.index("EM_JS(unsigned, rgGate1bBrowserCapabilityBits")
        ]
        self.assertIn("emscripten::val commandValue", boundary)
        self.assertIn("emscripten::val payloadValue", boundary)
        self.assertNotIn("const std::string &commandUtf8", boundary)
        first_conversion = boundary.index(".as<std::string>()")
        for marker in (
            "commandValue.isString()",
            "payloadValue.isString()",
            'commandValue["length"]',
            'payloadValue["length"]',
            "maximumRuntimeCommandNameBytes",
            "maximumRuntimeCommandPayloadBytes",
        ):
            with self.subTest(marker=marker):
                self.assertLess(boundary.index(marker), first_conversion)
        typed_catch = boundary.index(
            "catch (const std::exception &error)"
        )
        fallback_catch = boundary.index("catch (...)")
        self.assertLess(first_conversion, typed_catch)
        self.assertLess(typed_catch, fallback_catch)
        for marker in (
            "encodeRuntimeCommandSuccess(reply)",
            "normalizedRuntimeCommandErrorCode(error.what())",
            "runtime-command-native-failure",
        ):
            with self.subTest(result_boundary_marker=marker):
                self.assertIn(marker, boundary)
        for marker in (
            "std::string encodeRuntimeCommandSuccess",
            "std::string encodeRuntimeCommandFailure",
            '{QStringLiteral("ok"), true}',
            '{QStringLiteral("reply"), reply}',
            '{QStringLiteral("error"), errorCode}',
            '{QStringLiteral("ok"), false}',
        ):
            with self.subTest(result_envelope_marker=marker):
                self.assertIn(marker, bridge)

        start = jspi[
            jspi.index(
                "void JspiNestedLoopProbe::startExclusiveSuspendGuardProbe"
            ):
            jspi.index(
                "void JspiNestedLoopProbe::finishExclusiveSuspendGuardProbe"
            )
        ]
        self.assertLess(
            start.index("m_exclusiveFinalizationWatchdog->start("),
            start.index("scheduleExclusiveSuspendGuardProbe("),
        )
        self.assertIn("exclusiveNativeFinalizationDeadline", jspi)
        self.assertIn("m_exclusiveFinalizationWatchdog->stop()", start)


if __name__ == "__main__":
    unittest.main()
