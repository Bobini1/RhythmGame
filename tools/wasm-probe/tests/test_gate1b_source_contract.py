from __future__ import annotations

import json
import re
import subprocess
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
PROBE = REPO / "tools" / "wasm-probe"
BROWSER = PROBE / "browser"

DIRECT_DEPENDENCIES = {
    "@playwright/test": "1.62.0",
    "pngjs": "7.0.0",
    "selfsigned": "5.5.0",
    "ws": "8.21.1",
}
BLOCKING_BROWSER_LANES = (
    ("chromium-cft", "chromium"),
    ("chrome-stable", "chrome"),
    ("chrome-beta", "chrome-beta"),
)
TASK3_TRACKED_INPUTS = (
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
NEW_TRACKED_INPUTS = (
    "tools/wasm-probe/browser/fixtures/probe.webm",
    "tools/wasm-probe/browser/fixtures/README.md",
    "tools/wasm-probe/browser/lib/browser-matrix.mjs",
    "tools/wasm-probe/browser/lib/chromium-lifecycle-policy.mjs",
    "tools/wasm-probe/browser/lib/external-lifecycle-browser.mjs",
    "tools/wasm-probe/browser/package-lock.json",
    "tools/wasm-probe/browser/package.json",
    "tools/wasm-probe/browser/playwright.config.mjs",
    "tools/wasm-probe/browser/run-browser-tool.mjs",
    "tools/wasm-probe/browser/server/artifact-manifest.mjs",
    "tools/wasm-probe/browser/server/policy.mjs",
    "tools/wasm-probe/browser/server/probe-server.mjs",
    "tools/wasm-probe/browser/server/probe-server.test.mjs",
    "tools/wasm-probe/browser/tests/browser-matrix.test.mjs",
    "tools/wasm-probe/browser/tests/task4-adversarial.test.mjs",
    "tools/wasm-probe/browser/web/bootstrap.mjs",
    "tools/wasm-probe/browser/web/preflight-worker.mjs",
    "tools/wasm-probe/browser/web/probe.css",
    "tools/wasm-probe/browser/web/RhythmGameWasmProbe.html.in",
    "tools/wasm-probe/scripts/package_runtime_artifacts.py",
    "tools/wasm-probe/tests/test_gate1b_source_contract.py",
    "tools/wasm-probe/tests/test_package_runtime_artifacts.py",
    *TASK3_TRACKED_INPUTS,
)


class Gate1BSourceContractTest(unittest.TestCase):
    def _read_required(self, path: Path) -> str:
        self.assertTrue(
            path.is_file(),
            f"Gate 1B browser harness file is missing: {path}",
        )
        return path.read_text("utf-8")

    def test_browser_package_and_lock_use_exact_direct_versions(self) -> None:
        package = json.loads(
            self._read_required(BROWSER / "package.json")
        )
        package_direct = {
            **package.get("dependencies", {}),
            **package.get("devDependencies", {}),
        }
        self.assertEqual(package_direct, DIRECT_DEPENDENCIES)

        lock = json.loads(
            self._read_required(BROWSER / "package-lock.json")
        )
        self.assertEqual(lock["lockfileVersion"], 3)
        lock_root = lock["packages"][""]
        lock_direct = {
            **lock_root.get("dependencies", {}),
            **lock_root.get("devDependencies", {}),
        }
        self.assertEqual(lock_direct, DIRECT_DEPENDENCIES)
        for name, version in DIRECT_DEPENDENCIES.items():
            with self.subTest(package=name):
                self.assertEqual(
                    lock["packages"][f"node_modules/{name}"]["version"],
                    version,
                )

    def test_playwright_matrix_is_blocking_and_has_no_bypass(self) -> None:
        config = self._read_required(
            BROWSER / "playwright.config.mjs"
        )
        lifecycle_policy = self._read_required(
            BROWSER / "lib" / "chromium-lifecycle-policy.mjs"
        )
        self.assertRegex(config, r"\bretries\s*:\s*0\b")
        self.assertRegex(config, r"\bworkers\s*:\s*1\b")
        self.assertRegex(
            config,
            r'testMatch\s*:\s*["\']\*\*/\*\.spec\.mjs["\']',
        )
        for project, channel in BLOCKING_BROWSER_LANES:
            with self.subTest(project=project):
                self.assertRegex(
                    config,
                    rf"\bname\s*:\s*[\"']{re.escape(project)}[\"']",
                )
                self.assertRegex(
                    config,
                    rf"\bchannel\s*:\s*[\"']{re.escape(channel)}[\"']",
                )
        for forbidden in (
            "--enable-features=SharedArrayBuffer",
            "--enable-features=WebAssemblyJSPromiseIntegration",
            "--disable-web-security",
            "--autoplay-policy=no-user-gesture-required",
            "bypassCSP",
        ):
            self.assertNotIn(forbidden, config)

        def frozen_arguments(export_name: str) -> tuple[str, ...]:
            match = re.search(
                (
                    rf"export const {re.escape(export_name)}\s*="
                    r"\s*Object\.freeze\(\[(?P<body>.*?)\]\);"
                ),
                lifecycle_policy,
                re.DOTALL,
            )
            self.assertIsNotNone(match)
            return tuple(
                re.findall(r'"(--[^"]+)"', match.group("body"))
            )

        self.assertEqual(
            frozen_arguments("bfcacheIgnoredDefaultArguments"),
            ("--disable-back-forward-cache",),
        )
        self.assertEqual(
            frozen_arguments("browserInspectionArguments"),
            ("--enable-automation",),
        )
        self.assertEqual(
            frozen_arguments("headedLifecycleIgnoredDefaultArguments"),
            (
                "--disable-background-timer-throttling",
                "--disable-backgrounding-occluded-windows",
                "--disable-back-forward-cache",
                "--disable-renderer-backgrounding",
            ),
        )
        self.assertEqual(config.count("ignoreDefaultArgs:"), 3)
        self.assertEqual(config.count("args: browserInspectionArguments"), 3)
        self.assertNotRegex(
            config,
            r"use\s*:\s*\{\s*args\s*:\s*browserInspectionArguments",
        )
        self.assertEqual(
            len(re.findall(
                (
                    r"launchOptions\s*:\s*\{\s*"
                    r"args\s*:\s*browserInspectionArguments\s*,\s*"
                    r"ignoreDefaultArgs\s*:"
                ),
                config,
            )),
            3,
        )
        self.assertEqual(
            config.count("bfcacheIgnoredDefaultArguments"),
            2,
        )
        self.assertEqual(
            config.count("headedLifecycleIgnoredDefaultArguments"),
            3,
        )
        self.assertRegex(
            config,
            (
                r'name:\s*"chromium-cft",\s*'
                r"grepInvert:\s*/@headed/"
            ),
        )
        for marker in (
            "Browser.getBrowserCommandLine",
            "auditLifecycleArguments",
            "requiredAbsent",
            "verifiedVia",
        ):
            with self.subTest(lifecycle_audit_marker=marker):
                self.assertIn(
                    marker,
                    (
                        lifecycle_policy
                        + self._read_required(
                            BROWSER / "tests" / "gate1b.spec.mjs"
                        )
                    ),
                )

    def test_task3_core_spec_is_discoverable_and_uses_only_cft_focus(
        self,
    ) -> None:
        config = self._read_required(
            BROWSER / "playwright.config.mjs"
        )
        spec_path = BROWSER / "tests" / "gate1b.spec.mjs"
        spec = self._read_required(spec_path)
        self.assertRegex(config, r'testDir\s*:\s*["\']\./tests["\']')
        self.assertEqual(spec_path.parent, BROWSER / "tests")
        self.assertIn('test("@core ', spec)
        self.assertIn('name: "chromium-cft"', config)
        self.assertIn('channel: "chromium"', config)
        self.assertNotIn("test.skip", spec)
        self.assertNotIn("test.fixme", spec)

    def test_task3_bootstrap_validates_the_shared_report_schema(self) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        for marker in (
            "gate1bEventKeys",
            "monotonicMicroseconds",
            "sequence",
            "payload",
            "gate1bSnapshotKeys",
            "cycleSummary",
            "authority",
            "validateGate1bEvent",
            "validateGate1bSnapshot",
            "appendValidatedGate1bEvent",
            "appendBrowserTerminalEvent",
            '"terminal-failure"',
            "gate1bEventLog.length",
            "readyResolutionRecord",
            "readyResolution:",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)
        self.assertRegex(
            bootstrap,
            r"event\.sequence\s*!==\s*gate1bEventLog\.length",
        )
        self.assertRegex(
            bootstrap,
            r"sequence\s*:\s*gate1bEventLog\.length",
        )
        self.assertRegex(
            bootstrap,
            r"monotonicMicroseconds\s*:",
        )
        self.assertEqual(bootstrap.count("gate1bEventLog.push("), 1)
        self.assertIn("gate1bEventLog.push(event)", bootstrap)
        self.assertNotIn("report.events.push(", bootstrap)
        ready_record = bootstrap.find(
            "readyResolutionRecord = cloneAndFreezeJson({"
        )
        ready_resolve = bootstrap.find(
            "resolveReady(publishedGate1bSnapshot)",
            ready_record,
        )
        self.assertGreater(ready_record, 0)
        self.assertGreater(ready_resolve, ready_record)

    def test_task3_bootstrap_terminal_event_has_the_exact_schema(
        self,
    ) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        terminal = re.search(
            (
                r"function\s+appendBrowserTerminalEvent\s*"
                r"\([^)]*\)\s*\{(?P<body>.*?)\n\}"
            ),
            bootstrap,
            re.DOTALL,
        )
        self.assertIsNotNone(terminal)
        body = terminal.group("body")
        for marker in (
            "sequence: gate1bEventLog.length",
            "monotonicMicroseconds:",
            'type: "terminal-failure"',
            "payload:",
            "validateGate1bEvent",
            "appendValidatedGate1bEvent",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, body)

    def test_task3_bootstrap_keeps_style_adoption_out_of_events(self) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        for marker in (
            "let styleAdoptionRecord = null",
            "styleAdoption:",
            "return styleAdoptionRecord",
            "styleAdoptionRecord = Object.freeze({",
            "configurable: false",
            "adoptionCount: 1",
            "inlineStyleCount: 0",
            "stylesheetCount: 1",
            "bytes: bytes.byteLength",
            "ruleCount: sheet.cssRules.length",
            "sha256: actual.hex",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)
        self.assertNotIn('type: "qt-style-adopted"', bootstrap)

    def test_task3_bootstrap_has_only_fixed_cpp_command_ingress(self) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        ordinary_match = re.search(
            (
                r"const ordinaryRuntimeCommandNames = Object\.freeze\(\["
                r"(?P<body>.*?)"
                r"\]\);"
            ),
            bootstrap,
            re.DOTALL,
        )
        self.assertIsNotNone(ordinary_match)
        self.assertEqual(
            re.findall(r'"([^"]+)"', ordinary_match.group("body")),
            [
                "ack-media-frame-capture",
                "arm-bfcache-resume-probe",
                "arm-hidden-timer-probe",
                "arm-visible-resume-timer-probe",
                "begin-foreground-latency-sampling",
                "probe-ping",
                "set-shader-phase",
            ],
        )
        adversarial_match = re.search(
            (
                r"const adversarialRuntimeCommandModes = Object\.freeze\(\{"
                r"(?P<body>.*?)"
                r"\}\);"
            ),
            bootstrap,
            re.DOTALL,
        )
        self.assertIsNotNone(adversarial_match)
        self.assertEqual(
            re.findall(
                r'"([^"]+)":\s*"([^"]+)"',
                adversarial_match.group("body"),
            ),
            [
                (
                    "trigger-native-depth-limit",
                    "native-depth-limit",
                ),
                (
                    "trigger-native-suspension-trap",
                    "native-suspension-trap",
                ),
            ],
        )
        for marker in (
            '"probe-ping"',
            '"set-shader-phase"',
            "rhythmGameGate1bCommand",
            "retainRuntimeCommandAuthority(instance)",
            "retainedRuntimeCommand === null",
            "Reflect.deleteProperty(instance, "
            '"rhythmGameGate1bCommand")',
            '"rhythmGameGate1bCommand" in instance',
            "descriptor.value.bind(instance)",
            "JSON.stringify(payload)",
            "function parseRuntimeCommandResult(encodedResult)",
            '["ok", "reply"]',
            '["error", "ok"]',
            "runtime-command-boundary-threw",
            "runtime-command-result-invalid",
            "runtime-command-boundary-audit-mutated-report",
            "gate1b-command-in-flight",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)
        self.assertRegex(
            bootstrap,
            (
                r"retainedRuntimeCommand"
                r"\(\s*name\s*,\s*encodedPayload\s*,\s*\)"
            ),
        )
        self.assertIn(
            "const encodedPayload = JSON.stringify(payload);",
            bootstrap,
        )
        for forbidden in (
            "report.instance[name]",
            "instance[name]",
            "Module[name]",
            "globalThis[name]",
            "report.instance",
            "eval(",
            "new Function",
        ):
            self.assertNotIn(forbidden, bootstrap)

    def test_task3_bootstrap_bounds_the_retained_qt_event_pump(
        self,
    ) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        for marker in (
            "installQtEventPump",
            "Object.getOwnPropertyDescriptor(",
            '"qtSendPendingEvents"',
            '"qtSendPendingApplicationEvents"',
            (
                "nativeDescriptor?.value "
                "!== instance.qtSendPendingEvents"
            ),
            (
                "applicationDescriptor?.value\n"
                "            !== instance.qtSendPendingApplicationEvents"
            ),
            "instance.qtSendPendingEvents.bind(instance)",
            "instance.qtSendPendingApplicationEvents.bind(instance)",
            "directQtSendPendingEvents()",
            "directQtSendPendingApplicationEvents()",
            'typeof pending.then !== "function"',
            "qtScreenNativeDispatchEventTypes",
            "qtDocumentNativeDispatchEventTypes",
            '"beforeinput"',
            '"click"',
            '"compositionend"',
            '"compositionstart"',
            '"compositionupdate"',
            '"contextmenu"',
            '"copy"',
            '"cut"',
            '"dblclick"',
            '"dragend"',
            '"dragenter"',
            '"dragleave"',
            '"dragover"',
            '"dragstart"',
            '"drop"',
            '"input"',
            '"keydown"',
            '"keyup"',
            '"paste"',
            '"pointercancel"',
            '"pointerdown"',
            '"pointerenter"',
            '"pointerleave"',
            '"pointermove"',
            "primaryPumpInFlight",
            "pumpsInFlight",
            "maxSynchronousNativePumpDepth = 4",
            "nativeDepthLimitAttemptEventType",
            '"rhythmgame-gate1b-native-depth-attempt"',
            "const currentPump = new Promise",
            "startPumpCall",
            "drained = qtSendPendingEvents();",
            "const pending = qtSendPendingApplicationEvents();",
            "whenIdle",
            "requestAnimationFrame",
            "cancelAnimationFrame",
            '"pointerup"',
            '"wheel"',
            'void kick("input")',
            "qtEventPump.kick(",
            "commandKicks",
            "idleFrames",
            "inputKicks",
            "maxConcurrentCalls",
            "maxNativeDispatchDepth",
            "nativeDispatchDepthLimit",
            "nativeDispatchDepth",
            "nonBubblingInputKicks",
            "reentrantInputCalls",
            "exclusiveDeferrals",
            "fullCycleDeferrals",
            "idleFramesEnabled",
            "quiesceForTerminalProbe",
            "terminalProbeCommand",
            "windowWakeTargets",
            "eventPump",
            "qtEventPump?.stop",
            "MutationObserver",
            "qtWindowWakeTargets",
            "attachQtWindowWakeTarget",
            "detachQtWindowWakeTarget",
            '"runtime-native-event-pump-depth-limit"',
            '"runtime-native-event-pump-failed"',
            "removeEventListener",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)
        self.assertIn(
            'if (reason === "input" && pumpsInFlight.size !== 0)',
            bootstrap,
        )
        self.assertIn("return startPumpCall(false, reason)", bootstrap)
        self.assertIn(
            "return primaryPumpInFlight ?? whenIdle()",
            bootstrap,
        )
        self.assertIn("return startPumpCall(true, reason)", bootstrap)
        self.assertNotIn("queueMicrotask", bootstrap)
        observer_start = bootstrap.find(
            "qtWindowObserver.observe(qtShadowRoot"
        )
        initial_window_scan = bootstrap.find(
            (
                "for (const windowElement of "
                'qtShadowRoot.querySelectorAll(".qt-window"))'
            ),
            observer_start,
        )
        self.assertGreater(observer_start, 0)
        self.assertGreater(initial_window_scan, observer_start)
        self.assertNotIn(
            "qtWindowWakeTargets.size === 0",
            bootstrap,
        )
        self.assertNotIn(
            '"runtime-event-pump-window-target"',
            bootstrap,
        )
        self.assertIn(
            "an empty scan has no attachment gap",
            bootstrap,
        )
        input_handler = bootstrap.find(
            "const onNativeDispatch = (event) =>"
        )
        direct_kick = bootstrap.find(
            'void kick("input")',
            input_handler,
        )
        input_listener = bootstrap.find(
            (
                "for (const eventType of "
                "qtScreenNativeDispatchEventTypes)"
            ),
            direct_kick,
        )
        self.assertGreater(input_handler, 0)
        self.assertGreater(direct_kick, input_handler)
        self.assertGreater(input_listener, direct_kick)
        document_listener = bootstrap.find(
            (
                "for (const eventType of "
                "qtDocumentNativeDispatchEventTypes)"
            ),
            input_listener,
        )
        self.assertGreater(document_listener, input_listener)
        self.assertIn(
            "ownerDocument.addEventListener(eventType, onNativeDispatch)",
            bootstrap[document_listener:],
        )
        self.assertIn(
            (
                "Chromium installs Qt's clipboard listeners on document. "
                "Registering\n"
                "    // here, after qtLoad()"
            ),
            bootstrap,
        )
        pump_function = bootstrap.find(
            "function startPumpCall(primary, reason)"
        )
        pump_placeholder = bootstrap.find(
            "const currentPump = new Promise",
            pump_function,
        )
        direct_pump_call = bootstrap.find(
            'if (reason === "input")',
            pump_placeholder,
        )
        self.assertGreater(pump_function, 0)
        self.assertGreater(pump_placeholder, pump_function)
        self.assertGreater(direct_pump_call, pump_placeholder)
        pump_function_end = bootstrap.find(
            "\n    function whenIdle()",
            direct_pump_call,
        )
        self.assertGreater(pump_function_end, direct_pump_call)
        pump_body = bootstrap[pump_function:pump_function_end]
        self.assertIn(
            (
                "nativeDispatchDepth += 1;\n"
                "                telemetry.maxNativeDispatchDepth"
            ),
            pump_body,
        )
        native_call = pump_body.index("drained = qtSendPendingEvents();")
        native_failure = pump_body.index(
            '"runtime-native-event-pump-failed"',
            native_call,
        )
        native_finally = pump_body.index("} finally {", native_failure)
        native_depth_release = pump_body.index(
            "nativeDispatchDepth -= 1;",
            native_finally,
        )
        input_telemetry = pump_body.index(
            "telemetry.exclusiveDeferrals += 1;",
            native_depth_release,
        )
        input_complete = pump_body.index(
            "complete(false);",
            input_telemetry,
        )
        full_pump = pump_body.index(
            "const pending = qtSendPendingApplicationEvents();",
            input_complete,
        )
        full_telemetry = pump_body.index(
            "telemetry.fullCycleDeferrals += 1;",
            full_pump,
        )
        full_complete = pump_body.index(
            "complete(false);",
            full_telemetry,
        )
        self.assertLess(native_call, native_failure)
        self.assertLess(native_failure, native_finally)
        self.assertLess(native_finally, native_depth_release)
        self.assertLess(native_depth_release, input_telemetry)
        self.assertLess(input_telemetry, input_complete)
        self.assertLess(input_complete, full_pump)
        self.assertLess(full_pump, full_telemetry)
        self.assertLess(full_telemetry, full_complete)
        self.assertNotIn(
            "fail(",
            pump_body[input_telemetry:full_pump],
        )
        self.assertNotIn(
            "fail(",
            pump_body[full_telemetry:full_complete],
        )
        complete_start = pump_body.index("const complete = (result) =>")
        complete_end = pump_body.index(
            "\n\n        try {",
            complete_start,
        )
        complete_body = pump_body[complete_start:complete_end]
        self.assertIn(
            "if (active) {\n                scheduleIdleFrame();",
            complete_body,
        )
        kick_function = bootstrap.find(
            "function kick(reason)",
            direct_pump_call,
        )
        self.assertGreater(kick_function, direct_pump_call)
        retained = bootstrap.find("retainRuntimeCommandAuthority(instance)")
        installed = bootstrap.find(
            "installQtEventPump",
            retained,
        )
        self.assertGreater(retained, 0)
        self.assertGreater(installed, retained)
        command_idle = bootstrap.find(
            "await qtEventPump.whenIdle()",
            installed,
        )
        runtime_command = bootstrap.find(
            "retainedRuntimeCommand(",
            command_idle,
        )
        command_quiesce = bootstrap.find(
            "qtEventPump.quiesceForTerminalProbe()",
            command_idle,
        )
        command_kick = bootstrap.find(
            "qtEventPump.kick(",
            runtime_command,
        )
        self.assertGreater(command_idle, installed)
        self.assertGreater(command_quiesce, command_idle)
        self.assertGreater(runtime_command, command_idle)
        self.assertLess(command_quiesce, runtime_command)
        self.assertGreater(command_kick, runtime_command)
        self.assertIn(
            'qtEventPump?.stop("runtime-abort")',
            bootstrap,
        )
        self.assertGreaterEqual(
            bootstrap.count('qtEventPump?.stop("runtime-exit")'),
            2,
        )
        self.assertNotIn('instance["qtSendPendingEvents"]', bootstrap)
        self.assertNotIn(
            'instance["qtSendPendingApplicationEvents"]',
            bootstrap,
        )
        self.assertNotIn("instance[name]", bootstrap)

    def test_task3_runtime_proves_cycle_order_latency_and_lifecycle(
        self,
    ) -> None:
        bootstrap = self._read_required(BROWSER / "web" / "bootstrap.mjs")
        external_lifecycle = self._read_required(
            BROWSER / "lib" / "external-lifecycle-browser.mjs"
        )
        state = self._read_required(PROBE / "src" / "ProbeState.cpp")
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )

        for marker in (
            "class ApplicationCycleOrderProbe final",
            "QCoreApplication::postEvent(",
            "jsEventHandlerAt(m_nativeHandler)",
            'recordPhase(u"native")',
            'recordPhase(u"timer")',
            "QEvent::DeferredDelete",
            "m_deferredDeleteReceiver->deleteLater()",
            "QTimer::singleShot(0, this, [this]",
            '"qt-application-cycle-order"',
            "timer->setTimerType(Qt::PreciseTimer)",
            "timer->start(0)",
        ):
            with self.subTest(source_marker=marker):
                self.assertIn(marker, state)
        for phase in (
            'QStringLiteral("posted")',
            'QStringLiteral("native")',
            'QStringLiteral("timer")',
            'QStringLiteral("deferred-delete")',
        ):
            self.assertIn(phase, state)

        for marker in (
            "foregroundInputSampleTarget = 64",
            "hiddenIdleFallbackMilliseconds = 50",
            "applicationCyclePumpSerial",
            "activeApplicationPump",
            '"application-cycle-pump-owner-mismatch"',
            "bfcacheResumePumpSerial",
            '"bfcache-resume-pump-owner-mismatch"',
            "telemetry.foregroundInputLatencyMilliseconds",
            "foregroundTimerPumpSerials",
            '"foreground-qt-timer-pump-owner-mismatch"',
            "foregroundInputEventObserver",
            "hiddenTimerEventObserver",
            'ownerDocument.visibilityState === "hidden"',
            'kick("hidden-idle-timer")',
            "event.persisted === true",
            'globalThis.addEventListener("pageshow", onPageShow)',
            '"visibilitychange"',
            "bfcacheResumeProbe",
            "bfcacheNavigationArmed",
            "bfcacheExpectationStorageKey",
            '"bfcache-restoration-missed"',
            "hiddenQtTimerSentinels",
            "hiddenQtTimerPumpSerial",
            "resumedQtTimerPumpSerial",
            "resumedQtTimerSentinels",
            '"hidden-qt-timer-pump-owner-mismatch"',
            '"resumed-qt-timer-pump-owner-mismatch"',
            '"hidden-timer-probe-requires-hidden-page"',
            '"visible-resume-timer-probe-requires-visible-page"',
        ):
            with self.subTest(bootstrap_marker=marker):
                self.assertIn(marker, bootstrap)

        for marker in (
            "exerciseForegroundPumpLatency",
            "exercisePersistedPageLifecycle",
            "exerciseHiddenPageFallback",
            "FOREGROUND_INPUT_LATENCY_BUDGET_MS = 8",
            "FOREGROUND_INPUT_SAMPLE_COUNT = 64",
            "FOREGROUND_TIMER_INTERVAL_MS = 5",
            "FOREGROUND_TIMER_LATENESS_BUDGET_MS = 34",
            "FOREGROUND_TIMER_SAMPLE_COUNT = 32",
            "HIDDEN_TIMER_INTERVAL_MS = 125",
            "VISIBLE_RESUME_TIMER_INTERVAL_MS = 125",
            '"qt-application-cycle-order"',
            "foregroundInputLatencyMilliseconds",
            "nearestRankP95",
            "page.mouse.move(",
            '"qt-foreground-input-delivery"',
            '"qt-foreground-timer-delivery"',
            '"qt-hidden-timer-delivery"',
            '"qt-hidden-timer-sentinel"',
            '"qt-visible-resume-timer-delivery"',
            '"qt-visible-resume-timer-sentinel"',
            "/probe/bfcache-away",
            "page.goBack(",
            "[...globalThis.__rhythmGameGate1b.events]",
            "bfcacheRestores",
            "hiddenIdleTimers",
            "hiddenQtTimerSentinels",
            "lifecyclePaused",
            "postHiddenBaseline",
            "visibilityBaseline + 1",
            "visibilityReschedules",
        ):
            with self.subTest(spec_marker=marker):
                self.assertIn(marker, spec)
        self.assertNotIn(
            (
                "events: structuredClone(\n"
                "                globalThis.__rhythmGameGate1b.events"
            ),
            spec,
        )
        for marker in (
            "launchExternalLifecycleBrowser",
            "DevToolsActivePort",
            "chromium.connectOverCDP",
            "noDefaults: true",
            "Security.setIgnoreCertificateErrors",
            "certificateSession = await context.newCDPSession(page)",
            "await certificateSession.detach().catch",
            "windowsHide: false",
            "shell: false",
            "temporary-external-cdp",
            "auditLifecycleArguments",
            "assertNoAcceptanceBypass",
            "assertOwnedProfile",
            "Browser.getBrowserCommandLine",
            "verifyPinnedNoDefaultsContract",
            "playwrightCoreBundleSha256",
            "3258d1cf334c6afc95f22aa9c292436c",
        ):
            with self.subTest(external_lifecycle_marker=marker):
                self.assertIn(marker, external_lifecycle)
        for marker in (
            '"@core @headed proves real hidden and visible Chrome delivery"',
            "launchExternalLifecycleBrowser(",
            "gate1b-headed-lifecycle-provenance",
            "external.lifecycleArgumentAudit",
            "await exerciseHiddenPageFallback(page)",
        ):
            with self.subTest(headed_spec_marker=marker):
                self.assertIn(marker, spec)
        for forbidden in (
            "Page.setWebLifecycleState",
            "Emulation.setPageVisibilityOverride",
            "new PageTransitionEvent",
        ):
            with self.subTest(synthetic_lifecycle=forbidden):
                self.assertNotIn(
                    forbidden,
                    external_lifecycle + spec,
                )
        self.assertNotIn("--ignore-certificate-errors", external_lifecycle)
        cleanup_start = external_lifecycle.find(
            "const cleanup = async () => {"
        )
        cleanup_end = external_lifecycle.find(
            "\n    };",
            cleanup_start,
        )
        certificate_detach = external_lifecycle.find(
            "await certificateSession.detach().catch",
            cleanup_start,
        )
        certificate_create = external_lifecycle.find(
            "certificateSession = await context.newCDPSession(page)"
        )
        certificate_override = external_lifecycle.find(
            '"Security.setIgnoreCertificateErrors"',
            certificate_create,
        )
        returned_browser = external_lifecycle.find(
            "return Object.freeze({",
            certificate_override,
        )
        self.assertGreater(cleanup_start, 0)
        self.assertGreater(cleanup_end, cleanup_start)
        self.assertGreater(certificate_detach, cleanup_start)
        self.assertLess(certificate_detach, cleanup_end)
        self.assertGreater(certificate_create, cleanup_end)
        self.assertGreater(certificate_override, certificate_create)
        self.assertGreater(returned_browser, certificate_override)
        self.assertNotIn(
            "certificateSession.detach",
            external_lifecycle[certificate_create:returned_browser],
        )
        self.assertNotIn("new PageTransitionEvent", spec)
        self.assertNotIn("foregroundTimerLatencyP95Milliseconds", spec)

    def test_task3_core_spec_owns_diagnostics_and_provenance(self) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        main_test = spec.find(
            'test("@core @network @media '
            'executes the post-main Qt/Wasm runtime contract"'
        )
        self.assertGreater(main_test, 0)
        navigation = spec.find(
            "const navigation = await page.goto(`${probeServer.origin}/`",
            main_test,
        )
        self.assertGreater(navigation, main_test)
        for listener in (
            'page.on("console"',
            'page.on("pageerror"',
            'page.on("requestfailed"',
            'page.on("response"',
            'page.on("crash"',
            "page.addInitScript",
            "securitypolicyviolation",
        ):
            with self.subTest(listener=listener):
                listener_offset = spec.find(listener, main_test)
                self.assertGreater(listener_offset, main_test)
                self.assertLess(listener_offset, navigation)
        for marker in (
            "startProbeServer",
            "probeServer.close()",
            "runtimeLeaf",
            "build",
            "wasm-release",
            "runtime",
            "ignoreHTTPSErrors: true",
            "certificateTrustValidated: false",
            "browser.version()",
            "testInfo.project.name",
            "manifest.buildId",
            "computeCurrentInputBuildId",
            "input-manifest.txt",
            "expectedRuntimeBuildId",
            "actualRuntimeBuildId",
            "stale Gate 1B runtime",
            "probeServer.requestLogs",
            "gate1b-core-provenance",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        expected_digest = spec.find(
            "expectedRuntimeBuildId = await computeCurrentInputBuildId()"
        )
        runtime_manifest = spec.find(
            "actualRuntimeBuildId = runtimeManifest.buildId",
            expected_digest,
        )
        server_start = spec.find(
            "probeServer = await startProbeServer",
            runtime_manifest,
        )
        self.assertGreater(expected_digest, 0)
        self.assertGreater(runtime_manifest, expected_digest)
        self.assertGreater(server_start, runtime_manifest)
        for forbidden in (
            "bypassCSP",
            "--ignore-certificate-errors",
            "waitForTimeout",
            "locator.click(",
            "Input.dispatchMouseEvent",
            "new Function",
        ):
            self.assertNotIn(forbidden, spec)
        self.assertEqual(spec.count(".dispatchEvent("), 4)
        self.assertEqual(
            spec.count(
                "mediaDevices.dispatchEvent(event)"
            ),
            1,
        )
        self.assertIn(
            "document.dispatchEvent(clipboardEvent)",
            spec,
        )
        self.assertNotIn("new PageTransitionEvent", spec)
        self.assertIn("page.goBack({ waitUntil: \"commit\" })", spec)
        self.assertEqual(spec.count("test.setTimeout("), 8)
        self.assertNotRegex(
            spec,
            r"(?:^|[^A-Za-z0-9_$])Function\s*\(",
        )

    def test_task3_core_waits_surface_runtime_failure_immediately(
        self,
    ) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            "waitForReportOutcome",
            "page.waitForFunction",
            '"terminal-failure"',
            "report.snapshot?.failures",
            "FAILURE_EVENT_TAIL_LENGTH = 8",
            "eventTail",
            "frozenFailure",
            "frozenEventTail",
            "Object.freeze(",
            "Task 3 runtime/diagnostic failure",
            "createDiagnosticFailureLatch",
            "diagnosticFailure.promise",
            '"diagnostic-failure"',
            'diagnosticFailure.fail("console"',
            'diagnosticFailure.fail("pageerror"',
            'diagnosticFailure.fail("requestfailed"',
            'diagnosticFailure.fail("http"',
            'diagnosticFailure.fail("crash"',
            'diagnosticFailure.fail("csp"',
            "__rhythmGameGate1bReadyObservation",
            "readyResolution",
            "resolvedValue",
            "resolvedValueIsDeepFrozen",
            '"ready"',
            '"core-checks"',
            '"jspi-suspended"',
            '"pump-idle"',
            '"render-generation"',
            '"user-activation"',
            '"media-device-browser-enumeration"',
            '"media-device-settled"',
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        self.assertEqual(
            spec.count("await waitForReportOutcome("),
            16,
        )
        self.assertNotIn("expect.poll", spec)
        self.assertNotIn(
            "() => globalThis.__rhythmGameGate1b.ready",
            spec,
        )

    def test_task3_core_spec_proves_report_state_and_command_are_fixed(
        self,
    ) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            "REPORT_FIXED_METHODS",
            "REPORT_READ_ONLY_FIELDS",
            "REPORT_OWN_KEYS",
            "REPORT_ENUMERABLE_KEYS",
            "inspectReadOnlyReportAuthority",
            "verifyRetainedRuntimeCommand",
            "Object.isFrozen(report)",
            "isDeepFrozen",
            "eventPumpDescriptorIsGetter",
            "eventEntriesAreDeepFrozen",
            "snapshotIsDeepFrozen",
            "styleAdoptionIsDeepFrozen",
            "readyResolutionIsDeepFrozen",
            "reportEnumerableKeysAreExact",
            "reportOwnKeysAreExact",
            "schemaVersion: report.schemaVersion",
            "Object.getOwnPropertyDescriptor(",
            'Reflect.set(\n                    report,\n                    "instance"',
            "eventPushRejected",
            "eventPreventExtensionsRejected",
            "globalIsFixed",
            "methodDescriptorsAreFixed",
            "fieldsAreReadOnly",
            "reportReferenceUnchanged",
            "instanceAbsent",
            "structuredClone([...report.events])",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        self.assertIn(
            'await report.command("probe-ping", {})',
            spec,
        )
        self.assertIn(
            "expect(boundCommand.instanceAbsent).toBe(true)",
            spec,
        )

    def test_task3_core_spec_validates_schema_authority_and_ordering(
        self,
    ) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            "EVENT_KEYS",
            "FRAME_PAYLOAD_KEYS",
            "SNAPSHOT_KEYS",
            "CYCLE_SUMMARY_KEYS",
            "AUTHORITY_FIELDS",
            "CAPABILITY_FIELDS",
            "gate1bTechnicalPassed",
            "gate0Satisfied",
            "formalGate1EntryAuthorized",
            "gate1Passed",
            "productionPortAuthorized",
            "Task 3 has no authority to set",
            "event.sequence",
            "event.monotonicMicroseconds",
            "previousMicros",
            "toBeGreaterThan(\n            previousMicros",
            "main-returning",
            "post-main-tick",
            "post-main-application-state",
            "POST_MAIN_APPLICATION_STATE_KEYS",
            "validatePostMainApplicationState",
            "qml-root-attached",
            "command-acknowledged",
            "probe-ping",
            "post-main",
            "cycleSummary.status",
            '"not-started"',
            "cycleSummary.completed",
            "STYLE_ADOPTION_KEYS",
            "styleAdoption",
            "adoptionCount: 1",
            "inlineStyleCount: 0",
            "stylesheetCount: 1",
            "bytes: 5_238",
            "ruleCount: 37",
            "qt-shadow-container",
            "6b7168686da79590ea116889998716dfa",
            "EVENT_PUMP_KEYS",
            "eventPump",
            "commandKicks",
            "idleFrames",
            "inputKicks",
            "maxConcurrentCalls",
            "maxNativeDispatchDepth",
            "nativeDispatchDepthLimit",
            "nonBubblingInputKicks",
            "reentrantInputCalls",
            "exclusiveDeferrals",
            "fullCycleDeferrals",
            "windowWakeTargets",
            "CORE_TEST_TIMEOUT_MS",
            "test.setTimeout(CORE_TEST_TIMEOUT_MS)",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        self.assertIn("const CORE_TEST_TIMEOUT_MS = 120_000;", spec)
        self.assertRegex(
            spec,
            (
                r"expect\(postMainIndex\)"
                r"\.toBeGreaterThan\(mainReturningIndex\)"
            ),
        )
        self.assertRegex(
            spec,
            (
                r"expect\(pingIndex\)"
                r"\.toBeGreaterThan\(mainReturningIndex\)"
            ),
        )
        self.assertRegex(
            spec,
            (
                r"expect\(pingIndex\)"
                r"\.toBeLessThan\(postMainIndex\)"
            ),
        )

    def test_task3_core_spec_requires_real_threads_jspi_and_qt_render(
        self,
    ) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            '"explicit-pthread"',
            '"qt-concurrent"',
            '"jspi-nested-loop"',
            '"qt-exclusive-suspend-guard"',
            '"qt-render-webgl2"',
            "mainThreadId",
            "threadId",
            "new Set(threadIdentities).size",
            "PTHREAD_NONCE_XOR",
            "concurrent.result",
            "promiseResolvedWhileExec",
            "quitDelivered",
            "postLoopSentinel",
            "watchdogTimedOut",
            "JSPI_EVENTS",
            'graphicsApi).toBe("OpenGL")',
            "contextHandle",
            "contextAttributesResult",
            "majorVersion).toBe(2)",
            "postMainFrameCount",
            "const postMainFrames = validateRenderFrames",
            "const finalPostMainPayload = postMainFrames[1].payload",
            "expect(render.postMainFrameCount).toBe(2)",
            "phaseFrameContracts",
            "2 + (2 * phaseFrameContracts.length)",
            "for (const contract of phaseFrameContracts)",
            "validateJspiEvents",
            '"qt-render-frame"',
            "explicitReadyMicroseconds",
            "explicitCompletedMicroseconds",
            "qtConcurrentStartedMicroseconds",
            "qtConcurrentObservedReadyMicroseconds",
            "qtConcurrentReleaseMicroseconds",
            "lifecycleMicros",
            "exclusiveGuard.guardObservations",
            "foreignDrainResults: [false, false]",
            "exclusiveDrainCount: 1",
            "completionDrainCount: 1",
            "completionFinalizedAfterOwnerReturn: true",
            "normalDrainArmed: true",
            "ownerResumedByExclusive: true",
            "exclusiveDomDispatch: true",
            "queuePreserved: true",
            'deliveryOrder: [\n'
            '            "exclusive",\n'
            '            "first",\n'
            '            "second",\n'
            '            "completion",',
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        self.assertEqual(spec.count("validateJspiEvents("), 3)
        self.assertNotIn('getContext("webgl2")', spec)
        self.assertNotIn("document.createElement", spec)

    def test_task3_core_spec_uses_trusted_click_and_exact_pixels(self) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            "viewport: { height: 360, width: 640 }",
            "deviceScaleFactor: 1",
            'colorScheme: "dark"',
            "BUTTON_POINT",
            "page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y)",
            '"user-activation-sampled"',
            "payload.active === true",
            "activationBaseline",
            "waitForJspiSuspension",
            '"jspi-suspended"',
            "activationBaseline.jspiPromiseResolvedCount",
            "activationBaseline.pumpInFlight",
            "sequenceBaseline",
            "globalThis.__rhythmGameGate1b.events.at(-1).sequence",
            "verifyContextMenuSuppression",
            "__gate1bContextMenuObservations",
            'button: "right"',
            "defaultPrevented: true",
            "isTrusted: true",
            "consumeTransientUserActivation",
            '"gate1b-activation-consumer"',
            "activeAfter: false",
            "activeBefore: true",
            "popupOpened: true",
            "activeAfter: navigator.userActivation.isActive",
            "verifyNonBubblingWindowWakeups",
            "nonBubblingInputKickDelta: 2",
            "reentrantInputCallDelta: 2",
            'new PointerEvent(type, {',
            "bubbles: false",
            "verifyClipboardDispatchTiming",
            'new ClipboardEvent("paste"',
            "gate1b-clipboard-timing",
            "inputKickDelta: 1",
            "reentrantInputCallDelta: 1",
            "dispatchReturned: false",
            "inFlightBefore: true",
            "isTrusted: false",
            "finalJspiEvents[1].sequence",
            "finalJspiEvents[2].sequence",
            "inspectQtCanvasCoverage",
            "newCDPSession",
            '"DOM.getNodeForLocation"',
            "ignorePointerEventsNone: true",
            '"DOM.describeNode"',
            "hit.backendNodeId === canvasBackendNodeId",
            "compositorHitIsExactCanvas",
            "canvasPointerEvents",
            "PHASES = Object.freeze([0.20, 0.80])",
            '"set-shader-phase"',
            "frameBaseline",
            "generation",
            "CAPTURE_RECT",
            "SAMPLE_COORDINATES",
            "PNG.sync.read",
            "PIXEL_TOLERANCE = 4",
            "0.10 + 0.50 * phase",
            "0.20 + 0.25 * phase",
            "0.80 - 0.50 * phase",
            "Math.round(1.00 * 255)",
            'createHash("sha256")',
            "captures[0].hash",
            "captures[1].hash",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        activation_consumed = spec.index(
            "expect(await consumeTransientUserActivation(page))"
        )
        primary_click = spec.index(
            "await page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y)",
            activation_consumed,
        )
        self.assertNotIn(
            "page.evaluate",
            spec[activation_consumed:primary_click],
        )
        self.assertNotIn("elementFromPoint", spec)

    def test_task3_core_spec_inspects_the_emitted_jspi_import_shape(
        self,
    ) -> None:
        spec = self._read_required(
            BROWSER / "tests" / "gate1b.spec.mjs"
        )
        for marker in (
            "OWNED_ASYNC_IMPORT",
            '"__asyncjs__rgGate1bAwaitOwnedNonce"',
            "WebAssembly.compile",
            "WebAssembly.Module.imports",
            "manifest.artifacts.mainJs.url",
            "manifest.artifacts.wasm.url",
            "Asyncify.handleAsync",
            "importPatternOwnsAsyncJsInWrapper",
            "importPattern.test(x)",
            "instrumentWasmImports(imports){",
            "},instrumentWasmExports",
            "ownedFunctionUsesPromiseRace",
            "ownedFunctionOwnsResumeWatchdog",
            "ownedFunctionRequiresTrustedPointer",
            "ownedFunctionCancelsOnTerminal",
            "report.ready",
            ".catch",
            "cancelOwnedImport",
            "capture:true",
            "event.button!==0",
            "jspi-resume-watchdog-timeout",
            "jspi-after-exec",
            "postLoopSentinel",
            "#screen",
            "pointerup",
            "isTrusted",
            "new WebAssembly.Suspending(original)",
            "ownedImports",
            'kind: "function"',
            'module: "env"',
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)

    def test_dispatcher_and_browser_identity_exports_are_locked(self) -> None:
        dispatcher = self._read_required(
            BROWSER / "run-browser-tool.mjs"
        )
        for command in (
            "npm-lock",
            "npm-ci",
            "install-chromium",
            "install-chrome-beta",
            "node-test",
            "playwright-test",
            "qualify",
            "fsa",
        ):
            self.assertIn(f'"{command}"', dispatcher)
        for required in (
            "process.execPath",
            "process.version",
            "v20.18.0",
            "10.8.2",
            "PLAYWRIGHT_BROWSERS_PATH",
            "shell: false",
        ):
            self.assertIn(required, dispatcher)
        self.assertNotIn("shell: true", dispatcher)

        matrix = self._read_required(
            BROWSER / "lib" / "browser-matrix.mjs"
        )
        for exported in (
            "blockingBrowserLanes",
            "resolveBrowserLane",
            "describeBrowser",
            "assertNoAcceptanceBypass",
        ):
            self.assertRegex(matrix, rf"\bexport\b[^;]*\b{exported}\b")
        for project, _ in BLOCKING_BROWSER_LANES:
            self.assertIn(f'"{project}"', matrix)
        self.assertIn("sha256", matrix.casefold())

    def test_generated_browser_outputs_are_ignored_but_lock_is_not(self) -> None:
        ignored = (
            ".toolchains/playwright/",
            "tools/wasm-probe/browser/node_modules/",
            "tools/wasm-probe/browser/.profiles/",
            "tools/wasm-probe/browser/.traces/",
            "tools/wasm-probe/browser/playwright-report/",
            "tools/wasm-probe/browser/.certificates/",
            "tools/wasm-probe/browser/.native-chooser-runs/",
        )
        for relative in ignored:
            with self.subTest(ignored=relative):
                result = subprocess.run(
                    [
                        "git",
                        "check-ignore",
                        "--no-index",
                        "--quiet",
                        relative,
                    ],
                    cwd=REPO,
                    check=False,
                )
                self.assertEqual(result.returncode, 0, relative)

        lock = subprocess.run(
            [
                "git",
                "check-ignore",
                "--no-index",
                "--quiet",
                "tools/wasm-probe/browser/package-lock.json",
            ],
            cwd=REPO,
            check=False,
        )
        self.assertNotEqual(lock.returncode, 0)

    def test_manifests_are_unique_sorted_and_cover_new_inputs(self) -> None:
        input_entries = (
            PROBE / "input-manifest.txt"
        ).read_text("utf-8").splitlines()
        control_entries = (
            PROBE / "build-control-manifest.txt"
        ).read_text("utf-8").splitlines()
        for label, entries in (
            ("input", input_entries),
            ("build control", control_entries),
        ):
            with self.subTest(manifest=label):
                self.assertEqual(
                    len(entries),
                    len({entry.casefold() for entry in entries}),
                )
                self.assertEqual(entries, sorted(entries, key=str.casefold))
        for relative in NEW_TRACKED_INPUTS:
            with self.subTest(input=relative):
                self.assertEqual(input_entries.count(relative), 1)

    def test_strict_origin_sources_freeze_policy_and_negative_modes(self) -> None:
        policy = self._read_required(
            BROWSER / "server" / "policy.mjs"
        )
        server = self._read_required(
            BROWSER / "server" / "probe-server.mjs"
        )
        for marker in (
            "Cross-Origin-Opener-Policy",
            "same-origin",
            "Cross-Origin-Embedder-Policy",
            "require-corp",
            "Cross-Origin-Resource-Policy",
            "X-Content-Type-Options",
            "nosniff",
            "Referrer-Policy",
            "no-referrer",
            "Permissions-Policy",
            "fullscreen=(self), gamepad=(self), hid=(self), unload=()",
            "'wasm-unsafe-eval'",
            "worker-src 'self'",
            "wss://127.0.0.1:",
        ):
            self.assertIn(marker, policy)
        for forbidden in ("'unsafe-eval'", "'unsafe-inline'", "blob:"):
            self.assertNotIn(forbidden, policy)
        for mode in (
            "missing-coop",
            "missing-coep",
            "wrong-wasm-mime",
            "missing-wasm-unsafe-eval",
            "blocked-worker-src",
            "corrupt-bootstrap",
            "corrupt-main-js",
            "corrupt-wasm",
            "corrupt-qtloader",
            "native-depth-limit",
            "native-suspension-trap",
        ):
            self.assertIn(f'"{mode}"', policy + server)
        self.assertIn("https.createServer", server)
        self.assertIn("noServer: true", server)
        self.assertIn('"127.0.0.1"', server)
        self.assertNotIn("express", server.casefold())

    def test_bootstrap_owns_preflight_and_negative_terminal_codes(self) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        main_index = bootstrap.find("mainJs")
        loader_index = bootstrap.find("qtloader")
        qt_load_index = bootstrap.find("qtLoad(")
        self.assertGreater(main_index, 0)
        self.assertGreater(loader_index, main_index)
        self.assertGreater(qt_load_index, loader_index)
        for marker in (
            "globalThis.__rhythmGameGate1b",
            'cache: "no-store"',
            "crypto.subtle.digest",
            "isSecureContext",
            "crossOriginIsolated",
            "SharedArrayBuffer",
            "WebAssembly.Suspending",
            "WebAssembly.promising",
            "webgl2",
            "AudioWorklet",
            "new Worker",
            "worker.terminate()",
            "new WebSocket",
            "window.RhythmGameWasmProbe_entry",
            "locateFile",
            "onExit",
            "unhandledrejection",
            "securitypolicyviolation",
        ):
            self.assertIn(marker, bootstrap)
        for terminal_code in (
            "policy-coop-missing",
            "policy-coep-missing",
            "artifact-wasm-mime",
            "policy-wasm-eval-missing",
            "preflight-worker-csp-blocked",
            "sri-main-js-rejected",
            "artifact-wasm-digest",
            "artifact-bootstrap-mime",
            "artifact-bootstrap-bytes",
            "sri-qtloader-rejected",
        ):
            self.assertIn(terminal_code, bootstrap)
        self.assertRegex(
            bootstrap,
            (
                r"bootstrapResponse\.contentType\s*"
                r"!==\s*bootstrapArtifact\.mime"
            ),
        )
        self.assertRegex(
            bootstrap,
            (
                r"bootstrapResponse\.bytes\.byteLength\s*"
                r"!==\s*bootstrapArtifact\.bytes"
            ),
        )
        self.assertNotIn("eval(", bootstrap)
        self.assertNotIn("new Function", bootstrap)

    def test_blocked_worker_csp_delta_is_owned_only_by_html_audit(self) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        self.assertIn("function expectedCspFor(role)", bootstrap)
        self.assertRegex(
            bootstrap,
            (
                r'negativeMode === "blocked-worker-src"\s*'
                r'&& role === "html"'
            ),
        )
        self.assertIn(
            'replace("worker-src \'self\'", "worker-src \'none\'")',
            bootstrap,
        )

    def test_qt_shadow_style_adapter_is_exact_and_lifecycle_owned(
        self,
    ) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        policy = self._read_required(
            BROWSER / "server" / "policy.mjs"
        )

        for marker in (
            "6b7168686da79590ea116889998716dfa"
            "624e1467411daa2bffee066a867d53e",
            "5238",
            "37",
            '".qt-screen"',
            '".qt-window"',
            '"qt-shadow-container"',
            "new TextEncoder()",
            "new CSSStyleSheet()",
            "replaceSync",
            "adoptedStyleSheets",
            "ShadowRoot.prototype",
            "HTMLStyleElement",
            "Reflect.apply(originalAppendChild",
            "Object.getOwnPropertyDescriptor",
            "Object.defineProperty",
            "originalOwnDescriptor",
            "installedOwnDescriptor",
            "Reflect.deleteProperty",
            "setInterval",
            "styleAdoptionRecord = Object.freeze({",
            "single initial Qt container",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)

        for terminal_code in (
            "preflight-constructable-stylesheet",
            "qt-style-shape",
            "qt-style-fingerprint",
            "qt-style-duplicate-root",
            "qt-style-initial-missing",
            "qt-style-adapter-ownership-loss",
        ):
            self.assertIn(terminal_code, bootstrap)

        for structural_check in (
            r"child\s+instanceof\s+HTMLStyleElement",
            r"root\.host\.id\s+===\s+qtShadowHostId",
            r"root\.host\.shadowRoot\s+===\s+root",
            r"root\.ownerDocument\s+===\s+screen\.ownerDocument",
            r"child\.ownerDocument\s+===\s+screen\.ownerDocument",
            r"root\.host\.parentNode\s+===\s+screen",
            r"child\.parentNode\s+===\s+null",
            r"!child\.isConnected",
            (
                r"Reflect\.apply\("
                r"originalAppendChild,\s*root,\s*arguments"
            ),
            r"text\.startsWith\(\"\\n\"\)",
            r"foldedText\.includes\(\"@import\"\)",
            r"foldedText\.includes\(\"url\(\"\)",
            r"sheet\.replaceSync\(text\)",
            r"sheet\.cssRules\.length\s+!==\s+qtStyleRuleCount",
            r"root\.querySelectorAll\(\"style\"\)\.length\s+!==\s+0",
            r"adoptionCount\s+!==\s+1",
            (
                r"samePropertyDescriptor\("
                r"descriptor,\s*installedOwnDescriptor"
            ),
        ):
            with self.subTest(structural_check=structural_check):
                self.assertRegex(bootstrap, structural_check)

        install = bootstrap.find(
            "const qtStyleAdapter = installQtShadowStyleAdapter(screen);"
        )
        qt_load = bootstrap.find("await window.qtLoad(", install)
        initial_check = bootstrap.find(
            "await qtStyleAdapter.requireInitialAdoption()",
            qt_load,
        )
        publish_instance = bootstrap.find(
            "retainRuntimeCommandAuthority(instance)",
            initial_check,
        )
        self.assertGreater(install, 0)
        self.assertGreater(qt_load, install)
        self.assertGreater(initial_check, qt_load)
        self.assertGreater(publish_instance, initial_check)
        self.assertNotIn("await ", bootstrap[install:qt_load])
        self.assertIn(
            'qtStyleAdapter.restore("qtLoad-rejection")',
            bootstrap[qt_load:initial_check],
        )
        self.assertIn(
            'qtStyleAdapter.restore("qt-style-initial-rejection")',
            bootstrap[initial_check:publish_instance],
        )
        self.assertEqual(
            bootstrap.count("qtStyleAdapter.restoreAndFail("),
            3,
        )

        fingerprint_check = bootstrap.find(
            "actual.hex !== qtStyleSha256"
        )
        adoption_record = bootstrap.find(
            "styleAdoptionRecord = Object.freeze({",
            fingerprint_check,
        )
        self.assertGreater(fingerprint_check, 0)
        self.assertGreater(adoption_record, fingerprint_check)

        adapter_start = bootstrap.find("function adapterAppendChild(child)")
        duplicate_guard = bootstrap.find(
            "adoptionCount !== 0 || adoptedRoots.has(root)",
            adapter_start,
        )
        stylesheet_construction = bootstrap.find(
            "new CSSStyleSheet()",
            adapter_start,
        )
        self.assertGreater(adapter_start, 0)
        self.assertGreater(duplicate_guard, adapter_start)
        self.assertGreater(stylesheet_construction, duplicate_guard)

        self.assertNotIn("Node.prototype", bootstrap)
        self.assertNotIn("Document.prototype.createElement", bootstrap)
        self.assertNotIn("HTMLElement.prototype", bootstrap)
        self.assertNotIn("instance.qtAddContainerElement", bootstrap)
        self.assertNotIn("instance.qtRemoveContainerElement", bootstrap)
        self.assertNotIn("'unsafe-inline'", policy)
        self.assertNotRegex(policy, r"'nonce-[^']+'")

    def test_packager_and_fixture_provenance_are_source_bound(self) -> None:
        packager = self._read_required(
            PROBE / "scripts" / "package_runtime_artifacts.py"
        )
        provenance = self._read_required(
            BROWSER / "fixtures" / "README.md"
        )
        for marker in (
            "hashlib.sha256",
            "sort_keys=True",
            'separators=(",", ":")',
            "RhythmGameWasmProbe.aw.js",
            "RhythmGameWasmProbe.ww.js",
            "unexpected generated runtime asset",
            "dynamic code execution",
            "buildId",
        ):
            self.assertIn(marker, packager)
        for marker in (
            "ffmpeg",
            "64x64",
            "2",
            "VP8",
            "Opus",
            "SHA-256",
            "repository-owned",
        ):
            self.assertIn(marker, provenance)

    def test_runtime_shell_suppresses_implicit_favicon_fetch(self) -> None:
        template = self._read_required(
            BROWSER / "web" / "RhythmGameWasmProbe.html.in"
        )
        server = self._read_required(BROWSER / "server" / "probe-server.mjs")
        self.assertEqual(
            template.count('<link rel="icon" href="data:,">'),
            1,
        )
        self.assertNotIn("favicon", template.casefold())
        away_route = re.search(
            (
                r'if \(route === "/probe/bfcache-away"\) \{'
                r".*?"
                r"\n\s+return;"
            ),
            server,
            re.DOTALL,
        )
        self.assertIsNotNone(away_route)
        self.assertEqual(
            away_route.group(0).count(
                r'<link rel=\"icon\" href=\"data:,\">'
            ),
            1,
        )

    def test_runtime_package_validates_real_shape_and_has_read_only_verifier(
        self,
    ) -> None:
        cmake = self._read_required(PROBE / "CMakeLists.txt")
        self.assertRegex(
            cmake,
            (
                r"--generated-shape-dir\s+"
                r'"\$\{CMAKE_CURRENT_BINARY_DIR\}"'
            ),
        )
        verifier = re.search(
            (
                r"add_custom_target\("
                r"RhythmGameWasmProbeRuntimeVerify(?P<body>.*?)\n\)"
            ),
            cmake,
            flags=re.DOTALL,
        )
        self.assertIsNotNone(verifier)
        self.assertNotRegex(verifier.group("body"), r"\bALL\b")
        self.assertIn("--verify-output-dir", verifier.group("body"))
        self.assertIn("--expected-build-id", verifier.group("body"))
        self.assertRegex(
            cmake,
            (
                r"add_dependencies\(\s*"
                r"RhythmGameWasmProbeRuntimeVerify\s+"
                r"RhythmGameWasmProbeRuntimePackage"
            ),
        )
        for stale_shell in (
            "RhythmGameWasmProbe.html",
            "qtloader.js",
            "qtlogo.svg",
        ):
            self.assertIn(stale_shell, cmake)
        self.assertIn("file(REMOVE", cmake)

    def test_task4_server_owns_fixed_media_alias_and_probe_log_identity(
        self,
    ) -> None:
        manifest = self._read_required(
            BROWSER / "server" / "artifact-manifest.mjs"
        )
        server = self._read_required(BROWSER / "server" / "probe-server.mjs")
        for marker in (
            '"/fixtures/probe.webm"',
            "artifact.bytes",
            "artifact.sha256",
            "artifact.sri",
            "sha256Sri(bytes)",
            '"cache-control": noStoreCache',
            "probeLogs",
            "requestId",
            "connectionId",
            "runNonce",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, manifest + server)
        self.assertNotIn(
            'path.join(browserDirectory, "fixtures"',
            server,
        )

    def test_task4_acceptance_is_nonce_correlated_and_orders_media_teardown(
        self,
    ) -> None:
        spec = self._read_required(BROWSER / "tests" / "gate1b.spec.mjs")
        bootstrap = self._read_required(BROWSER / "web" / "bootstrap.mjs")
        for marker in (
            "@network",
            "@media",
            "@media-devices",
            "probeServer.probeLogs",
            "qt-media-capture-ready",
            "qt-media-capture-pause-requested",
            "qt-media-resume-requested",
            "qt-media-playback-resumed",
            "qt-media-device-snapshot",
            "ack-media-frame-capture",
            "qt-media-natural-end",
            "qt-media-element-resource-released",
            "qt-media-backend-removed",
            "qt-media-player-destroyed",
            "qt-media-player-output-created",
            "qt-media-post-seek-video-frame",
            "qt-media-audio-output-destroyed",
            "callCountImmediatelyAfterDispatch",
            "secondEnumerateStartedAfterDispatchMilliseconds",
            "audioInputSignalCount",
            "audioOutputSignalCount",
            "maxOccurrences: 1",
            "nativeWebSocketEvents",
            "payload.connectionId === wss.connectionId",
            "ADVERSARIAL_MEDIA_CAPTURE_HOLD_MS = 2_500",
            "holdMilliseconds",
            "element?.paused",
            "terminalFailureCount",
            "currentTimeMilliseconds",
            "durationMilliseconds",
            "resumePositionMilliseconds",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, spec)
        for marker in (
            "ack-media-frame-capture",
            "matchingElements.length !== 1",
            "record.element.src === expectedSource.href",
            "record.element.currentSrc === expectedSource.href",
            "state.ownedRecord = matchingElements[0]",
            "const armRemoval = (runNonce) =>",
            "observerMutatedElement: false",
            "hadResourceBeforeDestruction",
            "record.element.isConnected === false",
            "mediaElementResourceReleased",
            "const release = (runNonce) =>",
            "HTMLMediaElement.NETWORK_EMPTY",
            "HTMLMediaElement.HAVE_NOTHING",
            "Number.isNaN(element.duration)",
            'querySelectorAll("source")',
            'addEventListener("seeking"',
            'addEventListener("seeked"',
        ):
            with self.subTest(bootstrap_marker=marker):
                self.assertIn(marker, bootstrap)
        tracker_start = bootstrap.index(
            "function createMediaBackendTracker()"
        )
        tracker_end = bootstrap.index(
            "const mediaBackendTracker = createMediaBackendTracker();",
            tracker_start,
        )
        tracker = bootstrap[tracker_start:tracker_end]
        for forbidden in (
            'removeAttribute("src")',
            "element.load()",
            "element.remove()",
        ):
            with self.subTest(tracker_mutation=forbidden):
                self.assertNotIn(forbidden, tracker)
        self.assertNotRegex(
            tracker,
            r"element\.srcObject\s*=(?!=)",
        )

    def test_task4_media_device_backport_has_browser_adversaries(
        self,
    ) -> None:
        spec = self._read_required(BROWSER / "tests" / "gate1b.spec.mjs")
        for marker in (
            "installMediaDeviceScenario",
            "__gate1bMediaDeviceScenario",
            'mode: "unavailable"',
            'mode: "controlled"',
            "initialDelayMilliseconds: 125",
            "initialDelayTurns: 4",
            '"audioinput"',
            '"audiooutput"',
            'new Event("devicechange")',
            "dispatchSyntheticOutputlessChange",
            "holdFirstEnumeration: true",
            "releaseFirstEnumeration",
            "callCountImmediatelyAfterDispatch: 1",
            "deviceChangeDispatches: 2",
            "coalesces rapid changes while enumeration is pending",
            "firstDelayElapsedMilliseconds",
            "enumerateResolvedCount",
            "qt-media-device-snapshot",
            "qt-media-device-batch-settled",
            "waitForStableMediaDeviceSettlement",
            "beforeActivation",
            "const mediaDevices = new EventTarget()",
            "isTrusted: event.isTrusted",
            'type: "warning"',
            "No media devices found",
            "changedInputHash",
            "initialOutputHash",
        ):
            with self.subTest(media_device_marker=marker):
                self.assertIn(marker, spec)
        self.assertIn(
            (
                "/allow_blocking_on_main_thread|"
                "blocking on the main thread|futex/i"
            ),
            spec,
        )

    def test_prebuild_browser_regresses_runtime_command_input_bounds(
        self,
    ) -> None:
        bootstrap = self._read_required(
            BROWSER / "web" / "bootstrap.mjs"
        )
        for marker in (
            "const boundaryCases = [",
            '"x".repeat(65)',
            '" ".repeat(4097)',
            '"1".repeat(80)',
            "runtime-command-name-too-large",
            "runtime-command-payload-too-large",
            "ack-media-frame-capture-request-id",
            "runtime-command-boundary-audit-threw",
            "runtime-command-boundary-audit-mismatch",
            "runtime-command-boundary-audit-mutated-report",
            "eventCountBefore",
            "snapshotBefore",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, bootstrap)


if __name__ == "__main__":
    unittest.main()
