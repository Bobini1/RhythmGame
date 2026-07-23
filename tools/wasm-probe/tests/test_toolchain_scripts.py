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
PROCESS_DOUBLE = PROBE / "tests" / "_toolchain_process_double.py"


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
        child: str | Path,
        child_arguments: list[str] | None = None,
        wrapper: Path = WRAPPER,
    ) -> subprocess.CompletedProcess[str]:
        environment = dict(fixture["environment"])
        return self._run(
            wrapper,
            [
                "-ToolchainRoot",
                str(fixture["root"]),
                "-BinaryCache",
                str(fixture["cache"]),
                "--",
                str(child),
                *(child_arguments or []),
            ],
            environment,
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

    @staticmethod
    def _hostile_arguments(side_effect: Path) -> list[str]:
        return [
            "ordinary",
            "spaced value",
            "",
            "--",
            "-Verbose",
            "-ToolchainRoot",
            "child toolchain root",
            "-BinaryCache",
            "child binary cache",
            "--option-like",
            "-DNAME=a b",
            "one terminal backslash\\",
            "two terminal backslashes\\\\",
            'embedded "quote"',
            "%PATH%",
            "carets ^ ^^ ^^^",
            "&|<>()",
            "Zażółć gęślą jaźń 日本語 🎵",
            (
                f'mixed %PATH%^&|<>() "quoted" \\\\ '
                f'> "{side_effect}"'
            ),
        ]

    @staticmethod
    def _native_capture_command(
        arguments: list[str],
    ) -> tuple[Path, list[str]]:
        python = Path(sys.executable).resolve()
        return (
            python,
            [
                str(PROCESS_DOUBLE),
                "capture",
                str(python),
                *arguments,
            ],
        )

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
            side_effect = sandbox / "native dispatch side effect"
            arguments = self._hostile_arguments(side_effect)
            child, child_arguments = self._native_capture_command(arguments)
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment.update(
                {
                    "PATH": (
                        f"{poison}{os.pathsep}"
                        f"{environment.get('PATH', '')}"
                    ),
                    "EMSDK": "T:\\poison emsdk",
                    "EMSDK_PYTHON": "T:\\poison python",
                    "EMSCRIPTEN_ROOT": "T:\\poison emscripten",
                    "EMSCRIPTEN_VERSION": "4.0.7",
                    "VCPKG_ROOT": "T:\\poison vcpkg",
                    "VCPKG_DEFAULT_BINARY_CACHE": "T:\\poison cache",
                    "TOOLCHAIN_DOUBLE_CHILD_EXIT": "37",
                }
            )
            invalid_invocations = (
                (
                    ["-Unknown", "value", "--", str(child)],
                    "Unknown wrapper option",
                ),
                (
                    ["-ToolchainRoot"],
                    "Missing value for wrapper option",
                ),
                (
                    ["-ToolchainRoot", str(root)],
                    "Missing mandatory -- wrapper delimiter",
                ),
                (
                    ["--"],
                    "Missing executable after -- wrapper delimiter",
                ),
            )
            for invocation, expected_error in invalid_invocations:
                with self.subTest(wrapper_error=expected_error):
                    invalid = self._run(
                        WRAPPER,
                        invocation,
                        environment,
                    )
                    combined = invalid.stdout + invalid.stderr
                    self.assertNotEqual(invalid.returncode, 0, combined)
                    self.assertIn(expected_error, combined)
            self.assertEqual(self._events(fixture["events"]), [])

            parent_before = os.environ.copy()
            fixture["environment"] = environment
            result = self._run_wrapper(
                fixture,
                child,
                child_arguments,
            )
            self.assertEqual(result.returncode, 37, result.stdout + result.stderr)
            self.assertEqual(os.environ, parent_before)
            self.assertFalse(side_effect.exists())

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
                captured_environment["EMSDK_PYTHON"],
                str(
                    (
                        root
                        / "emsdk-4.0.7"
                        / "python"
                        / "python.exe"
                    ).resolve()
                ),
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

    def test_lock_directories_cannot_escape_toolchain_root(self) -> None:
        def copy_probe(
            destination: Path,
            artifact: str | None = None,
            directory: str | None = None,
        ) -> tuple[Path, Path]:
            copied_probe = destination / "copied probe"
            copied_scripts = copied_probe / "scripts"
            copied_scripts.mkdir(parents=True)
            bootstrap = copied_scripts / BOOTSTRAP.name
            wrapper = copied_scripts / WRAPPER.name
            shutil.copy2(BOOTSTRAP, bootstrap)
            shutil.copy2(WRAPPER, wrapper)
            lock = json.loads(
                (PROBE / "toolchain-lock.json").read_text("utf-8")
            )
            if artifact is not None:
                lock["buildTools"][artifact]["directory"] = directory
            (copied_probe / "toolchain-lock.json").write_text(
                json.dumps(lock),
                encoding="utf-8",
            )
            return bootstrap, wrapper

        with tempfile.TemporaryDirectory(
            prefix="rg valid copied lock "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = fixture["root"]
            sandbox = fixture["sandbox"]
            assert isinstance(root, Path)
            assert isinstance(sandbox, Path)
            self._seed_complete(root)
            _, copied_wrapper = copy_probe(sandbox / "valid leaf")
            child, child_arguments = self._native_capture_command([])
            valid = self._run_wrapper(
                fixture,
                child,
                child_arguments,
                wrapper=copied_wrapper,
            )
            self.assertEqual(valid.returncode, 0, valid.stdout + valid.stderr)

        malicious_cases = (
            ("cmake", "rooted"),
            ("ninja", "dotdot"),
        )
        for artifact, attack in malicious_cases:
            for entrypoint in ("bootstrap", "wrapper"):
                with self.subTest(
                    artifact=artifact,
                    attack=attack,
                    entrypoint=entrypoint,
                ):
                    with tempfile.TemporaryDirectory(
                        prefix="rg malicious lock "
                    ) as temporary:
                        fixture = self._fixture(temporary)
                        root = fixture["root"]
                        sandbox = fixture["sandbox"]
                        assert isinstance(root, Path)
                        assert isinstance(sandbox, Path)
                        self._seed_complete(root)
                        outside = sandbox / "outside sentinel"
                        outside.mkdir()
                        outside_file = outside / "keep.bin"
                        outside_file.write_bytes(b"outside sentinel bytes")
                        similar = (
                            root
                            / (
                                "cmake-4.2.3-windows-x86_64"
                                ".bootstrap-tmp-user"
                            )
                        )
                        similar.mkdir()
                        similar_file = similar / "keep.bin"
                        similar_file.write_bytes(b"similar sentinel bytes")
                        bad_directory = (
                            str(outside.resolve())
                            if attack == "rooted"
                            else ".."
                        )
                        copied_bootstrap, copied_wrapper = copy_probe(
                            sandbox / f"{artifact} {attack}",
                            artifact,
                            bad_directory,
                        )
                        child = sandbox / "must not run.cmd"
                        write_launcher(child, "capture")
                        if entrypoint == "bootstrap":
                            result = self._run(
                                copied_bootstrap,
                                ["-ToolchainRoot", str(root)],
                                fixture["environment"],
                            )
                        else:
                            result = self._run_wrapper(
                                fixture,
                                child,
                                wrapper=copied_wrapper,
                            )
                        combined = result.stdout + result.stderr
                        self.assertNotEqual(result.returncode, 0, combined)
                        self.assertIn("safe leaf", combined)
                        self.assertEqual(
                            outside_file.read_bytes(),
                            b"outside sentinel bytes",
                        )
                        self.assertEqual(
                            similar_file.read_bytes(),
                            b"similar sentinel bytes",
                        )
                        self.assertFalse((root / "downloads").exists())
                        self.assertEqual(
                            self._events(fixture["events"]),
                            [],
                        )

    def test_batch_children_fail_closed_and_emscripten_uses_native_driver(
        self,
    ) -> None:
        wrapper_text = WRAPPER.read_text("utf-8")
        for forbidden in (
            "ConvertTo-CmdArgument",
            "ComSpec",
            "cmd.exe",
            "$commandLine",
            "Start-Process",
        ):
            self.assertNotIn(forbidden.casefold(), wrapper_text.casefold())
        self.assertNotRegex(wrapper_text, r"(?i)\bcall\b")
        self.assertIn("ProcessStartInfo", wrapper_text)
        self.assertIn("ArgumentList", wrapper_text)

        with tempfile.TemporaryDirectory(
            prefix="rg batch fail closed "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = fixture["root"]
            sandbox = fixture["sandbox"]
            assert isinstance(root, Path)
            assert isinstance(sandbox, Path)
            self._seed_complete(root)
            explicit = sandbox / "explicit unknown child.bat"
            search = sandbox / "batch child search"
            write_launcher(explicit, "explicit-batch-child")
            write_launcher(search / "path-batch-child.cmd", "path-batch-child")
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["PATH"] = (
                f"{search}{os.pathsep}{environment.get('PATH', '')}"
            )
            fixture["environment"] = environment

            for child in (str(explicit), "path-batch-child"):
                with self.subTest(batch_child=child):
                    result = self._run_wrapper(fixture, child)
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn(
                        "Batch child executables are not supported",
                        combined,
                    )
            events = self._events(fixture["events"])
            self.assertFalse(
                {
                    "explicit-batch-child",
                    "path-batch-child",
                }.intersection(str(event["tool"]) for event in events),
                events,
            )

        with tempfile.TemporaryDirectory(
            prefix="rg emscripten driver "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = fixture["root"]
            sandbox = fixture["sandbox"]
            assert isinstance(root, Path)
            assert isinstance(sandbox, Path)
            self._seed_complete(root)
            side_effect = sandbox / "emscripten side effect"
            arguments = self._hostile_arguments(side_effect)
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["TOOLCHAIN_DOUBLE_CHILD_EXIT"] = "37"
            fixture["environment"] = environment
            emscripten = (
                root / "emsdk-4.0.7" / "upstream" / "emscripten"
            )
            python = root / "emsdk-4.0.7" / "python" / "python.exe"
            commands = (
                ("em++", "em++-driver", emscripten / "em++.py"),
                (
                    str(emscripten / "em++.cmd"),
                    "em++-driver",
                    emscripten / "em++.py",
                ),
                ("emcc", "emcc-driver", emscripten / "emcc.py"),
                (
                    str(emscripten / "emcc.cmd"),
                    "emcc-driver",
                    emscripten / "emcc.py",
                ),
            )
            for child, expected_tool, expected_driver in commands:
                with self.subTest(emscripten_child=child):
                    result = self._run_wrapper(
                        fixture,
                        child,
                        arguments,
                    )
                    self.assertEqual(
                        result.returncode,
                        37,
                        result.stdout + result.stderr,
                    )
                    drivers = [
                        event
                        for event in self._events(fixture["events"])
                        if event["tool"] == expected_tool
                    ]
                    self.assertTrue(drivers)
                    driver = drivers[-1]
                    self.assertEqual(driver["args"], arguments)
                    self.assertEqual(
                        Path(str(driver["source"])).resolve(),
                        expected_driver.resolve(),
                    )
                    self.assertEqual(
                        Path(str(driver["runtime"])).resolve(),
                        python.resolve(),
                    )
                    self.assertTrue(driver["ignore_environment"])
                    self.assertFalse(side_effect.exists())

        failure_cases = (
            "missing-python",
            "outside-python",
            "missing-driver",
            "outside-driver",
        )
        for failure_case in failure_cases:
            with self.subTest(emscripten_failure=failure_case):
                with tempfile.TemporaryDirectory(
                    prefix="rg emscripten fail closed "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = fixture["root"]
                    sandbox = fixture["sandbox"]
                    assert isinstance(root, Path)
                    assert isinstance(sandbox, Path)
                    self._seed_complete(root)
                    emsdk = root / "emsdk-4.0.7"
                    emscripten = emsdk / "upstream" / "emscripten"
                    environment_script = emsdk / "emsdk_env.ps1"
                    expected_error = "EMSDK_PYTHON"
                    if failure_case == "missing-python":
                        (emsdk / "python" / "python.exe").unlink()
                    elif failure_case == "outside-python":
                        lines = environment_script.read_text("utf-8").splitlines()
                        lines[1] = (
                            f"$env:EMSDK_PYTHON = '{Path(sys.executable).resolve()}'"
                        )
                        environment_script.write_text(
                            "\n".join((*lines, "")),
                            encoding="utf-8",
                        )
                    elif failure_case == "missing-driver":
                        (emscripten / "em++.py").unlink()
                        expected_error = "Emscripten driver"
                    else:
                        driver = emscripten / "em++.py"
                        outside = sandbox / "outside em++.py"
                        outside.write_text(
                            "raise SystemExit(73)\n",
                            encoding="utf-8",
                        )
                        driver.unlink()
                        driver.symlink_to(outside)
                        expected_error = "Emscripten driver"

                    result = self._run_wrapper(
                        fixture,
                        "em++",
                        ["--version"],
                    )
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn(expected_error, combined)
                    self.assertFalse(
                        any(
                            event["tool"] == "em++-driver"
                            for event in self._events(fixture["events"])
                        )
                    )


if __name__ == "__main__":
    unittest.main()
