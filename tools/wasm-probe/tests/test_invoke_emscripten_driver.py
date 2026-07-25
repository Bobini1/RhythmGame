from __future__ import annotations

import argparse
import contextlib
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

import invoke_emscripten_driver as adapter


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def python_import_closure(root: Path) -> dict[str, object]:
    modules = sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.casefold() == ".py"
    )
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for path in modules:
        relative = path.relative_to(root).as_posix()
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        total_bytes += len(data)
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{digest}\n".encode("utf-8"))
    return {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "fileCount": len(modules),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }


def qualification_environment() -> dict[str, str]:
    return {
        "RHYTHMGAME_WASM_QUALIFICATION": "1",
        "RHYTHMGAME_WASM_QUALIFICATION_ALGORITHM": (
            "sha256-logical-null-bytes-null-digest-lf-v1"
        ),
        "RHYTHMGAME_WASM_QUALIFICATION_FILE_COUNT": "1",
        "RHYTHMGAME_WASM_QUALIFICATION_TOTAL_BYTES": "1",
        "RHYTHMGAME_WASM_QUALIFICATION_INVENTORY_SHA256": "1" * 64,
        "RHYTHMGAME_WASM_QUALIFICATION_AGGREGATE_SHA256": "2" * 64,
    }


class InvokeEmscriptenDriverTest(unittest.TestCase):
    def _fixture(
        self,
        root: Path,
        *,
        system_exit_code: int | None | object = ...,
    ) -> tuple[argparse.Namespace, Path, Path]:
        emsdk = root / "emsdk root"
        emscripten = emsdk / "upstream" / "emscripten"
        tools = emscripten / "tools"
        tools.mkdir(parents=True)
        (tools / "__init__.py").write_text("", encoding="utf-8")
        shared = tools / "shared.py"
        shared.write_text("run_via_emxx = False\n", encoding="utf-8")
        response_file = tools / "response_file.py"
        response_file.write_text("fixture = True\n", encoding="utf-8")
        config = tools / "config.py"
        config.write_text("fixture = True\n", encoding="utf-8")
        system_libs = tools / "system_libs.py"
        system_libs.write_text(
            "authenticated_value = 'system-libs-original'\n",
            encoding="utf-8",
        )
        diagnostics = tools / "diagnostics.py"
        diagnostics.write_text(
            "authenticated_value = 'diagnostics-original'\n",
            encoding="utf-8",
        )

        capture = root / "captured arguments.json"
        emcc = emscripten / "emcc.py"
        emcc.write_text(
            "\n".join(
                (
                    "import json",
                    "from pathlib import Path",
                    "from tools import diagnostics, system_libs",
                    "",
                    "def main(args):",
                    "    assert system_libs.authenticated_value == (",
                    "        'system-libs-original'",
                    "    )",
                    "    assert diagnostics.authenticated_value == (",
                    "        'diagnostics-original'",
                    "    )",
                    f"    capture = Path({str(capture)!r})",
                    "    capture.write_text(",
                    "        json.dumps(args),",
                    "        encoding='utf-8',",
                    "    )",
                    (
                        "    return 0"
                        if system_exit_code is ...
                        else f"    raise SystemExit({system_exit_code!r})"
                    ),
                    "",
                )
            ),
            encoding="utf-8",
        )
        launcher = emscripten / "emcc.bat"
        launcher.write_bytes(b"@echo off\r\nexit /b 99\r\n")

        em_config = emsdk / ".emscripten"
        em_config.write_text(
            "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'\n",
            encoding="utf-8",
        )
        cache = root / "frozen cache"
        cache.mkdir()
        auditor = root / "audit_emscripten_response_files.py"
        auditor.write_bytes(
            (SCRIPTS / "audit_emscripten_response_files.py").read_bytes()
        )
        lock = root / "toolchain-lock.json"
        lock.write_text(
            json.dumps(
                {
                    "gateTools": {
                        "responseAuditorSha256": sha256(auditor),
                    },
                    "emscripten": {
                        "cLauncher": (
                            "upstream/emscripten/emcc.bat"
                        ),
                        "cLauncherSha256": sha256(launcher),
                        "cxxLauncher": (
                            "upstream/emscripten/em++.bat"
                        ),
                        "cxxLauncherSha256": "0" * 64,
                        "driverApi": {
                            "emccPySha256": sha256(emcc),
                            "sharedPySha256": sha256(shared),
                            "responseFilePySha256": sha256(response_file),
                            "configPySha256": sha256(config),
                            "pythonImportClosure": python_import_closure(
                                emscripten
                            ),
                        },
                    },
                }
            ),
            encoding="utf-8",
        )
        response = root / "compile response.rsp"
        response.write_text(
            "object.o -sJSPI=1\n",
            encoding="utf-8",
        )
        parsed = argparse.Namespace(
            lock=lock,
            auditor=auditor,
            emscripten_root=emscripten,
            driver_kind="emcc",
            em_config=em_config,
            em_config_sha256=sha256(em_config),
            cache_root=cache,
            arguments=[str(launcher), f"@{response}"],
        )
        return parsed, response, capture

    def test_successful_system_exit_finalizes_compile_sidecar(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter successful system exit "
        ) as directory:
            root = Path(directory)
            parsed, _, _ = self._fixture(root, system_exit_code=0)
            source = root / "source.cpp"
            output = root / "object.o"
            source.write_text("int fixture;\n", encoding="utf-8")
            parsed.arguments = [
                parsed.arguments[0],
                "-o",
                str(output),
                "-c",
                str(source),
            ]
            prepared = {
                "output": output,
                "sidecar": output.with_name(
                    output.name + ".rg-compile-inputs.json"
                ),
            }
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "EM_CONFIG": str(parsed.em_config),
                        "EM_CACHE": str(parsed.cache_root),
                        "EM_FROZEN_CACHE": "1",
                        **qualification_environment(),
                    },
                    clear=True,
                ),
                mock.patch.object(
                    adapter,
                    "_prepare_compile_dependency_closure",
                    return_value=prepared,
                ) as prepare,
                mock.patch.object(
                    adapter,
                    "_finish_compile_dependency_closure",
                ) as finish,
            ):
                self.assertEqual(adapter.run_driver(parsed), 0)
            prepare.assert_called_once()
            finish.assert_called_once_with(
                mock.ANY,
                prepared,
            )

    def test_nonzero_system_exit_is_preserved(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter failed system exit "
        ) as directory:
            parsed, _, _ = self._fixture(
                Path(directory),
                system_exit_code=7,
            )
            with (
                mock.patch.dict(
                    os.environ,
                    {
                        "EM_CONFIG": str(parsed.em_config),
                        "EM_CACHE": str(parsed.cache_root),
                        "EM_FROZEN_CACHE": "1",
                    },
                    clear=True,
                ),
                self.assertRaises(SystemExit) as raised,
            ):
                adapter.run_driver(parsed)
            self.assertEqual(raised.exception.code, 7)

    def test_response_file_is_read_once_before_a_swap(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter response swap "
        ) as directory:
            parsed, response, capture = self._fixture(Path(directory))

            def swap_response() -> None:
                response.write_text(
                    "--shell-file malicious.html\n",
                    encoding="utf-8",
                )

            with mock.patch.dict(
                os.environ,
                {
                    "EM_CONFIG": str(parsed.em_config),
                    "EM_CACHE": str(parsed.cache_root),
                    "EM_FROZEN_CACHE": "1",
                },
                clear=True,
            ):
                self.assertEqual(
                    adapter.run_driver(
                        parsed,
                        after_expand=swap_response,
                    ),
                    0,
                )
            effective = json.loads(capture.read_text("utf-8"))
            self.assertEqual(
                effective[1:],
                ["object.o", "-sJSPI=1"],
            )
            self.assertNotIn("--shell-file", effective)

    def test_driver_swap_after_response_audit_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter driver swap "
        ) as directory:
            parsed, _, _ = self._fixture(Path(directory))
            emcc = parsed.emscripten_root / "emcc.py"

            def swap_driver() -> None:
                emcc.write_text(
                    "def main(args):\n    return 73\n",
                    encoding="utf-8",
                )

            with mock.patch.dict(
                os.environ,
                {
                    "EM_CONFIG": str(parsed.em_config),
                    "EM_CACHE": str(parsed.cache_root),
                    "EM_FROZEN_CACHE": "1",
                },
                clear=True,
            ):
                with self.assertRaisesRegex(
                    adapter.DriverError,
                    "Emscripten Python import .* drifted",
                ):
                    adapter.run_driver(
                        parsed,
                        after_expand=swap_driver,
                    )

    def test_auditor_swap_after_hash_uses_locked_verified_bytes(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter auditor barrier "
        ) as directory:
            parsed, _, capture = self._fixture(Path(directory))

            def swap_auditor() -> None:
                try:
                    parsed.auditor.write_text(
                        "raise RuntimeError('malicious auditor executed')\n",
                        encoding="utf-8",
                    )
                except OSError:
                    pass

            with mock.patch.dict(
                os.environ,
                {
                    "EM_CONFIG": str(parsed.em_config),
                    "EM_CACHE": str(parsed.cache_root),
                    "EM_FROZEN_CACHE": "1",
                },
                clear=True,
            ):
                self.assertEqual(
                    adapter.run_driver(
                        parsed,
                        after_auditor_authenticate=swap_auditor,
                    ),
                    0,
                )
            self.assertTrue(capture.is_file())

    def test_driver_swap_after_hash_uses_locked_verified_bytes(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver adapter import barrier "
        ) as directory:
            parsed, _, capture = self._fixture(Path(directory))
            emcc = parsed.emscripten_root / "emcc.py"

            def swap_driver() -> None:
                try:
                    emcc.write_text(
                        "\n".join(
                            (
                                "def main(args):",
                                "    raise RuntimeError(",
                                "        'malicious driver executed'",
                                "    )",
                                "",
                            )
                        ),
                        encoding="utf-8",
                    )
                except OSError:
                    pass

            with mock.patch.dict(
                os.environ,
                {
                    "EM_CONFIG": str(parsed.em_config),
                    "EM_CACHE": str(parsed.cache_root),
                    "EM_FROZEN_CACHE": "1",
                },
                clear=True,
            ):
                self.assertEqual(
                    adapter.run_driver(
                        parsed,
                        after_driver_authenticate=swap_driver,
                    ),
                    0,
                )
            self.assertTrue(capture.is_file())

    def test_transitive_import_swaps_use_locked_verified_bytes(self) -> None:
        for relative in (
            "tools/system_libs.py",
            "tools/diagnostics.py",
        ):
            with self.subTest(module=relative):
                with tempfile.TemporaryDirectory(
                    prefix="rg driver adapter transitive barrier "
                ) as directory:
                    parsed, _, capture = self._fixture(Path(directory))
                    module = parsed.emscripten_root / relative

                    def swap_transitive() -> None:
                        try:
                            module.write_text(
                                "raise RuntimeError("
                                "'malicious transitive module executed'"
                                ")\n",
                                encoding="utf-8",
                            )
                        except OSError:
                            pass

                    with mock.patch.dict(
                        os.environ,
                        {
                            "EM_CONFIG": str(parsed.em_config),
                            "EM_CACHE": str(parsed.cache_root),
                            "EM_FROZEN_CACHE": "1",
                        },
                        clear=True,
                    ):
                        self.assertEqual(
                            adapter.run_driver(
                                parsed,
                                after_driver_authenticate=swap_transitive,
                            ),
                            0,
                        )
                    self.assertTrue(capture.is_file())

    def test_selected_link_injects_bound_build_id_and_locks_inputs(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg driver selected link binding "
        ) as directory:
            root = Path(directory)
            parsed, response, capture = self._fixture(root)
            object_file = root / "selected object.o"
            archive = root / "selected archive.a"
            object_file.write_bytes(b"\0asm\1\0\0\0selected object")
            original_archive = b"!<arch>\n"
            archive.write_bytes(original_archive)
            qualification = {
                "algorithm": (
                    "sha256-logical-null-bytes-null-digest-lf-v1"
                ),
                "fileCount": 1,
                "totalBytes": 1,
                "inventorySha256": "1" * 64,
                "aggregateSha256": "2" * 64,
            }
            sidecar = object_file.with_name(
                object_file.name + ".rg-compile-inputs.json"
            )
            sidecar.write_text(
                json.dumps(
                    {
                        "schemaVersion": 1,
                        "algorithm": (
                            "sha256-compile-dependency-files-json-v1"
                        ),
                        "qualification": qualification,
                        "driverKind": "emcc",
                        "output": (
                            f"build-output/{object_file.name}"
                        ),
                        "arguments": [],
                        "dependencyDiscovery": {
                            "preScanMethod": "emscripten-M",
                            "actualCompileMethod": "MD-MF",
                            "exactPathSetMatch": True,
                            "dependencyCount": 1,
                        },
                        "dependencies": [
                            {
                                "path": "repo-input/source.c",
                                "bytes": 1,
                                "sha256": "3" * 64,
                            }
                        ],
                        "closureSha256": "4" * 64,
                        "outputBytes": object_file.stat().st_size,
                        "outputSha256": sha256(object_file),
                    }
                )
                + "\n",
                encoding="utf-8",
            )
            response.write_text(
                " ".join(
                    (
                        f'"{object_file}"',
                        f'"{archive}"',
                        "-lembind",
                        "-o",
                        "RhythmGameWasmProbe.js",
                    )
                )
                + "\n",
                encoding="utf-8",
            )

            def swap_archive() -> None:
                try:
                    archive.write_bytes(b"!<arch>\nmalicious replacement")
                except OSError:
                    pass

            with mock.patch.dict(
                os.environ,
                {
                    "EM_CONFIG": str(parsed.em_config),
                    "EM_CACHE": str(parsed.cache_root),
                    "EM_FROZEN_CACHE": "1",
                    **qualification_environment(),
                },
                clear=True,
            ):
                previous_cwd = Path.cwd()
                try:
                    os.chdir(root)
                    self.assertEqual(
                        adapter.run_driver(
                            parsed,
                            after_driver_authenticate=swap_archive,
                        ),
                        0,
                    )
                finally:
                    os.chdir(previous_cwd)
            effective = json.loads(capture.read_text("utf-8"))[1:]
            build_ids = [
                argument
                for argument in effective
                if argument.startswith("-Wl,--build-id=0x")
            ]
            self.assertEqual(len(build_ids), 1, effective)
            self.assertRegex(
                build_ids[0],
                r"^-Wl,--build-id=0x[0-9a-f]{64}$",
            )
            if os.name == "nt":
                self.assertEqual(archive.read_bytes(), original_archive)

    def test_make_dependency_parser_preserves_windows_paths_and_spaces(
        self,
    ) -> None:
        content = (
            br"qualified-object: T:\root\source.cpp "
            + b"\\\n"
            + br"  T:\root\include\header.h "
            + b"\\\n"
            + br"  T:\root\path\ with\ spaces\header.h"
            + b"\n"
        )
        self.assertEqual(
            adapter._parse_make_dependencies(content),
            [
                r"T:\root\source.cpp",
                r"T:\root\include\header.h",
                r"T:\root\path with spaces\header.h",
            ],
        )

    def test_compiler_search_root_must_be_modeled(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg search root audit "
        ) as directory:
            root = Path(directory)
            build = root / "build"
            allowed = root / "allowed"
            outside = root / "outside"
            build.mkdir()
            allowed.mkdir()
            outside.mkdir()
            with self.assertRaisesRegex(
                adapter.DriverError,
                "escaped modeled roots",
            ):
                adapter._compiler_search_paths(
                    ["-I", str(outside)],
                    build,
                    (allowed,),
                )

    def test_compile_dependency_scan_locks_and_records_exact_files(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg compile dependency closure "
        ) as directory:
            repo = Path(directory)
            source_root = repo / "tools" / "wasm-probe"
            build = source_root / "build" / "wasm-release"
            target = (
                repo
                / ".wasm-vcpkg"
                / "installed"
                / "wasm32-emscripten-rg"
            )
            emsdk = repo / ".toolchains" / "emsdk"
            emscripten = emsdk / "upstream" / "emscripten"
            cache = repo / ".toolchains" / "cache"
            for path in (build, target, emscripten, cache):
                path.mkdir(parents=True)
            source = source_root / "src" / "source.cpp"
            header = source_root / "src" / "header.h"
            source.parent.mkdir()
            source.write_text('#include "header.h"\n', encoding="utf-8")
            header.write_text("original\n", encoding="utf-8")
            manifest = source_root / "input-manifest.txt"
            manifest.write_text(
                "tools/wasm-probe/src/header.h\n"
                "tools/wasm-probe/src/source.cpp\n",
                encoding="utf-8",
            )
            driver = emscripten / "em++.py"
            driver.write_text("# fixture\n", encoding="utf-8")
            output = build / "CMakeFiles" / "source.cpp.o"
            actual_depfile = output.with_suffix(output.suffix + ".d")
            output.parent.mkdir()

            def dependency_scan(
                command: list[str],
                **_: object,
            ) -> subprocess.CompletedProcess[bytes]:
                depfile = Path(command[command.index("-MF") + 1])

                def escaped(path: Path) -> str:
                    return str(path).replace(" ", r"\ ")

                depfile.write_text(
                    "qualified-object: "
                    f"{escaped(source)} \\\n"
                    f"  {escaped(header)}\n",
                    encoding="utf-8",
                )
                return subprocess.CompletedProcess(command, 0, b"", b"")

            qualification = {
                "algorithm": (
                    "sha256-logical-null-bytes-null-digest-lf-v1"
                ),
                "fileCount": 2,
                "totalBytes": 2,
                "inventorySha256": "1" * 64,
                "aggregateSha256": "2" * 64,
            }
            with contextlib.ExitStack() as locks:
                driver_lock = locks.enter_context(
                    adapter._AuthenticatedFileLock(
                        driver,
                        sha256(driver),
                        "fixture driver",
                    )
                )
                with mock.patch.object(
                    adapter.subprocess,
                    "run",
                    side_effect=dependency_scan,
                ):
                    prepared = adapter._prepare_compile_dependency_closure(
                        locks,
                        arguments=[
                            "-I",
                            str(source_root),
                            "-MD",
                            "-MF",
                            str(actual_depfile),
                            "-o",
                            str(output),
                            "-c",
                            str(source),
                        ],
                        repo=repo,
                        build=build,
                        emsdk_root=emsdk,
                        cache_root=cache,
                        driver_kind="em++",
                        driver_script=driver_lock,
                        qualification=qualification,
                    )
                if os.name == "nt":
                    with self.assertRaises(OSError):
                        header.write_text("mutated\n", encoding="utf-8")
                escaped_source = str(source).replace(" ", "\\ ")
                escaped_header = str(header).replace(" ", "\\ ")
                actual_depfile.write_text(
                    "qualified-object: "
                    f"{escaped_source} \\\n"
                    f"  {escaped_header}\n",
                    encoding="utf-8",
                )
                output.write_bytes(b"\0asm\1\0\0\0compiled")
                adapter._finish_compile_dependency_closure(
                    locks,
                    prepared,
                )
                payload = json.loads(
                    Path(str(prepared["sidecar"])).read_text("utf-8")
                )
                self.assertEqual(
                    [entry["path"] for entry in payload["dependencies"]],
                    [
                        "repo-input/tools/wasm-probe/src/header.h",
                        "repo-input/tools/wasm-probe/src/source.cpp",
                    ],
                )
                self.assertEqual(payload["outputSha256"], sha256(output))
                self.assertEqual(
                    payload["dependencyDiscovery"],
                    {
                        "preScanMethod": "emscripten-M",
                        "actualCompileMethod": "MD-MF",
                        "exactPathSetMatch": True,
                        "dependencyCount": 2,
                    },
                )

    def test_actual_compile_dependency_addition_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg actual compile dependency mismatch "
        ) as directory:
            build = Path(directory)
            source = build / "source.cpp"
            unexpected = build / "shadow.h"
            depfile = build / "source.cpp.o.d"
            source.write_text("int fixture;\n", encoding="utf-8")
            unexpected.write_text("unexpected\n", encoding="utf-8")
            depfile.write_text(
                "source.cpp.o: source.cpp shadow.h\n",
                encoding="utf-8",
            )
            prepared: dict[str, object] = {
                "build": build,
                "dependencyFile": depfile,
                "dependencyKeys": (
                    os.path.normcase(str(source.resolve())),
                ),
            }
            with (
                contextlib.ExitStack() as locks,
                self.assertRaisesRegex(
                    adapter.DriverError,
                    "differed from the locked pre-scan",
                ),
            ):
                adapter._validate_actual_compile_dependencies(
                    locks,
                    prepared,
                )


if __name__ == "__main__":
    unittest.main()
