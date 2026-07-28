"""Fail when the private chart root leaks into generated or linked files."""

from __future__ import annotations

import argparse
import os
import stat
import sys
from pathlib import Path


FILE_ATTRIBUTE_REPARSE_POINT = 0x400
EXPECTED_LEAVES = {
    "RhythmGameWasmProbe.aw.js",
    "RhythmGameWasmProbe.js",
    "RhythmGameWasmProbe.wasm",
    "RhythmGameWasmProbe.ww.js",
}


def _is_reparse(path: Path, metadata: os.stat_result) -> bool:
    return path.is_symlink() or bool(
        (getattr(metadata, "st_file_attributes", 0) or 0)
        & FILE_ATTRIBUTE_REPARSE_POINT
    )


def _assert_regular(path: Path) -> Path:
    lexical = Path(os.path.abspath(path))
    for candidate in [*reversed(lexical.parents), lexical]:
        if not candidate.exists() and not candidate.is_symlink():
            raise RuntimeError(f"artifact path is missing: {candidate}")
        metadata = candidate.lstat()
        if _is_reparse(candidate, metadata):
            raise RuntimeError(
                f"artifact reparse component is forbidden: {candidate}"
            )
        if candidate == lexical:
            if not stat.S_ISREG(metadata.st_mode):
                raise RuntimeError(f"artifact is not regular: {candidate}")
        elif not stat.S_ISDIR(metadata.st_mode):
            raise RuntimeError(
                f"artifact path component is not a directory: {candidate}"
            )
    return lexical


def _private_root_needles(host_root: str) -> set[tuple[str, bytes]]:
    if not host_root:
        raise RuntimeError("host root is empty")
    spellings = {
        host_root,
        host_root.replace("\\", "/"),
        host_root.replace("/", "\\"),
    }
    return {
        (encoding, spelling.encode(encoding))
        for spelling in spellings
        for encoding in ("utf-8", "utf-16le", "utf-16be")
    }


def _scan_paths(host_root: str, paths: list[Path]) -> None:
    checked = [_assert_regular(path) for path in paths]
    if not checked:
        raise RuntimeError("at least one scan input is required")
    if len({os.path.normcase(str(path)) for path in checked}) != len(checked):
        raise RuntimeError("scan input paths are duplicated")

    needles = _private_root_needles(host_root)
    for path in checked:
        content = path.read_bytes()
        folded_content = content.lower()
        for encoding, needle in needles:
            if needle and needle.lower() in folded_content:
                raise RuntimeError(
                    f"private chart root leak ({encoding}) in {path.name}"
                )


def scan(host_root: str, artifacts: list[Path]) -> None:
    if len(artifacts) != 4:
        raise RuntimeError("exactly four post-link artifacts are required")
    paths = [_assert_regular(path) for path in artifacts]
    if {path.name for path in paths} != EXPECTED_LEAVES:
        raise RuntimeError("post-link artifact leaf inventory drifted")
    _scan_paths(host_root, paths)


def scan_generated_inputs(host_root: str, inputs: list[Path]) -> None:
    _scan_paths(host_root, inputs)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host-root", required=True)
    parser.add_argument("--artifact", action="append", type=Path)
    parser.add_argument("--generated-input", action="append", type=Path)
    arguments = parser.parse_args(argv)
    try:
        artifacts = arguments.artifact or []
        generated_inputs = arguments.generated_input or []
        if bool(artifacts) == bool(generated_inputs):
            raise RuntimeError(
                "select exactly one scan mode: --artifact or "
                "--generated-input"
            )
        if artifacts:
            scan(arguments.host_root, artifacts)
            message = "artifact-scan: four link leaves are host-path-free"
        else:
            scan_generated_inputs(arguments.host_root, generated_inputs)
            message = (
                "artifact-scan: generated C++ and manifests are "
                "host-path-free"
            )
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"artifact-scan error: {error}", file=sys.stderr)
        return 2
    print(message)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
