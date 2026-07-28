from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


PLAYTEST = Path(__file__).resolve().parents[1]
GENERATOR = PLAYTEST / "scripts" / "generate_chart_package.py"
TEMPLATE = PLAYTEST / "cmake" / "WebPlaytestChartManifest.json.in"


def _load_generator():
    specification = importlib.util.spec_from_file_location(
        "web_playtest_chart_generator", GENERATOR
    )
    if specification is None or specification.loader is None:
        raise RuntimeError("could not load chart generator")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


class ChartPackageGeneratorTest(unittest.TestCase):
    def _run(
        self,
        root: Path,
        selected: str,
        output: Path,
        *,
        template: Path = TEMPLATE,
        staging_dir: Path | None = None,
        manifest_output: Path | None = None,
        cmake_output: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        staging_dir = staging_dir or output / "chart-staging"
        manifest_output = (
            manifest_output
            or output / "web-playtest-chart-manifest.json"
        )
        cmake_output = cmake_output or output / "chart-package.cmake"
        return subprocess.run(
            [
                sys.executable,
                "-I",
                "-B",
                str(GENERATOR),
                "--chart-root",
                str(root),
                "--selected-relative-path",
                selected,
                "--staging-dir",
                str(staging_dir),
                "--manifest-template",
                str(template),
                "--manifest-output",
                str(manifest_output),
                "--cmake-output",
                str(cmake_output),
            ],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_unicode_nested_package_is_deterministic_and_host_path_free(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package unicode "
        ) as temporary:
            base = Path(temporary)
            root = base / "private chart root"
            (root / "Béat" / "深い").mkdir(parents=True)
            chart = root / "Dstorv_act1_evo.bme"
            chart.write_bytes(b"#TITLE Dstorv\n")
            sample = root / "Béat" / "深い" / "keysound.ogg"
            sample.write_bytes(b"OggS\0fixture")

            outputs = (base / "one", base / "two")
            results = [
                self._run(root, "Dstorv_act1_evo.bme", output)
                for output in outputs
            ]
            for result in results:
                self.assertEqual(result.returncode, 0, result.stderr)

            manifests = [
                (output / "web-playtest-chart-manifest.json").read_bytes()
                for output in outputs
            ]
            self.assertEqual(manifests[0], manifests[1])
            self.assertTrue(all(byte < 0x80 for byte in manifests[0]))
            manifest = json.loads(manifests[0].decode("ascii"))
            self.assertEqual(manifest["schema"], 1)
            self.assertEqual(
                manifest["selectedVirtualPath"],
                "/playtest/chart/Dstorv_act1_evo.bme",
            )
            self.assertEqual(
                [entry["virtualPath"] for entry in manifest["files"]],
                sorted(entry["virtualPath"] for entry in manifest["files"]),
            )
            by_path = {
                entry["virtualPath"]: entry for entry in manifest["files"]
            }
            self.assertEqual(
                by_path["/playtest/chart/Dstorv_act1_evo.bme"]["sha256"],
                hashlib.sha256(chart.read_bytes()).hexdigest(),
            )
            self.assertEqual(
                by_path["/playtest/chart/Béat/深い/keysound.ogg"]["size"],
                sample.stat().st_size,
            )
            for output in outputs:
                for generated in (
                    output / "web-playtest-chart-manifest.json",
                    output / "chart-package.cmake",
                ):
                    self.assertNotIn(
                        str(root).encode("utf-8"), generated.read_bytes()
                    )

    def test_unsafe_selected_paths_and_extensions_fail(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package unsafe "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            root.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")
            outside = base / "outside.bme"
            outside.write_text("#TITLE outside\n", "utf-8")
            cases = (
                "",
                "../outside.bme",
                str(outside),
                r"C:\outside.bme",
                r"\\server\share\outside.bme",
                "song.bme:stream",
                "song.exe",
                "missing.bms",
            )
            for index, selected in enumerate(cases):
                with self.subTest(selected=selected):
                    result = self._run(
                        root, selected, base / f"output-{index}"
                    )
                    self.assertNotEqual(result.returncode, 0)

    def test_casefold_and_unicode_normalization_collisions_fail(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package collisions "
        ) as temporary:
            base = Path(temporary)
            for name, pair in (
                ("casefold", ("ss.ogg", "ß.ogg")),
                ("normalization", ("é.ogg", "e\u0301.ogg")),
            ):
                with self.subTest(kind=name):
                    root = base / name
                    root.mkdir()
                    (root / "song.bme").write_text(
                        "#TITLE fixture\n", "utf-8"
                    )
                    try:
                        for relative in pair:
                            (root / relative).write_bytes(relative.encode())
                    except OSError as error:
                        self.skipTest(
                            f"filesystem cannot create collision fixture: {error}"
                        )
                    result = self._run(
                        root, "song.bme", base / f"{name}-output"
                    )
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn(
                        "normalization/casefold collision", result.stderr
                    )

    def test_symlink_or_reparse_input_fails(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package reparse "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            outside = base / "outside"
            root.mkdir()
            outside.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")
            (outside / "sample.ogg").write_bytes(b"OggS")
            linked = root / "linked"
            try:
                if os.name == "nt":
                    result = subprocess.run(
                        ["cmd", "/c", "mklink", "/J", str(linked), str(outside)],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    if result.returncode != 0:
                        self.skipTest(result.stdout + result.stderr)
                else:
                    linked.symlink_to(outside, target_is_directory=True)
                generated = self._run(
                    root, "song.bme", base / "reparse-output"
                )
                self.assertNotEqual(
                    generated.returncode, 0, generated.stdout
                )
            finally:
                if linked.exists() or linked.is_symlink():
                    if os.name == "nt":
                        os.rmdir(linked)
                    else:
                        linked.unlink()

    def test_reparse_chart_root_path_chain_fails(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package root reparse "
        ) as temporary:
            base = Path(temporary)
            real_root = base / "real-chart"
            real_root.mkdir()
            (real_root / "song.bme").write_text(
                "#TITLE fixture\n", "utf-8"
            )
            linked_root = base / "linked-chart"
            try:
                if os.name == "nt":
                    created = subprocess.run(
                        [
                            "cmd",
                            "/c",
                            "mklink",
                            "/J",
                            str(linked_root),
                            str(real_root),
                        ],
                        capture_output=True,
                        text=True,
                        check=False,
                    )
                    if created.returncode != 0:
                        self.skipTest(created.stdout + created.stderr)
                else:
                    linked_root.symlink_to(
                        real_root, target_is_directory=True
                    )
                result = self._run(
                    linked_root, "song.bme", base / "root-reparse-output"
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("reparse-point component", result.stderr)
            finally:
                if linked_root.exists() or linked_root.is_symlink():
                    if os.name == "nt":
                        os.rmdir(linked_root)
                    else:
                        linked_root.unlink()

    def test_non_regular_entry_fails(self) -> None:
        generator = _load_generator()
        with tempfile.TemporaryDirectory(
            prefix="rg chart package nonregular "
        ) as temporary:
            root = Path(temporary)

            class NonRegularEntry:
                name = "named-pipe"
                path = str(root / name)

                @staticmethod
                def stat(*, follow_symlinks: bool):
                    self.assertFalse(follow_symlinks)
                    return os.stat_result(
                        (stat.S_IFIFO, 0, 0, 0, 0, 0, 0, 0, 0, 0)
                    )

            with mock.patch.object(
                generator.os, "scandir", return_value=[NonRegularEntry()]
            ):
                with self.assertRaisesRegex(
                    generator.PackageError, "not a regular file"
                ):
                    generator._scan_chart_root(root)

    def test_chart_root_inside_staging_is_rejected_without_deletion(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package overlap "
        ) as temporary:
            output = Path(temporary) / "output"
            root = output / "chart-staging" / "private-source"
            root.mkdir(parents=True)
            chart = root / "song.bme"
            chart.write_text("#TITLE preserve me\n", "utf-8")

            result = self._run(root, "song.bme", output)

            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn("overlap", result.stderr.casefold())
            self.assertEqual(chart.read_text("utf-8"), "#TITLE preserve me\n")

    def test_containment_is_case_insensitive_on_windows(self) -> None:
        if os.name != "nt":
            self.skipTest("Windows path comparison contract")
        generator = _load_generator()
        self.assertTrue(
            generator._is_within(
                Path(r"c:\Temp\Chart\output"),
                Path(r"C:\temp\chart"),
            )
        )

    def test_outputs_are_distinct_and_cannot_overwrite_template(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg chart package output layout "
        ) as temporary:
            base = Path(temporary)
            root = base / "chart"
            root.mkdir()
            (root / "song.bme").write_text("#TITLE fixture\n", "utf-8")

            duplicate_output = base / "duplicate" / "same.txt"
            duplicate = self._run(
                root,
                "song.bme",
                base / "duplicate",
                manifest_output=duplicate_output,
                cmake_output=duplicate_output,
            )
            self.assertNotEqual(duplicate.returncode, 0)
            self.assertIn("overlap", duplicate.stderr.casefold())

            copied_template = base / "template.in"
            copied_template.write_bytes(TEMPLATE.read_bytes())
            overwrite = self._run(
                root,
                "song.bme",
                base,
                template=copied_template,
                manifest_output=copied_template,
            )
            self.assertNotEqual(overwrite.returncode, 0)
            self.assertEqual(copied_template.read_bytes(), TEMPLATE.read_bytes())

            staging = base / "nested" / "chart-staging"
            staging.mkdir(parents=True)
            nested_template = staging / "template.in"
            nested_template.write_bytes(TEMPLATE.read_bytes())
            nested = self._run(
                root,
                "song.bme",
                base / "nested",
                template=nested_template,
                staging_dir=staging,
            )
            self.assertNotEqual(nested.returncode, 0)
            self.assertTrue(nested_template.is_file())


if __name__ == "__main__":
    unittest.main()
