from __future__ import annotations

import base64
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROBE = Path(__file__).resolve().parents[1]
PACKAGER = PROBE / "scripts" / "package_runtime_artifacts.py"
HTML_TEMPLATE = PROBE / "browser" / "web" / "RhythmGameWasmProbe.html.in"
BUILD_ID = "8" * 64


class RuntimePackagerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.generated = self.root / "generated"
        self.generated_shape = self.generated
        self.expected_audio_worklet_occurrences = "1"
        self.expected_wasm_worker_occurrences = "1"
        self.output = self.root / "runtime"
        self.generated.mkdir()
        self.sources = self.root / "sources"
        self.sources.mkdir()

        (self.generated / "RhythmGameWasmProbe.js").write_text(
            'const aw = "RhythmGameWasmProbe.aw.js";\n'
            'const ww = "RhythmGameWasmProbe.ww.js";\n'
            'const wasm = "RhythmGameWasmProbe.wasm";\n',
            encoding="utf-8",
        )
        (self.generated / "RhythmGameWasmProbe.wasm").write_bytes(
            b"\x00asm\x01\x00\x00\x00"
        )
        (self.generated / "RhythmGameWasmProbe.aw.js").write_text(
            "globalThis.audioWorkletFixture = true;\n",
            encoding="utf-8",
        )
        (self.generated / "RhythmGameWasmProbe.ww.js").write_text(
            "globalThis.wasmWorkerFixture = true;\n",
            encoding="utf-8",
        )
        (self.sources / "probe.css").write_text(
            "#screen { inline-size: 64px; }\n",
            encoding="utf-8",
        )
        (self.sources / "bootstrap.mjs").write_text(
            "globalThis.__bootstrapFixture = true;\n",
            encoding="utf-8",
        )
        (self.sources / "preflight-worker.mjs").write_text(
            "self.onmessage = ({data}) => postMessage(data);\n",
            encoding="utf-8",
        )
        (self.sources / "qtloader.js").write_text(
            "globalThis.qtLoad = async () => ({});\n",
            encoding="utf-8",
        )
        (self.sources / "probe.webm").write_bytes(
            b"\x1aE\xdf\xa3synthetic-webm"
        )
        self.template = self.sources / "RhythmGameWasmProbe.html.in"
        self.template.write_text(
            "<!doctype html>\n"
            '<link rel="icon" href="data:,">\n'
            '<link rel="stylesheet" href="@PROBE_CSS_URL@" '
            'integrity="@PROBE_CSS_SRI@" crossorigin="anonymous">\n'
            '<main id="screen"></main>\n'
            '<script type="module" src="@PROBE_BOOTSTRAP_URL@" '
            'integrity="@PROBE_BOOTSTRAP_SRI@" '
            'crossorigin="anonymous"></script>\n',
            encoding="utf-8",
        )
        self.css = self.sources / "probe.css"
        self.bootstrap = self.sources / "bootstrap.mjs"
        self.preflight_worker = self.sources / "preflight-worker.mjs"
        self.qtloader = self.sources / "qtloader.js"
        self.media = self.sources / "probe.webm"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def command(self, *extra: str) -> list[str]:
        return [
            sys.executable,
            "-I",
            "-B",
            str(PACKAGER),
            "--build-id",
            BUILD_ID,
            "--generated-dir",
            str(self.generated),
            "--generated-shape-dir",
            str(self.generated_shape),
            "--output-dir",
            str(self.output),
            "--html-template",
            str(self.template),
            "--css",
            str(self.css),
            "--bootstrap",
            str(self.bootstrap),
            "--preflight-worker",
            str(self.preflight_worker),
            "--qtloader",
            str(self.qtloader),
            "--media",
            str(self.media),
            "--expected-audio-worklet-occurrences",
            self.expected_audio_worklet_occurrences,
            "--expected-wasm-worker-occurrences",
            self.expected_wasm_worker_occurrences,
            *extra,
        ]

    def run_packager(self, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            self.command(*extra),
            cwd=PROBE.parents[1],
            check=False,
            capture_output=True,
            text=True,
        )

    def run_verifier(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                "-I",
                "-B",
                str(PACKAGER),
                "--verify-output-dir",
                str(self.output),
                "--expected-build-id",
                BUILD_ID,
            ],
            cwd=PROBE.parents[1],
            check=False,
            capture_output=True,
            text=True,
        )

    def assert_main_fragment_rejected(
        self,
        fragment: str,
        expected_error: str,
    ) -> None:
        main = self.generated / "RhythmGameWasmProbe.js"
        original = main.read_text("utf-8")
        try:
            main.write_text(original + fragment, encoding="utf-8")
            result = self.run_packager()
            self.assertNotEqual(result.returncode, 0, result.stderr)
            self.assertIn(expected_error, result.stderr)
        finally:
            main.write_text(original, encoding="utf-8")

    def test_packages_exact_content_addressed_set_and_canonical_manifest(
        self,
    ) -> None:
        result = self.run_packager()
        self.assertEqual(result.returncode, 0, result.stderr)

        manifest_raw = (self.output / "runtime-artifacts.json").read_bytes()
        manifest = json.loads(manifest_raw)
        self.assertEqual(
            manifest_raw,
            (
                json.dumps(
                    manifest,
                    ensure_ascii=True,
                    separators=(",", ":"),
                    sort_keys=True,
                )
                + "\n"
            ).encode("ascii"),
        )
        self.assertEqual(manifest["schemaVersion"], 1)
        self.assertEqual(manifest["buildId"], BUILD_ID)
        self.assertEqual(
            set(manifest["artifacts"]),
            {
                "audioWorklet",
                "bootstrap",
                "css",
                "html",
                "mainJs",
                "media",
                "preflightWorker",
                "qtloader",
                "wasm",
                "wasmWorker",
            },
        )

        for role, artifact in manifest["artifacts"].items():
            with self.subTest(role=role):
                target = self.output / artifact["url"]
                self.assertTrue(target.is_file())
                payload = target.read_bytes()
                actual = hashlib.sha256(payload).hexdigest()
                self.assertEqual(artifact["sha256"], actual)
                self.assertEqual(artifact["bytes"], len(payload))
                self.assertEqual(artifact["buildId"], BUILD_ID)
                self.assertRegex(artifact["sri"], r"^sha256-[A-Za-z0-9+/]{43}=$")
                if role != "html":
                    self.assertIn(actual, artifact["url"])

        main = (
            self.output / manifest["artifacts"]["mainJs"]["url"]
        ).read_text("utf-8")
        self.assertIn(manifest["artifacts"]["audioWorklet"]["url"], main)
        self.assertIn(manifest["artifacts"]["wasmWorker"]["url"], main)
        self.assertNotIn("RhythmGameWasmProbe.aw.js", main)
        self.assertNotIn("RhythmGameWasmProbe.ww.js", main)

        html = (self.output / "RhythmGameWasmProbe.html").read_text("utf-8")
        self.assertIn(manifest["artifacts"]["css"]["url"], html)
        self.assertIn(manifest["artifacts"]["bootstrap"]["url"], html)
        self.assertEqual(
            html.count('<link rel="icon" href="data:,">'),
            1,
        )
        self.assertEqual(html.count("<script"), 1)
        self.assertNotIn("<style", html)
        self.assertNotIn("qtloader.js", html)
        self.assertFalse((self.output / "qtlogo.svg").exists())

        all_output = "\n".join(
            path.name for path in sorted(self.output.iterdir())
        )
        self.assertNotIn(str(self.root), all_output)
        self.assertNotRegex(manifest_raw.decode("ascii"), r"\d{4}-\d{2}-\d{2}T")

    def test_repeated_packaging_is_byte_identical(self) -> None:
        first = self.run_packager()
        self.assertEqual(first.returncode, 0, first.stderr)
        before = {
            path.name: path.read_bytes()
            for path in self.output.iterdir()
            if path.is_file()
        }
        second = self.run_packager()
        self.assertEqual(second.returncode, 0, second.stderr)
        after = {
            path.name: path.read_bytes()
            for path in self.output.iterdir()
            if path.is_file()
        }
        self.assertEqual(before, after)

    def test_rejects_missing_extra_and_unknown_generated_shapes(self) -> None:
        (self.generated / "RhythmGameWasmProbe.aw.js").unlink()
        missing = self.run_packager()
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn("missing", missing.stderr.casefold())

        (self.generated / "RhythmGameWasmProbe.aw.js").write_text(
            "worker\n", encoding="utf-8"
        )
        (self.generated / "RhythmGameWasmProbe.worker.js").write_text(
            "extra\n", encoding="utf-8"
        )
        extra = self.run_packager()
        self.assertNotEqual(extra.returncode, 0)
        self.assertIn("unexpected generated runtime asset", extra.stderr)

    def test_rejects_worker_literal_occurrence_drift(self) -> None:
        main = self.generated / "RhythmGameWasmProbe.js"
        main.write_text(
            main.read_text("utf-8").replace(
                'const aw = "RhythmGameWasmProbe.aw.js";\n',
                "",
            ),
            encoding="utf-8",
        )
        result = self.run_packager()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("audio-worklet basename occurrence", result.stderr)

    def test_rejects_comment_decoys_and_constructed_worker_basenames(
        self,
    ) -> None:
        (self.generated / "RhythmGameWasmProbe.js").write_text(
            "// RhythmGameWasmProbe.aw.js decoy\n"
            'const aw = "RhythmGameWasmProbe." + "aw.js";\n'
            'const ww = "RhythmGameWasmProbe.ww.js";\n',
            encoding="utf-8",
        )
        result = self.run_packager()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("audio-worklet basename occurrence", result.stderr)

    def test_rejects_executable_dynamic_code_generation_only(self) -> None:
        for fragment in (
            'const invoke = new Function("value", "return value");\n',
            'const invoke = Function("value", "return value");\n',
            'const invoke = globalThis.Function("return 1");\n',
            'const invoke = new window.Function("return 1");\n',
            'const result = eval("40 + 2");\n',
            'const result = globalThis.eval("40 + 2");\n',
            'const result = window.eval("40 + 2");\n',
            'const result = `${new Function("return 1")()}`;\n',
        ):
            with self.subTest(fragment=fragment):
                self.assert_main_fragment_rejected(
                    fragment,
                    "dynamic code execution",
                )

        main = self.generated / "RhythmGameWasmProbe.js"
        main.write_text(
            main.read_text("utf-8")
            + 'const diagnostic = "new Function(\\"return 1\\")";\n'
            + "// eval('comment only');\n"
            + "const evaluate = value => value;\n"
            + (
                "const jspi = new WebAssembly.Function("
                "{ parameters: [], results: [] }, value => value);\n"
            ),
            encoding="utf-8",
        )
        inert = self.run_packager()
        self.assertEqual(inert.returncode, 0, inert.stderr)

    def test_read_only_verifier_rejects_rebound_dynamic_main(self) -> None:
        packaged = self.run_packager()
        self.assertEqual(packaged.returncode, 0, packaged.stderr)
        manifest_path = self.output / "runtime-artifacts.json"
        manifest = json.loads(manifest_path.read_bytes())
        entry = manifest["artifacts"]["mainJs"]
        old_main = self.output / entry["url"]
        payload = (
            old_main.read_bytes()
            + b'\nconst invoke = new Function("return 1");\n'
        )
        digest = hashlib.sha256(payload).hexdigest()
        new_name = f"RhythmGameWasmProbe.{digest}.js"
        old_main.unlink()
        (self.output / new_name).write_bytes(payload)
        entry.update({
            "bytes": len(payload),
            "sha256": digest,
            "sri": (
                "sha256-"
                + base64.b64encode(
                    hashlib.sha256(payload).digest()
                ).decode("ascii")
            ),
            "url": new_name,
        })
        manifest_path.write_bytes(
            (
                json.dumps(
                    manifest,
                    ensure_ascii=True,
                    separators=(",", ":"),
                    sort_keys=True,
                )
                + "\n"
            ).encode("ascii"),
        )

        verified = self.run_verifier()
        self.assertNotEqual(verified.returncode, 0, verified.stderr)
        self.assertIn("dynamic code execution", verified.stderr)

    def test_rejects_private_paths_and_timestamps_in_every_text_asset(
        self,
    ) -> None:
        text_assets = (
            self.generated / "RhythmGameWasmProbe.js",
            self.generated / "RhythmGameWasmProbe.aw.js",
            self.generated / "RhythmGameWasmProbe.ww.js",
            self.sources / "probe.css",
            self.sources / "bootstrap.mjs",
            self.sources / "preflight-worker.mjs",
            self.sources / "qtloader.js",
            self.template,
        )
        for target in text_assets:
            with self.subTest(target=target.name):
                original = target.read_text("utf-8")
                target.write_text(
                    f'{original}\nconst leaked = "T:/private/build/source.cpp";\n',
                    encoding="utf-8",
                )
                result = self.run_packager()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("private build path", result.stderr)
                target.write_text(original, encoding="utf-8")

        main = self.generated / "RhythmGameWasmProbe.js"
        original = main.read_text("utf-8")
        for leak, label in (
            ('const leaked = "/home/builder/project/source.cpp";\n',
             "private build path"),
            ('const stamp = "2026-07-25T18:42:17Z";\n',
             "build timestamp"),
        ):
            with self.subTest(leak=label):
                main.write_text(original + leak, encoding="utf-8")
                result = self.run_packager()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(label, result.stderr)
        main.write_text(original, encoding="utf-8")

    def test_allows_pinned_emscripten_virtual_filesystem_roots(self) -> None:
        main = self.generated / "RhythmGameWasmProbe.js"
        main.write_text(
            main.read_text("utf-8")
            + 'FS.mkdir("/tmp");\n'
            + 'FS.mkdir("/home");\n'
            + 'FS.mkdir("/home/web_user");\n',
            encoding="utf-8",
        )
        result = self.run_packager()
        self.assertEqual(result.returncode, 0, result.stderr)
        main.write_text(
            main.read_text("utf-8")
            + 'FS.mkdir("/home/web_user/cache");\n',
            encoding="utf-8",
        )
        descendant = self.run_packager()
        self.assertNotEqual(descendant.returncode, 0)
        self.assertIn("private build path", descendant.stderr)

    def test_text_hygiene_rejects_every_unreviewed_path_form(self) -> None:
        rejected_fragments = (
            'const p = "/root/private/build/source.cpp";\n',
            'const p = "/srv/ci/checkout/source.cpp";\n',
            'const p = "/var/lib/jenkins/workspace/source.cpp";\n',
            'const p = "/usr/src/app/source.cpp";\n',
            'const p = "/root";\n',
            'const p = "/home/web_user/cache";\n',
            'const p = "/dev/shm/tmp/x";\n',
            'const p = "/fixtures/other.webm";\n',
            'const p = "/fixtures/private/probe.webm";\n',
            'const p = "/fixtures/probe.webm/child";\n',
            'const p = "//root/private/source.cpp";\n',
            'const p = "///root/private/source.cpp";\n',
            'const p = "////root";\n',
            r'const p = "\\server\share\profile\source.cpp";' + "\n",
            r'const p = "\\\\server\\share\\profile\\source.cpp";' + "\n",
            r'const p = "\\?\C:\private\source.cpp";' + "\n",
            r'const p = "\\\\?\\C:\\private\\source.cpp";' + "\n",
            'const p = "file:///root/private/source.cpp";\n',
            'const p = "file:/root/private/source.cpp";\n',
            r'const p = "file:C:\private\source.cpp";' + "\n",
            'const p = "file:relative/private/source.cpp";\n',
            'const p = "file://server/share/private/source.cpp";\n',
            'const p = "FILE:///root/private/source.cpp";\n',
            'const p = "File://server/share/private/source.cpp";\n',
            'const p = "at /root/private/source.cpp";\n',
            'const p = "source=/srv/ci/checkout/source.cpp";\n',
            'const p = `failure at /var/lib/jenkins/source.cpp`;\n',
            'const p = "prefix /home/builder/project/source.cpp suffix";\n',
            r'const p = "\/root\/private\/source.cpp";' + "\n",
            r'const p = "file:\/\/\/root\/private\/source.cpp";' + "\n",
            r'const p = "\x2froot\x2fprivate\x2fsource.cpp";' + "\n",
            r'const p = "\u002froot\u002fprivate\u002fsource.cpp";' + "\n",
            r'const p = "\u{2f}root\u{2f}private\u{2f}source.cpp";'
            + "\n",
            r'const p = "\u{002f}root\u{002f}private\u{002f}source.cpp";'
            + "\n",
            r'const p = "\057root\057private\057source.cpp";' + "\n",
            (
                'const p = "/home/web_user/\\\n'
                'cache/source.cpp";\n'
            ),
            r'const p = "\x5c\x5cserver\x5cshare\x5csource.cpp";'
            + "\n",
            r'const p = "\u005c\u005cserver\u005cshare\u005csource.cpp";'
            + "\n",
            r'const p = "\u{5c}\u{5c}server\u{5c}share\u{5c}source.cpp";'
            + "\n",
            r'const p = "\134\134server\134share\134source.cpp";'
            + "\n",
            (
                r'const p = "\x66\x69\x6c\x65\x3a'
                r'\x2f\x2f\x2froot\x2fprivate\x2fsource.cpp";'
                + "\n"
            ),
            r'const p = "\\сервер\общий\source.cpp";' + "\n",
            r'const p = "\\server\共有\source.cpp";' + "\n",
            "//# sourceMappingURL=/root/private/source.js.map\n",
            "/* /root/private/source.cpp */\n",
        )
        for fragment in rejected_fragments:
            with self.subTest(fragment=fragment):
                self.assert_main_fragment_rejected(
                    fragment,
                    "private build path",
                )

        css = self.sources / "probe.css"
        original_css = css.read_text("utf-8")
        try:
            css.write_text(
                original_css
                + "body { background: url(/root/private/source.png); }\n",
                encoding="utf-8",
            )
            result = self.run_packager()
            self.assertNotEqual(result.returncode, 0, result.stderr)
            self.assertIn("private build path", result.stderr)

            css.write_text(
                original_css
                + r"body { background: url(\2f root\2f private"
                + r"\2f source.png); }"
                + "\n",
                encoding="utf-8",
            )
            escaped_slash = self.run_packager()
            self.assertNotEqual(
                escaped_slash.returncode,
                0,
                escaped_slash.stderr,
            )
            self.assertIn(
                "private build path",
                escaped_slash.stderr,
            )

            css.write_text(
                original_css
                + r"body { background: url(\2f root\2f private"
                + r"\2f a.png); }"
                + "\n",
                encoding="utf-8",
            )
            hex_terminated = self.run_packager()
            self.assertNotEqual(
                hex_terminated.returncode,
                0,
                hex_terminated.stderr,
            )
            self.assertIn(
                "private build path",
                hex_terminated.stderr,
            )

            css.write_text(
                original_css
                + r"body { background: url(\/root\/private"
                + r"\/source.png); }"
                + "\n",
                encoding="utf-8",
            )
            simple_escaped = self.run_packager()
            self.assertNotEqual(
                simple_escaped.returncode,
                0,
                simple_escaped.stderr,
            )
            self.assertIn(
                "private build path",
                simple_escaped.stderr,
            )

            css.write_text(
                original_css
                + r"body { background: url(\66 ile:\2f \2f \2f root"
                + r"\2f private\2f source.png); }"
                + "\n",
                encoding="utf-8",
            )
            encoded_scheme = self.run_packager()
            self.assertNotEqual(
                encoded_scheme.returncode,
                0,
                encoded_scheme.stderr,
            )
            self.assertIn(
                "private build path",
                encoded_scheme.stderr,
            )

            css.write_text(
                original_css
                + r"body { background: url(\5c \5c server"
                + r"\5c share\5c source.png); }"
                + "\n",
                encoding="utf-8",
            )
            escaped_unc = self.run_packager()
            self.assertNotEqual(
                escaped_unc.returncode,
                0,
                escaped_unc.stderr,
            )
            self.assertIn("private build path", escaped_unc.stderr)
        finally:
            css.write_text(original_css, encoding="utf-8")

    def test_text_hygiene_accepts_only_reviewed_slash_literals(
        self,
    ) -> None:
        allowed = (
            "/",
            "//",
            "/dev",
            "/dev/null",
            "/dev/shm",
            "/dev/shm/tmp",
            "/dev/stderr",
            "/dev/stdin",
            "/dev/stdout",
            "/dev/tty",
            "/dev/tty1",
            "/fixtures/probe.webm",
            "/home",
            "/home/web_user",
            "/proc",
            "/proc/self",
            "/proc/self/fd",
            "/tmp",
            "/probe/ws",
            "/path/to/destination",
        )
        main = self.generated / "RhythmGameWasmProbe.js"
        main.write_text(
            main.read_text("utf-8")
            + "".join(
                f'const reviewedPath{index} = "{value}";\n'
                for index, value in enumerate(allowed)
            )
            + 'const marker = "file://";\n'
            + 'const secure = "https://example.invalid/path";\n'
            + 'const socket = "wss://";\n'
            + r'const stackA = "\\s*(.*?)@(.*?):([0-9]+):([0-9]+)";'
            + "\n"
            + r'const stackB = "\\s*at (.*?) \\((.*):(.*):(.*)\\)";'
            + "\n"
            + "const ratio = left / right / 2;\n"
            + "const dynamicPath = `${mount}/${path}`;\n"
            + (
                "const mediaRoute = "
                "`/fixtures/probe.webm?nonce=${runNonce}`;\n"
            )
            + (
                "const located = `${config.qt.qtdir}/lib/"
                "${originalLocatedFilename}`;\n"
            )
            + r"const route = /^\/probe\/ws$/;"
            + "\n"
            + 'const closingTag = "</script>";\n',
            encoding="utf-8",
        )
        css = self.sources / "probe.css"
        css.write_text(
            css.read_text("utf-8")
            + "body { background: url(https://example.invalid/path/a.png); }\n",
            encoding="utf-8",
        )
        result = self.run_packager()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_text_hygiene_rejects_deterministic_timestamps(self) -> None:
        timestamps = (
            'const stamp = "2026-07-25";\n',
            'const stamp = "2026-07-25T18:42:17Z";\n',
            'const stamp = "Jul 25 2026";\n',
            'const stamp = "Jul  5 2026";\n',
            'const stamp = "17:42:17";\n',
            'const stamp = "Sat, 25 Jul 2026 17:42:17 GMT";\n',
            'const stamp = "20260725T184217Z";\n',
            "const stamp = __DATE__;\n",
            "const stamp = __TIME__;\n",
        )
        for fragment in timestamps:
            with self.subTest(fragment=fragment):
                self.assert_main_fragment_rejected(
                    fragment,
                    "build timestamp",
                )

    def test_generated_shape_is_checked_at_the_real_link_directory(
        self,
    ) -> None:
        actual = self.root / "real-link-output"
        shutil.copytree(self.generated, actual)
        (actual / "RhythmGameWasmProbe.unexpected.js").write_text(
            "unexpected\n",
            encoding="utf-8",
        )
        self.generated_shape = actual
        result = self.run_packager()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unexpected generated runtime asset", result.stderr)

    def test_mixed_packaging_staging_is_not_the_generated_shape_root(
        self,
    ) -> None:
        actual = self.root / "real-link-output"
        shutil.copytree(self.generated, actual)
        staged_template = (
            self.generated / "RhythmGameWasmProbe.html.in"
        )
        shutil.copy2(self.template, staged_template)
        self.template = staged_template
        self.generated_shape = actual
        result = self.run_packager()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_current_real_generated_bytes_obey_package_policy_if_present(
        self,
    ) -> None:
        current = PROBE / "build" / "wasm-release"
        paths = [current / name for name in sorted({
            "RhythmGameWasmProbe.aw.js",
            "RhythmGameWasmProbe.js",
            "RhythmGameWasmProbe.wasm",
            "RhythmGameWasmProbe.ww.js",
        })]
        if not all(path.is_file() for path in paths):
            self.skipTest("current authenticated generated bytes are absent")
        for source in paths:
            shutil.copy2(source, self.generated / source.name)
        self.template = HTML_TEMPLATE
        self.css = PROBE / "browser" / "web" / "probe.css"
        self.bootstrap = PROBE / "browser" / "web" / "bootstrap.mjs"
        self.preflight_worker = (
            PROBE / "browser" / "web" / "preflight-worker.mjs"
        )
        self.qtloader = (
            PROBE.parents[1]
            / ".wasm-vcpkg"
            / "installed"
            / "wasm32-emscripten-rg"
            / "plugins"
            / "platforms"
            / "qtloader.js"
        )
        self.media = PROBE / "browser" / "fixtures" / "probe.webm"
        self.expected_audio_worklet_occurrences = "0"
        self.expected_wasm_worker_occurrences = "0"
        result = self.run_packager()
        current_main = (
            self.generated / "RhythmGameWasmProbe.js"
        ).read_text("utf-8")
        if re.search(
            r"\b(?:eval|new\s+Function)\s*\(",
            current_main,
        ):
            self.assertNotEqual(result.returncode, 0, result.stderr)
            self.assertIn("dynamic code execution", result.stderr)
            sanitized, replacements = re.subn(
                r"\bnew\s+Function\s*\(",
                "new WebAssembly.Function(",
                current_main,
            )
            self.assertGreater(replacements, 0)
            (
                self.generated / "RhythmGameWasmProbe.js"
            ).write_text(sanitized, encoding="utf-8")
            without_owned_failure = self.run_packager()
            self.assertEqual(
                without_owned_failure.returncode,
                0,
                without_owned_failure.stderr,
            )
            verified = self.run_verifier()
            self.assertEqual(verified.returncode, 0, verified.stderr)
            self.assertEqual(len(list(self.output.iterdir())), 11)
            return
        self.assertEqual(result.returncode, 0, result.stderr)
        verified = self.run_verifier()
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertEqual(len(list(self.output.iterdir())), 11)

    def test_read_only_verifier_fails_on_deleted_content_addressed_leaf(
        self,
    ) -> None:
        packaged = self.run_packager()
        self.assertEqual(packaged.returncode, 0, packaged.stderr)
        before = {
            path.name: (path.read_bytes(), path.stat().st_mtime_ns)
            for path in self.output.iterdir()
        }
        verified = self.run_verifier()
        self.assertEqual(verified.returncode, 0, verified.stderr)
        after = {
            path.name: (path.read_bytes(), path.stat().st_mtime_ns)
            for path in self.output.iterdir()
        }
        self.assertEqual(after, before)

        manifest = json.loads(
            (self.output / "runtime-artifacts.json").read_bytes()
        )
        (self.output / manifest["artifacts"]["bootstrap"]["url"]).unlink()
        missing = self.run_verifier()
        self.assertNotEqual(missing.returncode, 0)
        self.assertIn("runtime directory set", missing.stderr)

    def test_read_only_verifier_fails_on_added_or_changed_leaf(self) -> None:
        packaged = self.run_packager()
        self.assertEqual(packaged.returncode, 0, packaged.stderr)
        manifest = json.loads(
            (self.output / "runtime-artifacts.json").read_bytes()
        )
        main = self.output / manifest["artifacts"]["mainJs"]["url"]
        main.write_bytes(main.read_bytes() + b"\nchanged\n")
        changed = self.run_verifier()
        self.assertNotEqual(changed.returncode, 0)
        self.assertIn("runtime artifact mainJs bytes drifted", changed.stderr)

        repackaged = self.run_packager()
        self.assertEqual(repackaged.returncode, 0, repackaged.stderr)
        (self.output / "unexpected.txt").write_text(
            "unexpected\n",
            encoding="utf-8",
        )
        added = self.run_verifier()
        self.assertNotEqual(added.returncode, 0)
        self.assertIn("runtime directory set drifted", added.stderr)

    def test_repository_template_has_only_bootstrap_resources(self) -> None:
        source = HTML_TEMPLATE.read_text("utf-8")
        self.assertEqual(source.count("@PROBE_CSS_URL@"), 1)
        self.assertEqual(source.count("@PROBE_CSS_SRI@"), 1)
        self.assertEqual(source.count("@PROBE_BOOTSTRAP_URL@"), 1)
        self.assertEqual(source.count("@PROBE_BOOTSTRAP_SRI@"), 1)
        self.assertEqual(
            source.count('<link rel="icon" href="data:,">'),
            1,
        )
        self.assertEqual(source.count("<script"), 1)
        self.assertNotIn("<style", source)
        self.assertNotRegex(source, r"\son[a-z]+\s*=")

    def test_packaging_rejects_missing_changed_or_duplicate_favicon(
        self,
    ) -> None:
        original = self.template.read_text("utf-8")
        marker = '<link rel="icon" href="data:,">'
        variants = {
            "missing": original.replace(marker + "\n", ""),
            "changed": original.replace(
                marker,
                '<link rel="icon" href="data:text/plain,">',
            ),
            "duplicate": original.replace(
                marker,
                marker + "\n" + marker,
            ),
        }
        try:
            for label, source in variants.items():
                with self.subTest(label=label):
                    self.template.write_text(source, encoding="utf-8")
                    result = self.run_packager()
                    self.assertNotEqual(
                        result.returncode,
                        0,
                        result.stderr,
                    )
                    self.assertIn("favicon", result.stderr.casefold())
        finally:
            self.template.write_text(original, encoding="utf-8")


if __name__ == "__main__":
    unittest.main()
