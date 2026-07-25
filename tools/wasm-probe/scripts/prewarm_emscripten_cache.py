"""Prewarm Emscripten SYSTEM libraries with a reproducible cache prefix."""

from __future__ import annotations

import argparse
import hashlib
import inspect
import os
import stat
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path
from typing import Any, NoReturn


def _fail(message: str) -> NoReturn:
    raise RuntimeError(message)


def _same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(str(left)) == os.path.normcase(str(right))


def _parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--emscripten-root", type=Path, required=True)
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("--prefix-target", required=True)
    parser.add_argument("--system-libs-sha256", required=True)
    parser.add_argument("--emcc-launcher-sha256", required=True)
    parser.add_argument("--emcc-py-sha256", required=True)
    parser.add_argument("--emxx-launcher-sha256", required=True)
    parser.add_argument("--emxx-py-sha256", required=True)
    parser.add_argument("--emar-launcher-sha256", required=True)
    parser.add_argument("--emar-py-sha256", required=True)
    parser.add_argument("--emranlib-launcher-sha256", required=True)
    parser.add_argument("--emranlib-py-sha256", required=True)
    return parser.parse_args()


def _mapped_cflags(
    original: Callable[..., list[str]],
    cache_root: Path,
    prefix_target: str,
) -> tuple[Callable[..., list[str]], dict[str, int]]:
    mapping = f"-ffile-prefix-map={cache_root}={prefix_target}"
    audit = {"calls": 0}

    def mapped(*args: Any, **kwargs: Any) -> list[str]:
        flags = original(*args, **kwargs)
        if type(flags) is not list or not all(
            type(flag) is str for flag in flags
        ):
            _fail("Emscripten get_base_cflags return contract drifted")
        if mapping in flags:
            _fail("Emscripten cache file-prefix-map was already present")
        result = [*flags, mapping]
        if result[-1] != mapping or result.count(mapping) != 1:
            _fail("Emscripten cache file-prefix-map injection drifted")
        audit["calls"] += 1
        return result

    return mapped, audit


def _assert_regular_file(path: Path, expected_sha256: str) -> Path:
    resolved = path.resolve(strict=True)
    metadata = resolved.stat()
    if not stat.S_ISREG(metadata.st_mode) or resolved.is_symlink():
        _fail(f"Pinned Emscripten child input is not regular: {resolved}")
    actual = hashlib.sha256(resolved.read_bytes()).hexdigest()
    if actual.casefold() != expected_sha256.casefold():
        _fail(f"Pinned Emscripten child input SHA-256 drifted: {resolved}")
    return resolved


def _isolated_child_rewriter(
    emscripten_root: Path,
    arguments: argparse.Namespace,
) -> tuple[
    Callable[..., subprocess.Popen[Any]],
    dict[str, int],
]:
    python = Path(sys.executable).resolve(strict=True)
    configured_python = os.environ.get("EMSDK_PYTHON")
    if not configured_python or not _same_path(
        Path(configured_python).resolve(strict=True),
        python,
    ):
        _fail("EMSDK_PYTHON does not match the pinned prewarm Python")
    specifications = {
        "emcc": (
            "emcc.bat",
            "emcc.py",
            arguments.emcc_py_sha256,
        ),
        "em++": (
            "em++.bat",
            "em++.py",
            arguments.emxx_py_sha256,
        ),
        "emar": (
            "emar.bat",
            "emar.py",
            arguments.emar_py_sha256,
        ),
        "emranlib": (
            "emranlib.bat",
            "emranlib.py",
            arguments.emranlib_py_sha256,
        ),
    }
    expected_launchers = {
        "emcc": arguments.emcc_launcher_sha256,
        "em++": arguments.emxx_launcher_sha256,
        "emar": arguments.emar_launcher_sha256,
        "emranlib": arguments.emranlib_launcher_sha256,
    }
    launchers: dict[Path, tuple[str, Path]] = {}
    counters = {name: 0 for name in specifications}
    for name, (launcher_name, driver_name, driver_sha256) in (
        specifications.items()
    ):
        launcher = _assert_regular_file(
            emscripten_root / launcher_name,
            expected_launchers[name],
        )
        driver = _assert_regular_file(
            emscripten_root / driver_name,
            driver_sha256,
        )
        launchers[launcher] = (name, driver)

    original = subprocess.Popen

    def guarded_popen(
        command: Any,
        *popen_arguments: Any,
        **popen_keywords: Any,
    ) -> subprocess.Popen[Any]:
        executable = popen_keywords.get("executable")
        if executable is not None:
            _fail("Emscripten prewarm child executable override is forbidden")
        if isinstance(command, (str, bytes, os.PathLike)):
            _fail("Emscripten prewarm string child command is forbidden")
        child = [os.fspath(value) for value in command]
        if not child:
            _fail("Emscripten prewarm child command is empty")
        head = Path(child[0]).resolve(strict=True)
        replacement = launchers.get(head)
        if replacement is not None:
            name, driver = replacement
            counters[name] += 1
            child = [
                str(python),
                "-s",
                "-B",
                "-E",
                str(driver),
                *child[1:],
            ]
        elif head.suffix.casefold() in {".bat", ".cmd"}:
            try:
                head.relative_to(emscripten_root)
            except ValueError:
                pass
            else:
                _fail(
                    "Unexpected Emscripten prewarm batch child is forbidden: "
                    f"{head}"
                )
        return original(child, *popen_arguments, **popen_keywords)

    return guarded_popen, counters


def main() -> int:
    arguments = _parse_arguments()
    emscripten_root = arguments.emscripten_root.resolve(strict=True)
    cache_root = arguments.cache_root.resolve(strict=True)
    if not emscripten_root.is_dir() or not cache_root.is_dir():
        _fail("Emscripten and cache roots must be regular directories")
    configured_cache = os.environ.get("EM_CACHE")
    if not configured_cache or not _same_path(
        Path(configured_cache).resolve(strict=True),
        cache_root,
    ):
        _fail("EM_CACHE does not match the authenticated cache root")
    if os.environ.get("EMCC_CORES") != "4":
        _fail("EMCC_CORES must be exactly 4")
    if arguments.prefix_target != "/emsdk/cache":
        _fail("Emscripten cache prefix target drifted")
    forbidden_environment = (
        "CCC_OVERRIDE_OPTIONS",
        "EMCC_CFLAGS",
        "EM_COMPILER_WRAPPER",
        "EM_COMPILER_WRAPPER2",
        "NODE_OPTIONS",
        "NODE_PATH",
        "PYTHONHOME",
        "PYTHONPATH",
    )
    survivors = [
        name for name in forbidden_environment if name in os.environ
    ]
    if survivors:
        _fail(f"Ambient prewarm environment survived scrubbing: {survivors}")
    if os.environ.get("PYTHONNOUSERSITE") != "1":
        _fail("PYTHONNOUSERSITE must be exactly 1")

    sys.path.insert(0, str(emscripten_root))
    import embuilder  # type: ignore[import-not-found]  # noqa: PLC0415
    from tools import system_libs  # type: ignore[import-not-found]  # noqa: PLC0415

    if embuilder.system_libs is not system_libs:
        _fail("Emscripten system_libs module identity drifted")
    if system_libs.USE_NINJA != 0:
        _fail("Emscripten SYSTEM prewarm must not use Ninja")
    system_libs_path = (
        emscripten_root / "tools" / "system_libs.py"
    ).resolve(strict=True)
    imported_source = inspect.getsourcefile(system_libs.get_base_cflags)
    if imported_source is None or not _same_path(
        Path(imported_source).resolve(strict=True),
        system_libs_path,
    ):
        _fail("Emscripten get_base_cflags source path drifted")
    source_sha256 = hashlib.sha256(system_libs_path.read_bytes()).hexdigest()
    if source_sha256.casefold() != arguments.system_libs_sha256.casefold():
        _fail("Emscripten system_libs.py SHA-256 drifted")
    if (
        str(inspect.signature(system_libs.get_base_cflags))
        != "(build_dir, force_object_files=False, preprocess=True)"
    ):
        _fail("Emscripten get_base_cflags signature drifted")
    mapped, audit = _mapped_cflags(
        system_libs.get_base_cflags,
        cache_root,
        arguments.prefix_target,
    )
    system_libs.get_base_cflags = mapped
    sys.argv = [str(emscripten_root / "embuilder.py"), "build", "SYSTEM"]
    guarded_popen, child_audit = _isolated_child_rewriter(
        emscripten_root,
        arguments,
    )
    original_popen = subprocess.Popen
    try:
        subprocess.Popen = guarded_popen
        result = int(embuilder.main())
    finally:
        subprocess.Popen = original_popen
    if audit["calls"] <= 0:
        _fail("Emscripten get_base_cflags prefix map was never exercised")
    for required in ("emcc", "em++", "emar"):
        if child_audit[required] <= 0:
            _fail(
                f"Emscripten isolated {required} child path was not exercised"
            )
    return result


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
