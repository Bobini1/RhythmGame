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
NEW_TRACKED_INPUTS = (
    "tools/wasm-probe/browser/fixtures/probe.webm",
    "tools/wasm-probe/browser/fixtures/README.md",
    "tools/wasm-probe/browser/lib/browser-matrix.mjs",
    "tools/wasm-probe/browser/package-lock.json",
    "tools/wasm-probe/browser/package.json",
    "tools/wasm-probe/browser/playwright.config.mjs",
    "tools/wasm-probe/browser/run-browser-tool.mjs",
    "tools/wasm-probe/browser/server/artifact-manifest.mjs",
    "tools/wasm-probe/browser/server/policy.mjs",
    "tools/wasm-probe/browser/server/probe-server.mjs",
    "tools/wasm-probe/browser/server/probe-server.test.mjs",
    "tools/wasm-probe/browser/tests/browser-matrix.test.mjs",
    "tools/wasm-probe/browser/web/bootstrap.mjs",
    "tools/wasm-probe/browser/web/preflight-worker.mjs",
    "tools/wasm-probe/browser/web/probe.css",
    "tools/wasm-probe/browser/web/RhythmGameWasmProbe.html.in",
    "tools/wasm-probe/scripts/package_runtime_artifacts.py",
    "tools/wasm-probe/tests/test_gate1b_source_contract.py",
    "tools/wasm-probe/tests/test_package_runtime_artifacts.py",
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
        self.assertRegex(config, r"\bretries\s*:\s*0\b")
        self.assertRegex(config, r"\bworkers\s*:\s*1\b")
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
            "fullscreen=(self), gamepad=(self), hid=(self)",
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
            'type: "qt-style-adopted"',
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
            "report.instance = instance",
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
        adoption_event = bootstrap.find(
            'type: "qt-style-adopted"',
            fingerprint_check,
        )
        self.assertGreater(fingerprint_check, 0)
        self.assertGreater(adoption_event, fingerprint_check)

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
        self.assertEqual(
            template.count('<link rel="icon" href="data:,">'),
            1,
        )
        self.assertNotIn("favicon", template.casefold())

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


if __name__ == "__main__":
    unittest.main()
