from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from _toolchain_process_double import (
    EMSDK_COMMIT,
    FAIL_ONCE_EXIT,
    VCPKG_COMMIT,
    seed_build_tools,
    seed_repository,
    write_launcher,
)


REPO = Path(__file__).resolve().parents[3]
PROBE = REPO / "tools" / "wasm-probe"
BOOTSTRAP = PROBE / "scripts" / "Bootstrap-Toolchains.ps1"
WRAPPER = PROBE / "scripts" / "Invoke-WithToolchains.ps1"


@unittest.skipUnless(os.name == "nt", "PowerShell toolchain tests are Windows-only")
class ToolchainScriptTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        pwsh = shutil.which("pwsh.exe") or shutil.which("pwsh")
        if pwsh is None:
            raise unittest.SkipTest("pwsh.exe is required")
        cls.pwsh = str(Path(pwsh).resolve())

    def _fixture(self, temporary: str) -> dict[str, object]:
        sandbox = Path(temporary) / "sandbox with spaces"
        root = sandbox / "toolchains with spaces"
        shims = sandbox / "process shims"
        events = sandbox / "events.jsonl"
        cache = sandbox / "binary cache with spaces"
        root.mkdir(parents=True)
        write_launcher(shims / "git.cmd", "git")
        environment = os.environ.copy()
        environment.update(
            {
                "PATH": f"{shims}{os.pathsep}{environment.get('PATH', '')}",
                "TOOLCHAIN_DOUBLE_EVENTS": str(events),
                "TOOLCHAIN_DOUBLE_ROOT": str(root),
                "HTTP_PROXY": "http://127.0.0.1:9",
                "HTTPS_PROXY": "http://127.0.0.1:9",
                "ALL_PROXY": "http://127.0.0.1:9",
                "NO_PROXY": "",
            }
        )
        return {
            "sandbox": sandbox,
            "root": root,
            "shims": shims,
            "events": events,
            "cache": cache,
            "environment": environment,
        }

    def _run(
        self,
        script: Path,
        arguments: list[str],
        environment: dict[str, str],
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                self.pwsh,
                "-NoLogo",
                "-NoProfile",
                "-NonInteractive",
                "-File",
                str(script),
                *arguments,
            ],
            cwd=REPO,
            env=environment,
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )

    def _run_wrapper(
        self,
        fixture: dict[str, object],
        child: Path,
        child_arguments: list[str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        environment = dict(fixture["environment"])
        environment.update(
            {
                "TOOLCHAIN_DOUBLE_WRAPPER": str(WRAPPER),
                "TOOLCHAIN_DOUBLE_TOOLCHAIN_ROOT": str(fixture["root"]),
                "TOOLCHAIN_DOUBLE_BINARY_CACHE": str(fixture["cache"]),
                "TOOLCHAIN_DOUBLE_CHILD": str(child),
                "TOOLCHAIN_DOUBLE_FORWARD_ARGS": json.dumps(
                    child_arguments or []
                ),
            }
        )
        command = "\n".join(
            (
                "$ErrorActionPreference = 'Stop'",
                (
                    "[string[]]$forward = ConvertFrom-Json "
                    "-InputObject $env:TOOLCHAIN_DOUBLE_FORWARD_ARGS "
                    "-NoEnumerate"
                ),
                (
                    "& $env:TOOLCHAIN_DOUBLE_WRAPPER "
                    "-ToolchainRoot $env:TOOLCHAIN_DOUBLE_TOOLCHAIN_ROOT "
                    "-BinaryCache $env:TOOLCHAIN_DOUBLE_BINARY_CACHE "
                    "-Executable $env:TOOLCHAIN_DOUBLE_CHILD "
                    "-Arguments $forward"
                ),
                "exit $LASTEXITCODE",
            )
        )
        return subprocess.run(
            [
                self.pwsh,
                "-NoLogo",
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                command,
            ],
            cwd=REPO,
            env=environment,
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )

    @staticmethod
    def _events(path: Path) -> list[dict[str, object]]:
        if not path.exists():
            return []
        return [
            json.loads(line)
            for line in path.read_text("utf-8").splitlines()
            if line
        ]

    @staticmethod
    def _seed_complete(root: Path) -> None:
        seed_repository(root / "emsdk-4.0.7", "emsdk", EMSDK_COMMIT)
        seed_repository(root / "vcpkg-a0400024", "vcpkg", VCPKG_COMMIT)
        seed_build_tools(root)

    def test_wrong_heads_fail_closed(self) -> None:
        wrong = "1111111111111111111111111111111111111111"
        repositories = (
            ("emsdk", "emsdk-4.0.7", EMSDK_COMMIT),
            ("vcpkg", "vcpkg-a0400024", VCPKG_COMMIT),
        )
        for kind, directory, expected in repositories:
            for entrypoint in ("bootstrap", "wrapper"):
                with self.subTest(repository=kind, entrypoint=entrypoint):
                    with tempfile.TemporaryDirectory(
                        prefix="rg toolchain drift "
                    ) as temporary:
                        fixture = self._fixture(temporary)
                        root = fixture["root"]
                        assert isinstance(root, Path)
                        self._seed_complete(root)
                        head = root / directory / ".fixture-head"
                        head.write_text(wrong, encoding="ascii")
                        if entrypoint == "bootstrap":
                            result = self._run(
                                BOOTSTRAP,
                                ["-ToolchainRoot", str(root)],
                                fixture["environment"],
                            )
                        else:
                            child = (
                                fixture["sandbox"] / "capture child.cmd"
                            )
                            assert isinstance(child, Path)
                            write_launcher(child, "capture")
                            result = self._run_wrapper(fixture, child)
                        combined = result.stdout + result.stderr
                        self.assertNotEqual(result.returncode, 0, combined)
                        self.assertIn(expected, combined)
                        self.assertIn(wrong, combined)
                        self.assertEqual(head.read_text("ascii"), wrong)
                        events = self._events(fixture["events"])
                        forbidden = {
                            "emsdk",
                            "vcpkg-bootstrap",
                            "em++",
                            "vcpkg",
                            "cmake",
                            "ninja",
                            "capture",
                            "capture-environment",
                        }
                        self.assertFalse(
                            forbidden.intersection(
                                str(event["tool"]) for event in events
                            ),
                            events,
                        )
                        git_operations = [
                            event["args"]
                            for event in events
                            if event["tool"] == "git"
                        ]
                        self.assertTrue(git_operations, events)
                        self.assertTrue(
                            all(
                                operation[-2:] == ["rev-parse", "HEAD"]
                                for operation in git_operations
                            ),
                            git_operations,
                        )

    def test_bootstrap_recovers_owned_partial_clone(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg toolchain recovery "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = fixture["root"]
            assert isinstance(root, Path)
            seed_build_tools(root)
            emsdk_temporary = root / "emsdk-4.0.7.bootstrap-tmp"
            vcpkg_temporary = root / "vcpkg-a0400024.bootstrap-tmp"
            emsdk_temporary.mkdir()
            vcpkg_temporary.mkdir()
            (emsdk_temporary / "stale").write_text("stale", encoding="ascii")
            (vcpkg_temporary / "stale").write_text("stale", encoding="ascii")
            sentinel = root / "emsdk-4.0.7.bootstrap-tmp-user-sentinel"
            sentinel.mkdir()
            (sentinel / "keep").write_text("keep", encoding="ascii")
            marker = fixture["sandbox"] / "clone failed once"
            assert isinstance(marker, Path)
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["TOOLCHAIN_DOUBLE_FAIL_CLONE_ONCE"] = str(marker)

            first = self._run(
                BOOTSTRAP,
                ["-ToolchainRoot", str(root)],
                environment,
            )
            self.assertNotEqual(first.returncode, 0, first.stderr)
            self.assertIn(
                str(FAIL_ONCE_EXIT),
                first.stdout + first.stderr,
            )
            self.assertFalse((root / "emsdk-4.0.7").exists())
            self.assertFalse((root / "vcpkg-a0400024").exists())
            self.assertFalse(emsdk_temporary.exists())
            self.assertTrue(vcpkg_temporary.exists())
            self.assertTrue((sentinel / "keep").is_file())

            second = self._run(
                BOOTSTRAP,
                ["-ToolchainRoot", str(root)],
                environment,
            )
            self.assertEqual(second.returncode, 0, second.stdout + second.stderr)
            self.assertEqual(
                (root / "emsdk-4.0.7" / ".fixture-head").read_text("ascii"),
                EMSDK_COMMIT,
            )
            self.assertEqual(
                (root / "vcpkg-a0400024" / ".fixture-head").read_text("ascii"),
                VCPKG_COMMIT,
            )
            self.assertFalse(emsdk_temporary.exists())
            self.assertFalse(vcpkg_temporary.exists())
            self.assertTrue((sentinel / "keep").is_file())

            events = self._events(fixture["events"])
            clones = [
                event
                for event in events
                if event["tool"] == "git"
                and event["args"][0:2] == ["clone", "--filter=blob:none"]
            ]
            expected_destinations = {
                str(emsdk_temporary.resolve()).casefold(),
                str(vcpkg_temporary.resolve()).casefold(),
            }
            self.assertEqual(
                {
                    str(event["args"][-1]).casefold()
                    for event in clones
                },
                expected_destinations,
            )
            self.assertTrue(
                all(not bool(event["canonical_exists"]) for event in clones),
                clones,
            )
            temporary_git_events = [
                event
                for event in events
                if event["tool"] == "git"
                and ".bootstrap-tmp" in str(event["args"])
            ]
            self.assertTrue(
                all(
                    not bool(event["canonical_exists"])
                    for event in temporary_git_events
                ),
                temporary_git_events,
            )
            sdk_events = [
                (event["tool"], event["args"])
                for event in events
                if event["tool"] in {
                    "emsdk",
                    "vcpkg-bootstrap",
                }
            ]
            self.assertEqual(
                sdk_events,
                [
                    ("emsdk", ["install", "4.0.7"]),
                    ("emsdk", ["activate", "4.0.7"]),
                    ("vcpkg-bootstrap", ["-disableMetrics"]),
                ],
            )

    def test_wrapper_is_hermetic_transparent_and_propagates_exit(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg toolchain wrapper "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = fixture["root"]
            sandbox = fixture["sandbox"]
            assert isinstance(root, Path)
            assert isinstance(sandbox, Path)
            self._seed_complete(root)
            poison = sandbox / "poison matching globals"
            for name, tool in (
                ("em++.cmd", "poison-em++"),
                ("vcpkg.cmd", "poison-vcpkg"),
                ("cmake.cmd", "poison-cmake"),
                ("ninja.cmd", "poison-ninja"),
            ):
                write_launcher(poison / name, tool)
            child = sandbox / "capture child.cmd"
            write_launcher(child, "capture")
            arguments = [
                "ordinary",
                "spaced value",
                "",
                "--",
                "--option-like",
                "-DNAME=a b",
            ]
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment.update(
                {
                    "PATH": (
                        f"{poison}{os.pathsep}"
                        f"{environment.get('PATH', '')}"
                    ),
                    "EMSDK": "T:\\poison emsdk",
                    "EMSCRIPTEN_ROOT": "T:\\poison emscripten",
                    "EMSCRIPTEN_VERSION": "4.0.7",
                    "VCPKG_ROOT": "T:\\poison vcpkg",
                    "VCPKG_DEFAULT_BINARY_CACHE": "T:\\poison cache",
                    "TOOLCHAIN_DOUBLE_CHILD_EXIT": "37",
                }
            )
            parent_before = os.environ.copy()
            fixture["environment"] = environment
            result = self._run_wrapper(fixture, child, arguments)
            self.assertEqual(result.returncode, 37, result.stdout + result.stderr)
            self.assertEqual(os.environ, parent_before)

            events = self._events(fixture["events"])
            self.assertFalse(
                any(
                    str(event["tool"]).startswith("poison-")
                    for event in events
                ),
                events,
            )
            version_events = {
                event["tool"]: event
                for event in events
                if event["tool"] in {"em++", "vcpkg", "cmake", "ninja"}
            }
            self.assertEqual(
                set(version_events),
                {"em++", "vcpkg", "cmake", "ninja"},
            )
            expected_roots = {
                "em++": root / "emsdk-4.0.7" / "upstream" / "emscripten",
                "vcpkg": root / "vcpkg-a0400024",
                "cmake": (
                    root / "cmake-4.2.3-windows-x86_64" / "bin"
                ),
                "ninja": root / "ninja-1.13.2-win",
            }
            for tool, expected_root in expected_roots.items():
                source = Path(str(version_events[tool]["source"]))
                self.assertTrue(
                    source.is_relative_to(expected_root.resolve()),
                    (tool, source, expected_root),
                )

            capture = [
                event
                for event in events
                if event["tool"] == "capture-environment"
            ]
            self.assertEqual(len(capture), 1, events)
            self.assertEqual(capture[0]["args"], arguments)
            captured_environment = capture[0]["environment"]
            self.assertEqual(
                captured_environment["EMSDK"],
                str((root / "emsdk-4.0.7").resolve()),
            )
            self.assertEqual(
                captured_environment["EMSCRIPTEN_ROOT"],
                str(
                    (
                        root
                        / "emsdk-4.0.7"
                        / "upstream"
                        / "emscripten"
                    ).resolve()
                ),
            )
            self.assertEqual(
                captured_environment["EMSCRIPTEN_VERSION"],
                "4.0.7",
            )
            self.assertEqual(
                captured_environment["VCPKG_ROOT"],
                str((root / "vcpkg-a0400024").resolve()),
            )
            self.assertEqual(
                captured_environment["VCPKG_DISABLE_METRICS"],
                "1",
            )
            self.assertEqual(
                captured_environment["VCPKG_DEFAULT_BINARY_CACHE"],
                str(Path(fixture["cache"]).resolve()),
            )
            self.assertTrue(Path(fixture["cache"]).is_dir())

            pinned_path_prefix = os.pathsep.join(
                str(path.resolve())
                for path in (
                    root / "vcpkg-a0400024",
                    root / "cmake-4.2.3-windows-x86_64" / "bin",
                    root / "ninja-1.13.2-win",
                    root
                    / "emsdk-4.0.7"
                    / "upstream"
                    / "emscripten",
                )
            )
            self.assertTrue(
                str(captured_environment["PATH"]).startswith(
                    pinned_path_prefix + os.pathsep
                ),
                captured_environment["PATH"],
            )


if __name__ == "__main__":
    unittest.main()
