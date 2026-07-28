from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PLAYTEST = Path(__file__).resolve().parents[1]
SCRIPT = PLAYTEST / "cmake" / "GenerateWebPlaytestChart.cmake"
GENERATOR = PLAYTEST / "scripts" / "generate_chart_package.py"
TEMPLATE = PLAYTEST / "cmake" / "WebPlaytestChartManifest.json.in"


class ChartPackageCmakeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cmake = shutil.which("cmake.exe") or shutil.which("cmake")
        if cmake is None:
            raise unittest.SkipTest("cmake is required")
        cls.cmake = cmake

    def _run(
        self,
        root: Path | None,
        selected: str | None,
        output: Path,
    ) -> subprocess.CompletedProcess[str]:
        arguments = [
            self.cmake,
            f"-DRG_WEB_PLAYTEST_PYTHON={sys.executable}",
            f"-DRG_WEB_PLAYTEST_CHART_GENERATOR={GENERATOR}",
            f"-DRG_WEB_PLAYTEST_CHART_STAGING_DIR={output / 'chart-staging'}",
            f"-DRG_WEB_PLAYTEST_CHART_MANIFEST_TEMPLATE={TEMPLATE}",
            (
                "-DRG_WEB_PLAYTEST_CHART_MANIFEST="
                f"{output / 'web-playtest-chart-manifest.json'}"
            ),
            (
                "-DRG_WEB_PLAYTEST_CHART_PACKAGE_CMAKE="
                f"{output / 'chart-package.cmake'}"
            ),
        ]
        if root is not None:
            arguments.append(f"-DRG_WEB_PLAYTEST_CHART_DIR={root}")
        if selected is not None:
            arguments.append(
                f"-DRG_WEB_PLAYTEST_CHART_RELATIVE_PATH={selected}"
            )
        arguments.extend(("-P", str(SCRIPT)))
        return subprocess.run(
            arguments,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_valid_package_passes_script_mode(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart cmake valid "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            root.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")
            result = self._run(root, "song.bme", base / "output")
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_missing_and_unsafe_inputs_fail_script_mode(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart cmake invalid "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            root.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")
            outside = base / "outside.bme"
            outside.write_text("#TITLE outside\n", "utf-8")
            cases = (
                ("missing-root", None, "song.bme"),
                ("missing-relative", root, None),
                ("parent-traversal", root, "../outside.bme"),
                ("absolute-selected", root, str(outside)),
                ("unsupported-extension", root, "song.exe"),
            )
            for name, selected_root, selected in cases:
                with self.subTest(case=name):
                    result = self._run(
                        selected_root,
                        selected,
                        base / f"output-{name}",
                    )
                    self.assertNotEqual(
                        result.returncode,
                        0,
                        result.stdout + result.stderr,
                    )

    def test_reparse_entry_fails_script_mode(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart cmake reparse "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            outside = base / "outside"
            root.mkdir()
            outside.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")
            linked = root / "linked"
            try:
                if os.name == "nt":
                    created = subprocess.run(
                        ["cmd", "/c", "mklink", "/J", str(linked), str(outside)],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    if created.returncode != 0:
                        self.skipTest(created.stdout + created.stderr)
                else:
                    linked.symlink_to(outside, target_is_directory=True)
                result = self._run(
                    root, "song.bme", base / "output-reparse"
                )
                self.assertNotEqual(result.returncode, 0)
            finally:
                if linked.exists() or linked.is_symlink():
                    if os.name == "nt":
                        os.rmdir(linked)
                    else:
                        linked.unlink()


if __name__ == "__main__":
    unittest.main()
