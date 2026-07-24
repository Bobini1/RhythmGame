"""Fail-closed audit for the pinned Qt/Emscripten Gate 1A build."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import tempfile
import xml.etree.ElementTree as ElementTree
import zipfile
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


EXPECTED_EMSCRIPTEN = "4.0.7"
EXPECTED_EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
EXPECTED_EMSDK_PYTHON = (
    ".toolchains/emsdk-4.0.7/python/3.9.2-nuget_64bit/python.exe"
)
EXPECTED_EMSDK_NODE = (
    ".toolchains/emsdk-4.0.7/node/20.18.0_64bit/bin/node.exe"
)
EXPECTED_EMSCRIPTEN_LOCK_ENTRY = {
    "version": EXPECTED_EMSCRIPTEN,
    "emsdkCommit": EXPECTED_EMSDK_COMMIT,
    "releaseManifest": {
        "path": "emscripten-releases-tags.json",
        "sha256": (
            "d833b45d63b12690ce222a6a7b9fe8e7d05be53d5883162852f2483204c5a0f5"
        ),
    },
    "releaseHash": "ef4e9cedeac3332e4738087567552063f4f250d3",
    "packageUrl": (
        "https://storage.googleapis.com/webassembly/"
        "emscripten-releases-builds/win/"
        "ef4e9cedeac3332e4738087567552063f4f250d3/"
        "wasm-binaries.zip"
    ),
    "nodeExecutable": "node/20.18.0_64bit/bin/node.exe",
    "pythonExecutable": "python/3.9.2-nuget_64bit/python.exe",
    "generatedBytecode": {
        "cacheDirectory": "__pycache__",
        "fileSuffix": ".pyc",
        "normalization": (
            "authenticate-non-bytecode-delete-cache-authenticate-full-v1"
        ),
    },
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "roots": [
            ".emscripten",
            "upstream/bin",
            "upstream/lib",
            "upstream/emscripten",
            "node/20.18.0_64bit",
            "python/3.9.2-nuget_64bit",
        ],
        "excludedPrefixes": ["upstream/emscripten/cache/"],
        "excludedSegments": [],
        "excludedSuffixes": [],
        "fileCount": 14842,
        "inventorySha256": (
            "8c87639389a8b929b4d917e2cb41c09f40746f76ac4144143cf530a6c9345fbb"
        ),
        "aggregateSha256": (
            "d6a1ec1d8b7582a01e6f69fe951a16c620866eb18c9738420a22393756b89d99"
        ),
    },
}
EXPECTED_EMXX_VERSION_LINE = (
    "emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) "
    "4.0.7 (8dc91db45bf96c174531006839472a3924d105aa)"
)
EXPECTED_VCPKG_COMMIT = "a0400024711b283056538ac19ced80b91a83c24c"
EXPECTED_VCPKG_VERSION_LINE = (
    "vcpkg package management program version "
    "2026-05-27-d5b6777d666efc1a7f491babfcdab37794c1ae3e"
)
EXPECTED_OUTER_CMAKE = "4.2.3"
EXPECTED_OUTER_CMAKE_LOCK_ENTRY = {
    "version": EXPECTED_OUTER_CMAKE,
    "url": (
        "https://cmake.org/files/v4.2/"
        "cmake-4.2.3-windows-x86_64.zip"
    ),
    "sha256": (
        "eb4ebf5155dbb05436d675706b2a08189430df58904257ae5e91bcba4c86933c"
    ),
    "archiveFile": "cmake-4.2.3-windows-x86_64.zip",
    "executableSha256": (
        "daae341e73330c9c5bf391f22d10510c0f70e5f01d2b61b0fd256cd9edd28379"
    ),
    "directory": "cmake-4.2.3-windows-x86_64",
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "stripPrefix": "cmake-4.2.3-windows-x86_64",
        "fileCount": 8525,
        "directoryCount": 148,
        "totalBytes": 132316585,
        "inventorySha256": (
            "48f746fb6ca853fd693c018eaa3220da06b4251275a15a3e41a93bc1013c3dd3"
        ),
        "directoryInventorySha256": (
            "649f6d95061ef02fe899c0c7062b551e401e372a098c9a9880cbbee4d72fc814"
        ),
        "aggregateSha256": (
            "38d0389fbb638f78ec2a624592d331efeef3c3f951eb3a3aff1eacbff16146f7"
        ),
    },
}
EXPECTED_VCPKG_PORT_CMAKE = "4.3.3"
EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY = {
    "name": "cmake",
    "os": "windows",
    "arch": "amd64",
    "version": EXPECTED_VCPKG_PORT_CMAKE,
    "executable": (
        "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
    ),
    "url": (
        "https://github.com/Kitware/CMake/releases/download/v4.3.3/"
        "cmake-4.3.3-windows-x86_64.zip"
    ),
    "sha512": (
        "0df613db23b315d81895e672e8460f2b35f4c2dd2eb9f07e10b13389a675edcb"
        "09836a458b90d8c0211b3a2d6da38183714410769935a580b86a6177df691f6a"
    ),
    "archive": "cmake-4.3.3-windows-x86_64.zip",
}
EXPECTED_VCPKG_TOOLS_MANIFEST_SHA256 = (
    "7757067afc4839dd982eee8cfb68cb500c4255a64c37a7c150ee9244342595e6"
)
EXPECTED_VCPKG_PORT_CMAKE_EXECUTABLE_SHA256 = (
    "70fa92ce2ac9f54b0ae395b0b3790d9147ef2ebdbd7c4e0bb20852aac581baea"
)
EXPECTED_NINJA = "1.13.2"
EXPECTED_NINJA_LOCK_ENTRY = {
    "version": EXPECTED_NINJA,
    "url": (
        "https://github.com/ninja-build/ninja/releases/download/"
        "v1.13.2/ninja-win.zip"
    ),
    "sha256": (
        "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65"
    ),
    "archiveFile": "ninja-win.zip",
    "executableSha256": (
        "e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8"
    ),
    "directory": "ninja-1.13.2-win",
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "stripPrefix": "",
        "fileCount": 1,
        "directoryCount": 0,
        "totalBytes": 603648,
        "inventorySha256": (
            "014cb71e5fd86a18adb79f57e26acbd58577f6f84a52491297a3454a3b38f477"
        ),
        "directoryInventorySha256": (
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        ),
        "aggregateSha256": (
            "90779f6fe330259113900c836a8dbf973c87905e098e0ada0121b5ac135d6dac"
        ),
    },
}
EXPECTED_QT = "6.11.1"
EXPECTED_HOST_COMPILER_CONTRACT = {
    "basename": "cl.exe",
    "toolsetRelativePath": (
        "VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe"
    ),
    "cmakeCompilerId": "MSVC",
    "cmakeCompilerVersion": "19.51.36244.0",
    "frontendVariant": "MSVC",
    "architecture": "x64",
    "platform": "Windows",
    "executableSha256": (
        "9cf613fcbebece019712511eec4b1a32d0a9c94e65249a1aeb326305c2388d6b"
    ),
}

TARGET_TRIPLET = "wasm32-emscripten-rg"
HOST_TRIPLET = "x64-windows-rg-host-release"
TARGET_BUILD_SUFFIX = f"{TARGET_TRIPLET}-"

EXPECTED_QTBASE_TREE = "29a7f9f115d568b271a3b99fabeac886ec248f9f"
EXPECTED_QTDECLARATIVE_TREE = "846c872082b8bf0c50d13dc8ead681ae6fc6280a"
EXPECTED_INSTALLED_WASM_HELPER_SHA256 = (
    "eb6af811ddb6a4315ef9eb6a819e4e8c09c76bb5d202391b78a636480cdb34d1"
)
EXPECTED_OVERLAY_SHA256 = {
    "qtbase/portfile.cmake": (
        "dde572451242adc43c23ce22e6539c111cbb538a0e0e9a18b73a1cab3d49c7a9"
    ),
    "qtbase/vcpkg.json": (
        "2e347149af40d74e171fa5f2f8c21d48828bc855bc3e24cf75fa1e120aafc405"
    ),
    "qtbase/restore-wasm-version-check.patch": (
        "8aa3ed93e30f16c3c9691b35dee1f27c9608bcf424fc4b0e86009ce64709c786"
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
EXPECTED_TARGET_COMPILE_PORTS = (
    "brotli",
    "bzip2",
    "double-conversion",
    "freetype",
    "harfbuzz",
    "libjpeg-turbo",
    "libpng",
    "md4c",
    "pcre2",
    "qtbase",
    "qtdeclarative",
    "qtimageformats",
    "qtlanguageserver",
    "qtmultimedia",
    "qtshadertools",
    "qtsvg",
    "qtwebsockets",
    "zlib",
)
C_COMPILE_SETTINGS = ("-pthread", "-sSUPPORT_LONGJMP=wasm")
CXX_COMPILE_SETTINGS = (
    "-pthread",
    "-fwasm-exceptions",
    "-sSUPPORT_LONGJMP=wasm",
)
C_COMPILE_EMSCRIPTEN_SETTINGS = {"SUPPORT_LONGJMP": "wasm"}
CXX_COMPILE_EMSCRIPTEN_SETTINGS = {"SUPPORT_LONGJMP": "wasm"}
APPLICATION_EMSCRIPTEN_SETTINGS = {
    "SUPPORT_LONGJMP": "wasm",
    "JSPI": "1",
    "AUDIO_WORKLET": "1",
    "WASM_WORKERS": "1",
    "PTHREAD_POOL_SIZE": "4",
    "PTHREAD_POOL_SIZE_STRICT": "2",
    "ALLOW_BLOCKING_ON_MAIN_THREAD": "0",
}
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
FORBIDDEN_TARGET_ARGUMENTS = (
    "-fexceptions",
    "-fno-wasm-exceptions",
    "-sASYNCIFY",
    "-sNO_WASM_EXCEPTIONS",
    "-sWASM_EXCEPTIONS=0",
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
RECOGNIZED_WEB_DEPLOYABLE_SUFFIXES = (
    ".html",
    ".htm",
    ".js",
    ".mjs",
    ".wasm",
    ".data",
    ".mem",
    ".map",
    ".symbols",
    ".css",
    ".svg",
    ".png",
    ".jpg",
    ".jpeg",
    ".gif",
    ".ico",
    ".webmanifest",
    ".json",
)
NON_DEPLOYABLE_ROOT_FILES = {"compile_commands.json"}
SHADER_RESOURCE_PREFIX = "/qt/qml/RhythmGame/WasmProbe/shaders"
SHADER_RESOURCE_ALIAS = "pulse.frag.qsb"
SHADER_RESOURCE_PATH = f":{SHADER_RESOURCE_PREFIX}/{SHADER_RESOURCE_ALIAS}"
SHADER_RESOURCE_TREE = (
    ":",
    ":/qt",
    ":/qt/qml",
    ":/qt/qml/RhythmGame",
    ":/qt/qml/RhythmGame/WasmProbe",
    f":{SHADER_RESOURCE_PREFIX}",
    SHADER_RESOURCE_PATH,
)
GATE_SCOPE = (
    "Technical Qt/Emscripten build probe only; browser runtime "
    "capabilities remain Gate 1B work."
)
GATE_1B_LIMITATIONS = (
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
)
HOST_TOOL_VERSION_LINES = {
    "moc.exe": f"moc {EXPECTED_QT}",
    "qmlcachegen.exe": f"qmlcachegen {EXPECTED_QT}",
    "qmltyperegistrar.exe": f"qmltyperegistrar {EXPECTED_QT}",
    "qsb.exe": f"qsb {EXPECTED_QT}",
    "lrelease.exe": f"lrelease version {EXPECTED_QT}",
    "lupdate.exe": f"lupdate version {EXPECTED_QT}",
}
EXPECTED_TARGET_ENV_PASSTHROUGH = (
    "EMSCRIPTEN_ROOT",
    "EMSCRIPTEN_VERSION",
    "EMSDK",
    "EMSDK_PYTHON",
    "CMAKE_NINJA_FORCE_RESPONSE_FILE",
)
FORBIDDEN_BUILD_ENVIRONMENT_NAMES = (
    "AR",
    "BINARYEN_ROOT",
    "CC",
    "CL",
    "CPP",
    "CPPFLAGS",
    "CXX",
    "CFLAGS",
    "CXXFLAGS",
    "EMCC_CFLAGS",
    "EMCC_DEBUG",
    "EMMAKEN_CFLAGS",
    "EMMAKEN_COMPILER",
    "EM_COMPILER_WRAPPER",
    "EM_COMPILER_WRAPPER2",
    "LD",
    "LDFLAGS",
    "LLVM_ROOT",
    "NM",
    "NODE_JS",
    "PYTHONHOME",
    "PYTHONPATH",
    "RANLIB",
    "STRIP",
    "_CL_",
    "_EMCC_CCACHE",
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


def sha512(path: Path) -> str:
    digest = hashlib.sha512()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def verify_locked_executable(
    path: Path,
    expected_sha256: str,
    label: str,
) -> str:
    require(
        path.is_file() and not _is_reparse_or_symlink(path),
        f"{label} is missing or a reparse point: {path}",
    )
    require_sha256_value(expected_sha256, f"{label} expected SHA-256")
    actual = sha256(path)
    require(
        actual == expected_sha256,
        f"{label} SHA-256 drifted: expected {expected_sha256}, got {actual}",
    )
    return actual


def _is_reparse_or_symlink(path: Path) -> bool:
    attributes = getattr(path.lstat(), "st_file_attributes", 0)
    reparse_flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    return path.is_symlink() or bool(attributes & reparse_flag)


def verify_authenticated_build_tool(
    repo: Path,
    name: str,
    artifact: Mapping[str, Any],
    installation: Path,
) -> dict[str, Any]:
    require_exact_keys(
        artifact,
        {
            "version",
            "url",
            "sha256",
            "archiveFile",
            "executableSha256",
            "directory",
            "payload",
        },
        f"buildTools.{name}",
    )
    archive_name = _safe_contract_relative_path(
        artifact["archiveFile"],
        f"buildTools.{name}.archiveFile",
    )
    require("/" not in archive_name, f"{name} archive must be a leaf name")
    archive = repo / ".toolchains" / "downloads" / archive_name
    require(
        archive.is_file() and not _is_reparse_or_symlink(archive),
        f"{name} retained archive is missing or a reparse point: {archive}",
    )
    payload = require_mapping(
        artifact.get("payload"),
        f"buildTools.{name}.payload",
    )
    require_exact_keys(
        payload,
        {
            "algorithm",
            "stripPrefix",
            "fileCount",
            "directoryCount",
            "totalBytes",
            "inventorySha256",
            "directoryInventorySha256",
            "aggregateSha256",
        },
        f"buildTools.{name}.payload",
    )
    require(
        payload["algorithm"] == "sha256-path-null-digest-lf-v1",
        f"unsupported {name} payload digest algorithm",
    )
    prefix = payload["stripPrefix"]
    require(type(prefix) is str, f"{name} archive prefix must be a string")
    if prefix:
        prefix = _safe_contract_relative_path(
            prefix,
            f"buildTools.{name}.payload.stripPrefix",
        )

    # Authenticate and parse the same open file. The ZipFile constructor is
    # reached only after the complete archive SHA-256 has matched.
    with archive.open("rb") as archive_stream:
        archive_digest = hashlib.sha256()
        for chunk in iter(lambda: archive_stream.read(1024 * 1024), b""):
            archive_digest.update(chunk)
        archive_sha256 = archive_digest.hexdigest()
        require(
            archive_sha256 == artifact["sha256"],
            f"{name} retained archive SHA-256 drifted",
        )
        archive_stream.seek(0)
        with zipfile.ZipFile(archive_stream) as bundle:
            path_kinds: dict[str, tuple[str, str]] = {}
            explicit_directories: set[str] = set()
            directories: dict[str, str] = {}
            file_entries: list[tuple[str, zipfile.ZipInfo]] = []

            # Full entry preflight precedes every entry-content read.
            for info in bundle.infolist():
                raw = info.filename
                require(
                    "\\" not in raw,
                    f"{name} archive backslash path is forbidden: {raw}",
                )
                is_directory = info.is_dir()
                relative = raw.rstrip("/")
                if not relative:
                    require(
                        is_directory,
                        f"{name} archive contains an empty file path",
                    )
                    continue
                if prefix:
                    if relative == prefix:
                        require(
                            is_directory,
                            f"{name} archive prefix is a file",
                        )
                        continue
                    archive_prefix = f"{prefix}/"
                    require(
                        relative.startswith(archive_prefix),
                        f"unexpected {name} archive entry: {raw}",
                    )
                    relative = relative[len(archive_prefix):]
                relative = _safe_contract_relative_path(
                    relative,
                    f"{name} archive entry",
                )

                unix_type = (info.external_attr >> 16) & 0o170000
                expected_type = stat.S_IFDIR if is_directory else stat.S_IFREG
                require(
                    unix_type in (0, expected_type)
                    and not (
                        info.external_attr
                        & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
                    ),
                    f"{name} archive link/reparse or type mismatch: {raw}",
                )
                key = relative.casefold()
                if is_directory:
                    require(
                        info.file_size == 0,
                        f"{name} archive directory has content: {raw}",
                    )
                    require(
                        key not in explicit_directories,
                        f"duplicate {name} archive directory: {relative}",
                    )
                    explicit_directories.add(key)
                    existing = path_kinds.get(key)
                    require(
                        existing is None
                        or (
                            existing[1] == "directory"
                            and existing[0] == relative
                        ),
                        f"{name} archive path collision: {relative}",
                    )
                    if existing is None:
                        path_kinds[key] = (relative, "directory")
                        directories[key] = relative
                    continue

                require(
                    key not in path_kinds,
                    f"duplicate/colliding {name} archive target: {relative}",
                )
                parts = relative.split("/")
                for index in range(1, len(parts)):
                    parent = "/".join(parts[:index])
                    parent_key = parent.casefold()
                    existing = path_kinds.get(parent_key)
                    require(
                        existing is None
                        or (
                            existing[1] == "directory"
                            and existing[0] == parent
                        ),
                        f"{name} archive file/directory collision: {parent}",
                    )
                    if existing is None:
                        path_kinds[parent_key] = (parent, "directory")
                        directories[parent_key] = parent
                path_kinds[key] = (relative, "file")
                file_entries.append((relative, info))

            require(file_entries, f"{name} archive file inventory is empty")
            archive_files: dict[str, tuple[str, int]] = {}
            for relative, info in file_entries:
                digest = hashlib.sha256()
                with bundle.open(info) as entry_stream:
                    for chunk in iter(
                        lambda: entry_stream.read(1024 * 1024),
                        b"",
                    ):
                        digest.update(chunk)
                archive_files[relative] = (digest.hexdigest(), info.file_size)

    ordered_files = sorted(archive_files)
    ordered_directories = sorted(directories.values())
    inventory = hashlib.sha256()
    directory_inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative in ordered_files:
        digest, length = archive_files[relative]
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{digest}\n".encode("utf-8"))
        total_bytes += length
    for relative in ordered_directories:
        directory_inventory.update(f"{relative}/\n".encode("utf-8"))
    actual_payload = {
        "algorithm": payload["algorithm"],
        "stripPrefix": prefix,
        "fileCount": len(ordered_files),
        "directoryCount": len(ordered_directories),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "directoryInventorySha256": directory_inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }
    require(
        actual_payload == dict(payload),
        f"{name} authenticated archive payload drifted",
    )

    require(
        installation.is_dir()
        and not _is_reparse_or_symlink(installation),
        f"{name} installation is missing or a reparse point: {installation}",
    )
    installed_files: dict[str, Path] = {}
    installed_directories: dict[str, str] = {}
    for candidate in installation.rglob("*"):
        require(
            not _is_reparse_or_symlink(candidate),
            f"{name} installation reparse point is forbidden: {candidate}",
        )
        relative = candidate.relative_to(installation).as_posix()
        key = relative.casefold()
        if candidate.is_dir():
            require(
                key not in installed_directories,
                f"{name} installation directory collision: {relative}",
            )
            installed_directories[key] = relative
        else:
            require(
                candidate.is_file() and key not in installed_files,
                f"{name} installation file collision/type drift: {relative}",
            )
            installed_files[key] = candidate
    require(
        len(installed_files) == len(ordered_files)
        and len(installed_directories) == len(ordered_directories),
        f"{name} installed file/directory count drifted",
    )
    for relative in ordered_directories:
        require(
            installed_directories.get(relative.casefold()) == relative,
            f"{name} installed directory is missing/case-drifted: {relative}",
        )
    installed_bytes = 0
    for relative in ordered_files:
        candidate = installed_files.get(relative.casefold())
        require(
            candidate is not None
            and candidate.relative_to(installation).as_posix() == relative,
            f"{name} installed file is missing/case-drifted: {relative}",
        )
        expected_digest, expected_size = archive_files[relative]
        require(
            candidate.stat().st_size == expected_size,
            f"{name} installed file size drifted: {relative}",
        )
        require(
            sha256(candidate) == expected_digest,
            f"{name} installed file SHA-256 drifted: {relative}",
        )
        installed_bytes += expected_size
    require(
        installed_bytes == total_bytes,
        f"{name} installed byte count drifted",
    )
    return {
        "archive": {
            "path": relative_path(repo, archive),
            "sha256": archive_sha256,
        },
        "payload": actual_payload,
        "installationRoot": relative_path(repo, installation),
    }


def _safe_contract_relative_path(value: Any, label: str) -> str:
    require(type(value) is str and bool(value), f"{label} must be non-empty")
    normalized = value.replace("\\", "/")
    path = Path(normalized)
    parts = normalized.split("/")
    require(
        not path.is_absolute()
        and not re.match(r"^[A-Za-z]:[/\\]", normalized)
        and not normalized.startswith(("/", "\\"))
        and all(part not in ("", ".", "..") for part in parts),
        f"{label} must be a safe relative path: {value}",
    )
    reserved = re.compile(
        r"^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$",
        re.IGNORECASE,
    )
    for part in parts:
        require(
            not part.endswith((" ", "."))
            and not any(ord(character) < 32 for character in part)
            and not any(character in '<>:"|?*' for character in part)
            and reserved.fullmatch(part.split(".", 1)[0]) is None,
            f"{label} must be Windows-safe: {value}",
        )
    return normalized


def _emscripten_payload_files(
    emsdk: Path,
    payload: Mapping[str, Any],
) -> list[tuple[str, Path]]:
    roots = payload.get("roots")
    excluded_prefixes = payload.get("excludedPrefixes")
    excluded_segments = payload.get("excludedSegments")
    excluded_suffixes = payload.get("excludedSuffixes")
    require(
        isinstance(roots, list) and roots,
        "Emscripten payload roots must be a non-empty list",
    )
    for value, label in (
        (excluded_prefixes, "excludedPrefixes"),
        (excluded_segments, "excludedSegments"),
        (excluded_suffixes, "excludedSuffixes"),
    ):
        require(isinstance(value, list), f"Emscripten payload {label} invalid")
        require(
            all(type(item) is str and item for item in value),
            f"Emscripten payload {label} must contain strings",
        )
    prefixes = [
        _safe_contract_relative_path(prefix.rstrip("/"), "excluded prefix")
        + "/"
        for prefix in excluded_prefixes
    ]
    segments = set(excluded_segments)
    suffixes = tuple(excluded_suffixes)
    resolved_emsdk = emsdk.resolve()
    files: dict[str, Path] = {}

    for root_value in roots:
        root_relative = _safe_contract_relative_path(
            root_value,
            "Emscripten payload root",
        )
        root_candidate = emsdk / root_relative
        require(
            root_candidate.exists()
            and not _is_reparse_or_symlink(root_candidate),
            f"Emscripten payload root missing/reparse: {root_candidate}",
        )
        root = root_candidate.resolve()
        require(
            root.is_relative_to(resolved_emsdk),
            f"Emscripten payload root escapes emsdk: {root_relative}",
        )
        require(root.exists(), f"Emscripten payload root missing: {root}")
        candidates = [root] if root.is_file() else root.rglob("*")
        for candidate in candidates:
            require(
                not _is_reparse_or_symlink(candidate),
                f"Emscripten payload reparse point is forbidden: {candidate}",
            )
            if candidate.is_dir():
                continue
            require(
                candidate.is_file(),
                f"Emscripten payload non-file entry is forbidden: {candidate}",
            )
            resolved = candidate.resolve()
            require(
                resolved.is_relative_to(resolved_emsdk),
                f"Emscripten payload file escapes emsdk: {candidate}",
            )
            relative = resolved.relative_to(resolved_emsdk).as_posix()
            parts = relative.split("/")
            excluded = (
                any(relative.startswith(prefix) for prefix in prefixes)
                or any(segment in parts for segment in segments)
                or relative.endswith(suffixes)
            )
            if excluded:
                continue
            key = relative.casefold()
            require(
                key not in files,
                f"duplicate Emscripten payload path: {relative}",
            )
            files[key] = resolved

    ordered = sorted(
        ((path.relative_to(resolved_emsdk).as_posix(), path)
         for path in files.values()),
        key=lambda item: item[0],
    )
    require(ordered, "Emscripten payload inventory is empty")
    return ordered


def verify_emscripten_installation(
    emsdk: Path,
    version: str,
    contract: Mapping[str, Any],
) -> dict[str, Any]:
    release_manifest = require_mapping(
        contract.get("releaseManifest"),
        "emscripten.releaseManifest",
    )
    require_exact_keys(
        release_manifest,
        {"path", "sha256"},
        "emscripten.releaseManifest",
    )
    manifest_relative = _safe_contract_relative_path(
        release_manifest["path"],
        "Emscripten release manifest",
    )
    manifest = (emsdk / manifest_relative).resolve()
    require(
        manifest.is_relative_to(emsdk.resolve()) and manifest.is_file(),
        f"Emscripten release manifest missing or escapes emsdk: {manifest}",
    )
    manifest_sha256 = verify_locked_executable(
        manifest,
        release_manifest["sha256"],
        "Emscripten release manifest",
    )
    releases = json.loads(manifest.read_text("utf-8"))
    release_hash = contract.get("releaseHash")
    require(
        type(release_hash) is str
        and re.fullmatch(r"[0-9a-f]{40}", release_hash) is not None,
        "Emscripten release hash must be a lowercase Git object id",
    )
    require(
        releases.get("releases", {}).get(version) == release_hash,
        f"Emscripten {version} release mapping drifted",
    )
    expected_url = (
        "https://storage.googleapis.com/webassembly/"
        f"emscripten-releases-builds/win/{release_hash}/"
        "wasm-binaries.zip"
    )
    require(
        contract.get("packageUrl") == expected_url,
        "Emscripten release package URL drifted",
    )

    generated_bytecode = require_mapping(
        contract.get("generatedBytecode"),
        "emscripten.generatedBytecode",
    )
    require_exact_keys(
        generated_bytecode,
        {"cacheDirectory", "fileSuffix", "normalization"},
        "emscripten.generatedBytecode",
    )
    require(
        generated_bytecode
        == {
            "cacheDirectory": "__pycache__",
            "fileSuffix": ".pyc",
            "normalization": (
                "authenticate-non-bytecode-delete-cache-"
                "authenticate-full-v1"
            ),
        },
        "Emscripten generated-bytecode contract drifted",
    )

    payload = require_mapping(contract.get("payload"), "emscripten.payload")
    require_exact_keys(
        payload,
        {
            "algorithm",
            "roots",
            "excludedPrefixes",
            "excludedSegments",
            "excludedSuffixes",
            "fileCount",
            "inventorySha256",
            "aggregateSha256",
        },
        "emscripten.payload",
    )
    require(
        payload["algorithm"] == "sha256-path-null-digest-lf-v1",
        "unsupported Emscripten payload digest algorithm",
    )
    files = _emscripten_payload_files(emsdk, payload)
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    for relative, path in files:
        digest = sha256(path)
        inventory.update(relative.encode("utf-8") + b"\n")
        aggregate.update(relative.encode("utf-8") + b"\0")
        aggregate.update(digest.encode("ascii") + b"\n")
    actual_inventory = inventory.hexdigest()
    actual_aggregate = aggregate.hexdigest()
    require(
        len(files) == payload["fileCount"],
        "Emscripten payload file count drifted: "
        f"expected {payload['fileCount']}, got {len(files)}",
    )
    require(
        actual_inventory == payload["inventorySha256"],
        "Emscripten payload inventory SHA-256 drifted",
    )
    require(
        actual_aggregate == payload["aggregateSha256"],
        "Emscripten payload aggregate SHA-256 drifted",
    )
    return {
        "releaseManifest": {
            "path": manifest_relative,
            "sha256": manifest_sha256,
        },
        "releaseHash": release_hash,
        "packageUrl": expected_url,
        "generatedBytecode": dict(generated_bytecode),
        "payload": {
            "algorithm": payload["algorithm"],
            "roots": list(payload["roots"]),
            "excludedPrefixes": list(payload["excludedPrefixes"]),
            "excludedSegments": list(payload["excludedSegments"]),
            "excludedSuffixes": list(payload["excludedSuffixes"]),
            "fileCount": len(files),
            "inventorySha256": actual_inventory,
            "aggregateSha256": actual_aggregate,
        },
    }


def verify_host_compiler_identity(
    compiler: Mapping[str, Any],
    contract: Mapping[str, Any],
) -> dict[str, Any]:
    path = Path(str(compiler.get("path", ""))).resolve()
    require(path.is_file(), f"host compiler is missing: {path}")
    relative = _safe_contract_relative_path(
        contract.get("toolsetRelativePath"),
        "host compiler toolset-relative path",
    )
    expected_parts = tuple(part.casefold() for part in relative.split("/"))
    actual_parts = tuple(part.casefold() for part in path.parts)
    require(
        len(actual_parts) >= len(expected_parts)
        and actual_parts[-len(expected_parts):] == expected_parts,
        "host compiler toolset-relative path drifted",
    )
    expected_identity = {
        "id": contract.get("cmakeCompilerId"),
        "version": contract.get("cmakeCompilerVersion"),
        "frontendVariant": contract.get("frontendVariant"),
        "architecture": contract.get("architecture"),
        "platform": contract.get("platform"),
    }
    require(
        {name: compiler.get(name) for name in expected_identity}
        == expected_identity,
        "host compiler CMake identity drifted",
    )
    require(
        path.name.casefold()
        == str(contract.get("basename", "")).casefold()
        == "cl.exe",
        "host compiler basename drifted",
    )
    executable_sha256 = verify_locked_executable(
        path,
        contract.get("executableSha256"),
        "host compiler",
    )
    return {
        "basename": "cl.exe",
        "toolsetRelativePath": relative,
        "pathAuthenticated": True,
        "executableSha256": executable_sha256,
        **expected_identity,
    }


def require_no_absolute_path_strings(
    value: Any,
    label: str = "evidence",
) -> None:
    if isinstance(value, Mapping):
        for name, child in value.items():
            require_no_absolute_path_strings(child, f"{label}.{name}")
        return
    if isinstance(value, list):
        for index, child in enumerate(value):
            require_no_absolute_path_strings(child, f"{label}[{index}]")
        return
    if type(value) is not str:
        return
    windows_absolute = (
        re.match(r"^[A-Za-z]:[/\\]", value) is not None
        or value.startswith(("\\\\", "//"))
    )
    require(
        not windows_absolute,
        f"{label} contains an absolute path: {value}",
    )


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


def verify_pinned_repository(
    repository: Path,
    expected_commit: str,
    label: str,
) -> str:
    head = git(repository, "rev-parse", "HEAD").strip()
    require(head == expected_commit, f"{label} HEAD {head}")
    checks = (
        (
            "tracked worktree",
            [
                "diff",
                "--quiet",
                "--no-ext-diff",
                "--no-textconv",
                "--ignore-submodules=all",
                "--",
            ],
        ),
        (
            "index",
            [
                "diff",
                "--cached",
                "--quiet",
                "--no-ext-diff",
                "--no-textconv",
                "--ignore-submodules=all",
                "HEAD",
                "--",
            ],
        ),
    )
    for description, arguments in checks:
        process = subprocess.run(
            ["git", "-C", str(repository), *arguments],
            capture_output=True,
            text=True,
            check=False,
        )
        require(
            process.returncode == 0,
            f"{label} {description} is dirty or unreadable "
            f"(git exit {process.returncode})",
        )
    return head


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


def target_compile_databases(
    buildtrees: Path,
    *,
    expected_ports: Sequence[str] = EXPECTED_TARGET_COMPILE_PORTS,
) -> list[Path]:
    expected = [
        buildtrees
        / port
        / f"{TARGET_TRIPLET}-rel"
        / "compile_commands.json"
        for port in expected_ports
    ]
    discovered = [
        database
        for database in buildtrees.rglob("compile_commands.json")
        if any(
            part.startswith(TARGET_BUILD_SUFFIX)
            for part in database.relative_to(buildtrees).parts
        )
    ]
    expected_keys = {path_key(path) for path in expected}
    discovered_keys = {path_key(path) for path in discovered}
    require(
        discovered_keys == expected_keys
        and len(discovered) == len(expected),
        "target compile database layout must be exactly "
        ".wb/<expected-port>/wasm32-emscripten-rg-rel/"
        "compile_commands.json; "
        f"expected {sorted(path.as_posix() for path in expected)}, "
        f"found {sorted(path.as_posix() for path in discovered)}",
    )
    for database in expected:
        require(
            database.is_file(),
            f"missing target compile database: {database}",
        )
    return expected


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


def select_vcpkg_port_cmake_manifest_entry(
    manifest: Mapping[str, Any],
) -> dict[str, str]:
    require(
        type(manifest.get("schema-version")) is int
        and manifest["schema-version"] == 1,
        "vcpkg tools manifest schema is not exactly 1",
    )
    tools = manifest.get("tools")
    require(
        isinstance(tools, list)
        and all(isinstance(entry, Mapping) for entry in tools),
        "vcpkg tools manifest tools must be a list of mappings",
    )
    matching = [
        entry
        for entry in tools
        if entry.get("name") == "cmake"
        and entry.get("os") == "windows"
        and entry.get("arch") == "amd64"
    ]
    require(
        len(matching) == 1,
        "vcpkg tools manifest must contain exactly one Windows amd64 "
        "CMake entry",
    )
    entry = dict(matching[0])
    require(
        entry == EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY,
        "vcpkg Windows amd64 CMake manifest entry drifted",
    )
    return entry


def require_archive_member_matches_file(
    archive: Path,
    member: str,
    extracted: Path,
) -> str:
    require(archive.is_file(), f"missing authenticated archive: {archive}")
    require(extracted.is_file(), f"missing extracted archive file: {extracted}")
    with zipfile.ZipFile(archive) as package:
        matches = [
            info
            for info in package.infolist()
            if info.filename == member and not info.is_dir()
        ]
        require(
            len(matches) == 1,
            f"expected one archive member {member!r}, found {len(matches)}",
        )
        member_digest = hashlib.sha256()
        with package.open(matches[0]) as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                member_digest.update(chunk)
    extracted_digest = sha256(extracted)
    require(
        extracted_digest == member_digest.hexdigest(),
        f"extracted file does not match authenticated archive member {member}",
    )
    return extracted_digest


def require_cache_cmake_command(
    cache: Mapping[str, str],
    expected: Path,
    label: str,
) -> Path:
    command = cache.get("CMAKE_COMMAND", "")
    require_same_path(command, expected, f"{label} CMAKE_COMMAND")
    return Path(command).resolve()


def require_exact_cache_values(
    cache: Mapping[str, str],
    expected: Mapping[str, str],
    label: str,
) -> None:
    for name, expected_value in expected.items():
        require(
            cache.get(name) == expected_value,
            f"{label} {name} is {cache.get(name)!r}, "
            f"expected {expected_value!r}",
        )


def require_port_version(
    record: Mapping[str, str],
    expected: str,
    package: str,
) -> None:
    require(
        record.get("Port-Version") == expected,
        f"{package} Port-Version is {record.get('Port-Version')!r}, "
        f"expected {expected!r}",
    )


def require_vcpkg_env_passthrough(
    triplet: str,
    required_names: Sequence[str],
) -> list[str]:
    matches = re.findall(
        r"set\(VCPKG_ENV_PASSTHROUGH(?P<body>.*?)\)",
        triplet,
        re.DOTALL,
    )
    require(
        len(matches) == 1,
        "target triplet must define VCPKG_ENV_PASSTHROUGH exactly once",
    )
    names = matches[0].split()
    for name in required_names:
        require(
            names.count(name) == 1,
            f"target triplet VCPKG_ENV_PASSTHROUGH must contain "
            f"{name} exactly once",
        )
    return names


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


def parse_emscripten_settings(
    arguments: Sequence[str],
) -> dict[str, list[dict[str, Any]]]:
    """Parse compact and split Emscripten -s settings without substrings."""
    parsed: dict[str, list[dict[str, Any]]] = {}
    index = 0
    while index < len(arguments):
        token = arguments[index]
        if token == "-s":
            require(
                index + 1 < len(arguments),
                "Emscripten -s option has no setting",
            )
            payload = arguments[index + 1]
            index += 2
        elif token.startswith("-s") and len(token) > 2:
            payload = token[2:]
            index += 1
        else:
            index += 1
            continue

        match = re.fullmatch(
            r"(?P<name>[A-Z][A-Z0-9_]*)(?:=(?P<value>.*))?",
            payload,
        )
        if match is None:
            continue
        raw_name = match.group("name")
        negative = raw_name.startswith("NO_") and len(raw_name) > 3
        name = raw_name[3:] if negative else raw_name
        value = "0" if negative else (match.group("value") or "1")
        parsed.setdefault(name, []).append(
            {
                "value": value,
                "negative": negative,
                "argument": f"-s{payload}",
            }
        )
    return parsed


def require_effective_emscripten_settings(
    arguments: Sequence[str],
    expected: Mapping[str, str],
    context: str,
) -> dict[str, Any]:
    parsed = parse_emscripten_settings(arguments)
    require(
        "ASYNCIFY" not in parsed,
        f"{context}: Asyncify must not be configured",
    )
    effective: dict[str, str] = {}
    occurrences: dict[str, int] = {}
    for name, expected_value in expected.items():
        values = parsed.get(name, [])
        require(values, f"{context}: missing -s{name}={expected_value}")
        require(
            not any(item["negative"] for item in values),
            f"{context}: negative -sNO_{name} override is forbidden",
        )
        observed = {str(item["value"]) for item in values}
        require(
            observed == {expected_value},
            f"{context}: conflicting/effective {name} values "
            f"{sorted(observed)}, expected {expected_value}",
        )
        effective[name] = expected_value
        occurrences[name] = len(values)
    return {
        "effectiveValues": effective,
        "occurrences": occurrences,
    }


def require_wasm_compile_contract(
    arguments: Sequence[str],
    *,
    language: str,
    context: str,
) -> dict[str, Any]:
    require(language in {"c", "cxx"}, f"unsupported language: {language}")
    require("-pthread" in arguments, f"{context}: missing -pthread")
    parsed_settings = parse_emscripten_settings(arguments)
    wasm_exception_settings = parsed_settings.get("WASM_EXCEPTIONS", [])
    require(
        not any(item["value"] == "0" for item in wasm_exception_settings),
        f"{context}: negative Wasm exception setting is forbidden",
    )
    if language == "cxx":
        require(
            "-fwasm-exceptions" in arguments,
            f"{context}: missing -fwasm-exceptions",
        )
        require(
            "-fexceptions" not in arguments,
            f"{context}: forbidden -fexceptions",
        )
        require(
            "-fno-wasm-exceptions" not in arguments,
            f"{context}: forbidden -fno-wasm-exceptions",
        )
        expected = CXX_COMPILE_EMSCRIPTEN_SETTINGS
    else:
        expected = C_COMPILE_EMSCRIPTEN_SETTINGS
    return require_effective_emscripten_settings(
        arguments,
        expected,
        context,
    )


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


def verify_ninja_noop(ninja: Path, build: Path) -> list[str]:
    output = run_text(
        ninja,
        "-C",
        build,
        "-n",
        "-v",
        "RhythmGameWasmProbe",
    )
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    require(
        len(lines) == 2
        and lines[0].startswith("ninja: Entering directory ")
        and lines[1] == "ninja: no work to do.",
        "RhythmGameWasmProbe target closure is not a no-op; "
        f"dry-run output was {lines}",
    )
    return lines


def probe_input_identity(repo: Path) -> dict[str, Any]:
    manifest = repo / "tools" / "wasm-probe" / "input-manifest.txt"
    require(manifest.is_file(), f"missing probe input manifest: {manifest}")
    raw = manifest.read_bytes()
    require(raw.endswith(b"\n"), "probe input manifest must end with LF")
    text = raw.decode("utf-8")
    require("\r" not in text, "probe input manifest must use LF")
    relative_paths = text.splitlines()
    require(
        relative_paths
        and all(path and path == path.strip() for path in relative_paths),
        "probe input manifest contains blank or padded paths",
    )
    require(
        relative_paths == sorted(set(relative_paths), key=str.casefold),
        "probe input manifest must be unique and case-insensitively sorted",
    )
    inputs: list[Path] = []
    repo_resolved = repo.resolve()
    for relative in relative_paths:
        normalized = _safe_contract_relative_path(
            relative,
            "probe input manifest entry",
        )
        require(
            normalized == relative,
            f"probe input path is not canonical: {relative}",
        )
        path = (repo / relative).resolve()
        require(
            path.is_relative_to(repo_resolved) and path.is_file(),
            f"missing or escaping probe input: {relative}",
        )
        inputs.append(path)

    tracked_relatives = [
        "tools/wasm-probe/input-manifest.txt",
        *relative_paths,
    ]
    tracked = subprocess.run(
        [
            "git",
            "-C",
            str(repo),
            "ls-files",
            "--stage",
            "-z",
            "--",
            *tracked_relatives,
        ],
        capture_output=True,
        check=False,
    )
    require(
        tracked.returncode == 0,
        "unable to verify tracked probe inputs",
    )
    tracked_paths = {
        record.split(b"\t", 1)[1].decode("utf-8")
        for record in tracked.stdout.split(b"\0")
        if record
    }
    require(
        tracked_paths == set(tracked_relatives),
        "probe input manifest and every listed input must be tracked: "
        f"missing={sorted(set(tracked_relatives) - tracked_paths)}",
    )

    manifest_sha256 = hashlib.sha256(raw).hexdigest()
    payload = f"manifest={manifest_sha256}\n"
    for relative, path in zip(relative_paths, inputs, strict=True):
        payload += f"{relative}={sha256(path)}\n"
    aggregate = sha256_text(payload)
    return {
        "algorithm": "sha256-manifest-path-equals-digest-lf-v1",
        "tracking": "git-index",
        "manifest": relative_path(repo, manifest),
        "manifestSha256": manifest_sha256,
        "count": len(inputs),
        "paths": relative_paths,
        "aggregateSha256": aggregate,
        "_files": inputs,
    }


def probe_source_inputs(repo: Path) -> list[Path]:
    return list(probe_input_identity(repo)["_files"])


def verify_probe_input_binding(
    repo: Path,
    build: Path,
    inputs: Mapping[str, Any] | None = None,
) -> dict[str, Any]:
    identity = dict(inputs or probe_input_identity(repo))
    identity.pop("_files", None)
    digest = identity["aggregateSha256"]
    marker = f"RG_WASM_PROBE_INPUT_SHA256={digest}"
    generated = build / "generated" / "ProbeInputDigest.cpp"
    artifact = build / "RhythmGameWasmProbe.wasm"
    require(
        generated.is_file(),
        f"generated probe input digest source missing: {generated}",
    )
    require(
        generated.read_text("utf-8").count(marker) == 1,
        "configured probe input digest does not match current tracked inputs",
    )
    require(artifact.is_file(), f"bound Wasm artifact missing: {artifact}")
    require(
        marker.encode("ascii") in artifact.read_bytes(),
        "Wasm artifact is not cryptographically bound to current inputs",
    )
    return {
        "generatedSource": relative_path(repo, generated),
        "artifact": relative_path(repo, artifact),
        "configuredAggregateSha256": digest,
        "markerSha256": sha256_text(marker),
    }


def verify_build_freshness(
    repo: Path,
    build: Path,
    ninja: Path,
) -> dict[str, Any]:
    lines = verify_ninja_noop(ninja, build)
    canonical_lines = [
        canonical_command(repo, line)
        for line in lines
    ]
    inputs = probe_input_identity(repo)
    files = inputs.pop("_files")
    require(
        len(files) == inputs["count"],
        "probe input identity file count drifted",
    )
    binding = verify_probe_input_binding(repo, build, inputs)
    command = [
        relative_path(repo, ninja),
        "-C",
        relative_path(repo, build),
        "-n",
        "-v",
        "RhythmGameWasmProbe",
    ]
    return {
        "target": "RhythmGameWasmProbe",
        "command": command,
        "output": canonical_lines,
        "outputSha256": sha256_text("\n".join(canonical_lines) + "\n"),
        "sourceInputs": {
            **inputs,
        },
        "artifactBinding": binding,
    }


def parse_application_link_arguments(
    command: str,
    expected_emxx: Path,
) -> list[str]:
    outer = split_windows_command_line(command)
    require(
        len(outer) == 3
        and Path(outer[0]).name.casefold() == "cmd.exe"
        and outer[1].casefold() == "/c",
        f"unexpected application link wrapper: {outer[:2]}",
    )
    body = outer[2].strip()
    prefix = "cd . && "
    suffix = " && cd ."
    require(
        body.startswith(prefix) and body.endswith(suffix),
        f"unexpected application link shell body: {body}",
    )
    arguments = split_windows_command_line(
        body[len(prefix) : -len(suffix)].strip()
    )
    require(arguments, "application link parsed to no arguments")
    require_same_path(arguments[0], expected_emxx, "application link compiler")
    require(
        arguments.count("@CMakeFiles\\RhythmGameWasmProbe.rsp") == 1,
        "application link must use the exact generated response file",
    )
    return arguments


def ninja_logical_lines(value: str) -> list[str]:
    logical: list[str] = []
    pending = ""
    for raw_line in value.replace("\r\n", "\n").splitlines():
        line = pending + (raw_line.lstrip() if pending else raw_line)
        if line.endswith("$"):
            pending = line[:-1]
            continue
        logical.append(line)
        pending = ""
    require(not pending, "unterminated Ninja line continuation")
    return logical


def split_ninja_arguments(value: str) -> list[str]:
    arguments: list[str] = []
    current: list[str] = []
    index = 0
    while index < len(value):
        character = value[index]
        if character.isspace():
            if current:
                arguments.append("".join(current))
                current = []
            index += 1
            continue
        if character == "$" and index + 1 < len(value):
            escaped = value[index + 1]
            if escaped in {"$", " ", ":", "|"}:
                current.append(escaped)
                index += 2
                continue
        current.append(character)
        index += 1
    if current:
        arguments.append("".join(current))
    return arguments


def selected_application_link_edge(build_ninja: str) -> dict[str, Any]:
    lines = ninja_logical_lines(build_ninja)
    starts = [
        index
        for index, line in enumerate(lines)
        if line.startswith("build RhythmGameWasmProbe.js:")
    ]
    require(
        len(starts) == 1,
        "expected exactly one RhythmGameWasmProbe.js Ninja edge",
    )
    header = lines[starts[0]]
    tokens = split_ninja_arguments(header.partition(":")[2].strip())
    require(tokens, "selected application link edge has no rule")
    rule = tokens[0]
    explicit_inputs: list[str] = []
    implicit_inputs: list[str] = []
    order_only_inputs: list[str] = []
    destination = explicit_inputs
    for token in tokens[1:]:
        if token == "|":
            destination = implicit_inputs
        elif token == "||":
            destination = order_only_inputs
        else:
            destination.append(token)

    bindings: dict[str, str] = {}
    for line in lines[starts[0] + 1 :]:
        if not line.startswith("  "):
            break
        key, separator, value = line.strip().partition(" = ")
        if separator:
            require(key not in bindings, f"duplicate Ninja edge binding: {key}")
            bindings[key] = value
    require(
        bindings.get("RSP_FILE") == "CMakeFiles\\RhythmGameWasmProbe.rsp",
        "selected application link edge has an unexpected response file",
    )
    require(
        "LINK_LIBRARIES" in bindings,
        "selected application link edge has no LINK_LIBRARIES",
    )
    return {
        "rule": rule,
        "explicitInputs": explicit_inputs,
        "implicitInputs": implicit_inputs,
        "orderOnlyInputs": order_only_inputs,
        "bindings": bindings,
    }


def selected_ninja_rule(
    rules_ninja: str,
    rule_name: str,
) -> dict[str, str]:
    lines = ninja_logical_lines(rules_ninja)
    starts = [
        index
        for index, line in enumerate(lines)
        if line == f"rule {rule_name}"
    ]
    require(len(starts) == 1, f"expected exactly one Ninja rule {rule_name}")
    bindings: dict[str, str] = {}
    for line in lines[starts[0] + 1 :]:
        if not line.startswith("  "):
            break
        key, separator, value = line.strip().partition(" = ")
        if separator:
            require(
                key not in bindings,
                f"duplicate Ninja rule binding: {rule_name}.{key}",
            )
            bindings[key] = value
    return bindings


def application_response_arguments(
    build_ninja: str,
    rules_ninja: str,
) -> tuple[list[str], dict[str, Any], dict[str, str]]:
    edge = selected_application_link_edge(build_ninja)
    bindings = edge["bindings"]
    rule = selected_ninja_rule(rules_ninja, edge["rule"])
    require(
        rule.get("rspfile") == "$RSP_FILE",
        "selected application link rule does not use edge RSP_FILE",
    )
    template = split_ninja_arguments(rule.get("rspfile_content", ""))
    require(
        template == ["$in", "$LINK_PATH", "$LINK_LIBRARIES"],
        "selected application response content must be exactly "
        "$in $LINK_PATH $LINK_LIBRARIES",
    )
    values = {
        # Ninja's $in expansion contains explicit inputs; CMake places link
        # dependencies after "|" and supplies their actual arguments through
        # LINK_LIBRARIES to avoid duplicating them in the response file.
        "$in": edge["explicitInputs"],
        "$LINK_PATH": split_ninja_arguments(bindings.get("LINK_PATH", "")),
        "$LINK_LIBRARIES": split_ninja_arguments(
            bindings["LINK_LIBRARIES"]
        ),
    }
    response: list[str] = []
    for variable in template:
        response.extend(values[variable])
    require(response, "selected application response file is empty")
    return response, edge, rule


def require_final_link_archive(
    build_ninja: str,
    rules_ninja: str,
) -> str:
    response, _, _ = application_response_arguments(
        build_ninja,
        rules_ninja,
    )
    archive = "libWasmProbeExceptionBoundary.a"
    require(
        response.count(archive) == 1,
        f"{archive} is not an actual response argument on the selected "
        "application link edge",
    )
    return archive


def verify_application_link_argument_stream(
    command: str,
    expected_emxx: Path,
    build_ninja: str,
    rules_ninja: str,
) -> dict[str, Any]:
    outer = parse_application_link_arguments(command, expected_emxx)
    response, edge, rule = application_response_arguments(
        build_ninja,
        rules_ninja,
    )
    response_token = f"@{edge['bindings']['RSP_FILE']}"
    response_index = outer.index(response_token)
    effective = (
        outer[:response_index]
        + response
        + outer[response_index + 1 :]
    )
    compile_contract = require_wasm_compile_contract(
        effective,
        language="cxx",
        context="application link effective argument stream",
    )
    setting_contract = require_effective_emscripten_settings(
        effective,
        APPLICATION_EMSCRIPTEN_SETTINGS,
        "application link effective argument stream",
    )
    archive = "libWasmProbeExceptionBoundary.a"
    require(
        response.count(archive) == 1,
        f"{archive} is not an actual response argument on the selected "
        "application link edge",
    )
    return {
        "outerArguments": outer,
        "responseArguments": response,
        "effectiveArguments": effective,
        "compileContract": compile_contract,
        "settingContract": setting_contract,
        "archive": archive,
        "edge": edge,
        "rule": rule,
    }


def validate_autogen_predefs_paths(
    declarative_target: Path,
    paths: Sequence[str],
) -> list[Path]:
    target = declarative_target.resolve()
    resolved: list[Path] = []
    seen: set[str] = set()
    for value in paths:
        generated = Path(value).resolve()
        require(
            generated.name == "moc_predefs.h",
            f"AutoGen predefines output is not moc_predefs.h: {generated}",
        )
        require(
            generated.is_relative_to(target),
            f"AutoGen predefines output escapes target build: {generated}",
        )
        require(
            generated.is_file() and generated.stat().st_size > 0,
            f"compiler predefines were not generated: {generated}",
        )
        key = path_key(generated)
        require(
            key not in seen,
            f"AutoGen predefines output referenced twice: {generated}",
        )
        seen.add(key)
        resolved.append(generated)
    return resolved


def require_host_tool_version(name: str, output: str) -> str:
    reported = first_line(output)
    expected = HOST_TOOL_VERSION_LINES[name]
    require(
        reported == expected,
        f"{name} version output {reported!r}, expected {expected!r}",
    )
    return reported


def verify_vcpkg_port_build_cmake(
    repo: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    manifest_path = vcpkg / "scripts" / "vcpkg-tools.json"
    require(
        manifest_path.is_file(),
        f"missing pinned vcpkg tools manifest: {manifest_path}",
    )
    manifest_sha256 = sha256(manifest_path)
    require(
        manifest_sha256 == EXPECTED_VCPKG_TOOLS_MANIFEST_SHA256,
        "pinned vcpkg tools manifest SHA-256 drifted",
    )
    manifest = json.loads(manifest_path.read_text("utf-8"))
    require(
        isinstance(manifest, Mapping),
        "pinned vcpkg tools manifest root must be a mapping",
    )
    entry = select_vcpkg_port_cmake_manifest_entry(manifest)

    downloads = repo / ".wasm-vcpkg" / "downloads"
    tool_root = (
        downloads
        / "tools"
        / f"cmake-{EXPECTED_VCPKG_PORT_CMAKE}-windows"
    ).resolve()
    executable = (tool_root / Path(entry["executable"])).resolve()
    require(
        executable.is_relative_to(tool_root),
        "vcpkg port-build CMake executable escapes its fixed tool root",
    )
    require(
        executable.is_file(),
        f"missing vcpkg port-build CMake executable: {executable}",
    )

    archive = (downloads / entry["archive"]).resolve()
    require(
        archive.parent == downloads.resolve() and archive.is_file(),
        "vcpkg port-build CMake archive escapes downloads or is missing",
    )
    archive_sha512 = sha512(archive)
    require(
        archive_sha512 == entry["sha512"],
        "vcpkg port-build CMake archive SHA-512 does not match manifest",
    )
    executable_sha256 = require_archive_member_matches_file(
        archive,
        entry["executable"],
        executable,
    )
    require(
        executable_sha256
        == EXPECTED_VCPKG_PORT_CMAKE_EXECUTABLE_SHA256,
        "vcpkg port-build CMake executable SHA-256 drifted",
    )
    version_output = first_line(run_text(executable, "--version"))
    require(
        version_output
        == f"cmake version {EXPECTED_VCPKG_PORT_CMAKE}",
        f"unexpected vcpkg port-build CMake identity: {version_output}",
    )
    return {
        "version": EXPECTED_VCPKG_PORT_CMAKE,
        "versionOutput": version_output,
        "executable": relative_path(repo, executable),
        "executableSha256": executable_sha256,
        "toolsManifest": {
            "path": relative_path(repo, manifest_path),
            "sha256": manifest_sha256,
            "entry": entry,
        },
        "archive": {
            "path": relative_path(repo, archive),
            "sha512": archive_sha512,
        },
    }


def verify_hermetic_environment(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    environment = {name.upper(): value for name, value in os.environ.items()}
    for name in FORBIDDEN_BUILD_ENVIRONMENT_NAMES:
        require(
            name not in environment,
            f"ambient build environment variable survived scrubbing: {name}",
        )
    allowed_prefixed = {
        "CMAKE_NINJA_FORCE_RESPONSE_FILE",
        "EMSCRIPTEN_ROOT",
        "EMSCRIPTEN_VERSION",
        "EMSDK",
        "EMSDK_NODE",
        "EMSDK_PYTHON",
        "EM_CACHE",
        "EM_CONFIG",
        "VCPKG_DEFAULT_BINARY_CACHE",
        "VCPKG_DISABLE_METRICS",
        "VCPKG_ROOT",
    }
    hostile_prefixes = (
        "CCACHE_",
        "CMAKE_",
        "EMCC_",
        "EMMAKEN_",
        "EMSCRIPTEN_",
        "EMSCONS_PKG_CONFIG_",
        "EMSDK_",
        "EM_",
        "GIT_",
        "PKG_CONFIG_",
        "VCPKG_",
        "X_VCPKG_",
    )
    survivors = sorted(
        name
        for name in environment
        if name not in allowed_prefixed
        and any(name.startswith(prefix) for prefix in hostile_prefixes)
    )
    require(
        not survivors,
        f"ambient build environment overrides survived: {survivors}",
    )

    expected_paths = {
        "EMSDK": emsdk,
        "EMSDK_NODE": repo / EXPECTED_EMSDK_NODE,
        "EMSDK_PYTHON": repo / EXPECTED_EMSDK_PYTHON,
        "EMSCRIPTEN_ROOT": emsdk / "upstream" / "emscripten",
        "EM_CONFIG": emsdk / ".emscripten",
        "EM_CACHE": emsdk / "upstream" / "emscripten" / "cache",
        "VCPKG_ROOT": vcpkg,
        "VCPKG_DEFAULT_BINARY_CACHE": (
            repo / ".wasm-vcpkg" / "bincache"
        ),
    }
    canonical: dict[str, str] = {}
    for name, expected in expected_paths.items():
        require_same_path(environment.get(name, ""), expected, name)
        canonical[name] = relative_path(repo, expected)
    expected_values = {
        "EMSCRIPTEN_VERSION": EXPECTED_EMSCRIPTEN,
        "VCPKG_DISABLE_METRICS": "1",
        "CMAKE_NINJA_FORCE_RESPONSE_FILE": "1",
        "PYTHONDONTWRITEBYTECODE": "1",
    }
    for name, expected in expected_values.items():
        require(
            environment.get(name) == expected,
            f"canonical environment {name} drifted",
        )
    canonical.update(expected_values)
    return {
        "forbiddenNamesAbsent": list(FORBIDDEN_BUILD_ENVIRONMENT_NAMES),
        "canonicalVariables": canonical,
    }


def verify_toolchains(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    lock_path = repo / "tools" / "wasm-probe" / "toolchain-lock.json"
    lock = json.loads(lock_path.read_text("utf-8"))
    require(lock["qt"]["version"] == EXPECTED_QT, "Qt lock drift")
    require(
        lock["qt"]["qtbaseWasmPatchSha256"].casefold()
        == EXPECTED_OVERLAY_SHA256[
            "qtbase/restore-wasm-version-check.patch"
        ],
        "QtBase Wasm helper patch lock drift",
    )
    require(
        lock["emscripten"] == EXPECTED_EMSCRIPTEN_LOCK_ENTRY,
        "Emscripten lock drift",
    )
    require(
        lock["vcpkg"]["baseline"] == EXPECTED_VCPKG_COMMIT,
        "vcpkg lock drift",
    )
    require(
        lock["buildTools"]["cmake"] == EXPECTED_OUTER_CMAKE_LOCK_ENTRY,
        "outer/probe CMake lock drift",
    )
    require(
        lock["buildTools"]["ninja"] == EXPECTED_NINJA_LOCK_ENTRY,
        "Ninja lock drift",
    )
    require(
        lock["hostCompiler"] == EXPECTED_HOST_COMPILER_CONTRACT,
        "host compiler lock drift",
    )

    expected_emsdk = repo / ".toolchains" / f"emsdk-{EXPECTED_EMSCRIPTEN}"
    expected_vcpkg = (
        repo / ".toolchains" / f"vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}"
    )
    expected_outer_cmake = (
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
    expected_emsdk_python = repo / EXPECTED_EMSDK_PYTHON
    expected_emsdk_node = repo / EXPECTED_EMSDK_NODE

    require_same_path(emsdk, expected_emsdk, "emsdk argument")
    require_same_path(vcpkg, expected_vcpkg, "vcpkg argument")
    for path in (
        expected_outer_cmake,
        expected_ninja,
        expected_vcpkg_exe,
        expected_emxx,
        expected_emcc,
        expected_emxx_driver,
        expected_emsdk_python,
        expected_emsdk_node,
    ):
        require(path.is_file(), f"missing pinned tool: {path}")

    resolved_commands = {
        name: shutil.which(name)
        for name in ("cmake", "ninja", "vcpkg", "em++", "emcc")
    }
    for name, value in resolved_commands.items():
        require(value is not None, f"{name} is not available in wrapper PATH")
    require_same_path(
        resolved_commands["cmake"],
        expected_outer_cmake,
        "outer/probe CMake PATH",
    )
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
    require_same_path(
        emsdk_python,
        expected_emsdk_python,
        "EMSDK_PYTHON",
    )
    require_same_path(
        os.environ.get("EMSDK_NODE", ""),
        expected_emsdk_node,
        "EMSDK_NODE",
    )
    require_same_path(
        os.environ.get("EM_CONFIG", ""),
        expected_emsdk / ".emscripten",
        "EM_CONFIG",
    )
    hermetic_environment = verify_hermetic_environment(repo, emsdk, vcpkg)

    emsdk_head = verify_pinned_repository(
        emsdk,
        EXPECTED_EMSDK_COMMIT,
        "emsdk",
    )
    vcpkg_head = verify_pinned_repository(
        vcpkg,
        EXPECTED_VCPKG_COMMIT,
        "vcpkg",
    )
    emscripten_installation = verify_emscripten_installation(
        emsdk,
        EXPECTED_EMSCRIPTEN,
        lock["emscripten"],
    )
    outer_cmake_installation = verify_authenticated_build_tool(
        repo,
        "CMake",
        lock["buildTools"]["cmake"],
        expected_outer_cmake.parent.parent,
    )
    ninja_installation = verify_authenticated_build_tool(
        repo,
        "Ninja",
        lock["buildTools"]["ninja"],
        expected_ninja.parent,
    )
    # No version-bearing executable is run until the full Emscripten, CMake,
    # and Ninja byte inventories have all passed.
    vcpkg_port_cmake = verify_vcpkg_port_build_cmake(repo, vcpkg)

    outer_cmake_sha256 = verify_locked_executable(
        expected_outer_cmake,
        lock["buildTools"]["cmake"]["executableSha256"],
        "outer/probe CMake",
    )
    ninja_sha256 = verify_locked_executable(
        expected_ninja,
        lock["buildTools"]["ninja"]["executableSha256"],
        "Ninja",
    )
    emxx_version = run_text(
        emsdk_python,
        "-B",
        "-E",
        expected_emxx_driver,
        "--version",
    )
    vcpkg_version = run_text(expected_vcpkg_exe, "version")
    outer_cmake_version = run_text(expected_outer_cmake, "--version")
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
        first_line(outer_cmake_version)
        == f"cmake version {EXPECTED_OUTER_CMAKE}",
        "unexpected outer/probe CMake identity: "
        f"{first_line(outer_cmake_version)}",
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
            "emsdkRoot": relative_path(repo, expected_emsdk),
            "python": relative_path(repo, emsdk_python),
            "node": relative_path(repo, expected_emsdk_node),
            "launcher": relative_path(repo, expected_emxx),
            "driver": relative_path(repo, expected_emxx_driver),
            "installation": emscripten_installation,
        },
        "vcpkg": {
            "baselineCommit": vcpkg_head,
            "versionOutput": first_line(vcpkg_version),
            "executable": relative_path(repo, expected_vcpkg_exe),
        },
        "outerProbeCMake": {
            "version": EXPECTED_OUTER_CMAKE,
            "versionOutput": first_line(outer_cmake_version),
            "executable": relative_path(repo, expected_outer_cmake),
            "executableSha256": outer_cmake_sha256,
            "lockEntry": dict(EXPECTED_OUTER_CMAKE_LOCK_ENTRY),
            "installation": outer_cmake_installation,
        },
        "vcpkgPortBuildCMake": vcpkg_port_cmake,
        "ninja": {
            "version": EXPECTED_NINJA,
            "versionOutput": first_line(ninja_version),
            "executable": relative_path(repo, expected_ninja),
            "executableSha256": ninja_sha256,
            "lockEntry": dict(EXPECTED_NINJA_LOCK_ENTRY),
            "installation": ninja_installation,
        },
        "hermeticEnvironment": hermetic_environment,
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
    require_port_version(target_packages["qtbase"], "2", "qtbase")
    require_port_version(
        target_packages["qtdeclarative"],
        "1",
        "qtdeclarative",
    )
    require_status_record(
        status_records,
        "qtmultimedia",
        TARGET_TRIPLET,
        feature="qml",
    )
    require_status_record(status_records, "qtbase", HOST_TRIPLET)
    host_qtdeclarative = require_status_record(
        status_records,
        "qtdeclarative",
        HOST_TRIPLET,
    )
    require_port_version(
        host_qtdeclarative,
        "1",
        "host qtdeclarative",
    )
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
    passthrough = require_vcpkg_env_passthrough(
        target_triplet,
        ("EMSDK", "EMSDK_PYTHON"),
    )
    require(
        tuple(passthrough) == EXPECTED_TARGET_ENV_PASSTHROUGH,
        "target triplet VCPKG_ENV_PASSTHROUGH drifted: "
        f"{passthrough}",
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
        reported = require_host_tool_version(name, output)
        host_tools[name] = {
            "path": relative_path(repo, candidates[0]),
            "versionOutput": reported,
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
    require(
        probe_cache.get("CMAKE_GENERATOR") == "Ninja",
        "probe CMAKE_GENERATOR is not exactly Ninja",
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
            "qtDeclarativePortVersion": "1",
            "qtCoreDllCount": len(host_core_dlls),
            "tools": host_tools,
        },
        "targetTripletEnvironmentPassthrough": passthrough,
    }


def compiler_identity_from_cmake(
    build: Path,
    language: str,
) -> dict[str, Any]:
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
    properties: dict[str, str] = {}
    for suffix in (
        "COMPILER",
        "COMPILER_ID",
        "COMPILER_VERSION",
        "COMPILER_FRONTEND_VARIANT",
        "COMPILER_ARCHITECTURE_ID",
        "PLATFORM_ID",
    ):
        name = f"CMAKE_{language}_{suffix}"
        match = re.search(
            rf'set\({re.escape(name)} "([^"]*)"\)',
            text,
        )
        require(match is not None, f"{build}: {name} missing")
        properties[suffix] = match.group(1)
    compiler = Path(properties["COMPILER"]).resolve()
    require(compiler.is_file(), f"missing recorded compiler: {compiler}")
    return {
        "path": compiler,
        "id": properties["COMPILER_ID"],
        "version": properties["COMPILER_VERSION"],
        "frontendVariant": properties["COMPILER_FRONTEND_VARIANT"],
        "architecture": properties["COMPILER_ARCHITECTURE_ID"],
        "platform": properties["PLATFORM_ID"],
    }


def compiler_path_from_cmake(build: Path, language: str) -> tuple[Path, str]:
    identity = compiler_identity_from_cmake(build, language)
    return identity["path"], identity["version"]


def audited_path(repo: Path, path: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(repo.resolve()).as_posix()
    except ValueError:
        return resolved.as_posix()


def verify_qtdeclarative_cache_provenance(
    repo: Path,
    installed: Path,
    buildtrees: Path,
    vcpkg_port_cmake: Path,
) -> dict[str, Any]:
    lock = json.loads(
        (
            repo / "tools" / "wasm-probe" / "toolchain-lock.json"
        ).read_text("utf-8")
    )
    host_compiler_contract = lock["hostCompiler"]
    source_root = buildtrees / "qtdeclarative" / "src"
    packages = installed.parent / "packages"
    target_build = buildtrees / "qtdeclarative" / f"{TARGET_TRIPLET}-rel"
    host_build = buildtrees / "qtdeclarative" / f"{HOST_TRIPLET}-rel"
    expected_toolchain = (
        repo
        / ".toolchains"
        / f"vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}"
        / "scripts"
        / "buildsystems"
        / "vcpkg.cmake"
    )
    expected_emxx = (
        repo
        / ".toolchains"
        / f"emsdk-{EXPECTED_EMSCRIPTEN}"
        / "upstream"
        / "emscripten"
        / "em++.bat"
    )
    records: dict[str, dict[str, Any]] = {}
    specifications = (
        (
            "target",
            target_build,
            TARGET_TRIPLET,
            installed / TARGET_TRIPLET,
            installed / HOST_TRIPLET,
        ),
        (
            "host",
            host_build,
            HOST_TRIPLET,
            installed / HOST_TRIPLET,
            None,
        ),
    )
    sources: list[Path] = []
    for label, build, triplet, qt_prefix, qt_host_path in specifications:
        require(
            build.resolve()
            == (
                buildtrees / "qtdeclarative" / f"{triplet}-rel"
            ).resolve(),
            f"QtDeclarative {label} build directory drifted",
        )
        cache = parse_cmake_cache(build / "CMakeCache.txt")
        require_cache_cmake_command(
            cache,
            vcpkg_port_cmake,
            f"QtDeclarative {label}",
        )
        expected_install_prefix = packages / f"qtdeclarative_{triplet}"
        expected_cache = {
            "VCPKG_TARGET_TRIPLET": triplet,
            "CMAKE_GENERATOR": "Ninja",
            "CMAKE_INSTALL_PREFIX": str(expected_install_prefix),
            "VCPKG_INSTALLED_DIR": str(installed),
            "Qt6_DIR": str(qt_prefix / "share" / "Qt6"),
            "CMAKE_TOOLCHAIN_FILE": str(expected_toolchain),
            "QT_HOST_PATH": (
                str(qt_host_path) if qt_host_path is not None else ""
            ),
        }
        for name, expected_value in expected_cache.items():
            if name in {
                "CMAKE_INSTALL_PREFIX",
                "VCPKG_INSTALLED_DIR",
                "Qt6_DIR",
                "CMAKE_TOOLCHAIN_FILE",
                "QT_HOST_PATH",
            } and expected_value:
                require_same_path(
                    cache.get(name, ""),
                    Path(expected_value),
                    f"QtDeclarative {label} {name}",
                )
            else:
                require_exact_cache_values(
                    cache,
                    {name: expected_value},
                    f"QtDeclarative {label}",
                )
        for required_path in (
            expected_install_prefix,
            installed,
            qt_prefix / "share" / "Qt6",
            expected_toolchain,
        ):
            require(
                required_path.exists(),
                f"QtDeclarative {label} provenance path missing: "
                f"{required_path}",
            )
        source = Path(cache.get("CMAKE_HOME_DIRECTORY", "")).resolve()
        require(
            source.is_dir()
            and source.is_relative_to(source_root.resolve())
            and re.fullmatch(
                r"here-src-\d+-[0-9a-f]+\.clean",
                source.name,
            )
            is not None
            and (source / "CMakeLists.txt").is_file(),
            f"QtDeclarative {label} source directory is stale or escapes "
            f"the fixed source tree: {source}",
        )
        sources.append(source)

        compiler = compiler_identity_from_cmake(build, "CXX")
        if label == "target":
            require_same_path(
                compiler["path"],
                expected_emxx,
                "QtDeclarative target compiler",
            )
            require(
                {
                    key: compiler[key]
                    for key in (
                        "id",
                        "version",
                        "frontendVariant",
                        "architecture",
                        "platform",
                    )
                }
                == {
                    "id": "Clang",
                    "version": "21.0.0",
                    "frontendVariant": "GNU",
                    "architecture": "wasm32",
                    "platform": "",
                },
                "QtDeclarative target compiler identity drifted",
            )
            compiler_evidence = {
                "path": audited_path(repo, compiler["path"]),
                "pathAuthenticated": True,
                **{
                    key: compiler[key]
                    for key in (
                        "id",
                        "version",
                        "frontendVariant",
                        "architecture",
                        "platform",
                    )
                },
            }
        else:
            require_same_path(
                cache.get("CMAKE_CXX_COMPILER", ""),
                compiler["path"],
                "QtDeclarative host cached compiler",
            )
            compiler_evidence = verify_host_compiler_identity(
                compiler,
                host_compiler_contract,
            )

        records[label] = {
            "buildDirectory": relative_path(repo, build),
            "sourceDirectory": relative_path(repo, source),
            "targetTriplet": triplet,
            "generator": "Ninja",
            "cmakeCommand": relative_path(repo, vcpkg_port_cmake),
            "installPrefix": relative_path(repo, expected_install_prefix),
            "installedRoot": relative_path(repo, installed),
            "qtPackagePrefix": relative_path(
                repo,
                qt_prefix / "share" / "Qt6",
            ),
            "qtHostPath": (
                relative_path(repo, qt_host_path)
                if qt_host_path is not None
                else ""
            ),
            "toolchain": relative_path(repo, expected_toolchain),
            "compiler": compiler_evidence,
        }
    require(
        path_key(sources[0]) == path_key(sources[1]),
        "QtDeclarative target and host caches use different source trees",
    )
    return records


def verify_cmake_identity(
    repo: Path,
    build: Path,
    buildtrees: Path,
    outer_probe_cmake: Path,
    vcpkg_port_cmake: Path,
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
    probe_cmake_command = require_cache_cmake_command(
        probe_cache,
        outer_probe_cmake,
        "outer probe",
    )
    require_same_path(
        probe_cache.get("CMAKE_MAKE_PROGRAM", ""),
        expected_ninja,
        "probe CMAKE_MAKE_PROGRAM",
    )
    require(
        probe_cache.get("CMAKE_GENERATOR") == "Ninja",
        "probe CMAKE_GENERATOR is not exactly Ninja",
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
    qtbase_cache = parse_cmake_cache(qtbase_build / "CMakeCache.txt")
    qtbase_cmake_command = require_cache_cmake_command(
        qtbase_cache,
        vcpkg_port_cmake,
        "QtBase target",
    )
    require(
        qtbase_cache.get("CMAKE_GENERATOR") == "Ninja",
        "QtBase target CMAKE_GENERATOR is not exactly Ninja",
    )
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
        "outerProbeCMakeCommand": relative_path(
            repo,
            probe_cmake_command,
        ),
        "qtBaseTargetCMakeCommand": relative_path(
            repo,
            qtbase_cmake_command,
        ),
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
            require_wasm_compile_contract(
                arguments,
                language=language,
                context=f"{port}: {entry.get('file')}",
            )
            for setting in settings:
                coverage[setting] += 1

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
        contract = require_wasm_compile_contract(
            arguments,
            language="cxx",
            context=source_name,
        )
        canonical = canonical_command(repo, "\0".join(arguments))
        boundary[source_name] = {
            "settings": list(CXX_COMPILE_SETTINGS),
            "effectiveSettings": contract["effectiveValues"],
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
        "forbiddenArgumentsAbsent": list(FORBIDDEN_TARGET_ARGUMENTS),
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
    expected_emxx = (
        repo
        / ".toolchains"
        / f"emsdk-{EXPECTED_EMSCRIPTEN}"
        / "upstream"
        / "emscripten"
        / "em++.bat"
    )
    build_ninja = (build / "build.ninja").read_text(
        "utf-8",
        errors="replace",
    )
    rules_ninja = (build / "CMakeFiles" / "rules.ninja").read_text(
        "utf-8",
        errors="replace",
    )
    stream = verify_application_link_argument_stream(
        application_link,
        expected_emxx,
        build_ninja,
        rules_ninja,
    )
    bindings = stream["edge"]["bindings"]
    setting_contract = stream["settingContract"]
    archive = stream["archive"]
    cmake_source = (
        repo / "tools" / "wasm-probe" / "CMakeLists.txt"
    ).read_text("utf-8")
    require(
        not configured_asyncify(cmake_source),
        "literal -sASYNCIFY is configured in probe CMake",
    )
    return {
        "compiler": relative_path(repo, expected_emxx),
        "responseFile": "CMakeFiles/RhythmGameWasmProbe.rsp",
        "responseFileContentTemplate": (
            "$in $LINK_PATH $LINK_LIBRARIES"
        ),
        "responseArgumentCount": len(stream["responseArguments"]),
        "responseArgumentsSha256": sha256_text(
            canonical_command(
                repo,
                "\0".join(stream["responseArguments"]),
            )
        ),
        "effectiveArgumentCount": len(stream["effectiveArguments"]),
        "effectiveArgumentsSha256": sha256_text(
            canonical_command(
                repo,
                "\0".join(stream["effectiveArguments"]),
            )
        ),
        "selectedNinjaRule": stream["edge"]["rule"],
        "settings": list(APPLICATION_LINK_SETTINGS),
        "forbiddenArgumentsAbsent": list(FORBIDDEN_TARGET_ARGUMENTS),
        "effectiveSettings": setting_contract["effectiveValues"],
        "settingOccurrences": setting_contract["occurrences"],
        "literalAsyncifyConfigured": False,
        "staticExceptionArchive": archive,
        "staticExceptionArchiveLinked": True,
        "commandSha256": sha256_text(
            canonical_command(repo, application_link)
        ),
        "linkLibrariesSha256": sha256_text(
            canonical_command(repo, bindings["LINK_LIBRARIES"])
        ),
    }


def verify_mismatched_emscripten_rejected(
    repo: Path,
    installed: Path,
) -> dict[str, Any]:
    cmake = (
        repo
        / ".toolchains"
        / "cmake-4.2.3-windows-x86_64"
        / "bin"
        / "cmake.exe"
    )
    qt = installed / TARGET_TRIPLET / "share" / "Qt6"
    install_paths = qt / "QtInstallPaths.cmake"
    helper = qt / "QtPublicWasmToolchainHelpers.cmake"
    for path in (cmake, install_paths, helper):
        require(path.is_file(), f"missing mismatch-contract input: {path}")

    with tempfile.TemporaryDirectory(
        prefix="rhythm-game-wasm-mismatch-"
    ) as temporary:
        root = Path(temporary)
        fake_sdk = root / "fake-emsdk"
        fake_emcc = fake_sdk / "upstream" / "emscripten" / "emcc.bat"
        fake_emcc.parent.mkdir(parents=True)
        (fake_sdk / ".emscripten").write_text(
            (
                "emsdk_path = r'ignored-by-qt-regex'\n"
                "EMSCRIPTEN_ROOT = emsdk_path + '/upstream/emscripten'\n"
            ),
            encoding="utf-8",
        )
        fake_emcc.write_text(
            (
                "@echo off\n"
                "echo emcc (Emscripten compiler) 9.9.9\n"
                "exit /b 0\n"
            ),
            encoding="utf-8",
        )
        check = root / "check-mismatch.cmake"
        check.write_text(
            (
                f'include("{install_paths.as_posix()}")\n'
                f'include("{helper.as_posix()}")\n'
                "_qt_test_emscripten_version()\n"
                'message(FATAL_ERROR "MISMATCH_SENTINEL")\n'
            ),
            encoding="utf-8",
        )
        environment = os.environ.copy()
        environment["EMSDK"] = str(fake_sdk)
        result = subprocess.run(
            [str(cmake), "-P", str(check)],
            cwd=repo,
            env=environment,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=30,
            check=False,
        )
    output = result.stdout + result.stderr
    require(result.returncode != 0, "mismatched Emscripten was accepted")
    require(
        "MISMATCH_SENTINEL" not in output,
        "mismatched Emscripten reached the post-check sentinel",
    )
    for diagnostic in (
        "Qt Wasm was built with Emscripten version: 4.0.7",
        "You are using Emscripten version: 9.9.9",
        "Stopping configuration due to mismatch",
    ):
        require(
            diagnostic in output,
            f"mismatch rejection diagnostic missing: {diagnostic}",
        )
    return {
        "builtVersion": EXPECTED_EMSCRIPTEN,
        "activeVersion": "9.9.9",
        "rejectedBeforeSentinel": True,
    }


def verify_features_and_autogen(
    repo: Path,
    installed: Path,
    buildtrees: Path,
    vcpkg_port_cmake: Path,
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
    declarative_cache_provenance = (
        verify_qtdeclarative_cache_provenance(
            repo,
            installed,
            buildtrees,
            vcpkg_port_cmake,
        )
    )
    for label, cache in (
        ("QtBase target", qtbase_cache),
        ("QtDeclarative target", target_cache),
        ("QtDeclarative host", host_cache),
    ):
        require(
            cache.get("CMAKE_GENERATOR") == "Ninja",
            f"{label} CMAKE_GENERATOR is not exactly Ninja",
        )

    qtbase_cache_state = {
        "EMCC_VERSION": EXPECTED_EMSCRIPTEN,
        "QT_AUTODETECT_WASM_IS_DONE": "TRUE",
        "QT_EMCC_RECOMMENDED_VERSION": EXPECTED_EMSCRIPTEN,
        "QT_QMAKE_TARGET_MKSPEC": "wasm-emscripten",
    }
    for name, value in qtbase_cache_state.items():
        require(
            qtbase_cache.get(name) == value,
            f"QtBase cache {name} is {qtbase_cache.get(name)!r}, "
            f"expected {value!r}",
        )
    qtbase_cache_text = (qtbase / "CMakeCache.txt").read_text(
        "utf-8",
        errors="replace",
    ).replace("\r\n", "\n")
    for name in ("EMCC_VERSION", "QT_EMCC_RECOMMENDED_VERSION"):
        require(
            re.search(
                rf"(?m)^//INTERNAL\n{name}:STRING="
                rf"{re.escape(EXPECTED_EMSCRIPTEN)}$",
                qtbase_cache_text,
            )
            is not None,
            f"QtBase cache {name} lost its documented STRING state",
        )
    require(
        re.search(
            r"(?m)^QT_AUTODETECT_WASM_IS_DONE:BOOL=TRUE$",
            qtbase_cache_text,
        )
        is not None,
        "QtBase cache auto-detection completion state drifted",
    )
    require(
        re.search(
            r"(?m)^QT_QMAKE_TARGET_MKSPEC:STRING=wasm-emscripten$",
            qtbase_cache_text,
        )
        is not None,
        "QtBase cache target mkspec state drifted",
    )

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

    qconfig_path = (
        installed
        / TARGET_TRIPLET
        / "include"
        / "Qt6"
        / "QtCore"
        / "qconfig.h"
    )
    qconfig = qconfig_path.read_text("utf-8", errors="replace")
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
    emcc_definitions = re.findall(
        r'^#define QT_EMCC_VERSION "([^"]+)"$',
        qconfig,
        re.MULTILINE,
    )
    require(
        emcc_definitions == [EXPECTED_EMSCRIPTEN],
        f"installed qconfig.h QT_EMCC_VERSION is {emcc_definitions}",
    )

    qconfig_pri_path = (
        installed
        / TARGET_TRIPLET
        / "share"
        / "Qt6"
        / "mkspecs"
        / "qconfig.pri"
    )
    qconfig_pri = qconfig_pri_path.read_text("utf-8", errors="replace")
    pri_versions = re.findall(
        r"^QT_EMCC_VERSION\s*=\s*(\S+)\s*$",
        qconfig_pri,
        re.MULTILINE,
    )
    require(
        pri_versions == [EXPECTED_EMSCRIPTEN],
        f"installed qconfig.pri QT_EMCC_VERSION is {pri_versions}",
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
    installed_helper_path = (
        installed
        / TARGET_TRIPLET
        / "share"
        / "Qt6"
        / "QtPublicWasmToolchainHelpers.cmake"
    )
    installed_helper = installed_helper_path.read_text(
        "utf-8",
        errors="replace",
    )
    require(
        sha256(installed_helper_path)
        == EXPECTED_INSTALLED_WASM_HELPER_SHA256,
        "installed Qt Wasm helper reviewed bytes drifted",
    )
    helper_branches = {
        "layoutAwareInstalledHeaders": (
            "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}/"
            "QtCore/qconfig.h"
        ),
        "buildTreeFallback": (
            "${WASM_BUILD_DIR}/src/corelib/global/qconfig.h"
        ),
        "defaultInstalledFallback": (
            "${WASM_BUILD_DIR}/include/QtCore/qconfig.h"
        ),
    }
    for label, fragment in helper_branches.items():
        require(
            fragment in installed_helper,
            f"installed Qt Wasm helper lost {label}",
        )
    mismatch_contract = verify_mismatched_emscripten_rejected(
        repo,
        installed,
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
    autogen_compiler, autogen_compiler_version = compiler_path_from_cmake(
        declarative_target,
        "CXX",
    )
    require_same_path(
        autogen_compiler,
        expected_emxx,
        "QtDeclarative target C++ compiler",
    )
    require(
        autogen_compiler_version == "21.0.0",
        "QtDeclarative target C++ compiler version drift",
    )
    autogen_infos = sorted(
        declarative_target.rglob("AutogenInfo.json"),
        key=lambda path: path.as_posix().casefold(),
    )
    require(autogen_infos, "no QtDeclarative AutoGen metadata found")
    predefs_values: list[str] = []
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
        predefs_values.append(predefs_file)

    predefs_files = validate_autogen_predefs_paths(
        declarative_target,
        predefs_values,
    )
    require(
        len(predefs_files) == len(autogen_infos),
        "not every AutoGen metadata record has a unique predefines output",
    )

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
        "qtDeclarativeCacheProvenance": declarative_cache_provenance,
        "emscriptenSdkContract": {
            "qtbaseCache": qtbase_cache_state,
            "installedQconfigHeaderVersion": EXPECTED_EMSCRIPTEN,
            "installedQconfigPriVersion": EXPECTED_EMSCRIPTEN,
            "installedHelper": {
                "path": relative_path(repo, installed_helper_path),
                "sha256": sha256(installed_helper_path),
                **{label: True for label in helper_branches},
            },
            "mismatchedConsumer": mismatch_contract,
        },
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
            "compiler": relative_path(repo, autogen_compiler),
            "compilerVersion": autogen_compiler_version,
            "metadataCount": len(autogen_infos),
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
    qtbase_patch = normalized_text(
        qtbase / "restore-wasm-version-check.patch"
    )
    for fragment in (
        "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}/QtCore/qconfig.h",
        "${WASM_BUILD_DIR}/src/corelib/global/qconfig.h",
        "${WASM_BUILD_DIR}/include/QtCore/qconfig.h",
    ):
        require(
            fragment in qtbase_patch,
            f"QtBase Wasm helper patch lost path branch: {fragment}",
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
        -DQT_QMAKE_TARGET_MKSPEC:STRING=wasm-emscripten
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
    baseline_qtbase_json["port-version"] = 2
    require(
        json.loads((qtbase / "vcpkg.json").read_text("utf-8"))
        == baseline_qtbase_json,
        "qtbase vcpkg.json differs beyond port-version 2",
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
                "seed wasm-emscripten target mkspec before project",
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


def require_exact_deployment_set(build: Path) -> list[str]:
    recognized = {
        path.name
        for path in build.iterdir()
        if path.is_file()
        and path.name not in NON_DEPLOYABLE_ROOT_FILES
        and path.name.casefold().endswith(
            RECOGNIZED_WEB_DEPLOYABLE_SUFFIXES
        )
    }
    expected = set(DEPLOYMENT_ARTIFACTS)
    require(
        recognized == expected,
        "recognized web deployment files must match the exact allowlist; "
        f"unexpected={sorted(recognized - expected)}, "
        f"missing={sorted(expected - recognized)}",
    )
    return list(DEPLOYMENT_ARTIFACTS)


def verify_shader_resource_contract(build: Path) -> dict[str, Any]:
    qrc = build / ".qt" / "rcc" / "wasm_probe_shaders.qrc"
    generated = build / ".qt" / "rcc" / "qrc_wasm_probe_shaders.cpp"
    require(qrc.is_file(), "generated shader resource QRC missing")
    require(generated.is_file(), "generated shader resource C++ missing")
    try:
        root = ElementTree.parse(qrc).getroot()
    except ElementTree.ParseError as error:
        raise AssertionError(f"shader resource QRC is invalid XML: {error}") from error
    require(
        root.tag == "RCC"
        and not root.attrib
        and len(root) == 1
        and root[0].tag == "qresource"
        and root[0].attrib == {"prefix": SHADER_RESOURCE_PREFIX},
        "shader resource QRC root/prefix set drifted",
    )
    files = list(root[0])
    require(
        len(files) == 1
        and files[0].tag == "file"
        and files[0].attrib == {"alias": SHADER_RESOURCE_ALIAS}
        and len(files[0]) == 0,
        "shader resource alias set must contain only pulse.frag.qsb",
    )
    source_value = (files[0].text or "").strip()
    require(source_value, "shader resource source path is empty")
    source = Path(source_value).resolve()
    expected_source = (build / ".qsb" / SHADER_RESOURCE_ALIAS).resolve()
    require_same_path(source, expected_source, "shader resource QSB source")
    require(
        source.is_relative_to(build.resolve())
        and source.is_file()
        and source.stat().st_size > 0,
        f"shader resource source is missing or escapes build: {source}",
    )

    generated_text = generated.read_text("utf-8", errors="replace")
    generated_tree = tuple(
        match.group(1).strip()
        for match in re.finditer(
            r"(?m)^\s*//\s+(:[^\r\n]*)$",
            generated_text,
        )
    )
    require(
        generated_tree == SHADER_RESOURCE_TREE,
        "generated shader resource binding set drifted: "
        f"{generated_tree}",
    )
    return {
        "prefix": SHADER_RESOURCE_PREFIX,
        "aliases": [SHADER_RESOURCE_ALIAS],
        "resourcePaths": [SHADER_RESOURCE_PATH],
        "source": source,
    }


def verify_artifacts(repo: Path, build: Path) -> dict[str, Any]:
    deployment_files = require_exact_deployment_set(build)
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

    shader = verify_shader_resource_contract(build)

    hashed = (*artifacts, exception_archive, compile_database)
    return {
        "deploymentFiles": deployment_files,
        "recognizedWebDeployables": deployment_files,
        "externalWorkerArtifacts": sorted(external_workers),
        "pthreadWorkerEmbeddedInMainJavaScript": True,
        "pthreadBootstrapMarkers": list(pthread_markers),
        "shaderResourceAlias": SHADER_RESOURCE_PATH,
        "shaderResourceAliases": shader["resourcePaths"],
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
    ninja = repo / toolchains["ninja"]["executable"]
    outer_probe_cmake = (
        repo / toolchains["outerProbeCMake"]["executable"]
    ).resolve()
    vcpkg_port_cmake = (
        repo / toolchains["vcpkgPortBuildCMake"]["executable"]
    ).resolve()
    build_freshness = verify_build_freshness(repo, build, ninja)
    qt = verify_qt_installation(repo, installed, build)
    cmake_identity = verify_cmake_identity(
        repo,
        build,
        buildtrees,
        outer_probe_cmake,
        vcpkg_port_cmake,
    )
    compile_commands = verify_compile_commands(repo, build, buildtrees)
    application_link = verify_application_link(repo, build, ninja)
    features = verify_features_and_autogen(
        repo,
        installed,
        buildtrees,
        vcpkg_port_cmake,
    )
    overlays = verify_overlays(repo, vcpkg)
    artifacts = verify_artifacts(repo, build)

    return {
        "schemaVersion": 2,
        "gate": "1A",
        "scope": GATE_SCOPE,
        "technicalProbePassed": True,
        "gate1aPassed": True,
        "gate0Satisfied": False,
        "formalGate1EntryAuthorized": False,
        "gate1Passed": False,
        "toolchains": toolchains,
        "buildFreshness": build_freshness,
        "cmakeBuild": cmake_identity,
        "qt": qt,
        "featuresAndAutogen": features,
        "compileCommands": compile_commands,
        "applicationLink": application_link,
        "overlays": overlays,
        "artifacts": artifacts,
        "unprovenUntilGate1B": list(GATE_1B_LIMITATIONS),
    }


def require_mapping(value: Any, label: str) -> Mapping[str, Any]:
    require(isinstance(value, Mapping), f"{label} must be a mapping")
    return value


def require_exact_keys(
    value: Mapping[str, Any],
    expected: Iterable[str],
    label: str,
) -> None:
    expected_set = set(expected)
    require(
        set(value) == expected_set,
        f"{label} keys must be exactly {sorted(expected_set)}, "
        f"got {sorted(value)}",
    )


def require_exact_type(value: Any, expected: type, label: str) -> None:
    require(
        type(value) is expected,
        f"{label} must be {expected.__name__}, got {type(value).__name__}",
    )


def require_sha256_value(value: Any, label: str) -> None:
    require_exact_type(value, str, label)
    require(
        re.fullmatch(r"[0-9a-f]{64}", value) is not None,
        f"{label} must be a lowercase SHA-256",
    )


def require_sha512_value(value: Any, label: str) -> None:
    require_exact_type(value, str, label)
    require(
        re.fullmatch(r"[0-9a-f]{128}", value) is not None,
        f"{label} must be a lowercase SHA-512",
    )


def require_positive_int(value: Any, label: str) -> None:
    require_exact_type(value, int, label)
    require(value > 0, f"{label} must be positive")


def validate_toolchain_evidence(value: Any) -> None:
    toolchains = require_mapping(value, "toolchains")
    require_exact_keys(
        toolchains,
        {
            "lockFileSha256",
            "emscripten",
            "vcpkg",
            "outerProbeCMake",
            "vcpkgPortBuildCMake",
            "ninja",
            "hermeticEnvironment",
        },
        "toolchains",
    )
    require_sha256_value(
        toolchains["lockFileSha256"],
        "toolchains.lockFileSha256",
    )

    emscripten = require_mapping(
        toolchains["emscripten"],
        "toolchains.emscripten",
    )
    require_exact_keys(
        emscripten,
        {
            "version",
            "versionOutput",
            "emsdkCommit",
            "emsdkRoot",
            "python",
            "node",
            "launcher",
            "driver",
            "installation",
        },
        "toolchains.emscripten",
    )
    require(
        emscripten["version"] == EXPECTED_EMSCRIPTEN
        and emscripten["versionOutput"] == EXPECTED_EMXX_VERSION_LINE
        and emscripten["emsdkCommit"] == EXPECTED_EMSDK_COMMIT
        and emscripten["emsdkRoot"] == ".toolchains/emsdk-4.0.7"
        and emscripten["python"] == EXPECTED_EMSDK_PYTHON
        and emscripten["node"] == EXPECTED_EMSDK_NODE
        and emscripten["launcher"]
        == (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/"
            "em++.bat"
        )
        and emscripten["driver"]
        == ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.py",
        "Emscripten evidence does not match the exact pin",
    )
    installation = require_mapping(
        emscripten["installation"],
        "toolchains.emscripten.installation",
    )
    require_exact_keys(
        installation,
        {
            "releaseManifest",
            "releaseHash",
            "packageUrl",
            "generatedBytecode",
            "payload",
        },
        "toolchains.emscripten.installation",
    )
    require(
        installation
        == {
            name: EXPECTED_EMSCRIPTEN_LOCK_ENTRY[name]
            for name in (
                "releaseManifest",
                "releaseHash",
                "packageUrl",
                "generatedBytecode",
                "payload",
            )
        },
        "Emscripten installed payload evidence drifted",
    )

    vcpkg = require_mapping(toolchains["vcpkg"], "toolchains.vcpkg")
    require_exact_keys(
        vcpkg,
        {"baselineCommit", "versionOutput", "executable"},
        "toolchains.vcpkg",
    )
    require(
        vcpkg
        == {
            "baselineCommit": EXPECTED_VCPKG_COMMIT,
            "versionOutput": EXPECTED_VCPKG_VERSION_LINE,
            "executable": (
                f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}/vcpkg.exe"
            ),
        },
        "vcpkg evidence does not match the exact pin",
    )
    outer_cmake = require_mapping(
        toolchains["outerProbeCMake"],
        "toolchains.outerProbeCMake",
    )
    require_exact_keys(
        outer_cmake,
        {
            "version",
            "versionOutput",
            "executable",
            "executableSha256",
            "lockEntry",
            "installation",
        },
        "toolchains.outerProbeCMake",
    )
    require(
        outer_cmake
        == {
            "version": EXPECTED_OUTER_CMAKE,
            "versionOutput": f"cmake version {EXPECTED_OUTER_CMAKE}",
            "executable": (
                ".toolchains/cmake-4.2.3-windows-x86_64/bin/cmake.exe"
            ),
            "executableSha256": (
                EXPECTED_OUTER_CMAKE_LOCK_ENTRY["executableSha256"]
            ),
            "lockEntry": EXPECTED_OUTER_CMAKE_LOCK_ENTRY,
            "installation": {
                "archive": {
                    "path": (
                        ".toolchains/downloads/"
                        + EXPECTED_OUTER_CMAKE_LOCK_ENTRY["archiveFile"]
                    ),
                    "sha256": EXPECTED_OUTER_CMAKE_LOCK_ENTRY["sha256"],
                },
                "payload": EXPECTED_OUTER_CMAKE_LOCK_ENTRY["payload"],
                "installationRoot": (
                    ".toolchains/cmake-4.2.3-windows-x86_64"
                ),
            },
        },
        "outer/probe CMake evidence does not match the exact pin",
    )

    port_cmake = require_mapping(
        toolchains["vcpkgPortBuildCMake"],
        "toolchains.vcpkgPortBuildCMake",
    )
    require_exact_keys(
        port_cmake,
        {
            "version",
            "versionOutput",
            "executable",
            "executableSha256",
            "toolsManifest",
            "archive",
        },
        "toolchains.vcpkgPortBuildCMake",
    )
    expected_port_executable = (
        ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows/"
        "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
    )
    require(
        port_cmake["version"] == EXPECTED_VCPKG_PORT_CMAKE
        and port_cmake["versionOutput"]
        == f"cmake version {EXPECTED_VCPKG_PORT_CMAKE}"
        and port_cmake["executable"] == expected_port_executable
        and port_cmake["executableSha256"]
        == EXPECTED_VCPKG_PORT_CMAKE_EXECUTABLE_SHA256,
        "vcpkg port-build CMake executable evidence drifted",
    )
    require_sha256_value(
        port_cmake["executableSha256"],
        "toolchains.vcpkgPortBuildCMake.executableSha256",
    )
    tools_manifest = require_mapping(
        port_cmake["toolsManifest"],
        "toolchains.vcpkgPortBuildCMake.toolsManifest",
    )
    require_exact_keys(
        tools_manifest,
        {"path", "sha256", "entry"},
        "toolchains.vcpkgPortBuildCMake.toolsManifest",
    )
    require_sha256_value(
        tools_manifest["sha256"],
        "toolchains.vcpkgPortBuildCMake.toolsManifest.sha256",
    )
    require(
        tools_manifest["path"]
        == (
            f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}/"
            "scripts/vcpkg-tools.json"
        )
        and tools_manifest["sha256"]
        == EXPECTED_VCPKG_TOOLS_MANIFEST_SHA256
        and tools_manifest["entry"]
        == EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY,
        "vcpkg port-build CMake manifest evidence drifted",
    )
    archive = require_mapping(
        port_cmake["archive"],
        "toolchains.vcpkgPortBuildCMake.archive",
    )
    require_exact_keys(
        archive,
        {"path", "sha512"},
        "toolchains.vcpkgPortBuildCMake.archive",
    )
    require_sha512_value(
        archive["sha512"],
        "toolchains.vcpkgPortBuildCMake.archive.sha512",
    )
    require(
        archive
        == {
            "path": (
                ".wasm-vcpkg/downloads/"
                "cmake-4.3.3-windows-x86_64.zip"
            ),
            "sha512": (
                EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["sha512"]
            ),
        },
        "vcpkg port-build CMake archive evidence drifted",
    )
    ninja = require_mapping(toolchains["ninja"], "toolchains.ninja")
    require_exact_keys(
        ninja,
        {
            "version",
            "versionOutput",
            "executable",
            "executableSha256",
            "lockEntry",
            "installation",
        },
        "toolchains.ninja",
    )
    require(
        ninja
        == {
            "version": EXPECTED_NINJA,
            "versionOutput": EXPECTED_NINJA,
            "executable": ".toolchains/ninja-1.13.2-win/ninja.exe",
            "executableSha256": (
                EXPECTED_NINJA_LOCK_ENTRY["executableSha256"]
            ),
            "lockEntry": EXPECTED_NINJA_LOCK_ENTRY,
            "installation": {
                "archive": {
                    "path": (
                        ".toolchains/downloads/"
                        + EXPECTED_NINJA_LOCK_ENTRY["archiveFile"]
                    ),
                    "sha256": EXPECTED_NINJA_LOCK_ENTRY["sha256"],
                },
                "payload": EXPECTED_NINJA_LOCK_ENTRY["payload"],
                "installationRoot": ".toolchains/ninja-1.13.2-win",
            },
        },
        "Ninja evidence does not match the exact pin",
    )
    hermetic = require_mapping(
        toolchains["hermeticEnvironment"],
        "toolchains.hermeticEnvironment",
    )
    require_exact_keys(
        hermetic,
        {"forbiddenNamesAbsent", "canonicalVariables"},
        "toolchains.hermeticEnvironment",
    )
    require(
        hermetic["forbiddenNamesAbsent"]
        == list(FORBIDDEN_BUILD_ENVIRONMENT_NAMES),
        "hermetic environment forbidden-name evidence drifted",
    )
    canonical = require_mapping(
        hermetic["canonicalVariables"],
        "toolchains.hermeticEnvironment.canonicalVariables",
    )
    require(
        canonical
        == {
            "EMSDK": ".toolchains/emsdk-4.0.7",
            "EMSDK_NODE": EXPECTED_EMSDK_NODE,
            "EMSDK_PYTHON": EXPECTED_EMSDK_PYTHON,
            "EMSCRIPTEN_ROOT": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten"
            ),
            "EM_CONFIG": ".toolchains/emsdk-4.0.7/.emscripten",
            "EM_CACHE": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten/cache"
            ),
            "VCPKG_ROOT": (
                f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}"
            ),
            "VCPKG_DEFAULT_BINARY_CACHE": ".wasm-vcpkg/bincache",
            "EMSCRIPTEN_VERSION": EXPECTED_EMSCRIPTEN,
            "VCPKG_DISABLE_METRICS": "1",
            "CMAKE_NINJA_FORCE_RESPONSE_FILE": "1",
            "PYTHONDONTWRITEBYTECODE": "1",
        },
        "hermetic environment canonical evidence drifted",
    )


def validate_build_freshness_evidence(value: Any) -> None:
    freshness = require_mapping(value, "buildFreshness")
    require_exact_keys(
        freshness,
        {
            "target",
            "command",
            "output",
            "outputSha256",
            "sourceInputs",
            "artifactBinding",
        },
        "buildFreshness",
    )
    require(
        freshness["target"] == "RhythmGameWasmProbe",
        "buildFreshness target drifted",
    )
    expected_command = [
        ".toolchains/ninja-1.13.2-win/ninja.exe",
        "-C",
        "tools/wasm-probe/build/wasm-release",
        "-n",
        "-v",
        "RhythmGameWasmProbe",
    ]
    require(
        freshness["command"] == expected_command,
        "buildFreshness command is not the exact pinned Ninja dry-run",
    )
    output = freshness["output"]
    require(
        isinstance(output, list)
        and len(output) == 2
        and all(type(line) is str for line in output)
        and output[0].startswith("ninja: Entering directory ")
        and output[1] == "ninja: no work to do.",
        "buildFreshness output is not the exact no-op proof",
    )
    require_sha256_value(
        freshness["outputSha256"],
        "buildFreshness.outputSha256",
    )
    require(
        freshness["outputSha256"]
        == sha256_text("\n".join(output) + "\n"),
        "buildFreshness output digest does not match output",
    )
    source_inputs = require_mapping(
        freshness["sourceInputs"],
        "buildFreshness.sourceInputs",
    )
    require_exact_keys(
        source_inputs,
        {
            "algorithm",
            "tracking",
            "manifest",
            "manifestSha256",
            "count",
            "paths",
            "aggregateSha256",
        },
        "buildFreshness.sourceInputs",
    )
    require(
        source_inputs["algorithm"]
        == "sha256-manifest-path-equals-digest-lf-v1"
        and source_inputs["tracking"] == "git-index"
        and source_inputs["manifest"]
        == "tools/wasm-probe/input-manifest.txt",
        "buildFreshness source input contract drifted",
    )
    require_sha256_value(
        source_inputs["manifestSha256"],
        "buildFreshness.sourceInputs.manifestSha256",
    )
    paths = source_inputs["paths"]
    require(
        isinstance(paths, list)
        and paths
        and all(type(path) is str and path for path in paths)
        and len(paths) == len(set(paths))
        and paths == sorted(paths, key=str.casefold),
        "buildFreshness source input paths must be unique and sorted",
    )
    require_exact_type(
        source_inputs["count"],
        int,
        "buildFreshness.sourceInputs.count",
    )
    require(
        source_inputs["count"] == len(paths),
        "buildFreshness source input count does not match paths",
    )
    require_sha256_value(
        source_inputs["aggregateSha256"],
        "buildFreshness.sourceInputs.aggregateSha256",
    )
    binding = require_mapping(
        freshness["artifactBinding"],
        "buildFreshness.artifactBinding",
    )
    require_exact_keys(
        binding,
        {
            "generatedSource",
            "artifact",
            "configuredAggregateSha256",
            "markerSha256",
        },
        "buildFreshness.artifactBinding",
    )
    require(
        binding["generatedSource"]
        == (
            "tools/wasm-probe/build/wasm-release/generated/"
            "ProbeInputDigest.cpp"
        )
        and binding["artifact"]
        == "tools/wasm-probe/build/wasm-release/RhythmGameWasmProbe.wasm"
        and binding["configuredAggregateSha256"]
        == source_inputs["aggregateSha256"],
        "buildFreshness artifact binding drifted",
    )
    require_sha256_value(
        binding["configuredAggregateSha256"],
        "buildFreshness.artifactBinding.configuredAggregateSha256",
    )
    require_sha256_value(
        binding["markerSha256"],
        "buildFreshness.artifactBinding.markerSha256",
    )
    require(
        binding["markerSha256"]
        == sha256_text(
            "RG_WASM_PROBE_INPUT_SHA256="
            + binding["configuredAggregateSha256"]
        ),
        "buildFreshness artifact marker digest drifted",
    )


def validate_cmake_evidence(value: Any) -> None:
    cmake = require_mapping(value, "cmakeBuild")
    require_exact_keys(
        cmake,
        {
            "generator",
            "makeProgram",
            "chainloadToolchain",
            "outerProbeCMakeCommand",
            "qtBaseTargetCMakeCommand",
            "probeCompiler",
            "probeCompilerVersion",
            "qtTargetCCompiler",
            "qtTargetCxxCompiler",
            "qtTargetCompilerVersion",
        },
        "cmakeBuild",
    )
    require(
        cmake
        == {
            "generator": "Ninja",
            "makeProgram": ".toolchains/ninja-1.13.2-win/ninja.exe",
            "chainloadToolchain": (
                "cmake/toolchains/vcpkg-emscripten.cmake"
            ),
            "outerProbeCMakeCommand": (
                ".toolchains/cmake-4.2.3-windows-x86_64/bin/cmake.exe"
            ),
            "qtBaseTargetCMakeCommand": (
                ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows/"
                "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
            ),
            "probeCompiler": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.bat"
            ),
            "probeCompilerVersion": "21.0.0",
            "qtTargetCCompiler": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten/emcc.bat"
            ),
            "qtTargetCxxCompiler": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.bat"
            ),
            "qtTargetCompilerVersion": "21.0.0",
        },
        "CMake build evidence does not match the exact configuration",
    )


def validate_cmake_role_cross_fields(
    toolchain_value: Any,
    cmake_build_value: Any,
    declarative_value: Any,
) -> None:
    toolchains = require_mapping(toolchain_value, "toolchains")
    outer = require_mapping(
        toolchains["outerProbeCMake"],
        "toolchains.outerProbeCMake",
    )
    port = require_mapping(
        toolchains["vcpkgPortBuildCMake"],
        "toolchains.vcpkgPortBuildCMake",
    )
    cmake_build = require_mapping(cmake_build_value, "cmakeBuild")
    declarative = require_mapping(
        declarative_value,
        "qtDeclarativeCacheProvenance",
    )
    target = require_mapping(
        declarative["target"],
        "qtDeclarativeCacheProvenance.target",
    )
    host = require_mapping(
        declarative["host"],
        "qtDeclarativeCacheProvenance.host",
    )

    outer_executable = outer.get("executable")
    port_executable = port.get("executable")
    require(
        outer.get("version") == EXPECTED_OUTER_CMAKE
        and port.get("version") == EXPECTED_VCPKG_PORT_CMAKE
        and outer_executable != port_executable,
        "CMake role identities were flattened or drifted",
    )
    require(
        cmake_build.get("outerProbeCMakeCommand") == outer_executable,
        "outer probe CMake role does not match its cache command",
    )
    require(
        cmake_build.get("qtBaseTargetCMakeCommand") == port_executable,
        "QtBase target CMake role does not match its cache command",
    )
    for label, record in (("target", target), ("host", host)):
        require(
            record.get("cmakeCommand") == port_executable,
            f"QtDeclarative {label} CMake role does not match its "
            "cache command",
        )


def validate_qt_evidence(value: Any) -> None:
    qt = require_mapping(value, "qt")
    require_exact_keys(
        qt,
        {
            "version",
            "target",
            "host",
            "targetTripletEnvironmentPassthrough",
        },
        "qt",
    )
    require(qt["version"] == EXPECTED_QT, "Qt version drift")
    require(
        qt["targetTripletEnvironmentPassthrough"]
        == list(EXPECTED_TARGET_ENV_PASSTHROUGH),
        "Qt target triplet environment passthrough drifted",
    )
    target = require_mapping(qt["target"], "qt.target")
    require_exact_keys(
        target,
        {
            "triplet",
            "version",
            "linkage",
            "staticArchiveCount",
            "sharedLibraryCount",
            "requiredPorts",
            "requiredModules",
        },
        "qt.target",
    )
    require(
        target["triplet"] == TARGET_TRIPLET
        and target["version"] == EXPECTED_QT
        and target["linkage"] == "static"
        and target["sharedLibraryCount"] == 0,
        "Qt target identity/linkage drift",
    )
    require_positive_int(
        target["staticArchiveCount"],
        "qt.target.staticArchiveCount",
    )
    require_exact_type(
        target["sharedLibraryCount"],
        int,
        "qt.target.sharedLibraryCount",
    )
    ports = require_mapping(target["requiredPorts"], "qt.target.requiredPorts")
    require_exact_keys(ports, REQUIRED_TARGET_PORTS, "qt.target.requiredPorts")
    for port, raw_record in ports.items():
        record = require_mapping(
            raw_record,
            f"qt.target.requiredPorts.{port}",
        )
        require(
            set(record) in (
                {"Version", "Abi"},
                {"Version", "Port-Version", "Abi"},
            ),
            f"qt.target.requiredPorts.{port} keys drifted",
        )
        require(
            record["Version"] == EXPECTED_QT
            and type(record["Abi"]) is str
            and bool(record["Abi"]),
            f"qt.target.requiredPorts.{port} identity drifted",
        )
    require(
        ports["qtbase"].get("Port-Version") == "2",
        "QtBase overlay port-version is not 2",
    )
    require(
        ports["qtdeclarative"].get("Port-Version") == "1",
        "QtDeclarative installed Port-Version is not 1",
    )
    modules = require_mapping(
        target["requiredModules"],
        "qt.target.requiredModules",
    )
    require_exact_keys(
        modules,
        REQUIRED_TARGET_QT_MODULES,
        "qt.target.requiredModules",
    )
    require(
        all(type(path) is str and path for path in modules.values()),
        "Qt target module paths must be non-empty strings",
    )

    host = require_mapping(qt["host"], "qt.host")
    require_exact_keys(
        host,
        {
            "triplet",
            "version",
            "linkage",
            "qtDeclarativePortVersion",
            "qtCoreDllCount",
            "tools",
        },
        "qt.host",
    )
    require(
        host["triplet"] == HOST_TRIPLET
        and host["version"] == EXPECTED_QT
        and host["linkage"] == "dynamic",
        "Qt host identity/linkage drift",
    )
    require(
        host["qtDeclarativePortVersion"] == "1",
        "host QtDeclarative installed Port-Version is not 1",
    )
    require_positive_int(host["qtCoreDllCount"], "qt.host.qtCoreDllCount")
    tools = require_mapping(host["tools"], "qt.host.tools")
    require_exact_keys(tools, HOST_TOOL_VERSION_LINES, "qt.host.tools")
    for name, raw_tool in tools.items():
        tool = require_mapping(raw_tool, f"qt.host.tools.{name}")
        require_exact_keys(
            tool,
            {"path", "versionOutput"},
            f"qt.host.tools.{name}",
        )
        require(
            type(tool["path"]) is str
            and bool(tool["path"])
            and tool["versionOutput"] == HOST_TOOL_VERSION_LINES[name],
            f"qt.host.tools.{name} identity drifted",
        )


def validate_qtdeclarative_cache_evidence(value: Any) -> None:
    provenance = require_mapping(
        value,
        "featuresAndAutogen.qtDeclarativeCacheProvenance",
    )
    require_exact_keys(
        provenance,
        {"target", "host"},
        "qtDeclarativeCacheProvenance",
    )
    common_keys = {
        "buildDirectory",
        "sourceDirectory",
        "targetTriplet",
        "generator",
        "cmakeCommand",
        "installPrefix",
        "installedRoot",
        "qtPackagePrefix",
        "qtHostPath",
        "toolchain",
        "compiler",
    }
    expected_toolchain = (
        f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}/"
        "scripts/buildsystems/vcpkg.cmake"
    )
    expected_records = {
        "target": {
            "buildDirectory": (
                f".wb/qtdeclarative/{TARGET_TRIPLET}-rel"
            ),
            "targetTriplet": TARGET_TRIPLET,
            "cmakeCommand": (
                ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows/"
                "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
            ),
            "installPrefix": (
                ".wasm-vcpkg/packages/"
                f"qtdeclarative_{TARGET_TRIPLET}"
            ),
            "installedRoot": ".wasm-vcpkg/installed",
            "qtPackagePrefix": (
                f".wasm-vcpkg/installed/{TARGET_TRIPLET}/share/Qt6"
            ),
            "qtHostPath": (
                f".wasm-vcpkg/installed/{HOST_TRIPLET}"
            ),
            "compiler": {
                "path": (
                    ".toolchains/emsdk-4.0.7/upstream/emscripten/"
                    "em++.bat"
                ),
                "pathAuthenticated": True,
                "id": "Clang",
                "version": "21.0.0",
                "frontendVariant": "GNU",
                "architecture": "wasm32",
                "platform": "",
            },
        },
        "host": {
            "buildDirectory": (
                f".wb/qtdeclarative/{HOST_TRIPLET}-rel"
            ),
            "targetTriplet": HOST_TRIPLET,
            "cmakeCommand": (
                ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows/"
                "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
            ),
            "installPrefix": (
                ".wasm-vcpkg/packages/"
                f"qtdeclarative_{HOST_TRIPLET}"
            ),
            "installedRoot": ".wasm-vcpkg/installed",
            "qtPackagePrefix": (
                f".wasm-vcpkg/installed/{HOST_TRIPLET}/share/Qt6"
            ),
            "qtHostPath": "",
            "compiler": {
                "basename": "cl.exe",
                "toolsetRelativePath": (
                    EXPECTED_HOST_COMPILER_CONTRACT[
                        "toolsetRelativePath"
                    ]
                ),
                "pathAuthenticated": True,
                "executableSha256": (
                    EXPECTED_HOST_COMPILER_CONTRACT[
                        "executableSha256"
                    ]
                ),
                "id": "MSVC",
                "version": (
                    EXPECTED_HOST_COMPILER_CONTRACT[
                        "cmakeCompilerVersion"
                    ]
                ),
                "frontendVariant": "MSVC",
                "architecture": "x64",
                "platform": "Windows",
            },
        },
    }
    source_directories: list[str] = []
    for label, expected in expected_records.items():
        record = require_mapping(
            provenance[label],
            f"qtDeclarativeCacheProvenance.{label}",
        )
        require_exact_keys(
            record,
            common_keys,
            f"qtDeclarativeCacheProvenance.{label}",
        )
        require(
            record["generator"] == "Ninja"
            and record["toolchain"] == expected_toolchain,
            f"QtDeclarative {label} generator/toolchain drifted",
        )
        for name in (
            "buildDirectory",
            "targetTriplet",
            "cmakeCommand",
            "installPrefix",
            "installedRoot",
            "qtPackagePrefix",
            "qtHostPath",
        ):
            require(
                record[name] == expected[name],
                f"QtDeclarative {label} {name} drifted",
            )
        source = record["sourceDirectory"]
        require(
            type(source) is str
            and re.fullmatch(
                r"\.wb/qtdeclarative/src/"
                r"here-src-\d+-[0-9a-f]+\.clean",
                source,
            )
            is not None,
            f"QtDeclarative {label} source directory evidence drifted",
        )
        source_directories.append(source)
        compiler = require_mapping(
            record["compiler"],
            f"qtDeclarativeCacheProvenance.{label}.compiler",
        )
        require_exact_keys(
            compiler,
            set(expected["compiler"]),
            f"QtDeclarative {label} compiler",
        )
        require(
            compiler == expected["compiler"],
            f"QtDeclarative {label} compiler evidence drifted",
        )
    require(
        source_directories[0] == source_directories[1],
        "QtDeclarative target/host source evidence differs",
    )


def validate_features_evidence(value: Any) -> None:
    features = require_mapping(value, "featuresAndAutogen")
    require_exact_keys(
        features,
        {
            "qtFeatures",
            "emscriptenVersionCheckRetained",
            "qtDeclarativeCacheProvenance",
            "emscriptenSdkContract",
            "quickControlsStyles",
            "autogen",
        },
        "featuresAndAutogen",
    )
    require(
        features["qtFeatures"]
        == {
            "thread": "ON",
            "wasm_exceptions": "ON",
            "wasm_jspi": "ON",
            "wasm_simd128": "OFF",
        },
        "Qt Wasm feature evidence drifted",
    )
    require(
        features["emscriptenVersionCheckRetained"] is True,
        "Qt Emscripten version check must be retained",
    )
    validate_qtdeclarative_cache_evidence(
        features["qtDeclarativeCacheProvenance"]
    )
    sdk = require_mapping(
        features["emscriptenSdkContract"],
        "featuresAndAutogen.emscriptenSdkContract",
    )
    require_exact_keys(
        sdk,
        {
            "qtbaseCache",
            "installedQconfigHeaderVersion",
            "installedQconfigPriVersion",
            "installedHelper",
            "mismatchedConsumer",
        },
        "featuresAndAutogen.emscriptenSdkContract",
    )
    require(
        sdk["qtbaseCache"]
        == {
            "EMCC_VERSION": EXPECTED_EMSCRIPTEN,
            "QT_AUTODETECT_WASM_IS_DONE": "TRUE",
            "QT_EMCC_RECOMMENDED_VERSION": EXPECTED_EMSCRIPTEN,
            "QT_QMAKE_TARGET_MKSPEC": "wasm-emscripten",
        }
        and sdk["installedQconfigHeaderVersion"] == EXPECTED_EMSCRIPTEN
        and sdk["installedQconfigPriVersion"] == EXPECTED_EMSCRIPTEN,
        "Qt installed Emscripten SDK identity drifted",
    )
    helper = require_mapping(
        sdk["installedHelper"],
        "featuresAndAutogen.emscriptenSdkContract.installedHelper",
    )
    require_exact_keys(
        helper,
        {
            "path",
            "sha256",
            "layoutAwareInstalledHeaders",
            "buildTreeFallback",
            "defaultInstalledFallback",
        },
        "featuresAndAutogen.emscriptenSdkContract.installedHelper",
    )
    require(
        helper["path"]
        == (
            ".wasm-vcpkg/installed/wasm32-emscripten-rg/share/Qt6/"
            "QtPublicWasmToolchainHelpers.cmake"
        )
        and helper["sha256"] == EXPECTED_INSTALLED_WASM_HELPER_SHA256
        and helper["layoutAwareInstalledHeaders"] is True
        and helper["buildTreeFallback"] is True
        and helper["defaultInstalledFallback"] is True,
        "installed Qt Wasm helper contract drifted",
    )
    mismatch = require_mapping(
        sdk["mismatchedConsumer"],
        "featuresAndAutogen.emscriptenSdkContract.mismatchedConsumer",
    )
    require(
        mismatch
        == {
            "builtVersion": EXPECTED_EMSCRIPTEN,
            "activeVersion": "9.9.9",
            "rejectedBeforeSentinel": True,
        },
        "installed-consumer mismatch rejection evidence drifted",
    )
    require(
        features["quickControlsStyles"]
        == {
            "target": {"FluentWinUI3": "ON", "Universal": "ON"},
            "host": {"FluentWinUI3": "OFF", "Universal": "OFF"},
        },
        "Quick Controls style evidence drifted",
    )
    autogen = require_mapping(
        features["autogen"],
        "featuresAndAutogen.autogen",
    )
    require_exact_keys(
        autogen,
        {
            "commandLineLengthMax",
            "compilerPredefinesEnabled",
            "compiler",
            "compilerVersion",
            "metadataCount",
            "compilerPredefinesCommandCount",
            "generatedPredefinesFileCount",
            "generatedPredefinesAggregateSha256",
        },
        "featuresAndAutogen.autogen",
    )
    require(
        autogen["commandLineLengthMax"] == 4096
        and autogen["compilerPredefinesEnabled"] is True
        and autogen["compiler"]
        == ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.bat"
        and autogen["compilerVersion"] == "21.0.0",
        "AutoGen compiler/configuration evidence drifted",
    )
    for name in (
        "metadataCount",
        "compilerPredefinesCommandCount",
        "generatedPredefinesFileCount",
    ):
        require_positive_int(autogen[name], f"featuresAndAutogen.autogen.{name}")
    require(
        autogen["metadataCount"]
        == autogen["compilerPredefinesCommandCount"]
        == autogen["generatedPredefinesFileCount"],
        "AutoGen metadata, command, and unique output counts differ",
    )
    require_sha256_value(
        autogen["generatedPredefinesAggregateSha256"],
        "featuresAndAutogen.autogen.generatedPredefinesAggregateSha256",
    )


def validate_compile_evidence(value: Any) -> None:
    compile_commands = require_mapping(value, "compileCommands")
    require_exact_keys(
        compile_commands,
        {
            "targetDatabaseCount",
            "targetDatabases",
            "targetCommandCounts",
            "targetSettingCoverage",
            "targetPortCommandCounts",
            "forbiddenArgumentsAbsent",
            "exceptionBoundary",
        },
        "compileCommands",
    )
    expected_databases = [
        f".wb/{port}/{TARGET_TRIPLET}-rel/compile_commands.json"
        for port in EXPECTED_TARGET_COMPILE_PORTS
    ]
    require(
        compile_commands["targetDatabases"] == expected_databases
        and compile_commands["targetDatabaseCount"]
        == len(expected_databases),
        "target compile database evidence does not match exact layout",
    )
    totals = require_mapping(
        compile_commands["targetCommandCounts"],
        "compileCommands.targetCommandCounts",
    )
    require_exact_keys(totals, {"c", "cxx"}, "targetCommandCounts")
    require_positive_int(totals["c"], "targetCommandCounts.c")
    require_positive_int(totals["cxx"], "targetCommandCounts.cxx")
    coverage = require_mapping(
        compile_commands["targetSettingCoverage"],
        "compileCommands.targetSettingCoverage",
    )
    require_exact_keys(coverage, {"c", "cxx"}, "targetSettingCoverage")
    c_coverage = require_mapping(coverage["c"], "targetSettingCoverage.c")
    cxx_coverage = require_mapping(
        coverage["cxx"],
        "targetSettingCoverage.cxx",
    )
    require_exact_keys(c_coverage, C_COMPILE_SETTINGS, "targetSettingCoverage.c")
    require_exact_keys(
        cxx_coverage,
        CXX_COMPILE_SETTINGS,
        "targetSettingCoverage.cxx",
    )
    require(
        set(c_coverage.values()) == {totals["c"]}
        and set(cxx_coverage.values()) == {totals["cxx"]},
        "target setting coverage does not equal command totals",
    )
    ports = require_mapping(
        compile_commands["targetPortCommandCounts"],
        "compileCommands.targetPortCommandCounts",
    )
    require_exact_keys(
        ports,
        EXPECTED_TARGET_COMPILE_PORTS,
        "targetPortCommandCounts",
    )
    port_totals = {"c": 0, "cxx": 0}
    for port, raw_counts in ports.items():
        counts = require_mapping(raw_counts, f"targetPortCommandCounts.{port}")
        require_exact_keys(counts, {"c", "cxx"}, f"port counts {port}")
        for language in ("c", "cxx"):
            require_exact_type(
                counts[language],
                int,
                f"targetPortCommandCounts.{port}.{language}",
            )
            require(
                counts[language] >= 0,
                f"targetPortCommandCounts.{port}.{language} is negative",
            )
            port_totals[language] += counts[language]
    require(
        port_totals == totals,
        "per-port compile counts do not sum to target totals",
    )
    require(
        compile_commands["forbiddenArgumentsAbsent"]
        == list(FORBIDDEN_TARGET_ARGUMENTS),
        "target forbidden-argument audit drifted",
    )
    boundary = require_mapping(
        compile_commands["exceptionBoundary"],
        "compileCommands.exceptionBoundary",
    )
    require_exact_keys(
        boundary,
        {"ExceptionBoundary.cpp", "ProbeState.cpp"},
        "compileCommands.exceptionBoundary",
    )
    for source, raw_entry in boundary.items():
        entry = require_mapping(
            raw_entry,
            f"compileCommands.exceptionBoundary.{source}",
        )
        require_exact_keys(
            entry,
            {"settings", "effectiveSettings", "commandSha256"},
            f"exception boundary {source}",
        )
        require(
            entry["settings"] == list(CXX_COMPILE_SETTINGS)
            and entry["effectiveSettings"]
            == CXX_COMPILE_EMSCRIPTEN_SETTINGS,
            f"exception boundary {source} settings drifted",
        )
        require_sha256_value(
            entry["commandSha256"],
            f"exception boundary {source} commandSha256",
        )


def validate_application_link_evidence(value: Any) -> None:
    link = require_mapping(value, "applicationLink")
    require_exact_keys(
        link,
        {
            "compiler",
            "responseFile",
            "responseFileContentTemplate",
            "responseArgumentCount",
            "responseArgumentsSha256",
            "effectiveArgumentCount",
            "effectiveArgumentsSha256",
            "selectedNinjaRule",
            "settings",
            "forbiddenArgumentsAbsent",
            "effectiveSettings",
            "settingOccurrences",
            "literalAsyncifyConfigured",
            "staticExceptionArchive",
            "staticExceptionArchiveLinked",
            "commandSha256",
            "linkLibrariesSha256",
        },
        "applicationLink",
    )
    require(
        link["compiler"]
        == ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.bat"
        and link["responseFile"]
        == "CMakeFiles/RhythmGameWasmProbe.rsp"
        and link["responseFileContentTemplate"]
        == "$in $LINK_PATH $LINK_LIBRARIES"
        and link["selectedNinjaRule"]
        == "CXX_EXECUTABLE_LINKER__RhythmGameWasmProbe_Release"
        and link["settings"] == list(APPLICATION_LINK_SETTINGS)
        and link["forbiddenArgumentsAbsent"]
        == list(FORBIDDEN_TARGET_ARGUMENTS)
        and link["effectiveSettings"] == APPLICATION_EMSCRIPTEN_SETTINGS
        and link["literalAsyncifyConfigured"] is False
        and link["staticExceptionArchive"]
        == "libWasmProbeExceptionBoundary.a"
        and link["staticExceptionArchiveLinked"] is True,
        "application link contract drifted",
    )
    require_positive_int(
        link["responseArgumentCount"],
        "applicationLink.responseArgumentCount",
    )
    require_positive_int(
        link["effectiveArgumentCount"],
        "applicationLink.effectiveArgumentCount",
    )
    require(
        link["effectiveArgumentCount"] > link["responseArgumentCount"],
        "application link effective argument count must include outer args",
    )
    occurrences = require_mapping(
        link["settingOccurrences"],
        "applicationLink.settingOccurrences",
    )
    require_exact_keys(
        occurrences,
        APPLICATION_EMSCRIPTEN_SETTINGS,
        "applicationLink.settingOccurrences",
    )
    for name, count in occurrences.items():
        require_positive_int(count, f"applicationLink occurrence {name}")
    require_sha256_value(link["commandSha256"], "applicationLink.commandSha256")
    require_sha256_value(
        link["linkLibrariesSha256"],
        "applicationLink.linkLibrariesSha256",
    )
    require_sha256_value(
        link["responseArgumentsSha256"],
        "applicationLink.responseArgumentsSha256",
    )
    require_sha256_value(
        link["effectiveArgumentsSha256"],
        "applicationLink.effectiveArgumentsSha256",
    )


def validate_overlay_evidence(value: Any) -> None:
    overlays = require_mapping(value, "overlays")
    require_exact_keys(
        overlays,
        {"qtbase", "qtdeclarative", "reviewedDelta"},
        "overlays",
    )
    expected = {
        "qtbase": {
            "baselineTree": EXPECTED_QTBASE_TREE,
            "modifiedBaselineFiles": ["portfile.cmake", "vcpkg.json"],
            "lineEndingOnlyBaselineFiles": [],
            "addedFiles": ["restore-wasm-version-check.patch"],
        },
        "qtdeclarative": {
            "baselineTree": EXPECTED_QTDECLARATIVE_TREE,
            "modifiedBaselineFiles": ["portfile.cmake", "vcpkg.json"],
            "lineEndingOnlyBaselineFiles": ["port.data.cmake"],
            "addedFiles": [
                "24205cd-qquickwindow-child-window-stacking.patch"
            ],
        },
    }
    for port, contract in expected.items():
        result = require_mapping(overlays[port], f"overlays.{port}")
        require_exact_keys(
            result,
            {
                "baselineTree",
                "fileCount",
                "modifiedBaselineFiles",
                "lineEndingOnlyBaselineFiles",
                "addedFiles",
                "fileSha256",
                "aggregateSha256",
            },
            f"overlays.{port}",
        )
        for key, expected_value in contract.items():
            require(
                result[key] == expected_value,
                f"overlays.{port}.{key} drifted",
            )
        require_positive_int(result["fileCount"], f"overlays.{port}.fileCount")
        file_hashes = require_mapping(
            result["fileSha256"],
            f"overlays.{port}.fileSha256",
        )
        require(
            len(file_hashes) == result["fileCount"],
            f"overlays.{port} file hash count drifted",
        )
        for relative, digest in file_hashes.items():
            require(
                type(relative) is str and bool(relative),
                f"overlays.{port} has invalid file path",
            )
            require_sha256_value(
                digest,
                f"overlays.{port}.fileSha256.{relative}",
            )
        require_sha256_value(
            result["aggregateSha256"],
            f"overlays.{port}.aggregateSha256",
        )
    for label, digest in EXPECTED_OVERLAY_SHA256.items():
        port, relative = label.split("/", 1)
        require(
            overlays[port]["fileSha256"].get(relative) == digest,
            f"overlays.{label} reviewed digest drifted",
        )
    require(
        overlays["reviewedDelta"]
        == {
            "qtbase": [
                "restore Emscripten version-helper include and check",
                "seed wasm-emscripten target mkspec before project",
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
        "reviewed overlay delta evidence drifted",
    )


def validate_artifact_evidence(value: Any) -> None:
    artifacts = require_mapping(value, "artifacts")
    require_exact_keys(
        artifacts,
        {
            "deploymentFiles",
            "recognizedWebDeployables",
            "externalWorkerArtifacts",
            "pthreadWorkerEmbeddedInMainJavaScript",
            "pthreadBootstrapMarkers",
            "shaderResourceAlias",
            "shaderResourceAliases",
            "files",
        },
        "artifacts",
    )
    require(
        artifacts["deploymentFiles"] == list(DEPLOYMENT_ARTIFACTS)
        and artifacts["recognizedWebDeployables"]
        == list(DEPLOYMENT_ARTIFACTS)
        and artifacts["externalWorkerArtifacts"]
        == [
            "RhythmGameWasmProbe.aw.js",
            "RhythmGameWasmProbe.ww.js",
        ]
        and artifacts["pthreadWorkerEmbeddedInMainJavaScript"] is True
        and artifacts["pthreadBootstrapMarkers"]
        == [
            "PThread.allocateUnusedWorker",
            "new Worker(pthreadMainJs",
            "ENVIRONMENT_IS_PTHREAD",
        ]
        and artifacts["shaderResourceAlias"] == SHADER_RESOURCE_PATH
        and artifacts["shaderResourceAliases"] == [SHADER_RESOURCE_PATH],
        "deployment artifact contract drifted",
    )
    files = require_mapping(artifacts["files"], "artifacts.files")
    expected_files = set(DEPLOYMENT_ARTIFACTS) | {
        "libWasmProbeExceptionBoundary.a",
        "compile_commands.json",
    }
    require_exact_keys(files, expected_files, "artifacts.files")
    for name, raw_file in files.items():
        file = require_mapping(raw_file, f"artifacts.files.{name}")
        require_exact_keys(file, {"bytes", "sha256"}, f"artifact {name}")
        require_positive_int(file["bytes"], f"artifact {name} bytes")
        require_sha256_value(file["sha256"], f"artifact {name} sha256")


def validate_evidence_for_write(evidence: Mapping[str, Any]) -> None:
    require(isinstance(evidence, Mapping), "evidence must be a mapping")
    require_no_absolute_path_strings(evidence)
    require_exact_keys(
        evidence,
        {
            "schemaVersion",
            "gate",
            "scope",
            "technicalProbePassed",
            "gate1aPassed",
            "gate0Satisfied",
            "formalGate1EntryAuthorized",
            "gate1Passed",
            "toolchains",
            "buildFreshness",
            "cmakeBuild",
            "qt",
            "featuresAndAutogen",
            "compileCommands",
            "applicationLink",
            "overlays",
            "artifacts",
            "unprovenUntilGate1B",
        },
        "evidence top-level",
    )
    require_exact_type(evidence["schemaVersion"], int, "schemaVersion")
    require(
        evidence["schemaVersion"] == 2
        and evidence["gate"] == "1A"
        and evidence["scope"] == GATE_SCOPE,
        "Gate 1A schema identity drifted",
    )
    for name, expected in {
        "technicalProbePassed": True,
        "gate1aPassed": True,
        "gate0Satisfied": False,
        "formalGate1EntryAuthorized": False,
        "gate1Passed": False,
    }.items():
        require_exact_type(evidence[name], bool, name)
        require(evidence[name] is expected, f"{name} must be {expected}")
    unproven = evidence["unprovenUntilGate1B"]
    require(
        isinstance(unproven, list)
        and unproven == list(GATE_1B_LIMITATIONS)
        and len(unproven) == len(set(unproven)),
        "Gate 1B limitations must be the exact reviewed set",
    )

    validate_toolchain_evidence(evidence["toolchains"])
    validate_build_freshness_evidence(evidence["buildFreshness"])
    validate_cmake_evidence(evidence["cmakeBuild"])
    validate_qt_evidence(evidence["qt"])
    validate_features_evidence(evidence["featuresAndAutogen"])
    validate_compile_evidence(evidence["compileCommands"])
    validate_application_link_evidence(evidence["applicationLink"])
    validate_overlay_evidence(evidence["overlays"])
    validate_artifact_evidence(evidence["artifacts"])
    features = require_mapping(
        evidence["featuresAndAutogen"],
        "featuresAndAutogen",
    )
    validate_cmake_role_cross_fields(
        evidence["toolchains"],
        evidence["cmakeBuild"],
        features["qtDeclarativeCacheProvenance"],
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


def invalidate_requested_output(output: Path) -> None:
    if output.is_symlink():
        output.unlink()
        return
    require(
        not output.is_dir(),
        f"requested evidence output is a directory: {output}",
    )
    if output.exists():
        output.unlink()


def generate_evidence(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
    output: Path,
) -> dict[str, Any]:
    invalidate_requested_output(output)
    evidence = build_evidence(repo, emsdk, vcpkg)
    write_evidence_atomic(output, evidence)
    return evidence


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
    output = Path(os.path.abspath(args.output))
    generate_evidence(
        repo,
        args.emsdk.resolve(),
        args.vcpkg.resolve(),
        output,
    )


if __name__ == "__main__":
    main()
