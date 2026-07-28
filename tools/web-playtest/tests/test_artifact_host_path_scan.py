from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCANNER = (
    Path(__file__).resolve().parents[1]
    / "scripts"
    / "scan_artifacts_for_host_path.py"
)
LEAVES = (
    "RhythmGameWasmProbe.js",
    "RhythmGameWasmProbe.wasm",
    "RhythmGameWasmProbe.aw.js",
    "RhythmGameWasmProbe.ww.js",
)


class ArtifactHostPathScanTest(unittest.TestCase):
    def _run(
        self,
        directory: Path,
        host_root: str,
    ) -> subprocess.CompletedProcess[str]:
        command = [
            sys.executable,
            "-I",
            "-B",
            str(SCANNER),
            "--host-root",
            host_root,
        ]
        for leaf in LEAVES:
            command.extend(("--artifact", str(directory / leaf)))
        return subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
        )

    def _run_generated(
        self,
        host_root: str,
        paths: tuple[Path, ...],
    ) -> subprocess.CompletedProcess[str]:
        command = [
            sys.executable,
            "-I",
            "-B",
            str(SCANNER),
            "--host-root",
            host_root,
        ]
        for path in paths:
            command.extend(("--generated-input", str(path)))
        return subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_exact_four_clean_leaves_pass(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg artifact path scan "
        ) as temporary:
            directory = Path(temporary)
            for leaf in LEAVES:
                (directory / leaf).write_bytes(b"clean artifact")
            result = self._run(directory, r"T:\private\chart")
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_utf8_utf16le_and_utf16be_leaks_fail(self) -> None:
        host_root = r"T:\private\chart"
        for encoding in ("utf-8", "utf-16le", "utf-16be"):
            with self.subTest(encoding=encoding):
                with tempfile.TemporaryDirectory(
                    prefix="rg artifact path leak "
                ) as temporary:
                    directory = Path(temporary)
                    for leaf in LEAVES:
                        (directory / leaf).write_bytes(b"clean artifact")
                    (directory / LEAVES[1]).write_bytes(
                        b"prefix" + host_root.encode(encoding) + b"suffix"
                    )
                    result = self._run(directory, host_root)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(encoding, result.stderr)

    def test_alternate_separator_spelling_fails(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg artifact alternate path leak "
        ) as temporary:
            directory = Path(temporary)
            for leaf in LEAVES:
                (directory / leaf).write_bytes(b"clean artifact")
            (directory / LEAVES[0]).write_bytes(
                b"T:/private/chart"
            )
            result = self._run(directory, r"T:\private\chart")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("private chart root leak", result.stderr)

    def test_case_only_windows_spelling_fails(self) -> None:
        for encoding in ("utf-8", "utf-16le", "utf-16be"):
            with self.subTest(encoding=encoding):
                with tempfile.TemporaryDirectory(
                    prefix="rg artifact case path leak "
                ) as temporary:
                    directory = Path(temporary)
                    for leaf in LEAVES:
                        (directory / leaf).write_bytes(b"clean artifact")
                    (directory / LEAVES[0]).write_bytes(
                        b"prefix"
                        + "t:/private/chart".encode(encoding)
                        + b"suffix"
                    )
                    result = self._run(directory, r"T:\Private\Chart")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(encoding, result.stderr)

    def test_generated_cpp_and_manifests_are_scanned(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg generated input path scan "
        ) as temporary:
            directory = Path(temporary)
            generated = (
                directory / "WebPlaytestInputDigest.cpp",
                directory / "web-playtest-chart-manifest.json",
                directory / "web_playtest_chart.qrc",
                directory / "qrc_web_playtest_chart_tmp.cpp",
            )
            for path in generated:
                path.write_bytes(b"clean generated input")
            clean = self._run_generated(r"T:\Private\Chart", generated)
            self.assertEqual(clean.returncode, 0, clean.stderr)

            generated[-1].write_bytes(b"prefix t:/private/chart suffix")
            leaked = self._run_generated(r"T:\Private\Chart", generated)
            self.assertNotEqual(leaked.returncode, 0)
            self.assertIn("private chart root leak", leaked.stderr)


if __name__ == "__main__":
    unittest.main()
