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
sys.path.insert(
    0,
    str(Path(__file__).resolve().parents[1] / "scripts"),
)

import emscripten_cache_identity
from _toolchain_process_double import (
    EMSDK_COMMIT,
    FAIL_ONCE_EXIT,
    PORT_CMAKE_ENTRY,
    VCPKG_COMMIT,
    seed_emscripten_cache,
    seed_build_tools,
    seed_repository,
    seed_vcpkg_port_cmake,
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


def _python_import_closure(root: Path) -> dict[str, object]:
    files = {
        path.relative_to(root).as_posix(): path.read_bytes()
        for path in root.rglob("*")
        if path.is_file() and path.suffix.casefold() == ".py"
    }
    payload = _payload_identity(files)
    return {
        field: payload[field]
        for field in (
            "algorithm",
            "fileCount",
            "totalBytes",
            "inventorySha256",
            "aggregateSha256",
        )
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
        state = sandbox / "vcpkg state with spaces"
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
        shutil.copy2(
            PROBE / "scripts" / "audit_emscripten_response_files.py",
            copied_scripts / "audit_emscripten_response_files.py",
        )
        shutil.copy2(
            PROBE / "scripts" / "emscripten_cache_identity.py",
            copied_scripts / "emscripten_cache_identity.py",
        )
        shutil.copy2(
            PROBE / "scripts" / "prewarm_emscripten_cache.py",
            copied_scripts / "prewarm_emscripten_cache.py",
        )
        shutil.copy2(
            PROBE / "scripts" / "invoke_emscripten_driver.py",
            copied_scripts / "invoke_emscripten_driver.py",
        )

        exemplar = sandbox / "payload exemplar"
        exemplar_emsdk = exemplar / "emsdk-4.0.7"
        seed_repository(exemplar_emsdk, "emsdk", EMSDK_COMMIT)
        seed_build_tools(exemplar)
        exemplar_cache = seed_emscripten_cache(exemplar)
        port_archive, port_installation = seed_vcpkg_port_cmake(
            exemplar / "vcpkg state"
        )
        PORT_CMAKE_ENTRY["sha512"] = hashlib.sha512(
            port_archive.read_bytes()
        ).hexdigest()
        exemplar_vcpkg = exemplar / "vcpkg-a0400024"
        seed_repository(exemplar_vcpkg, "vcpkg", VCPKG_COMMIT)
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
        lock["gateTools"] = {
            "adapterSha256": hashlib.sha256(
                (
                    copied_scripts
                    / "invoke_emscripten_driver.py"
                ).read_bytes()
            ).hexdigest(),
            "responseAuditorSha256": hashlib.sha256(
                (
                    copied_scripts
                    / "audit_emscripten_response_files.py"
                ).read_bytes()
            ).hexdigest(),
        }
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
                "bootstrapScript": "emsdk.py",
                "bootstrapScriptSha256": hashlib.sha256(
                    (exemplar_emsdk / "emsdk.py").read_bytes()
                ).hexdigest(),
                "cLauncher": "upstream/emscripten/emcc.bat",
                "cLauncherSha256": hashlib.sha256(
                    (
                        exemplar_emsdk
                        / "upstream"
                        / "emscripten"
                        / "emcc.bat"
                    ).read_bytes()
                ).hexdigest(),
                "cxxLauncher": "upstream/emscripten/em++.bat",
                "cxxLauncherSha256": hashlib.sha256(
                    (
                        exemplar_emsdk
                        / "upstream"
                        / "emscripten"
                        / "em++.bat"
                    ).read_bytes()
                ).hexdigest(),
                "driverApi": {
                    "emccPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "emcc.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "emxxPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "em++.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "emarLauncherSha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "emar.bat"
                        ).read_bytes()
                    ).hexdigest(),
                    "emarPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "emar.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "emranlibLauncherSha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "emranlib.bat"
                        ).read_bytes()
                    ).hexdigest(),
                    "emranlibPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "emranlib.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "sharedPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "tools"
                            / "shared.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "responseFilePySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "tools"
                            / "response_file.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "configPySha256": hashlib.sha256(
                        (
                            exemplar_emsdk
                            / "upstream"
                            / "emscripten"
                            / "tools"
                            / "config.py"
                        ).read_bytes()
                    ).hexdigest(),
                    "pythonImportClosure": _python_import_closure(
                        exemplar_emsdk / "upstream" / "emscripten"
                    ),
                },
                "releaseManifest": {
                    "path": "emscripten-releases-tags.json",
                    "sha256": hashlib.sha256(
                        release_manifest.read_bytes()
                    ).hexdigest(),
                },
                "nodeExecutable": "node/node.exe",
                "nodeExecutableSha256": hashlib.sha256(
                    (exemplar_emsdk / "node" / "node.exe").read_bytes()
                ).hexdigest(),
                "pythonExecutable": "python/python.exe",
                "payload": {
                    **emscripten_payload,
                    "roots": roots,
                    "excludedPrefixes": [],
                    "excludedSegments": [],
                    "excludedSuffixes": [],
                },
                "cache": {
                    "directory": "emscripten-cache-4.0.7",
                    "initializer": (
                        "prewarm_emscripten_cache.py -> "
                        "embuilder.py build SYSTEM"
                    ),
                    "prewarmCores": 4,
                    "compilerPathPrefixMap": {
                        "injection": (
                            "tracked-python-get_base_cflags-adapter"
                        ),
                        "flag": "-ffile-prefix-map",
                        "systemLibsSha256": (
                            hashlib.sha256(
                                (
                                    exemplar_emsdk
                                    / "upstream"
                                    / "emscripten"
                                    / "tools"
                                    / "system_libs.py"
                                ).read_bytes()
                            ).hexdigest()
                        ),
                        "target": "/emsdk/cache",
                    },
                    "frozenEnvironment": "EM_FROZEN_CACHE=1",
                    "volatileProducts": [
                        "sanity.txt",
                        "symbol_lists/*.json",
                    ],
                    "payload": emscripten_cache_identity.generate_identity(
                        exemplar_cache
                    ),
                },
            }
        )
        python_root = exemplar_emsdk / "python"
        python_files = {
            path.relative_to(python_root).as_posix(): path.read_bytes()
            for path in python_root.rglob("*")
            if path.is_file()
            and path.name != ".emsdk_version"
        }
        python_archive = root / "downloads" / "python-fixture.zip"
        _write_zip(python_archive, python_files)
        lock["emscripten"]["bootstrapPython"] = {
            "url": f"https://fixture.invalid/{python_archive.name}",
            "archiveFile": python_archive.name,
            "sha256": hashlib.sha256(
                python_archive.read_bytes()
            ).hexdigest(),
            "installationDirectory": "python",
            "executable": "python.exe",
            "executableSha256": hashlib.sha256(
                (python_root / "python.exe").read_bytes()
            ).hexdigest(),
            "payload": {
                **_payload_identity(python_files),
                "stripPrefix": "",
            },
            "allowedRuntimePrefixes": [],
            "allowedRuntimeFiles": [".emsdk_version"],
        }
        port_files = {
            path.relative_to(port_installation).as_posix(): path.read_bytes()
            for path in port_installation.rglob("*")
            if path.is_file()
        }
        lock["vcpkg"].update(
            {
                "bootstrapLauncher": "bootstrap-vcpkg.bat",
                "bootstrapLauncherSha256": hashlib.sha256(
                    (
                        exemplar_vcpkg
                        / "bootstrap-vcpkg.bat"
                    ).read_bytes()
                ).hexdigest(),
                "bootstrapScript": "scripts/bootstrap.ps1",
                "bootstrapScriptSha256": hashlib.sha256(
                    (
                        exemplar_vcpkg
                        / "scripts"
                        / "bootstrap.ps1"
                    ).read_bytes()
                ).hexdigest(),
                "toolMetadata": "scripts/vcpkg-tool-metadata.txt",
                "toolMetadataSha256": hashlib.sha256(
                    (
                        exemplar_vcpkg
                        / "scripts"
                        / "vcpkg-tool-metadata.txt"
                    ).read_bytes()
                ).hexdigest(),
                "toolReleaseTag": "fixture-release",
                "toolUrl": (
                    "https://github.com/microsoft/vcpkg-tool/releases/"
                    "download/fixture-release/vcpkg.exe"
                ),
                "executable": "vcpkg.cmd",
                "executableSha256": hashlib.sha256(
                    (exemplar_vcpkg / "vcpkg.cmd").read_bytes()
                ).hexdigest(),
                "portBuildCMake": {
                    "version": "4.3.3",
                    "toolsManifest": "scripts/vcpkg-tools.json",
                    "toolsManifestSha256": hashlib.sha256(
                        (
                            exemplar_vcpkg
                            / "scripts"
                            / "vcpkg-tools.json"
                        ).read_bytes()
                    ).hexdigest(),
                    "url": PORT_CMAKE_ENTRY["url"],
                    "sha512": PORT_CMAKE_ENTRY["sha512"],
                    "archiveBytes": port_archive.stat().st_size,
                    "archiveFile": PORT_CMAKE_ENTRY["archive"],
                    "installationDirectory": "cmake-4.3.3-windows",
                    "executable": PORT_CMAKE_ENTRY["executable"],
                    "executableSha256": hashlib.sha256(
                        (
                            port_installation
                            / PORT_CMAKE_ENTRY["executable"]
                        ).read_bytes()
                    ).hexdigest(),
                    "payload": {
                        **_payload_identity(port_files),
                        "stripPrefix": "",
                    },
                },
            }
        )
        for name, exemplar_root, prefix in (
            (
                "emscripten",
                exemplar_emsdk,
                f"emsdk-{EMSDK_COMMIT}",
            ),
            (
                "vcpkg",
                exemplar_vcpkg,
                f"vcpkg-{VCPKG_COMMIT}",
            ),
        ):
            source_files = {
                path.relative_to(exemplar_root).as_posix(): path.read_bytes()
                for path in exemplar_root.rglob("*")
                if path.is_file()
                and not (
                    name == "emscripten"
                    and path.relative_to(exemplar_root).parts[0] == "python"
                )
            }
            archive = (
                root
                / "downloads"
                / f"{name}-source-fixture.zip"
            )
            _write_zip(
                archive,
                {
                    f"{prefix}/{relative}": content
                    for relative, content in source_files.items()
                },
            )
            source_contract = {
                "url": f"https://fixture.invalid/{archive.name}",
                "archiveFile": archive.name,
                "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "payload": {
                    **_payload_identity(source_files),
                    "stripPrefix": prefix,
                },
                "allowedRuntimePrefixes": (
                    ["python", "upstream"]
                    if name == "emscripten"
                    else []
                ),
                "allowedRuntimeFiles": [],
            }
            if name == "emscripten":
                lock["emscripten"]["sourceArchive"] = source_contract
            else:
                lock["vcpkg"]["sourceArchive"] = source_contract
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
                "TOOLCHAIN_DOUBLE_VCPKG_STATE": str(state),
                "TOOLCHAIN_DOUBLE_PORT_CMAKE_SHA512": (
                    PORT_CMAKE_ENTRY["sha512"]
                ),
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
            "state": state,
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
                "-VcpkgStateRoot",
                str(fixture["state"]),
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
                [
                    "-ToolchainRoot",
                    str(fixture["root"]),
                    "-VcpkgStateRoot",
                    str(fixture["state"]),
                ],
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
            "port-cmake",
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
        seed_emscripten_cache(root)
        seed_vcpkg_port_cmake(root.parent / "vcpkg state with spaces")

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

    def test_authenticated_source_member_drift_fails_closed(self) -> None:
        wrong = "1111111111111111111111111111111111111111"
        repositories = (
            ("emsdk", "emsdk-4.0.7"),
            ("vcpkg", "vcpkg-a0400024"),
        )
        for kind, directory in repositories:
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
                                [
                                    "-ToolchainRoot",
                                    str(root),
                                    "-VcpkgStateRoot",
                                    str(fixture["state"]),
                                ],
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
                        self.assertIn(
                            f"{kind} source file '.fixture-head' "
                            "SHA-256 drifted",
                            combined,
                        )
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
                        self.assertEqual(git_operations, [], events)

    def test_bootstrap_recovers_owned_partial_source_extraction(self) -> None:
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
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            result = self._run(
                Path(str(fixture["bootstrap"])),
                [
                    "-ToolchainRoot",
                    str(root),
                    "-VcpkgStateRoot",
                    str(fixture["state"]),
                    "-InitializeEmscriptenCache",
                ],
                environment,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
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
            git_events = [
                event
                for event in events
                if event["tool"] == "git"
            ]
            self.assertEqual(git_events, [], events)
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
                ],
            )
            pinned_python = (
                root / "emsdk-4.0.7" / "python" / "python.exe"
            ).resolve()
            for event in events:
                if event["tool"] != "emsdk":
                    continue
                self.assertEqual(
                    Path(str(event["runtime"])).resolve(),
                    pinned_python,
                )
                self.assertTrue(event["ignore_environment"])
                self.assertTrue(event["no_user_site"])
                self.assertTrue(event["bytecode_disabled"])

    def test_frozen_cache_requires_explicit_initialization_and_is_reused(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg explicit frozen cache "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = Path(str(fixture["root"]))
            self._seed_complete(root)
            cache = root / "emscripten-cache-4.0.7"
            shutil.rmtree(cache)
            arguments = [
                "-ToolchainRoot",
                str(root),
                "-VcpkgStateRoot",
                str(fixture["state"]),
            ]
            environment = dict(fixture["environment"])
            environment.update(
                {
                    "CCC_OVERRIDE_OPTIONS": "+-fexceptions",
                    "EMCC_CFLAGS": "-fexceptions",
                    "EM_COMPILER_WRAPPER": "T:\\poison wrapper",
                    "EM_COMPILER_WRAPPER2": "T:\\poison wrapper 2",
                    "CPATH": "T:\\poison include",
                    "C_INCLUDE_PATH": "T:\\poison c include",
                    "CPLUS_INCLUDE_PATH": "T:\\poison cxx include",
                    "CFLAGS": "-fexceptions",
                    "CXXFLAGS": "-fexceptions",
                    "LDFLAGS": "-fexceptions",
                    "NODE_OPTIONS": "--require=T:\\poison.js",
                    "NODE_PATH": "T:\\poison node modules",
                    "PYTHONNOUSERSITE": "0",
                }
            )

            refused = self._run(
                Path(str(fixture["bootstrap"])),
                arguments,
                environment,
            )
            self.assertNotEqual(
                refused.returncode,
                0,
                refused.stdout + refused.stderr,
            )
            self.assertIn(
                "-InitializeEmscriptenCache",
                refused.stdout + refused.stderr,
            )
            self.assertFalse(cache.exists())
            self.assertFalse(
                any(
                    event["tool"] == "embuilder"
                    for event in self._events(Path(str(fixture["events"])))
                )
            )

            initialized = self._run(
                Path(str(fixture["bootstrap"])),
                [*arguments, "-InitializeEmscriptenCache"],
                environment,
            )
            self.assertEqual(
                initialized.returncode,
                0,
                initialized.stdout + initialized.stderr,
            )
            self.assertTrue(cache.is_dir())
            self.assertFalse((cache / "sanity.txt").exists())
            self.assertFalse((cache / "symbol_lists").exists())
            self.assertFalse(
                (root / "emscripten-cache-4.0.7.bootstrap-tmp").exists()
            )
            first_events = self._events(Path(str(fixture["events"])))
            self.assertEqual(
                sum(event["tool"] == "embuilder" for event in first_events),
                1,
            )

            reused = self._run(
                Path(str(fixture["bootstrap"])),
                arguments,
                environment,
            )
            self.assertEqual(
                reused.returncode,
                0,
                reused.stdout + reused.stderr,
            )
            second_events = self._events(Path(str(fixture["events"])))
            self.assertEqual(
                sum(event["tool"] == "embuilder" for event in second_events),
                1,
            )

    def test_frozen_cache_is_authenticated_before_and_after_child(
        self,
    ) -> None:
        for mutation in ("before", "during"):
            with self.subTest(mutation=mutation):
                with tempfile.TemporaryDirectory(
                    prefix="rg frozen cache tamper "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    self._seed_complete(root)
                    cache_library = (
                        root
                        / "emscripten-cache-4.0.7"
                        / "sysroot"
                        / "lib"
                        / "wasm32-emscripten"
                        / "libc.a"
                    )
                    timestamps = (
                        cache_library.stat().st_atime_ns,
                        cache_library.stat().st_mtime_ns,
                    )
                    child, arguments = self._native_capture_command(
                        [mutation]
                    )
                    if mutation == "before":
                        content = cache_library.read_bytes()
                        cache_library.write_bytes(
                            bytes((content[0] ^ 0x01,))
                            + content[1:]
                        )
                        os.utime(cache_library, ns=timestamps)
                    else:
                        environment = fixture["environment"]
                        assert isinstance(environment, dict)
                        environment["TOOLCHAIN_DOUBLE_MUTATE_CACHE"] = "1"

                    result = self._run_wrapper(
                        fixture,
                        child,
                        arguments,
                    )
                    self.assertNotEqual(
                        result.returncode,
                        0,
                        result.stdout + result.stderr,
                    )
                    self.assertIn(
                        "Emscripten frozen cache",
                        result.stdout + result.stderr,
                    )
                    events = self._events(Path(str(fixture["events"])))
                    captures = [
                        event
                        for event in events
                        if event["tool"] == "capture-environment"
                    ]
                    self.assertEqual(
                        len(captures),
                        0 if mutation == "before" else 1,
                        events,
                    )
                    if mutation == "before":
                        self._assert_no_tool_execution(fixture)

    def test_runtime_and_port_cmake_are_locked_for_child_lifetime(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg executable lifetime locks "
        ) as temporary:
            fixture = self._fixture(temporary)
            root = Path(str(fixture["root"]))
            state = Path(str(fixture["state"]))
            self._seed_complete(root)
            lock = json.loads(
                Path(str(fixture["lock"])).read_text("utf-8")
            )
            node = root / "emsdk-4.0.7" / lock["emscripten"][
                "nodeExecutable"
            ]
            port_cmake_contract = lock["vcpkg"]["portBuildCMake"]
            port_cmake = (
                state
                / "downloads"
                / "tools"
                / port_cmake_contract["installationDirectory"]
                / port_cmake_contract["executable"]
            )
            originals = {
                path: path.read_bytes()
                for path in (node, port_cmake)
            }
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["TOOLCHAIN_DOUBLE_MUTATE_LOCKED_PATHS"] = "1"
            child, arguments = self._native_capture_command(
                [str(node), str(port_cmake)]
            )

            result = self._run_wrapper(fixture, child, arguments)
            self.assertEqual(
                result.returncode,
                0,
                result.stdout + result.stderr,
            )
            events = self._events(Path(str(fixture["events"])))
            mutations = [
                event
                for event in events
                if event["tool"] == "locked-path-mutation"
            ]
            self.assertEqual(len(mutations), 1, events)
            outcomes = mutations[0]["outcomes"]
            self.assertEqual(len(outcomes), 2, outcomes)
            self.assertTrue(
                all(bool(outcome["denied"]) for outcome in outcomes),
                outcomes,
            )
            for path, content in originals.items():
                self.assertEqual(path.read_bytes(), content)

    def test_qualification_build_control_is_lifetime_locked(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg build control lifetime lock "
        ) as directory:
            build_control = Path(directory) / "build.ninja"
            build_control.write_text(
                "build selected: trusted-rule\n",
                encoding="utf-8",
            )
            script = "\n".join(
                (
                    "$ErrorActionPreference = 'Stop'",
                    f". {str(PROVENANCE)!r}",
                    f"$control = {str(build_control)!r}",
                    (
                        "$closure = Open-QualificationClosure "
                        "-Roots @() -Files @([PSCustomObject]@{"
                    ),
                    "  Logical = 'build-control/build.ninja'",
                    "  Path = $control",
                    "})",
                    "try {",
                    "  $start = [Diagnostics.ProcessStartInfo]::new()",
                    f"  $start.FileName = {str(Path(sys.executable).resolve())!r}",
                    "  $start.UseShellExecute = $false",
                    "  $start.RedirectStandardError = $true",
                    "  $start.ArgumentList.Add('-c')",
                    (
                        "  $start.ArgumentList.Add("
                        + repr(
                            "from pathlib import Path; "
                            f"Path({str(build_control)!r}).write_text("
                            "'forged', encoding='utf-8')"
                        )
                        + ")"
                    ),
                    "  $process = [Diagnostics.Process]::Start($start)",
                    "  $process.WaitForExit()",
                    "  $stderr = $process.StandardError.ReadToEnd()",
                    "  if ($process.ExitCode -eq 0) {",
                    "    throw 'Build-control mutation was not denied'",
                    "  }",
                    "} finally {",
                    "  foreach ($stream in @($closure.Streams)) {",
                    "    $stream.Dispose()",
                    "  }",
                    "}",
                    "Write-Output 'BUILD_CONTROL_MUTATION_DENIED=1'",
                )
            )
            result = subprocess.run(
                [
                    self.pwsh,
                    "-NoProfile",
                    "-NonInteractive",
                    "-Command",
                    script,
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(
                result.returncode,
                0,
                result.stdout + result.stderr,
            )
            self.assertIn(
                "BUILD_CONTROL_MUTATION_DENIED=1",
                result.stdout,
            )
            self.assertEqual(
                build_control.read_text("utf-8"),
                "build selected: trusted-rule\n",
            )
            wrapper_text = WRAPPER.read_text("utf-8")
            self.assertIn("build-control-manifest.txt", wrapper_text)
            self.assertIn('"build-control/$relative"', wrapper_text)
            self.assertLess(
                wrapper_text.index("build-control-manifest.txt"),
                wrapper_text.index("Open-QualificationClosure"),
            )
            self.assertLess(
                wrapper_text.index("Open-QualificationClosure"),
                wrapper_text.index(
                    "$childExitCode = Invoke-NativeProcess",
                ),
            )

    def test_source_tree_and_archive_drift_fail_before_tool_execution(
        self,
    ) -> None:
        cases = (
            ("emsdk", "emsdk-4.0.7", "extra"),
            ("emsdk", "emsdk-4.0.7", "archive"),
            ("vcpkg", "vcpkg-a0400024", "extra"),
            ("vcpkg", "vcpkg-a0400024", "archive"),
        )
        for kind, directory, mutation in cases:
            for entrypoint in ("bootstrap", "wrapper"):
                with self.subTest(
                    repository=kind,
                    mutation=mutation,
                    entrypoint=entrypoint,
                ):
                    with tempfile.TemporaryDirectory(
                        prefix="rg tracked toolchain drift "
                    ) as temporary:
                        fixture = self._fixture(temporary)
                        root = Path(str(fixture["root"]))
                        self._seed_complete(root)
                        if mutation == "extra":
                            marker = (
                                root / directory / "unmodeled-source-extra"
                            )
                            marker.write_text("dirty", encoding="ascii")
                            expected = (
                                f"{kind} unexpected file: "
                                "unmodeled-source-extra"
                            )
                        else:
                            lock = json.loads(
                                Path(str(fixture["lock"])).read_text("utf-8")
                            )
                            contract = (
                                lock["emscripten"]
                                if kind == "emsdk"
                                else lock["vcpkg"]
                            )
                            archive = (
                                root
                                / "downloads"
                                / contract["sourceArchive"]["archiveFile"]
                            )
                            content = archive.read_bytes()
                            archive.write_bytes(
                                content[:-1]
                                + bytes((content[-1] ^ 0x01,))
                            )
                            expected = (
                                f"{kind} source archive SHA-256 drifted"
                                if entrypoint == "bootstrap"
                                else "Build-tool archive SHA-256 drifted"
                            )

                        result = self._run_entrypoint(
                            fixture,
                            entrypoint,
                        )
                        combined = result.stdout + result.stderr
                        self.assertNotEqual(result.returncode, 0, combined)
                        self.assertIn(expected, combined)
                        self._assert_no_tool_execution(fixture)

    def test_source_metadata_case_and_rogue_python_fail_before_execution(
        self,
    ) -> None:
        mutations = (
            (
                "emsdk-git-metadata",
                "emsdk-4.0.7",
                "emsdk unexpected directory: .git",
            ),
            (
                "vcpkg-git-metadata",
                "vcpkg-a0400024",
                "vcpkg unexpected directory: .git",
            ),
            ("emsdk-source-case", "emsdk-4.0.7", "emsdk unexpected file"),
            (
                "rogue-preferred-python",
                "emsdk-4.0.7",
                "Emscripten",
            ),
        )
        for mutation, directory, expected in mutations:
            for entrypoint in ("bootstrap", "wrapper"):
                with self.subTest(
                    mutation=mutation,
                    entrypoint=entrypoint,
                ):
                    with tempfile.TemporaryDirectory(
                        prefix="rg exact source membership "
                    ) as temporary:
                        fixture = self._fixture(temporary)
                        root = Path(str(fixture["root"]))
                        self._seed_complete(root)
                        installation = root / directory
                        if mutation.endswith("git-metadata"):
                            metadata = installation / ".git" / "config"
                            metadata.parent.mkdir()
                            metadata.write_text(
                                "[core]\nrepositoryformatversion = 0\n",
                                encoding="utf-8",
                            )
                        elif mutation == "emsdk-source-case":
                            source = installation / "emsdk.py"
                            intermediate = installation / "emsdk-case-tmp.py"
                            source.rename(intermediate)
                            intermediate.rename(installation / "EMSDK.PY")
                        elif mutation == "rogue-preferred-python":
                            rogue = (
                                installation
                                / "python"
                                / "3.9.2-1_64bit"
                                / "python.exe"
                            )
                            rogue.parent.mkdir(parents=True)
                            rogue.write_bytes(
                                (installation / "python" / "python.exe").read_bytes()
                            )
                        else:
                            self.fail(f"unknown mutation: {mutation}")

                        result = self._run_entrypoint(fixture, entrypoint)
                        combined = result.stdout + result.stderr
                        self.assertNotEqual(result.returncode, 0, combined)
                        self.assertIn(expected, combined)
                        self._assert_no_tool_execution(fixture)

    def test_entrypoints_reject_reparse_provenance_helper_before_sourcing(
        self,
    ) -> None:
        for entrypoint in ("bootstrap", "wrapper"):
            with self.subTest(entrypoint=entrypoint):
                with tempfile.TemporaryDirectory(
                    prefix="rg reparse provenance helper "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    self._seed_complete(root)
                    copied_helper = (
                        Path(str(fixture["wrapper"])).parent
                        / PROVENANCE.name
                    )
                    copied_helper.unlink()
                    try:
                        copied_helper.symlink_to(PROVENANCE)
                    except OSError as error:
                        self.skipTest(
                            f"symbolic links unavailable: {error}"
                        )
                    result = self._run_entrypoint(fixture, entrypoint)
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn("reparse-point component", combined)
                    self._assert_no_tool_execution(fixture)

    def test_unmodeled_lookalikes_and_source_member_drift_fail_closed(
        self,
    ) -> None:
        for mutation in (
            "emsdk-bootstrap-lookalike",
            "vcpkg-bootstrap-lookalike",
            "emsdk-bootstrap-source-bytes",
            "vcpkg-bootstrap-source-bytes",
            "emcc-launcher-source-bytes",
            "emxx-launcher-source-bytes",
            "vcpkg-executable-bytes",
        ):
            with self.subTest(mutation=mutation):
                with tempfile.TemporaryDirectory(
                    prefix="rg exact launcher provenance "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    self._seed_complete(root)
                    lock_path = Path(str(fixture["lock"]))
                    lock = json.loads(lock_path.read_text("utf-8"))
                    if mutation == "emsdk-bootstrap-lookalike":
                        relative = "untracked-emsdk-bootstrap.cmd"
                        write_launcher(
                            root / "emsdk-4.0.7" / relative,
                            "emsdk",
                        )
                        lock["emscripten"]["bootstrapScript"] = relative
                        expected = (
                            "emsdk unexpected file: "
                            "untracked-emsdk-bootstrap.cmd"
                        )
                    elif mutation == "vcpkg-bootstrap-lookalike":
                        relative = "untracked-vcpkg-bootstrap.cmd"
                        write_launcher(
                            root / "vcpkg-a0400024" / relative,
                            "vcpkg-bootstrap",
                        )
                        lock["vcpkg"]["bootstrapLauncher"] = relative
                        expected = (
                            "vcpkg unexpected file: "
                            "untracked-vcpkg-bootstrap.cmd"
                        )
                    elif mutation in {
                        "emsdk-bootstrap-source-bytes",
                        "vcpkg-bootstrap-source-bytes",
                        "emcc-launcher-source-bytes",
                        "emxx-launcher-source-bytes",
                    }:
                        specifications = {
                            "emsdk-bootstrap-source-bytes": (
                                root / "emsdk-4.0.7" / "emsdk.py",
                                "emsdk source file 'emsdk.py' SHA-256 drifted",
                            ),
                            "vcpkg-bootstrap-source-bytes": (
                                root
                                / "vcpkg-a0400024"
                                / "bootstrap-vcpkg.bat",
                                (
                                    "vcpkg source file 'bootstrap-vcpkg.bat' "
                                    "SHA-256 drifted"
                                ),
                            ),
                            "emcc-launcher-source-bytes": (
                                root
                                / "emsdk-4.0.7"
                                / "upstream"
                                / "emscripten"
                                / "emcc.bat",
                                (
                                    "emsdk source file "
                                    "'upstream/emscripten/emcc.bat' "
                                    "SHA-256 drifted"
                                ),
                            ),
                            "emxx-launcher-source-bytes": (
                                root
                                / "emsdk-4.0.7"
                                / "upstream"
                                / "emscripten"
                                / "em++.bat",
                                (
                                    "emsdk source file "
                                    "'upstream/emscripten/em++.bat' "
                                    "SHA-256 drifted"
                                ),
                            ),
                        }
                        executable, expected = specifications[mutation]
                        timestamps = (
                            executable.stat().st_atime_ns,
                            executable.stat().st_mtime_ns,
                        )
                        content = executable.read_bytes()
                        executable.write_bytes(
                            bytes((content[0] ^ 0x01,)) + content[1:]
                        )
                        os.utime(executable, ns=timestamps)
                    elif mutation == "vcpkg-executable-bytes":
                        executable = (
                            root
                            / "vcpkg-a0400024"
                            / lock["vcpkg"]["executable"]
                        )
                        timestamps = (
                            executable.stat().st_atime_ns,
                            executable.stat().st_mtime_ns,
                        )
                        content = executable.read_bytes()
                        executable.write_bytes(
                            bytes((content[0] ^ 0x01,))
                            + content[1:]
                        )
                        os.utime(executable, ns=timestamps)
                        expected = (
                            "vcpkg source file 'vcpkg.cmd' SHA-256 drifted"
                        )
                    else:
                        self.fail(f"unknown mutation: {mutation}")
                    lock_path.write_text(json.dumps(lock), encoding="utf-8")

                    child, arguments = self._native_capture_command([])
                    result = self._run_wrapper(fixture, child, arguments)
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
            ("port-cmake-support-bytes", "wrapper"),
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
                    port_root = (
                        Path(str(fixture["state"]))
                        / "downloads"
                        / "tools"
                        / lock["vcpkg"]["portBuildCMake"][
                            "installationDirectory"
                        ]
                    )

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
                    elif mutation == "port-cmake-support-bytes":
                        support = (
                            port_root
                            / "cmake-4.3.3-windows-x86_64"
                            / "share"
                            / "cmake-4.3"
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
            helper_directory = Path(str(fixture["wrapper"])).parent
            for module in ("hashlib.py", "json.py"):
                (helper_directory / module).write_text(
                    'raise RuntimeError("adjacent stdlib shadow imported")\n',
                    encoding="utf-8",
                )
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
                    "EM_FROZEN_CACHE": "0",
                    "EMCC_CFLAGS": "-fexceptions",
                    "CCC_OVERRIDE_OPTIONS": "+-fexceptions",
                    "CPATH": "T:\\poison include",
                    "C_INCLUDE_PATH": "T:\\poison c include",
                    "CPLUS_INCLUDE_PATH": "T:\\poison cxx include",
                    "OBJC_INCLUDE_PATH": "T:\\poison objc include",
                    "LIBRARY_PATH": "T:\\poison library",
                    "COMPILER_PATH": "T:\\poison compiler",
                    "GCC_EXEC_PREFIX": "T:\\poison gcc",
                    "SDKROOT": "T:\\poison sdk",
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
                    "VCPKG_MAX_CONCURRENCY": "33",
                    "NODE_OPTIONS": "--require=T:\\poison.js",
                    "NODE_PATH": "T:\\poison node modules",
                    "PYTHONNOUSERSITE": "0",
                    "SOURCE_DATE_EPOCH": "9999999999",
                    "QT_RCC_SOURCE_DATE_OVERRIDE": "8888888888",
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
                in {
                    "em++-driver",
                    "vcpkg",
                    "cmake",
                    "ninja",
                    "port-cmake",
                }
            }
            self.assertEqual(
                set(version_events),
                {
                    "em++-driver",
                    "vcpkg",
                    "cmake",
                    "ninja",
                    "port-cmake",
                },
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
                "port-cmake": (
                    Path(str(fixture["state"]))
                    / "downloads"
                    / "tools"
                    / "cmake-4.3.3-windows"
                ),
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
                        / "emscripten-cache-4.0.7"
                    ).resolve()
                ),
            )
            self.assertEqual(
                captured_environment["PYTHONDONTWRITEBYTECODE"],
                "1",
            )
            self.assertEqual(
                captured_environment["PYTHONNOUSERSITE"],
                "1",
            )
            self.assertEqual(
                captured_environment["EM_FROZEN_CACHE"],
                "1",
            )
            self.assertEqual(
                captured_environment["SOURCE_DATE_EPOCH"],
                "1782488244",
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
                "CCC_OVERRIDE_OPTIONS",
                "CPATH",
                "C_INCLUDE_PATH",
                "CPLUS_INCLUDE_PATH",
                "OBJC_INCLUDE_PATH",
                "LIBRARY_PATH",
                "COMPILER_PATH",
                "GCC_EXEC_PREFIX",
                "SDKROOT",
                "NODE_OPTIONS",
                "NODE_PATH",
                "QT_RCC_SOURCE_DATE_OVERRIDE",
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
                captured_environment["VCPKG_MAX_CONCURRENCY"],
                "8",
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

    def test_node_aliases_use_authenticated_runtime_without_path_injection(
        self,
    ) -> None:
        for requested in ("node", "node.exe"):
            with self.subTest(requested=requested):
                with tempfile.TemporaryDirectory(
                    prefix="rg pinned node alias "
                ) as temporary:
                    fixture = self._fixture(temporary)
                    root = Path(str(fixture["root"]))
                    sandbox = Path(str(fixture["sandbox"]))
                    self._seed_complete(root)
                    pinned_node = (
                        root
                        / "emsdk-4.0.7"
                        / "node"
                        / "node.exe"
                    ).resolve()
                    poison = sandbox / "ambient node poison"
                    write_launcher(poison / "node.cmd", "poison-node")
                    environment = fixture["environment"]
                    assert isinstance(environment, dict)
                    environment["PATH"] = (
                        f"{poison}{os.pathsep}"
                        f"{environment.get('PATH', '')}"
                    )
                    environment["PYTHONHOME"] = sys.prefix
                    fixture["environment"] = environment
                    capture = sandbox / f"{requested}.json"
                    script = sandbox / "capture-node-runtime.py"
                    script.write_text(
                        "\n".join(
                            (
                                "import json",
                                "import os",
                                "import sys",
                                "from pathlib import Path",
                                "Path(sys.argv[1]).write_text(",
                                "    json.dumps({",
                                "        'runtime': sys.executable,",
                                "        'arguments': sys.argv[2:],",
                                "        'path': os.environ['PATH'],",
                                "    }),",
                                "    encoding='utf-8',",
                                ")",
                                "",
                            )
                        ),
                        encoding="utf-8",
                    )
                    arguments = [
                        str(script),
                        str(capture),
                        "argument with spaces",
                        "--option-like",
                    ]

                    result = self._run_wrapper(
                        fixture,
                        requested,
                        arguments,
                    )
                    self.assertEqual(
                        result.returncode,
                        0,
                        result.stdout + result.stderr,
                    )
                    payload = json.loads(capture.read_text("utf-8"))
                    self.assertEqual(
                        Path(payload["runtime"]).resolve(),
                        pinned_node,
                    )
                    self.assertEqual(payload["arguments"], arguments[2:])
                    path_entries = {
                        os.path.normcase(str(Path(entry).resolve()))
                        for entry in str(payload["path"]).split(os.pathsep)
                        if entry
                    }
                    self.assertNotIn(
                        os.path.normcase(str(pinned_node.parent)),
                        path_entries,
                    )
                    self.assertFalse(
                        any(
                            event["tool"] == "poison-node"
                            for event in self._events(
                                Path(str(fixture["events"]))
                            )
                        )
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
            (Path(str(fixture["wrapper"])).parent / "shlex.py").write_text(
                'raise RuntimeError("adjacent shlex shadow imported")\n',
                encoding="utf-8",
            )
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
            shutil.copy2(
                PROBE / "scripts" / "audit_emscripten_response_files.py",
                copied_scripts / "audit_emscripten_response_files.py",
            )
            shutil.copy2(
                PROBE / "scripts" / "emscripten_cache_identity.py",
                copied_scripts / "emscripten_cache_identity.py",
            )
            shutil.copy2(
                PROBE / "scripts" / "invoke_emscripten_driver.py",
                copied_scripts / "invoke_emscripten_driver.py",
            )
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
            arguments = [
                argument
                for argument in self._hostile_arguments(side_effect)
                if argument not in {"-BinaryCache", "child binary cache"}
            ]
            environment = fixture["environment"]
            assert isinstance(environment, dict)
            environment["TOOLCHAIN_DOUBLE_CHILD_EXIT"] = "37"
            fixture["environment"] = environment
            emscripten = (
                root / "emsdk-4.0.7" / "upstream" / "emscripten"
            )
            python = root / "emsdk-4.0.7" / "python" / "python.exe"
            commands = (
                ("em++", "em++-driver", emscripten / "emcc.py"),
                (
                    str(emscripten / "em++.bat"),
                    "em++-driver",
                    emscripten / "emcc.py",
                ),
                ("emcc", "emcc-driver", emscripten / "emcc.py"),
                (
                    str(emscripten / "emcc.bat"),
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
                    self.assertEqual(
                        Path(str(driver["adapter_runtime"])).resolve(),
                        python.resolve(),
                    )
                    self.assertTrue(driver["adapter_ignore_environment"])
                    self.assertTrue(driver["adapter_no_user_site"])
                    self.assertTrue(driver["adapter_bytecode_disabled"])
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
                    expected_error = (
                        "Emscripten payload"
                        if failure_case
                        in {"missing-python", "tampered-runtime-metadata"}
                        else "emsdk source file"
                    )
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
