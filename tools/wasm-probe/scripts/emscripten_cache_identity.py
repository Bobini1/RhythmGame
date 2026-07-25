"""Authenticate the exact raw file tree of a frozen Emscripten cache."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import stat
import sys
from pathlib import Path
from typing import NoReturn


ALGORITHM = "sha256-path-null-digest-lf-v1"
FILE_ATTRIBUTE_REPARSE_POINT = 0x400


class CacheIdentityError(RuntimeError):
    pass


def _fail(message: str) -> NoReturn:
    raise CacheIdentityError(message)


def _is_reparse(path: Path) -> bool:
    metadata = path.lstat()
    return path.is_symlink() or bool(
        getattr(metadata, "st_file_attributes", 0)
        & FILE_ATTRIBUTE_REPARSE_POINT
    )


def _assert_root_chain(root: Path) -> None:
    for candidate in reversed((root, *root.parents)):
        if candidate.exists() and _is_reparse(candidate):
            _fail(
                "Emscripten cache path reparse point is forbidden: "
                f"{candidate}"
            )


def _entries(root: Path) -> tuple[list[tuple[str, Path]], list[str]]:
    files: dict[str, tuple[str, Path]] = {}
    directories: dict[str, str] = {}
    pending = [root]
    while pending:
        directory = pending.pop()
        with os.scandir(directory) as scanned:
            for entry in scanned:
                path = Path(entry.path)
                metadata = entry.stat(follow_symlinks=False)
                if entry.is_symlink() or bool(
                    getattr(metadata, "st_file_attributes", 0)
                    & FILE_ATTRIBUTE_REPARSE_POINT
                ):
                    _fail(f"Emscripten cache reparse point is forbidden: {path}")
                relative = path.relative_to(root).as_posix()
                key = relative.casefold()
                if key in files or key in directories:
                    _fail(
                        "Emscripten cache duplicate/colliding path: "
                        f"{relative}"
                    )
                if stat.S_ISDIR(metadata.st_mode):
                    directories[key] = relative
                    pending.append(path)
                elif stat.S_ISREG(metadata.st_mode):
                    files[key] = (relative, path)
                else:
                    _fail(
                        "Emscripten cache special filesystem entry is "
                        f"forbidden: {path}"
                    )
    if not files:
        _fail("Emscripten cache file inventory is empty")
    return (
        sorted(files.values(), key=lambda item: item[0]),
        sorted(directories.values()),
    )


def generate_identity(cache_root: Path) -> dict[str, object]:
    lexical = Path(os.path.abspath(cache_root))
    _assert_root_chain(lexical)
    root = lexical.resolve(strict=True)
    if (
        not root.is_dir()
        or os.path.normcase(str(root)) != os.path.normcase(str(lexical))
    ):
        _fail(
            "Emscripten cache root is not a canonical regular directory: "
            f"{lexical}"
        )
    files, directories = _entries(root)
    inventory = hashlib.sha256()
    directory_inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative, path in files:
        content = path.read_bytes()
        total_bytes += len(content)
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(
            (
                f"{relative}\0{hashlib.sha256(content).hexdigest()}\n"
            ).encode("utf-8")
        )
    for relative in directories:
        directory_inventory.update(f"{relative}/\n".encode("utf-8"))
    return {
        "algorithm": ALGORITHM,
        "fileCount": len(files),
        "directoryCount": len(directories),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "directoryInventorySha256": directory_inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }


def generate_report(cache_root: Path) -> dict[str, object]:
    return {"payload": generate_identity(cache_root)}


def verify_identity(
    cache_root: Path,
    expected: object,
) -> dict[str, object]:
    identity = generate_identity(cache_root)
    if identity != expected:
        _fail(
            "Emscripten frozen cache identity drifted: "
            f"expected {json.dumps(expected, sort_keys=True)}, "
            f"got {json.dumps(identity, sort_keys=True)}"
        )
    return {"payload": identity}


def _parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache-root", type=Path, required=True)
    parser.add_argument("--expected-json")
    return parser.parse_args()


def main() -> int:
    arguments = _parse_arguments()
    if arguments.expected_json is None:
        report = generate_report(arguments.cache_root)
    else:
        try:
            expected = json.loads(arguments.expected_json)
        except json.JSONDecodeError as error:
            _fail(f"Invalid Emscripten cache expected JSON: {error}")
        report = verify_identity(arguments.cache_root, expected)
    print(json.dumps(report, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (CacheIdentityError, FileNotFoundError, OSError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
