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
    "tools/wasm-probe/browser/lib/browser-matrix.mjs",
    "tools/wasm-probe/browser/package-lock.json",
    "tools/wasm-probe/browser/package.json",
    "tools/wasm-probe/browser/playwright.config.mjs",
    "tools/wasm-probe/browser/run-browser-tool.mjs",
    "tools/wasm-probe/browser/tests/browser-matrix.test.mjs",
    "tools/wasm-probe/tests/test_gate1b_source_contract.py",
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


if __name__ == "__main__":
    unittest.main()
