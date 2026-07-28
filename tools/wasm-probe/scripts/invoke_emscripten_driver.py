"""Run pinned Emscripten 4.0.7 after one-shot response-file auditing."""

from __future__ import annotations

import argparse
import contextlib
import ctypes
import hashlib
import importlib
import importlib.util
import inspect
import json
import os
import re
import stat
import subprocess
import sys
import tempfile
from collections.abc import Callable
from pathlib import Path
from types import ModuleType
from typing import Any, NoReturn

if os.name == "nt":
    import msvcrt
    from ctypes import wintypes


FILE_ATTRIBUTE_REPARSE_POINT = 0x400


class DriverError(RuntimeError):
    pass


def _fail(message: str) -> NoReturn:
    raise DriverError(message)


def _same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(str(left)) == os.path.normcase(str(right))


def _is_reparse(path: Path) -> bool:
    metadata = path.lstat()
    return path.is_symlink() or bool(
        getattr(metadata, "st_file_attributes", 0)
        & FILE_ATTRIBUTE_REPARSE_POINT
    )


def _assert_chain_not_reparse(path: Path, description: str) -> Path:
    lexical = Path(os.path.abspath(path))
    chain = [lexical, *lexical.parents]
    missing_suffix = False
    for candidate in reversed(chain):
        if not candidate.exists() and not candidate.is_symlink():
            missing_suffix = True
            continue
        if missing_suffix:
            _fail(f"{description} path changed during inspection: {candidate}")
        if _is_reparse(candidate):
            _fail(f"{description} reparse component is forbidden: {candidate}")
        if candidate != lexical and not candidate.is_dir():
            _fail(f"{description} non-directory component: {candidate}")
    return lexical


def _assert_regular_file(
    path: Path,
    expected_sha256: str,
    description: str,
) -> Path:
    lexical = _assert_chain_not_reparse(path, description)
    if not lexical.exists():
        _fail(f"{description} is missing: {lexical}")
    metadata = lexical.stat()
    if not stat.S_ISREG(metadata.st_mode) or _is_reparse(lexical):
        _fail(f"{description} is not a regular file: {lexical}")
    actual = hashlib.sha256(lexical.read_bytes()).hexdigest()
    if actual.casefold() != expected_sha256.casefold():
        _fail(
            f"{description} SHA-256 drifted: expected {expected_sha256}, "
            f"got {actual}"
        )
    return lexical


def _assert_directory(path: Path, description: str) -> Path:
    lexical = _assert_chain_not_reparse(path, description)
    if not lexical.is_dir() or _is_reparse(lexical):
        _fail(f"{description} is not a regular directory: {lexical}")
    return lexical


class _AuthenticatedFileLock:
    def __init__(
        self,
        path: Path,
        expected_sha256: str | None,
        description: str,
    ) -> None:
        self.path = _assert_chain_not_reparse(path, description)
        self.expected_sha256 = expected_sha256
        self.description = description
        self.stream: Any | None = None
        self.data = b""

    def __enter__(self) -> "_AuthenticatedFileLock":
        if not self.path.exists():
            _fail(f"{self.description} is missing: {self.path}")
        metadata = self.path.stat()
        if not stat.S_ISREG(metadata.st_mode) or _is_reparse(self.path):
            _fail(
                f"{self.description} is not a regular file: {self.path}"
            )
        if os.name == "nt":
            kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
            create_file = kernel32.CreateFileW
            create_file.argtypes = (
                wintypes.LPCWSTR,
                wintypes.DWORD,
                wintypes.DWORD,
                wintypes.LPVOID,
                wintypes.DWORD,
                wintypes.DWORD,
                wintypes.HANDLE,
            )
            create_file.restype = wintypes.HANDLE
            handle = create_file(
                str(self.path),
                0x80000000,  # GENERIC_READ
                0x00000001,  # FILE_SHARE_READ: deny write and delete
                None,
                3,  # OPEN_EXISTING
                0x00200080,  # OPEN_REPARSE_POINT | NORMAL
                None,
            )
            invalid = ctypes.c_void_p(-1).value
            if handle == invalid:
                error = ctypes.get_last_error()
                raise OSError(error, os.strerror(error), str(self.path))
            try:
                get_final_path = kernel32.GetFinalPathNameByHandleW
                get_final_path.argtypes = (
                    wintypes.HANDLE,
                    wintypes.LPWSTR,
                    wintypes.DWORD,
                    wintypes.DWORD,
                )
                get_final_path.restype = wintypes.DWORD
                buffer = ctypes.create_unicode_buffer(32768)
                length = get_final_path(handle, buffer, len(buffer), 0)
                if length == 0 or length >= len(buffer):
                    error = ctypes.get_last_error()
                    raise OSError(
                        error,
                        os.strerror(error),
                        str(self.path),
                    )
                final_path = buffer.value
                if final_path.startswith("\\\\?\\UNC\\"):
                    final_path = "\\\\" + final_path[8:]
                elif final_path.startswith("\\\\?\\"):
                    final_path = final_path[4:]
                if not _same_path(Path(final_path), self.path):
                    _fail(
                        f"{self.description} opened path drifted: "
                        f"{final_path}"
                    )
                descriptor = msvcrt.open_osfhandle(
                    int(handle),
                    os.O_RDONLY,
                )
                handle = None
                self.stream = os.fdopen(descriptor, "rb", closefd=True)
            finally:
                if handle is not None:
                    kernel32.CloseHandle(handle)
        else:
            # Gate 1A is Windows-qualified. Loading verified bytes below still
            # prevents hash-to-import drift on other hosts; the deny-write
            # sharing contract is supplied by CreateFileW on Windows.
            self.stream = self.path.open("rb")
        try:
            self.data = self.stream.read()
            self.stream.seek(0)
            actual = hashlib.sha256(self.data).hexdigest()
            if (
                self.expected_sha256 is not None
                and actual.casefold() != self.expected_sha256.casefold()
            ):
                _fail(
                    f"{self.description} SHA-256 drifted: expected "
                    f"{self.expected_sha256}, got {actual}"
                )
        except BaseException:
            self.stream.close()
            self.stream = None
            raise
        return self

    def __exit__(self, *_: object) -> None:
        if self.stream is not None:
            self.stream.close()
            self.stream = None


def _load_module_from_bytes(
    path: Path,
    module_name: str,
    source: bytes,
    *,
    register: bool = False,
) -> ModuleType:
    module = ModuleType(module_name)
    module.__file__ = str(path)
    module.__package__ = module_name.rpartition(".")[0]
    if register:
        sys.modules[module_name] = module
    try:
        code = compile(source, str(path), "exec", dont_inherit=True)
        exec(code, module.__dict__)
    except BaseException:
        if register and sys.modules.get(module_name) is module:
            del sys.modules[module_name]
        raise
    return module


def _python_import_paths(root: Path) -> dict[str, Path]:
    paths: dict[str, Path] = {}
    casefolded: dict[str, str] = {}
    for current_text, directory_names, file_names in os.walk(
        root,
        topdown=True,
        followlinks=False,
    ):
        current = _assert_chain_not_reparse(
            Path(current_text),
            "Emscripten Python import directory",
        )
        directory_names.sort()
        file_names.sort()
        for name in directory_names:
            candidate = current / name
            if _is_reparse(candidate):
                _fail(
                    "Emscripten Python import surface reparse point is "
                    f"forbidden: {candidate}"
                )
        for name in file_names:
            if Path(name).suffix.casefold() != ".py":
                continue
            candidate = _assert_chain_not_reparse(
                current / name,
                "Emscripten Python import module",
            )
            relative = candidate.relative_to(root).as_posix()
            folded = relative.casefold()
            if folded in casefolded:
                _fail(
                    "Case-insensitive Emscripten Python module collision: "
                    f"{casefolded[folded]} and {relative}"
                )
            casefolded[folded] = relative
            paths[relative] = candidate
    if not paths:
        _fail("Emscripten Python import surface is empty")
    return dict(sorted(paths.items()))


def _lock_python_import_closure(
    locks: contextlib.ExitStack,
    root: Path,
    contract: dict[str, Any],
) -> dict[str, _AuthenticatedFileLock]:
    expected_fields = {
        "algorithm",
        "fileCount",
        "totalBytes",
        "inventorySha256",
        "aggregateSha256",
    }
    if set(contract) != expected_fields:
        _fail("Emscripten Python import closure contract fields drifted")
    if contract["algorithm"] != "sha256-path-null-digest-lf-v1":
        _fail("Unsupported Emscripten Python import digest algorithm")

    paths = _python_import_paths(root)
    authenticated: dict[str, _AuthenticatedFileLock] = {}
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative, path in paths.items():
        opened = locks.enter_context(
            _AuthenticatedFileLock(
                path,
                None,
                f"Emscripten Python module '{relative}'",
            )
        )
        digest = hashlib.sha256(opened.data).hexdigest()
        total_bytes += len(opened.data)
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{digest}\n".encode("utf-8"))
        authenticated[relative] = opened

    # A second exact enumeration closes the inventory/open gap. The parent
    # PowerShell wrapper independently performs the same check and retains
    # all deny-write/no-delete handles for the complete child lifetime.
    if tuple(_python_import_paths(root)) != tuple(authenticated):
        _fail("Emscripten Python import path set changed while locking")
    actual = {
        "fileCount": len(authenticated),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }
    for field, value in actual.items():
        if value != contract[field]:
            _fail(
                f"Emscripten Python import {field} drifted: expected "
                f"{contract[field]}, got {value}"
            )
    return authenticated


def _assert_locked_sha256(
    authenticated: _AuthenticatedFileLock,
    expected: str,
    description: str,
) -> None:
    actual = hashlib.sha256(authenticated.data).hexdigest()
    if actual.casefold() != expected.casefold():
        _fail(
            f"{description} SHA-256 drifted: expected {expected}, got {actual}"
        )


def _canonical_link_argument(repo: Path, argument: str) -> str:
    canonical = argument.replace("\\", "/")
    repo_text = str(repo).replace("\\", "/").rstrip("/")
    canonical = re.sub(
        re.escape(repo_text),
        "${REPO}",
        canonical,
        flags=re.IGNORECASE,
    )
    return canonical


def _qualification_identity(*, required: bool) -> dict[str, object] | None:
    prefix = "RHYTHMGAME_WASM_QUALIFICATION_"
    fields = {
        "algorithm": prefix + "ALGORITHM",
        "fileCount": prefix + "FILE_COUNT",
        "totalBytes": prefix + "TOTAL_BYTES",
        "inventorySha256": prefix + "INVENTORY_SHA256",
        "aggregateSha256": prefix + "AGGREGATE_SHA256",
    }
    enabled = os.environ.get("RHYTHMGAME_WASM_QUALIFICATION")
    present = {
        name: os.environ.get(environment)
        for name, environment in fields.items()
    }
    if enabled is None:
        if any(value is not None for value in present.values()):
            _fail("Partial qualification closure environment is forbidden")
        if required:
            _fail("Selected probe build requires a locked qualification closure")
        return None
    if enabled != "1":
        _fail("RHYTHMGAME_WASM_QUALIFICATION must be exactly 1")
    if any(value is None for value in present.values()):
        _fail("Qualification closure environment is incomplete")
    if (
        present["algorithm"]
        != "sha256-logical-null-bytes-null-digest-lf-v1"
    ):
        _fail("Qualification closure algorithm drifted")
    numeric: dict[str, int] = {}
    for name in ("fileCount", "totalBytes"):
        raw = str(present[name])
        if not raw.isdecimal() or str(int(raw)) != raw or int(raw) <= 0:
            _fail(f"Qualification closure {name} is invalid: {raw}")
        numeric[name] = int(raw)
    for name in ("inventorySha256", "aggregateSha256"):
        raw = str(present[name])
        if not re.fullmatch(r"[0-9a-f]{64}", raw):
            _fail(f"Qualification closure {name} is invalid")
    return {
        "algorithm": present["algorithm"],
        "fileCount": numeric["fileCount"],
        "totalBytes": numeric["totalBytes"],
        "inventorySha256": present["inventorySha256"],
        "aggregateSha256": present["aggregateSha256"],
    }


def _relative_to(path: Path, root: Path) -> Path | None:
    try:
        return path.relative_to(root)
    except ValueError:
        return None


def _resolve_compiler_path(
    value: str,
    cwd: Path,
    description: str,
    *,
    directory: bool,
) -> Path:
    candidate = Path(value)
    if not candidate.is_absolute():
        candidate = cwd / candidate
    candidate = _assert_chain_not_reparse(candidate, description)
    if directory:
        return _assert_directory(candidate, description)
    if not candidate.is_file() or _is_reparse(candidate):
        _fail(f"{description} is not a regular file: {candidate}")
    return candidate


def _compiler_search_paths(
    arguments: list[str],
    cwd: Path,
    allowed_roots: tuple[Path, ...],
) -> list[Path]:
    split_options = {"-I", "-idirafter", "-iquote", "-isystem"}
    compact_options = ("-idirafter", "-iquote", "-isystem", "-I")
    roots: list[Path] = []
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        value: str | None = None
        if argument in split_options:
            if index + 1 >= len(arguments):
                _fail(f"Compiler search option has no operand: {argument}")
            value = arguments[index + 1]
            index += 1
        else:
            for option in compact_options:
                if argument.startswith(option) and argument != option:
                    value = argument[len(option) :]
                    break
        if value is not None:
            root = _resolve_compiler_path(
                value,
                cwd,
                f"compiler search root from {argument}",
                directory=True,
            )
            if not any(_relative_to(root, allowed) is not None
                       for allowed in allowed_roots):
                _fail(f"Compiler search root escaped modeled roots: {root}")
            roots.append(root)
        index += 1
    return roots


def _parse_make_dependencies(content: bytes) -> list[str]:
    try:
        text = content.decode("utf-8")
    except UnicodeDecodeError as error:
        _fail(f"Compiler dependency file is not UTF-8: {error}")
    text = text.replace("\\\r\n", "").replace("\\\n", "")
    separator = text.find(":")
    if separator < 1:
        _fail("Compiler dependency file has no target separator")
    body = text[separator + 1 :]
    tokens: list[str] = []
    current: list[str] = []
    index = 0
    while index < len(body):
        character = body[index]
        if character.isspace():
            if current:
                tokens.append("".join(current))
                current.clear()
            index += 1
            continue
        if character == "\\" and index + 1 < len(body):
            following = body[index + 1]
            if following.isspace() or following in {"#", "$"}:
                current.append(following)
                index += 2
                continue
        if character == "$" and index + 1 < len(body):
            if body[index + 1] == "$":
                current.append("$")
                index += 2
                continue
        current.append(character)
        index += 1
    if current:
        tokens.append("".join(current))
    if not tokens or any(not token for token in tokens):
        _fail("Compiler dependency file contains no dependencies")
    return tokens


def _repo_input_paths(repo: Path) -> set[str]:
    manifest = repo / "tools" / "wasm-probe" / "input-manifest.txt"
    _assert_chain_not_reparse(manifest, "probe input manifest")
    paths: set[str] = set()
    for raw in manifest.read_text("utf-8").splitlines():
        if not raw:
            continue
        relative = Path(raw)
        if (
            relative.is_absolute()
            or "\\" in raw
            or ".." in relative.parts
            or "." in relative.parts
        ):
            _fail(f"Unsafe probe input-manifest path: {raw}")
        candidate = _resolve_compiler_path(
            str(repo / relative),
            repo,
            f"probe input '{raw}'",
            directory=False,
        )
        paths.add(os.path.normcase(str(candidate)))
    return paths


def _dependency_identity(
    path: Path,
    *,
    repo: Path,
    build: Path,
    source_root: Path,
    target_root: Path,
    emsdk_root: Path,
    cache_root: Path,
    repo_inputs: set[str],
) -> tuple[str, Path]:
    roots = (
        ("build-generated", build),
        ("probe-source", source_root),
        ("vcpkg-target", target_root),
        ("emsdk", emsdk_root),
        ("emscripten-cache", cache_root),
    )
    for label, root in roots:
        relative = _relative_to(path, root)
        if relative is None:
            continue
        if label == "probe-source":
            if os.path.normcase(str(path)) not in repo_inputs:
                _fail(
                    "Compiler dependency is not in the explicit probe input "
                    f"manifest: {path}"
                )
            relative = path.relative_to(repo)
            label = "repo-input"
        return f"{label}/{relative.as_posix()}", root
    _fail(f"Compiler dependency escaped modeled roots: {path}")


def _scan_arguments(
    arguments: list[str],
) -> tuple[list[str], Path, Path]:
    stripped: list[str] = []
    output: Path | None = None
    dependency_file: Path | None = None
    dependency_mode: str | None = None
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument in {"-MD", "-MMD"}:
            if dependency_mode is not None:
                _fail("Compile dependency mode is duplicated")
            dependency_mode = argument
            index += 1
            continue
        if argument == "-MP":
            _fail("Phony dependency targets are forbidden")
        if argument == "-MF":
            if index + 1 >= len(arguments):
                _fail(f"Compile dependency option has no operand: {argument}")
            if dependency_file is not None:
                _fail("Compile dependency output is duplicated")
            dependency_file = Path(arguments[index + 1])
            index += 2
            continue
        if argument in {"-MQ", "-MT"}:
            if index + 1 >= len(arguments):
                _fail(f"Compile dependency option has no operand: {argument}")
            index += 2
            continue
        if argument.startswith(("-MF", "-MQ", "-MT")):
            _fail(
                "Joined compile dependency options are forbidden: "
                f"{argument}"
            )
        if argument == "-o":
            if index + 1 >= len(arguments) or output is not None:
                _fail("Compile output option is missing or duplicated")
            output = Path(arguments[index + 1])
            index += 2
            continue
        if argument == "-c":
            index += 1
            continue
        if argument in {"-M", "-MM", "-MG", "-MJ"}:
            _fail(f"Unmodeled compile dependency mode is forbidden: {argument}")
        stripped.append(argument)
        index += 1
    if output is None:
        _fail("Qualified compile edge has no output")
    if dependency_mode is None or dependency_file is None:
        _fail(
            "Qualified compile edge must emit one explicit dependency file"
        )
    return stripped, output, dependency_file


def _write_json_atomic(path: Path, value: dict[str, object]) -> bytes:
    payload = (
        json.dumps(
            value,
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            dir=path.parent,
            prefix=f".{path.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(payload)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_name, path)
        temporary_name = None
    finally:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)
    return payload


def _prepare_compile_dependency_closure(
    locks: contextlib.ExitStack,
    *,
    arguments: list[str],
    repo: Path,
    build: Path,
    emsdk_root: Path,
    cache_root: Path,
    driver_kind: str,
    driver_script: _AuthenticatedFileLock,
    qualification: dict[str, object],
) -> dict[str, object]:
    scan_arguments, output_value, dependency_file_value = _scan_arguments(
        arguments
    )
    output = _resolve_compiler_path(
        str(output_value.parent if output_value.parent != Path("") else build),
        build,
        "qualified compile output directory",
        directory=True,
    ) / output_value.name
    output = Path(os.path.abspath(output))
    if _relative_to(output, build) is None:
        _fail(f"Qualified compile output escaped build root: {output}")
    dependency_file = _resolve_compiler_path(
        str(
            dependency_file_value.parent
            if dependency_file_value.parent != Path("")
            else build
        ),
        build,
        "qualified compile dependency output directory",
        directory=True,
    ) / dependency_file_value.name
    dependency_file = Path(os.path.abspath(dependency_file))
    if _relative_to(dependency_file, build) is None:
        _fail(
            "Qualified compile dependency output escaped build root: "
            f"{dependency_file}"
        )
    sidecar = output.with_name(output.name + ".rg-compile-inputs.json")
    if dependency_file in {output, sidecar}:
        _fail("Qualified compile outputs alias each other")
    if dependency_file.exists() or dependency_file.is_symlink():
        _assert_chain_not_reparse(
            dependency_file,
            "stale compile dependency output",
        )
        if not dependency_file.is_file():
            _fail(
                "Compile dependency output is not a file: "
                f"{dependency_file}"
            )
        dependency_file.unlink()
    if sidecar.exists() or sidecar.is_symlink():
        _assert_chain_not_reparse(sidecar, "stale compile-input sidecar")
        if not sidecar.is_file():
            _fail(f"Compile-input sidecar is not a file: {sidecar}")
        sidecar.unlink()

    source_root = repo / "tools" / "wasm-probe"
    emscripten_root = driver_script.path.parent
    if _relative_to(emscripten_root, emsdk_root) is None:
        _fail("Authenticated Emscripten driver escaped the emsdk root")
    target_root = (
        repo
        / ".wasm-vcpkg"
        / "installed"
        / "wasm32-emscripten-rg"
    )
    allowed_roots = tuple(
        _assert_directory(root, f"modeled compiler root '{root.name}'")
        for root in (
            build,
            source_root,
            target_root,
            emsdk_root,
            cache_root,
        )
    )
    _compiler_search_paths(arguments, build, allowed_roots)

    runner = (
        "import runpy,sys;"
        "root=sys.argv.pop(1);"
        "script=sys.argv.pop(1);"
        "sys.path.insert(0,root);"
        "sys.argv[0]=script;"
        "runpy.run_path(script,run_name='__main__')"
    )
    with tempfile.TemporaryDirectory(
        prefix=".rg-dependency-scan-",
        dir=output.parent,
    ) as temporary:
        depfile = Path(temporary) / "dependencies.d"
        command = [
            sys.executable,
            "-I",
            "-B",
            "-c",
            runner,
            str(emscripten_root),
            str(driver_script.path),
            *scan_arguments,
            "-M",
            "-MF",
            str(depfile),
            "-MT",
            "rhythmgame-qualified-object",
        ]
        result = subprocess.run(
            command,
            cwd=build,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode != 0:
            diagnostics = (
                result.stderr + b"\n" + result.stdout
            ).decode("utf-8", errors="replace")
            _fail(
                "Qualified compiler dependency scan failed: "
                + diagnostics[-4000:]
            )
        with _AuthenticatedFileLock(
            depfile,
            None,
            "compiler dependency file",
        ) as depfile_lock:
            dependency_values = _parse_make_dependencies(depfile_lock.data)

    repo_inputs = _repo_input_paths(repo)
    records: list[dict[str, object]] = []
    seen: set[str] = set()
    for value in dependency_values:
        dependency = _resolve_compiler_path(
            value,
            build,
            "compiler dependency",
            directory=False,
        )
        key = os.path.normcase(str(dependency))
        if key in seen:
            continue
        seen.add(key)
        logical, _ = _dependency_identity(
            dependency,
            repo=repo,
            build=build,
            source_root=source_root,
            target_root=target_root,
            emsdk_root=emsdk_root,
            cache_root=cache_root,
            repo_inputs=repo_inputs,
        )
        opened = locks.enter_context(
            _AuthenticatedFileLock(
                dependency,
                None,
                f"compile dependency '{logical}'",
            )
        )
        records.append(
            {
                "path": logical,
                "bytes": len(opened.data),
                "sha256": hashlib.sha256(opened.data).hexdigest(),
            }
        )
    records.sort(key=lambda entry: str(entry["path"]))
    if not records:
        _fail("Qualified compile dependency closure is empty")
    output_relative = output.relative_to(build).as_posix()
    closure_payload = {
        "algorithm": "sha256-compile-dependency-files-json-v1",
        "qualification": qualification,
        "driverKind": driver_kind,
        "output": f"build-output/{output_relative}",
        "arguments": [
            _canonical_link_argument(repo, argument)
            for argument in arguments
        ],
        "dependencyDiscovery": {
            "preScanMethod": "emscripten-M",
            "actualCompileMethod": "MD-MF",
            "exactPathSetMatch": True,
            "dependencyCount": len(records),
        },
        "dependencies": records,
    }
    encoded = (
        json.dumps(
            closure_payload,
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")
    return {
        "output": output,
        "sidecar": sidecar,
        "build": build,
        "dependencyFile": dependency_file,
        "dependencyKeys": tuple(sorted(seen)),
        "payload": closure_payload,
        "closureSha256": hashlib.sha256(encoded).hexdigest(),
    }


def _validate_actual_compile_dependencies(
    locks: contextlib.ExitStack,
    prepared: dict[str, object],
) -> None:
    dependency_file = Path(str(prepared["dependencyFile"]))
    dependency_file_lock = locks.enter_context(
        _AuthenticatedFileLock(
            dependency_file,
            None,
            "actual compiler dependency file",
        )
    )
    actual_values = _parse_make_dependencies(dependency_file_lock.data)
    build = Path(str(prepared["build"]))
    actual_keys = {
        os.path.normcase(
            str(
                _resolve_compiler_path(
                    value,
                    build,
                    "actual compiler dependency",
                    directory=False,
                )
            )
        )
        for value in actual_values
    }
    expected_keys = set(prepared["dependencyKeys"])
    if actual_keys != expected_keys:
        added = sorted(actual_keys - expected_keys)
        removed = sorted(expected_keys - actual_keys)
        _fail(
            "Actual compile dependency set differed from the locked "
            f"pre-scan: added={added[:5]}, removed={removed[:5]}"
        )


def _finish_compile_dependency_closure(
    locks: contextlib.ExitStack,
    prepared: dict[str, object],
) -> None:
    _validate_actual_compile_dependencies(locks, prepared)
    output = Path(str(prepared["output"]))
    opened = locks.enter_context(
        _AuthenticatedFileLock(
            output,
            None,
            "qualified compile output",
        )
    )
    sidecar_payload = {
        "schemaVersion": 1,
        **dict(prepared["payload"]),
        "closureSha256": prepared["closureSha256"],
        "outputBytes": len(opened.data),
        "outputSha256": hashlib.sha256(opened.data).hexdigest(),
    }
    _write_json_atomic(Path(str(prepared["sidecar"])), sidecar_payload)


def _archive_members(data: bytes) -> list[tuple[str, bytes]]:
    if not data.startswith(b"!<arch>\n"):
        _fail("Build-local archive magic drifted")
    members: list[tuple[str, bytes]] = []
    offset = 8
    string_table = b""
    while offset < len(data):
        if offset + 60 > len(data):
            _fail("Build-local archive header is truncated")
        header = data[offset : offset + 60]
        if header[58:60] != b"`\n":
            _fail("Build-local archive header trailer drifted")
        try:
            size = int(header[48:58].decode("ascii").strip())
        except (UnicodeDecodeError, ValueError):
            _fail("Build-local archive member size is invalid")
        offset += 60
        end = offset + size
        if end > len(data):
            _fail("Build-local archive member is truncated")
        body = data[offset:end]
        raw_name = header[:16].decode("ascii", errors="strict").strip()
        name = raw_name.rstrip("/")
        if raw_name == "//":
            string_table = body
        elif raw_name not in {"/", "/SYM64/"}:
            if raw_name.startswith("#1/"):
                try:
                    name_length = int(raw_name[3:])
                except ValueError:
                    _fail("BSD archive member name length is invalid")
                name = body[:name_length].decode("utf-8")
                body = body[name_length:]
            elif raw_name.startswith("/") and raw_name[1:].isdigit():
                table_offset = int(raw_name[1:])
                if table_offset >= len(string_table):
                    _fail("GNU archive member name offset is invalid")
                terminator = string_table.find(b"/\n", table_offset)
                if terminator < 0:
                    _fail("GNU archive member name is unterminated")
                name = string_table[table_offset:terminator].decode("utf-8")
            members.append((name, body))
        offset = end + (size % 2)
    return members


def _selected_compile_sidecars(
    locks: contextlib.ExitStack,
    *,
    build: Path,
    static_files: list[tuple[Path, str, _AuthenticatedFileLock]],
    qualification: dict[str, object],
) -> list[dict[str, object]]:
    direct: dict[str, tuple[str, str]] = {}
    archive_members: list[tuple[str, str, str]] = []
    for path, kind, opened in static_files:
        if _relative_to(path, build) is None:
            continue
        digest = hashlib.sha256(opened.data).hexdigest()
        if kind == "wasm-object":
            direct[os.path.normcase(str(path))] = (path.name, digest)
        else:
            for member_name, content in _archive_members(opened.data):
                if content.startswith(b"\0asm\1\0\0\0"):
                    archive_members.append(
                        (
                            path.relative_to(build).as_posix(),
                            Path(member_name).name,
                            hashlib.sha256(content).hexdigest(),
                        )
                    )

    candidates: list[
        tuple[Path, _AuthenticatedFileLock, dict[str, object]]
    ] = []
    for path in sorted(build.rglob("*.o.rg-compile-inputs.json")):
        opened = locks.enter_context(
            _AuthenticatedFileLock(
                path,
                None,
                "compile-input sidecar",
            )
        )
        try:
            payload = json.loads(opened.data.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            _fail(f"Invalid compile-input sidecar {path}: {error}")
        if not isinstance(payload, dict):
            _fail(f"Compile-input sidecar is not an object: {path}")
        candidates.append((path, opened, payload))

    selected: list[dict[str, object]] = []
    consumed: set[Path] = set()
    for object_path, (name, digest) in sorted(direct.items()):
        matches = [
            candidate
            for candidate in candidates
            if Path(str(candidate[2].get("output", ""))).name == name
            and candidate[2].get("outputSha256") == digest
            and os.path.normcase(
                str(candidate[0])[
                    : -len(".rg-compile-inputs.json")
                ]
            )
            == object_path
        ]
        if len(matches) != 1:
            _fail(
                "Selected direct object has no unique compile-input "
                f"sidecar: {object_path}"
            )
        path, opened, payload = matches[0]
        consumed.add(path)
        selected.append(
            {
                "output": payload.get("output"),
                "closureSha256": payload.get("closureSha256"),
                "outputSha256": payload.get("outputSha256"),
                "sidecarSha256": hashlib.sha256(opened.data).hexdigest(),
            }
        )
    for archive, member_name, digest in sorted(archive_members):
        matches = [
            candidate
            for candidate in candidates
            if candidate[0] not in consumed
            and Path(str(candidate[2].get("output", ""))).name
            == member_name
            and candidate[2].get("outputSha256") == digest
        ]
        if len(matches) != 1:
            _fail(
                "Selected archive member has no unique compile-input "
                f"sidecar: {archive}({member_name})"
            )
        path, opened, payload = matches[0]
        consumed.add(path)
        selected.append(
            {
                "archive": archive,
                "member": member_name,
                "output": payload.get("output"),
                "closureSha256": payload.get("closureSha256"),
                "outputSha256": payload.get("outputSha256"),
                "sidecarSha256": hashlib.sha256(opened.data).hexdigest(),
            }
        )
    for record in selected:
        matching = [
            payload
            for _, _, payload in candidates
            if payload.get("output") == record["output"]
            and payload.get("outputSha256") == record["outputSha256"]
        ]
        payload = matching[0] if len(matching) == 1 else {}
        dependencies = payload.get("dependencies")
        discovery = payload.get("dependencyDiscovery")
        if (
            len(matching) != 1
            or set(payload)
            != {
                "schemaVersion",
                "algorithm",
                "qualification",
                "driverKind",
                "output",
                "arguments",
                "dependencyDiscovery",
                "dependencies",
                "closureSha256",
                "outputBytes",
                "outputSha256",
            }
            or payload.get("schemaVersion") != 1
            or payload.get("algorithm")
            != "sha256-compile-dependency-files-json-v1"
            or payload.get("qualification") != qualification
            or not isinstance(dependencies, list)
            or not dependencies
            or discovery
            != {
                "preScanMethod": "emscripten-M",
                "actualCompileMethod": "MD-MF",
                "exactPathSetMatch": True,
                "dependencyCount": len(dependencies or ()),
            }
        ):
            _fail("Selected compile-input sidecar contract drifted")
    selected.sort(
        key=lambda record: (
            str(record.get("archive", "")),
            str(record.get("member", "")),
            str(record["output"]),
        )
    )
    if not selected:
        _fail("Selected application link has no local compile-input sidecars")
    return selected


def _selected_application_link_mode(
    arguments: list[str],
    repo: Path,
    cwd: Path,
    qualification: dict[str, object] | None,
) -> str | None:
    outputs = [
        arguments[index + 1]
        for index, argument in enumerate(arguments[:-1])
        if argument == "-o"
    ]
    if not outputs or Path(outputs[-1]).name != "RhythmGameWasmProbe.js":
        return None
    if len(outputs) != 1 or "-c" in arguments:
        _fail("Selected application link output/compile contract drifted")
    output = Path(outputs[0])
    if not output.is_absolute():
        output = cwd / output
    output = Path(os.path.abspath(output))
    expected_output = Path(
        os.path.abspath(cwd / "RhythmGameWasmProbe.js")
    )
    if not _same_path(output, expected_output):
        _fail(
            "Selected application link output/cwd pairing drifted: "
            f"expected {expected_output}, got {output}"
        )

    canonical_probe = (
        repo / "tools" / "wasm-probe" / "build" / "wasm-release"
    )
    canonical_playtest = (
        repo / "tools" / "web-playtest" / "build" / "wasm-release"
    )
    if _same_path(cwd, canonical_probe):
        if qualification is None:
            _fail("Selected probe link requires qualification closure")
        return "qualification"
    if _same_path(cwd, canonical_playtest):
        if qualification is not None:
            _fail(
                "Selected web-playtest link must not inherit qualification "
                "closure"
            )
        return "web-playtest"
    _fail(
        "Selected application link must run from a canonical build directory: "
        f"{canonical_probe} or {canonical_playtest}; got {cwd}"
    )


def _selected_application_link_identity(
    locks: contextlib.ExitStack,
    arguments: list[str],
    repo: Path,
    cwd: Path,
    qualification: dict[str, object] | None,
) -> str | None:
    mode = _selected_application_link_mode(
        arguments,
        repo,
        cwd,
        qualification,
    )
    if mode is None or mode == "web-playtest":
        return None
    if qualification is None:
        _fail("Selected probe link requires qualification closure")

    opened: dict[str, _AuthenticatedFileLock] = {}
    files: list[dict[str, object]] = []
    static_files: list[tuple[Path, str, _AuthenticatedFileLock]] = []
    repo_absolute = Path(os.path.abspath(repo))
    for index, argument in enumerate(arguments):
        if argument.startswith("-"):
            continue
        candidate = Path(argument)
        if not candidate.is_absolute():
            candidate = cwd / candidate
        candidate = Path(os.path.abspath(candidate))
        if not candidate.is_file() or candidate.suffix.casefold() not in {
            ".a",
            ".o",
        }:
            continue
        try:
            relative = candidate.relative_to(repo_absolute).as_posix()
        except ValueError:
            _fail(
                "Selected application static input escaped repository: "
                f"{candidate}"
            )
        key = os.path.normcase(str(candidate))
        authenticated = opened.get(key)
        if authenticated is None:
            authenticated = locks.enter_context(
                _AuthenticatedFileLock(
                    candidate,
                    None,
                    f"selected application static input '{relative}'",
                )
            )
            opened[key] = authenticated
        if candidate.suffix.casefold() == ".a":
            if not authenticated.data.startswith(b"!<arch>\n"):
                _fail(
                    "Selected application archive magic drifted: "
                    f"{candidate}"
                )
            kind = "archive"
        else:
            if not authenticated.data.startswith(b"\0asm\1\0\0\0"):
                _fail(
                    "Selected application Wasm object magic drifted: "
                    f"{candidate}"
                )
            kind = "wasm-object"
        files.append(
            {
                "argumentIndex": index,
                "path": relative,
                "kind": kind,
                "bytes": len(authenticated.data),
                "sha256": hashlib.sha256(authenticated.data).hexdigest(),
            }
        )
        static_files.append((candidate, kind, authenticated))
    if not files:
        _fail("Selected application link has no authenticated static inputs")
    compile_inputs = _selected_compile_sidecars(
        locks,
        build=cwd,
        static_files=static_files,
        qualification=qualification,
    )
    payload = {
        "algorithm": (
            "sha256-selected-link-argv-files-compile-qualification-json-v2"
        ),
        "qualification": qualification,
        "arguments": [
            _canonical_link_argument(repo_absolute, argument)
            for argument in arguments
        ],
        "files": files,
        "compileInputs": compile_inputs,
    }
    encoded = (
        json.dumps(
            payload,
            ensure_ascii=True,
            separators=(",", ":"),
            sort_keys=True,
        )
        + "\n"
    ).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


class _LockedModuleLoader:
    def __init__(
        self,
        module_name: str,
        authenticated: _AuthenticatedFileLock,
        *,
        is_package: bool,
    ) -> None:
        self.module_name = module_name
        self.authenticated = authenticated
        self.is_package = is_package

    def create_module(self, _: object) -> None:
        return None

    def exec_module(self, module: ModuleType) -> None:
        module.__file__ = str(self.authenticated.path)
        module.__loader__ = self
        if self.is_package:
            module.__package__ = self.module_name
            module.__path__ = [str(self.authenticated.path.parent)]
        else:
            module.__package__ = self.module_name.rpartition(".")[0]
        code = compile(
            self.authenticated.data,
            str(self.authenticated.path),
            "exec",
            dont_inherit=True,
        )
        exec(code, module.__dict__)


class _LockedModuleFinder:
    def __init__(
        self,
        authenticated: dict[str, _AuthenticatedFileLock],
    ) -> None:
        self.modules: dict[
            str,
            tuple[_AuthenticatedFileLock, bool],
        ] = {}
        for relative, opened in authenticated.items():
            parts = relative.split("/")
            if parts[-1] == "__init__.py":
                module_parts = parts[:-1]
                is_package = True
            else:
                module_parts = [*parts[:-1], Path(parts[-1]).stem]
                is_package = False
            if (
                not module_parts
                or not all(part.isidentifier() for part in module_parts)
            ):
                continue
            module_name = ".".join(module_parts)
            if module_name in self.modules:
                _fail(
                    "Emscripten Python import name collision: "
                    f"{module_name}"
                )
            self.modules[module_name] = (opened, is_package)

    def find_spec(
        self,
        fullname: str,
        _path: object = None,
        _target: object = None,
    ) -> object:
        record = self.modules.get(fullname)
        if record is None:
            return None
        authenticated, is_package = record
        loader = _LockedModuleLoader(
            fullname,
            authenticated,
            is_package=is_package,
        )
        return importlib.util.spec_from_loader(
            fullname,
            loader,
            origin=str(authenticated.path),
            is_package=is_package,
        )


def _parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--lock", type=Path, required=True)
    parser.add_argument("--auditor", type=Path, required=True)
    parser.add_argument("--emscripten-root", type=Path, required=True)
    parser.add_argument("--driver-kind", choices=("emcc", "em++"), required=True)
    parser.add_argument("--em-config", type=Path, required=True)
    parser.add_argument("--em-config-sha256", required=True)
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    parsed = parser.parse_args()
    if parsed.arguments and parsed.arguments[0] == "--":
        parsed.arguments = parsed.arguments[1:]
    return parsed


def _assert_environment(em_config: Path, cache_root: Path) -> None:
    forbidden_exact = {
        "CCC_OVERRIDE_OPTIONS",
        "EMCC_CFLAGS",
        "EMCC_REPRODUCE",
        "EM_COMPILER_WRAPPER",
        "EM_COMPILER_WRAPPER2",
        "NODE_OPTIONS",
        "NODE_PATH",
        "PYTHONHOME",
        "PYTHONPATH",
        "_EMCC_CCACHE",
    }
    forbidden_prefixes = (
        "EMMAKEN_",
        "QML_",
        "QT_",
    )
    survivors = sorted(
        name
        for name in os.environ
        if name.upper() in forbidden_exact
        or name.upper().startswith(forbidden_prefixes)
        or (
            name.upper().startswith("EMCC_")
            and name.upper() not in {"EMCC_CORES"}
        )
    )
    if survivors:
        _fail(f"Ambient compiler environment survived scrubbing: {survivors}")
    configured = os.environ.get("EM_CONFIG")
    if not configured or not _same_path(
        _assert_chain_not_reparse(Path(configured), "EM_CONFIG"),
        em_config,
    ):
        _fail("EM_CONFIG does not match the authenticated activation file")
    configured_cache = os.environ.get("EM_CACHE")
    if not configured_cache or not _same_path(
        _assert_directory(Path(configured_cache), "EM_CACHE"),
        cache_root,
    ):
        _fail("EM_CACHE does not match the authenticated frozen cache")
    if os.environ.get("EM_FROZEN_CACHE") != "1":
        _fail("EM_FROZEN_CACHE must be exactly 1")


def run_driver(
    parsed: argparse.Namespace,
    *,
    after_expand: Callable[[], None] | None = None,
    after_auditor_authenticate: Callable[[], None] | None = None,
    after_driver_authenticate: Callable[[], None] | None = None,
) -> int:
    with contextlib.ExitStack() as locks:
        lock_file = locks.enter_context(
            _AuthenticatedFileLock(
                parsed.lock,
                None,
                "toolchain lock",
            )
        )
        lock = json.loads(lock_file.data.decode("utf-8"))
        contract = lock["emscripten"]
        driver_api = contract["driverApi"]
        emscripten_root = _assert_directory(
            parsed.emscripten_root,
            "Emscripten root",
        )
        cache_root = _assert_directory(parsed.cache_root, "frozen cache root")
        em_config_lock = locks.enter_context(
            _AuthenticatedFileLock(
                parsed.em_config,
                parsed.em_config_sha256,
                "emsdk activation file",
            )
        )
        em_config = em_config_lock.path
        _assert_environment(em_config, cache_root)

        launcher_relative = (
            contract["cxxLauncher"]
            if parsed.driver_kind == "em++"
            else contract["cLauncher"]
        )
        launcher_hash = (
            contract["cxxLauncherSha256"]
            if parsed.driver_kind == "em++"
            else contract["cLauncherSha256"]
        )
        emsdk_root = emscripten_root.parent.parent
        expected_launcher = _assert_regular_file(
            emsdk_root / launcher_relative,
            launcher_hash,
            f"pinned {parsed.driver_kind} launcher",
        )
        if not parsed.arguments:
            _fail("Compiler launcher did not receive the compiler command")
        actual_launcher = _assert_chain_not_reparse(
            Path(parsed.arguments[0]),
            "compiler command",
        )
        if not _same_path(actual_launcher, expected_launcher):
            _fail(
                f"Compiler command is not pinned {parsed.driver_kind}: "
                f"{actual_launcher}"
            )

        auditor_lock = locks.enter_context(
            _AuthenticatedFileLock(
                parsed.auditor,
                lock["gateTools"]["responseAuditorSha256"],
                "response-file auditor",
            )
        )
        if after_auditor_authenticate is not None:
            after_auditor_authenticate()
        audit_module = _load_module_from_bytes(
            auditor_lock.path,
            "_rhythmgame_authenticated_response_auditor",
            auditor_lock.data,
        )
        response_audit = audit_module.ResponseAudit(Path.cwd())
        effective = response_audit.expand(list(parsed.arguments[1:]))
        if after_expand is not None:
            after_expand()
        if any(
            argument.startswith("@") or argument.startswith("-Wl,@")
            for argument in effective
        ):
            _fail("Effective compiler arguments still contain a response file")
        qualification = _qualification_identity(required=False)
        selected_link_digest = _selected_application_link_identity(
            locks,
            effective,
            parsed.lock.resolve().parents[2],
            Path.cwd(),
            qualification,
        )
        if selected_link_digest is not None:
            effective.append(
                f"-Wl,--build-id=0x{selected_link_digest}"
            )

        python_imports = _lock_python_import_closure(
            locks,
            emscripten_root,
            driver_api["pythonImportClosure"],
        )
        emcc_lock = python_imports["emcc.py"]
        _assert_locked_sha256(
            emcc_lock,
            driver_api["emccPySha256"],
            "pinned emcc.py",
        )
        driver_script = (
            python_imports["em++.py"]
            if parsed.driver_kind == "em++"
            else emcc_lock
        )
        _assert_locked_sha256(
            driver_script,
            (
                driver_api["emxxPySha256"]
                if parsed.driver_kind == "em++"
                else driver_api["emccPySha256"]
            ),
            f"pinned {parsed.driver_kind}.py",
        )
        compile_closure: dict[str, object] | None = None
        if qualification is not None and "-c" in effective:
            compile_closure = _prepare_compile_dependency_closure(
                locks,
                arguments=effective,
                repo=parsed.lock.resolve().parents[2],
                build=Path.cwd(),
                emsdk_root=emsdk_root,
                cache_root=cache_root,
                driver_kind=parsed.driver_kind,
                driver_script=driver_script,
                qualification=qualification,
            )
        authenticated_modules = {
            "shared": python_imports["tools/shared.py"],
            "response_file": python_imports["tools/response_file.py"],
            "config": python_imports["tools/config.py"],
        }
        for name, field in (
            ("shared", "sharedPySha256"),
            ("response_file", "responseFilePySha256"),
            ("config", "configPySha256"),
        ):
            _assert_locked_sha256(
                authenticated_modules[name],
                driver_api[field],
                f"pinned tools/{name}.py",
            )
        if after_driver_authenticate is not None:
            after_driver_authenticate()

        original_argv = sys.argv
        original_path = list(sys.path)
        module_finder = _LockedModuleFinder(python_imports)
        managed_names = set(module_finder.modules)
        managed_names.update(
            name
            for name in sys.modules
            if name == "tools" or name.startswith("tools.")
        )
        previous_modules = {
            name: sys.modules[name]
            for name in managed_names
            if name in sys.modules
        }
        try:
            sys.argv = [str(emcc_lock.path), *effective]
            sys.path.insert(0, str(emscripten_root))
            sys.meta_path.insert(0, module_finder)
            for name in managed_names:
                sys.modules.pop(name, None)
            tools_package = importlib.import_module("tools")
            package_file = Path(
                inspect.getsourcefile(tools_package) or ""
            ).resolve()
            if package_file != emscripten_root / "tools" / "__init__.py":
                _fail("Imported tools package path drifted")
            loaded_authenticated_modules: dict[str, ModuleType] = {}
            for name in ("config", "response_file", "shared"):
                qualified_name = f"tools.{name}"
                authenticated = authenticated_modules[name]
                loaded_module = _load_module_from_bytes(
                    authenticated.path,
                    qualified_name,
                    authenticated.data,
                    register=True,
                )
                setattr(tools_package, name, loaded_module)
                loaded_authenticated_modules[name] = loaded_module
            emcc = _load_module_from_bytes(
                emcc_lock.path,
                "emcc",
                emcc_lock.data,
                register=True,
            )
            shared = loaded_authenticated_modules["shared"]

            if (
                Path(inspect.getsourcefile(emcc) or "").resolve()
                != emcc_lock.path
            ):
                _fail("Imported emcc module path drifted")
            if str(inspect.signature(emcc.main)) != "(args)":
                _fail("Pinned emcc.main signature drifted")
            imported_shared = Path(
                inspect.getsourcefile(shared) or ""
            ).resolve()
            if imported_shared != authenticated_modules["shared"].path:
                _fail("Imported tools.shared module path drifted")
            shared.run_via_emxx = parsed.driver_kind == "em++"
            try:
                result = int(emcc.main(sys.argv))
            except SystemExit as error:
                if error.code not in (None, 0):
                    raise
                result = 0
            if result == 0 and compile_closure is not None:
                _finish_compile_dependency_closure(
                    locks,
                    compile_closure,
                )
            return result
        finally:
            if module_finder in sys.meta_path:
                sys.meta_path.remove(module_finder)
            for name in managed_names:
                sys.modules.pop(name, None)
            sys.modules.update(previous_modules)
            sys.argv = original_argv
            sys.path[:] = original_path


def main() -> int:
    return run_driver(_parse_arguments())


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (
        DriverError,
        KeyError,
        OSError,
        TypeError,
        ValueError,
    ) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
