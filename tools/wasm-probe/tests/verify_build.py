"""Fail-closed audit for the pinned Qt/Emscripten Gate 1A build."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


EXPECTED_EMSCRIPTEN = "4.0.7"
EXPECTED_EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
EXPECTED_EMXX_VERSION_LINE = (
    "emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) "
    "4.0.7 (8dc91db45bf96c174531006839472a3924d105aa)"
)
EXPECTED_VCPKG_COMMIT = "a0400024711b283056538ac19ced80b91a83c24c"
EXPECTED_VCPKG_VERSION_LINE = (
    "vcpkg package management program version "
    "2026-05-27-d5b6777d666efc1a7f491babfcdab37794c1ae3e"
)
EXPECTED_CMAKE = "4.2.3"
EXPECTED_NINJA = "1.13.2"
EXPECTED_QT = "6.11.1"

TARGET_TRIPLET = "wasm32-emscripten-rg"
HOST_TRIPLET = "x64-windows-rg-host-release"
TARGET_BUILD_SUFFIX = f"{TARGET_TRIPLET}-"

EXPECTED_QTBASE_TREE = "29a7f9f115d568b271a3b99fabeac886ec248f9f"
EXPECTED_QTDECLARATIVE_TREE = "846c872082b8bf0c50d13dc8ead681ae6fc6280a"
EXPECTED_OVERLAY_SHA256 = {
    "qtbase/portfile.cmake": (
        "edfa1c77d076848b1f0044e805894fc90430099bf6c62317107a8fa671b80df8"
    ),
    "qtbase/vcpkg.json": (
        "3bbf9ff2d47f0c88f3b1680edf532f47f69fe195b8473b3f87dcbc070c57db22"
    ),
    "qtbase/restore-wasm-version-check.patch": (
        "d9ed64da369eeb3aedc6830a2649925bcf8e8742d1c5fc86c21adf5bb168d5a0"
    ),
    "qtdeclarative/portfile.cmake": (
        "582a11da34542aa0288c51027979d49b0b29f1fc508ee9d266f5678de811f58a"
    ),
    "qtdeclarative/port.data.cmake": (
        "882a6ee414049bfc814995a53b909b520142fc6f8b74539099e9158a1bcb9053"
    ),
    "qtdeclarative/vcpkg.json": (
        "f553239a65f7a08da3051acba395cbbe77d8591afb42fb0a40d1e00cfec10b3a"
    ),
    "qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch": (
        "2a015242af462be117a2924d4d8db2c753b29891921e714c23bf1ab4355c4c50"
    ),
}

REQUIRED_TARGET_PORTS = (
    "qtbase",
    "qtdeclarative",
    "qtimageformats",
    "qtmultimedia",
    "qtshadertools",
    "qtsvg",
    "qtwebsockets",
)
REQUIRED_TARGET_QT_MODULES = (
    "Concurrent",
    "Multimedia",
    "Network",
    "Qml",
    "Quick",
    "ShaderTools",
    "WebSockets",
)
C_COMPILE_SETTINGS = ("-pthread", "-sSUPPORT_LONGJMP=wasm")
CXX_COMPILE_SETTINGS = (
    "-pthread",
    "-fwasm-exceptions",
    "-sSUPPORT_LONGJMP=wasm",
)
APPLICATION_LINK_SETTINGS = (
    "-pthread",
    "-fwasm-exceptions",
    "-sSUPPORT_LONGJMP=wasm",
    "-sJSPI",
    "-sAUDIO_WORKLET=1",
    "-sWASM_WORKERS=1",
    "-sPTHREAD_POOL_SIZE=4",
    "-sPTHREAD_POOL_SIZE_STRICT=2",
    "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
)
DEPLOYMENT_ARTIFACTS = (
    "RhythmGameWasmProbe.html",
    "RhythmGameWasmProbe.js",
    "RhythmGameWasmProbe.wasm",
    "RhythmGameWasmProbe.aw.js",
    "RhythmGameWasmProbe.ww.js",
    "qtloader.js",
    "qtlogo.svg",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def run_text(*command: str | Path) -> str:
    result = subprocess.run(
        [str(part) for part in command],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return "\n".join(
        part.strip()
        for part in (result.stdout, result.stderr)
        if part.strip()
    )


def git(repository: Path, *arguments: str) -> str:
    return run_text("git", "-C", repository, *arguments)


def first_line(value: str) -> str:
    lines = [line.strip() for line in value.splitlines() if line.strip()]
    require(bool(lines), "expected non-empty command output")
    return lines[0]


def path_key(path: Path) -> str:
    return os.path.normcase(os.path.normpath(str(path.resolve())))


def require_same_path(actual: str | Path, expected: Path, label: str) -> None:
    require(
        path_key(Path(actual)) == path_key(expected),
        f"{label}: expected {expected}, got {actual}",
    )


def relative_path(repo: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(repo.resolve()).as_posix()
    except ValueError as error:
        raise AssertionError(f"path escapes repository: {path}") from error


def split_windows_command_line(command: str) -> list[str]:
    """Split a CreateProcess-style command line using CRT quoting rules."""
    arguments: list[str] = []
    index = 0
    length = len(command)
    while True:
        while index < length and command[index] in " \t":
            index += 1
        if index == length:
            return arguments

        argument: list[str] = []
        in_quotes = False
        while index < length:
            if command[index] in " \t" and not in_quotes:
                break

            slash_count = 0
            while index < length and command[index] == "\\":
                slash_count += 1
                index += 1

            if index < length and command[index] == '"':
                argument.extend("\\" * (slash_count // 2))
                if slash_count % 2:
                    argument.append('"')
                    index += 1
                elif in_quotes and index + 1 < length and command[index + 1] == '"':
                    argument.append('"')
                    index += 2
                else:
                    in_quotes = not in_quotes
                    index += 1
                continue

            argument.extend("\\" * slash_count)
            if index >= length:
                break
            argument.append(command[index])
            index += 1

        require(not in_quotes, f"unterminated quote in command: {command}")
        arguments.append("".join(argument))
        while index < length and command[index] in " \t":
            index += 1


def compile_entry_arguments(entry: Mapping[str, Any]) -> list[str]:
    structured = entry.get("arguments")
    if structured is not None:
        require(
            isinstance(structured, list)
            and bool(structured)
            and all(isinstance(value, str) for value in structured),
            "compile entry arguments must be a non-empty string list",
        )
        return list(structured)

    command = entry.get("command")
    require(
        isinstance(command, str) and bool(command.strip()),
        "compile entry requires command or arguments",
    )
    arguments = split_windows_command_line(command)
    require(bool(arguments), "compile entry command parsed to no arguments")
    return arguments


def target_compile_databases(buildtrees: Path) -> list[Path]:
    databases = []
    for database in buildtrees.rglob("compile_commands.json"):
        relative = database.relative_to(buildtrees)
        if any(part.startswith(TARGET_BUILD_SUFFIX) for part in relative.parts):
            databases.append(database)
    return sorted(databases, key=lambda path: path.as_posix().casefold())


def parse_cmake_cache(path: Path) -> dict[str, str]:
    require(path.is_file(), f"missing CMake cache: {path}")
    result: dict[str, str] = {}
    for line in path.read_text("utf-8", errors="replace").splitlines():
        if not line or line.startswith(("#", "//")):
            continue
        match = re.fullmatch(r"([^:=]+):[^=]*=(.*)", line)
        if match:
            result[match.group(1)] = match.group(2)
    return result


def parse_vcpkg_status(path: Path) -> list[dict[str, str]]:
    require(path.is_file(), f"missing vcpkg status database: {path}")
    records: list[dict[str, str]] = []
    paragraphs = re.split(r"(?:\r?\n){2,}", path.read_text("utf-8"))
    for paragraph in paragraphs:
        if not paragraph.strip():
            continue
        record: dict[str, str] = {}
        for line in paragraph.splitlines():
            if line.startswith((" ", "\t")):
                continue
            key, separator, value = line.partition(":")
            if separator:
                record[key] = value.strip()
        records.append(record)
    return records


def require_status_record(
    records: Sequence[Mapping[str, str]],
    package: str,
    triplet: str,
    *,
    feature: str | None = None,
) -> Mapping[str, str]:
    matching = [
        record
        for record in records
        if record.get("Package") == package
        and record.get("Architecture") == triplet
        and record.get("Feature") == feature
    ]
    require(
        len(matching) == 1,
        f"expected one installed {package}[{feature or 'core'}]:{triplet}",
    )
    record = matching[0]
    require(
        record.get("Status") == "install ok installed",
        f"{package}:{triplet} is not installed",
    )
    if feature is None:
        require(
            record.get("Version") == EXPECTED_QT,
            f"{package}:{triplet} version is {record.get('Version')}",
        )
    return record


def has_setting(arguments: Sequence[str], setting: str) -> bool:
    if setting in arguments:
        return True
    if setting.startswith("-s") and len(setting) > 2:
        return any(
            arguments[index] == "-s"
            and arguments[index + 1] == setting[2:]
            for index in range(len(arguments) - 1)
        )
    return False


def command_has_setting(command: str, setting: str) -> bool:
    compact = re.escape(setting)
    if re.search(rf"(?<!\S){compact}(?!\S)", command):
        return True
    if setting.startswith("-s") and len(setting) > 2:
        split = re.escape(setting[2:])
        return re.search(rf"(?<!\S)-s\s+{split}(?!\S)", command) is not None
    return False


def configured_asyncify(command: str) -> bool:
    return (
        re.search(
            r"(?<!\S)-s(?:\s+)?ASYNCIFY(?:=|(?=\s)|$)",
            command,
        )
        is not None
    )


def canonical_command(repo: Path, command: str) -> str:
    variants = {
        str(repo.resolve()),
        str(repo.resolve()).replace("\\", "/"),
    }
    result = command
    for variant in sorted(variants, key=len, reverse=True):
        result = re.sub(re.escape(variant), "${REPO}", result, flags=re.I)
    return result.replace("\\", "/")


def verify_toolchains(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    lock_path = repo / "tools" / "wasm-probe" / "toolchain-lock.json"
    lock = json.loads(lock_path.read_text("utf-8"))
    require(lock["qt"]["version"] == EXPECTED_QT, "Qt lock drift")
    require(
        lock["emscripten"]
        == {
            "version": EXPECTED_EMSCRIPTEN,
            "emsdkCommit": EXPECTED_EMSDK_COMMIT,
        },
        "Emscripten lock drift",
    )
    require(
        lock["vcpkg"]["baseline"] == EXPECTED_VCPKG_COMMIT,
        "vcpkg lock drift",
    )
    require(
        lock["buildTools"]["cmake"]["version"] == EXPECTED_CMAKE,
        "CMake lock drift",
    )
    require(
        lock["buildTools"]["ninja"]["version"] == EXPECTED_NINJA,
        "Ninja lock drift",
    )

    expected_emsdk = repo / ".toolchains" / f"emsdk-{EXPECTED_EMSCRIPTEN}"
    expected_vcpkg = (
        repo / ".toolchains" / f"vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}"
    )
    expected_cmake = (
        repo
        / ".toolchains"
        / lock["buildTools"]["cmake"]["directory"]
        / "bin"
        / "cmake.exe"
    )
    expected_ninja = (
        repo
        / ".toolchains"
        / lock["buildTools"]["ninja"]["directory"]
        / "ninja.exe"
    )
    expected_vcpkg_exe = expected_vcpkg / "vcpkg.exe"
    expected_emxx = (
        expected_emsdk / "upstream" / "emscripten" / "em++.bat"
    )
    expected_emcc = (
        expected_emsdk / "upstream" / "emscripten" / "emcc.bat"
    )
    expected_emxx_driver = (
        expected_emsdk / "upstream" / "emscripten" / "em++.py"
    )

    require_same_path(emsdk, expected_emsdk, "emsdk argument")
    require_same_path(vcpkg, expected_vcpkg, "vcpkg argument")
    for path in (
        expected_cmake,
        expected_ninja,
        expected_vcpkg_exe,
        expected_emxx,
        expected_emcc,
        expected_emxx_driver,
    ):
        require(path.is_file(), f"missing pinned tool: {path}")

    resolved_commands = {
        name: shutil.which(name)
        for name in ("cmake", "ninja", "vcpkg", "em++", "emcc")
    }
    for name, value in resolved_commands.items():
        require(value is not None, f"{name} is not available in wrapper PATH")
    require_same_path(resolved_commands["cmake"], expected_cmake, "CMake PATH")
    require_same_path(resolved_commands["ninja"], expected_ninja, "Ninja PATH")
    require_same_path(
        resolved_commands["vcpkg"],
        expected_vcpkg_exe,
        "vcpkg PATH",
    )
    require_same_path(resolved_commands["em++"], expected_emxx, "em++ PATH")
    require_same_path(resolved_commands["emcc"], expected_emcc, "emcc PATH")

    require_same_path(os.environ.get("EMSDK", ""), expected_emsdk, "EMSDK")
    require_same_path(
        os.environ.get("EMSCRIPTEN_ROOT", ""),
        expected_emxx.parent,
        "EMSCRIPTEN_ROOT",
    )
    require_same_path(
        os.environ.get("VCPKG_ROOT", ""),
        expected_vcpkg,
        "VCPKG_ROOT",
    )
    require(
        os.environ.get("EMSCRIPTEN_VERSION") == EXPECTED_EMSCRIPTEN,
        "EMSCRIPTEN_VERSION drift",
    )
    emsdk_python_value = os.environ.get("EMSDK_PYTHON", "")
    require(bool(emsdk_python_value), "EMSDK_PYTHON is missing")
    emsdk_python = Path(emsdk_python_value).resolve()
    require(emsdk_python.is_file(), f"missing EMSDK_PYTHON: {emsdk_python}")
    require(
        emsdk_python.is_relative_to(expected_emsdk.resolve()),
        f"EMSDK_PYTHON escapes pinned emsdk: {emsdk_python}",
    )

    emsdk_head = git(emsdk, "rev-parse", "HEAD")
    vcpkg_head = git(vcpkg, "rev-parse", "HEAD")
    require(emsdk_head == EXPECTED_EMSDK_COMMIT, f"emsdk HEAD {emsdk_head}")
    require(vcpkg_head == EXPECTED_VCPKG_COMMIT, f"vcpkg HEAD {vcpkg_head}")
    require(not git(emsdk, "status", "--porcelain"), "emsdk is dirty")
    require(not git(vcpkg, "status", "--porcelain"), "vcpkg is dirty")

    emxx_version = run_text(
        emsdk_python,
        "-E",
        expected_emxx_driver,
        "--version",
    )
    vcpkg_version = run_text(expected_vcpkg_exe, "version")
    cmake_version = run_text(expected_cmake, "--version")
    ninja_version = run_text(expected_ninja, "--version")
    require(
        first_line(emxx_version) == EXPECTED_EMXX_VERSION_LINE,
        f"unexpected em++ identity: {first_line(emxx_version)}",
    )
    require(
        first_line(vcpkg_version) == EXPECTED_VCPKG_VERSION_LINE,
        f"unexpected vcpkg identity: {first_line(vcpkg_version)}",
    )
    require(
        first_line(cmake_version) == f"cmake version {EXPECTED_CMAKE}",
        f"unexpected CMake identity: {first_line(cmake_version)}",
    )
    require(
        first_line(ninja_version) == EXPECTED_NINJA,
        f"unexpected Ninja identity: {first_line(ninja_version)}",
    )

    return {
        "lockFileSha256": sha256(lock_path),
        "emscripten": {
            "version": EXPECTED_EMSCRIPTEN,
            "versionOutput": first_line(emxx_version),
            "emsdkCommit": emsdk_head,
            "launcher": relative_path(repo, expected_emxx),
            "driver": relative_path(repo, expected_emxx_driver),
        },
        "vcpkg": {
            "baselineCommit": vcpkg_head,
            "versionOutput": first_line(vcpkg_version),
            "executable": relative_path(repo, expected_vcpkg_exe),
        },
        "cmake": {
            "version": EXPECTED_CMAKE,
            "versionOutput": first_line(cmake_version),
            "executable": relative_path(repo, expected_cmake),
        },
        "ninja": {
            "version": EXPECTED_NINJA,
            "versionOutput": first_line(ninja_version),
            "executable": relative_path(repo, expected_ninja),
        },
    }


def require_qt_version(root: Path, label: str) -> str:
    config = root / "share" / "Qt6" / "Qt6ConfigVersion.cmake"
    implementation = config.with_name("Qt6ConfigVersionImpl.cmake")
    require(config.is_file(), f"{label}: Qt6ConfigVersion.cmake missing")
    require(
        implementation.is_file(),
        f"{label}: Qt6ConfigVersionImpl.cmake missing",
    )
    match = re.search(
        r'set\(PACKAGE_VERSION "([^"]+)"\)',
        implementation.read_text("utf-8", errors="replace"),
    )
    require(match is not None, f"{label}: PACKAGE_VERSION missing")
    version = match.group(1)
    require(version == EXPECTED_QT, f"{label}: expected Qt {EXPECTED_QT}")
    return version


def verify_qt_installation(
    repo: Path,
    installed: Path,
    build: Path,
) -> dict[str, Any]:
    target = installed / TARGET_TRIPLET
    host = installed / HOST_TRIPLET
    require(target.is_dir(), f"missing target installation: {target}")
    require(host.is_dir(), f"missing host installation: {host}")
    target_version = require_qt_version(target, "target")
    host_version = require_qt_version(host, "host")

    status_records = parse_vcpkg_status(installed / "vcpkg" / "status")
    target_packages: dict[str, dict[str, str]] = {}
    for port in REQUIRED_TARGET_PORTS:
        record = require_status_record(status_records, port, TARGET_TRIPLET)
        target_packages[port] = {
            key: record[key]
            for key in ("Version", "Port-Version", "Abi")
            if key in record
        }
    require_status_record(
        status_records,
        "qtmultimedia",
        TARGET_TRIPLET,
        feature="qml",
    )
    require_status_record(status_records, "qtbase", HOST_TRIPLET)
    require_status_record(status_records, "qtdeclarative", HOST_TRIPLET)
    require_status_record(status_records, "qtshadertools", HOST_TRIPLET)
    require_status_record(status_records, "qttools", HOST_TRIPLET)
    require_status_record(
        status_records,
        "qttools",
        HOST_TRIPLET,
        feature="linguist",
    )

    module_configs: dict[str, str] = {}
    for module in REQUIRED_TARGET_QT_MODULES:
        config = (
            target
            / "share"
            / f"Qt6{module}"
            / f"Qt6{module}Config.cmake"
        )
        require(config.is_file(), f"missing target Qt module: {module}")
        module_configs[module] = relative_path(repo, config)

    shared_suffixes = {".dll", ".so", ".dylib"}
    target_shared = [
        path
        for path in target.rglob("*")
        if path.is_file() and path.suffix.casefold() in shared_suffixes
    ]
    require(not target_shared, f"target shared libraries: {target_shared}")
    target_archives = [path for path in target.rglob("*.a") if path.is_file()]
    require(target_archives, "target installation has no static archives")

    host_core_dlls = list(host.rglob("Qt6Core.dll"))
    require(host_core_dlls, "host Qt is not a dynamic Windows build")
    target_triplet = (
        repo / "vcpkgTriplets" / f"{TARGET_TRIPLET}.cmake"
    ).read_text("utf-8")
    host_triplet = (
        repo / "vcpkgTriplets" / f"{HOST_TRIPLET}.cmake"
    ).read_text("utf-8")
    require(
        "set(VCPKG_LIBRARY_LINKAGE static)" in target_triplet,
        "target triplet is not static",
    )
    require(
        "set(VCPKG_LIBRARY_LINKAGE dynamic)" in host_triplet,
        "host triplet is not dynamic",
    )

    host_tool_specs = {
        "moc.exe": ("--version",),
        "qmlcachegen.exe": ("--version",),
        "qmltyperegistrar.exe": ("--version",),
        "qsb.exe": ("--version",),
        "lrelease.exe": ("-version",),
        "lupdate.exe": ("-version",),
    }
    host_tools: dict[str, dict[str, str]] = {}
    for name, options in host_tool_specs.items():
        candidates = list(host.rglob(name))
        require(
            len(candidates) == 1,
            f"expected exactly one host {name}: {candidates}",
        )
        output = run_text(candidates[0], *options)
        require(
            EXPECTED_QT in first_line(output),
            f"{name} does not report Qt {EXPECTED_QT}",
        )
        host_tools[name] = {
            "path": relative_path(repo, candidates[0]),
            "versionOutput": first_line(output),
        }

    probe_cache = parse_cmake_cache(build / "CMakeCache.txt")
    require(
        probe_cache.get("VCPKG_TARGET_TRIPLET") == TARGET_TRIPLET,
        "probe target triplet drift",
    )
    require(
        probe_cache.get("VCPKG_HOST_TRIPLET") == HOST_TRIPLET,
        "probe host triplet drift",
    )
    require_same_path(
        probe_cache.get("Qt6_DIR", ""),
        target / "share" / "Qt6",
        "probe Qt6_DIR",
    )
    require_same_path(
        probe_cache.get("QT_HOST_PATH", ""),
        host,
        "probe QT_HOST_PATH",
    )

    return {
        "version": EXPECTED_QT,
        "target": {
            "triplet": TARGET_TRIPLET,
            "version": target_version,
            "linkage": "static",
            "staticArchiveCount": len(target_archives),
            "sharedLibraryCount": 0,
            "requiredPorts": target_packages,
            "requiredModules": module_configs,
        },
        "host": {
            "triplet": HOST_TRIPLET,
            "version": host_version,
            "linkage": "dynamic",
            "qtCoreDllCount": len(host_core_dlls),
            "tools": host_tools,
        },
    }


def compiler_path_from_cmake(build: Path, language: str) -> tuple[Path, str]:
    candidates = list(
        (build / "CMakeFiles").glob(
            f"*/CMake{language}Compiler.cmake"
        )
    )
    require(
        len(candidates) == 1,
        f"{build}: expected one {language} compiler file",
    )
    text = candidates[0].read_text("utf-8", errors="replace")
    compiler = re.search(
        rf'set\(CMAKE_{language}_COMPILER "([^"]+)"\)',
        text,
    )
    version = re.search(
        rf'set\(CMAKE_{language}_COMPILER_VERSION "([^"]+)"\)',
        text,
    )
    require(compiler is not None, f"{language} compiler path missing")
    require(version is not None, f"{language} compiler version missing")
    return Path(compiler.group(1)), version.group(1)


def verify_cmake_identity(
    repo: Path,
    build: Path,
    buildtrees: Path,
) -> dict[str, Any]:
    probe_cache = parse_cmake_cache(build / "CMakeCache.txt")
    expected_ninja = (
        repo / ".toolchains" / "ninja-1.13.2-win" / "ninja.exe"
    )
    expected_emxx = (
        repo
        / ".toolchains"
        / "emsdk-4.0.7"
        / "upstream"
        / "emscripten"
        / "em++.bat"
    )
    require_same_path(
        probe_cache.get("CMAKE_MAKE_PROGRAM", ""),
        expected_ninja,
        "probe CMAKE_MAKE_PROGRAM",
    )
    require(
        probe_cache.get("CMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX") == "4096",
        "probe AutoGen response threshold is not 4096",
    )
    require_same_path(
        probe_cache.get("VCPKG_CHAINLOAD_TOOLCHAIN_FILE", ""),
        repo / "cmake" / "toolchains" / "vcpkg-emscripten.cmake",
        "probe chainload",
    )
    probe_compiler, compiler_version = compiler_path_from_cmake(build, "CXX")
    require_same_path(probe_compiler, expected_emxx, "probe C++ compiler")
    require(compiler_version == "21.0.0", "probe Clang version drift")

    qtbase_build = buildtrees / "qtbase" / f"{TARGET_TRIPLET}-rel"
    qt_c_compiler, qt_c_version = compiler_path_from_cmake(qtbase_build, "C")
    qt_cxx_compiler, qt_cxx_version = compiler_path_from_cmake(
        qtbase_build,
        "CXX",
    )
    expected_emcc = expected_emxx.with_name("emcc.bat")
    require_same_path(qt_c_compiler, expected_emcc, "Qt target C compiler")
    require_same_path(qt_cxx_compiler, expected_emxx, "Qt target C++ compiler")
    require(qt_c_version == "21.0.0", "Qt target C compiler version drift")
    require(
        qt_cxx_version == "21.0.0",
        "Qt target C++ compiler version drift",
    )
    return {
        "generator": "Ninja",
        "makeProgram": relative_path(repo, expected_ninja),
        "chainloadToolchain": "cmake/toolchains/vcpkg-emscripten.cmake",
        "probeCompiler": relative_path(repo, probe_compiler),
        "probeCompilerVersion": compiler_version,
        "qtTargetCCompiler": relative_path(repo, qt_c_compiler),
        "qtTargetCxxCompiler": relative_path(repo, qt_cxx_compiler),
        "qtTargetCompilerVersion": qt_cxx_version,
    }


def verify_compile_commands(
    repo: Path,
    build: Path,
    buildtrees: Path,
) -> dict[str, Any]:
    expected_emxx = (
        repo
        / ".toolchains"
        / "emsdk-4.0.7"
        / "upstream"
        / "emscripten"
        / "em++.bat"
    )
    expected_emcc = expected_emxx.with_name("emcc.bat")

    databases = target_compile_databases(buildtrees)
    require(databases, "no target Emscripten compile databases found")
    target_counts: dict[str, dict[str, int]] = {}
    total_c = 0
    total_cxx = 0
    c_coverage = {setting: 0 for setting in C_COMPILE_SETTINGS}
    cxx_coverage = {setting: 0 for setting in CXX_COMPILE_SETTINGS}

    for database in databases:
        relative = database.relative_to(buildtrees)
        require(
            any(part.startswith(TARGET_BUILD_SUFFIX) for part in relative.parts),
            f"host compile database entered target audit: {relative}",
        )
        require(
            not any(HOST_TRIPLET in part for part in relative.parts),
            f"host compile database entered target audit: {relative}",
        )
        port = relative.parts[0]
        entries = json.loads(database.read_text("utf-8"))
        require(isinstance(entries, list) and entries, f"empty {database}")
        port_counts = target_counts.setdefault(port, {"c": 0, "cxx": 0})
        for entry in entries:
            require(isinstance(entry, dict), f"invalid entry in {database}")
            arguments = compile_entry_arguments(entry)
            compiler = Path(arguments[0])
            if path_key(compiler) == path_key(expected_emxx):
                language = "cxx"
                settings = CXX_COMPILE_SETTINGS
                total_cxx += 1
                port_counts["cxx"] += 1
                coverage = cxx_coverage
            elif path_key(compiler) == path_key(expected_emcc):
                language = "c"
                settings = C_COMPILE_SETTINGS
                total_c += 1
                port_counts["c"] += 1
                coverage = c_coverage
            else:
                raise AssertionError(
                    f"{port}: non-pinned compiler in {database}: "
                    f"{arguments[0]}"
                )
            for setting in settings:
                require(
                    has_setting(arguments, setting),
                    f"{port}: missing {language} setting {setting} for "
                    f"{entry.get('file')}",
                )
                coverage[setting] += 1
            require(
                not any(
                    token.startswith("-sASYNCIFY")
                    for token in arguments
                ),
                f"{port}: literal Asyncify option configured",
            )

    for port in REQUIRED_TARGET_PORTS:
        require(
            target_counts.get(port, {}).get("cxx", 0) > 0,
            f"no audited target C++ commands for {port}",
        )
    require(total_c > 0, "no target Emscripten C commands found")
    require(total_cxx > 0, "no target Emscripten C++ commands found")
    require(
        all(count == total_c for count in c_coverage.values()),
        "not every target C command has the required flags",
    )
    require(
        all(count == total_cxx for count in cxx_coverage.values()),
        "not every target C++ command has the required flags",
    )

    probe_database = build / "compile_commands.json"
    entries = json.loads(probe_database.read_text("utf-8"))
    require(isinstance(entries, list) and entries, "probe compile DB is empty")
    boundary: dict[str, dict[str, Any]] = {}
    for source_name in ("ExceptionBoundary.cpp", "ProbeState.cpp"):
        matching = [
            entry
            for entry in entries
            if Path(str(entry.get("file", ""))).name == source_name
        ]
        require(
            len(matching) == 1,
            f"expected one generated {source_name} compile entry",
        )
        arguments = compile_entry_arguments(matching[0])
        require_same_path(
            arguments[0],
            expected_emxx,
            f"{source_name} compiler",
        )
        for setting in CXX_COMPILE_SETTINGS:
            require(
                has_setting(arguments, setting),
                f"{source_name} missing {setting}",
            )
        canonical = canonical_command(repo, "\0".join(arguments))
        boundary[source_name] = {
            "settings": list(CXX_COMPILE_SETTINGS),
            "commandSha256": sha256_text(canonical),
        }

    return {
        "targetDatabaseCount": len(databases),
        "targetDatabases": [
            relative_path(repo, database) for database in databases
        ],
        "targetCommandCounts": {"c": total_c, "cxx": total_cxx},
        "targetSettingCoverage": {
            "c": c_coverage,
            "cxx": cxx_coverage,
        },
        "targetPortCommandCounts": target_counts,
        "exceptionBoundary": boundary,
    }


def verify_application_link(
    repo: Path,
    build: Path,
    ninja: Path,
) -> dict[str, Any]:
    commands = run_text(
        ninja,
        "-C",
        build,
        "-t",
        "commands",
        "RhythmGameWasmProbe",
    )
    links = [
        line
        for line in commands.splitlines()
        if "RhythmGameWasmProbe.js" in line and "em++.bat" in line
    ]
    require(len(links) == 1, f"expected one application link: {links}")
    application_link = links[0]
    setting_counts: dict[str, int] = {}
    for setting in APPLICATION_LINK_SETTINGS:
        require(
            command_has_setting(application_link, setting),
            f"application link missing {setting}",
        )
        compact_count = len(
            re.findall(
                rf"(?<!\S){re.escape(setting)}(?!\S)",
                application_link,
            )
        )
        split_count = 0
        if setting.startswith("-s") and len(setting) > 2:
            split_count = len(
                re.findall(
                    rf"(?<!\S)-s\s+{re.escape(setting[2:])}(?!\S)",
                    application_link,
                )
            )
        setting_counts[setting] = compact_count + split_count
    require(
        not configured_asyncify(application_link),
        "literal -sASYNCIFY is configured on the application link",
    )

    build_ninja = (build / "build.ninja").read_text(
        "utf-8",
        errors="replace",
    )
    require(
        "libWasmProbeExceptionBoundary.a" in build_ninja,
        "static exception boundary is absent from the link graph",
    )
    cmake_source = (
        repo / "tools" / "wasm-probe" / "CMakeLists.txt"
    ).read_text("utf-8")
    require(
        not configured_asyncify(cmake_source),
        "literal -sASYNCIFY is configured in probe CMake",
    )
    return {
        "settings": list(APPLICATION_LINK_SETTINGS),
        "settingOccurrences": setting_counts,
        "literalAsyncifyConfigured": False,
        "staticExceptionArchiveLinked": True,
        "commandSha256": sha256_text(
            canonical_command(repo, application_link)
        ),
    }


def verify_features_and_autogen(
    repo: Path,
    installed: Path,
    buildtrees: Path,
) -> dict[str, Any]:
    qtbase = buildtrees / "qtbase" / f"{TARGET_TRIPLET}-rel"
    declarative_target = (
        buildtrees / "qtdeclarative" / f"{TARGET_TRIPLET}-rel"
    )
    declarative_host = (
        buildtrees / "qtdeclarative" / f"{HOST_TRIPLET}-rel"
    )
    qtbase_cache = parse_cmake_cache(qtbase / "CMakeCache.txt")
    target_cache = parse_cmake_cache(declarative_target / "CMakeCache.txt")
    host_cache = parse_cmake_cache(declarative_host / "CMakeCache.txt")

    expected_features = {
        "thread": "ON",
        "wasm_exceptions": "ON",
        "wasm_jspi": "ON",
        "wasm_simd128": "OFF",
    }
    for feature, value in expected_features.items():
        require(
            qtbase_cache.get(f"FEATURE_{feature}") == value,
            f"QtBase FEATURE_{feature} is not {value}",
        )
        require(
            qtbase_cache.get(f"QT_FEATURE_{feature}") == value,
            f"QtBase QT_FEATURE_{feature} is not {value}",
        )

    qconfig = (
        installed
        / TARGET_TRIPLET
        / "include"
        / "Qt6"
        / "QtCore"
        / "qconfig.h"
    ).read_text("utf-8", errors="replace")
    for feature in ("thread", "wasm_exceptions", "wasm_jspi"):
        require(
            f"#define QT_FEATURE_{feature} 1" in qconfig,
            f"installed Qt missing {feature}",
        )
    require(
        "#define QT_FEATURE_wasm_simd128 -1" in qconfig
        or "#define QT_FEATURE_wasm_simd128 0" in qconfig,
        "installed Qt has Wasm SIMD enabled",
    )

    wasm_macros = (
        installed
        / TARGET_TRIPLET
        / "share"
        / "Qt6Core"
        / "Qt6WasmMacros.cmake"
    ).read_text("utf-8")
    require(
        "include(QtPublicWasmToolchainHelpers)" in wasm_macros,
        "Qt Wasm version-helper include missing",
    )
    require(
        "_qt_test_emscripten_version()" in wasm_macros,
        "Qt Emscripten version check was suppressed",
    )

    style_features = (
        "quickcontrols2_fluentwinui3",
        "quickcontrols2_universal",
    )
    for feature in style_features:
        require(
            target_cache.get(f"FEATURE_{feature}") == "ON"
            and target_cache.get(f"QT_FEATURE_{feature}") == "ON",
            f"target style {feature} is not ON",
        )
        require(
            host_cache.get(f"FEATURE_{feature}") == "OFF"
            and host_cache.get(f"QT_FEATURE_{feature}") == "OFF",
            f"host style {feature} is not OFF",
        )
    target_style_configs = {
        "FluentWinUI3": (
            installed
            / TARGET_TRIPLET
            / "share"
            / "Qt6QuickControls2FluentWinUI3StyleImpl"
            / "Qt6QuickControls2FluentWinUI3StyleImplConfig.cmake"
        ),
        "Universal": (
            installed
            / TARGET_TRIPLET
            / "share"
            / "Qt6QuickControls2Universal"
            / "Qt6QuickControls2UniversalConfig.cmake"
        ),
    }
    for name, config in target_style_configs.items():
        require(config.is_file(), f"target {name} style was not installed")

    require(
        target_cache.get("CMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX") == "4096",
        "QtDeclarative target AutoGen threshold is not 4096",
    )
    require(
        target_cache.get("CMAKE_AUTOMOC_COMPILER_PREDEFINES", "ON")
        not in {"0", "OFF", "FALSE"},
        "AutoMoc compiler predefines were disabled",
    )

    expected_emxx = (
        repo
        / ".toolchains"
        / "emsdk-4.0.7"
        / "upstream"
        / "emscripten"
        / "em++.bat"
    )
    autogen_infos = sorted(
        declarative_target.rglob("AutogenInfo.json"),
        key=lambda path: path.as_posix().casefold(),
    )
    require(autogen_infos, "no QtDeclarative AutoGen metadata found")
    predefs_files: list[Path] = []
    for info_path in autogen_infos:
        info = json.loads(info_path.read_text("utf-8"))
        command = info.get("MOC_PREDEFS_CMD")
        predefs_file = info.get("MOC_PREDEFS_FILE")
        require(
            isinstance(command, list) and bool(command),
            f"compiler predefines command missing: {info_path}",
        )
        require(
            all(isinstance(part, str) for part in command),
            f"invalid compiler predefines command: {info_path}",
        )
        require_same_path(
            command[0],
            expected_emxx,
            f"AutoGen compiler for {info_path}",
        )
        require(
            "-dM" in command and "-E" in command,
            f"invalid compiler predefines command: {info_path}",
        )
        require(
            isinstance(predefs_file, str) and bool(predefs_file),
            f"compiler predefines output missing: {info_path}",
        )
        generated = Path(predefs_file)
        require(
            generated.is_file() and generated.stat().st_size > 0,
            f"compiler predefines were not generated: {generated}",
        )
        predefs_files.append(generated)

    aggregate = hashlib.sha256()
    for generated in sorted(
        predefs_files,
        key=lambda path: path.as_posix().casefold(),
    ):
        aggregate.update(
            relative_path(repo, generated).encode("utf-8") + b"\0"
        )
        aggregate.update(sha256(generated).encode("ascii") + b"\n")

    return {
        "qtFeatures": expected_features,
        "emscriptenVersionCheckRetained": True,
        "quickControlsStyles": {
            "target": {
                "FluentWinUI3": "ON",
                "Universal": "ON",
            },
            "host": {
                "FluentWinUI3": "OFF",
                "Universal": "OFF",
            },
        },
        "autogen": {
            "commandLineLengthMax": 4096,
            "compilerPredefinesEnabled": True,
            "compilerPredefinesCommandCount": len(autogen_infos),
            "generatedPredefinesFileCount": len(predefs_files),
            "generatedPredefinesAggregateSha256": aggregate.hexdigest(),
        },
    }


def baseline_blobs(vcpkg: Path, port: str) -> tuple[dict[str, str], str]:
    prefix = f"ports/{port}/"
    output = git(
        vcpkg,
        "ls-tree",
        "-r",
        EXPECTED_VCPKG_COMMIT,
        f"ports/{port}",
    )
    blobs: dict[str, str] = {}
    for entry in output.splitlines():
        match = re.fullmatch(r"\d+ blob ([0-9a-f]{40})\s+(.+)", entry)
        require(match is not None, f"unexpected {port} tree entry: {entry}")
        path = match.group(2)
        require(path.startswith(prefix), f"unexpected {port} path: {path}")
        blobs[path[len(prefix) :]] = match.group(1)
    tree = git(
        vcpkg,
        "rev-parse",
        f"{EXPECTED_VCPKG_COMMIT}:ports/{port}",
    )
    return blobs, tree


def normalized_text(path: Path) -> str:
    return path.read_bytes().decode("utf-8").replace("\r\n", "\n")


def verify_overlay_structure(
    repo: Path,
    vcpkg: Path,
    port: str,
    overlay: Path,
    modified: set[str],
    additions: set[str],
    normalized_baseline: set[str],
    expected_tree: str,
) -> dict[str, Any]:
    blobs, tree = baseline_blobs(vcpkg, port)
    require(tree == expected_tree, f"{port} baseline tree drift: {tree}")
    actual = {
        path.relative_to(overlay).as_posix()
        for path in overlay.rglob("*")
        if path.is_file()
    }
    expected = set(blobs) | additions
    require(
        actual == expected,
        f"{port}: unexpected overlay files: {sorted(actual ^ expected)}",
    )
    for relative, expected_blob in blobs.items():
        if relative in modified or relative in normalized_baseline:
            continue
        actual_blob = run_text(
            "git",
            "hash-object",
            "--no-filters",
            overlay / relative,
        )
        require(
            actual_blob == expected_blob,
            f"{port}: baseline byte drift in {relative}",
        )
    for relative in normalized_baseline:
        require(relative in blobs, f"{port}: no baseline for {relative}")
        require(
            normalized_text(overlay / relative)
            == normalized_text(vcpkg / "ports" / port / relative),
            f"{port}: non-line-ending drift in {relative}",
        )

    hashes = {
        relative: sha256(overlay / relative)
        for relative in sorted(actual)
    }
    aggregate = hashlib.sha256()
    for relative, digest in hashes.items():
        aggregate.update(relative.encode("utf-8") + b"\0")
        aggregate.update(digest.encode("ascii") + b"\n")
    return {
        "baselineTree": tree,
        "fileCount": len(actual),
        "modifiedBaselineFiles": sorted(modified),
        "lineEndingOnlyBaselineFiles": sorted(normalized_baseline),
        "addedFiles": sorted(additions),
        "fileSha256": hashes,
        "aggregateSha256": aggregate.hexdigest(),
    }


def verify_overlays(repo: Path, vcpkg: Path) -> dict[str, Any]:
    qtbase = repo / "vcpkgOverlayPortsWasm" / "qtbase"
    declarative = repo / "vcpkgOverlayPorts" / "qtdeclarative"
    qtbase_result = verify_overlay_structure(
        repo,
        vcpkg,
        "qtbase",
        qtbase,
        {"portfile.cmake", "vcpkg.json"},
        {"restore-wasm-version-check.patch"},
        set(),
        EXPECTED_QTBASE_TREE,
    )
    declarative_result = verify_overlay_structure(
        repo,
        vcpkg,
        "qtdeclarative",
        declarative,
        {"portfile.cmake", "vcpkg.json"},
        {"24205cd-qquickwindow-child-window-stacking.patch"},
        {"port.data.cmake"},
        EXPECTED_QTDECLARATIVE_TREE,
    )

    checked_files = {
        "qtbase/portfile.cmake": qtbase / "portfile.cmake",
        "qtbase/vcpkg.json": qtbase / "vcpkg.json",
        "qtbase/restore-wasm-version-check.patch": (
            qtbase / "restore-wasm-version-check.patch"
        ),
        "qtdeclarative/portfile.cmake": declarative / "portfile.cmake",
        "qtdeclarative/port.data.cmake": declarative / "port.data.cmake",
        "qtdeclarative/vcpkg.json": declarative / "vcpkg.json",
        "qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch": (
            declarative
            / "24205cd-qquickwindow-child-window-stacking.patch"
        ),
    }
    for label, path in checked_files.items():
        require(
            sha256(path) == EXPECTED_OVERLAY_SHA256[label],
            f"{label}: reviewed bytes drifted",
        )

    baseline_qtbase = normalized_text(vcpkg / "ports" / "qtbase" / "portfile.cmake")
    expected_qtbase = baseline_qtbase.replace(
        "        QTBUG-145703.patch # https://github.com/qt/qtbase/commit/"
        "239c54452fa60157c90901c8be8685048a65ad0a\n",
        "        QTBUG-145703.patch # https://github.com/qt/qtbase/commit/"
        "239c54452fa60157c90901c8be8685048a65ad0a\n"
        "        restore-wasm-version-check.patch\n",
        1,
    )
    qtbase_feature_block = """if(VCPKG_TARGET_IS_EMSCRIPTEN)
    list(APPEND FEATURE_OPTIONS
        -DFEATURE_thread:BOOL=ON
        -DFEATURE_wasm_exceptions:BOOL=ON
        -DFEATURE_wasm_jspi:BOOL=ON
        -DFEATURE_wasm_simd128:BOOL=OFF
    )
endif()

"""
    expected_qtbase = expected_qtbase.replace(
        "qt_install_submodule(",
        qtbase_feature_block + "qt_install_submodule(",
        1,
    )
    suppressed_version_check = (
        "if(VCPKG_TARGET_IS_EMSCRIPTEN)\n"
        '  vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6Core/'
        'Qt6WasmMacros.cmake" "_qt_test_emscripten_version()" "") '
        "# this is missing a include(QtPublicWasmToolchainHelpers)\n"
        "endif()\n\n"
    )
    require(
        suppressed_version_check in expected_qtbase,
        "qtbase baseline version workaround changed unexpectedly",
    )
    expected_qtbase = expected_qtbase.replace(
        suppressed_version_check,
        "",
        1,
    )
    require(
        normalized_text(qtbase / "portfile.cmake") == expected_qtbase,
        "qtbase portfile has changes outside the reviewed Wasm delta",
    )

    baseline_qtbase_json = json.loads(
        (vcpkg / "ports" / "qtbase" / "vcpkg.json").read_text("utf-8")
    )
    baseline_qtbase_json["port-version"] = 1
    require(
        json.loads((qtbase / "vcpkg.json").read_text("utf-8"))
        == baseline_qtbase_json,
        "qtbase vcpkg.json differs beyond port-version 1",
    )

    baseline_declarative = normalized_text(
        vcpkg / "ports" / "qtdeclarative" / "portfile.cmake"
    )
    expected_declarative = baseline_declarative.replace(
        'set(${PORT}_PATCHES "")',
        "set(${PORT}_PATCHES\n"
        "    24205cd-qquickwindow-child-window-stacking.patch\n"
        ")",
        1,
    )
    declarative_options = """set(QTDECLARATIVE_CONFIGURE_OPTIONS
    -DCMAKE_DISABLE_FIND_PACKAGE_LTTngUST:BOOL=ON
)
if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    list(APPEND QTDECLARATIVE_CONFIGURE_OPTIONS
        -DCMAKE_AUTOGEN_COMMAND_LINE_LENGTH_MAX:STRING=4096
    )
endif()
if(VCPKG_TARGET_IS_WINDOWS AND
   TARGET_TRIPLET STREQUAL "x64-windows-rg-host-release")
    # This package only provides host tools for the Wasm cross-build.
    # Keep the Wasm target's optional style features at upstream defaults.
    list(APPEND QTDECLARATIVE_CONFIGURE_OPTIONS
        -DFEATURE_quickcontrols2_fluentwinui3:BOOL=OFF
        -DFEATURE_quickcontrols2_universal:BOOL=OFF
    )
endif()

"""
    expected_declarative = expected_declarative.replace(
        "qt_install_submodule(",
        declarative_options + "qt_install_submodule(",
        1,
    )
    expected_declarative = expected_declarative.replace(
        "                      -DCMAKE_DISABLE_FIND_PACKAGE_LTTngUST:BOOL=ON\n",
        "                      ${QTDECLARATIVE_CONFIGURE_OPTIONS}\n",
        1,
    )
    require(
        normalized_text(declarative / "portfile.cmake")
        == expected_declarative,
        "qtdeclarative portfile differs outside the reviewed delta",
    )

    baseline_declarative_json = json.loads(
        (vcpkg / "ports" / "qtdeclarative" / "vcpkg.json").read_text(
            "utf-8"
        )
    )
    baseline_declarative_json["port-version"] = 1
    require(
        json.loads((declarative / "vcpkg.json").read_text("utf-8"))
        == baseline_declarative_json,
        "qtdeclarative vcpkg.json differs beyond port-version 1",
    )
    return {
        "qtbase": qtbase_result,
        "qtdeclarative": declarative_result,
        "reviewedDelta": {
            "qtbase": [
                "restore Emscripten version-helper include and check",
                "thread ON",
                "wasm_exceptions ON",
                "wasm_jspi ON",
                "wasm_simd128 OFF",
            ],
            "qtdeclarative": [
                "existing child-window stacking patch",
                "Emscripten AutoGen response threshold 4096",
                "host-only FluentWinUI3 OFF",
                "host-only Universal OFF",
            ],
        },
    }


def verify_artifacts(repo: Path, build: Path) -> dict[str, Any]:
    artifacts = [build / name for name in DEPLOYMENT_ARTIFACTS]
    exception_archive = build / "libWasmProbeExceptionBoundary.a"
    compile_database = build / "compile_commands.json"
    for artifact in (*artifacts, exception_archive, compile_database):
        require(
            artifact.is_file() and artifact.stat().st_size > 0,
            f"missing generated artifact: {artifact}",
        )

    external_workers = {
        path.name
        for path in build.iterdir()
        if path.is_file()
        and path.name.endswith((".worker.js", ".ww.js", ".aw.js"))
    }
    expected_workers = {
        "RhythmGameWasmProbe.aw.js",
        "RhythmGameWasmProbe.ww.js",
    }
    require(
        external_workers == expected_workers,
        f"unexpected external worker artifacts: {external_workers}",
    )
    require(
        not (build / "RhythmGameWasmProbe.worker.js").exists(),
        "unexpected separate pthread worker artifact",
    )

    javascript = (build / "RhythmGameWasmProbe.js").read_text(
        "utf-8",
        errors="replace",
    )
    pthread_markers = (
        "PThread.allocateUnusedWorker",
        "new Worker(pthreadMainJs",
        "ENVIRONMENT_IS_PTHREAD",
    )
    for marker in pthread_markers:
        require(marker in javascript, f"pthread JS bootstrap missing: {marker}")

    html = (build / "RhythmGameWasmProbe.html").read_text(
        "utf-8",
        errors="replace",
    )
    for script in ("RhythmGameWasmProbe.js", "qtloader.js"):
        require(
            re.search(
                rf'<script[^>]+src=["\']{re.escape(script)}["\']',
                html,
            )
            is not None,
            f"HTML does not load {script}",
        )

    qrc = build / ".qt" / "rcc" / "wasm_probe_shaders.qrc"
    generated_qrc = build / ".qt" / "rcc" / "qrc_wasm_probe_shaders.cpp"
    require(qrc.is_file(), "generated shader qrc missing")
    require(generated_qrc.is_file(), "generated shader resource C++ missing")
    qrc_text = qrc.read_text("utf-8", errors="replace")
    generated_text = generated_qrc.read_text("utf-8", errors="replace")
    require(
        '<qresource prefix="/qt/qml/RhythmGame/WasmProbe/shaders">' in qrc_text,
        "shader resource prefix drifted",
    )
    require(
        '<file alias="pulse.frag.qsb">' in qrc_text,
        "shader QSB alias missing",
    )
    require(
        ":/qt/qml/RhythmGame/WasmProbe/shaders/pulse.frag.qsb"
        in generated_text,
        "generated QSB resource path missing",
    )
    require(
        "shaders/qml/pulse.frag.qsb" not in generated_text,
        "generated QSB resource retained an unwanted qml segment",
    )

    hashed = (*artifacts, exception_archive, compile_database)
    return {
        "deploymentFiles": list(DEPLOYMENT_ARTIFACTS),
        "externalWorkerArtifacts": sorted(external_workers),
        "pthreadWorkerEmbeddedInMainJavaScript": True,
        "pthreadBootstrapMarkers": list(pthread_markers),
        "shaderResourceAlias": (
            ":/qt/qml/RhythmGame/WasmProbe/shaders/pulse.frag.qsb"
        ),
        "files": {
            path.name: {
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for path in hashed
        },
    }


def build_evidence(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    build = repo / "tools" / "wasm-probe" / "build" / "wasm-release"
    installed = repo / ".wasm-vcpkg" / "installed"
    buildtrees = repo / ".wb"
    require(build.is_dir(), f"missing probe build: {build}")
    require(installed.is_dir(), f"missing vcpkg installation: {installed}")
    require(buildtrees.is_dir(), f"missing vcpkg buildtrees: {buildtrees}")

    toolchains = verify_toolchains(repo, emsdk, vcpkg)
    qt = verify_qt_installation(repo, installed, build)
    cmake_identity = verify_cmake_identity(repo, build, buildtrees)
    compile_commands = verify_compile_commands(repo, build, buildtrees)
    ninja = repo / toolchains["ninja"]["executable"]
    application_link = verify_application_link(repo, build, ninja)
    features = verify_features_and_autogen(repo, installed, buildtrees)
    overlays = verify_overlays(repo, vcpkg)
    artifacts = verify_artifacts(repo, build)

    return {
        "schemaVersion": 1,
        "gate": "1A",
        "scope": (
            "Technical Qt/Emscripten build probe only; browser runtime "
            "capabilities remain Gate 1B work."
        ),
        "technicalProbePassed": True,
        "gate1aPassed": True,
        "gate0Satisfied": False,
        "formalGate1EntryAuthorized": False,
        "gate1Passed": False,
        "toolchains": toolchains,
        "cmakeBuild": cmake_identity,
        "qt": qt,
        "featuresAndAutogen": features,
        "compileCommands": compile_commands,
        "applicationLink": application_link,
        "overlays": overlays,
        "artifacts": artifacts,
        "unprovenUntilGate1B": [
            "Chromium runtime",
            "cross-origin isolation",
            "JSPI nested event loop",
            "QML and QSB rendering",
            "static-library exception execution",
            "QtConcurrent thread execution",
            "pthread lifecycle",
            "AudioWorklet audio callback",
            "shared-memory growth",
            "network and WebSocket behavior",
            "served video",
            "OPFS and File System Access",
            "1000-cycle teardown stress",
        ],
    }


def validate_evidence_for_write(evidence: Mapping[str, Any]) -> None:
    require(
        evidence.get("technicalProbePassed") is True,
        "technicalProbePassed must be true",
    )
    require(
        evidence.get("gate1aPassed") is True
        and evidence.get("gate0Satisfied") is False
        and evidence.get("formalGate1EntryAuthorized") is False
        and evidence.get("gate1Passed") is False,
        "gate1aPassed may only be true for a technical Gate 1A result with "
        "Gate 0, formal Gate 1 entry, and Gate 1 still false",
    )
    unproven = evidence.get("unprovenUntilGate1B")
    require(
        isinstance(unproven, list) and bool(unproven),
        "Gate 1B limitations must be explicit",
    )


def write_evidence_atomic(
    output: Path,
    evidence: Mapping[str, Any],
) -> None:
    validate_evidence_for_write(evidence)
    payload = json.dumps(evidence, indent=2, sort_keys=True) + "\n"
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            dir=output.parent,
            prefix=f".{output.name}.",
            suffix=".tmp",
            delete=False,
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(payload)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_name, output)
        temporary_name = None
    finally:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--emsdk", type=Path, required=True)
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> None:
    args = parse_arguments()
    repo = args.repo.resolve()
    evidence = build_evidence(
        repo,
        args.emsdk.resolve(),
        args.vcpkg.resolve(),
    )
    write_evidence_atomic(args.output.resolve(), evidence)


if __name__ == "__main__":
    main()
