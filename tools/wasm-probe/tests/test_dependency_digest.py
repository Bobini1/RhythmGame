from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
GENERATOR = (
    REPO
    / "tools"
    / "wasm-probe"
    / "scripts"
    / "generate_dependency_digest.py"
)
CONTRACT = REPO / "tools" / "wasm-probe" / "dependency-archive-contract.json"


class DependencyDigestGeneratorTest(unittest.TestCase):
    def _run(
        self,
        installed: Path,
        cpp: Path,
        manifest: Path,
        generator: Path = GENERATOR,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                "-I",
                "-B",
                str(generator),
                "--contract",
                str(CONTRACT),
                "--installed-root",
                str(installed),
                "--output-cpp",
                str(cpp),
                "--output-manifest",
                str(manifest),
            ],
            cwd=REPO,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )

    def test_static_archive_superset_is_deterministic_and_embeddable(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg dependency digest ") as value:
            root = Path(value)
            installed = root / "installed"
            (installed / "lib").mkdir(parents=True)
            (installed / "plugins").mkdir()
            (installed / "lib" / "Qt6Core.a").write_bytes(
                b"!<arch>\ncore"
            )
            (installed / "plugins" / "libPlugin.a").write_bytes(
                b"!<arch>\nplugin"
            )
            (installed / "plugins" / "Plugin_init.cpp.o").write_bytes(
                b"\x00asm\x01\x00\x00\x00plugin object"
            )
            (installed / "lib" / "ignored.lib").write_bytes(b"ignored")
            cpp = root / "generated" / "ProbeDependencyDigest.cpp"
            manifest = root / "generated" / "dependency-digest.json"

            first = self._run(installed, cpp, manifest)

            self.assertEqual(first.returncode, 0, first.stdout + first.stderr)
            payload = json.loads(manifest.read_text("utf-8"))
            self.assertEqual(payload["algorithm"], "sha256-path-null-digest-lf-v1")
            self.assertEqual(payload["schemaVersion"], 2)
            self.assertEqual(payload["fileCount"], 3)
            self.assertEqual(
                [entry["path"] for entry in payload["files"]],
                [
                    "lib/Qt6Core.a",
                    "plugins/libPlugin.a",
                    "plugins/Plugin_init.cpp.o",
                ],
            )
            self.assertEqual(
                [entry["kind"] for entry in payload["files"]],
                ["archive", "archive", "wasm-object"],
            )
            marker = (
                "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
                + payload["aggregateSha256"]
            )
            self.assertIn(marker, cpp.read_text("utf-8"))

            cpp_before = cpp.read_bytes()
            manifest_before = manifest.read_bytes()
            cpp_mtime = cpp.stat().st_mtime_ns
            manifest_mtime = manifest.stat().st_mtime_ns
            second = self._run(installed, cpp, manifest)
            self.assertEqual(second.returncode, 0, second.stdout + second.stderr)
            self.assertEqual(cpp.read_bytes(), cpp_before)
            self.assertEqual(manifest.read_bytes(), manifest_before)
            self.assertEqual(cpp.stat().st_mtime_ns, cpp_mtime)
            self.assertEqual(manifest.stat().st_mtime_ns, manifest_mtime)

            archive = installed / "lib" / "Qt6Core.a"
            archive.write_bytes(b"!<arch>\nCORE")
            changed = self._run(installed, cpp, manifest)
            self.assertEqual(changed.returncode, 0, changed.stdout + changed.stderr)
            changed_payload = json.loads(manifest.read_text("utf-8"))
            self.assertNotEqual(
                changed_payload["aggregateSha256"],
                payload["aggregateSha256"],
            )

    def test_isolated_mode_rejects_adjacent_stdlib_shadow(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg dependency shadow ") as value:
            root = Path(value)
            scripts = root / "scripts"
            scripts.mkdir()
            generator = scripts / GENERATOR.name
            shutil.copy2(GENERATOR, generator)
            (scripts / "hashlib.py").write_text(
                'raise RuntimeError("adjacent hashlib shadow imported")\n',
                encoding="utf-8",
            )
            installed = root / "installed"
            installed.mkdir()
            (installed / "fixture.a").write_bytes(b"!<arch>\nfixture")

            result = self._run(
                installed,
                root / "ProbeDependencyDigest.cpp",
                root / "dependency-digest.json",
                generator=generator,
            )

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn(
                "adjacent hashlib shadow",
                result.stdout + result.stderr,
            )

    @unittest.skipUnless(
        hasattr(os, "symlink"),
        "symbolic links are unavailable",
    )
    def test_reparse_archive_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg dependency reparse ") as value:
            root = Path(value)
            installed = root / "installed"
            installed.mkdir()
            outside = root / "outside.a"
            outside.write_bytes(b"outside")
            link = installed / "linked.a"
            try:
                link.symlink_to(outside)
            except OSError as error:
                self.skipTest(f"cannot create archive symlink: {error}")

            result = self._run(
                installed,
                root / "ProbeDependencyDigest.cpp",
                root / "dependency-digest.json",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "reparse",
                (result.stdout + result.stderr).lower(),
            )

    def test_thin_and_unknown_archive_magic_are_rejected(self) -> None:
        cases = {
            "thin": b"!<thin>\nmember",
            "unknown": b"not-an-archive",
        }
        for name, content in cases.items():
            with self.subTest(name=name):
                with tempfile.TemporaryDirectory(
                    prefix="rg dependency magic "
                ) as value:
                    root = Path(value)
                    installed = root / "installed"
                    installed.mkdir()
                    (installed / "fixture.a").write_bytes(content)

                    result = self._run(
                        installed,
                        root / "ProbeDependencyDigest.cpp",
                        root / "dependency-digest.json",
                    )

                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "archive",
                        (result.stdout + result.stderr).lower(),
                    )

    def test_non_wasm_object_magic_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg dependency object magic "
        ) as value:
            root = Path(value)
            installed = root / "installed"
            installed.mkdir()
            (installed / "fixture.a").write_bytes(b"!<arch>\nfixture")
            (installed / "invalid.o").write_bytes(b"not wasm")

            result = self._run(
                installed,
                root / "ProbeDependencyDigest.cpp",
                root / "dependency-digest.json",
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "wasm object magic/version",
                (result.stdout + result.stderr).lower(),
            )


if __name__ == "__main__":
    unittest.main()
