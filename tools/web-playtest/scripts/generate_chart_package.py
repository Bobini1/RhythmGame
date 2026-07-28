"""Validate and stage one private BMS chart package deterministically."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import stat
import sys
import unicodedata
from dataclasses import dataclass
from pathlib import Path, PurePosixPath, PureWindowsPath
from typing import NoReturn


FILE_ATTRIBUTE_REPARSE_POINT = 0x400
ALLOWED_CHART_EXTENSIONS = {".bms", ".bme", ".bml", ".pms"}
WINDOWS_RESERVED_NAMES = {
    "aux",
    "con",
    "nul",
    "prn",
    *(f"com{index}" for index in range(1, 10)),
    *(f"lpt{index}" for index in range(1, 10)),
}
TEMPLATE_MARKERS = (
    "@SELECTED_VIRTUAL_PATH_JSON@",
    "@SELECTED_CHART_SHA256_JSON@",
    "@FILES_JSON@",
)


class PackageError(RuntimeError):
    pass


@dataclass(frozen=True)
class SourceFile:
    path: Path
    relative: str


@dataclass(frozen=True)
class PackagedFile:
    staged_path: Path
    relative: str
    sha256: str
    size: int


def _fail(message: str) -> NoReturn:
    raise PackageError(message)


def _is_reparse(path: Path, metadata: os.stat_result) -> bool:
    return path.is_symlink() or bool(
        (getattr(metadata, "st_file_attributes", 0) or 0)
        & FILE_ATTRIBUTE_REPARSE_POINT
    )


def _absolute_lexical(path: Path) -> Path:
    return Path(os.path.abspath(path))


def _assert_existing_path_chain(path: Path, description: str) -> Path:
    lexical = _absolute_lexical(path)
    candidates = [*reversed(lexical.parents), lexical]
    missing_seen = False
    for candidate in candidates:
        if not candidate.exists() and not candidate.is_symlink():
            missing_seen = True
            continue
        if missing_seen:
            _fail(f"{description} path changed during inspection: {candidate}")
        metadata = candidate.lstat()
        if _is_reparse(candidate, metadata):
            _fail(
                f"{description} reparse-point component is forbidden: "
                f"{candidate}"
            )
        if candidate != lexical and not stat.S_ISDIR(metadata.st_mode):
            _fail(f"{description} has a non-directory component: {candidate}")
    if not lexical.exists():
        _fail(f"{description} does not exist: {lexical}")
    return lexical


def _prepare_output_parent(path: Path, description: str) -> Path:
    lexical = _absolute_lexical(path)
    ancestor = lexical.parent
    while not ancestor.exists() and ancestor != ancestor.parent:
        ancestor = ancestor.parent
    _assert_existing_path_chain(ancestor, description)
    lexical.parent.mkdir(parents=True, exist_ok=True)
    _assert_existing_path_chain(lexical.parent, description)
    return lexical


def _prepare_output_file(path: Path, description: str) -> Path:
    lexical = _prepare_output_parent(path, description)
    if lexical.exists() or lexical.is_symlink():
        metadata = lexical.lstat()
        if _is_reparse(lexical, metadata):
            _fail(f"{description} is a reparse point: {lexical}")
        if not stat.S_ISREG(metadata.st_mode):
            _fail(f"{description} is not a regular file: {lexical}")
    return lexical


def _validate_component(component: str, description: str) -> str:
    normalized = unicodedata.normalize("NFC", component)
    if (
        normalized in {"", ".", ".."}
        or normalized.endswith((" ", "."))
        or any(ord(character) < 32 for character in normalized)
        or any(
            character in '<>:"|?*;\\/$'
            for character in normalized
        )
    ):
        _fail(f"{description} contains an unsafe component: {component!r}")
    if normalized.split(".", 1)[0].casefold() in WINDOWS_RESERVED_NAMES:
        _fail(f"{description} contains a reserved component: {component!r}")
    return normalized


def _normalize_relative_path(value: str, description: str) -> str:
    if not value:
        _fail(f"{description} is empty")
    windows = PureWindowsPath(value)
    posix = PurePosixPath(value)
    if windows.drive or windows.root or posix.is_absolute():
        _fail(f"{description} must be relative: {value!r}")
    canonical_separators = value.replace("\\", "/")
    parts = canonical_separators.split("/")
    if any(part in {"", ".", ".."} for part in parts):
        _fail(f"{description} contains traversal or empty parts: {value!r}")
    return "/".join(
        _validate_component(part, description) for part in parts
    )


def _is_within(candidate: Path, root: Path) -> bool:
    candidate_key = os.path.normcase(
        os.path.normpath(str(_absolute_lexical(candidate)))
    )
    root_key = os.path.normcase(
        os.path.normpath(str(_absolute_lexical(root)))
    )
    try:
        return os.path.commonpath((candidate_key, root_key)) == root_key
    except ValueError:
        return False


def _paths_overlap(first: Path, second: Path) -> bool:
    return _is_within(first, second) or _is_within(second, first)


def _validate_output_layout(
    chart_root: Path,
    template: Path,
    final_staging: Path,
    manifest_output: Path,
    cmake_output: Path,
) -> None:
    outputs = (
        ("staging directory", final_staging),
        ("manifest output", manifest_output),
        ("CMake output", cmake_output),
    )
    for label, output in outputs:
        if _paths_overlap(output, chart_root):
            _fail(
                f"Chart package {label} overlaps the private chart root"
            )
        if _paths_overlap(output, template):
            _fail(f"Chart package {label} overlaps the manifest template")
    for index, (first_label, first) in enumerate(outputs):
        for second_label, second in outputs[index + 1 :]:
            if _paths_overlap(first, second):
                _fail(
                    f"Chart package {first_label} overlaps {second_label}"
                )


def _scan_chart_root(root: Path) -> list[SourceFile]:
    seen: dict[str, tuple[str, str]] = {}
    files: list[SourceFile] = []

    def visit(
        directory: Path,
        relative_parts: tuple[str, ...],
        lexical_parts: tuple[str, ...],
    ) -> None:
        try:
            entries = list(os.scandir(directory))
        except OSError as error:
            _fail(f"Could not enumerate chart directory {directory}: {error}")
        normalized_entries: list[
            tuple[str, os.DirEntry[str], os.stat_result]
        ] = []
        for entry in entries:
            normalized_name = _validate_component(
                entry.name, "chart package path"
            )
            try:
                metadata = entry.stat(follow_symlinks=False)
            except OSError as error:
                _fail(f"Could not inspect chart entry {entry.path}: {error}")
            entry_path = Path(entry.path)
            if _is_reparse(entry_path, metadata):
                _fail(f"Chart reparse point is forbidden: {entry_path}")
            normalized_entries.append(
                (normalized_name, entry, metadata)
            )

        normalized_entries.sort(key=lambda item: item[0])
        for normalized_name, entry, metadata in normalized_entries:
            normalized_parts = (*relative_parts, normalized_name)
            relative = "/".join(normalized_parts)
            lexical_relative = "/".join((*lexical_parts, entry.name))
            collision_key = relative.casefold()
            previous = seen.get(collision_key)
            if previous is not None:
                _fail(
                    "Chart normalization/casefold collision: "
                    f"{previous[1]!r} and {lexical_relative!r} both map to "
                    f"{relative!r}"
                )
            seen[collision_key] = (relative, lexical_relative)
            entry_path = Path(entry.path)
            if stat.S_ISDIR(metadata.st_mode):
                visit(
                    entry_path,
                    normalized_parts,
                    (*lexical_parts, entry.name),
                )
            elif stat.S_ISREG(metadata.st_mode):
                files.append(SourceFile(entry_path, relative))
            else:
                _fail(f"Chart entry is not a regular file: {entry_path}")

    visit(root, (), ())
    files.sort(key=lambda item: item.relative)
    return files


def _copy_and_hash(source: SourceFile, destination: Path) -> PackagedFile:
    _assert_existing_path_chain(source.path, "chart source file")
    destination.parent.mkdir(parents=True, exist_ok=True)
    digest = hashlib.sha256()
    size = 0
    with source.path.open("rb") as input_file:
        before = os.fstat(input_file.fileno())
        if not stat.S_ISREG(before.st_mode):
            _fail(f"Chart source is not a regular file: {source.path}")
        with destination.open("xb") as output_file:
            while True:
                chunk = input_file.read(1024 * 1024)
                if not chunk:
                    break
                output_file.write(chunk)
                digest.update(chunk)
                size += len(chunk)
        after = os.fstat(input_file.fileno())
    stable_fields = ("st_dev", "st_ino", "st_size", "st_mtime_ns")
    if any(
        getattr(before, field, None) != getattr(after, field, None)
        for field in stable_fields
    ) or size != before.st_size:
        destination.unlink(missing_ok=True)
        _fail(f"Chart source changed while packaging: {source.path}")
    return PackagedFile(
        staged_path=destination,
        relative=source.relative,
        sha256=digest.hexdigest(),
        size=size,
    )


def _replace_staging_directory(
    temporary_staging: Path,
    final_staging: Path,
) -> None:
    if final_staging.exists() or final_staging.is_symlink():
        metadata = final_staging.lstat()
        if _is_reparse(final_staging, metadata):
            _fail(f"Staging directory is a reparse point: {final_staging}")
        if not stat.S_ISDIR(metadata.st_mode):
            _fail(f"Staging path is not a directory: {final_staging}")
        shutil.rmtree(final_staging)
    os.replace(temporary_staging, final_staging)


def _render_manifest(
    template: Path,
    selected_virtual_path: str,
    selected_sha256: str,
    files: list[PackagedFile],
) -> bytes:
    template_text = template.read_text(encoding="ascii")
    template_text = template_text.replace("\r\n", "\n").replace("\r", "\n")
    for marker in TEMPLATE_MARKERS:
        if template_text.count(marker) != 1:
            _fail(f"Manifest template marker contract drifted: {marker}")
    file_entries = [
        {
            "virtualPath": f"/playtest/chart/{item.relative}",
            "sha256": item.sha256,
            "size": item.size,
        }
        for item in files
    ]
    replacements = {
        "@SELECTED_VIRTUAL_PATH_JSON@": json.dumps(
            selected_virtual_path, ensure_ascii=True
        ),
        "@SELECTED_CHART_SHA256_JSON@": json.dumps(selected_sha256),
        "@FILES_JSON@": json.dumps(
            file_entries,
            ensure_ascii=True,
            separators=(",", ":"),
        ),
    }
    for marker, replacement in replacements.items():
        template_text = template_text.replace(marker, replacement)
    encoded = template_text.encode("ascii")
    parsed = json.loads(encoded)
    if parsed.get("schema") != 1 or parsed.get("files") != file_entries:
        _fail("Rendered manifest failed its own schema contract")
    return encoded


def _render_cmake(
    final_staging: Path,
    files: list[PackagedFile],
    manifest_sha256: str,
    selected_virtual_path: str,
) -> bytes:
    staged_paths = [
        (final_staging / Path(*item.relative.split("/")))
        .as_posix()
        .replace('"', '\\"')
        for item in files
    ]
    lines = [
        "set(RG_WEB_PLAYTEST_CHART_MANIFEST_SHA256",
        f'    "{manifest_sha256}")',
        "set(RG_WEB_PLAYTEST_SELECTED_VIRTUAL_PATH",
        f'    "{selected_virtual_path}")',
        "set(RG_WEB_PLAYTEST_STAGED_CHART_FILES",
        *(f'    "{path}"' for path in staged_paths),
        ")",
        "",
    ]
    return "\n".join(lines).encode("utf-8")


def generate(arguments: argparse.Namespace) -> dict[str, object]:
    root = _assert_existing_path_chain(
        Path(arguments.chart_root), "chart root"
    )
    root_metadata = root.stat()
    if not stat.S_ISDIR(root_metadata.st_mode):
        _fail(f"Chart root is not a directory: {root}")

    selected = _normalize_relative_path(
        arguments.selected_relative_path, "selected chart path"
    )
    if PurePosixPath(selected).suffix.casefold() not in ALLOWED_CHART_EXTENSIONS:
        _fail(f"Selected chart extension is unsupported: {selected!r}")

    final_staging = _absolute_lexical(Path(arguments.staging_dir))
    manifest_output = _absolute_lexical(Path(arguments.manifest_output))
    cmake_output = _absolute_lexical(Path(arguments.cmake_output))
    template = _assert_existing_path_chain(
        Path(arguments.manifest_template), "manifest template"
    )
    if not stat.S_ISREG(template.stat().st_mode):
        _fail(f"Manifest template is not a regular file: {template}")
    if final_staging.name != "chart-staging":
        _fail("Staging directory must use the fixed chart-staging leaf")
    if not (
        manifest_output.parent == final_staging.parent
        and cmake_output.parent == final_staging.parent
    ):
        _fail("Chart package outputs must share the staging parent")
    _validate_output_layout(
        root,
        template,
        final_staging,
        manifest_output,
        cmake_output,
    )
    manifest_output = _prepare_output_file(
        manifest_output, "manifest output"
    )
    cmake_output = _prepare_output_file(cmake_output, "CMake output")

    sources = _scan_chart_root(root)
    by_key = {source.relative.casefold(): source for source in sources}
    selected_source = by_key.get(selected.casefold())
    if selected_source is None:
        _fail(f"Selected chart is missing or not regular: {selected!r}")

    _prepare_output_parent(
        final_staging / "placeholder", "chart staging output"
    )
    temporary_staging = final_staging.with_name(
        f".{final_staging.name}.tmp-{os.getpid()}"
    )
    if temporary_staging.exists() or temporary_staging.is_symlink():
        _fail(f"Temporary staging path already exists: {temporary_staging}")
    temporary_staging.mkdir()
    packaged: list[PackagedFile] = []
    try:
        for source in sources:
            destination = temporary_staging / Path(
                *source.relative.split("/")
            )
            packaged.append(_copy_and_hash(source, destination))
        selected_packaged = next(
            item
            for item in packaged
            if item.relative.casefold() == selected.casefold()
        )
        selected_virtual_path = (
            f"/playtest/chart/{selected_packaged.relative}"
        )
        manifest_bytes = _render_manifest(
            template,
            selected_virtual_path,
            selected_packaged.sha256,
            packaged,
        )
        manifest_sha256 = hashlib.sha256(manifest_bytes).hexdigest()
        _replace_staging_directory(temporary_staging, final_staging)
        manifest_output.write_bytes(manifest_bytes)
        cmake_output.write_bytes(
            _render_cmake(
                final_staging,
                packaged,
                manifest_sha256,
                selected_virtual_path,
            )
        )
    finally:
        if temporary_staging.exists():
            shutil.rmtree(temporary_staging)

    return {
        "schema": 1,
        "fileCount": len(packaged),
        "manifestSha256": manifest_sha256,
        "selectedVirtualPath": selected_virtual_path,
    }


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--chart-root", required=True)
    parser.add_argument("--selected-relative-path", required=True)
    parser.add_argument("--staging-dir", required=True)
    parser.add_argument("--manifest-template", required=True)
    parser.add_argument("--manifest-output", required=True)
    parser.add_argument("--cmake-output", required=True)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    try:
        summary = generate(parse_args(argv))
    except (OSError, PackageError, UnicodeError, ValueError) as error:
        print(f"chart-package error: {error}", file=sys.stderr)
        return 2
    print(
        json.dumps(summary, ensure_ascii=True, separators=(",", ":")),
        flush=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
