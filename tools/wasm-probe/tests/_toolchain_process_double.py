"""Strict process double for the Windows toolchain bootstrap tests."""

from __future__ import annotations

import json
import os
import shutil
import sys
from pathlib import Path


EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
VCPKG_COMMIT = "a0400024711b283056538ac19ced80b91a83c24c"
EMSDK_URL = "https://github.com/emscripten-core/emsdk.git"
VCPKG_URL = "https://github.com/microsoft/vcpkg.git"
FAIL_ONCE_EXIT = 86


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


def _write_emscripten_driver(path: Path, tool: str) -> None:
    process_double = Path(__file__).resolve()
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


def create_repository_layout(repository: Path, kind: str) -> None:
    if kind == "emsdk":
        write_launcher(repository / "emsdk.bat", "emsdk")
        emscripten = repository / "upstream" / "emscripten"
        write_launcher(emscripten / "em++.cmd", "em++")
        write_launcher(emscripten / "emcc.cmd", "emcc")
        _write_emscripten_driver(
            emscripten / "em++.py",
            "em++-driver",
        )
        _write_emscripten_driver(
            emscripten / "emcc.py",
            "emcc-driver",
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

    _event("git", source, arguments, rejected=True)
    print(f"Unknown git invocation: {arguments!r}", file=sys.stderr)
    return 98


def _run_known_tool(tool: str, source: str, arguments: list[str]) -> int:
    _event(tool, source, arguments)
    if tool == "emsdk":
        if arguments in (["install", "4.0.7"], ["activate", "4.0.7"]):
            return 0
    elif tool == "vcpkg-bootstrap":
        if arguments == ["-disableMetrics"]:
            return 0
    elif tool in ("em++", "emcc", "poison-em++"):
        if arguments == ["--version"]:
            print("emcc 4.0.7")
            return 0
    elif tool in ("vcpkg", "poison-vcpkg"):
        if arguments == ["version"]:
            print("vcpkg package management program version fixture")
            return 0
    elif tool in ("cmake", "poison-cmake"):
        if arguments == ["--version"]:
            print("cmake version 4.2.3")
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
                "CMAKE_NINJA_FORCE_RESPONSE_FILE",
                "EM_CACHE",
                "EM_CONFIG",
                "EMSDK_NODE",
                "PYTHONDONTWRITEBYTECODE",
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
                "PATH",
            )
        }
        _event(
            "capture-environment",
            source,
            arguments,
            environment=environment,
        )
        return int(os.environ.get("TOOLCHAIN_DOUBLE_CHILD_EXIT", "0"))
    elif tool in ("em++-driver", "emcc-driver"):
        _event(
            tool,
            source,
            arguments,
            runtime=str(Path(sys.executable).resolve()),
            ignore_environment=bool(sys.flags.ignore_environment),
        )
        if arguments == ["--version"]:
            print("emcc 4.0.7")
            return 0
        return int(os.environ.get("TOOLCHAIN_DOUBLE_CHILD_EXIT", "0"))
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
        "vcpkg",
        "cmake",
        "ninja",
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
