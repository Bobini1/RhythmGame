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
import sys
import tempfile
import xml.etree.ElementTree as ElementTree
import zipfile
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence


EXPECTED_EMSCRIPTEN = "4.0.7"
EXPECTED_EMSDK_COMMIT = "c69d433d8509c5c64564c2f0d054bf102a5cf67e"
EXPECTED_SOURCE_DATE_EPOCH = 1782488244
EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY = {
    "sourceDateEpoch": EXPECTED_SOURCE_DATE_EPOCH,
    "derivation": "vcpkg-baseline-source-archive-root-entry-utc",
    "vcpkgMaxConcurrency": 8,
}
QUALIFICATION_CLOSURE_ALGORITHM = (
    "sha256-logical-null-bytes-null-digest-lf-v1"
)
QUALIFICATION_IDENTITY_FIELDS = (
    "algorithm",
    "fileCount",
    "totalBytes",
    "inventorySha256",
    "aggregateSha256",
)


def compiler_qualification_identity(
    qualification: Mapping[str, Any],
) -> dict[str, Any]:
    return {
        field: qualification[field]
        for field in QUALIFICATION_IDENTITY_FIELDS
    }
COMPILE_DEPENDENCY_ALGORITHM = (
    "sha256-compile-dependency-files-json-v1"
)
SELECTED_LINK_IDENTITY_ALGORITHM = (
    "sha256-selected-link-argv-files-compile-qualification-json-v2"
)
EXPECTED_BUILD_CONTROL_PATHS = (
    ".qt/.qmlls.build.ini.part",
    ".qt/bin/qt_setup_tool_path.bat",
    ".qt/qml_imports/RhythmGameWasmProbe_conf.cmake",
    ".qt/rcc/qmake_RhythmGame_WasmProbe.qrc",
    ".qt/rcc/RhythmGameWasmProbe_raw_qml_0.qrc",
    ".qt/rcc/RhythmGameWasmProbe_raw_qml_0_extra_qmldirs.qrc",
    ".qt/rcc/wasm_probe_shaders.qrc",
    ".qt/RhythmGameWasmProbe_qml.cmake",
    ".qt/RhythmGameWasmProbe_res.cmake",
    ".rcc/qmlcache/RhythmGameWasmProbe_qml_loader_file_list.rsp",
    "build.ninja",
    "CMakeCache.txt",
    "CMakeFiles/4.2.3/CMakeCCompiler.cmake",
    "CMakeFiles/4.2.3/CMakeCXXCompiler.cmake",
    "CMakeFiles/4.2.3/CMakeSystem.cmake",
    (
        "CMakeFiles/RhythmGameWasmCLauncherProbe_autogen.dir/"
        "AutogenInfo.json"
    ),
    "CMakeFiles/RhythmGameWasmProbe.dir/post-build.bat",
    "CMakeFiles/RhythmGameWasmProbe_autogen.dir/AutogenInfo.json",
    "CMakeFiles/rules.ninja",
    "CMakeFiles/VerifyGlobs.cmake",
    (
        "CMakeFiles/WasmProbeExceptionBoundary_autogen.dir/"
        "AutogenInfo.json"
    ),
    "compile_commands.json",
    "generated/ProbeDependencyDigest.cpp",
    "generated/ProbeInputDigest.cpp",
    "qmltypes/RhythmGameWasmProbe_foreign_types.txt",
    "RhythmGame/WasmProbe/qml/qmldir",
    "RhythmGame/WasmProbe/qmldir",
)
QUALIFICATION_COMMAND_ONLY_BUILD_CONTROL_PATHS = (
    ".qt/.qmlls.build.ini.part",
    ".qt/bin/qt_setup_tool_path.bat",
    ".qt/RhythmGameWasmProbe_qml.cmake",
    ".qt/RhythmGameWasmProbe_res.cmake",
    "CMakeFiles/RhythmGameWasmProbe.dir/post-build.bat",
)
QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS = (
    (
        "CMakeFiles/RhythmGameWasmCLauncherProbe_autogen.dir/"
        "AutogenUsed.txt"
    ),
    (
        "CMakeFiles/RhythmGameWasmCLauncherProbe_autogen.dir/"
        "ParseCache.txt"
    ),
    "CMakeFiles/RhythmGameWasmProbe_autogen.dir/AutogenUsed.txt",
    "CMakeFiles/RhythmGameWasmProbe_autogen.dir/ParseCache.txt",
    (
        "CMakeFiles/WasmProbeExceptionBoundary_autogen.dir/"
        "AutogenUsed.txt"
    ),
    (
        "CMakeFiles/WasmProbeExceptionBoundary_autogen.dir/"
        "ParseCache.txt"
    ),
)
EXPECTED_EMSDK_PYTHON = (
    ".toolchains/emsdk-4.0.7/python/3.9.2-nuget_64bit/python.exe"
)
EXPECTED_EMSDK_NODE = (
    ".toolchains/emsdk-4.0.7/node/20.18.0_64bit/bin/node.exe"
)
EXPECTED_GATE_TOOLS_LOCK_ENTRY = {
    "adapterSha256": (
        "e2e241adc9d47c9d3f2d50dd679a7b67ec587e6c0515ee1f1b0d64c57e2515b8"
    ),
    "responseAuditorSha256": (
        "18bbb2f3791715035dd7a0f644290173a1dc98a3e816d9621ada24079b59466b"
    ),
}
EXPECTED_EMSDK_SOURCE_ARCHIVE = {
    "url": (
        "https://github.com/emscripten-core/emsdk/archive/"
        "c69d433d8509c5c64564c2f0d054bf102a5cf67e.zip"
    ),
    "archiveFile": (
        "emsdk-c69d433d8509c5c64564c2f0d054bf102a5cf67e.zip"
    ),
    "sha256": (
        "e0be07abfa84ada42b6d895ff751a825878c07140798ddf5f94774a085122ce3"
    ),
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "stripPrefix": (
            "emsdk-c69d433d8509c5c64564c2f0d054bf102a5cf67e"
        ),
        "fileCount": 134,
        "directoryCount": 13,
        "totalBytes": 1011834,
        "inventorySha256": (
            "ac2a983596b73ff45b2c67de5b1246006671ede5e907c2118ae9a5e6bec5e237"
        ),
        "directoryInventorySha256": (
            "babd394224ccaac5a946ae0a291fe53132fdd1fe558dfd31a6cd5c4db0d358dd"
        ),
        "aggregateSha256": (
            "9a06ab588de3663c38c9b7ddb19034d3479087af8844b456f47df039665f1a94"
        ),
    },
    "allowedRuntimePrefixes": [
        "downloads",
        "node/20.18.0_64bit",
        "python/3.9.2-nuget_64bit",
        "upstream/bin",
        "upstream/emscripten",
        "upstream/lib",
    ],
    "allowedRuntimeFiles": [
        ".emscripten",
        ".emscripten.old",
        "upstream/.emsdk_version",
        "upstream/emscripten_config",
    ],
}
EXPECTED_EMSDK_BOOTSTRAP_PYTHON = {
    "url": (
        "https://storage.googleapis.com/webassembly/"
        "emscripten-releases-builds/deps/"
        "python-3.9.2-4-amd64+pywin32.zip"
    ),
    "archiveFile": "python-3.9.2-4-amd64+pywin32.zip",
    "sha256": (
        "e47e5f00c8970cee28c228f100269aaad20f7fd1405e2bd4a6c9da33076830ac"
    ),
    "installationDirectory": "python/3.9.2-nuget_64bit",
    "executable": "python.exe",
    "executableSha256": (
        "786e68ded8af18f36274d78ea00ff11289c27107dd9f8fdd2f6b4732a3b8a2da"
    ),
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "stripPrefix": "",
        "fileCount": 1486,
        "directoryCount": 118,
        "totalBytes": 37105575,
        "inventorySha256": (
            "76f67170d0868c9de9b41da44b94e8e64952fe54f6afe9cb40113c084ca38fa8"
        ),
        "directoryInventorySha256": (
            "c5d479f28a943783e867b63ccad150b785cb20505be4c6f4dbe358cb8e0fc285"
        ),
        "aggregateSha256": (
            "97e984800db090cd44394fd54f45ebdf14bd6084a91a41d25496fb8d16ce09c1"
        ),
    },
    "allowedRuntimePrefixes": [],
    "allowedRuntimeFiles": [".emsdk_version"],
}
EXPECTED_EMSCRIPTEN_DRIVER_API = {
    "emccPySha256": (
        "2884a798ef2ad1c43f20ea1121c83ba18caec15158124b472be5494c3dc8c3f0"
    ),
    "emxxPySha256": (
        "017f735c953318fdaaa4af68c8d4bf13936a787ddbd5dad804d532b0223b6c2f"
    ),
    "emarLauncherSha256": (
        "f9b690c5abe642942e5d3dabb1f9f87603734c9f18b01dad2f3475120d2f33ef"
    ),
    "emarPySha256": (
        "ba2d6247890bcfa38c228562d025d4f9c96f659ea297881600adf09a66dbc202"
    ),
    "emranlibLauncherSha256": (
        "f9b690c5abe642942e5d3dabb1f9f87603734c9f18b01dad2f3475120d2f33ef"
    ),
    "emranlibPySha256": (
        "916dc7b3b80e51638977ffdabbd00ccee0fd7bc447cfcc8dc206201c75098501"
    ),
    "sharedPySha256": (
        "8d26ae87a03d55c99bf9a07f86c1fc56484d76a18f501962b5ec0f90cde8a8fe"
    ),
    "responseFilePySha256": (
        "815ec85ec4d2f0d471a648a375c2fbd39daf194ac9e557d7e14901f4cac0004f"
    ),
    "configPySha256": (
        "58294addc063d9ad954f767f8b28659d7c720218e40692b20e155a003192728d"
    ),
    "pythonImportClosure": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "fileCount": 225,
        "totalBytes": 3159759,
        "inventorySha256": (
            "acdbf9111c4779b62764eaa595a184179f7ffe8f46de07ea4303f635d1308477"
        ),
        "aggregateSha256": (
            "4a378899a0a3ad36e19f7f5e0170641fdb647d61aaf464c721b07d729abdf6f6"
        ),
    },
}
EXPECTED_EMSCRIPTEN_LOCK_ENTRY = {
    "version": EXPECTED_EMSCRIPTEN,
    "emsdkCommit": EXPECTED_EMSDK_COMMIT,
    "sourceArchive": EXPECTED_EMSDK_SOURCE_ARCHIVE,
    "bootstrapPython": EXPECTED_EMSDK_BOOTSTRAP_PYTHON,
    "releaseManifest": {
        "path": "emscripten-releases-tags.json",
        "sha256": (
            "30a7e6ee492f1e3e576a97d281d68a283e2d74c4bc65bee1d298d767d50d855e"
        ),
    },
    "releaseHash": "ef4e9cedeac3332e4738087567552063f4f250d3",
    "packageUrl": (
        "https://storage.googleapis.com/webassembly/"
        "emscripten-releases-builds/win/"
        "ef4e9cedeac3332e4738087567552063f4f250d3/"
        "wasm-binaries.zip"
    ),
    "bootstrapScript": "emsdk.py",
    "bootstrapScriptSha256": (
        "8b6da54e6c8d72605183036d412b2878d7ec9437533cd9db4dcc6b46fc687824"
    ),
    "cLauncher": "upstream/emscripten/emcc.bat",
    "cLauncherSha256": (
        "b61f25a114b9b93444de62bc31bf05d1b12b3986513c75f6b23ccc1711c6a634"
    ),
    "cxxLauncher": "upstream/emscripten/em++.bat",
    "cxxLauncherSha256": (
        "b61f25a114b9b93444de62bc31bf05d1b12b3986513c75f6b23ccc1711c6a634"
    ),
    "driverApi": EXPECTED_EMSCRIPTEN_DRIVER_API,
    "nodeExecutable": "node/20.18.0_64bit/bin/node.exe",
    "nodeExecutableSha256": (
        "35b7c95a379beb606f5798ed83081690df13190077630b234163c6607aa4cc94"
    ),
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
    "cache": {
        "directory": "emscripten-cache-4.0.7",
        "initializer": (
            "prewarm_emscripten_cache.py -> embuilder.py build SYSTEM"
        ),
        "prewarmCores": 4,
        "compilerPathPrefixMap": {
            "injection": "tracked-python-get_base_cflags-adapter",
            "flag": "-ffile-prefix-map",
            "systemLibsSha256": (
                "48c216b787a80270e6bcaac896a8e952"
                "ccc7978025fdee6c8d61f4138ce5c43d"
            ),
            "target": "/emsdk/cache",
        },
        "frozenEnvironment": "EM_FROZEN_CACHE=1",
        "volatileProducts": [
            "sanity.txt",
            "symbol_lists/*.json",
        ],
        "payload": {
            "algorithm": "sha256-path-null-digest-lf-v1",
            "fileCount": 1825,
            "directoryCount": 89,
            "totalBytes": 407765837,
            "inventorySha256": (
                "e0eb0b8e63f577bf3cb8eefa1a4f1eedd796f0127574093560a97c4938b7cb4a"
            ),
            "directoryInventorySha256": (
                "8e7094c96f2baf5c464d2cbef307ce059716c744be8d773a2528bbe5e1e33c07"
            ),
            "aggregateSha256": (
                "5eddb15053811f76fb148e45a4f0994f958a20046cbc205f7b36ed8a94330ea0"
            ),
        },
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
EXPECTED_VCPKG_EXECUTABLE_SHA256 = (
    "da75e3312ff6881c89f6171363eedb92933b0f79456cd6ee636316edef860ff7"
)
EXPECTED_VCPKG_BOOTSTRAP_LAUNCHER_SHA256 = (
    "8e8ec8a66db4f9fa69e51ca725a2aeec3c2a99e4ee0040f50358ffd8776f82b5"
)
EXPECTED_VCPKG_SOURCE_ARCHIVE = {
    "url": (
        "https://github.com/microsoft/vcpkg/archive/"
        "a0400024711b283056538ac19ced80b91a83c24c.zip"
    ),
    "archiveFile": (
        "vcpkg-a0400024711b283056538ac19ced80b91a83c24c.zip"
    ),
    "sha256": (
        "1a3d2abdd7ca6d479d7f1c2e437fd9765790c09613baa2631c87b1726f905c00"
    ),
    "payload": {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "stripPrefix": (
            "vcpkg-a0400024711b283056538ac19ced80b91a83c24c"
        ),
        "fileCount": 14073,
        "directoryCount": 3215,
        "totalBytes": 20252983,
        "inventorySha256": (
            "501493e641af8100034e7a74e03d506a8f2d65a8a01ba50a773265c38d22ca2b"
        ),
        "directoryInventorySha256": (
            "3f037e402ca926d9c233c6b5cdeb8c1c54ff0d92967b5c1bf38f88bc55fb6546"
        ),
        "aggregateSha256": (
            "9357531ef9ffe513b71dc4e46698cea4ab0da993c1f4780d97e920f712511d94"
        ),
    },
    "allowedRuntimePrefixes": [],
    "allowedRuntimeFiles": ["vcpkg.disable-metrics", "vcpkg.exe"],
}
EXPECTED_VCPKG_BOOTSTRAP_SCRIPT_SHA256 = (
    "6d4020f84b6997deabea5e2ad362f1885b52f9c5a2bd4b1cf087ca4100974a9f"
)
EXPECTED_VCPKG_TOOL_METADATA_SHA256 = (
    "fd4c5bdfb023c65cee97488622bb689243e92d30e830e621877ba55f90df7e15"
)
EXPECTED_VCPKG_TOOL_RELEASE_TAG = "2026-05-27"
EXPECTED_VCPKG_TOOL_URL = (
    "https://github.com/microsoft/vcpkg-tool/releases/download/"
    "2026-05-27/vcpkg.exe"
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
EXPECTED_VCPKG_PORT_CMAKE_PAYLOAD = {
    "algorithm": "sha256-path-null-digest-lf-v1",
    "stripPrefix": "",
    "fileCount": 8584,
    "directoryCount": 153,
    "totalBytes": 139214481,
    "inventorySha256": (
        "3601d4f354eccef36c5d53a9454cbc54cdd3fcea252455c866227fc220e721b9"
    ),
    "directoryInventorySha256": (
        "2dbf4923414ee77924e27bde4c2c612fee537fcba58689c6b9d59fd1debb449d"
    ),
    "aggregateSha256": (
        "6046bdae94472ac6feb362fb890079b991e7997af15ce5787954225ba7466d20"
    ),
}
EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY = {
    "version": EXPECTED_VCPKG_PORT_CMAKE,
    "toolsManifest": "scripts/vcpkg-tools.json",
    "toolsManifestSha256": EXPECTED_VCPKG_TOOLS_MANIFEST_SHA256,
    "url": EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["url"],
    "sha512": EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["sha512"],
    "archiveBytes": 52967828,
    "archiveFile": EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["archive"],
    "installationDirectory": "cmake-4.3.3-windows",
    "executable": EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["executable"],
    "executableSha256": EXPECTED_VCPKG_PORT_CMAKE_EXECUTABLE_SHA256,
    "payload": EXPECTED_VCPKG_PORT_CMAKE_PAYLOAD,
}
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
    "EM_CACHE",
    "EM_FROZEN_CACHE",
    "EMSCRIPTEN_ROOT",
    "EMSCRIPTEN_VERSION",
    "EMSDK",
    "EMSDK_PYTHON",
    "PYTHONNOUSERSITE",
    "SOURCE_DATE_EPOCH",
    "CMAKE_NINJA_FORCE_RESPONSE_FILE",
)
EXPECTED_HOST_ENV_PASSTHROUGH = (
    "PYTHONNOUSERSITE",
    "SOURCE_DATE_EPOCH",
)
FORBIDDEN_BUILD_ENVIRONMENT_NAMES = (
    "AR",
    "AS",
    "BASH_ENV",
    "BINARYEN_ROOT",
    "CC",
    "CCC_OVERRIDE_OPTIONS",
    "CL",
    "COMPILER_PATH",
    "CPATH",
    "CPP",
    "CPPFLAGS",
    "C_INCLUDE_PATH",
    "CXX",
    "CFLAGS",
    "CPLUS_INCLUDE_PATH",
    "CXXFLAGS",
    "EMCC_CFLAGS",
    "EMCC_DEBUG",
    "EMMAKEN_CFLAGS",
    "EMMAKEN_COMPILER",
    "EM_COMPILER_WRAPPER",
    "EM_COMPILER_WRAPPER2",
    "ENV",
    "LD",
    "LDFLAGS",
    "LIB",
    "LIBPATH",
    "LIBRARY_PATH",
    "LLVM_ROOT",
    "NM",
    "NODE_OPTIONS",
    "NODE_PATH",
    "NODE_JS",
    "PYTHONHOME",
    "PYTHONPATH",
    "RANLIB",
    "GCC_EXEC_PREFIX",
    "INCLUDE",
    "IPHONEOS_DEPLOYMENT_TARGET",
    "MACOSX_DEPLOYMENT_TARGET",
    "OBJC_INCLUDE_PATH",
    "OBJCPLUS_INCLUDE_PATH",
    "RC",
    "SDKROOT",
    "QT_RCC_SOURCE_DATE_OVERRIDE",
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


def require_no_reparse_chain(path: Path, label: str) -> Path:
    lexical = Path(os.path.abspath(path))
    missing_suffix = False
    for ancestor in reversed((lexical, *lexical.parents)):
        if not ancestor.exists() and not ancestor.is_symlink():
            missing_suffix = True
            continue
        require(
            not missing_suffix,
            f"{label} path changed during chain inspection: {ancestor}",
        )
        require(
            not _is_reparse_or_symlink(ancestor),
            f"{label} reparse component is forbidden: {ancestor}",
        )
        if ancestor != lexical:
            require(
                ancestor.is_dir(),
                f"{label} non-directory component: {ancestor}",
            )
    return lexical


def qualification_environment_identity(
    environment: Mapping[str, str] | None = None,
) -> dict[str, Any]:
    values = environment if environment is not None else os.environ
    require(
        values.get("RHYTHMGAME_WASM_QUALIFICATION") == "1",
        "qualification closure must be enabled by the toolchain wrapper",
    )
    prefix = "RHYTHMGAME_WASM_QUALIFICATION_"
    algorithm = values.get(prefix + "ALGORITHM")
    require(
        algorithm == QUALIFICATION_CLOSURE_ALGORITHM,
        "qualification closure algorithm drifted",
    )
    numeric: dict[str, int] = {}
    for name, suffix in (
        ("fileCount", "FILE_COUNT"),
        ("totalBytes", "TOTAL_BYTES"),
    ):
        raw = values.get(prefix + suffix, "")
        require(
            raw.isdecimal()
            and str(int(raw)) == raw
            and int(raw) > 0,
            f"qualification closure {name} is invalid: {raw!r}",
        )
        numeric[name] = int(raw)
    digests: dict[str, str] = {}
    for name, suffix in (
        ("inventorySha256", "INVENTORY_SHA256"),
        ("aggregateSha256", "AGGREGATE_SHA256"),
    ):
        raw = values.get(prefix + suffix, "")
        require(
            re.fullmatch(r"[0-9a-f]{64}", raw) is not None,
            f"qualification closure {name} is invalid",
        )
        digests[name] = raw
    return {
        "algorithm": algorithm,
        **numeric,
        **digests,
    }


def _qualification_file_digest(path: Path, label: str) -> tuple[int, str]:
    path = require_no_reparse_chain(path, label)
    require(
        path.is_file() and not _is_reparse_or_symlink(path),
        f"{label} is not a regular file: {path}",
    )
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        before = os.fstat(stream.fileno())
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
        after = os.fstat(stream.fileno())
    require(
        before.st_dev == after.st_dev
        and before.st_ino == after.st_ino
        and before.st_size == after.st_size,
        f"{label} changed while hashing: {path}",
    )
    return before.st_size, digest.hexdigest()


def qualification_closure_identity(
    roots: Sequence[tuple[str, Path, Sequence[str]]],
    files: Sequence[tuple[str, Path]],
    environment: Mapping[str, str] | None = None,
) -> dict[str, Any]:
    """Independently reproduce the wrapper's complete qualification closure."""

    def collect() -> dict[str, tuple[Path, str]]:
        entries: dict[str, tuple[Path, str]] = {}
        folded_logical: set[str] = set()
        physical: set[str] = set()

        def add(logical: str, path: Path, source: str) -> None:
            parts = logical.split("/")
            require(
                bool(logical)
                and not logical.startswith("/")
                and not logical.endswith("/")
                and "\\" not in logical
                and all(part not in {"", ".", ".."} for part in parts),
                f"unsafe qualification closure logical path: {logical!r}",
            )
            folded = logical.casefold()
            require(
                folded not in folded_logical,
                "case-insensitive qualification closure logical collision: "
                f"{logical}",
            )
            candidate = require_no_reparse_chain(
                path,
                f"qualification closure {logical!r}",
            )
            require(
                candidate.is_file()
                and not _is_reparse_or_symlink(candidate),
                f"qualification closure file is missing: {candidate}",
            )
            key = path_key(candidate)
            require(
                key not in physical,
                "qualification closure physical file is modeled twice: "
                f"{candidate}",
            )
            folded_logical.add(folded)
            physical.add(key)
            entries[logical] = (candidate, source)

        for label, raw_root, raw_suffixes in roots:
            require(
                re.fullmatch(r"[a-z0-9][a-z0-9-]*", label) is not None,
                f"unsafe qualification closure root label: {label!r}",
            )
            suffixes = tuple(raw_suffixes)
            require(
                all(
                    re.fullmatch(r"\.[a-z0-9-]+", suffix) is not None
                    for suffix in suffixes
                ),
                f"unsafe qualification closure excluded suffixes: {suffixes}",
            )
            root = require_no_reparse_chain(
                raw_root,
                f"qualification closure root {label!r}",
            )
            require(
                root.is_dir() and not _is_reparse_or_symlink(root),
                f"qualification closure root is missing: {root}",
            )
            for candidate in root.rglob("*"):
                require(
                    not _is_reparse_or_symlink(candidate),
                    "qualification closure root contains a reparse point: "
                    f"{candidate}",
                )
                if candidate.is_dir():
                    continue
                require(
                    candidate.is_file(),
                    "qualification closure root contains a special entry: "
                    f"{candidate}",
                )
                if candidate.suffix.casefold() in suffixes:
                    continue
                relative = candidate.relative_to(root).as_posix()
                add(f"{label}/{relative}", candidate, label)

        for logical, path in files:
            add(logical, path, "explicit")
        require(entries, "qualification closure is empty")
        return entries

    entries = collect()
    logical_paths = sorted(entries)
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    source_counts: dict[str, int] = {}
    source_bytes: dict[str, int] = {}
    for logical in logical_paths:
        path, source = entries[logical]
        length, digest = _qualification_file_digest(
            path,
            f"qualification closure {logical!r}",
        )
        total_bytes += length
        source_counts[source] = source_counts.get(source, 0) + 1
        source_bytes[source] = source_bytes.get(source, 0) + length
        inventory.update(f"{logical}\n".encode("utf-8"))
        aggregate.update(
            f"{logical}\0{length}\0{digest}\n".encode("utf-8")
        )

    rechecked = collect()
    require(
        [
            (logical, path_key(rechecked[logical][0]))
            for logical in sorted(rechecked)
        ]
        == [
            (logical, path_key(entries[logical][0]))
            for logical in logical_paths
        ],
        "qualification closure path set changed while hashing",
    )
    actual = {
        "algorithm": QUALIFICATION_CLOSURE_ALGORITHM,
        "fileCount": len(logical_paths),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }
    expected = qualification_environment_identity(environment)
    require(
        actual == expected,
        "qualification closure environment does not match independently "
        f"hashed bytes: expected {expected}, got {actual}",
    )
    return {
        **actual,
        "rootFileCounts": {
            label: source_counts.get(label, 0)
            for label, _, _ in roots
        },
        "rootByteCounts": {
            label: source_bytes.get(label, 0)
            for label, _, _ in roots
        },
        "explicitFileCount": source_counts.get("explicit", 0),
        "explicitTotalBytes": source_bytes.get("explicit", 0),
        "sameHandleLifetimeLockedByWrapper": True,
        "independentlyRehashedByVerifier": True,
    }


def verify_authenticated_build_tool(
    repo: Path,
    name: str,
    artifact: Mapping[str, Any],
    installation: Path,
    archive_root: Path | None = None,
) -> dict[str, Any]:
    archive_hash_key = "sha512" if "sha512" in artifact else "sha256"
    require_exact_keys(
        artifact,
        {
            "version",
            "url",
            archive_hash_key,
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
    archive = (
        archive_root
        if archive_root is not None
        else repo / ".toolchains" / "downloads"
    ) / archive_name
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
    # reached only after the complete archive hash has matched.
    with archive.open("rb") as archive_stream:
        archive_digest = (
            hashlib.sha512()
            if archive_hash_key == "sha512"
            else hashlib.sha256()
        )
        for chunk in iter(lambda: archive_stream.read(1024 * 1024), b""):
            archive_digest.update(chunk)
        archive_hash = archive_digest.hexdigest()
        require(
            archive_hash == artifact[archive_hash_key],
            (
                f"{name} retained archive "
                f"{'SHA-512' if archive_hash_key == 'sha512' else 'SHA-256'} "
                "drifted"
            ),
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
            archive_hash_key: archive_hash,
        },
        "payload": actual_payload,
        "installationRoot": relative_path(repo, installation),
    }


def verify_authenticated_source_archive(
    repo: Path,
    name: str,
    artifact: Mapping[str, Any],
    installation: Path,
) -> dict[str, Any]:
    """Bind a retained source ZIP to every installed source member.

    Runtime products may coexist with the source tree only under the explicit
    top-level prefixes/files in the lock. Source members themselves must still
    match the authenticated archive byte-for-byte and case-for-case.
    """
    require_exact_keys(
        artifact,
        {
            "url",
            "archiveFile",
            "sha256",
            "payload",
            "allowedRuntimePrefixes",
            "allowedRuntimeFiles",
        },
        f"{name}.sourceArchive",
    )
    archive_name = _safe_contract_relative_path(
        artifact["archiveFile"],
        f"{name}.sourceArchive.archiveFile",
    )
    require("/" not in archive_name, f"{name} source archive must be a leaf")
    archive = repo / ".toolchains" / "downloads" / archive_name
    require_no_reparse_chain(archive, f"{name} retained source archive")
    require(
        archive.is_file() and not _is_reparse_or_symlink(archive),
        f"{name} retained source archive is missing/reparse: {archive}",
    )
    actual_archive_sha256 = sha256(archive)
    require(
        actual_archive_sha256 == artifact["sha256"],
        f"{name} retained source archive SHA-256 drifted",
    )

    payload = require_mapping(
        artifact["payload"],
        f"{name}.sourceArchive.payload",
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
        f"{name}.sourceArchive.payload",
    )
    require(
        payload["algorithm"] == "sha256-path-null-digest-lf-v1",
        f"unsupported {name} source payload algorithm",
    )
    raw_prefix = payload["stripPrefix"]
    require(
        type(raw_prefix) is str,
        f"{name}.sourceArchive.payload.stripPrefix must be a string",
    )
    prefix = (
        _safe_contract_relative_path(
            raw_prefix,
            f"{name}.sourceArchive.payload.stripPrefix",
        )
        if raw_prefix
        else ""
    )

    prefixes = artifact["allowedRuntimePrefixes"]
    runtime_files = artifact["allowedRuntimeFiles"]
    require(
        isinstance(prefixes, list)
        and isinstance(runtime_files, list),
        f"{name} source runtime allowances must be lists",
    )
    normalized_prefixes = [
        _safe_contract_relative_path(
            value,
            f"{name}.sourceArchive.allowedRuntimePrefixes",
        )
        for value in prefixes
    ]
    normalized_runtime_files = [
        _safe_contract_relative_path(
            value,
            f"{name}.sourceArchive.allowedRuntimeFiles",
        )
        for value in runtime_files
    ]
    require(
        len({value.casefold() for value in normalized_prefixes})
        == len(normalized_prefixes)
        and len({value.casefold() for value in normalized_runtime_files})
        == len(normalized_runtime_files),
        f"{name} source runtime allowances collide",
    )

    source_files: dict[str, tuple[str, str, int]] = {}
    source_directories: dict[str, str] = {}
    path_kinds: dict[str, tuple[str, str]] = {}
    with archive.open("rb") as archive_stream:
        # Recheck the same open handle immediately before ZIP parsing.
        digest = hashlib.sha256()
        for chunk in iter(lambda: archive_stream.read(1024 * 1024), b""):
            digest.update(chunk)
        require(
            digest.hexdigest() == artifact["sha256"],
            f"{name} source archive changed before parsing",
        )
        archive_stream.seek(0)
        with zipfile.ZipFile(archive_stream) as bundle:
            entries: list[tuple[str, zipfile.ZipInfo]] = []
            explicit_directories: set[str] = set()
            for info in bundle.infolist():
                raw = info.filename
                require(
                    "\\" not in raw,
                    f"{name} source archive backslash path: {raw}",
                )
                is_directory = info.is_dir()
                relative = raw.rstrip("/")
                if prefix:
                    if relative == prefix:
                        require(
                            is_directory,
                            f"{name} source archive prefix is a file",
                        )
                        continue
                    require(
                        relative.startswith(f"{prefix}/"),
                        f"unexpected {name} source archive entry: {raw}",
                    )
                    relative = relative[len(prefix) + 1 :]
                relative = _safe_contract_relative_path(
                    relative,
                    f"{name} source archive entry",
                )
                unix_type = (info.external_attr >> 16) & 0o170000
                expected_type = stat.S_IFDIR if is_directory else stat.S_IFREG
                require(
                    unix_type in (0, expected_type)
                    and not (
                        info.external_attr
                        & getattr(
                            stat,
                            "FILE_ATTRIBUTE_REPARSE_POINT",
                            0x400,
                        )
                    ),
                    f"{name} source archive link/type mismatch: {raw}",
                )
                key = relative.casefold()
                if is_directory:
                    require(
                        info.file_size == 0
                        and key not in explicit_directories,
                        f"duplicate/contentful {name} source directory: {raw}",
                    )
                    explicit_directories.add(key)
                    existing = path_kinds.get(key)
                    require(
                        existing is None
                        or (
                            existing[0] == relative
                            and existing[1] == "directory"
                        ),
                        f"{name} source path collision: {relative}",
                    )
                    if existing is None:
                        path_kinds[key] = (relative, "directory")
                        source_directories[key] = relative
                    continue
                require(
                    key not in path_kinds,
                    f"duplicate/colliding {name} source file: {relative}",
                )
                parts = relative.split("/")
                for index in range(1, len(parts)):
                    parent = "/".join(parts[:index])
                    parent_key = parent.casefold()
                    existing = path_kinds.get(parent_key)
                    require(
                        existing is None
                        or (
                            existing[0] == parent
                            and existing[1] == "directory"
                        ),
                        f"{name} source file/directory collision: {parent}",
                    )
                    if existing is None:
                        path_kinds[parent_key] = (parent, "directory")
                        source_directories[parent_key] = parent
                path_kinds[key] = (relative, "file")
                entries.append((relative, info))
            require(entries, f"{name} source archive file inventory is empty")
            for relative, info in entries:
                entry_digest = hashlib.sha256()
                with bundle.open(info) as entry_stream:
                    for chunk in iter(
                        lambda: entry_stream.read(1024 * 1024),
                        b"",
                    ):
                        entry_digest.update(chunk)
                source_files[relative.casefold()] = (
                    relative,
                    entry_digest.hexdigest(),
                    info.file_size,
                )

    ordered_files = sorted(record[0] for record in source_files.values())
    ordered_directories = sorted(source_directories.values())
    inventory = hashlib.sha256()
    directory_inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative in ordered_files:
        _, file_digest, length = source_files[relative.casefold()]
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{file_digest}\n".encode("utf-8"))
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
        f"{name} authenticated source archive payload drifted",
    )

    require_no_reparse_chain(installation, f"{name} source installation")
    require(
        installation.is_dir() and not _is_reparse_or_symlink(installation),
        f"{name} source installation is missing/reparse: {installation}",
    )
    seen_sources: set[str] = set()
    for candidate in installation.rglob("*"):
        require(
            not _is_reparse_or_symlink(candidate),
            f"{name} source installation reparse point: {candidate}",
        )
        relative = candidate.relative_to(installation).as_posix()
        key = relative.casefold()
        source_file = source_files.get(key)
        source_directory = source_directories.get(key)
        if source_file is not None:
            require(
                candidate.is_file()
                and relative == source_file[0]
                and candidate.stat().st_size == source_file[2]
                and sha256(candidate) == source_file[1],
                f"{name} installed source bytes/case drifted: {relative}",
            )
            seen_sources.add(key)
            continue
        if source_directory is not None:
            require(
                candidate.is_dir() and relative == source_directory,
                f"{name} installed source directory drifted: {relative}",
            )
            continue
        if candidate.is_dir():
            allowed = (
                any(
                    relative == runtime_prefix
                    or relative.startswith(f"{runtime_prefix}/")
                    or runtime_prefix.startswith(f"{relative}/")
                    for runtime_prefix in normalized_prefixes
                )
                or any(
                    runtime_file.startswith(f"{relative}/")
                    for runtime_file in normalized_runtime_files
                )
            )
        else:
            allowed = (
                candidate.is_file()
                and (
                    relative in normalized_runtime_files
                    or any(
                        relative.startswith(f"{runtime_prefix}/")
                        for runtime_prefix in normalized_prefixes
                    )
                )
            )
        require(
            allowed,
            f"{name} unmodeled runtime/source path: {relative}",
        )
    require(
        seen_sources == set(source_files),
        f"{name} installed source member set is incomplete",
    )
    return {
        "archive": {
            "path": relative_path(repo, archive),
            "sha256": actual_archive_sha256,
        },
        "payload": actual_payload,
        "installationRoot": relative_path(repo, installation),
        "allowedRuntimePrefixes": normalized_prefixes,
        "allowedRuntimeFiles": normalized_runtime_files,
    }


def verify_emscripten_cache_payload(
    repo: Path,
    root: Path,
    contract: Mapping[str, Any],
    label: str,
) -> dict[str, Any]:
    algorithm = "sha256-path-null-digest-lf-v1"
    require_exact_keys(
        contract,
        {
            "algorithm",
            "fileCount",
            "directoryCount",
            "totalBytes",
            "inventorySha256",
            "directoryInventorySha256",
            "aggregateSha256",
        },
        f"{label}.payload",
    )
    require(
        contract["algorithm"] == algorithm,
        f"{label} payload algorithm drifted",
    )
    require(
        root.is_dir() and not _is_reparse_or_symlink(root),
        f"{label} root is missing or a reparse point: {root}",
    )
    lexical = Path(os.path.abspath(root))
    canonical = lexical.resolve(strict=True)
    require_same_path(
        canonical,
        lexical,
        f"{label} canonical root",
    )
    for ancestor in (lexical, *lexical.parents):
        if ancestor.exists():
            require(
                not _is_reparse_or_symlink(ancestor),
                f"{label} root-chain reparse point is forbidden: {ancestor}",
            )
    files: dict[str, Path] = {}
    directories: dict[str, str] = {}
    for candidate in root.rglob("*"):
        require(
            not _is_reparse_or_symlink(candidate),
            f"{label} reparse point is forbidden: {candidate}",
        )
        relative = candidate.relative_to(root).as_posix()
        key = relative.casefold()
        require(
            key not in files and key not in directories,
            f"{label} duplicate/colliding path: {relative}",
        )
        if candidate.is_dir():
            directories[key] = relative
        else:
            require(
                candidate.is_file(),
                f"{label} special filesystem entry is forbidden: {candidate}",
            )
            files[key] = candidate
    ordered_files = sorted(
        (path.relative_to(root).as_posix(), path)
        for path in files.values()
    )
    ordered_directories = sorted(directories.values())
    require(ordered_files, f"{label} file inventory is empty")
    inventory = hashlib.sha256()
    directory_inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative, path in ordered_files:
        content = path.read_bytes()
        total_bytes += len(content)
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(
            (
                f"{relative}\0{hashlib.sha256(content).hexdigest()}\n"
            ).encode("utf-8")
        )
    for relative in ordered_directories:
        directory_inventory.update(f"{relative}/\n".encode("utf-8"))
    actual = {
        "algorithm": algorithm,
        "fileCount": len(ordered_files),
        "directoryCount": len(ordered_directories),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "directoryInventorySha256": directory_inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }
    require(actual == dict(contract), f"{label} payload identity drifted")
    return {
        "root": relative_path(repo, root),
        "payload": actual,
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


def verify_emscripten_python_import_closure(
    root: Path,
    contract: Mapping[str, Any],
) -> dict[str, Any]:
    require_exact_keys(
        contract,
        {
            "algorithm",
            "fileCount",
            "totalBytes",
            "inventorySha256",
            "aggregateSha256",
        },
        "emscripten.driverApi.pythonImportClosure",
    )
    require(
        contract["algorithm"] == "sha256-path-null-digest-lf-v1",
        "Emscripten Python import closure algorithm drifted",
    )
    files: dict[str, Path] = {}
    for candidate in root.rglob("*"):
        require(
            not _is_reparse_or_symlink(candidate),
            f"Emscripten Python import reparse point: {candidate}",
        )
        if (
            not candidate.is_file()
            or candidate.suffix.casefold() != ".py"
        ):
            continue
        relative = candidate.relative_to(root).as_posix()
        key = relative.casefold()
        require(
            key not in files,
            f"Emscripten Python import path collision: {relative}",
        )
        files[key] = candidate
    ordered = sorted(
        (
            (path.relative_to(root).as_posix(), path)
            for path in files.values()
        ),
        key=lambda item: item[0],
    )
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    for relative, path in ordered:
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        total_bytes += len(data)
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{digest}\n".encode("utf-8"))
    actual = {
        "algorithm": "sha256-path-null-digest-lf-v1",
        "fileCount": len(ordered),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
    }
    require(
        actual == dict(contract),
        "Emscripten Python import closure bytes/path set drifted",
    )
    return actual


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


def compile_entry_output(
    entry: Mapping[str, Any],
    build: Path,
    context: str,
) -> Path:
    directory_value = entry.get("directory")
    output_value = entry.get("output")
    require(
        isinstance(directory_value, str) and bool(directory_value),
        f"{context}: compile entry directory is missing",
    )
    require(
        isinstance(output_value, str) and bool(output_value),
        f"{context}: compile entry output is missing",
    )
    directory = Path(directory_value)
    require_same_path(directory, build, f"{context}: compile directory")
    output = Path(output_value)
    if not output.is_absolute():
        output = directory / output
    output = require_no_reparse_chain(
        output,
        f"{context}: compile output",
    )
    require(
        output.is_relative_to(build.resolve())
        and output.suffix.casefold() == ".o",
        f"{context}: compile output escaped the selected build: {output}",
    )
    return output


def strip_ninja_dependency_bookkeeping(
    arguments: Sequence[str],
    context: str,
) -> list[str]:
    """Remove only CMake/Ninja depfile operands absent from compile DB."""
    result: list[str] = []
    occurrences = {"-MD": 0, "-MT": 0, "-MF": 0}
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument == "-MD":
            occurrences["-MD"] += 1
            index += 1
            continue
        if argument in {"-MT", "-MF"}:
            occurrences[argument] += 1
            require(
                index + 1 < len(arguments),
                f"{context}: {argument} has no operand",
            )
            index += 2
            continue
        result.append(argument)
        index += 1
    require(
        occurrences == {"-MD": 1, "-MT": 1, "-MF": 1},
        f"{context}: Ninja dependency bookkeeping drifted: {occurrences}",
    )
    return result


def canonical_compile_parity_arguments(
    arguments: Sequence[str],
) -> list[str]:
    """Normalize only Windows path spelling; preserve every semantic flag."""
    path_operand_options = {
        "-I",
        "-c",
        "-idirafter",
        "-imacros",
        "-include",
        "-iquote",
        "-isystem",
        "-o",
    }
    result: list[str] = []
    path_operand = False
    for index, argument in enumerate(arguments):
        is_joined_path_option = (
            argument.startswith("-I") and argument != "-I"
        ) or (
            argument.startswith("-isystem")
            and argument != "-isystem"
        )
        normalize = index == 0 or path_operand or is_joined_path_option
        result.append(argument.replace("\\", "/") if normalize else argument)
        path_operand = argument in path_operand_options
    return result


def require_compile_argv_parity(
    compile_database_arguments: Sequence[str],
    expanded_ninja_arguments: Sequence[str],
    context: str,
) -> list[str]:
    actual = strip_ninja_dependency_bookkeeping(
        expanded_ninja_arguments,
        context,
    )
    expected_canonical = canonical_compile_parity_arguments(
        compile_database_arguments
    )
    actual_canonical = canonical_compile_parity_arguments(actual)
    require(
        actual_canonical == expected_canonical,
        f"{context}: expanded Ninja compiler argv differs from "
        "compile_commands.json; "
        f"expected {expected_canonical}, got {actual_canonical}",
    )
    return actual


def authenticated_adapter_compiler_arguments(
    arguments: Sequence[str],
    repo: Path,
    driver_kind: str,
    context: str,
) -> list[str]:
    """Validate the exact CMake launcher prefix and return emcc/em++ argv."""
    require(
        driver_kind in {"emcc", "em++"},
        f"{context}: unsupported driver kind {driver_kind}",
    )
    emsdk = repo / ".toolchains" / f"emsdk-{EXPECTED_EMSCRIPTEN}"
    python = repo / EXPECTED_EMSDK_PYTHON
    adapter = (
        repo
        / "tools"
        / "wasm-probe"
        / "scripts"
        / "invoke_emscripten_driver.py"
    )
    lock = repo / "tools" / "wasm-probe" / "toolchain-lock.json"
    auditor = (
        repo
        / "tools"
        / "wasm-probe"
        / "scripts"
        / "audit_emscripten_response_files.py"
    )
    emscripten_root = emsdk / "upstream" / "emscripten"
    em_config = emsdk / ".emscripten"
    cache = (
        repo
        / ".toolchains"
        / EXPECTED_EMSCRIPTEN_LOCK_ENTRY["cache"]["directory"]
    )
    compiler = emscripten_root / f"{driver_kind}.bat"
    require(
        len(arguments) >= 20,
        f"{context}: compiler launcher argument stream is too short",
    )
    require_same_path(arguments[0], python, f"{context}: launcher Python")
    require(
        list(arguments[1:3]) == ["-I", "-B"],
        f"{context}: launcher Python isolation flags drifted",
    )
    require_same_path(arguments[3], adapter, f"{context}: driver adapter")
    expected_options = (
        ("--lock", lock),
        ("--auditor", auditor),
        ("--emscripten-root", emscripten_root),
        ("--em-config", em_config),
    )
    cursor = 4
    for option, expected in expected_options:
        require(
            arguments[cursor] == option,
            f"{context}: expected launcher option {option}",
        )
        require_same_path(
            arguments[cursor + 1],
            expected,
            f"{context}: {option}",
        )
        cursor += 2
        if option == "--em-config":
            require(
                arguments[cursor] == "--em-config-sha256"
                and arguments[cursor + 1] == sha256(em_config),
                f"{context}: activation-file authentication drifted",
            )
            cursor += 2
    require(
        arguments[cursor] == "--cache-root",
        f"{context}: expected launcher option --cache-root",
    )
    require_same_path(
        arguments[cursor + 1],
        cache,
        f"{context}: --cache-root",
    )
    cursor += 2
    require(
        arguments[cursor : cursor + 2] == ["--driver-kind", driver_kind],
        f"{context}: driver kind drifted",
    )
    cursor += 2
    require(
        arguments[cursor] == "--",
        f"{context}: launcher separator is missing",
    )
    cursor += 1
    require_same_path(
        arguments[cursor],
        compiler,
        f"{context}: pinned compiler",
    )
    return list(arguments[cursor:])


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


def clear_mutable_autogen_state(build: Path) -> list[str]:
    build_root = require_no_reparse_chain(
        build,
        "qualification build root",
    )
    require(
        build_root.is_dir() and not _is_reparse_or_symlink(build_root),
        f"qualification build root is invalid: {build_root}",
    )
    removed: list[str] = []
    for relative in QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS:
        normalized = _safe_contract_relative_path(
            relative,
            "mutable Autogen state path",
        )
        require(
            normalized == relative,
            f"mutable Autogen state path is not canonical: {relative}",
        )
        candidate = build_root / relative
        require_no_reparse_chain(
            candidate.parent,
            f"mutable Autogen state parent {relative!r}",
        )
        if candidate.exists() or candidate.is_symlink():
            candidate = require_no_reparse_chain(
                candidate,
                f"mutable Autogen state {relative!r}",
            )
            require(
                candidate.is_file()
                and not _is_reparse_or_symlink(candidate),
                f"mutable Autogen state is not a regular file: {candidate}",
            )
            candidate.unlink()
            removed.append(relative)
        require(
            not candidate.exists() and not candidate.is_symlink(),
            f"mutable Autogen state survived reset: {candidate}",
        )
    return removed


def clean_rebuild_selected_targets(
    repo: Path,
    build: Path,
    ninja: Path,
) -> dict[str, Any]:
    qualification_environment_identity()
    targets = [
        "RhythmGameWasmCLauncherProbe",
        "RhythmGameWasmProbe",
    ]
    removed_autogen_state = clear_mutable_autogen_state(build)

    def invoke(arguments: Sequence[str], label: str) -> list[str]:
        result = subprocess.run(
            [str(ninja), "-C", str(build), *arguments],
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        output = "\n".join(
            part.strip()
            for part in (result.stdout, result.stderr)
            if part.strip()
        )
        require(
            result.returncode == 0,
            f"qualified {label} failed ({result.returncode}): "
            f"{output[-8000:]}",
        )
        return [line.strip() for line in output.splitlines() if line.strip()]

    clean_lines = invoke(["-t", "clean", *targets], "clean")
    clean_matches = [
        match
        for line in clean_lines
        if (
            match := re.fullmatch(
                r"Cleaning\.\.\.\s+(\d+)\s+files?\.",
                line,
            )
        )
    ]
    require(
        len(clean_matches) == 1,
        f"qualified clean output shape drifted: {clean_lines}",
    )
    build_lines = invoke(["-v", *targets], "rebuild")
    executed_edges = sum(
        re.match(r"^\[\d+/\d+\]\s+", line) is not None
        for line in build_lines
    )
    require(
        executed_edges > 0
        and not any(line == "ninja: no work to do." for line in build_lines),
        f"qualified clean rebuild executed no build edges: {build_lines}",
    )
    return {
        "targets": targets,
        "cleanCommand": [
            relative_path(repo, ninja),
            "-C",
            relative_path(repo, build),
            "-t",
            "clean",
            *targets,
        ],
        "buildCommand": [
            relative_path(repo, ninja),
            "-C",
            relative_path(repo, build),
            "-v",
            *targets,
        ],
        "cleanedOutputCount": int(clean_matches[0].group(1)),
        "executedEdgeCount": executed_edges,
        "mutableAutogenStatePaths": list(
            QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS
        ),
        "removedMutableAutogenStateCount": len(removed_autogen_state),
        "allMutableAutogenStateAbsentBeforeCleanRebuild": True,
        "allCommandsRanInsideQualificationClosure": True,
    }


def verify_ninja_noop(
    ninja: Path,
    build: Path,
    target: str = "RhythmGameWasmProbe",
) -> list[str]:
    output = run_text(
        ninja,
        "-C",
        build,
        "-v",
        target,
    )
    lines = [line.strip() for line in output.splitlines() if line.strip()]
    clean_noop = (
        len(lines) == 2
        and lines[0].startswith("ninja: Entering directory ")
        and lines[1] == "ninja: no work to do."
    )
    configure_depends_noop = False
    if (
        len(lines) == 3
        and lines[0].startswith("ninja: Entering directory ")
        and lines[2] == "ninja: no work to do."
    ):
        cache = parse_cmake_cache(build / "CMakeCache.txt")
        expected_cmake = cache.get("CMAKE_COMMAND", "")

        def command_tokens(line: str) -> list[str]:
            match = re.fullmatch(r"\[\d+/\d+\]\s+(.+)", line)
            return split_windows_command_line(match.group(1)) if match else []

        verify_globs = command_tokens(lines[1])
        configure_depends_noop = (
            len(verify_globs) == 3
            and verify_globs[1] == "-P"
        )
        if configure_depends_noop:
            require_same_path(
                verify_globs[0],
                Path(expected_cmake),
                "Ninja CONFIGURE_DEPENDS CMake",
            )
            require_same_path(
                verify_globs[2],
                build / "CMakeFiles" / "VerifyGlobs.cmake",
                "Ninja CONFIGURE_DEPENDS glob verifier",
            )
    require(
        clean_noop or configure_depends_noop,
        f"{target} target closure is not a no-op; "
        f"actual build output was {lines}",
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

    manifest_sha256 = hashlib.sha256(raw).hexdigest()
    payload = f"manifest={manifest_sha256}\n"
    for relative, path in zip(relative_paths, inputs):
        payload += f"{relative}={sha256(path)}\n"
    aggregate = sha256_text(payload)
    return {
        "algorithm": "sha256-manifest-path-equals-digest-lf-v1",
        "tracking": "explicit-input-manifest",
        "manifest": relative_path(repo, manifest),
        "manifestSha256": manifest_sha256,
        "count": len(inputs),
        "paths": relative_paths,
        "aggregateSha256": aggregate,
        "_files": inputs,
    }


def probe_source_inputs(repo: Path) -> list[Path]:
    return list(probe_input_identity(repo)["_files"])


def probe_build_control_identity(repo: Path) -> dict[str, Any]:
    probe = repo / "tools" / "wasm-probe"
    manifest = probe / "build-control-manifest.txt"
    input_identity = probe_input_identity(repo)
    require(
        "tools/wasm-probe/build-control-manifest.txt"
        in input_identity["paths"],
        "build-control manifest is absent from the probe input manifest",
    )
    raw = manifest.read_bytes()
    require(raw.endswith(b"\n"), "build-control manifest must end with LF")
    text = raw.decode("utf-8")
    require("\r" not in text, "build-control manifest must use LF")
    relative_paths = text.splitlines()
    require(
        tuple(relative_paths) == EXPECTED_BUILD_CONTROL_PATHS,
        "build-control manifest path set/order drifted",
    )
    build = (probe / "build" / "wasm-release").resolve()
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    files: list[Path] = []
    for relative in relative_paths:
        normalized = _safe_contract_relative_path(
            relative,
            "build-control manifest entry",
        )
        require(
            normalized == relative,
            f"build-control path is not canonical: {relative}",
        )
        candidate = require_no_reparse_chain(
            build / relative,
            f"build control {relative!r}",
        )
        require(
            candidate.is_relative_to(build)
            and candidate.is_file()
            and not _is_reparse_or_symlink(candidate),
            f"build control is missing or escaping: {relative}",
        )
        length, digest = _qualification_file_digest(
            candidate,
            f"build control {relative!r}",
        )
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(
            f"{relative}\0{length}\0{digest}\n".encode("utf-8")
        )
        total_bytes += length
        files.append(candidate)
    build_ninja = (build / "build.ninja").read_text(
        "utf-8",
        errors="replace",
    )
    producerless_inputs = selected_target_build_root_leaf_inputs(
        build,
        build_ninja,
    )
    locked_paths = set(relative_paths)
    require(
        set(producerless_inputs).issubset(locked_paths),
        "selected target has producerless build-root inputs outside the "
        "immutable build-control manifest: "
        f"{sorted(set(producerless_inputs) - locked_paths)}",
    )
    command_only_inputs = list(
        QUALIFICATION_COMMAND_ONLY_BUILD_CONTROL_PATHS
    )
    normalized_graph = build_ninja.replace("\\", "/")
    require(
        all(
            path in locked_paths and path in normalized_graph
            for path in command_only_inputs
        ),
        "selected command-only build control is absent from the locked graph",
    )
    mutable_state: set[str] = set()
    for relative, candidate in zip(relative_paths, files):
        if not relative.endswith("_autogen.dir/AutogenInfo.json"):
            continue
        payload = require_mapping(
            json.loads(candidate.read_text("utf-8")),
            f"Autogen build control {relative}",
        )
        for key in ("PARSE_CACHE_FILE", "SETTINGS_FILE"):
            value = payload.get(key)
            require(
                isinstance(value, str) and value,
                f"Autogen build control {relative} has no {key}",
            )
            state = Path(os.path.abspath(value))
            require(
                state.is_relative_to(build),
                f"Autogen mutable state escaped build root: {state}",
            )
            mutable_state.add(state.relative_to(build).as_posix())
    mutable_state_paths = sorted(mutable_state, key=lambda value: value.casefold())
    require(
        tuple(mutable_state_paths)
        == QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS,
        "Autogen mutable state classification drifted",
    )
    return {
        "schemaVersion": 1,
        "algorithm": QUALIFICATION_CLOSURE_ALGORITHM,
        "manifest": "tools/wasm-probe/build-control-manifest.txt",
        "fileCount": len(files),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
        "sameHandleLifetimeLockedByWrapper": True,
        "configureMustBeSettledBeforeQualification": True,
        "selectedTargetProducerlessInputs": producerless_inputs,
        "commandOnlyImmutableInputs": command_only_inputs,
        "mutableGeneratorStateResetPaths": mutable_state_paths,
        "allSelectedBuildInputsClassified": True,
        "_files": files,
        "_paths": relative_paths,
    }


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
    repo: Path,
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
    compiler_arguments = authenticated_adapter_compiler_arguments(
        arguments,
        repo,
        "em++",
        "application link",
    )
    require_same_path(
        compiler_arguments[0],
        expected_emxx,
        "application link compiler",
    )
    require(
        compiler_arguments.count("@CMakeFiles\\RhythmGameWasmProbe.rsp") == 1,
        "application link must use the exact generated response file",
    )
    return compiler_arguments


def parse_adapter_link_arguments(
    command: str,
    repo: Path,
    expected_compiler: Path,
    driver_kind: str,
    context: str,
) -> list[str]:
    outer = split_windows_command_line(command)
    require(
        len(outer) == 3
        and Path(outer[0]).name.casefold() == "cmd.exe"
        and outer[1].casefold() == "/c",
        f"{context}: unexpected link wrapper: {outer[:2]}",
    )
    body = outer[2].strip()
    prefix = "cd . && "
    suffix = " && cd ."
    require(
        body.startswith(prefix) and body.endswith(suffix),
        f"{context}: unexpected shell body: {body}",
    )
    arguments = split_windows_command_line(
        body[len(prefix) : -len(suffix)].strip()
    )
    compiler_arguments = authenticated_adapter_compiler_arguments(
        arguments,
        repo,
        driver_kind,
        context,
    )
    require_same_path(
        compiler_arguments[0],
        expected_compiler,
        f"{context} compiler",
    )
    return compiler_arguments


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


def ninja_build_edges(
    build: Path,
    build_ninja: str,
) -> dict[str, dict[str, Any]]:
    """Parse Ninja build edges and key every literal output by absolute path."""
    edges: dict[str, dict[str, Any]] = {}
    for line in ninja_logical_lines(build_ninja):
        if not line.startswith("build "):
            continue
        separator = None
        index = len("build ")
        while index < len(line):
            if line[index] == ":" and line[index - 1] != "$":
                separator = index
                break
            index += 1
        require(separator is not None, f"Ninja build edge has no colon: {line}")
        output_tokens = split_ninja_arguments(
            line[len("build ") : separator]
        )
        input_tokens = split_ninja_arguments(line[separator + 1 :].strip())
        require(input_tokens, f"Ninja build edge has no rule: {line}")
        outputs = [
            token
            for token in output_tokens
            if token != "|" and "$" not in token
        ]
        explicit_inputs: list[str] = []
        implicit_inputs: list[str] = []
        order_only_inputs: list[str] = []
        destination = explicit_inputs
        for token in input_tokens[1:]:
            if token == "|":
                destination = implicit_inputs
            elif token == "||":
                destination = order_only_inputs
            else:
                destination.append(token)
        edge = {
            "rule": input_tokens[0],
            "outputs": outputs,
            "explicitInputs": explicit_inputs,
            "implicitInputs": implicit_inputs,
            "orderOnlyInputs": order_only_inputs,
        }
        for output in outputs:
            candidate = Path(output)
            if not candidate.is_absolute():
                candidate = build / candidate
            key = path_key(candidate)
            require(
                key not in edges,
                f"duplicate literal Ninja output: {output}",
            )
            edges[key] = edge
    return edges


def selected_target_build_root_leaf_inputs(
    build: Path,
    build_ninja: str,
) -> list[str]:
    """Return selected-target build-root inputs with no producer edge."""
    build_root = build.resolve()
    edges = ninja_build_edges(build_root, build_ninja)
    visited: set[str] = set()
    leaves: dict[str, str] = {}

    def resolve(value: str) -> Path:
        candidate = Path(value)
        if not candidate.is_absolute():
            candidate = build_root / candidate
        return Path(os.path.abspath(candidate))

    def visit(candidate: Path) -> None:
        key = path_key(candidate)
        if key in visited:
            return
        visited.add(key)
        edge = edges.get(key)
        if edge is None:
            if candidate.is_relative_to(build_root) and candidate.is_file():
                relative = candidate.relative_to(build_root).as_posix()
                leaves[relative.casefold()] = relative
            return
        for value in (
            *edge["explicitInputs"],
            *edge["implicitInputs"],
            *edge["orderOnlyInputs"],
        ):
            require(
                "$" not in value,
                f"selected target graph has unresolved input: {value}",
            )
            visit(resolve(value))

    for target in (
        "RhythmGameWasmCLauncherProbe",
        "RhythmGameWasmProbe",
    ):
        target_path = build_root / target
        require(
            path_key(target_path) in edges,
            f"selected target graph edge is missing: {target}",
        )
        visit(target_path)
    return sorted(leaves.values(), key=lambda value: value.casefold())


def selected_application_compile_outputs(
    build: Path,
    build_ninja: str,
) -> dict[str, Any]:
    """Resolve the exact build-local object/archive closure of the app link."""
    edges = ninja_build_edges(build, build_ninja)
    build_root = build.resolve()
    selected_objects: dict[str, Path] = {}
    selected_archives: dict[str, Path] = {}
    visiting: set[str] = set()

    def local_static_path(value: str) -> Path | None:
        candidate = Path(value)
        if not candidate.is_absolute():
            candidate = build / candidate
        candidate = Path(os.path.abspath(candidate))
        if not candidate.is_relative_to(build_root):
            return None
        if candidate.suffix.casefold() not in {".o", ".a"}:
            return None
        return candidate

    def consume(candidate: Path) -> None:
        key = path_key(candidate)
        if candidate.suffix.casefold() == ".o":
            require(
                key in edges,
                "selected build-local object has no Ninja producer edge: "
                f"{candidate}",
            )
            selected_objects[key] = candidate
            return
        if key in selected_archives:
            return
        require(
            key not in visiting,
            f"selected build-local archive graph is cyclic: {candidate}",
        )
        producer = edges.get(key)
        require(
            producer is not None,
            "selected build-local archive has no Ninja producer edge: "
            f"{candidate}",
        )
        visiting.add(key)
        selected_archives[key] = candidate
        member_count = 0
        for value in (
            *producer["explicitInputs"],
            *producer["implicitInputs"],
        ):
            member = local_static_path(value)
            if member is None:
                continue
            member_count += 1
            consume(member)
        visiting.remove(key)
        require(
            member_count > 0,
            "selected build-local archive has no modeled object/archive "
            f"inputs: {candidate}",
        )

    application = build / "RhythmGameWasmProbe.js"
    link = edges.get(path_key(application))
    require(link is not None, "selected application Ninja edge is missing")
    for value in (*link["explicitInputs"], *link["implicitInputs"]):
        candidate = local_static_path(value)
        if candidate is not None:
            consume(candidate)
    require(
        selected_objects,
        "selected application target graph has no build-local objects",
    )
    relative_objects = sorted(
        path.relative_to(build_root).as_posix()
        for path in selected_objects.values()
    )
    relative_archives = sorted(
        path.relative_to(build_root).as_posix()
        for path in selected_archives.values()
    )
    aggregate = hashlib.sha256()
    for relative in relative_objects:
        aggregate.update(f"{relative}\n".encode("utf-8"))
    return {
        "_objectPaths": {
            key: selected_objects[key] for key in sorted(selected_objects)
        },
        "target": "RhythmGameWasmProbe",
        "graphSource": "build.ninja",
        "objectOutputCount": len(relative_objects),
        "archiveOutputCount": len(relative_archives),
        "objectOutputsSha256": aggregate.hexdigest(),
        "allSelectedOutputsMatchedByExactOutput": True,
    }


def require_selected_compile_output_correlation(
    selected_outputs: Iterable[str],
    compile_database_outputs: Iterable[str],
    expanded_ninja_outputs: Iterable[str],
) -> None:
    selected = set(selected_outputs)
    database = set(compile_database_outputs)
    expanded = set(expanded_ninja_outputs)
    require(
        bool(selected)
        and selected.issubset(database)
        and selected.issubset(expanded),
        "selected application target/archive object closure is not exactly "
        "correlated by compile output",
    )


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
    repo: Path,
    expected_emxx: Path,
    build_ninja: str,
    rules_ninja: str,
) -> dict[str, Any]:
    outer = parse_application_link_arguments(command, repo, expected_emxx)
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


def _static_link_input_superset(root: Path) -> dict[str, Any]:
    require(
        root.is_dir() and not _is_reparse_or_symlink(root),
        f"target installed root is missing or a reparse point: {root}",
    )
    link_inputs: list[tuple[str, Path]] = []
    for candidate in root.rglob("*"):
        require(
            not _is_reparse_or_symlink(candidate),
            f"target installed reparse point is forbidden: {candidate}",
        )
        if (
            candidate.is_file()
            and candidate.suffix in {".a", ".o"}
        ):
            link_inputs.append(
                (candidate.relative_to(root).as_posix(), candidate)
            )
        else:
            require(
                candidate.is_dir() or candidate.is_file(),
                f"target installed special entry is forbidden: {candidate}",
            )
    link_inputs.sort(key=lambda item: (item[0].casefold(), item[0]))
    require(
        link_inputs,
        "target installed static-link-input inventory is empty",
    )
    seen: set[str] = set()
    inventory = hashlib.sha256()
    aggregate = hashlib.sha256()
    total_bytes = 0
    files: list[dict[str, Any]] = []
    for relative, link_input in link_inputs:
        key = relative.casefold()
        require(
            key not in seen,
            "duplicate target static-link-input path under case-folding: "
            f"{relative}",
        )
        seen.add(key)
        magic = link_input.read_bytes()[:8]
        if link_input.suffix == ".a":
            require(
                magic != b"!<thin>\n",
                f"thin target static archive is forbidden: {relative}",
            )
            require(
                magic == b"!<arch>\n",
                f"target static archive magic is invalid: {relative}",
            )
            kind = "archive"
        else:
            require(
                magic == b"\x00asm\x01\x00\x00\x00",
                f"target Wasm object magic/version is invalid: {relative}",
            )
            kind = "wasm-object"
        digest = sha256(link_input)
        length = link_input.stat().st_size
        inventory.update(f"{relative}\n".encode("utf-8"))
        aggregate.update(f"{relative}\0{digest}\n".encode("utf-8"))
        total_bytes += length
        files.append(
            {
                "path": relative,
                "kind": kind,
                "bytes": length,
                "sha256": digest,
            }
        )
    return {
        "schemaVersion": 2,
        "algorithm": "sha256-path-null-digest-lf-v1",
        "fileCount": len(files),
        "totalBytes": total_bytes,
        "inventorySha256": inventory.hexdigest(),
        "aggregateSha256": aggregate.hexdigest(),
        "files": files,
    }


def _wasm_sections(data: bytes) -> list[tuple[int, bytes]]:
    require(
        data.startswith(b"\x00asm\x01\x00\x00\x00"),
        "dependency-bound artifact is not a Wasm 1 module",
    )

    def read_uleb(offset: int) -> tuple[int, int]:
        value = 0
        shift = 0
        for _ in range(5):
            require(offset < len(data), "truncated Wasm section size")
            byte = data[offset]
            offset += 1
            value |= (byte & 0x7F) << shift
            if byte & 0x80 == 0:
                return value, offset
            shift += 7
        raise AssertionError("overlong Wasm section size")

    sections: list[tuple[int, bytes]] = []
    offset = 8
    while offset < len(data):
        section_id = data[offset]
        offset += 1
        length, offset = read_uleb(offset)
        end = offset + length
        require(end <= len(data), "truncated Wasm section payload")
        sections.append((section_id, data[offset:end]))
        offset = end
    require(offset == len(data), "Wasm section parse did not consume artifact")
    return sections


def _wasm_custom_sections(data: bytes) -> list[tuple[str, bytes]]:
    custom: list[tuple[str, bytes]] = []
    for section_id, payload in _wasm_sections(data):
        if section_id != 0:
            continue
        value = 0
        shift = 0
        offset = 0
        for _ in range(5):
            require(
                offset < len(payload),
                "truncated Wasm custom-section name length",
            )
            byte = payload[offset]
            offset += 1
            value |= (byte & 0x7F) << shift
            if byte & 0x80 == 0:
                break
            shift += 7
        else:
            raise AssertionError("overlong Wasm custom-section name length")
        end = offset + value
        require(end <= len(payload), "truncated Wasm custom-section name")
        name = payload[offset:end].decode("utf-8")
        custom.append((name, payload[end:]))
    return custom


def _resolve_compile_dependency_path(
    logical: str,
    *,
    repo: Path,
    build: Path,
    repo_inputs: set[str],
) -> Path:
    label, separator, raw_relative = logical.partition("/")
    require(separator == "/" and raw_relative, f"invalid dependency: {logical}")
    relative = _safe_contract_relative_path(
        raw_relative,
        f"compile dependency {logical!r}",
    )
    roots = {
        "build-generated": build,
        "repo-input": repo,
        "vcpkg-target": (
            repo / ".wasm-vcpkg" / "installed" / TARGET_TRIPLET
        ),
        "emsdk": (
            repo
            / ".toolchains"
            / f"emsdk-{EXPECTED_EMSCRIPTEN}"
        ),
        "emscripten-cache": (
            repo
            / ".toolchains"
            / EXPECTED_EMSCRIPTEN_LOCK_ENTRY["cache"]["directory"]
        ),
    }
    require(label in roots, f"unknown compile dependency root: {label}")
    if label == "repo-input":
        require(
            relative in repo_inputs,
            "compile dependency is absent from the explicit input manifest: "
            f"{relative}",
        )
    root = roots[label].resolve()
    candidate = require_no_reparse_chain(
        root / relative,
        f"compile dependency {logical!r}",
    )
    require(
        candidate.is_relative_to(root)
        and candidate.is_file()
        and not _is_reparse_or_symlink(candidate),
        f"compile dependency escaped or is not a file: {candidate}",
    )
    return candidate


def verify_compile_dependency_sidecars(
    repo: Path,
    build: Path,
    qualification: Mapping[str, Any],
    selected_compile_records: Mapping[str, Mapping[str, Any]],
) -> dict[str, Any]:
    manifest_inputs = set(probe_input_identity(repo)["paths"])
    verified: dict[str, dict[str, Any]] = {}
    unique_dependencies: dict[str, tuple[int, str]] = {}
    dependency_occurrences = 0
    aggregate = hashlib.sha256()
    for output_key, command in sorted(
        selected_compile_records.items(),
        key=lambda item: str(item[1]["output"]).casefold(),
    ):
        output = Path(str(command["output"]))
        require(
            path_key(output) == output_key
            and output.is_relative_to(build.resolve()),
            f"selected compile record output drifted: {output}",
        )
        sidecar = output.with_name(
            output.name + ".rg-compile-inputs.json"
        )
        sidecar = require_no_reparse_chain(
            sidecar,
            "selected compile-input sidecar",
        )
        require(
            sidecar.is_file() and not _is_reparse_or_symlink(sidecar),
            f"selected compile-input sidecar is missing: {sidecar}",
        )
        raw = sidecar.read_bytes()
        try:
            payload = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise AssertionError(
                f"invalid selected compile-input sidecar {sidecar}: {error}"
            ) from error
        require(
            isinstance(payload, Mapping),
            f"selected compile-input sidecar is not a mapping: {sidecar}",
        )
        require_exact_keys(
            payload,
            {
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
            },
            f"compile-input sidecar {sidecar.name}",
        )
        require(
            payload["schemaVersion"] == 1
            and payload["algorithm"] == COMPILE_DEPENDENCY_ALGORITHM
            and payload["qualification"] == dict(qualification),
            f"compile-input sidecar contract drifted: {sidecar}",
        )
        driver_kind = command["driverKind"]
        require(
            payload["driverKind"] == driver_kind,
            f"compile-input driver kind drifted: {sidecar}",
        )
        output_relative = output.relative_to(build.resolve()).as_posix()
        require(
            payload["output"] == f"build-output/{output_relative}",
            f"compile-input output identity drifted: {sidecar}",
        )
        expected_arguments = [
            canonical_command(repo, argument)
            for argument in command["arguments"]
        ]
        require(
            payload["arguments"] == expected_arguments,
            f"compile-input argument identity drifted: {sidecar}",
        )
        dependency_discovery = require_mapping(
            payload["dependencyDiscovery"],
            f"compile-input dependency discovery {sidecar.name}",
        )
        require_exact_keys(
            dependency_discovery,
            {
                "preScanMethod",
                "actualCompileMethod",
                "exactPathSetMatch",
                "dependencyCount",
            },
            f"compile-input dependency discovery {sidecar.name}",
        )
        dependencies = payload["dependencies"]
        require(
            isinstance(dependencies, list) and dependencies,
            f"compile-input dependency closure is empty: {sidecar}",
        )
        require(
            dependency_discovery
            == {
                "preScanMethod": "emscripten-M",
                "actualCompileMethod": "MD-MF",
                "exactPathSetMatch": True,
                "dependencyCount": len(dependencies),
            },
            f"compile-input dependency discovery drifted: {sidecar}",
        )
        paths: list[str] = []
        for index, raw_entry in enumerate(dependencies):
            entry = require_mapping(
                raw_entry,
                f"compile-input dependency {sidecar.name}[{index}]",
            )
            require_exact_keys(
                entry,
                {"path", "bytes", "sha256"},
                f"compile-input dependency {sidecar.name}[{index}]",
            )
            logical = entry["path"]
            require(
                isinstance(logical, str),
                f"compile-input dependency path is not a string: {sidecar}",
            )
            dependency = _resolve_compile_dependency_path(
                logical,
                repo=repo,
                build=build,
                repo_inputs=manifest_inputs,
            )
            length, digest = _qualification_file_digest(
                dependency,
                f"compile dependency {logical!r}",
            )
            require(
                type(entry["bytes"]) is int
                and entry["bytes"] >= 0
                and entry["bytes"] == length
                and entry["sha256"] == digest,
                f"compile dependency current bytes drifted: {logical}",
            )
            previous = unique_dependencies.get(logical)
            require(
                previous is None or previous == (length, digest),
                f"compile dependency identity conflicts: {logical}",
            )
            unique_dependencies[logical] = (length, digest)
            paths.append(logical)
            dependency_occurrences += 1
        require(
            paths == sorted(set(paths)),
            f"compile-input dependencies are not unique/sorted: {sidecar}",
        )
        closure_payload = {
            "algorithm": payload["algorithm"],
            "qualification": payload["qualification"],
            "driverKind": payload["driverKind"],
            "output": payload["output"],
            "arguments": payload["arguments"],
            "dependencyDiscovery": payload["dependencyDiscovery"],
            "dependencies": payload["dependencies"],
        }
        closure_bytes = (
            json.dumps(
                closure_payload,
                ensure_ascii=True,
                separators=(",", ":"),
                sort_keys=True,
            )
            + "\n"
        ).encode("utf-8")
        require(
            payload["closureSha256"]
            == hashlib.sha256(closure_bytes).hexdigest(),
            f"compile-input closure digest drifted: {sidecar}",
        )
        output_length, output_digest = _qualification_file_digest(
            output,
            "selected compiled object",
        )
        require(
            payload["outputBytes"] == output_length
            and payload["outputSha256"] == output_digest,
            f"compile-input object bytes drifted: {output}",
        )
        canonical_sidecar = (
            json.dumps(
                dict(payload),
                ensure_ascii=True,
                separators=(",", ":"),
                sort_keys=True,
            )
            + "\n"
        ).encode("utf-8")
        require(
            raw == canonical_sidecar,
            f"compile-input sidecar encoding is not canonical: {sidecar}",
        )
        sidecar_digest = hashlib.sha256(raw).hexdigest()
        aggregate.update(
            (
                f"{output_relative}\0{payload['closureSha256']}\0"
                f"{output_digest}\0{sidecar_digest}\n"
            ).encode("utf-8")
        )
        verified[output_key] = {
            "path": sidecar,
            "payload": dict(payload),
            "sidecarSha256": sidecar_digest,
        }
    require(
        len(verified) == len(selected_compile_records) > 0,
        "selected compile-input sidecar coverage is incomplete",
    )
    return {
        "schemaVersion": 1,
        "algorithm": COMPILE_DEPENDENCY_ALGORITHM,
        "selectedObjectCount": len(verified),
        "dependencyOccurrenceCount": dependency_occurrences,
        "uniqueDependencyCount": len(unique_dependencies),
        "aggregateSha256": aggregate.hexdigest(),
        "qualificationAggregateSha256": qualification["aggregateSha256"],
        "allCurrentDependencyBytesVerified": True,
        "allCurrentObjectBytesVerified": True,
        "_sidecars": verified,
    }


def _archive_wasm_members(data: bytes) -> list[tuple[str, str]]:
    require(data.startswith(b"!<arch>\n"), "build-local archive magic drifted")
    members: list[tuple[str, str]] = []
    string_table = b""
    offset = 8
    while offset < len(data):
        require(offset + 60 <= len(data), "archive member header truncated")
        header = data[offset : offset + 60]
        require(header[58:60] == b"`\n", "archive header trailer drifted")
        try:
            size = int(header[48:58].decode("ascii").strip())
            raw_name = header[:16].decode("ascii").strip()
        except (UnicodeDecodeError, ValueError) as error:
            raise AssertionError("archive member header is invalid") from error
        offset += 60
        end = offset + size
        require(end <= len(data), "archive member body is truncated")
        body = data[offset:end]
        name = raw_name.rstrip("/")
        if raw_name == "//":
            string_table = body
        elif raw_name not in {"/", "/SYM64/"}:
            if raw_name.startswith("#1/"):
                try:
                    name_length = int(raw_name[3:])
                except ValueError as error:
                    raise AssertionError(
                        "BSD archive member name length is invalid"
                    ) from error
                require(
                    name_length <= len(body),
                    "BSD archive member name is truncated",
                )
                name = body[:name_length].decode("utf-8")
                body = body[name_length:]
            elif raw_name.startswith("/") and raw_name[1:].isdigit():
                table_offset = int(raw_name[1:])
                require(
                    table_offset < len(string_table),
                    "GNU archive member name offset is invalid",
                )
                terminator = string_table.find(b"/\n", table_offset)
                require(
                    terminator >= 0,
                    "GNU archive member name is unterminated",
                )
                name = string_table[table_offset:terminator].decode("utf-8")
            if body.startswith(b"\0asm\1\0\0\0"):
                members.append((Path(name).name, hashlib.sha256(body).hexdigest()))
        offset = end + (size % 2)
    return members


def _selected_link_compile_inputs(
    *,
    build: Path,
    static_files: Sequence[tuple[Path, str, bytes]],
    sidecars: Mapping[str, Mapping[str, Any]],
    qualification: Mapping[str, Any],
) -> list[dict[str, Any]]:
    candidates = list(sidecars.items())
    selected: list[dict[str, Any]] = []
    consumed: set[str] = set()
    direct: list[tuple[str, Path, str]] = []
    archive_members: list[tuple[str, str, str]] = []
    for path, kind, data in static_files:
        if not path.is_relative_to(build.resolve()):
            continue
        if kind == "wasm-object":
            direct.append((path_key(path), path, hashlib.sha256(data).hexdigest()))
        else:
            archive = path.relative_to(build.resolve()).as_posix()
            for member_name, digest in _archive_wasm_members(data):
                archive_members.append((archive, member_name, digest))

    for output_key, path, digest in sorted(direct):
        matches = [
            (key, sidecar)
            for key, sidecar in candidates
            if key == output_key
            and sidecar["payload"]["outputSha256"] == digest
        ]
        require(
            len(matches) == 1,
            f"selected direct object has no unique sidecar: {path}",
        )
        key, sidecar = matches[0]
        consumed.add(key)
        payload = sidecar["payload"]
        selected.append(
            {
                "output": payload["output"],
                "closureSha256": payload["closureSha256"],
                "outputSha256": payload["outputSha256"],
                "sidecarSha256": sidecar["sidecarSha256"],
            }
        )
    for archive, member_name, digest in sorted(archive_members):
        matches = [
            (key, sidecar)
            for key, sidecar in candidates
            if key not in consumed
            and Path(str(sidecar["payload"]["output"])).name == member_name
            and sidecar["payload"]["outputSha256"] == digest
        ]
        require(
            len(matches) == 1,
            "selected archive member has no unique sidecar: "
            f"{archive}({member_name})",
        )
        key, sidecar = matches[0]
        consumed.add(key)
        payload = sidecar["payload"]
        require(
            payload["qualification"] == dict(qualification),
            "selected archive-member qualification identity drifted",
        )
        selected.append(
            {
                "archive": archive,
                "member": member_name,
                "output": payload["output"],
                "closureSha256": payload["closureSha256"],
                "outputSha256": payload["outputSha256"],
                "sidecarSha256": sidecar["sidecarSha256"],
            }
        )
    selected.sort(
        key=lambda record: (
            str(record.get("archive", "")),
            str(record.get("member", "")),
            str(record["output"]),
        )
    )
    require(
        selected and len(consumed) == len(sidecars),
        "selected link compile-input coverage differs from the selected "
        "target/archive object closure",
    )
    return selected


def selected_application_link_identity(
    repo: Path,
    build: Path,
    arguments: Sequence[str],
    qualification: Mapping[str, Any],
    sidecars: Mapping[str, Mapping[str, Any]],
) -> dict[str, Any]:
    files: list[dict[str, Any]] = []
    static_files: list[tuple[Path, str, bytes]] = []
    for index, argument in enumerate(arguments):
        if argument.startswith("-"):
            continue
        candidate = Path(argument)
        if not candidate.is_absolute():
            candidate = build / candidate
        candidate = Path(os.path.abspath(candidate))
        if (
            not candidate.is_file()
            or candidate.suffix.casefold() not in {".a", ".o"}
        ):
            continue
        require(
            candidate.is_relative_to(repo.resolve()),
            "selected application static input escaped repository: "
            f"{candidate}",
        )
        data = candidate.read_bytes()
        if candidate.suffix.casefold() == ".a":
            require(
                data.startswith(b"!<arch>\n"),
                f"selected application archive magic drifted: {candidate}",
            )
            kind = "archive"
        else:
            require(
                data.startswith(b"\0asm\1\0\0\0"),
                f"selected application Wasm object magic drifted: {candidate}",
            )
            kind = "wasm-object"
        files.append(
            {
                "argumentIndex": index,
                "path": candidate.relative_to(repo.resolve()).as_posix(),
                "kind": kind,
                "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
        )
        static_files.append((candidate, kind, data))
    require(files, "selected application link has no static inputs")
    compile_inputs = _selected_link_compile_inputs(
        build=build,
        static_files=static_files,
        sidecars=sidecars,
        qualification=qualification,
    )
    payload = {
        "algorithm": SELECTED_LINK_IDENTITY_ALGORITHM,
        "qualification": dict(qualification),
        "arguments": [
            canonical_command(repo, argument)
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
    return {
        "algorithm": payload["algorithm"],
        "sha256": hashlib.sha256(encoded).hexdigest(),
        "argumentCount": len(arguments),
        "staticInputOccurrenceCount": len(files),
        "staticInputUniqueCount": len(
            {entry["path"].casefold() for entry in files}
        ),
        "compileInputCount": len(compile_inputs),
        "qualificationAggregateSha256": qualification["aggregateSha256"],
    }


def verify_selected_link_artifact_binding(
    artifact: Path,
    identity: Mapping[str, Any],
) -> dict[str, Any]:
    digest = str(identity["sha256"])
    build_ids = [
        content
        for name, content in _wasm_custom_sections(artifact.read_bytes())
        if name == "build_id"
    ]
    require(
        len(build_ids) == 1,
        f"expected exactly one Wasm build_id custom section: {len(build_ids)}",
    )
    require(
        build_ids[0] == bytes((32,)) + bytes.fromhex(digest),
        "Wasm build_id does not bind the current selected link argv/inputs",
    )
    return {
        **identity,
        "customSection": "build_id",
        "payloadEncoding": "uleb32-plus-32-byte-sha256",
        "artifactBound": True,
    }


def require_no_dependency_link_indirection(
    arguments: Sequence[str],
) -> None:
    forbidden_prefixes = (
        "@",
        "-Wl,",
        "-Xlinker",
        "-L",
        "-T",
        "--script",
        "--sysroot",
        "-B",
        "-fuse-ld",
        "--ld-path",
        "--config",
        "-vfsoverlay",
        "-ivfsoverlay",
    )
    for argument in arguments:
        require(
            not argument.startswith(forbidden_prefixes),
            "unmodeled dependency-link indirection is forbidden: "
            f"{argument}",
        )


def verify_dependency_archive_binding(
    repo: Path,
    build: Path,
    effective_arguments: Sequence[str],
) -> dict[str, Any]:
    probe = repo / "tools" / "wasm-probe"
    contract_path = probe / "dependency-archive-contract.json"
    generator = probe / "scripts" / "generate_dependency_digest.py"
    generated_source = build / "generated" / "ProbeDependencyDigest.cpp"
    generated_manifest = (
        build / "generated" / "dependency-archive-digest.json"
    )
    artifact = build / "RhythmGameWasmProbe.wasm"
    for path, label in (
        (contract_path, "dependency archive contract"),
        (generator, "dependency archive generator"),
        (generated_source, "generated dependency marker source"),
        (generated_manifest, "generated dependency archive manifest"),
        (artifact, "dependency-bound Wasm artifact"),
    ):
        require(
            path.is_file() and not _is_reparse_or_symlink(path),
            f"{label} is missing or a reparse point: {path}",
        )
    expected_contract = {
        "schemaVersion": 2,
        "algorithm": "sha256-path-null-digest-lf-v1",
        "archiveSuffix": ".a",
        "wasmObjectSuffix": ".o",
        "markerPrefix": (
            "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
        ),
        "targetTriplet": TARGET_TRIPLET,
    }
    require(
        json.loads(contract_path.read_text("utf-8")) == expected_contract,
        "dependency archive contract drifted",
    )
    target_root = repo / ".wasm-vcpkg" / "installed" / TARGET_TRIPLET
    superset = _static_link_input_superset(target_root)
    manifest = json.loads(generated_manifest.read_text("utf-8"))
    require(
        manifest == superset,
        "generated dependency archive manifest does not match current bytes",
    )
    by_path = {entry["path"]: entry for entry in superset["files"]}

    installed_occurrences: list[dict[str, Any]] = []
    system_libraries: list[str] = []
    boundary_count = 0
    build_local_object_count = 0
    allowed_system_libraries = {
        "-lembind",
        "-lwebsocket.js",
        "-lopenal",
    }
    require_no_dependency_link_indirection(effective_arguments)
    operand_options = {
        "-I",
        "-L",
        "-D",
        "-U",
        "-include",
        "-include-pch",
        "-imacros",
        "-isystem",
        "-iquote",
        "-mllvm",
        "-o",
        "-s",
        "-target",
        "--target",
        "-x",
    }
    skip_operand = False
    build_root = build.resolve()
    target_root_resolved = target_root.resolve()
    boundary_archive = (build / "libWasmProbeExceptionBoundary.a").resolve()
    for argument in effective_arguments:
        if skip_operand:
            skip_operand = False
            continue
        if argument in operand_options:
            skip_operand = True
            continue
        if argument.startswith("-l"):
            require(
                argument in allowed_system_libraries,
                f"unknown application-link system library: {argument}",
            )
            system_libraries.append(argument)
            continue
        if argument.startswith("-") or argument == "":
            continue
        parts = Path(argument).parts
        require(
            all(
                component not in {"", ".", ".."}
                and not component.endswith((" ", "."))
                for component in parts
            ),
            f"unsafe application-link positional path: {argument}",
        )
        lexical = Path(argument)
        if not lexical.is_absolute():
            lexical = build / lexical
        lexical = require_no_reparse_chain(
            lexical,
            "application-link positional input",
        )
        require(
            lexical.exists(),
            f"application-link positional input is missing: {argument}",
        )
        require(
            lexical.is_file(),
            f"application-link positional input is not a file: {argument}",
        )
        resolved = lexical.resolve(strict=True)
        magic = resolved.read_bytes()[:8]
        require(
            magic != b"!<thin>\n",
            f"thin application-link archive is forbidden: {argument}",
        )
        if resolved.suffix == ".a":
            require(
                magic == b"!<arch>\n",
                "application-link static archive magic is invalid: "
                f"{argument}",
            )
            kind = "archive"
        elif resolved.suffix == ".o":
            require(
                magic == b"\x00asm\x01\x00\x00\x00",
                "application-link Wasm object magic/version is invalid: "
                f"{argument}",
            )
            kind = "wasm-object"
        else:
            require(
                magic not in {
                    b"!<arch>\n",
                    b"\x00asm\x01\x00\x00\x00",
                },
                "renamed application-link static input is forbidden: "
                f"{argument}",
            )
            raise AssertionError(
                "unclassified application-link positional file: "
                f"{resolved}"
            )
        if resolved == boundary_archive:
            require(
                kind == "archive",
                "build-local exception boundary is not an archive",
            )
            boundary_count += 1
            continue
        if resolved.is_relative_to(build_root):
            require(
                kind == "wasm-object",
                "unmodeled build-local archive is forbidden: "
                f"{resolved}",
            )
            build_local_object_count += 1
            continue
        require(
            resolved.is_relative_to(target_root_resolved),
            "external application-link static input is forbidden: "
            f"{resolved}",
        )
        relative = resolved.relative_to(target_root_resolved).as_posix()
        entry = by_path.get(relative)
        require(
            entry is not None,
            "linked static input is absent from authenticated superset: "
            f"{relative}",
        )
        require(
            entry["kind"] == kind
            and resolved.suffix
            == (".a" if entry["kind"] == "archive" else ".o")
            and
            resolved.stat().st_size == entry["bytes"]
            and sha256(resolved) == entry["sha256"],
            f"linked static input bytes/kind drifted from superset: {relative}",
        )
        installed_occurrences.append(dict(entry))
    require(
        not skip_operand,
        "application-link option is missing its positional operand",
    )
    require(boundary_count == 1, "build-local exception boundary count drifted")
    require(
        installed_occurrences,
        "application link contains no installed static link inputs",
    )
    require(
        build_local_object_count > 0,
        "application link contains no build-local Wasm objects",
    )
    ordered = hashlib.sha256()
    for entry in installed_occurrences:
        ordered.update(
            f"{entry['path']}\0{entry['sha256']}\n".encode("utf-8")
        )
    unique_by_path = {
        entry["path"]: entry
        for entry in installed_occurrences
    }
    unique_entries = [
        unique_by_path[path]
        for path in sorted(unique_by_path)
    ]
    unique = hashlib.sha256()
    for entry in unique_entries:
        unique.update(
            f"{entry['path']}\0{entry['sha256']}\n".encode("utf-8")
        )

    marker = expected_contract["markerPrefix"] + superset["aggregateSha256"]
    source = generated_source.read_text("utf-8")
    require(
        source.count(marker) == 1,
        "generated dependency marker source is missing or duplicated",
    )
    marker_bytes = marker.encode("ascii")
    wasm = artifact.read_bytes()
    sections = _wasm_sections(wasm)
    data_occurrences = sum(
        payload.count(marker_bytes)
        for section_id, payload in sections
        if section_id == 11
    )
    all_occurrences = wasm.count(marker_bytes)
    require(
        data_occurrences == 1 and all_occurrences == 1,
        "dependency archive marker is not exactly once in the Wasm data section",
    )
    return {
        "contract": {
            "path": relative_path(repo, contract_path),
            "sha256": sha256(contract_path),
        },
        "generator": {
            "path": relative_path(repo, generator),
            "sha256": sha256(generator),
        },
        "manifest": {
            "path": relative_path(repo, generated_manifest),
            "sha256": sha256(generated_manifest),
        },
        "superset": {
            key: superset[key]
            for key in (
                "algorithm",
                "fileCount",
                "totalBytes",
                "inventorySha256",
                "aggregateSha256",
            )
        },
        "linkedClosure": {
            "occurrenceCount": len(installed_occurrences),
            "uniqueFileCount": len(unique_entries),
            "uniqueBytes": sum(entry["bytes"] for entry in unique_entries),
            "orderedAggregateSha256": ordered.hexdigest(),
            "uniqueAggregateSha256": unique.hexdigest(),
            "archives": installed_occurrences,
            "systemLibraries": system_libraries,
            "buildLocalArchive": "libWasmProbeExceptionBoundary.a",
            "buildLocalWasmObjectCount": build_local_object_count,
        },
        "marker": {
            "value": marker,
            "source": relative_path(repo, generated_source),
            "wasmDataSectionOccurrences": data_occurrences,
        },
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
    contract: Mapping[str, Any],
) -> dict[str, Any]:
    require(
        dict(contract) == EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY,
        "vcpkg port-build CMake lock entry drifted",
    )
    manifest_path = vcpkg / contract["toolsManifest"]
    require(
        manifest_path.is_file(),
        f"missing pinned vcpkg tools manifest: {manifest_path}",
    )
    manifest_sha256 = sha256(manifest_path)
    require(
        manifest_sha256 == contract["toolsManifestSha256"],
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
        / contract["installationDirectory"]
    )
    archive = downloads / entry["archive"]
    require(
        archive.is_file()
        and not _is_reparse_or_symlink(archive)
        and archive.stat().st_size == contract["archiveBytes"],
        "vcpkg port-build CMake archive bytes/path/type drifted",
    )
    installation = verify_authenticated_build_tool(
        repo,
        "vcpkgPortBuildCMake",
        {
            "version": contract["version"],
            "url": contract["url"],
            "sha512": contract["sha512"],
            "archiveFile": contract["archiveFile"],
            "executableSha256": contract["executableSha256"],
            "directory": contract["installationDirectory"],
            "payload": contract["payload"],
        },
        tool_root,
        archive_root=downloads,
    )
    executable = (tool_root / Path(entry["executable"])).resolve()
    require(
        executable.is_relative_to(tool_root),
        "vcpkg port-build CMake executable escapes its fixed tool root",
    )
    require(
        executable.is_file(),
        f"missing vcpkg port-build CMake executable: {executable}",
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
            "bytes": archive.stat().st_size,
        },
        "installation": installation,
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
        "EM_FROZEN_CACHE",
        "VCPKG_DEFAULT_BINARY_CACHE",
        "VCPKG_DISABLE_METRICS",
        "VCPKG_MAX_CONCURRENCY",
        "VCPKG_ROOT",
        "RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER",
        "RHYTHMGAME_WASM_TOOLCHAIN_LOCK",
        "RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR",
        "RHYTHMGAME_EMSCRIPTEN_ROOT",
        "RHYTHMGAME_EM_CONFIG",
        "RHYTHMGAME_EM_CONFIG_SHA256",
        "RHYTHMGAME_EM_CACHE",
        "RHYTHMGAME_WASM_QUALIFICATION",
        "RHYTHMGAME_WASM_QUALIFICATION_ALGORITHM",
        "RHYTHMGAME_WASM_QUALIFICATION_FILE_COUNT",
        "RHYTHMGAME_WASM_QUALIFICATION_TOTAL_BYTES",
        "RHYTHMGAME_WASM_QUALIFICATION_INVENTORY_SHA256",
        "RHYTHMGAME_WASM_QUALIFICATION_AGGREGATE_SHA256",
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
        "QML_",
        "QT_",
        "RHYTHMGAME_",
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
    system_directory = Path(os.environ.get("SystemRoot", "")) / "System32"
    native_cmd = system_directory / "cmd.exe"
    require_same_path(
        environment.get("COMSPEC", ""),
        native_cmd,
        "native ComSpec",
    )
    require(
        system_directory.is_dir()
        and native_cmd.is_file()
        and not _is_reparse_or_symlink(native_cmd),
        "native Windows command interpreter identity drifted",
    )

    expected_paths = {
        "EMSDK": emsdk,
        "EMSDK_NODE": repo / EXPECTED_EMSDK_NODE,
        "EMSDK_PYTHON": repo / EXPECTED_EMSDK_PYTHON,
        "EMSCRIPTEN_ROOT": emsdk / "upstream" / "emscripten",
        "EM_CONFIG": emsdk / ".emscripten",
        "EM_CACHE": repo / ".toolchains" / f"emscripten-cache-{EXPECTED_EMSCRIPTEN}",
        "VCPKG_ROOT": vcpkg,
        "VCPKG_DEFAULT_BINARY_CACHE": (
            repo / ".wasm-vcpkg" / "bincache"
        ),
        "RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER": (
            repo
            / "tools"
            / "wasm-probe"
            / "scripts"
            / "invoke_emscripten_driver.py"
        ),
        "RHYTHMGAME_WASM_TOOLCHAIN_LOCK": (
            repo / "tools" / "wasm-probe" / "toolchain-lock.json"
        ),
        "RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR": (
            repo
            / "tools"
            / "wasm-probe"
            / "scripts"
            / "audit_emscripten_response_files.py"
        ),
        "RHYTHMGAME_EMSCRIPTEN_ROOT": emsdk / "upstream" / "emscripten",
        "RHYTHMGAME_EM_CONFIG": emsdk / ".emscripten",
        "RHYTHMGAME_EM_CACHE": (
            repo
            / ".toolchains"
            / f"emscripten-cache-{EXPECTED_EMSCRIPTEN}"
        ),
    }
    canonical: dict[str, str] = {}
    for name, expected in expected_paths.items():
        require_same_path(environment.get(name, ""), expected, name)
        canonical[name] = relative_path(repo, expected)
    expected_values = {
        "EMSCRIPTEN_VERSION": EXPECTED_EMSCRIPTEN,
        "VCPKG_DISABLE_METRICS": "1",
        "VCPKG_MAX_CONCURRENCY": str(
            EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY[
                "vcpkgMaxConcurrency"
            ]
        ),
        "CMAKE_NINJA_FORCE_RESPONSE_FILE": "1",
        "PYTHONDONTWRITEBYTECODE": "1",
        "PYTHONNOUSERSITE": "1",
        "EM_FROZEN_CACHE": "1",
        "SOURCE_DATE_EPOCH": str(EXPECTED_SOURCE_DATE_EPOCH),
    }
    for name, expected in expected_values.items():
        require(
            environment.get(name) == expected,
            f"canonical environment {name} drifted",
        )
    canonical.update(expected_values)
    em_config = expected_paths["EM_CONFIG"]
    require(
        environment.get("RHYTHMGAME_EM_CONFIG_SHA256") == sha256(em_config),
        "canonical RHYTHMGAME_EM_CONFIG_SHA256 drifted",
    )
    canonical["RHYTHMGAME_EM_CONFIG_SHA256"] = sha256(em_config)
    return {
        "forbiddenNamesAbsent": list(FORBIDDEN_BUILD_ENVIRONMENT_NAMES),
        "scrubbedVariableFamilies": list(hostile_prefixes),
        "canonicalVariables": canonical,
        "nativeCommandInterpreterAuthenticated": True,
    }


def verify_toolchains(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
) -> dict[str, Any]:
    lock_path = repo / "tools" / "wasm-probe" / "toolchain-lock.json"
    lock = json.loads(lock_path.read_text("utf-8"))
    require(
        lock.get("gateTools") == EXPECTED_GATE_TOOLS_LOCK_ENTRY,
        "Gate tool lock drift",
    )
    require(
        lock.get("reproducibleBuild")
        == EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY,
        "reproducible build lock drift",
    )
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
    require_exact_keys(
        lock["vcpkg"],
        {
            "baseline",
            "sourceArchive",
            "emscriptenWrapperSourceCommit",
            "bootstrapLauncher",
            "bootstrapLauncherSha256",
            "bootstrapScript",
            "bootstrapScriptSha256",
            "toolMetadata",
            "toolMetadataSha256",
            "toolReleaseTag",
            "toolUrl",
            "executable",
            "executableSha256",
            "portBuildCMake",
        },
        "vcpkg",
    )
    require(
        lock["vcpkg"]["bootstrapLauncher"] == "bootstrap-vcpkg.bat"
        and lock["vcpkg"]["bootstrapLauncherSha256"]
        == EXPECTED_VCPKG_BOOTSTRAP_LAUNCHER_SHA256
        and lock["vcpkg"]["sourceArchive"]
        == EXPECTED_VCPKG_SOURCE_ARCHIVE
        and lock["vcpkg"]["bootstrapScript"] == "scripts/bootstrap.ps1"
        and lock["vcpkg"]["bootstrapScriptSha256"]
        == EXPECTED_VCPKG_BOOTSTRAP_SCRIPT_SHA256
        and lock["vcpkg"]["toolMetadata"]
        == "scripts/vcpkg-tool-metadata.txt"
        and lock["vcpkg"]["toolMetadataSha256"]
        == EXPECTED_VCPKG_TOOL_METADATA_SHA256
        and lock["vcpkg"]["toolReleaseTag"]
        == EXPECTED_VCPKG_TOOL_RELEASE_TAG
        and lock["vcpkg"]["toolUrl"] == EXPECTED_VCPKG_TOOL_URL
        and lock["vcpkg"]["executable"] == "vcpkg.exe"
        and lock["vcpkg"]["executableSha256"]
        == EXPECTED_VCPKG_EXECUTABLE_SHA256
        and lock["vcpkg"]["portBuildCMake"]
        == EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY,
        "vcpkg executable/port-build CMake lock drift",
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
    expected_driver_adapter = (
        repo
        / "tools"
        / "wasm-probe"
        / "scripts"
        / "invoke_emscripten_driver.py"
    )
    expected_response_auditor = (
        repo
        / "tools"
        / "wasm-probe"
        / "scripts"
        / "audit_emscripten_response_files.py"
    )
    expected_emsdk_python = repo / EXPECTED_EMSDK_PYTHON
    expected_emsdk_node = repo / EXPECTED_EMSDK_NODE

    require_same_path(
        sys.executable,
        expected_emsdk_python,
        "evidence verifier Python",
    )
    require(
        sys.flags.isolated == 1
        and sys.flags.ignore_environment == 1
        and sys.flags.no_user_site == 1
        and sys.flags.dont_write_bytecode == 1,
        "evidence verifier Python must run with -I -B",
    )
    require_same_path(emsdk, expected_emsdk, "emsdk argument")
    require_same_path(vcpkg, expected_vcpkg, "vcpkg argument")
    for path in (
        expected_outer_cmake,
        expected_ninja,
        expected_vcpkg_exe,
        expected_emxx,
        expected_emcc,
        expected_emxx_driver,
        expected_driver_adapter,
        expected_response_auditor,
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

    source_archives = {
        "emsdk": verify_authenticated_source_archive(
            repo,
            "emsdk",
            lock["emscripten"]["sourceArchive"],
            emsdk,
        ),
        "vcpkg": verify_authenticated_source_archive(
            repo,
            "vcpkg",
            lock["vcpkg"]["sourceArchive"],
            vcpkg,
        ),
    }
    bootstrap_python_contract = lock["emscripten"]["bootstrapPython"]
    bootstrap_python_source_contract = {
        name: bootstrap_python_contract[name]
        for name in (
            "url",
            "archiveFile",
            "sha256",
            "payload",
            "allowedRuntimePrefixes",
            "allowedRuntimeFiles",
        )
    }
    bootstrap_python_root = (
        emsdk / bootstrap_python_contract["installationDirectory"]
    )
    bootstrap_python_source = verify_authenticated_source_archive(
        repo,
        "emsdk bootstrap Python",
        bootstrap_python_source_contract,
        bootstrap_python_root,
    )
    bootstrap_python_executable = (
        bootstrap_python_root / bootstrap_python_contract["executable"]
    )
    bootstrap_python_sha256 = verify_locked_executable(
        bootstrap_python_executable,
        bootstrap_python_contract["executableSha256"],
        "emsdk bootstrap Python executable",
    )
    bootstrap_launchers = {
        "emsdk": {
            "path": lock["emscripten"]["bootstrapScript"],
            "sha256": verify_locked_executable(
                emsdk / lock["emscripten"]["bootstrapScript"],
                lock["emscripten"]["bootstrapScriptSha256"],
                "emsdk bootstrap script",
            ),
        },
        "vcpkg": {
            "path": lock["vcpkg"]["bootstrapLauncher"],
            "sha256": verify_locked_executable(
                vcpkg / lock["vcpkg"]["bootstrapLauncher"],
                lock["vcpkg"]["bootstrapLauncherSha256"],
                "vcpkg bootstrap launcher",
            ),
        },
    }
    vcpkg_bootstrap_inputs = {
        "script": {
            "path": lock["vcpkg"]["bootstrapScript"],
            "sha256": verify_locked_executable(
                vcpkg / lock["vcpkg"]["bootstrapScript"],
                lock["vcpkg"]["bootstrapScriptSha256"],
                "vcpkg bootstrap implementation",
            ),
        },
        "toolMetadata": {
            "path": lock["vcpkg"]["toolMetadata"],
            "sha256": verify_locked_executable(
                vcpkg / lock["vcpkg"]["toolMetadata"],
                lock["vcpkg"]["toolMetadataSha256"],
                "vcpkg tool metadata",
            ),
        },
        "toolReleaseTag": lock["vcpkg"]["toolReleaseTag"],
        "toolUrl": lock["vcpkg"]["toolUrl"],
    }
    emscripten_installation = verify_emscripten_installation(
        emsdk,
        EXPECTED_EMSCRIPTEN,
        lock["emscripten"],
    )
    cache_contract = require_mapping(
        lock["emscripten"]["cache"],
        "emscripten.cache",
    )
    expected_cache = (
        repo / ".toolchains" / cache_contract["directory"]
    )
    cache_identity = verify_emscripten_cache_payload(
        repo,
        expected_cache,
        require_mapping(cache_contract["payload"], "emscripten.cache.payload"),
        "Emscripten frozen cache",
    )
    cache_evidence = {
        "directory": cache_contract["directory"],
        "initializer": cache_contract["initializer"],
        "prewarmCores": cache_contract["prewarmCores"],
        "compilerPathPrefixMap": dict(
            cache_contract["compilerPathPrefixMap"]
        ),
        "frozenEnvironment": cache_contract["frozenEnvironment"],
        "volatileProducts": list(cache_contract["volatileProducts"]),
        **cache_identity,
    }
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
    vcpkg_executable_sha256 = verify_locked_executable(
        expected_vcpkg_exe,
        lock["vcpkg"]["executableSha256"],
        "vcpkg executable",
    )
    emcc_launcher_sha256 = verify_locked_executable(
        expected_emcc,
        lock["emscripten"]["cLauncherSha256"],
        "emcc launcher",
    )
    emxx_launcher_sha256 = verify_locked_executable(
        expected_emxx,
        lock["emscripten"]["cxxLauncherSha256"],
        "em++ launcher",
    )
    driver_paths = {
        "emccPySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "emcc.py",
        "emxxPySha256": expected_emxx_driver,
        "emarLauncherSha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "emar.bat",
        "emarPySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "emar.py",
        "emranlibLauncherSha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "emranlib.bat",
        "emranlibPySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "emranlib.py",
        "sharedPySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "tools"
        / "shared.py",
        "responseFilePySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "tools"
        / "response_file.py",
        "configPySha256": expected_emsdk
        / "upstream"
        / "emscripten"
        / "tools"
        / "config.py",
    }
    driver_api = {
        name: {
            "path": relative_path(repo, path),
            "sha256": verify_locked_executable(
                path,
                lock["emscripten"]["driverApi"][name],
                f"Emscripten driver API {name}",
            ),
        }
        for name, path in driver_paths.items()
    }
    driver_api["pythonImportClosure"] = (
        verify_emscripten_python_import_closure(
            expected_emsdk / "upstream" / "emscripten",
            require_mapping(
                lock["emscripten"]["driverApi"]["pythonImportClosure"],
                "emscripten.driverApi.pythonImportClosure",
            ),
        )
    )
    node_sha256 = verify_locked_executable(
        expected_emsdk_node,
        lock["emscripten"]["nodeExecutableSha256"],
        "Emscripten Node executable",
    )
    response_auditor_sha256 = verify_locked_executable(
        expected_response_auditor,
        lock["gateTools"]["responseAuditorSha256"],
        "response-file auditor",
    )
    driver_adapter_sha256 = verify_locked_executable(
        expected_driver_adapter,
        lock["gateTools"]["adapterSha256"],
        "Emscripten compiler adapter",
    )
    # No version-bearing executable is run until the full Emscripten, CMake,
    # and Ninja byte inventories have all passed.
    vcpkg_port_cmake = verify_vcpkg_port_build_cmake(
        repo,
        vcpkg,
        lock["vcpkg"]["portBuildCMake"],
    )

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
    em_config = expected_emsdk / ".emscripten"
    emxx_version = run_text(
        emsdk_python,
        "-I",
        "-B",
        expected_driver_adapter,
        "--lock",
        lock_path,
        "--auditor",
        expected_response_auditor,
        "--emscripten-root",
        expected_emxx.parent,
        "--driver-kind",
        "em++",
        "--em-config",
        em_config,
        "--em-config-sha256",
        sha256(em_config),
        "--cache-root",
        expected_cache,
        "--",
        expected_emxx,
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
        "reproducibleBuild": dict(
            EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY
        ),
        "emscripten": {
            "version": EXPECTED_EMSCRIPTEN,
            "versionOutput": first_line(emxx_version),
            "emsdkCommit": EXPECTED_EMSDK_COMMIT,
            "emsdkRoot": relative_path(repo, expected_emsdk),
            "python": relative_path(repo, emsdk_python),
            "verifierPython": {
                "path": relative_path(repo, Path(sys.executable)),
                "sha256": bootstrap_python_sha256,
                "isolated": True,
                "bytecodeDisabled": True,
            },
            "node": relative_path(repo, expected_emsdk_node),
            "nodeSha256": node_sha256,
            "launcher": relative_path(repo, expected_emxx),
            "cLauncherSha256": emcc_launcher_sha256,
            "cxxLauncherSha256": emxx_launcher_sha256,
            "driver": relative_path(repo, expected_emxx_driver),
            "driverAdapter": {
                "path": relative_path(repo, expected_driver_adapter),
                "sha256": driver_adapter_sha256,
            },
            "driverApi": driver_api,
            "responseAuditor": {
                "path": relative_path(repo, expected_response_auditor),
                "sha256": response_auditor_sha256,
            },
            "sourceArchive": source_archives["emsdk"],
            "bootstrapPython": {
                "sourceArchive": bootstrap_python_source,
                "executable": {
                    "path": relative_path(
                        repo,
                        bootstrap_python_executable,
                    ),
                    "sha256": bootstrap_python_sha256,
                },
            },
            "installation": emscripten_installation,
            "cache": cache_evidence,
        },
        "vcpkg": {
            "baselineCommit": EXPECTED_VCPKG_COMMIT,
            "versionOutput": first_line(vcpkg_version),
            "executable": relative_path(repo, expected_vcpkg_exe),
            "executableSha256": vcpkg_executable_sha256,
            "bootstrapLauncher": bootstrap_launchers["vcpkg"],
            "bootstrapInputs": vcpkg_bootstrap_inputs,
            "sourceArchive": source_archives["vcpkg"],
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
        "bootstrapLaunchers": bootstrap_launchers,
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
    target_passthrough = require_vcpkg_env_passthrough(
        target_triplet,
        ("EMSDK", "EMSDK_PYTHON"),
    )
    require(
        tuple(target_passthrough) == EXPECTED_TARGET_ENV_PASSTHROUGH,
        "target triplet VCPKG_ENV_PASSTHROUGH drifted: "
        f"{target_passthrough}",
    )
    host_passthrough = require_vcpkg_env_passthrough(
        host_triplet,
        EXPECTED_HOST_ENV_PASSTHROUGH,
    )
    require(
        tuple(host_passthrough) == EXPECTED_HOST_ENV_PASSTHROUGH,
        "host triplet VCPKG_ENV_PASSTHROUGH drifted: "
        f"{host_passthrough}",
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
        "targetTripletEnvironmentPassthrough": target_passthrough,
        "hostTripletEnvironmentPassthrough": host_passthrough,
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
    authenticated_launchers: dict[str, str] = {}
    for cache_name, driver_kind in (
        ("CMAKE_C_COMPILER_LAUNCHER", "emcc"),
        ("CMAKE_CXX_COMPILER_LAUNCHER", "em++"),
        ("CMAKE_C_LINKER_LAUNCHER", "emcc"),
        ("CMAKE_CXX_LINKER_LAUNCHER", "em++"),
    ):
        launcher = probe_cache.get(cache_name, "").split(";")
        require(
            launcher and all(launcher),
            f"probe {cache_name} is missing/empty",
        )
        compiler = (
            repo
            / ".toolchains"
            / f"emsdk-{EXPECTED_EMSCRIPTEN}"
            / "upstream"
            / "emscripten"
            / f"{driver_kind}.bat"
        )
        effective = authenticated_adapter_compiler_arguments(
            [*launcher, str(compiler)],
            repo,
            driver_kind,
            f"probe {cache_name}",
        )
        require(
            len(effective) == 1,
            f"probe {cache_name} contains post-separator arguments",
        )
        authenticated_launchers[cache_name] = driver_kind
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
        "authenticatedLaunchers": authenticated_launchers,
        "qtTargetCCompiler": relative_path(repo, qt_c_compiler),
        "qtTargetCxxCompiler": relative_path(repo, qt_cxx_compiler),
        "qtTargetCompilerVersion": qt_cxx_version,
    }


def verify_compile_commands(
    repo: Path,
    build: Path,
    buildtrees: Path,
    ninja: Path,
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
    probe_launcher_counts = {"c": 0, "cxx": 0}
    audited_probe_commands: dict[str, dict[str, Any]] = {}
    for entry in entries:
        require(isinstance(entry, dict), "invalid probe compile entry")
        compiler_arguments = compile_entry_arguments(entry)
        raw_compiler = Path(compiler_arguments[0])
        if path_key(raw_compiler) == path_key(expected_emxx):
            language = "cxx"
        elif path_key(raw_compiler) == path_key(expected_emcc):
            language = "c"
        else:
            raise AssertionError(
                "probe compile entry has a non-pinned compiler: "
                f"{compiler_arguments[0]}"
            )
        contract = require_wasm_compile_contract(
            compiler_arguments,
            language=language,
            context=f"probe compile: {entry.get('file')}",
        )
        output = compile_entry_output(
            entry,
            build,
            f"probe compile: {entry.get('file')}",
        )
        output_key = path_key(output)
        source_key = path_key(Path(str(entry.get("file", ""))))
        require(
            output_key not in audited_probe_commands,
            f"duplicate probe compile output: {output}",
        )
        audited_probe_commands[output_key] = {
            "arguments": compiler_arguments,
            "contract": contract,
            "language": language,
            "output": output,
            "sourceKey": source_key,
        }

    expanded_compdb = json.loads(
        run_text(ninja, "-C", build, "-t", "compdb", "-x")
    )
    require(
        isinstance(expanded_compdb, list) and expanded_compdb,
        "expanded Ninja compilation database is empty",
    )
    launched_outputs: set[str] = set()
    actual_probe_commands: dict[str, dict[str, Any]] = {}
    for entry in expanded_compdb:
        require(
            isinstance(entry, Mapping),
            "expanded Ninja compdb entry is not a mapping",
        )
        line = entry.get("command")
        require(
            isinstance(line, str),
            "expanded Ninja compdb entry command is not a string",
        )
        # `ninja -t compdb -x` includes phony/order-only edges with an empty
        # command.  They are not compiler invocations.  The exact source-set
        # equality below still requires every compile_commands.json source to
        # have one authenticated, non-empty Ninja compile edge.
        if not line:
            continue
        folded = line.casefold()
        if "emcc.bat" not in folded and "em++.bat" not in folded:
            continue
        launcher_arguments = split_windows_command_line(line)
        if "-c" not in launcher_arguments:
            continue
        driver_positions = [
            index
            for index, argument in enumerate(launcher_arguments[:-1])
            if argument == "--driver-kind"
        ]
        require(
            len(driver_positions) == 1,
            "probe Ninja compile edge bypassed the authenticated "
            f"driver adapter: {line}",
        )
        driver_kind = launcher_arguments[driver_positions[0] + 1]
        require(
            driver_kind in {"emcc", "em++"},
            f"probe Ninja compile edge has invalid driver: {driver_kind}",
        )
        compiler_arguments = authenticated_adapter_compiler_arguments(
            launcher_arguments,
            repo,
            driver_kind,
            "probe Ninja compile edge",
        )
        require(
            compiler_arguments.count("-c") == 1,
            "probe Ninja compile edge must contain one -c",
        )
        compile_index = compiler_arguments.index("-c")
        require(
            compile_index == len(compiler_arguments) - 2,
            "probe Ninja compile source must immediately follow -c",
        )
        output = compile_entry_output(
            entry,
            build,
            "expanded probe Ninja compile edge",
        )
        output_key = path_key(output)
        source_key = path_key(Path(compiler_arguments[compile_index + 1]))
        audited = audited_probe_commands.get(output_key)
        require(
            audited is not None,
            "probe Ninja launcher output is absent from compile DB: "
            f"{output}",
        )
        require(
            source_key == audited["sourceKey"],
            "probe Ninja launcher source does not match the compile DB "
            f"entry for output {output}",
        )
        require(
            output_key not in launched_outputs,
            "duplicate authenticated probe Ninja compile output: "
            f"{output}",
        )
        language = str(audited["language"])
        require(
            driver_kind == ("emcc" if language == "c" else "em++"),
            "probe Ninja launcher language/driver mismatch: "
            f"{compiler_arguments[compile_index + 1]}",
        )
        database_arguments = list(audited["arguments"])
        parity_arguments = require_compile_argv_parity(
            database_arguments,
            compiler_arguments,
            f"probe Ninja compile: {compiler_arguments[compile_index + 1]}",
        )
        actual_contract = require_wasm_compile_contract(
            parity_arguments,
            language=language,
            context=(
                "expanded probe Ninja compile: "
                f"{compiler_arguments[compile_index + 1]}"
            ),
        )
        actual_probe_commands[output_key] = {
            # The adapter receives argv after the pinned emcc/em++ launcher.
            "arguments": compiler_arguments[1:],
            "parityArguments": parity_arguments,
            "contract": actual_contract,
            "driverKind": driver_kind,
            "language": language,
            "output": output,
            "sourceKey": source_key,
        }
        launched_outputs.add(output_key)
        probe_launcher_counts[language] += 1
    require(
        launched_outputs == set(audited_probe_commands),
        "not every probe compile DB output has one exact authenticated "
        "Ninja launcher edge",
    )
    selected_graph = selected_application_compile_outputs(
        build,
        (build / "build.ninja").read_text("utf-8", errors="replace"),
    )
    selected_object_paths = dict(selected_graph.pop("_objectPaths"))
    require_selected_compile_output_correlation(
        selected_object_paths,
        audited_probe_commands,
        actual_probe_commands,
    )
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
        output = compile_entry_output(
            matching[0],
            build,
            f"{source_name} compile entry",
        )
        command = actual_probe_commands[path_key(output)]
        arguments = list(command["parityArguments"])
        contract = command["contract"]
        require_same_path(
            arguments[0],
            expected_emxx,
            f"{source_name} compiler",
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
        "probeAdapterCommandCounts": probe_launcher_counts,
        "probeCompileDbNinjaParity": {
            "expandedCommandSource": "ninja -t compdb -x",
            "matchedCommandCount": len(launched_outputs),
            "correlationKey": "directory-plus-output",
            "dependencyBookkeepingRemoved": ["-MD", "-MT", "-MF"],
            "exactAfterPathNormalization": True,
        },
        "selectedTargetGraph": selected_graph,
        "exceptionBoundary": boundary,
        "_selectedCompileRecords": {
            key: actual_probe_commands[key]
            for key in selected_object_paths
        },
    }


def verify_application_link(
    repo: Path,
    build: Path,
    ninja: Path,
    qualification: Mapping[str, Any],
    selected_compile_records: Mapping[str, Mapping[str, Any]],
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
        repo,
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
    dependency_archives = verify_dependency_archive_binding(
        repo,
        build,
        stream["effectiveArguments"][1:],
    )
    compile_dependencies = verify_compile_dependency_sidecars(
        repo,
        build,
        qualification,
        selected_compile_records,
    )
    sidecars = dict(compile_dependencies.pop("_sidecars"))
    selected_link_binding = verify_selected_link_artifact_binding(
        build / "RhythmGameWasmProbe.wasm",
        selected_application_link_identity(
            repo,
            build,
            stream["effectiveArguments"][1:],
            qualification,
            sidecars,
        ),
    )
    c_launcher_probe = verify_c_launcher_link(repo, build, ninja)
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
        "dependencyArchives": dependency_archives,
        "compileDependencyClosure": compile_dependencies,
        "selectedLinkArtifactBinding": selected_link_binding,
        "effectiveSettings": setting_contract["effectiveValues"],
        "settingOccurrences": setting_contract["occurrences"],
        "literalAsyncifyConfigured": False,
        "staticExceptionArchive": archive,
        "staticExceptionArchiveLinked": True,
        "adapterAuthenticated": True,
        "cLauncherProbe": c_launcher_probe,
        "commandSha256": sha256_text(
            canonical_command(repo, application_link)
        ),
        "linkLibrariesSha256": sha256_text(
            canonical_command(repo, bindings["LINK_LIBRARIES"])
        ),
    }


def verify_c_launcher_link(
    repo: Path,
    build: Path,
    ninja: Path,
) -> dict[str, Any]:
    target = "RhythmGameWasmCLauncherProbe"
    commands = run_text(ninja, "-C", build, "-t", "commands", target)
    links = [
        line
        for line in commands.splitlines()
        if f"{target}.js" in line and "emcc.bat" in line
    ]
    require(len(links) == 1, f"expected one C launcher link: {links}")
    expected_emcc = (
        repo
        / ".toolchains"
        / f"emsdk-{EXPECTED_EMSCRIPTEN}"
        / "upstream"
        / "emscripten"
        / "emcc.bat"
    )
    arguments = parse_adapter_link_arguments(
        links[0],
        repo,
        expected_emcc,
        "emcc",
        "C launcher probe link",
    )
    require(
        "-c" not in arguments,
        "C launcher probe link was parsed as a compile command",
    )
    setting_contract = require_wasm_compile_contract(
        arguments,
        language="c",
        context="C launcher probe link",
    )
    output = (
        build
        / "CMakeFiles"
        / "c-launcher-probe"
        / f"{target}.wasm"
    )
    require(output.is_file(), f"C launcher probe output is missing: {output}")
    build_ninja = (build / "build.ninja").read_text("utf-8")
    edge = re.findall(
        rf"^build CMakeFiles/c-launcher-probe/{target}\.js: ([^\s]+)",
        build_ninja,
        flags=re.MULTILINE,
    )
    require(len(edge) == 1, f"expected one C launcher Ninja edge: {edge}")
    verify_ninja_noop(ninja, build, target)
    return {
        "target": target,
        "compiler": relative_path(repo, expected_emcc),
        "output": relative_path(repo, output),
        "selectedNinjaRule": edge[0],
        "adapterAuthenticated": True,
        "effectiveSettings": setting_contract["effectiveValues"],
        "commandSha256": sha256_text(canonical_command(repo, links[0])),
        "noOp": True,
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


def git_object_id(kind: str, content: bytes) -> bytes:
    header = f"{kind} {len(content)}\0".encode("ascii")
    return hashlib.sha1(header + content).digest()


def git_tree_id(files: Mapping[str, bytes], prefix: str = "") -> bytes:
    direct_files: dict[str, bytes] = {}
    directories: set[str] = set()
    for relative, content in files.items():
        if not relative.startswith(prefix):
            continue
        remainder = relative[len(prefix) :]
        if "/" in remainder:
            directories.add(remainder.split("/", 1)[0])
        else:
            direct_files[remainder] = content
    entries: list[tuple[bytes, bytes]] = []
    for name, content in direct_files.items():
        encoded = name.encode("utf-8")
        entries.append(
            (
                encoded,
                b"100644 " + encoded + b"\0" + git_object_id("blob", content),
            )
        )
    for name in directories:
        encoded = name.encode("utf-8")
        entries.append(
            (
                encoded + b"/",
                b"40000 "
                + encoded
                + b"\0"
                + git_tree_id(files, f"{prefix}{name}/"),
            )
        )
    content = b"".join(record for _, record in sorted(entries))
    return git_object_id("tree", content)


def baseline_blobs(vcpkg: Path, port: str) -> tuple[dict[str, str], str]:
    archive = (
        vcpkg.parent
        / "downloads"
        / EXPECTED_VCPKG_SOURCE_ARCHIVE["archiveFile"]
    )
    require_no_reparse_chain(archive, "vcpkg retained source ZIP")
    require(
        archive.is_file()
        and sha256(archive) == EXPECTED_VCPKG_SOURCE_ARCHIVE["sha256"],
        "vcpkg retained source ZIP drifted before overlay audit",
    )
    archive_prefix = EXPECTED_VCPKG_SOURCE_ARCHIVE["payload"][
        "stripPrefix"
    ]
    member_prefix = f"{archive_prefix}/ports/{port}/"
    files: dict[str, bytes] = {}
    with archive.open("rb") as stream:
        digest = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
        require(
            digest.hexdigest() == EXPECTED_VCPKG_SOURCE_ARCHIVE["sha256"],
            "vcpkg retained source ZIP changed before overlay audit",
        )
        stream.seek(0)
        with zipfile.ZipFile(stream) as bundle:
            for info in bundle.infolist():
                if (
                    not info.is_dir()
                    and info.filename.startswith(member_prefix)
                ):
                    relative = info.filename[len(member_prefix) :]
                    require(
                        relative
                        and relative
                        == _safe_contract_relative_path(
                            relative,
                            f"{port} archive member",
                        ),
                        f"unsafe {port} archive member: {info.filename}",
                    )
                    require(
                        relative not in files,
                        f"duplicate {port} archive member: {relative}",
                    )
                    files[relative] = bundle.read(info)
    require(files, f"no {port} members in retained vcpkg source ZIP")
    blobs = {
        relative: git_object_id("blob", content).hex()
        for relative, content in files.items()
    }
    return blobs, git_tree_id(files).hex()


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
        actual_blob = git_object_id(
            "blob",
            (overlay / relative).read_bytes(),
        ).hex()
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


def qt_resource_last_modified_timestamp(generated: Path) -> int:
    generated_text = generated.read_text("utf-8", errors="replace")
    match = re.search(
        (
            r"static\s+const\s+unsigned\s+char\s+"
            r"qt_resource_struct\[\]\s*=\s*\{"
            r"(?P<body>.*?)"
            r"\n\s*\};"
        ),
        generated_text,
        re.DOTALL,
    )
    require(
        match is not None,
        "generated shader resource structure is missing",
    )
    body = re.sub(r"//[^\r\n]*", "", match.group("body"))
    byte_values = re.findall(r"0x([0-9a-fA-F]{1,2})", body)
    residue = re.sub(
        r"0x[0-9a-fA-F]{1,2}|[\s,]",
        "",
        body,
    )
    require(
        not residue,
        "generated shader resource structure contains unexpected syntax",
    )
    resource_struct = bytes(int(value, 16) for value in byte_values)
    expected_bytes = len(SHADER_RESOURCE_TREE) * 22
    require(
        len(resource_struct) == expected_bytes,
        "generated shader resource structure size drifted: "
        f"expected {expected_bytes}, got {len(resource_struct)}",
    )
    return int.from_bytes(resource_struct[-8:], "big")


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
    resource_timestamp = qt_resource_last_modified_timestamp(generated)
    expected_timestamp = EXPECTED_SOURCE_DATE_EPOCH * 1000
    require(
        resource_timestamp == expected_timestamp,
        "generated shader resource timestamp drifted: "
        f"expected {expected_timestamp}, got {resource_timestamp}",
    )
    return {
        "prefix": SHADER_RESOURCE_PREFIX,
        "aliases": [SHADER_RESOURCE_ALIAS],
        "resourcePaths": [SHADER_RESOURCE_PATH],
        "source": source,
        "sourceDateEpoch": EXPECTED_SOURCE_DATE_EPOCH,
        "resourceTimestampMilliseconds": resource_timestamp,
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
        "shaderResourceSourceDateEpoch": shader["sourceDateEpoch"],
        "shaderResourceTimestampMilliseconds": (
            shader["resourceTimestampMilliseconds"]
        ),
        "files": {
            path.name: {
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for path in hashed
        },
    }


def verify_active_qualification_closure(
    repo: Path,
    emsdk: Path,
    vcpkg: Path,
    toolchains: Mapping[str, Any],
) -> dict[str, Any]:
    lock = json.loads(
        (
            repo / "tools" / "wasm-probe" / "toolchain-lock.json"
        ).read_text("utf-8")
    )
    outer_cmake = repo / toolchains["outerProbeCMake"]["executable"]
    ninja = repo / toolchains["ninja"]["executable"]
    port_cmake_root = (
        repo
        / ".wasm-vcpkg"
        / "downloads"
        / "tools"
        / lock["vcpkg"]["portBuildCMake"]["installationDirectory"]
    )
    installed = repo / ".wasm-vcpkg" / "installed"
    roots: list[tuple[str, Path, Sequence[str]]] = [
        ("emsdk", emsdk, ()),
        (
            "emscripten-cache",
            repo
            / ".toolchains"
            / lock["emscripten"]["cache"]["directory"],
            (),
        ),
        ("outer-cmake", outer_cmake.parent.parent, ()),
        ("ninja", ninja.parent, ()),
        ("vcpkg", vcpkg, ()),
        ("vcpkg-port-cmake", port_cmake_root, ()),
        ("vcpkg-target", installed / TARGET_TRIPLET, ()),
        (
            "vcpkg-host-runtime",
            installed / HOST_TRIPLET,
            (".pdb", ".lib"),
        ),
    ]
    retained = repo / ".toolchains" / "downloads"
    explicit: list[tuple[str, Path]] = [
        (
            "retained/emsdk-source.zip",
            retained / lock["emscripten"]["sourceArchive"]["archiveFile"],
        ),
        (
            "retained/vcpkg-source.zip",
            retained / lock["vcpkg"]["sourceArchive"]["archiveFile"],
        ),
        (
            "retained/outer-cmake.zip",
            retained / lock["buildTools"]["cmake"]["archiveFile"],
        ),
        (
            "retained/ninja.zip",
            retained / lock["buildTools"]["ninja"]["archiveFile"],
        ),
        (
            "retained/vcpkg-port-cmake.zip",
            repo
            / ".wasm-vcpkg"
            / "downloads"
            / lock["vcpkg"]["portBuildCMake"]["archiveFile"],
        ),
    ]
    manifest = repo / "tools" / "wasm-probe" / "input-manifest.txt"
    input_identity = probe_input_identity(repo)
    explicit.append(
        (
            "repo/tools/wasm-probe/input-manifest.txt",
            manifest,
        )
    )
    explicit.extend(
        (f"repo/{relative}", repo / relative)
        for relative in input_identity["paths"]
    )
    build_controls = probe_build_control_identity(repo)
    explicit.extend(
        (f"build-control/{relative}", path)
        for relative, path in zip(
            build_controls["_paths"],
            build_controls["_files"],
        )
    )
    expected_verifier = (
        repo / "tools" / "wasm-probe" / "tests" / "verify_build.py"
    )
    require_same_path(__file__, expected_verifier, "qualification verifier")
    require(
        "tools/wasm-probe/tests/verify_build.py" in input_identity["paths"],
        "qualification verifier is absent from the explicit input manifest",
    )
    identity = qualification_closure_identity(roots, explicit)
    identity["buildControls"] = {
        key: value
        for key, value in build_controls.items()
        if not key.startswith("_")
    }
    return identity


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
    qualification = verify_active_qualification_closure(
        repo,
        emsdk,
        vcpkg,
        toolchains,
    )
    compiler_qualification = compiler_qualification_identity(qualification)
    ninja = repo / toolchains["ninja"]["executable"]
    outer_probe_cmake = (
        repo / toolchains["outerProbeCMake"]["executable"]
    ).resolve()
    vcpkg_port_cmake = (
        repo / toolchains["vcpkgPortBuildCMake"]["executable"]
    ).resolve()
    qualification_build = clean_rebuild_selected_targets(
        repo,
        build,
        ninja,
    )
    build_freshness = verify_build_freshness(repo, build, ninja)
    qt = verify_qt_installation(repo, installed, build)
    cmake_identity = verify_cmake_identity(
        repo,
        build,
        buildtrees,
        outer_probe_cmake,
        vcpkg_port_cmake,
    )
    compile_commands = verify_compile_commands(
        repo,
        build,
        buildtrees,
        ninja,
    )
    selected_compile_records = dict(
        compile_commands.pop("_selectedCompileRecords")
    )
    application_link = verify_application_link(
        repo,
        build,
        ninja,
        compiler_qualification,
        selected_compile_records,
    )
    features = verify_features_and_autogen(
        repo,
        installed,
        buildtrees,
        vcpkg_port_cmake,
    )
    overlays = verify_overlays(repo, vcpkg)
    artifacts = verify_artifacts(repo, build)

    return {
        "schemaVersion": 4,
        "gate": "1A",
        "scope": GATE_SCOPE,
        "technicalProbePassed": True,
        "gate1aPassed": True,
        "gate0Satisfied": False,
        "formalGate1EntryAuthorized": False,
        "gate1Passed": False,
        "toolchains": toolchains,
        "qualificationClosure": qualification,
        "qualificationBuild": qualification_build,
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


def validate_source_archive_evidence(
    value: Any,
    expected: Mapping[str, Any],
    installation_root: str,
    label: str,
) -> None:
    source = require_mapping(value, label)
    require_exact_keys(
        source,
        {
            "archive",
            "payload",
            "installationRoot",
            "allowedRuntimePrefixes",
            "allowedRuntimeFiles",
        },
        label,
    )
    archive = require_mapping(source["archive"], f"{label}.archive")
    require_exact_keys(archive, {"path", "sha256"}, f"{label}.archive")
    require_sha256_value(archive["sha256"], f"{label}.archive.sha256")
    require(
        archive
        == {
            "path": f".toolchains/downloads/{expected['archiveFile']}",
            "sha256": expected["sha256"],
        }
        and source["payload"] == expected["payload"]
        and source["installationRoot"] == installation_root
        and source["allowedRuntimePrefixes"]
        == expected["allowedRuntimePrefixes"]
        and source["allowedRuntimeFiles"] == expected["allowedRuntimeFiles"],
        f"{label} does not match the authenticated retained source ZIP",
    )


def validate_toolchain_evidence(value: Any) -> None:
    toolchains = require_mapping(value, "toolchains")
    require_exact_keys(
        toolchains,
        {
            "lockFileSha256",
            "reproducibleBuild",
            "emscripten",
            "vcpkg",
            "outerProbeCMake",
            "vcpkgPortBuildCMake",
            "ninja",
            "hermeticEnvironment",
            "bootstrapLaunchers",
        },
        "toolchains",
    )
    require_sha256_value(
        toolchains["lockFileSha256"],
        "toolchains.lockFileSha256",
    )
    require(
        toolchains["reproducibleBuild"]
        == EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY,
        "toolchains reproducible-build evidence drifted",
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
            "verifierPython",
            "node",
            "nodeSha256",
            "launcher",
            "cLauncherSha256",
            "cxxLauncherSha256",
            "driver",
            "driverAdapter",
            "driverApi",
            "responseAuditor",
            "sourceArchive",
            "bootstrapPython",
            "installation",
            "cache",
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
        and emscripten["nodeSha256"]
        == EXPECTED_EMSCRIPTEN_LOCK_ENTRY["nodeExecutableSha256"]
        and emscripten["launcher"]
        == (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/"
            "em++.bat"
        )
        and emscripten["cLauncherSha256"]
        == EXPECTED_EMSCRIPTEN_LOCK_ENTRY["cLauncherSha256"]
        and emscripten["cxxLauncherSha256"]
        == EXPECTED_EMSCRIPTEN_LOCK_ENTRY["cxxLauncherSha256"]
        and emscripten["driver"]
        == ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.py",
        "Emscripten evidence does not match the exact pin",
    )
    verifier_python = require_mapping(
        emscripten["verifierPython"],
        "toolchains.emscripten.verifierPython",
    )
    require_exact_keys(
        verifier_python,
        {"path", "sha256", "isolated", "bytecodeDisabled"},
        "toolchains.emscripten.verifierPython",
    )
    require(
        verifier_python
        == {
            "path": EXPECTED_EMSDK_PYTHON,
            "sha256": EXPECTED_EMSDK_BOOTSTRAP_PYTHON[
                "executableSha256"
            ],
            "isolated": True,
            "bytecodeDisabled": True,
        },
        "Evidence verifier Python contract drifted",
    )
    driver_adapter = require_mapping(
        emscripten["driverAdapter"],
        "toolchains.emscripten.driverAdapter",
    )
    require_exact_keys(
        driver_adapter,
        {"path", "sha256"},
        "toolchains.emscripten.driverAdapter",
    )
    require(
        driver_adapter
        == {
            "path": (
                "tools/wasm-probe/scripts/"
                "invoke_emscripten_driver.py"
            ),
            "sha256": EXPECTED_GATE_TOOLS_LOCK_ENTRY[
                "adapterSha256"
            ],
        },
        "Emscripten in-process driver adapter evidence drifted",
    )
    driver_api = require_mapping(
        emscripten["driverApi"],
        "toolchains.emscripten.driverApi",
    )
    expected_driver_paths = {
        "emccPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/emcc.py"
        ),
        "emxxPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/em++.py"
        ),
        "emarLauncherSha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/emar.bat"
        ),
        "emarPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/emar.py"
        ),
        "emranlibLauncherSha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/emranlib.bat"
        ),
        "emranlibPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/emranlib.py"
        ),
        "sharedPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/tools/shared.py"
        ),
        "responseFilePySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/"
            "tools/response_file.py"
        ),
        "configPySha256": (
            ".toolchains/emsdk-4.0.7/upstream/emscripten/tools/config.py"
        ),
    }
    require_exact_keys(
        driver_api,
        EXPECTED_EMSCRIPTEN_DRIVER_API,
        "toolchains.emscripten.driverApi",
    )
    for name, expected_path in expected_driver_paths.items():
        driver_file = require_mapping(
            driver_api[name],
            f"toolchains.emscripten.driverApi.{name}",
        )
        require_exact_keys(
            driver_file,
            {"path", "sha256"},
            f"toolchains.emscripten.driverApi.{name}",
        )
        require_sha256_value(
            driver_file["sha256"],
            f"toolchains.emscripten.driverApi.{name}.sha256",
        )
        require(
            driver_file
            == {
                "path": expected_path,
                "sha256": EXPECTED_EMSCRIPTEN_DRIVER_API[name],
            },
            f"Emscripten driver API evidence drifted: {name}",
        )
    python_import_closure = require_mapping(
        driver_api["pythonImportClosure"],
        "toolchains.emscripten.driverApi.pythonImportClosure",
    )
    require(
        python_import_closure
        == EXPECTED_EMSCRIPTEN_DRIVER_API["pythonImportClosure"],
        "Emscripten Python import closure evidence drifted",
    )
    require_sha256_value(
        emscripten["nodeSha256"],
        "toolchains.emscripten.nodeSha256",
    )
    response_auditor = require_mapping(
        emscripten["responseAuditor"],
        "toolchains.emscripten.responseAuditor",
    )
    require_exact_keys(
        response_auditor,
        {"path", "sha256"},
        "toolchains.emscripten.responseAuditor",
    )
    require(
        response_auditor
        == {
            "path": (
                "tools/wasm-probe/scripts/"
                "audit_emscripten_response_files.py"
            ),
            "sha256": EXPECTED_GATE_TOOLS_LOCK_ENTRY[
                "responseAuditorSha256"
            ],
        },
        "Emscripten response auditor evidence drifted",
    )
    validate_source_archive_evidence(
        emscripten["sourceArchive"],
        EXPECTED_EMSDK_SOURCE_ARCHIVE,
        ".toolchains/emsdk-4.0.7",
        "toolchains.emscripten.sourceArchive",
    )
    bootstrap_python = require_mapping(
        emscripten["bootstrapPython"],
        "toolchains.emscripten.bootstrapPython",
    )
    require_exact_keys(
        bootstrap_python,
        {"sourceArchive", "executable"},
        "toolchains.emscripten.bootstrapPython",
    )
    bootstrap_source_expected = {
        name: EXPECTED_EMSDK_BOOTSTRAP_PYTHON[name]
        for name in (
            "archiveFile",
            "sha256",
            "payload",
            "allowedRuntimePrefixes",
            "allowedRuntimeFiles",
        )
    }
    validate_source_archive_evidence(
        bootstrap_python["sourceArchive"],
        bootstrap_source_expected,
        (
            ".toolchains/emsdk-4.0.7/"
            "python/3.9.2-nuget_64bit"
        ),
        "toolchains.emscripten.bootstrapPython.sourceArchive",
    )
    bootstrap_executable = require_mapping(
        bootstrap_python["executable"],
        "toolchains.emscripten.bootstrapPython.executable",
    )
    require_exact_keys(
        bootstrap_executable,
        {"path", "sha256"},
        "toolchains.emscripten.bootstrapPython.executable",
    )
    require(
        bootstrap_executable
        == {
            "path": EXPECTED_EMSDK_PYTHON,
            "sha256": EXPECTED_EMSDK_BOOTSTRAP_PYTHON[
                "executableSha256"
            ],
        },
        "Emscripten bootstrap Python executable evidence drifted",
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
    cache = require_mapping(
        emscripten["cache"],
        "toolchains.emscripten.cache",
    )
    require_exact_keys(
        cache,
        {
            "directory",
            "initializer",
            "prewarmCores",
            "compilerPathPrefixMap",
            "frozenEnvironment",
            "volatileProducts",
            "root",
            "payload",
        },
        "toolchains.emscripten.cache",
    )
    expected_cache_contract = require_mapping(
        EXPECTED_EMSCRIPTEN_LOCK_ENTRY["cache"],
        "expected Emscripten cache contract",
    )
    require(
        {
            key: cache[key]
            for key in (
                "directory",
                "initializer",
                "prewarmCores",
                "compilerPathPrefixMap",
                "frozenEnvironment",
                "volatileProducts",
                "root",
                "payload",
            )
        }
        == {
            "directory": expected_cache_contract["directory"],
            "initializer": expected_cache_contract["initializer"],
            "prewarmCores": expected_cache_contract["prewarmCores"],
            "compilerPathPrefixMap": expected_cache_contract[
                "compilerPathPrefixMap"
            ],
            "frozenEnvironment": expected_cache_contract[
                "frozenEnvironment"
            ],
            "volatileProducts": expected_cache_contract[
                "volatileProducts"
            ],
            "root": ".toolchains/emscripten-cache-4.0.7",
            "payload": expected_cache_contract["payload"],
        },
        "Emscripten frozen-cache evidence drifted",
    )

    vcpkg = require_mapping(toolchains["vcpkg"], "toolchains.vcpkg")
    require_exact_keys(
        vcpkg,
        {
            "baselineCommit",
            "versionOutput",
            "executable",
            "executableSha256",
            "bootstrapLauncher",
            "bootstrapInputs",
            "sourceArchive",
        },
        "toolchains.vcpkg",
    )
    require(
        vcpkg["baselineCommit"] == EXPECTED_VCPKG_COMMIT
        and vcpkg["versionOutput"] == EXPECTED_VCPKG_VERSION_LINE
        and vcpkg["executable"]
        == f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}/vcpkg.exe"
        and vcpkg["executableSha256"] == EXPECTED_VCPKG_EXECUTABLE_SHA256,
        "vcpkg evidence does not match the exact pin",
    )
    require_sha256_value(
        vcpkg["executableSha256"],
        "toolchains.vcpkg.executableSha256",
    )
    vcpkg_bootstrap = require_mapping(
        vcpkg["bootstrapLauncher"],
        "toolchains.vcpkg.bootstrapLauncher",
    )
    require_exact_keys(
        vcpkg_bootstrap,
        {"path", "sha256"},
        "toolchains.vcpkg.bootstrapLauncher",
    )
    require(
        vcpkg_bootstrap["path"] == "bootstrap-vcpkg.bat",
        "vcpkg bootstrap launcher path drifted",
    )
    require_sha256_value(
        vcpkg_bootstrap["sha256"],
        "toolchains.vcpkg.bootstrapLauncher.sha256",
    )
    bootstrap_inputs = require_mapping(
        vcpkg["bootstrapInputs"],
        "toolchains.vcpkg.bootstrapInputs",
    )
    require_exact_keys(
        bootstrap_inputs,
        {"script", "toolMetadata", "toolReleaseTag", "toolUrl"},
        "toolchains.vcpkg.bootstrapInputs",
    )
    expected_bootstrap_files = {
        "script": (
            "scripts/bootstrap.ps1",
            EXPECTED_VCPKG_BOOTSTRAP_SCRIPT_SHA256,
        ),
        "toolMetadata": (
            "scripts/vcpkg-tool-metadata.txt",
            EXPECTED_VCPKG_TOOL_METADATA_SHA256,
        ),
    }
    for name, (expected_path, expected_sha256) in (
        expected_bootstrap_files.items()
    ):
        record = require_mapping(
            bootstrap_inputs[name],
            f"toolchains.vcpkg.bootstrapInputs.{name}",
        )
        require_exact_keys(
            record,
            {"path", "sha256"},
            f"toolchains.vcpkg.bootstrapInputs.{name}",
        )
        require_sha256_value(
            record["sha256"],
            f"toolchains.vcpkg.bootstrapInputs.{name}.sha256",
        )
        require(
            record
            == {"path": expected_path, "sha256": expected_sha256},
            f"vcpkg bootstrap input evidence drifted: {name}",
        )
    require(
        bootstrap_inputs["toolReleaseTag"] == EXPECTED_VCPKG_TOOL_RELEASE_TAG
        and bootstrap_inputs["toolUrl"] == EXPECTED_VCPKG_TOOL_URL,
        "vcpkg bootstrap release identity drifted",
    )
    validate_source_archive_evidence(
        vcpkg["sourceArchive"],
        EXPECTED_VCPKG_SOURCE_ARCHIVE,
        f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}",
        "toolchains.vcpkg.sourceArchive",
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
            "installation",
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
        {"path", "sha512", "bytes"},
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
            "bytes": EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY["archiveBytes"],
        },
        "vcpkg port-build CMake archive evidence drifted",
    )
    installation = require_mapping(
        port_cmake["installation"],
        "toolchains.vcpkgPortBuildCMake.installation",
    )
    require(
        installation
        == {
            "archive": {
                "path": (
                    ".wasm-vcpkg/downloads/"
                    "cmake-4.3.3-windows-x86_64.zip"
                ),
                "sha512": (
                    EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY["sha512"]
                ),
            },
            "payload": EXPECTED_VCPKG_PORT_CMAKE_PAYLOAD,
            "installationRoot": (
                ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows"
            ),
        },
        "vcpkg port-build CMake installation evidence drifted",
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
        {
            "forbiddenNamesAbsent",
            "scrubbedVariableFamilies",
            "canonicalVariables",
            "nativeCommandInterpreterAuthenticated",
        },
        "toolchains.hermeticEnvironment",
    )
    require(
        hermetic["forbiddenNamesAbsent"]
        == list(FORBIDDEN_BUILD_ENVIRONMENT_NAMES),
        "hermetic environment forbidden-name evidence drifted",
    )
    require(
        hermetic["scrubbedVariableFamilies"]
        == [
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
            "QML_",
            "QT_",
            "RHYTHMGAME_",
            "VCPKG_",
            "X_VCPKG_",
        ],
        "hermetic environment scrubbed-family evidence drifted",
    )
    canonical = require_mapping(
        hermetic["canonicalVariables"],
        "toolchains.hermeticEnvironment.canonicalVariables",
    )
    require_exact_keys(
        canonical,
        {
            "EMSDK",
            "EMSDK_NODE",
            "EMSDK_PYTHON",
            "EMSCRIPTEN_ROOT",
            "EM_CONFIG",
            "EM_CACHE",
            "VCPKG_ROOT",
            "VCPKG_DEFAULT_BINARY_CACHE",
            "RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER",
            "RHYTHMGAME_WASM_TOOLCHAIN_LOCK",
            "RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR",
            "RHYTHMGAME_EMSCRIPTEN_ROOT",
            "RHYTHMGAME_EM_CONFIG",
            "RHYTHMGAME_EM_CACHE",
            "RHYTHMGAME_EM_CONFIG_SHA256",
            "EMSCRIPTEN_VERSION",
            "VCPKG_DISABLE_METRICS",
            "VCPKG_MAX_CONCURRENCY",
            "CMAKE_NINJA_FORCE_RESPONSE_FILE",
            "PYTHONDONTWRITEBYTECODE",
            "PYTHONNOUSERSITE",
            "EM_FROZEN_CACHE",
            "SOURCE_DATE_EPOCH",
        },
        "toolchains.hermeticEnvironment.canonicalVariables",
    )
    require_sha256_value(
        canonical["RHYTHMGAME_EM_CONFIG_SHA256"],
        (
            "toolchains.hermeticEnvironment.canonicalVariables."
            "RHYTHMGAME_EM_CONFIG_SHA256"
        ),
    )
    canonical_without_config_digest = dict(canonical)
    del canonical_without_config_digest["RHYTHMGAME_EM_CONFIG_SHA256"]
    require(
        canonical_without_config_digest
        == {
            "EMSDK": ".toolchains/emsdk-4.0.7",
            "EMSDK_NODE": EXPECTED_EMSDK_NODE,
            "EMSDK_PYTHON": EXPECTED_EMSDK_PYTHON,
            "EMSCRIPTEN_ROOT": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten"
            ),
            "EM_CONFIG": ".toolchains/emsdk-4.0.7/.emscripten",
            "EM_CACHE": (
                ".toolchains/emscripten-cache-4.0.7"
            ),
            "VCPKG_ROOT": (
                f".toolchains/vcpkg-{EXPECTED_VCPKG_COMMIT[:8]}"
            ),
            "VCPKG_DEFAULT_BINARY_CACHE": ".wasm-vcpkg/bincache",
            "RHYTHMGAME_EMSCRIPTEN_DRIVER_ADAPTER": (
                "tools/wasm-probe/scripts/invoke_emscripten_driver.py"
            ),
            "RHYTHMGAME_WASM_TOOLCHAIN_LOCK": (
                "tools/wasm-probe/toolchain-lock.json"
            ),
            "RHYTHMGAME_EMSCRIPTEN_RESPONSE_AUDITOR": (
                "tools/wasm-probe/scripts/"
                "audit_emscripten_response_files.py"
            ),
            "RHYTHMGAME_EMSCRIPTEN_ROOT": (
                ".toolchains/emsdk-4.0.7/upstream/emscripten"
            ),
            "RHYTHMGAME_EM_CONFIG": (
                ".toolchains/emsdk-4.0.7/.emscripten"
            ),
            "RHYTHMGAME_EM_CACHE": (
                ".toolchains/emscripten-cache-4.0.7"
            ),
            "EMSCRIPTEN_VERSION": EXPECTED_EMSCRIPTEN,
            "VCPKG_DISABLE_METRICS": "1",
            "VCPKG_MAX_CONCURRENCY": str(
                EXPECTED_REPRODUCIBLE_BUILD_LOCK_ENTRY[
                    "vcpkgMaxConcurrency"
                ]
            ),
            "CMAKE_NINJA_FORCE_RESPONSE_FILE": "1",
            "PYTHONDONTWRITEBYTECODE": "1",
            "PYTHONNOUSERSITE": "1",
            "EM_FROZEN_CACHE": "1",
            "SOURCE_DATE_EPOCH": str(EXPECTED_SOURCE_DATE_EPOCH),
        },
        "hermetic environment canonical evidence drifted",
    )
    require(
        hermetic["nativeCommandInterpreterAuthenticated"] is True,
        "native Windows command interpreter was not authenticated",
    )
    launchers = require_mapping(
        toolchains["bootstrapLaunchers"],
        "toolchains.bootstrapLaunchers",
    )
    require_exact_keys(
        launchers,
        {"emsdk", "vcpkg"},
        "toolchains.bootstrapLaunchers",
    )
    for name, (expected_path, expected_sha256) in {
        "emsdk": (
            "emsdk.py",
            EXPECTED_EMSCRIPTEN_LOCK_ENTRY["bootstrapScriptSha256"],
        ),
        "vcpkg": (
            "bootstrap-vcpkg.bat",
            EXPECTED_VCPKG_BOOTSTRAP_LAUNCHER_SHA256,
        ),
    }.items():
        launcher = require_mapping(
            launchers[name],
            f"toolchains.bootstrapLaunchers.{name}",
        )
        require_exact_keys(
            launcher,
            {"path", "sha256"},
            f"toolchains.bootstrapLaunchers.{name}",
        )
        require(
            launcher["path"] == expected_path,
            f"{name} bootstrap launcher path drifted",
        )
        require_sha256_value(
            launcher["sha256"],
            f"toolchains.bootstrapLaunchers.{name}.sha256",
        )
        require(
            launcher["sha256"] == expected_sha256,
            f"{name} bootstrap launcher bytes drifted",
        )
    require(
        launchers["vcpkg"] == vcpkg_bootstrap,
        "vcpkg bootstrap launcher evidence disagrees across sections",
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
        "-v",
        "RhythmGameWasmProbe",
    ]
    require(
        freshness["command"] == expected_command,
        "buildFreshness command is not the exact pinned Ninja build",
    )
    output = freshness["output"]
    cmake = (
        "${REPO}/.toolchains/"
        f"{EXPECTED_OUTER_CMAKE_LOCK_ENTRY['directory']}/bin/cmake.exe"
    )
    build = "${REPO}/tools/wasm-probe/build/wasm-release"
    configure_depends_output = [
        (
            f"[0/2] {cmake} -P "
            f"{build}/CMakeFiles/VerifyGlobs.cmake"
        ),
        "ninja: no work to do.",
    ]
    exact_noop = (
        isinstance(output, list)
        and len(output) == 2
        and all(type(line) is str for line in output)
        and output[0].startswith("ninja: Entering directory ")
        and output[1] == "ninja: no work to do."
    )
    exact_configure_depends_noop = (
        isinstance(output, list)
        and len(output) == 3
        and all(type(line) is str for line in output)
        and output[0].startswith("ninja: Entering directory ")
        and output[1:] == configure_depends_output
    )
    require(
        exact_noop or exact_configure_depends_noop,
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
        and source_inputs["tracking"] == "explicit-input-manifest"
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
            "authenticatedLaunchers",
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
            "authenticatedLaunchers": {
                "CMAKE_C_COMPILER_LAUNCHER": "emcc",
                "CMAKE_CXX_COMPILER_LAUNCHER": "em++",
                "CMAKE_C_LINKER_LAUNCHER": "emcc",
                "CMAKE_CXX_LINKER_LAUNCHER": "em++",
            },
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
            "hostTripletEnvironmentPassthrough",
        },
        "qt",
    )
    require(qt["version"] == EXPECTED_QT, "Qt version drift")
    require(
        qt["targetTripletEnvironmentPassthrough"]
        == list(EXPECTED_TARGET_ENV_PASSTHROUGH),
        "Qt target triplet environment passthrough drifted",
    )
    require(
        qt["hostTripletEnvironmentPassthrough"]
        == list(EXPECTED_HOST_ENV_PASSTHROUGH),
        "Qt host triplet environment passthrough drifted",
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
            "probeAdapterCommandCounts",
            "probeCompileDbNinjaParity",
            "selectedTargetGraph",
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
    adapter_counts = require_mapping(
        compile_commands["probeAdapterCommandCounts"],
        "compileCommands.probeAdapterCommandCounts",
    )
    require_exact_keys(
        adapter_counts,
        {"c", "cxx"},
        "compileCommands.probeAdapterCommandCounts",
    )
    for language in ("c", "cxx"):
        require_exact_type(
            adapter_counts[language],
            int,
            f"compileCommands.probeAdapterCommandCounts.{language}",
        )
        require(
            adapter_counts[language] > 0,
            f"no probe {language} compile command used the "
            "authenticated adapter",
        )
    parity = require_mapping(
        compile_commands["probeCompileDbNinjaParity"],
        "compileCommands.probeCompileDbNinjaParity",
    )
    require(
        parity
        == {
            "expandedCommandSource": "ninja -t compdb -x",
            "matchedCommandCount": sum(adapter_counts.values()),
            "correlationKey": "directory-plus-output",
            "dependencyBookkeepingRemoved": ["-MD", "-MT", "-MF"],
            "exactAfterPathNormalization": True,
        },
        "probe compile DB/expanded Ninja argv parity evidence drifted",
    )
    selected_graph = require_mapping(
        compile_commands["selectedTargetGraph"],
        "compileCommands.selectedTargetGraph",
    )
    require_exact_keys(
        selected_graph,
        {
            "target",
            "graphSource",
            "objectOutputCount",
            "archiveOutputCount",
            "objectOutputsSha256",
            "allSelectedOutputsMatchedByExactOutput",
        },
        "compileCommands.selectedTargetGraph",
    )
    require(
        selected_graph["target"] == "RhythmGameWasmProbe"
        and selected_graph["graphSource"] == "build.ninja"
        and selected_graph["allSelectedOutputsMatchedByExactOutput"] is True,
        "selected target graph evidence drifted",
    )
    require_positive_int(
        selected_graph["objectOutputCount"],
        "selectedTargetGraph.objectOutputCount",
    )
    require_positive_int(
        selected_graph["archiveOutputCount"],
        "selectedTargetGraph.archiveOutputCount",
    )
    require_sha256_value(
        selected_graph["objectOutputsSha256"],
        "selectedTargetGraph.objectOutputsSha256",
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
            "dependencyArchives",
            "compileDependencyClosure",
            "selectedLinkArtifactBinding",
            "effectiveSettings",
            "settingOccurrences",
            "literalAsyncifyConfigured",
            "staticExceptionArchive",
            "staticExceptionArchiveLinked",
            "adapterAuthenticated",
            "cLauncherProbe",
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
        and link["staticExceptionArchiveLinked"] is True
        and link["adapterAuthenticated"] is True,
        "application link contract drifted",
    )
    c_probe = require_mapping(
        link["cLauncherProbe"],
        "applicationLink.cLauncherProbe",
    )
    require_exact_keys(
        c_probe,
        {
            "target",
            "compiler",
            "output",
            "selectedNinjaRule",
            "adapterAuthenticated",
            "effectiveSettings",
            "commandSha256",
            "noOp",
        },
        "applicationLink.cLauncherProbe",
    )
    require(
        c_probe["target"] == "RhythmGameWasmCLauncherProbe"
        and c_probe["compiler"]
        == ".toolchains/emsdk-4.0.7/upstream/emscripten/emcc.bat"
        and c_probe["output"]
        == (
            "tools/wasm-probe/build/wasm-release/CMakeFiles/"
            "c-launcher-probe/RhythmGameWasmCLauncherProbe.wasm"
        )
        and c_probe["selectedNinjaRule"]
        == "C_EXECUTABLE_LINKER__RhythmGameWasmCLauncherProbe_Release"
        and c_probe["adapterAuthenticated"] is True
        and c_probe["effectiveSettings"]
        == C_COMPILE_EMSCRIPTEN_SETTINGS
        and c_probe["noOp"] is True,
        "C launcher probe link contract drifted",
    )
    require_sha256_value(
        c_probe["commandSha256"],
        "applicationLink.cLauncherProbe.commandSha256",
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
    selected_binding = require_mapping(
        link["selectedLinkArtifactBinding"],
        "applicationLink.selectedLinkArtifactBinding",
    )
    require_exact_keys(
        selected_binding,
        {
            "algorithm",
            "sha256",
            "argumentCount",
            "staticInputOccurrenceCount",
            "staticInputUniqueCount",
            "compileInputCount",
            "qualificationAggregateSha256",
            "customSection",
            "payloadEncoding",
            "artifactBound",
        },
        "applicationLink.selectedLinkArtifactBinding",
    )
    require(
        selected_binding["algorithm"]
        == SELECTED_LINK_IDENTITY_ALGORITHM
        and selected_binding["argumentCount"]
        == link["effectiveArgumentCount"] - 1
        and selected_binding["customSection"] == "build_id"
        and selected_binding["payloadEncoding"]
        == "uleb32-plus-32-byte-sha256"
        and selected_binding["artifactBound"] is True,
        "selected application link artifact binding drifted",
    )
    require_positive_int(
        selected_binding["staticInputOccurrenceCount"],
        "selectedLinkArtifactBinding.staticInputOccurrenceCount",
    )
    require_positive_int(
        selected_binding["staticInputUniqueCount"],
        "selectedLinkArtifactBinding.staticInputUniqueCount",
    )
    require(
        selected_binding["staticInputUniqueCount"]
        <= selected_binding["staticInputOccurrenceCount"],
        "selected link unique inputs exceed occurrences",
    )
    require_sha256_value(
        selected_binding["sha256"],
        "selectedLinkArtifactBinding.sha256",
    )
    require_positive_int(
        selected_binding["compileInputCount"],
        "selectedLinkArtifactBinding.compileInputCount",
    )
    require_sha256_value(
        selected_binding["qualificationAggregateSha256"],
        "selectedLinkArtifactBinding.qualificationAggregateSha256",
    )
    compile_dependencies = require_mapping(
        link["compileDependencyClosure"],
        "applicationLink.compileDependencyClosure",
    )
    require_exact_keys(
        compile_dependencies,
        {
            "schemaVersion",
            "algorithm",
            "selectedObjectCount",
            "dependencyOccurrenceCount",
            "uniqueDependencyCount",
            "aggregateSha256",
            "qualificationAggregateSha256",
            "allCurrentDependencyBytesVerified",
            "allCurrentObjectBytesVerified",
        },
        "applicationLink.compileDependencyClosure",
    )
    require(
        compile_dependencies["schemaVersion"] == 1
        and compile_dependencies["algorithm"]
        == COMPILE_DEPENDENCY_ALGORITHM
        and compile_dependencies["selectedObjectCount"]
        == selected_binding["compileInputCount"]
        and compile_dependencies["allCurrentDependencyBytesVerified"] is True
        and compile_dependencies["allCurrentObjectBytesVerified"] is True
        and compile_dependencies["qualificationAggregateSha256"]
        == selected_binding["qualificationAggregateSha256"],
        "compile dependency closure evidence drifted",
    )
    for name in (
        "selectedObjectCount",
        "dependencyOccurrenceCount",
        "uniqueDependencyCount",
    ):
        require_positive_int(
            compile_dependencies[name],
            f"compileDependencyClosure.{name}",
        )
    require(
        compile_dependencies["uniqueDependencyCount"]
        <= compile_dependencies["dependencyOccurrenceCount"],
        "compile dependency unique count exceeds occurrences",
    )
    require_sha256_value(
        compile_dependencies["aggregateSha256"],
        "compileDependencyClosure.aggregateSha256",
    )
    require_sha256_value(
        compile_dependencies["qualificationAggregateSha256"],
        "compileDependencyClosure.qualificationAggregateSha256",
    )
    validate_dependency_archive_evidence(link["dependencyArchives"])


def validate_dependency_archive_evidence(value: Any) -> None:
    dependency = require_mapping(
        value,
        "applicationLink.dependencyArchives",
    )
    require_exact_keys(
        dependency,
        {
            "contract",
            "generator",
            "manifest",
            "superset",
            "linkedClosure",
            "marker",
        },
        "applicationLink.dependencyArchives",
    )
    for name, expected_path in {
        "contract": "tools/wasm-probe/dependency-archive-contract.json",
        "generator": (
            "tools/wasm-probe/scripts/generate_dependency_digest.py"
        ),
        "manifest": (
            "tools/wasm-probe/build/wasm-release/generated/"
            "dependency-archive-digest.json"
        ),
    }.items():
        item = require_mapping(
            dependency[name],
            f"applicationLink.dependencyArchives.{name}",
        )
        require_exact_keys(
            item,
            {"path", "sha256"},
            f"applicationLink.dependencyArchives.{name}",
        )
        require(
            item["path"] == expected_path,
            f"dependency archive {name} path drifted",
        )
        require_sha256_value(
            item["sha256"],
            f"applicationLink.dependencyArchives.{name}.sha256",
        )

    superset = require_mapping(
        dependency["superset"],
        "applicationLink.dependencyArchives.superset",
    )
    require_exact_keys(
        superset,
        {
            "algorithm",
            "fileCount",
            "totalBytes",
            "inventorySha256",
            "aggregateSha256",
        },
        "applicationLink.dependencyArchives.superset",
    )
    require(
        superset["algorithm"] == "sha256-path-null-digest-lf-v1",
        "dependency archive superset algorithm drifted",
    )
    for name in ("fileCount", "totalBytes"):
        require_positive_int(
            superset[name],
            f"applicationLink.dependencyArchives.superset.{name}",
        )
    for name in ("inventorySha256", "aggregateSha256"):
        require_sha256_value(
            superset[name],
            f"applicationLink.dependencyArchives.superset.{name}",
        )

    closure = require_mapping(
        dependency["linkedClosure"],
        "applicationLink.dependencyArchives.linkedClosure",
    )
    require_exact_keys(
        closure,
        {
            "occurrenceCount",
            "uniqueFileCount",
            "uniqueBytes",
            "orderedAggregateSha256",
            "uniqueAggregateSha256",
            "archives",
            "systemLibraries",
            "buildLocalArchive",
            "buildLocalWasmObjectCount",
        },
        "applicationLink.dependencyArchives.linkedClosure",
    )
    for name in ("occurrenceCount", "uniqueFileCount", "uniqueBytes"):
        require_positive_int(
            closure[name],
            f"applicationLink.dependencyArchives.linkedClosure.{name}",
        )
    for name in ("orderedAggregateSha256", "uniqueAggregateSha256"):
        require_sha256_value(
            closure[name],
            f"applicationLink.dependencyArchives.linkedClosure.{name}",
        )
    archives = closure["archives"]
    require(
        isinstance(archives, list)
        and len(archives) == closure["occurrenceCount"],
        "linked dependency archive occurrence evidence drifted",
    )
    archive_by_path: dict[str, Mapping[str, Any]] = {}
    for index, raw_archive in enumerate(archives):
        archive = require_mapping(
            raw_archive,
            (
                "applicationLink.dependencyArchives.linkedClosure."
                f"archives[{index}]"
            ),
        )
        require_exact_keys(
            archive,
            {"path", "kind", "bytes", "sha256"},
            f"linked dependency archive {index}",
        )
        _safe_contract_relative_path(
            archive["path"],
            f"linked dependency archive {index} path",
        )
        require_positive_int(
            archive["bytes"],
            f"linked dependency archive {index} bytes",
        )
        require_sha256_value(
            archive["sha256"],
            f"linked dependency archive {index} sha256",
        )
        require(
            archive["kind"] in {"archive", "wasm-object"}
            and archive["path"].endswith(
                ".a" if archive["kind"] == "archive" else ".o"
            ),
            f"linked dependency archive {index} kind/suffix drifted",
        )
        existing = archive_by_path.get(archive["path"])
        require(
            existing is None or existing == archive,
            f"linked dependency archive occurrence disagrees: {archive['path']}",
        )
        archive_by_path[archive["path"]] = archive
    require(
        len(archive_by_path) == closure["uniqueFileCount"]
        and sum(entry["bytes"] for entry in archive_by_path.values())
        == closure["uniqueBytes"],
        "linked dependency archive unique closure evidence drifted",
    )
    require(
        closure["buildLocalArchive"] == "libWasmProbeExceptionBoundary.a",
        "linked build-local archive evidence drifted",
    )
    require_positive_int(
        closure["buildLocalWasmObjectCount"],
        "linked build-local Wasm object count",
    )
    require(
        isinstance(closure["systemLibraries"], list)
        and all(
            library in {"-lembind", "-lwebsocket.js", "-lopenal"}
            for library in closure["systemLibraries"]
        ),
        "linked Emscripten system-library evidence drifted",
    )

    marker = require_mapping(
        dependency["marker"],
        "applicationLink.dependencyArchives.marker",
    )
    require_exact_keys(
        marker,
        {"value", "source", "wasmDataSectionOccurrences"},
        "applicationLink.dependencyArchives.marker",
    )
    require(
        marker["value"]
        == (
            "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
            + superset["aggregateSha256"]
        )
        and marker["source"]
        == (
            "tools/wasm-probe/build/wasm-release/generated/"
            "ProbeDependencyDigest.cpp"
        )
        and marker["wasmDataSectionOccurrences"] == 1,
        "dependency archive marker evidence drifted",
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
            "shaderResourceSourceDateEpoch",
            "shaderResourceTimestampMilliseconds",
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
        and artifacts["shaderResourceAliases"] == [SHADER_RESOURCE_PATH]
        and artifacts["shaderResourceSourceDateEpoch"]
        == EXPECTED_SOURCE_DATE_EPOCH
        and artifacts["shaderResourceTimestampMilliseconds"]
        == EXPECTED_SOURCE_DATE_EPOCH * 1000,
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


def validate_qualification_closure_evidence(value: Any) -> None:
    qualification = require_mapping(value, "qualificationClosure")
    require_exact_keys(
        qualification,
        {
            "algorithm",
            "fileCount",
            "totalBytes",
            "inventorySha256",
            "aggregateSha256",
            "rootFileCounts",
            "rootByteCounts",
            "explicitFileCount",
            "explicitTotalBytes",
            "sameHandleLifetimeLockedByWrapper",
            "independentlyRehashedByVerifier",
            "buildControls",
        },
        "qualificationClosure",
    )
    require(
        qualification["algorithm"] == QUALIFICATION_CLOSURE_ALGORITHM
        and qualification["sameHandleLifetimeLockedByWrapper"] is True
        and qualification["independentlyRehashedByVerifier"] is True,
        "qualification closure contract drifted",
    )
    for name in (
        "fileCount",
        "totalBytes",
        "explicitFileCount",
        "explicitTotalBytes",
    ):
        require_positive_int(qualification[name], f"qualificationClosure.{name}")
    require_sha256_value(
        qualification["inventorySha256"],
        "qualificationClosure.inventorySha256",
    )
    require_sha256_value(
        qualification["aggregateSha256"],
        "qualificationClosure.aggregateSha256",
    )
    expected_roots = {
        "emsdk",
        "emscripten-cache",
        "outer-cmake",
        "ninja",
        "vcpkg",
        "vcpkg-port-cmake",
        "vcpkg-target",
        "vcpkg-host-runtime",
    }
    root_counts = require_mapping(
        qualification["rootFileCounts"],
        "qualificationClosure.rootFileCounts",
    )
    root_bytes = require_mapping(
        qualification["rootByteCounts"],
        "qualificationClosure.rootByteCounts",
    )
    require_exact_keys(root_counts, expected_roots, "qualification root counts")
    require_exact_keys(root_bytes, expected_roots, "qualification root bytes")
    for label in expected_roots:
        require_positive_int(
            root_counts[label],
            f"qualification root file count {label}",
        )
        require_positive_int(
            root_bytes[label],
            f"qualification root byte count {label}",
        )
    require(
        sum(root_counts.values()) + qualification["explicitFileCount"]
        == qualification["fileCount"]
        and sum(root_bytes.values()) + qualification["explicitTotalBytes"]
        == qualification["totalBytes"],
        "qualification closure per-source totals drifted",
    )
    build_controls = require_mapping(
        qualification["buildControls"],
        "qualificationClosure.buildControls",
    )
    require_exact_keys(
        build_controls,
        {
            "schemaVersion",
            "algorithm",
            "manifest",
            "fileCount",
            "totalBytes",
            "inventorySha256",
            "aggregateSha256",
            "sameHandleLifetimeLockedByWrapper",
            "configureMustBeSettledBeforeQualification",
            "selectedTargetProducerlessInputs",
            "commandOnlyImmutableInputs",
            "mutableGeneratorStateResetPaths",
            "allSelectedBuildInputsClassified",
        },
        "qualificationClosure.buildControls",
    )
    require(
        build_controls["schemaVersion"] == 1
        and build_controls["algorithm"] == QUALIFICATION_CLOSURE_ALGORITHM
        and build_controls["manifest"]
        == "tools/wasm-probe/build-control-manifest.txt"
        and build_controls["fileCount"] == len(EXPECTED_BUILD_CONTROL_PATHS)
        and build_controls["sameHandleLifetimeLockedByWrapper"] is True
        and build_controls[
            "configureMustBeSettledBeforeQualification"
        ]
        is True,
        "qualification build-control contract drifted",
    )
    require(
        isinstance(build_controls["selectedTargetProducerlessInputs"], list)
        and build_controls["selectedTargetProducerlessInputs"]
        and set(build_controls["selectedTargetProducerlessInputs"]).issubset(
            set(EXPECTED_BUILD_CONTROL_PATHS)
        )
        and build_controls["commandOnlyImmutableInputs"]
        == list(QUALIFICATION_COMMAND_ONLY_BUILD_CONTROL_PATHS)
        and build_controls["mutableGeneratorStateResetPaths"]
        == list(QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS)
        and build_controls["allSelectedBuildInputsClassified"] is True,
        "qualification selected build-input classification drifted",
    )
    require_positive_int(
        build_controls["totalBytes"],
        "qualificationClosure.buildControls.totalBytes",
    )
    require_sha256_value(
        build_controls["inventorySha256"],
        "qualificationClosure.buildControls.inventorySha256",
    )
    require_sha256_value(
        build_controls["aggregateSha256"],
        "qualificationClosure.buildControls.aggregateSha256",
    )


def validate_qualification_build_evidence(value: Any) -> None:
    build = require_mapping(value, "qualificationBuild")
    require_exact_keys(
        build,
        {
            "targets",
            "cleanCommand",
            "buildCommand",
            "cleanedOutputCount",
            "executedEdgeCount",
            "mutableAutogenStatePaths",
            "removedMutableAutogenStateCount",
            "allMutableAutogenStateAbsentBeforeCleanRebuild",
            "allCommandsRanInsideQualificationClosure",
        },
        "qualificationBuild",
    )
    targets = [
        "RhythmGameWasmCLauncherProbe",
        "RhythmGameWasmProbe",
    ]
    prefix = [
        ".toolchains/ninja-1.13.2-win/ninja.exe",
        "-C",
        "tools/wasm-probe/build/wasm-release",
    ]
    require(
        build["targets"] == targets
        and build["cleanCommand"] == [*prefix, "-t", "clean", *targets]
        and build["buildCommand"] == [*prefix, "-v", *targets]
        and build["mutableAutogenStatePaths"]
        == list(QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS)
        and build[
            "allMutableAutogenStateAbsentBeforeCleanRebuild"
        ]
        is True
        and build["allCommandsRanInsideQualificationClosure"] is True,
        "qualification clean rebuild command contract drifted",
    )
    require_exact_type(
        build["cleanedOutputCount"],
        int,
        "qualificationBuild.cleanedOutputCount",
    )
    require(
        build["cleanedOutputCount"] >= 0,
        "qualification clean output count is negative",
    )
    require_positive_int(
        build["executedEdgeCount"],
        "qualificationBuild.executedEdgeCount",
    )
    require_exact_type(
        build["removedMutableAutogenStateCount"],
        int,
        "qualificationBuild.removedMutableAutogenStateCount",
    )
    require(
        0
        <= build["removedMutableAutogenStateCount"]
        <= len(QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS),
        "qualification removed mutable Autogen state count drifted",
    )


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
            "qualificationClosure",
            "qualificationBuild",
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
        evidence["schemaVersion"] == 4
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
    validate_qualification_closure_evidence(evidence["qualificationClosure"])
    validate_qualification_build_evidence(evidence["qualificationBuild"])
    validate_build_freshness_evidence(evidence["buildFreshness"])
    validate_cmake_evidence(evidence["cmakeBuild"])
    validate_qt_evidence(evidence["qt"])
    validate_features_evidence(evidence["featuresAndAutogen"])
    validate_compile_evidence(evidence["compileCommands"])
    validate_application_link_evidence(evidence["applicationLink"])
    qualification = require_mapping(
        evidence["qualificationClosure"],
        "qualificationClosure",
    )
    application_link = require_mapping(
        evidence["applicationLink"],
        "applicationLink",
    )
    selected_binding = require_mapping(
        application_link["selectedLinkArtifactBinding"],
        "applicationLink.selectedLinkArtifactBinding",
    )
    require(
        qualification["aggregateSha256"]
        == selected_binding["qualificationAggregateSha256"],
        "qualification closure and selected-link identity disagree",
    )
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
