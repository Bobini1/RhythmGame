"""Decode and audit Emscripten response files before invoking a driver."""

from __future__ import annotations

import argparse
import hashlib
import json
import locale
import os
import shlex
import stat
import sys
from pathlib import Path
from typing import NoReturn


MAX_RESPONSE_FILES = 64
MAX_RESPONSE_BYTES = 4 * 1024 * 1024
MAX_EFFECTIVE_ARGUMENTS = 1_000_000
FILE_ATTRIBUTE_REPARSE_POINT = 0x400
DANGEROUS_OPTIONS_WITH_VALUE = {
    "--closure-args",
    "--compiler-wrapper",
    "--embed-file",
    "--em-config",
    "--exclude-file",
    "--extern-post-js",
    "--extern-pre-js",
    "--js-library",
    "--js-transform",
    "--post-js",
    "--pre-js",
    "--preload-file",
    "--reproduce",
    "--shell-file",
    "--use-port",
    "--valid-abspath",
}
DANGEROUS_STANDALONE_OPTIONS = {
    "--cache",
    "--clear-cache",
    "--clear-ports",
    "--generate-config",
}
FORBIDDEN_TOOLCHAIN_OPTIONS_WITH_VALUE = {
    "-B",
    "--config",
    "--ld-path",
    "--plugin",
    "--script",
    "--sysroot",
    "-fpass-plugin",
    "-fplugin",
    "-fuse-ld",
    "-isysroot",
    "-mllvm",
}
FORBIDDEN_TOOLCHAIN_PREFIXES = (
    "--config=",
    "--config-system-dir=",
    "--config-user-dir=",
    "--ld-path=",
    "--plugin=",
    "--script=",
    "--sysroot=",
    "-fpass-plugin=",
    "-fplugin=",
    "-fuse-ld=",
    "-isysroot=",
)
FORBIDDEN_FILE_INPUT_OPTIONS_WITH_VALUE = {
    "-fmodule-file",
    "-fmodule-map-file",
    "-fmodules-cache-path",
    "-fprebuilt-module-path",
    "-gcc-toolchain",
    "-imacros",
    "-include",
    "-include-pch",
    "-include-pth",
    "-iprefix",
    "-ivfsoverlay",
    "-iwithprefix",
    "-iwithprefixbefore",
    "-resource-dir",
    "-specs",
    "-wrapper",
    "--gcc-toolchain",
    "--vfsoverlay",
}
FORBIDDEN_FILE_INPUT_PREFIXES = tuple(
    option + "=" for option in FORBIDDEN_FILE_INPUT_OPTIONS_WITH_VALUE
)
FORBIDDEN_PREPROCESSOR_FORWARDING_PREFIXES = (
    "-Wp,",
    "-Xpreprocessor",
)
FORBIDDEN_EXCEPTION_ARGUMENTS = {
    "-fexceptions",
    "-fignore-exceptions",
    "-fno-cxx-exceptions",
    "-fno-exceptions",
    "-fno-wasm-exceptions",
    "-mno-exception-handling",
}
FORBIDDEN_THREAD_ARGUMENTS = {
    "-mno-atomics",
    "-mno-bulk-memory",
    "-no-pthread",
}
EXPECTED_GATE_SETTINGS = {
    "ALLOW_BLOCKING_ON_MAIN_THREAD": "0",
    "AUDIO_WORKLET": "1",
    "DISABLE_EXCEPTION_CATCHING": "0",
    "DISABLE_EXCEPTION_THROWING": "0",
    "JSPI": "1",
    "PTHREADS": "1",
    "PTHREAD_POOL_SIZE": "4",
    "PTHREAD_POOL_SIZE_STRICT": "2",
    "SHARED_MEMORY": "1",
    "SUPPORT_LONGJMP": "wasm",
    "WASM_EXCEPTIONS": "1",
    "WASM_WORKERS": "1",
}
SETTING_ALIASES = {
    "USE_PTHREADS": "PTHREADS",
}


class AuditError(RuntimeError):
    pass


def _fail(message: str) -> NoReturn:
    raise AuditError(message)


def _is_reparse(path: Path) -> bool:
    metadata = path.lstat()
    return path.is_symlink() or bool(
        getattr(metadata, "st_file_attributes", 0)
        & FILE_ATTRIBUTE_REPARSE_POINT
    )


def _assert_chain_not_reparse(path: Path) -> None:
    chain = [path, *path.parents]
    for candidate in reversed(chain):
        if not candidate.exists() and not candidate.is_symlink():
            continue
        if _is_reparse(candidate):
            _fail(f"Response path reparse point is forbidden: {candidate}")


def _setting_assignment(value: str) -> tuple[str, str]:
    if not value:
        _fail("Emscripten -s setting is empty")
    if "=" in value:
        name, assigned = value.split("=", 1)
        if not name or assigned == "":
            _fail(f"Malformed Emscripten -s setting: {value}")
    else:
        name, assigned = value, "1"
    if assigned.startswith("@"):
        _fail(
            "External Emscripten setting-value files are forbidden: "
            f"{value}"
        )
    if not name.replace("_", "").isalnum() or name[0].isdigit():
        _fail(f"Malformed Emscripten -s setting name: {name}")
    if name.startswith("NO_"):
        positive = name[3:]
        if assigned not in {"0", "1"}:
            if positive in EXPECTED_GATE_SETTINGS:
                _fail(
                    "Non-boolean negative Gate setting is forbidden: "
                    f"{value}"
                )
        else:
            name = positive
            assigned = str(1 - int(assigned))
    name = SETTING_ALIASES.get(name, name)
    return name, assigned


def audit_effective_arguments(arguments: list[str]) -> None:
    assignments: dict[str, str] = {}
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument in FORBIDDEN_EXCEPTION_ARGUMENTS:
            _fail(f"Forbidden Emscripten Gate exception argument: {argument}")
        if argument in FORBIDDEN_THREAD_ARGUMENTS:
            _fail(f"Forbidden Emscripten Gate thread argument: {argument}")
        if argument.startswith("-Xclang=") and (
            argument.split("=", 1)[1] in FORBIDDEN_EXCEPTION_ARGUMENTS
        ):
            _fail(f"Forbidden Emscripten Gate cc1 argument: {argument}")
        if argument == "-Xclang" and index + 1 < len(arguments):
            cc1_argument = arguments[index + 1]
            if cc1_argument in FORBIDDEN_EXCEPTION_ARGUMENTS:
                _fail(
                    "Forbidden Emscripten Gate cc1 exception argument: "
                    f"{cc1_argument}"
                )
        if (
            argument.startswith("-B")
            and argument != "-B"
            or argument.startswith("-Xclang")
            or argument.startswith("-Xlinker")
            or argument.startswith("-Wl,")
            or any(
                argument.startswith(prefix)
                for prefix in FORBIDDEN_TOOLCHAIN_PREFIXES
            )
        ):
            _fail(
                "Forbidden compiler/linker toolchain override or plugin "
                f"option: {argument}"
            )
        if argument in FORBIDDEN_TOOLCHAIN_OPTIONS_WITH_VALUE:
            if index + 1 >= len(arguments):
                _fail(
                    "Forbidden compiler/linker toolchain option has no "
                    f"operand: {argument}"
                )
            _fail(
                "Forbidden compiler/linker toolchain override or plugin "
                f"option: {argument}"
            )
        if (
            argument in FORBIDDEN_FILE_INPUT_OPTIONS_WITH_VALUE
            or argument.startswith(FORBIDDEN_FILE_INPUT_PREFIXES)
            or argument.startswith(
                FORBIDDEN_PREPROCESSOR_FORWARDING_PREFIXES
            )
        ):
            if (
                argument in FORBIDDEN_FILE_INPUT_OPTIONS_WITH_VALUE
                and index + 1 >= len(arguments)
            ):
                _fail(
                    "Forbidden unmodeled compiler file-input option has no "
                    f"operand: {argument}"
                )
            _fail(
                "Forbidden unmodeled compiler file-input option: "
                f"{argument}"
            )
        if argument in DANGEROUS_STANDALONE_OPTIONS:
            _fail(f"Forbidden Emscripten state-changing option: {argument}")
        if any(
            argument.startswith(option + "=")
            for option in DANGEROUS_STANDALONE_OPTIONS
        ):
            _fail(f"Forbidden Emscripten state-changing option: {argument}")
        matched_option = next(
            (
                option
                for option in DANGEROUS_OPTIONS_WITH_VALUE
                if argument == option or argument.startswith(option + "=")
            ),
            None,
        )
        if matched_option is not None:
            if argument == matched_option and index + 1 >= len(arguments):
                _fail(
                    f"Forbidden Emscripten option has no operand: {argument}"
                )
            _fail(
                "Forbidden Emscripten external-input/execution option: "
                f"{argument}"
            )
        setting: str | None = None
        if argument == "-s":
            if index + 1 >= len(arguments):
                _fail("Emscripten -s option has no setting")
            setting = arguments[index + 1]
            index += 1
        elif argument.startswith("-s") and len(argument) > 2:
            setting = argument[2:]
        if setting is not None:
            name, assigned = _setting_assignment(setting)
            if name == "ASYNCIFY":
                _fail(f"Forbidden Emscripten Asyncify setting: {setting}")
            if name in EXPECTED_GATE_SETTINGS:
                expected = EXPECTED_GATE_SETTINGS[name]
                previous = assignments.get(name)
                if previous is not None and previous != assigned:
                    _fail(
                        "Forbidden Emscripten Gate setting conflict: "
                        f"{name}={previous} and {name}={assigned}"
                    )
                assignments[name] = assigned
                if assigned != expected:
                    _fail(
                        "Forbidden Emscripten Gate setting contradiction: "
                        f"{name}={assigned}, expected {expected}"
                    )
        index += 1


class ResponseAudit:
    def __init__(self, cwd: Path) -> None:
        self.cwd = cwd.absolute()
        self.response_files: list[dict[str, object]] = []
        self.total_bytes = 0

    def expand(self, arguments: list[str]) -> list[str]:
        effective: list[str] = []
        for argument in arguments:
            if argument.startswith("@"):
                if argument == "@":
                    _fail("Empty Emscripten response-file argument")
                effective.extend(
                    self._read_file(
                        argument[1:],
                    )
                )
            elif argument.startswith("-Wl,@"):
                if argument == "-Wl,@":
                    _fail("Empty Emscripten linker response-file argument")
                decoded = self._read_file(argument[5:])
                for linker_argument in decoded:
                    effective.append(
                        "-Wl," + linker_argument
                        if linker_argument.startswith("-")
                        else linker_argument
                    )
            else:
                effective.append(argument)
            if len(effective) > MAX_EFFECTIVE_ARGUMENTS:
                _fail("Emscripten response effective-argument limit exceeded")
        audit_effective_arguments(effective)
        return effective

    def _read_file(self, value: str) -> list[str]:
        lexical = Path(value)
        if not lexical.is_absolute():
            lexical = self.cwd / lexical
        lexical = Path(os.path.abspath(lexical))
        if not lexical.exists():
            _fail(f"Emscripten response file is missing: {lexical}")
        _assert_chain_not_reparse(lexical)
        metadata = lexical.stat()
        if not stat.S_ISREG(metadata.st_mode):
            _fail(f"Emscripten response path is not a regular file: {lexical}")
        if len(self.response_files) >= MAX_RESPONSE_FILES:
            _fail(
                "Emscripten response-file count exceeds "
                f"{MAX_RESPONSE_FILES}"
            )
        content = lexical.read_bytes()
        self.total_bytes += len(content)
        if self.total_bytes > MAX_RESPONSE_BYTES:
            _fail(
                "Emscripten response-file byte limit exceeds "
                f"{MAX_RESPONSE_BYTES}"
            )
        components = lexical.name.split(".")
        suffix = components[-1].lower()
        if len(components) > 1 and (
            suffix.startswith(("utf", "cp", "iso"))
            or suffix in {"ascii", "latin-1"}
        ):
            guessed_encoding = suffix
        else:
            guessed_encoding = "utf-8-sig"
        try:
            try:
                text = content.decode(guessed_encoding)
                used_encoding = guessed_encoding
            except (UnicodeDecodeError, LookupError):
                used_encoding = locale.getpreferredencoding(False)
                text = content.decode(used_encoding)
            decoded = shlex.split(text)
        except (UnicodeDecodeError, LookupError, ValueError) as error:
            _fail(f"Invalid Emscripten response file {lexical}: {error}")
        for argument in decoded:
            if argument.startswith("@") or argument.startswith("-Wl,@"):
                _fail(
                    "Nested response-file argument is unsupported and "
                    f"forbidden: {argument}"
                )
        self.response_files.append(
            {
                "path": str(lexical),
                "bytes": len(content),
                "sha256": hashlib.sha256(content).hexdigest(),
                "encoding": used_encoding,
            }
        )
        return decoded


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cwd", type=Path, required=True)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    parsed = parser.parse_args()
    if parsed.arguments and parsed.arguments[0] == "--":
        parsed.arguments = parsed.arguments[1:]
    return parsed


def main() -> int:
    parsed = _parse_args()
    cwd = parsed.cwd.absolute()
    if not cwd.is_dir():
        raise AuditError(f"Response working directory is missing: {cwd}")
    audit = ResponseAudit(cwd)
    effective = audit.expand(list(parsed.arguments))
    print(
        json.dumps(
            {
                "schemaVersion": 1,
                "parser": "emscripten-4.0.7-response-file-v1",
                "limits": {
                    "maxFiles": MAX_RESPONSE_FILES,
                    "maxBytes": MAX_RESPONSE_BYTES,
                    "maxEffectiveArguments": MAX_EFFECTIVE_ARGUMENTS,
                },
                "responseFileCount": len(audit.response_files),
                "responseBytes": audit.total_bytes,
                "responseFiles": audit.response_files,
                "effectiveArguments": effective,
            },
            sort_keys=True,
            separators=(",", ":"),
        )
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuditError as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
