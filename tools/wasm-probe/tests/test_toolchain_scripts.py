from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile
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
PROVENANCE = PROBE / "scripts" / "Toolchain-Provenance.ps1"


def _payload_identity(files: dict[str, bytes]) -> dict[str, object]:
    paths = sorted(files)
    directories = sorted(
        {
            "/".join(path.split("/")[:index])
            for path in paths
            for index in range(1, len(path.split("/")))
        }
    )
    inventory = hashlib.sha256(
        "".join(f"{path}\n" for path in paths).encode("utf-8")
    ).hexdigest()
    directory_inventory = hashlib.sha256(
        "".join(f"{path}/\n" for path in directories).encode("utf-8")
    ).hexdigest()
    aggregate = hashlib.sha256(
        "".join(
            f"{path}\0{hashlib.sha256(files[path]).hexdigest()}\n"
            for path in paths
        ).encode("utf-8")
    ).hexdigest()
    return {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "fileCount": len(paths),
        "directoryCount": len(directories),
        "totalBytes": sum(len(content) for content in files.values()),
        "inventorySha256": inventory,
        "directoryInventorySha256": directory_inventory,
        "aggregateSha256": aggregate,
    }


def _write_zip(archive: Path, entries: dict[str, bytes]) -> None:
    archive.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(
        archive,
        "w",
        compression=zipfile.ZIP_DEFLATED,
    ) as bundle:
        for name in sorted(entries):
            info = zipfile.ZipInfo(name, date_time=(2020, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            bundle.writestr(info, entries[name])


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
        copied_probe = sandbox / "copied probe"
        copied_scripts = copied_probe / "scripts"
        copied_scripts.mkdir(parents=True)
        bootstrap = copied_scripts / BOOTSTRAP.name
        wrapper = copied_scripts / WRAPPER.name
        shutil.copy2(BOOTSTRAP, bootstrap)
        shutil.copy2(WRAPPER, wrapper)
        shutil.copy2(PROVENANCE, copied_scripts / PROVENANCE.name)

        exemplar = sandbox / "payload exemplar"
        exemplar_emsdk = exemplar / "emsdk-4.0.7"
        seed_repository(exemplar_emsdk, "emsdk", EMSDK_COMMIT)
        seed_build_tools(exemplar)
        cmake_directory = "cmake-4.2.3-windows-x86_64"
        ninja_directory = "ninja-1.13.2-win"
        cmake_files = {
            "bin/cmake.cmd": (
                exemplar / cmake_directory / "bin" / "cmake.cmd"
            ).read_bytes(),
            "share/cmake-4.2/Modules/FixtureSupport.cmake": (
                exemplar
                / cmake_directory
                / "share"
                / "cmake-4.2"
                / "Modules"
                / "FixtureSupport.cmake"
            ).read_bytes(),
        }
        ninja_files = {
            "ninja.cmd": (
                exemplar / ninja_directory / "ninja.cmd"
            ).read_bytes(),
        }
        cmake_archive = root / "downloads" / f"{cmake_directory}.zip"
        ninja_archive = root / "downloads" / f"{ninja_directory}.zip"
        _write_zip(
            cmake_archive,
            {
                f"{cmake_directory}/{path}": content
                for path, content in cmake_files.items()
            },
        )
        _write_zip(ninja_archive, ninja_files)
        lock = json.loads(
            (PROBE / "toolchain-lock.json").read_text("utf-8")
        )
        release_manifest = (
            exemplar_emsdk / "emscripten-releases-tags.json"
        )
        roots = [
            ".emscripten",
            "emscripten-releases-tags.json",
            "upstream/bin",
            "upstream/emscripten",
            "node",
            "python",
        ]
        files: dict[str, bytes] = {}
        for relative_root in roots:
            root_path = exemplar_emsdk / relative_root
            candidates = (
                [root_path]
                if root_path.is_file()
                else [path for path in root_path.rglob("*") if path.is_file()]
            )
            for path in candidates:
                relative = path.relative_to(exemplar_emsdk).as_posix()
                files[relative] = path.read_bytes()
        emscripten_payload = _payload_identity(files)
        lock["emscripten"].update(
            {
                "releaseManifest": {
                    "path": "emscripten-releases-tags.json",
                    "sha256": hashlib.sha256(
                        release_manifest.read_bytes()
                    ).hexdigest(),
                },
                "nodeExecutable": "node/node.exe",
                "pythonExecutable": "python/python.exe",
                "payload": {
                    **emscripten_payload,
                    "roots": roots,
                    "excludedPrefixes": [],
                    "excludedSegments": [],
                    "excludedSuffixes": [],
                },
            }
        )
        for name, directory, archive, artifact_files, strip_prefix in (
            (
                "cmake",
                cmake_directory,
                cmake_archive,
                cmake_files,
                cmake_directory,
            ),
            (
                "ninja",
                ninja_directory,
                ninja_archive,
                ninja_files,
                "",
            ),
        ):
            artifact = lock["buildTools"][name]
            executable = (
                exemplar / directory / "bin" / "cmake.cmd"
                if name == "cmake"
                else exemplar / directory / "ninja.cmd"
            )
            artifact.update(
                {
                    "archiveFile": archive.name,
                    "sha256": hashlib.sha256(
                        archive.read_bytes()
                    ).hexdigest(),
                    "executableSha256": hashlib.sha256(
                        executable.read_bytes()
                    ).hexdigest(),
                    "payload": {
                        **_payload_identity(artifact_files),
                        "stripPrefix": strip_prefix,
                    },
                }
            )
        (copied_probe / "toolchain-lock.json").write_text(
            json.dumps(lock),
            encoding="utf-8",
        )
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
            "bootstrap": bootstrap,
            "wrapper": wrapper,
            "lock": copied_probe / "toolchain-lock.json",
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
        wrapper: Path | None = None,
    ) -> subprocess.CompletedProcess[str]:
        environment = dict(fixture["environment"])
        selected_wrapper = wrapper or Path(str(fixture["wrapper"]))
        return self._run(
            selected_wrapper,
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

    def _run_entrypoint(
        self,
        fixture: dict[str, object],
        entrypoint: str,
    ) -> subprocess.CompletedProcess[str]:
        if entrypoint == "bootstrap":
            return self._run(
                Path(str(fixture["bootstrap"])),
                ["-ToolchainRoot", str(fixture["root"])],
                dict(fixture["environment"]),
            )
        if entrypoint == "wrapper":
            sandbox = Path(str(fixture["sandbox"]))
            child = sandbox / "must not execute.cmd"
            write_launcher(child, "capture")
            return self._run_wrapper(fixture, child)
        raise AssertionError(f"unexpected entrypoint: {entrypoint}")

    def _assert_no_tool_execution(
        self,
        fixture: dict[str, object],
    ) -> None:
        forbidden = {
            "emsdk",
            "vcpkg-bootstrap",
            "em++",
            "em++-driver",
            "emcc",
            "emcc-driver",
            "vcpkg",
            "cmake",
            "ninja",
            "capture",
            "capture-environment",
        }
        events = self._events(Path(str(fixture["events"])))
        self.assertFalse(
            forbidden.intersection(
                str(event["tool"]) for event in events
            ),
            events,
        )

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
                                Path(str(fixture["bootstrap"])),
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
                            "em++-driver",
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
                        allowed_git_operations = {
                            ("rev-parse", "HEAD"),
                            (
                                "diff",
                                "--quiet",
                                "--no-ext-diff",
                                "--no-textconv",
                                "--ignore-submodules=all",
                                "--",
                            ),
                            (
                                "diff",
                                "--cached",
                                "--quiet",
                                "--no-ext-diff",
                                "--no-textconv",
                                "--ignore-submodules=all",
                                "HEAD",
                                "--",
                            ),
                        }
                        self.assertTrue(
                            all(
                                tuple(operation[2:])
                                in allowed_git_operations
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
                Path(str(fixture["bootstrap"])),
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
                Path(str(fixture["bootstrap"])),
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

    def test_tracked_repository_drift_fails_before_tool_execution(self) -> None:
        repositories = (
            ("emsdk-4.0.7", ".fixture-worktree-dirty", "unstaged"),
            ("emsdk-4.0.7", ".fixture-index-dirty", "staged"),
            ("vcpkg-a0400024", ".fixture-worktree-dirty", "unstaged"),
            ("vcpkg-a0400024", ".fixture-index-dirty", "staged"),
        )
        for directory, marker_name, expected in repositories:
            for entrypoint in ("bootstrap", "wrapper"):
                with self.subTest(
                    repository=directory,
                    state=expected,
                    entrypoint=entrypoint,
                ):
                    with tempfile.TemporaryDirectory(
                        prefix="rg tracked toolchain drift "
                    ) as temporary:
                        fixture = self._fixture(temporary)
                        root = Path(str(fixture["root"]))
                        self._seed_complete(root)
                        marker = root / directory / marker_name
                        marker.write_text("dirty", encoding="ascii")

                        result = self._run_entrypoint(
                            fixture,
                            entrypoint,
                        )
                        combined = result.stdout + result.stderr
                        self.assertNotEqual(result.returncode, 0, combined)
                        self.assertIn(expected, combined)
                        self._assert_no_tool_execution(fixture)

    def test_build_tool_tree_or_archive_drift_fails_before_versions(
        self,
    ) -> None:
        cases = (
            ("cmake-support-bytes", "bootstrap"),
            ("cmake-support-bytes", "wrapper"),
            ("ninja-archive-bytes", "bootstrap"),
            ("ninja-archive-bytes", "wrapper"),
            ("missing-cmake-archive", "wrapper"),
            ("extra-cmake-file", "wrapper"),
            ("extra-ninja-directory", "wrapper"),
        )
        for mutation, entrypoint in cases:
            with self.subTest(mutation=mutation, entrypoint=entrypoint):
                with tempfile.TemporaryDirectory(
                    prefix="rg build tool provenance "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    self._seed_complete(root)
                    lock = json.loads(
                        Path(str(fixture["lock"])).read_text("utf-8")
                    )
                    cmake_root = (
                        root / lock["buildTools"]["cmake"]["directory"]
                    )
                    ninja_root = (
                        root / lock["buildTools"]["ninja"]["directory"]
                    )
                    downloads = root / "downloads"

                    if mutation == "cmake-support-bytes":
                        support = (
                            cmake_root
                            / "share"
                            / "cmake-4.2"
                            / "Modules"
                            / "FixtureSupport.cmake"
                        )
                        timestamps = (
                            support.stat().st_atime_ns,
                            support.stat().st_mtime_ns,
                        )
                        original = support.read_bytes()
                        support.write_bytes(
                            original[:-1]
                            + bytes((original[-1] ^ 0x01,))
                        )
                        os.utime(support, ns=timestamps)
                    elif mutation == "ninja-archive-bytes":
                        archive = (
                            downloads
                            / lock["buildTools"]["ninja"]["archiveFile"]
                        )
                        with archive.open("ab") as stream:
                            stream.write(b"tampered")
                    elif mutation == "missing-cmake-archive":
                        (
                            downloads
                            / lock["buildTools"]["cmake"]["archiveFile"]
                        ).unlink()
                    elif mutation == "extra-cmake-file":
                        (cmake_root / "unexpected-module.cmake").write_text(
                            "unexpected",
                            encoding="ascii",
                        )
                    elif mutation == "extra-ninja-directory":
                        (ninja_root / "unexpected").mkdir()
                    else:
                        self.fail(f"unknown mutation: {mutation}")

                    result = self._run_entrypoint(fixture, entrypoint)
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertRegex(
                        combined,
                        r"(?i)(archive|installed|installation)",
                    )
                    self._assert_no_tool_execution(fixture)

    def test_bytecode_normalization_is_narrow_and_precedes_versions(
        self,
    ) -> None:
        cases = (
            ("non-bytecode-in-cache", "Non-bytecode file", True),
            ("pyc-outside-cache", ".pyc outside exact", True),
            ("cache-reparse", "reparse point", True),
        )

        with tempfile.TemporaryDirectory(
            prefix="rg benign bytecode cache "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = Path(str(fixture["root"]))
            self._seed_complete(root)
            cache = (
                root
                / "emsdk-4.0.7"
                / "python"
                / "__pycache__"
            )
            cache.mkdir()
            (cache / "fixture.cpython-39.pyc").write_bytes(
                b"generated bytecode"
            )
            child, arguments = self._native_capture_command(["normalized"])
            result = self._run_wrapper(fixture, child, arguments)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertFalse(cache.exists())

        for mutation, expected, retained in cases:
            with self.subTest(mutation=mutation):
                with tempfile.TemporaryDirectory(
                    prefix="rg hostile bytecode cache "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    sandbox = Path(str(fixture["sandbox"]))
                    self._seed_complete(root)
                    python_root = root / "emsdk-4.0.7" / "python"
                    cache = python_root / "__pycache__"
                    preserved: Path
                    if mutation == "non-bytecode-in-cache":
                        cache.mkdir()
                        preserved = cache / "not-bytecode.txt"
                        preserved.write_text("must survive", encoding="ascii")
                    elif mutation == "pyc-outside-cache":
                        preserved = python_root / "rogue.pyc"
                        preserved.write_bytes(b"must survive")
                    elif mutation == "cache-reparse":
                        target = sandbox / "outside cache target"
                        target.mkdir()
                        preserved = target / "must-survive.pyc"
                        preserved.write_bytes(b"must survive")
                        created = subprocess.run(
                            [
                                "cmd.exe",
                                "/d",
                                "/c",
                                "mklink",
                                "/J",
                                str(cache),
                                str(target),
                            ],
                            capture_output=True,
                            text=True,
                            check=False,
                        )
                        self.assertEqual(
                            created.returncode,
                            0,
                            created.stdout + created.stderr,
                        )
                    else:
                        self.fail(f"unknown mutation: {mutation}")

                    result = self._run_entrypoint(fixture, "wrapper")
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn(expected, combined)
                    if retained:
                        self.assertTrue(preserved.exists())
                    self._assert_no_tool_execution(fixture)

    def test_authenticated_archive_rejects_unsafe_layouts_before_versions(
        self,
    ) -> None:
        cases = (
            ("dotdot", "../outside.txt", 0o100644),
            ("backslash", r"unsafe\outside.txt", 0o100644),
            ("reserved", "AUX.txt", 0o100644),
            (
                "case-collision",
                "SHARE/cmake-4.2/modules/fixturesupport.cmake",
                0o100644,
            ),
            ("symlink", "unsafe-link", 0o120777),
        )
        for label, relative, unix_mode in cases:
            with self.subTest(layout=label):
                with tempfile.TemporaryDirectory(
                    prefix="rg unsafe build archive "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    self._seed_complete(root)
                    lock_path = Path(str(fixture["lock"]))
                    lock = json.loads(lock_path.read_text("utf-8"))
                    cmake = lock["buildTools"]["cmake"]
                    archive = (
                        root / "downloads" / cmake["archiveFile"]
                    )
                    prefix = cmake["payload"]["stripPrefix"]
                    entry = zipfile.ZipInfo(
                        f"{prefix}/{relative}",
                        date_time=(2020, 1, 1, 0, 0, 0),
                    )
                    entry.compress_type = zipfile.ZIP_DEFLATED
                    entry.external_attr = unix_mode << 16
                    with zipfile.ZipFile(
                        archive,
                        "a",
                        compression=zipfile.ZIP_DEFLATED,
                    ) as bundle:
                        bundle.writestr(entry, b"unsafe")
                    cmake["sha256"] = hashlib.sha256(
                        archive.read_bytes()
                    ).hexdigest()
                    lock_path.write_text(
                        json.dumps(lock),
                        encoding="utf-8",
                    )
                    outside = Path(str(fixture["sandbox"])) / "outside.txt"

                    result = self._run_entrypoint(fixture, "wrapper")
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertRegex(
                        combined,
                        r"(?i)(archive|relative path|collision|reparse)",
                    )
                    self.assertFalse(outside.exists())
                    self._assert_no_tool_execution(fixture)

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
                    "EMSDK_NODE": "T:\\poison node",
                    "EMSDK_PYTHON": "T:\\poison python",
                    "EMSCRIPTEN_ROOT": "T:\\poison emscripten",
                    "EMSCRIPTEN_VERSION": "4.0.7",
                    "EM_CACHE": "T:\\poison cache",
                    "EM_CONFIG": "T:\\poison config",
                    "EMCC_CFLAGS": "-fexceptions",
                    "CFLAGS": "-fexceptions",
                    "CXXFLAGS": "-fexceptions",
                    "CPPFLAGS": "-fexceptions",
                    "LDFLAGS": "-fexceptions",
                    "EM_COMPILER_WRAPPER": "T:\\poison wrapper",
                    "EMMAKEN_CFLAGS": "-fexceptions",
                    "GIT_CONFIG_GLOBAL": "T:\\poison git config",
                    "CMAKE_TOOLCHAIN_FILE": "T:\\poison toolchain",
                    "VCPKG_OVERLAY_PORTS": "T:\\poison ports",
                    "PKG_CONFIG_PATH": "T:\\poison pkgconfig",
                    "CCACHE_PREFIX": "T:\\poison ccache",
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
                        Path(str(fixture["wrapper"])),
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
                if event["tool"]
                in {"em++-driver", "vcpkg", "cmake", "ninja"}
            }
            self.assertEqual(
                set(version_events),
                {"em++-driver", "vcpkg", "cmake", "ninja"},
            )
            expected_roots = {
                "em++-driver": (
                    root / "emsdk-4.0.7" / "upstream" / "emscripten"
                ),
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
                captured_environment["EMSDK_NODE"],
                str(
                    (
                        root
                        / "emsdk-4.0.7"
                        / "node"
                        / "node.exe"
                    ).resolve()
                ),
            )
            self.assertEqual(
                captured_environment["EM_CONFIG"],
                str((root / "emsdk-4.0.7" / ".emscripten").resolve()),
            )
            self.assertEqual(
                captured_environment["EM_CACHE"],
                str(
                    (
                        root
                        / "emsdk-4.0.7"
                        / "upstream"
                        / "emscripten"
                        / "cache"
                    ).resolve()
                ),
            )
            self.assertEqual(
                captured_environment["PYTHONDONTWRITEBYTECODE"],
                "1",
            )
            for scrubbed in (
                "EMCC_CFLAGS",
                "CFLAGS",
                "CXXFLAGS",
                "CPPFLAGS",
                "LDFLAGS",
                "EM_COMPILER_WRAPPER",
                "EMMAKEN_CFLAGS",
                "GIT_CONFIG_GLOBAL",
                "CMAKE_TOOLCHAIN_FILE",
                "VCPKG_OVERLAY_PORTS",
                "PKG_CONFIG_PATH",
                "CCACHE_PREFIX",
            ):
                self.assertIsNone(
                    captured_environment[scrubbed],
                    (scrubbed, captured_environment),
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

    def test_direct_emscripten_rejects_legacy_exception_and_asyncify(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg forbidden emscripten arguments "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = Path(str(fixture["root"]))
            sandbox = Path(str(fixture["sandbox"]))
            self._seed_complete(root)
            response = sandbox / "forbidden response.rsp"
            response.write_text(
                "object.o -fexceptions -sJSPI=1\n",
                encoding="utf-8",
            )
            cases = (
                ["-fexceptions"],
                ["-s", "ASYNCIFY"],
                ["-sWASM_EXCEPTIONS=0"],
                [f"@{response}"],
            )
            for arguments in cases:
                with self.subTest(arguments=arguments):
                    result = self._run_wrapper(
                        fixture,
                        "em++",
                        arguments,
                    )
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn("Forbidden Emscripten", combined)

            drivers = [
                event
                for event in self._events(Path(str(fixture["events"])))
                if event["tool"] in {"em++-driver", "emcc-driver"}
            ]
            self.assertTrue(drivers)
            self.assertTrue(
                all(event["args"] == ["--version"] for event in drivers),
                drivers,
            )

    def test_lock_directories_cannot_escape_toolchain_root(self) -> None:
        def copy_probe(
            fixture: dict[str, object],
            destination: Path,
            artifact: str | None = None,
            directory: str | None = None,
        ) -> tuple[Path, Path]:
            copied_probe = destination / "copied probe"
            copied_scripts = copied_probe / "scripts"
            copied_scripts.mkdir(parents=True)
            bootstrap = copied_scripts / BOOTSTRAP.name
            wrapper = copied_scripts / WRAPPER.name
            shutil.copy2(Path(str(fixture["bootstrap"])), bootstrap)
            shutil.copy2(Path(str(fixture["wrapper"])), wrapper)
            shutil.copy2(PROVENANCE, copied_scripts / PROVENANCE.name)
            lock = json.loads(
                Path(str(fixture["lock"])).read_text("utf-8")
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
            _, copied_wrapper = copy_probe(
                fixture,
                sandbox / "valid leaf",
            )
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
                        retained_archives = {
                            path.name: path.read_bytes()
                            for path in (root / "downloads").iterdir()
                            if path.is_file()
                        }
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
                            fixture,
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
                        self.assertEqual(
                            {
                                path.name: path.read_bytes()
                                for path in (root / "downloads").iterdir()
                                if path.is_file()
                            },
                            retained_archives,
                        )
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
            nested = (
                root
                / "emsdk-4.0.7"
                / "upstream"
                / "emscripten"
                / "nested"
            )
            nested_emxx = nested / "em++.cmd"
            nested_emcc = nested / "emcc.bat"
            write_launcher(explicit, "explicit-batch-child")
            write_launcher(search / "path-batch-child.cmd", "path-batch-child")
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["PATH"] = (
                f"{search}{os.pathsep}{environment.get('PATH', '')}"
            )
            fixture["environment"] = environment

            for child in (
                str(explicit),
                "path-batch-child",
            ):
                with self.subTest(batch_child=child):
                    result = self._run_wrapper(fixture, child)
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn(
                        "Batch child executables are not supported",
                        combined,
                    )
            write_launcher(nested_emxx, "nested-em++")
            write_launcher(nested_emcc, "nested-emcc")
            for child in (str(nested_emxx), str(nested_emcc)):
                with self.subTest(tampered_sdk_batch_child=child):
                    result = self._run_wrapper(fixture, child)
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn("Emscripten payload", combined)
            events = self._events(fixture["events"])
            self.assertFalse(
                {
                    "explicit-batch-child",
                    "path-batch-child",
                    "nested-em++",
                    "nested-emcc",
                    "emcc-driver",
                }.intersection(str(event["tool"]) for event in events),
                events,
            )
            self.assertTrue(
                all(
                    event["args"] == ["--version"]
                    for event in events
                    if event["tool"] == "em++-driver"
                ),
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
            "tampered-runtime-metadata",
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
                    expected_error = "Emscripten payload"
                    if failure_case == "missing-python":
                        (emsdk / "python" / "python.exe").unlink()
                    elif failure_case == "tampered-runtime-metadata":
                        metadata = next(
                            (emsdk / "python").glob("python*._pth")
                        )
                        metadata.write_text(
                            metadata.read_text("utf-8") + "# tampered\n",
                            encoding="utf-8",
                        )
                    elif failure_case == "missing-driver":
                        (emscripten / "em++.py").unlink()
                    else:
                        driver = emscripten / "em++.py"
                        outside = sandbox / "outside em++.py"
                        outside.write_text(
                            "raise SystemExit(73)\n",
                            encoding="utf-8",
                        )
                        driver.unlink()
                        driver.symlink_to(outside)

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
