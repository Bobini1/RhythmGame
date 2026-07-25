"""Strict process double for the Windows toolchain bootstrap tests."""

from __future__ import annotations

import json
import os
import shutil
import sys
import zipfile
from pathlib import Path


EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
VCPKG_COMMIT = "a0400024711b283056538ac19ced80b91a83c24c"
EMSDK_URL = "https://github.com/emscripten-core/emsdk.git"
VCPKG_URL = "https://github.com/microsoft/vcpkg.git"
FAIL_ONCE_EXIT = 86
PORT_CMAKE_ENTRY = {
    "name": "cmake",
    "os": "windows",
    "arch": "amd64",
    "version": "4.3.3",
    "executable": "cmake-4.3.3-windows-x86_64/bin/cmake.cmd",
    "url": "https://fixture.invalid/cmake-4.3.3-windows-x86_64.zip",
    "sha512": "",
    "archive": "cmake-4.3.3-windows-x86_64.zip",
}


def _resolved(path: str | Path) -> Path:
    return Path(path).resolve(strict=False)


def _same_path(left: str | Path, right: str | Path) -> bool:
    return os.path.normcase(str(_resolved(left))) == os.path.normcase(
        str(_resolved(right))
    )


def write_launcher(path: Path, tool: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    python = Path(sys.executable).resolve()
    process_double = Path(__file__).resolve()
    path.write_text(
        "\n".join(
            (
                "@echo off",
                (
                    f'"{python}" "{process_double}" "{tool}" '
                    '"%~f0" %*'
                ),
                "exit /b %ERRORLEVEL%",
                "",
            )
        ),
        encoding="utf-8",
    )


def _copy_or_link(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        os.link(source, destination)
    except OSError:
        shutil.copy2(source, destination)


def _seed_native_python(repository: Path) -> None:
    source = Path(sys.executable).resolve()
    source_root = source.parent
    python_root = repository / "python"
    python = python_root / "python.exe"
    _copy_or_link(source, python)
    version = f"{sys.version_info.major}{sys.version_info.minor}"
    for name in (
        "python3.dll",
        f"python{version}.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll",
    ):
        candidate = source_root / name
        if candidate.is_file():
            _copy_or_link(candidate, python_root / name)
    (python_root / f"python{version}._pth").write_text(
        "\n".join(
            (
                str(source_root / f"python{version}.zip"),
                str(source_root / "DLLs"),
                str(source_root / "Lib"),
                "import site",
                "",
            )
        ),
        encoding="utf-8",
    )
    (python_root / ".emsdk_version").write_text(
        "fixture emsdk Python metadata\n",
        encoding="utf-8",
    )


def _write_emscripten_driver(
    path: Path,
    tool: str,
    *,
    importable_emcc: bool = False,
) -> None:
    process_double = Path(__file__).resolve()
    if importable_emcc:
        path.write_text(
            "\n".join(
                (
                    "from pathlib import Path",
                    "import subprocess",
                    "import sys",
                    "",
                    f"PROCESS_DOUBLE = Path({str(process_double)!r})",
                    "",
                    "def main(args):",
                    (
                        "    sys.path.insert("
                        "0, str(Path(__file__).resolve().parent))"
                    ),
                    "    from tools import shared",
                    "    tool = (",
                    "        'em++-driver'",
                    "        if shared.run_via_emxx",
                    "        else 'emcc-driver'",
                    "    )",
                    "    completed = subprocess.run(",
                    "        [",
                    "            sys.executable,",
                    "            str(PROCESS_DOUBLE),",
                    "            tool,",
                    "            str(Path(__file__).resolve()),",
                    "            *args[1:],",
                    "        ],",
                    "        check=False,",
                    "    )",
                    "    return completed.returncode",
                    "",
                    "if __name__ == '__main__':",
                    "    raise SystemExit(main(sys.argv))",
                    "",
                )
            ),
            encoding="utf-8",
        )
        return
    path.write_text(
        "\n".join(
            (
                "from pathlib import Path",
                "import runpy",
                "import sys",
                "",
                f"process_double = Path({str(process_double)!r})",
                (
                    f"sys.argv = [str(process_double), {tool!r}, "
                    "str(Path(__file__).resolve()), *sys.argv[1:]]"
                ),
                "runpy.run_path(str(process_double), run_name='__main__')",
                "",
            )
        ),
        encoding="utf-8",
    )


def _write_embuilder_module(path: Path) -> None:
    process_double = Path(__file__).resolve()
    path.write_text(
        "\n".join(
            (
                "import json",
                "import os",
                "from pathlib import Path",
                "import subprocess",
                "import sys",
                "from tools import system_libs",
                "",
                f"PROCESS_DOUBLE = Path({str(process_double)!r})",
                "",
                "def main():",
                "    if sys.argv[1:] != ['build', 'SYSTEM']:",
                "        return 97",
                "    representative = [",
                "        system_libs.get_base_cflags('c-build'),",
                "        system_libs.get_base_cflags(",
                "            'cxx-build', force_object_files=True",
                "        ),",
                "    ]",
                "    root = Path(__file__).resolve().parent",
                "    for child in ('emcc.bat', 'em++.bat', 'emar.bat'):",
                "        completed = subprocess.run(",
                "            [str(root / child), '--fixture-child'],",
                "            check=False,",
                "        )",
                "        if completed.returncode:",
                "            return completed.returncode",
                "    environment = os.environ.copy()",
                "    environment['TOOLCHAIN_DOUBLE_PREWARM_FLAGS'] = (",
                "        json.dumps(representative)",
                "    )",
                "    completed = subprocess.run(",
                "        [",
                "            sys.executable,",
                "            str(PROCESS_DOUBLE),",
                "            'embuilder',",
                "            str(Path(__file__).resolve()),",
                "            'build',",
                "            'SYSTEM',",
                "        ],",
                "        env=environment,",
                "        check=False,",
                "    )",
                "    return completed.returncode",
                "",
            )
        ),
        encoding="utf-8",
    )


def create_repository_layout(repository: Path, kind: str) -> None:
    if kind == "emsdk":
        _write_emscripten_driver(repository / "emsdk.py", "emsdk")
        emscripten = repository / "upstream" / "emscripten"
        write_launcher(emscripten / "em++.bat", "em++")
        write_launcher(emscripten / "emcc.bat", "emcc")
        write_launcher(emscripten / "emar.bat", "emar")
        write_launcher(emscripten / "emranlib.bat", "emranlib")
        _write_emscripten_driver(
            emscripten / "em++.py",
            "em++-driver",
        )
        _write_emscripten_driver(
            emscripten / "emcc.py",
            "emcc-driver",
            importable_emcc=True,
        )
        _write_emscripten_driver(
            emscripten / "emar.py",
            "emar-driver",
        )
        _write_emscripten_driver(
            emscripten / "emranlib.py",
            "emranlib-driver",
        )
        _write_embuilder_module(emscripten / "embuilder.py")
        tools = emscripten / "tools"
        tools.mkdir(parents=True, exist_ok=True)
        (tools / "__init__.py").write_text("", encoding="utf-8")
        (tools / "shared.py").write_text(
            "run_via_emxx = False\n",
            encoding="utf-8",
        )
        (tools / "response_file.py").write_text(
            "fixture = True\n",
            encoding="utf-8",
        )
        (tools / "config.py").write_text(
            "fixture = True\n",
            encoding="utf-8",
        )
        (tools / "system_libs.py").write_text(
            "\n".join(
                (
                    "USE_NINJA = 0",
                    "",
                    "def get_base_cflags(",
                    "    build_dir,",
                    "    force_object_files=False,",
                    "    preprocess=True,",
                    "):",
                    "    return [",
                    "        '-g',",
                    "        '-ffile-prefix-map=/source=/emsdk/emscripten',",
                    "        '-ffile-prefix-map=../source=/emsdk/emscripten',",
                    "    ]",
                    "",
                )
            ),
            encoding="utf-8",
        )
        _seed_native_python(repository)
        _copy_or_link(
            Path(sys.executable).resolve(),
            repository / "node" / "node.exe",
        )
        (repository / "upstream" / "bin").mkdir(
            parents=True,
            exist_ok=True,
        )
        (repository / "upstream" / "bin" / "clang.exe").write_bytes(
            b"fixture clang payload\n"
        )
        (repository / "emscripten-releases-tags.json").write_text(
            json.dumps(
                {
                    "releases": {
                        "4.0.7": (
                            "ef4e9cedeac3332e4738087567552063f4f250d3"
                        )
                    }
                },
                sort_keys=True,
            ),
            encoding="utf-8",
        )
        (repository / ".emscripten").write_text(
            "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'\n",
            encoding="utf-8",
        )
        (repository / "emsdk_env.ps1").write_text(
            "\n".join(
                (
                    "$env:EMSDK = $PSScriptRoot",
                    (
                        "$env:EMSDK_PYTHON = Join-Path "
                        "$PSScriptRoot 'python\\python.exe'"
                    ),
                    (
                        '$env:Path = "$PSScriptRoot'
                        "$([IO.Path]::PathSeparator)"
                        "$PSScriptRoot\\upstream\\emscripten"
                        '$([IO.Path]::PathSeparator)$env:Path"'
                    ),
                    "",
                )
            ),
            encoding="utf-8",
        )
    elif kind == "vcpkg":
        write_launcher(repository / "bootstrap-vcpkg.bat", "vcpkg-bootstrap")
        write_launcher(repository / "vcpkg.cmd", "vcpkg")
        bootstrap_script = repository / "scripts" / "bootstrap.ps1"
        bootstrap_script.parent.mkdir(parents=True, exist_ok=True)
        bootstrap_script.write_text(
            "# authenticated fixture bootstrap implementation\n",
            encoding="utf-8",
        )
        (repository / "scripts" / "vcpkg-tool-metadata.txt").write_text(
            "\n".join(
                (
                    "VCPKG_TOOL_RELEASE_TAG=fixture-release",
                    "",
                )
            ),
            encoding="utf-8",
        )
        tools = repository / "scripts" / "vcpkg-tools.json"
        tools.parent.mkdir(parents=True, exist_ok=True)
        port_entry = dict(PORT_CMAKE_ENTRY)
        port_entry["sha512"] = os.environ.get(
            "TOOLCHAIN_DOUBLE_PORT_CMAKE_SHA512",
            port_entry["sha512"],
        )
        tools.write_text(
            json.dumps(
                {
                    "schema-version": 1,
                    "tools": [port_entry],
                },
                sort_keys=True,
            ),
            encoding="utf-8",
        )
    else:
        raise ValueError(f"Unknown repository kind: {kind}")


def seed_repository(repository: Path, kind: str, head: str) -> None:
    repository.mkdir(parents=True, exist_ok=True)
    (repository / ".fixture-head").write_text(head, encoding="ascii")
    create_repository_layout(repository, kind)


def seed_build_tools(toolchain_root: Path) -> None:
    cmake_root = toolchain_root / "cmake-4.2.3-windows-x86_64"
    write_launcher(cmake_root / "bin" / "cmake.cmd", "cmake")
    support = (
        cmake_root
        / "share"
        / "cmake-4.2"
        / "Modules"
        / "FixtureSupport.cmake"
    )
    support.parent.mkdir(parents=True, exist_ok=True)
    support.write_bytes(b"set(FIXTURE_SUPPORT authenticated)\n")
    write_launcher(
        toolchain_root / "ninja-1.13.2-win" / "ninja.cmd",
        "ninja",
    )


def seed_emscripten_cache(toolchain_root: Path) -> Path:
    cache = toolchain_root / "emscripten-cache-4.0.7"
    library = cache / "sysroot" / "lib" / "wasm32-emscripten" / "libc.a"
    library.parent.mkdir(parents=True, exist_ok=True)
    library.write_bytes(b"fixture frozen libc archive\n")
    (cache / "sysroot_install.stamp").write_bytes(b"1")
    return cache


def seed_vcpkg_port_cmake(state_root: Path) -> tuple[Path, Path]:
    downloads = state_root / "downloads"
    installation = downloads / "tools" / "cmake-4.3.3-windows"
    executable = (
        installation
        / "cmake-4.3.3-windows-x86_64"
        / "bin"
        / "cmake.cmd"
    )
    write_launcher(executable, "port-cmake")
    support = (
        installation
        / "cmake-4.3.3-windows-x86_64"
        / "share"
        / "cmake-4.3"
        / "Modules"
        / "FixtureSupport.cmake"
    )
    support.parent.mkdir(parents=True, exist_ok=True)
    support.write_bytes(b"set(FIXTURE_PORT_CMAKE authenticated)\n")
    archive = downloads / PORT_CMAKE_ENTRY["archive"]
    archive.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(
        archive,
        "w",
        compression=zipfile.ZIP_DEFLATED,
    ) as bundle:
        for path in (executable, support):
            relative = path.relative_to(installation).as_posix()
            info = zipfile.ZipInfo(relative, date_time=(2020, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            bundle.writestr(info, path.read_bytes())
    return archive, installation


def _event_path() -> Path:
    value = os.environ.get("TOOLCHAIN_DOUBLE_EVENTS")
    if not value:
        raise RuntimeError("TOOLCHAIN_DOUBLE_EVENTS is required")
    return Path(value)


def _toolchain_root() -> Path:
    value = os.environ.get("TOOLCHAIN_DOUBLE_ROOT")
    if not value:
        raise RuntimeError("TOOLCHAIN_DOUBLE_ROOT is required")
    return _resolved(value)


def _event(tool: str, source: str, arguments: list[str], **extra: object) -> None:
    event: dict[str, object] = {
        "tool": tool,
        "args": arguments,
        "cwd": str(Path.cwd().resolve()),
        "source": str(_resolved(source)),
    }
    event.update(extra)
    target = _event_path()
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(event, sort_keys=True) + "\n")


def _repository_kind(repository: Path) -> str:
    name = repository.name
    if name in ("emsdk-4.0.7", "emsdk-4.0.7.bootstrap-tmp"):
        return "emsdk"
    if name in ("vcpkg-a0400024", "vcpkg-a0400024.bootstrap-tmp"):
        return "vcpkg"
    raise RuntimeError(f"Unexpected repository path: {repository}")


def _canonical_for(repository: Path) -> Path:
    suffix = ".bootstrap-tmp"
    if repository.name.endswith(suffix):
        return repository.with_name(repository.name[: -len(suffix)])
    return repository


def _run_git(source: str, arguments: list[str]) -> int:
    root = _toolchain_root()
    if arguments[:2] == ["clone", "--filter=blob:none"] and len(arguments) == 4:
        url = arguments[2]
        destination = _resolved(arguments[3])
        expected = {
            EMSDK_URL: root / "emsdk-4.0.7.bootstrap-tmp",
            VCPKG_URL: root / "vcpkg-a0400024.bootstrap-tmp",
        }
        if url not in expected or not _same_path(destination, expected[url]):
            _event("git", source, arguments, rejected=True)
            print("Rejected clone target", file=sys.stderr)
            return 94
        canonical = _canonical_for(destination)
        _event(
            "git",
            source,
            arguments,
            canonical_exists=canonical.exists(),
        )
        marker_value = os.environ.get("TOOLCHAIN_DOUBLE_FAIL_CLONE_ONCE")
        if marker_value:
            marker = Path(marker_value)
            if not marker.exists():
                destination.mkdir(parents=True, exist_ok=True)
                (destination / "partial").write_text("partial", encoding="ascii")
                marker.write_text("failed", encoding="ascii")
                print(
                    f"Intentional fail-once clone ({FAIL_ONCE_EXIT})",
                    file=sys.stderr,
                )
                return FAIL_ONCE_EXIT
        destination.mkdir(parents=True, exist_ok=False)
        return 0

    if len(arguments) >= 4 and arguments[0] == "-C":
        repository = _resolved(arguments[1])
        canonical = _canonical_for(repository)
        expected_paths = {
            root / "emsdk-4.0.7",
            root / "emsdk-4.0.7.bootstrap-tmp",
            root / "vcpkg-a0400024",
            root / "vcpkg-a0400024.bootstrap-tmp",
        }
        if not any(_same_path(repository, path) for path in expected_paths):
            _event("git", source, arguments, rejected=True)
            print("Rejected git repository", file=sys.stderr)
            return 95
        _event(
            "git",
            source,
            arguments,
            canonical_exists=canonical.exists(),
        )
        operation = arguments[2:]
        if operation[:2] == ["checkout", "--detach"] and len(operation) == 3:
            commit = operation[2]
            expected_commit = {
                "emsdk": EMSDK_COMMIT,
                "vcpkg": VCPKG_COMMIT,
            }[_repository_kind(repository)]
            if commit != expected_commit:
                print("Rejected checkout commit", file=sys.stderr)
                return 96
            (repository / ".fixture-head").write_text(commit, encoding="ascii")
            create_repository_layout(repository, _repository_kind(repository))
            return 0
        if operation == ["rev-parse", "HEAD"]:
            head_file = repository / ".fixture-head"
            if not head_file.is_file():
                print("Missing fixture HEAD", file=sys.stderr)
                return 97
            print(head_file.read_text(encoding="ascii").strip())
            return 0
        if operation == [
            "diff",
            "--quiet",
            "--no-ext-diff",
            "--no-textconv",
            "--ignore-submodules=all",
            "--",
        ]:
            return 1 if (repository / ".fixture-worktree-dirty").exists() else 0
        if operation == [
            "diff",
            "--cached",
            "--quiet",
            "--no-ext-diff",
            "--no-textconv",
            "--ignore-submodules=all",
            "HEAD",
            "--",
        ]:
            return 1 if (repository / ".fixture-index-dirty").exists() else 0
        if (
            operation[:3] == ["ls-files", "--error-unmatch", "--"]
            and len(operation) == 4
        ):
            relative = operation[3].replace("\\", "/")
            expected = {
                "emsdk": {"emsdk.bat"},
                "vcpkg": {
                    "bootstrap-vcpkg.bat",
                    "scripts/vcpkg-tools.json",
                },
            }[_repository_kind(repository)]
            if relative in expected and (repository / relative).is_file():
                print(relative)
                return 0
            return 1

    _event("git", source, arguments, rejected=True)
    print(f"Unknown git invocation: {arguments!r}", file=sys.stderr)
    return 98


def _run_known_tool(tool: str, source: str, arguments: list[str]) -> int:
    execution: dict[str, object] = {}
    if tool == "emsdk":
        execution = {
            "runtime": str(Path(sys.executable).resolve()),
            "ignore_environment": bool(sys.flags.ignore_environment),
            "no_user_site": bool(sys.flags.no_user_site),
            "bytecode_disabled": bool(sys.dont_write_bytecode),
        }
    _event(tool, source, arguments, **execution)
    if tool == "emsdk":
        if arguments in (["install", "4.0.7"], ["activate", "4.0.7"]):
            if arguments[0] == "install":
                (
                    Path(source).resolve().parent
                    / "python"
                    / ".emsdk_version"
                ).write_text(
                    "fixture emsdk Python metadata\n",
                    encoding="utf-8",
                )
            return 0
    elif tool == "vcpkg-bootstrap":
        if arguments == ["-disableMetrics"]:
            return 0
    elif tool in ("em++", "emcc", "poison-em++"):
        if arguments == ["--version"]:
            print("emcc 4.0.7")
            return 0
    elif tool in ("vcpkg", "poison-vcpkg"):
        if arguments in (["version"], ["version", "--disable-metrics"]):
            print("vcpkg package management program version fixture")
            return 0
        if (
            len(arguments) == 4
            and arguments[:2] == ["fetch", "cmake"]
            and arguments[2].startswith("--downloads-root=")
            and arguments[3].startswith("--vcpkg-root=")
        ):
            downloads = _resolved(arguments[2].partition("=")[2])
            state_value = os.environ.get("TOOLCHAIN_DOUBLE_VCPKG_STATE")
            if not state_value:
                raise RuntimeError("TOOLCHAIN_DOUBLE_VCPKG_STATE is required")
            state = _resolved(state_value)
            if not _same_path(downloads, state / "downloads"):
                print("Rejected vcpkg downloads root", file=sys.stderr)
                return 93
            _, installation = seed_vcpkg_port_cmake(state)
            print(
                installation
                / "cmake-4.3.3-windows-x86_64"
                / "bin"
                / "cmake.cmd"
            )
            return 0
    elif tool in ("cmake", "poison-cmake"):
        if arguments == ["--version"]:
            print("cmake version 4.2.3")
            return 0
    elif tool == "port-cmake":
        if arguments == ["--version"]:
            print("cmake version 4.3.3")
            return 0
    elif tool in ("ninja", "poison-ninja"):
        if arguments == ["--version"]:
            print("1.13.2")
            return 0
    elif tool == "capture":
        environment = {
            name: os.environ.get(name)
            for name in (
                "EMSDK",
                "EMSDK_PYTHON",
                "EMSCRIPTEN_ROOT",
                "EMSCRIPTEN_VERSION",
                "VCPKG_ROOT",
                "VCPKG_DISABLE_METRICS",
                "VCPKG_DEFAULT_BINARY_CACHE",
                "VCPKG_MAX_CONCURRENCY",
                "CMAKE_NINJA_FORCE_RESPONSE_FILE",
                "EM_CACHE",
                "EM_FROZEN_CACHE",
                "EM_CONFIG",
                "EMSDK_NODE",
                "PYTHONDONTWRITEBYTECODE",
                "PYTHONNOUSERSITE",
                "SOURCE_DATE_EPOCH",
                "QT_RCC_SOURCE_DATE_OVERRIDE",
                "NODE_OPTIONS",
                "NODE_PATH",
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
                "PATH",
            )
        }
        _event(
            "capture-environment",
            source,
            arguments,
            environment=environment,
        )
        if os.environ.get("TOOLCHAIN_DOUBLE_MUTATE_CACHE") == "1":
            cache = Path(str(os.environ["EM_CACHE"]))
            library = (
                cache
                / "sysroot"
                / "lib"
                / "wasm32-emscripten"
                / "libc.a"
            )
            timestamps = (
                library.stat().st_atime_ns,
                library.stat().st_mtime_ns,
            )
            content = library.read_bytes()
            library.write_bytes(
                bytes((content[0] ^ 0x01,))
                + content[1:]
            )
            os.utime(library, ns=timestamps)
        if os.environ.get("TOOLCHAIN_DOUBLE_MUTATE_LOCKED_PATHS") == "1":
            outcomes: list[dict[str, object]] = []
            for value in arguments:
                target = Path(value)
                original = target.read_bytes()
                try:
                    target.write_bytes(
                        bytes((original[0] ^ 0x01,))
                        + original[1:]
                    )
                except OSError as error:
                    outcomes.append(
                        {
                            "path": str(target),
                            "denied": True,
                            "winerror": getattr(error, "winerror", None),
                        }
                    )
                else:
                    outcomes.append(
                        {
                            "path": str(target),
                            "denied": False,
                        }
                    )
                    target.write_bytes(original)
            _event(
                "locked-path-mutation",
                source,
                [],
                outcomes=outcomes,
            )
            if not all(bool(outcome["denied"]) for outcome in outcomes):
                return 89
        return int(os.environ.get("TOOLCHAIN_DOUBLE_CHILD_EXIT", "0"))
    elif tool in (
        "em++-driver",
        "emcc-driver",
        "emar-driver",
        "emranlib-driver",
    ):
        _event(
            tool,
            source,
            arguments,
            runtime=str(Path(sys.executable).resolve()),
            ignore_environment=bool(sys.flags.ignore_environment),
            no_user_site=bool(sys.flags.no_user_site),
        )
        if arguments == ["--version"]:
            print("emcc 4.0.7")
            return 0
        return int(os.environ.get("TOOLCHAIN_DOUBLE_CHILD_EXIT", "0"))
    elif tool == "embuilder":
        if arguments == ["build", "SYSTEM"]:
            poisoned = [
                name
                for name in (
                    "EMCC_CFLAGS",
                    "EM_COMPILER_WRAPPER",
                    "EM_COMPILER_WRAPPER2",
                    "CPATH",
                    "C_INCLUDE_PATH",
                    "CPLUS_INCLUDE_PATH",
                    "CFLAGS",
                    "CXXFLAGS",
                    "LDFLAGS",
                    "NODE_OPTIONS",
                    "NODE_PATH",
                )
                if name in os.environ
            ]
            if poisoned:
                print(
                    f"Poisoned embuilder environment: {poisoned}",
                    file=sys.stderr,
                )
                return 89
            if os.environ.get("PYTHONNOUSERSITE") != "1":
                print(
                    "PYTHONNOUSERSITE must be exactly 1",
                    file=sys.stderr,
                )
                return 93
            if "CCC_OVERRIDE_OPTIONS" in os.environ:
                print(
                    "CCC_OVERRIDE_OPTIONS must remain scrubbed",
                    file=sys.stderr,
                )
                return 94
            expected_prefix_map = (
                "-ffile-prefix-map="
                f"{Path(str(os.environ.get('EM_CACHE', ''))).resolve()}"
                "=/emsdk/cache"
            )
            try:
                representative = json.loads(
                    os.environ["TOOLCHAIN_DOUBLE_PREWARM_FLAGS"]
                )
            except (KeyError, json.JSONDecodeError):
                print("Missing prewarm flag audit", file=sys.stderr)
                return 95
            if (
                len(representative) != 2
                or any(
                    flags[-1] != expected_prefix_map
                    or flags.count(expected_prefix_map) != 1
                    for flags in representative
                )
            ):
                print("Compiler path-prefix-map argv drifted", file=sys.stderr)
                return 96
            if os.environ.get("EMCC_CORES") != "4":
                print("EMCC_CORES must be exactly 4", file=sys.stderr)
                return 92
            cache_value = os.environ.get("EM_CACHE")
            if not cache_value:
                print("EM_CACHE is required", file=sys.stderr)
                return 91
            cache = Path(cache_value)
            if cache.exists() and any(cache.iterdir()):
                print("EM_CACHE must be exact-empty", file=sys.stderr)
                return 90
            library = (
                cache
                / "sysroot"
                / "lib"
                / "wasm32-emscripten"
                / "libc.a"
            )
            library.parent.mkdir(parents=True, exist_ok=True)
            library.write_bytes(b"fixture frozen libc archive\n")
            (cache / "sysroot_install.stamp").write_bytes(b"1")
            (cache / "sanity.txt").write_text(
                f"4.0.7|{cache}\n",
                encoding="utf-8",
            )
            symbols = cache / "symbol_lists"
            symbols.mkdir()
            (symbols / "fixture.json").write_text("{}\n", encoding="utf-8")
            return 0
    print(f"Unknown {tool} invocation: {arguments!r}", file=sys.stderr)
    return 99


def main() -> int:
    if len(sys.argv) < 3:
        print("Expected tool and launcher source", file=sys.stderr)
        return 100
    tool = sys.argv[1]
    source = sys.argv[2]
    arguments = sys.argv[3:]
    if tool == "git":
        return _run_git(source, arguments)
    known = {
        "emsdk",
        "vcpkg-bootstrap",
        "em++",
        "emcc",
        "em++-driver",
        "emcc-driver",
        "emar",
        "emar-driver",
        "emranlib",
        "emranlib-driver",
        "embuilder",
        "vcpkg",
        "cmake",
        "ninja",
        "port-cmake",
        "poison-em++",
        "poison-vcpkg",
        "poison-cmake",
        "poison-ninja",
        "capture",
    }
    if tool in known:
        return _run_known_tool(tool, source, arguments)
    _event(tool, source, arguments, rejected=True)
    print(f"Unknown tool: {tool}", file=sys.stderr)
    return 101


if __name__ == "__main__":
    raise SystemExit(main())
