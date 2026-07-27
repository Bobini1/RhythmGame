import copy
import hashlib
import json
import os
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

import verify_build


class VerifyBuildContractTest(unittest.TestCase):
    @staticmethod
    def overlay_evidence_fixture() -> dict[str, object]:
        repo = Path(verify_build.__file__).resolve().parents[3]
        roots = {
            "qtbase": repo / "vcpkgOverlayPortsWasm" / "qtbase",
            "qtdeclarative": (
                repo / "vcpkgOverlayPorts" / "qtdeclarative"
            ),
            "qtmultimedia": (
                repo / "vcpkgOverlayPortsWasm" / "qtmultimedia"
            ),
        }
        contracts = {
            "qtbase": {
                "baselineTree": verify_build.EXPECTED_QTBASE_TREE,
                "modifiedBaselineFiles": [
                    "portfile.cmake",
                    "vcpkg.json",
                ],
                "lineEndingOnlyBaselineFiles": [],
                "removedBaselineFiles": [],
                "addedFiles": [
                    "preserve-wasm-event-composed-path.patch",
                    "restore-wasm-version-check.patch",
                ],
            },
            "qtdeclarative": {
                "baselineTree": verify_build.EXPECTED_QTDECLARATIVE_TREE,
                "modifiedBaselineFiles": [
                    "portfile.cmake",
                    "vcpkg.json",
                ],
                "lineEndingOnlyBaselineFiles": ["port.data.cmake"],
                "removedBaselineFiles": [],
                "addedFiles": [
                    "24205cd-qquickwindow-child-window-stacking.patch"
                ],
            },
            "qtmultimedia": {
                "baselineTree": verify_build.EXPECTED_QTMULTIMEDIA_TREE,
                "modifiedBaselineFiles": [
                    "portfile.cmake",
                    "vcpkg.json",
                ],
                "lineEndingOnlyBaselineFiles": [],
                "removedBaselineFiles": [
                    "ffmpeg-devendor-signalsmith-stretch.patch",
                    "ffmpeg.patch",
                    "fix-msvc-x86-propvariant.patch",
                    "remove-static-ssl-stub.patch",
                    "static_find_modules.patch",
                ],
                "addedFiles": [
                    "canonicalize-wasm-build-paths.patch",
                    "correct-wasm-media-lifecycle.patch",
                    "defer-wasm-media-device-notifications.patch"
                ],
            },
        }
        evidence: dict[str, object] = {}
        for port, inventory in (
            verify_build.EXPECTED_OVERLAY_INVENTORY.items()
        ):
            hashes = {
                relative: verify_build.sha256(roots[port] / relative)
                for relative in inventory
            }
            evidence[port] = {
                **contracts[port],
                "fileCount": len(inventory),
                "fileSha256": hashes,
                "aggregateSha256": (
                    verify_build.EXPECTED_OVERLAY_AGGREGATE_SHA256[port]
                ),
            }
        evidence["reviewedDelta"] = {
            "qtbase": [
                "restore Emscripten version-helper include and check",
                "snapshot queued Wasm DOM event composedPath",
                "export synchronous native and promising full Wasm pumps",
                "use additive JSPI delivery depth accounting",
                "erase exact nested current-event context on LIFO unwind",
                "run posted/native/timer/deferred-delete application cycle",
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
            "qtmultimedia": [
                "canonicalize Wasm compiler and macro source paths",
                "omit non-Wasm baseline FFmpeg GObject and MSVC patches",
                "publish singleton before asynchronous device enumeration",
                "reconcile browser devices while retaining OpenAL defaults",
                "serialize and keep devicechange enumeration non-suspending",
                "queue five media-device cache notifications",
                "reject startup emptied as natural playback end",
                "silence idempotent Wasm media-player stop",
                "initialize optional Wasm media input stream pointer",
                "make Wasm video-output stop re-entry and null safe",
                "leave natural-end teardown to explicit source cleanup",
            ],
        }
        return evidence

    @staticmethod
    def overlay_hash_aggregate(file_hashes: dict[str, str]) -> str:
        aggregate = hashlib.sha256()
        for relative, digest in sorted(file_hashes.items()):
            aggregate.update(relative.encode("utf-8") + b"\0")
            aggregate.update(digest.encode("ascii") + b"\n")
        return aggregate.hexdigest()

    @staticmethod
    def qtmultimedia_reproducibility_fixture(
        repo: Path,
    ) -> dict[str, object]:
        primary_manifest = repo / "tools" / "wasm-probe" / "vcpkg.json"
        primary_manifest.parent.mkdir(parents=True)
        primary_manifest_payload = {
            "name": "reproducibility-fixture",
            "version-semver": "0.1.0",
            "builtin-baseline": verify_build.EXPECTED_VCPKG_COMMIT,
            "dependencies": ["qtmultimedia"],
        }
        primary_manifest.write_text(
            json.dumps(primary_manifest_payload, indent=2) + "\n",
            encoding="utf-8",
        )

        secondary_root = (
            repo / verify_build.QTMULTIMEDIA_SECONDARY_RELATIVE_ROOT
        )
        secondary_manifest = secondary_root / "manifest" / "vcpkg.json"
        secondary_manifest.parent.mkdir(parents=True)
        secondary_manifest_payload = dict(primary_manifest_payload)
        del secondary_manifest_payload["builtin-baseline"]
        secondary_manifest.write_text(
            json.dumps(secondary_manifest_payload, indent=2) + "\n",
            encoding="utf-8",
        )

        primary_build = (
            repo
            / ".wb"
            / "qtmultimedia"
            / f"{verify_build.TARGET_TRIPLET}-rel"
        )
        primary_source = (
            repo / ".wb" / "qtmultimedia" / "src" / "primary.clean"
        )
        secondary_buildtrees = secondary_root / "buildtrees"
        secondary_build = (
            secondary_buildtrees
            / "qtmultimedia"
            / f"{verify_build.TARGET_TRIPLET}-rel"
        )
        secondary_source = (
            secondary_buildtrees
            / "qtmultimedia"
            / "src"
            / "primary.clean"
        )
        for path in (
            primary_build,
            primary_source,
            secondary_build,
            secondary_source,
            secondary_root / "installed",
            secondary_root / "binary-cache",
            secondary_root / "packages",
            secondary_root / "probe-configure",
            repo / ".wasm-vcpkg" / "downloads",
        ):
            path.mkdir(parents=True, exist_ok=True)
        expected_outer_cmake = (
            repo
            / ".toolchains"
            / verify_build.EXPECTED_OUTER_CMAKE_LOCK_ENTRY["directory"]
            / "bin"
            / "cmake.exe"
        )
        expected_vcpkg_toolchain = (
            repo
            / ".toolchains"
            / f"vcpkg-{verify_build.EXPECTED_VCPKG_COMMIT[:8]}"
            / "scripts"
            / "buildsystems"
            / "vcpkg.cmake"
        )
        expected_chainload_toolchain = (
            repo / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
        )
        expected_sysroot = (
            repo
            / ".toolchains"
            / f"emscripten-cache-{verify_build.EXPECTED_EMSCRIPTEN}"
            / "sysroot"
        )
        (primary_build / "CMakeCache.txt").write_text(
            f"CMAKE_HOME_DIRECTORY:INTERNAL={primary_source.as_posix()}\n",
            encoding="utf-8",
        )
        (secondary_build / "CMakeCache.txt").write_text(
            f"CMAKE_HOME_DIRECTORY:INTERNAL={secondary_source.as_posix()}\n",
            encoding="utf-8",
        )
        install_options = ";".join((
            f"--x-buildtrees-root={secondary_buildtrees.as_posix()}",
            f"--x-packages-root={(secondary_root / 'packages').as_posix()}",
            (
                "--downloads-root="
                f"{(repo / '.wasm-vcpkg' / 'downloads').as_posix()}"
            ),
            "--binarysource=clear",
            (
                "--binarysource=files,"
                f"{(secondary_root / 'binary-cache').as_posix()},"
                "readwrite"
            ),
        ))
        (secondary_root / "probe-configure" / "CMakeCache.txt").write_text(
            (
                f"CMAKE_COMMAND:INTERNAL={expected_outer_cmake.as_posix()}\n"
                "CMAKE_GENERATOR:INTERNAL=Ninja\n"
                "CMAKE_HOME_DIRECTORY:INTERNAL="
                f"{(repo / 'tools' / 'wasm-probe').as_posix()}\n"
                "CMAKE_INSTALL_PREFIX:PATH="
                f"{expected_sysroot.as_posix()}\n"
                "CMAKE_TOOLCHAIN_FILE:FILEPATH="
                f"{expected_vcpkg_toolchain.as_posix()}\n"
                "VCPKG_CHAINLOAD_TOOLCHAIN_FILE:FILEPATH="
                f"{expected_chainload_toolchain.as_posix()}\n"
                "VCPKG_HOST_TRIPLET:STRING="
                f"{verify_build.HOST_TRIPLET}\n"
                "VCPKG_MANIFEST_DIR:PATH="
                f"{secondary_manifest.parent.as_posix()}\n"
                "VCPKG_INSTALLED_DIR:PATH="
                f"{(secondary_root / 'installed').as_posix()}\n"
                "VCPKG_OVERLAY_PORTS:STRING="
                f"{(repo / 'vcpkgOverlayPorts').as_posix()};"
                f"{(repo / 'vcpkgOverlayPortsWasm').as_posix()}\n"
                "VCPKG_OVERLAY_TRIPLETS:PATH="
                f"{(repo / 'vcpkgTriplets').as_posix()}\n"
                "VCPKG_TARGET_TRIPLET:STRING="
                f"{verify_build.TARGET_TRIPLET}\n"
                "VCPKG_FEATURE_FLAGS:STRING=-versions\n"
                f"VCPKG_INSTALL_OPTIONS:STRING={install_options}\n"
            ),
            encoding="utf-8",
        )
        abi_info = (
            secondary_buildtrees
            / "qtmultimedia"
            / f"{verify_build.TARGET_TRIPLET}.vcpkg_abi_info.txt"
        )
        abi_info.write_bytes(b"secondary reproducibility ABI fixture\n")
        abi = verify_build.sha256(abi_info)
        binary_contract = {
            "abi": abi,
            "aggregateSha256": "a" * 64,
            "memberCount": 2,
            "totalBytes": 42,
        }
        binary_identity = {
            "BinaryDeterministicPayloadAggregateSha256": "b" * 64,
            "BinaryInstallableFileCount": 3,
            "BinaryPackageCacheFile": (
                f"{verify_build.QTMULTIMEDIA_SECONDARY_RELATIVE_ROOT}/"
                f"binary-cache/{abi[:2]}/{abi}.zip"
            ),
            "BinaryPayloadAggregateSha256": (
                binary_contract["aggregateSha256"]
            ),
            "BinaryPayloadMemberCount": binary_contract["memberCount"],
            "BinaryPayloadTotalBytes": binary_contract["totalBytes"],
        }
        return {
            "abi": abi,
            "abiInfo": abi_info,
            "binaryContract": binary_contract,
            "binaryIdentity": binary_identity,
            "primaryCompileIdentity": copy.deepcopy(
                verify_build.EXPECTED_QTMULTIMEDIA_COMPILE_COMMAND_IDENTITY
            ),
            "primarySource": primary_source,
            "secondaryBuild": secondary_build,
            "secondaryCompileIdentity": copy.deepcopy(
                verify_build.EXPECTED_QTMULTIMEDIA_COMPILE_COMMAND_IDENTITY
            ),
            "secondaryProbeCache": (
                secondary_root / "probe-configure" / "CMakeCache.txt"
            ),
            "secondaryRoot": secondary_root,
            "secondarySource": secondary_source,
        }

    def application_link_fixture(
        self,
        *response_arguments: str,
    ) -> tuple[str, Path, Path, Path, str, str]:
        repo = Path(verify_build.__file__).resolve().parents[3]
        emsdk = repo / ".toolchains" / "emsdk-4.0.7"
        expected = emsdk / "upstream" / "emscripten" / "em++.bat"
        em_config = emsdk / ".emscripten"
        controller_directory = tempfile.TemporaryDirectory()
        self.addCleanup(controller_directory.cleanup)
        build = Path(controller_directory.name)
        adapter_arguments = [
            str(repo / verify_build.EXPECTED_EMSDK_PYTHON),
            "-I",
            "-B",
            str(
                repo
                / "tools"
                / "wasm-probe"
                / "scripts"
                / "invoke_emscripten_driver.py"
            ),
            "--lock",
            str(
                repo
                / "tools"
                / "wasm-probe"
                / "toolchain-lock.json"
            ),
            "--auditor",
            str(
                repo
                / "tools"
                / "wasm-probe"
                / "scripts"
                / "audit_emscripten_response_files.py"
            ),
            "--emscripten-root",
            str(expected.parent),
            "--em-config",
            str(em_config),
            "--em-config-sha256",
            verify_build.sha256(em_config),
            "--cache-root",
            str(repo / ".toolchains" / "emscripten-cache-4.0.7"),
            "--driver-kind",
            "em++",
            "--",
            str(expected),
            "-pthread",
            "-fwasm-exceptions",
            "-sSUPPORT_LONGJMP=wasm",
            "-sJSPI=1",
            "-sAUDIO_WORKLET=1",
            "-sWASM_WORKERS=1",
            "-sPTHREAD_POOL_SIZE=4",
            "-sPTHREAD_POOL_SIZE_STRICT=2",
            "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
            r"@CMakeFiles\RhythmGameWasmProbe.rsp",
            "-o",
            "RhythmGameWasmProbe.js",
        ]
        body = subprocess.list2cmdline(adapter_arguments)
        post_build_controller = (
            build
            / Path(
                verify_build.APPLICATION_POST_BUILD_CONTROLLER.replace(
                    "\\",
                    "/",
                )
            )
        )
        post_build_controller.parent.mkdir(parents=True)
        post_build_controller.write_bytes(b"@echo off\r\nexit /b 0\r\n")
        post_build_token = verify_build.sha256(post_build_controller)[:16]
        native_cmd = (
            Path(os.environ["SystemRoot"])
            / "System32"
            / "cmd.exe"
        )
        command = (
            f'{native_cmd} /C "cd . && {body}'
            f" && {verify_build.APPLICATION_POST_BUILD_CONTROLLER}"
            f' {post_build_token}"'
        )
        libraries = " ".join(
            (
                "libWasmProbeExceptionBoundary.a",
                *response_arguments,
            )
        )
        link_outputs = " ".join(
            (
                *verify_build.APPLICATION_LINK_OUTPUTS,
                "|",
                *verify_build.APPLICATION_LINK_IMPLICIT_OUTPUTS,
            )
        )
        build_ninja = (
            f"build {link_outputs}: "
            "CXX_EXECUTABLE_LINKER__RhythmGameWasmProbe_Release "
            "object.o\n"
            f"  LINK_LIBRARIES = {libraries}\n"
            "  RSP_FILE = CMakeFiles\\RhythmGameWasmProbe.rsp\n"
        )
        rules_ninja = (
            "rule CXX_EXECUTABLE_LINKER__RhythmGameWasmProbe_Release\n"
            "  command = em++.bat @$RSP_FILE -o $TARGET_FILE\n"
            "  rspfile = $RSP_FILE\n"
            "  rspfile_content = $in $LINK_PATH $LINK_LIBRARIES\n"
        )
        return command, repo, build, expected, build_ninja, rules_ninja

    @staticmethod
    def dependency_binding_fixture(
        root: Path,
    ) -> tuple[Path, Path, Path, Path, dict[str, object]]:
        repo = root / "repo"
        probe = repo / "tools" / "wasm-probe"
        build = probe / "build" / "wasm-release"
        generated = build / "generated"
        target = (
            repo
            / ".wasm-vcpkg"
            / "installed"
            / verify_build.TARGET_TRIPLET
        )
        generated.mkdir(parents=True)
        target.mkdir(parents=True)
        (probe / "scripts").mkdir()
        (probe / "dependency-archive-contract.json").write_text(
            json.dumps(
                {
                    "schemaVersion": 2,
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "archiveSuffix": ".a",
                    "wasmObjectSuffix": ".o",
                    "markerPrefix": (
                        "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
                    ),
                    "targetTriplet": verify_build.TARGET_TRIPLET,
                }
            ),
            encoding="utf-8",
        )
        (
            probe / "scripts" / "generate_dependency_digest.py"
        ).write_text("# authenticated fixture\n", encoding="utf-8")
        installed_archive = target / "lib" / "libQt6Core.a"
        installed_archive.parent.mkdir()
        installed_archive.write_bytes(b"!<arch>\nauthenticated Qt archive\n")
        installed_object = (
            target
            / "Qt6"
            / "plugins"
            / "objects-Release"
            / "Plugin_init.cpp.o"
        )
        installed_object.parent.mkdir(parents=True)
        installed_object.write_bytes(
            b"\x00asm\x01\x00\x00\x00installed object"
        )
        boundary = build / "libWasmProbeExceptionBoundary.a"
        boundary.write_bytes(b"!<arch>\nexception boundary\n")
        (build / "object.o").write_bytes(
            b"\x00asm\x01\x00\x00\x00build object"
        )
        superset = verify_build._static_link_input_superset(target)
        (generated / "dependency-archive-digest.json").write_text(
            json.dumps(superset),
            encoding="utf-8",
        )
        marker = (
            "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
            + superset["aggregateSha256"]
        )
        (generated / "ProbeDependencyDigest.cpp").write_text(
            marker,
            encoding="ascii",
        )
        marker_bytes = marker.encode("ascii")
        length = len(marker_bytes)
        encoded_length = bytearray()
        while True:
            byte = length & 0x7F
            length >>= 7
            if length:
                byte |= 0x80
            encoded_length.append(byte)
            if not length:
                break
        (build / "RhythmGameWasmProbe.wasm").write_bytes(
            b"\x00asm\x01\x00\x00\x00"
            + b"\x0b"
            + bytes(encoded_length)
            + marker_bytes
        )
        identity = {
            key: superset[key]
            for key in verify_build.TARGET_STATIC_LINK_INPUT_IDENTITY_FIELDS
        }
        return repo, build, boundary, installed_archive, identity

    def test_windows_command_parser_preserves_quoted_paths_and_flags(
        self,
    ) -> None:
        command = (
            '"T:\\tool chain\\em++.bat" '
            '"T:\\source tree\\ProbeState.cpp" '
            '-pthread -fwasm-exceptions -sSUPPORT_LONGJMP=wasm '
            '-DNAME=\\"quoted value\\"'
        )
        self.assertEqual(
            verify_build.split_windows_command_line(command),
            [
                r"T:\tool chain\em++.bat",
                r"T:\source tree\ProbeState.cpp",
                "-pthread",
                "-fwasm-exceptions",
                "-sSUPPORT_LONGJMP=wasm",
                '-DNAME="quoted',
                'value"',
            ],
        )

    def test_compile_entry_prefers_structured_arguments(self) -> None:
        entry = {
            "command": "this string must not be parsed",
            "arguments": [
                r"T:\emsdk\em++.bat",
                "-pthread",
                r"T:\source tree\ProbeState.cpp",
            ],
        }
        self.assertEqual(
            verify_build.compile_entry_arguments(entry),
            entry["arguments"],
        )

    def test_compile_entry_fails_closed_without_a_command(self) -> None:
        with self.assertRaisesRegex(AssertionError, "command or arguments"):
            verify_build.compile_entry_arguments({"file": "ProbeState.cpp"})

    def test_qualification_closure_rehashes_exact_bytes_and_environment(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = root / "runtime"
            runtime.mkdir()
            runtime_file = runtime / "tool.bin"
            explicit = root / "input.txt"
            runtime_file.write_bytes(b"runtime")
            explicit.write_bytes(b"input")
            records = [
                ("runtime/tool.bin", runtime_file.read_bytes()),
                ("repo/input.txt", explicit.read_bytes()),
            ]
            inventory = hashlib.sha256()
            aggregate = hashlib.sha256()
            for logical, data in sorted(records):
                inventory.update(f"{logical}\n".encode())
                aggregate.update(
                    (
                        f"{logical}\0{len(data)}\0"
                        f"{hashlib.sha256(data).hexdigest()}\n"
                    ).encode()
                )
            environment = {
                "RHYTHMGAME_WASM_QUALIFICATION": "1",
                "RHYTHMGAME_WASM_QUALIFICATION_ALGORITHM": (
                    verify_build.QUALIFICATION_CLOSURE_ALGORITHM
                ),
                "RHYTHMGAME_WASM_QUALIFICATION_FILE_COUNT": "2",
                "RHYTHMGAME_WASM_QUALIFICATION_TOTAL_BYTES": "12",
                "RHYTHMGAME_WASM_QUALIFICATION_INVENTORY_SHA256": (
                    inventory.hexdigest()
                ),
                "RHYTHMGAME_WASM_QUALIFICATION_AGGREGATE_SHA256": (
                    aggregate.hexdigest()
                ),
            }
            identity = verify_build.qualification_closure_identity(
                [("runtime", runtime, ())],
                [("repo/input.txt", explicit)],
                environment,
            )
            self.assertEqual(identity["fileCount"], 2)
            self.assertEqual(identity["rootFileCounts"], {"runtime": 1})
            self.assertEqual(identity["explicitFileCount"], 1)

            explicit.write_bytes(b"tampered")
            with self.assertRaisesRegex(
                AssertionError,
                "does not match independently hashed bytes",
            ):
                verify_build.qualification_closure_identity(
                    [("runtime", runtime, ())],
                    [("repo/input.txt", explicit)],
                    environment,
                )

    def test_build_control_manifest_hashes_exact_graph_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            probe = repo / "tools" / "wasm-probe"
            build = probe / "build" / "wasm-release"
            probe.mkdir(parents=True)
            (probe / "input-manifest.txt").write_bytes(
                b"tools/wasm-probe/build-control-manifest.txt\n",
            )
            (probe / "build-control-manifest.txt").write_bytes(
                "".join(
                    f"{relative}\n"
                    for relative in verify_build.EXPECTED_BUILD_CONTROL_PATHS
                ).encode("utf-8"),
            )
            command_only = (
                verify_build.QUALIFICATION_COMMAND_ONLY_BUILD_CONTROL_PATHS
            )
            for index, relative in enumerate(
                verify_build.EXPECTED_BUILD_CONTROL_PATHS
            ):
                path = build / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f"control-{index}\n", encoding="utf-8")
            (build / "build.ninja").write_text(
                "\n".join(
                    (
                        "# "
                        + " ".join(command_only),
                        (
                            "build RhythmGameWasmCLauncherProbe: phony "
                            "generated/ProbeDependencyDigest.cpp"
                        ),
                        (
                            "build RhythmGameWasmProbe: phony "
                            "generated/ProbeInputDigest.cpp"
                        ),
                        "",
                    )
                ),
                encoding="utf-8",
            )
            autogen_targets = (
                "RhythmGameWasmCLauncherProbe",
                "RhythmGameWasmProbe",
                "WasmProbeExceptionBoundary",
            )
            for target in autogen_targets:
                autogen = (
                    build
                    / "CMakeFiles"
                    / f"{target}_autogen.dir"
                    / "AutogenInfo.json"
                )
                autogen.write_text(
                    json.dumps(
                        {
                            "PARSE_CACHE_FILE": str(
                                autogen.with_name("ParseCache.txt")
                            ),
                            "SETTINGS_FILE": str(
                                autogen.with_name("AutogenUsed.txt")
                            ),
                        }
                    ),
                    encoding="utf-8",
                )

            identity = verify_build.probe_build_control_identity(repo)
            self.assertEqual(
                identity["fileCount"],
                len(verify_build.EXPECTED_BUILD_CONTROL_PATHS),
            )
            self.assertTrue(
                identity["sameHandleLifetimeLockedByWrapper"],
            )
            self.assertTrue(
                identity["configureMustBeSettledBeforeQualification"],
            )

            (build / "CMakeCache.txt").write_text(
                "changed control\n",
                encoding="utf-8",
            )
            changed = verify_build.probe_build_control_identity(repo)
            self.assertNotEqual(
                identity["aggregateSha256"],
                changed["aggregateSha256"],
            )

    def test_runtime_post_build_controller_is_command_only_control(self) -> None:
        controller = "CMakeFiles/RhythmGameWasmProbe.dir/post-build.bat"
        self.assertIn(
            controller,
            verify_build.EXPECTED_BUILD_CONTROL_PATHS,
        )
        self.assertIn(
            controller,
            verify_build.QUALIFICATION_COMMAND_ONLY_BUILD_CONTROL_PATHS,
        )
        self.assertEqual(
            verify_build.EXPECTED_BUILD_CONTROL_PATHS,
            tuple(
                sorted(
                    verify_build.EXPECTED_BUILD_CONTROL_PATHS,
                    key=str.casefold,
                )
            ),
        )

    def test_qualified_rebuild_clears_exact_mutable_autogen_state(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            for relative in (
                verify_build.QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS
            ):
                path = build / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("stale generator state\n", encoding="utf-8")
            outside = build / "must-remain.txt"
            outside.write_text("preserved\n", encoding="utf-8")

            removed = verify_build.clear_mutable_autogen_state(build)

            self.assertEqual(
                removed,
                list(
                    verify_build.QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS
                ),
            )
            self.assertTrue(
                all(
                    not (build / relative).exists()
                    for relative in (
                        verify_build.QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS
                    )
                )
            )
            self.assertEqual(
                outside.read_text("utf-8"),
                "preserved\n",
            )
            self.assertEqual(
                verify_build.clear_mutable_autogen_state(build),
                [],
            )

    def test_compile_argv_parity_accepts_only_depfile_bookkeeping(self) -> None:
        compile_database = [
            r"T:\emsdk\emcc.bat",
            "-pthread",
            "-o",
            r"CMakeFiles\probe.o",
            "-c",
            r"T:\source tree\probe.c",
        ]
        ninja = [
            r"T:\emsdk\emcc.bat",
            "-pthread",
            "-MD",
            "-MT",
            "CMakeFiles/probe.o",
            "-MF",
            r"CMakeFiles\probe.o.d",
            "-o",
            "CMakeFiles/probe.o",
            "-c",
            "T:/source tree/probe.c",
        ]
        self.assertEqual(
            verify_build.require_compile_argv_parity(
                compile_database,
                ninja,
                "fixture",
            ),
            [
                r"T:\emsdk\emcc.bat",
                "-pthread",
                "-o",
                "CMakeFiles/probe.o",
                "-c",
                "T:/source tree/probe.c",
            ],
        )

    def test_compile_argv_parity_rejects_unmodeled_ninja_flag(self) -> None:
        compile_database = [
            r"T:\emsdk\emcc.bat",
            "-pthread",
            "-o",
            "probe.o",
            "-c",
            "probe.c",
        ]
        ninja = [
            r"T:\emsdk\emcc.bat",
            "-pthread",
            "-fno-wasm-exceptions",
            "-MD",
            "-MT",
            "probe.o",
            "-MF",
            "probe.o.d",
            "-o",
            "probe.o",
            "-c",
            "probe.c",
        ]
        with self.assertRaisesRegex(
            AssertionError,
            "expanded Ninja compiler argv differs",
        ):
            verify_build.require_compile_argv_parity(
                compile_database,
                ninja,
                "fixture",
            )

    def test_selected_compile_output_cannot_be_masked_by_source_decoy(
        self,
    ) -> None:
        selected = {r"t:\build\unsafe.o"}
        safe_decoy = {r"t:\build\safe-decoy.o"}
        with self.assertRaisesRegex(
            AssertionError,
            "correlated by compile output",
        ):
            verify_build.require_selected_compile_output_correlation(
                selected,
                safe_decoy,
                safe_decoy,
            )
        verify_build.require_selected_compile_output_correlation(
            selected,
            selected | safe_decoy,
            selected | safe_decoy,
        )

    def test_compile_sidecar_rehashes_selected_header_and_object(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            probe = repo / "tools" / "wasm-probe"
            source = probe / "src" / "source.cpp"
            header = probe / "src" / "header.h"
            source.parent.mkdir(parents=True)
            source.write_text('#include "header.h"\n', encoding="utf-8")
            header.write_text("original\n", encoding="utf-8")
            (probe / "input-manifest.txt").write_bytes(
                b"tools/wasm-probe/src/header.h\n"
                b"tools/wasm-probe/src/source.cpp\n"
            )
            build = probe / "build"
            output = build / "CMakeFiles" / "source.cpp.o"
            output.parent.mkdir(parents=True)
            output.write_bytes(b"\0asm\1\0\0\0compiled")
            qualification = {
                "algorithm": verify_build.QUALIFICATION_CLOSURE_ALGORITHM,
                "fileCount": 1,
                "totalBytes": 1,
                "inventorySha256": "1" * 64,
                "aggregateSha256": "2" * 64,
            }
            dependencies = [
                {
                    "path": "repo-input/tools/wasm-probe/src/header.h",
                    "bytes": header.stat().st_size,
                    "sha256": verify_build.sha256(header),
                },
                {
                    "path": "repo-input/tools/wasm-probe/src/source.cpp",
                    "bytes": source.stat().st_size,
                    "sha256": verify_build.sha256(source),
                },
            ]
            arguments = ["-c", str(source), "-o", str(output)]
            closure = {
                "algorithm": verify_build.COMPILE_DEPENDENCY_ALGORITHM,
                "qualification": qualification,
                "driverKind": "em++",
                "output": "build-output/CMakeFiles/source.cpp.o",
                "arguments": [
                    verify_build.canonical_command(repo, argument)
                    for argument in arguments
                ],
                "dependencyDiscovery": {
                    "preScanMethod": "emscripten-M",
                    "actualCompileMethod": "MD-MF",
                    "exactPathSetMatch": True,
                    "dependencyCount": len(dependencies),
                },
                "dependencies": dependencies,
            }
            closure_bytes = (
                json.dumps(
                    closure,
                    ensure_ascii=True,
                    separators=(",", ":"),
                    sort_keys=True,
                )
                + "\n"
            ).encode()
            payload = {
                "schemaVersion": 1,
                **closure,
                "closureSha256": hashlib.sha256(
                    closure_bytes
                ).hexdigest(),
                "outputBytes": output.stat().st_size,
                "outputSha256": verify_build.sha256(output),
            }
            sidecar = output.with_name(
                output.name + ".rg-compile-inputs.json"
            )
            sidecar.write_bytes(
                (
                    json.dumps(
                        payload,
                        ensure_ascii=True,
                        separators=(",", ":"),
                        sort_keys=True,
                    )
                    + "\n"
                ).encode("utf-8")
            )
            records = {
                verify_build.path_key(output): {
                    "output": output,
                    "arguments": arguments,
                    "driverKind": "em++",
                }
            }
            evidence = verify_build.verify_compile_dependency_sidecars(
                repo,
                build,
                qualification,
                records,
            )
            self.assertEqual(evidence["selectedObjectCount"], 1)

            header.write_text("tampered\n", encoding="utf-8")
            with self.assertRaisesRegex(
                AssertionError,
                "current bytes drifted",
            ):
                verify_build.verify_compile_dependency_sidecars(
                    repo,
                    build,
                    qualification,
                    records,
                )

    def test_in_process_git_tree_hash_filters_recursive_siblings(self) -> None:
        files = {
            "a.txt": b"a",
            "nested/b.txt": b"b",
            "z.txt": b"z",
        }

        def object_id(kind: str, content: bytes) -> bytes:
            return hashlib.sha1(
                f"{kind} {len(content)}\0".encode("ascii") + content
            ).digest()

        nested_content = (
            b"100644 b.txt\0" + object_id("blob", b"b")
        )
        root_content = b"".join(
            (
                b"100644 a.txt\0" + object_id("blob", b"a"),
                b"40000 nested\0" + object_id("tree", nested_content),
                b"100644 z.txt\0" + object_id("blob", b"z"),
            )
        )
        self.assertEqual(
            verify_build.git_tree_id(files),
            object_id("tree", root_content),
        )

    def test_compile_evidence_requires_an_adapter_authenticated_c_edge(
        self,
    ) -> None:
        command_total = len(verify_build.EXPECTED_TARGET_COMPILE_PORTS)
        evidence = {
            "targetDatabaseCount": command_total,
            "targetDatabases": [
                (
                    f".wb/{port}/"
                    f"{verify_build.TARGET_TRIPLET}-rel/"
                    "compile_commands.json"
                )
                for port in verify_build.EXPECTED_TARGET_COMPILE_PORTS
            ],
            "targetCommandCounts": {
                "c": command_total,
                "cxx": command_total,
            },
            "targetSettingCoverage": {
                "c": {
                    setting: command_total
                    for setting in verify_build.C_COMPILE_SETTINGS
                },
                "cxx": {
                    setting: command_total
                    for setting in verify_build.CXX_COMPILE_SETTINGS
                },
            },
            "targetPortCommandCounts": {
                port: {"c": 1, "cxx": 1}
                for port in verify_build.EXPECTED_TARGET_COMPILE_PORTS
            },
            "forbiddenArgumentsAbsent": list(
                verify_build.FORBIDDEN_TARGET_ARGUMENTS
            ),
            "probeAdapterCommandCounts": {"c": 0, "cxx": 1},
            "probeCompileDbNinjaParity": {
                "expandedCommandSource": "ninja -t compdb -x",
                "matchedCommandCount": 1,
                "correlationKey": "directory-plus-output",
                "dependencyBookkeepingRemoved": ["-MD", "-MT", "-MF"],
                "exactAfterPathNormalization": True,
            },
            "selectedTargetGraph": {
                "target": "RhythmGameWasmProbe",
                "graphSource": "build.ninja",
                "objectOutputCount": 1,
                "archiveOutputCount": 1,
                "objectOutputsSha256": "0" * 64,
                "allSelectedOutputsMatchedByExactOutput": True,
            },
            "exceptionBoundary": {
                source: {
                    "settings": list(verify_build.CXX_COMPILE_SETTINGS),
                    "effectiveSettings": (
                        verify_build.CXX_COMPILE_EMSCRIPTEN_SETTINGS
                    ),
                    "commandSha256": "0" * 64,
                }
                for source in ("ExceptionBoundary.cpp", "ProbeState.cpp")
            },
        }
        with self.assertRaisesRegex(
            AssertionError,
            "no probe c compile command used the authenticated adapter",
        ):
            verify_build.validate_compile_evidence(evidence)

    def test_target_compile_database_discovery_excludes_host_builds(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            buildtrees = Path(directory)
            target = (
                buildtrees
                / "qtbase"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            host = (
                buildtrees
                / "qtbase"
                / "x64-windows-rg-host-release-rel"
                / "compile_commands.json"
            )
            target.parent.mkdir(parents=True)
            host.parent.mkdir(parents=True)
            target.write_text("[]", encoding="utf-8")
            host.write_text("[]", encoding="utf-8")

            self.assertEqual(
                verify_build.target_compile_databases(
                    buildtrees,
                    expected_ports=("qtbase",),
                ),
                [target],
            )

    def test_target_compile_database_discovery_rejects_wrong_layout(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            buildtrees = Path(directory)
            misplaced = (
                buildtrees
                / "qtbase"
                / "nested"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            misplaced.parent.mkdir(parents=True)
            misplaced.write_text("[]", encoding="utf-8")

            with self.assertRaisesRegex(
                AssertionError,
                "target compile database layout",
            ):
                verify_build.target_compile_databases(
                    buildtrees,
                    expected_ports=("qtbase",),
                )

    def test_target_compile_database_discovery_allows_locked_secondary(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            buildtrees = Path(directory) / ".wb"
            target = (
                buildtrees
                / "qtbase"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            secondary_relative = Path(
                verify_build.QTMULTIMEDIA_SECONDARY_RELATIVE_ROOT
            )
            self.assertEqual(secondary_relative.parts[0], ".wb")
            secondary = (
                buildtrees.joinpath(*secondary_relative.parts[1:])
                / "buildtrees"
                / "qtmultimedia"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            target.parent.mkdir(parents=True)
            secondary.parent.mkdir(parents=True)
            target.write_text("[]", encoding="utf-8")
            secondary.write_text("[]", encoding="utf-8")

            self.assertEqual(
                verify_build.target_compile_databases(
                    buildtrees,
                    expected_ports=("qtbase",),
                ),
                [target],
            )

    def test_target_compile_database_discovery_rejects_other_repro_roots(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            buildtrees = Path(directory) / ".wb"
            target = (
                buildtrees
                / "qtbase"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            quarantine = (
                buildtrees
                / "qtmultimedia-repro-quarantine"
                / "buildtrees"
                / "qtmultimedia"
                / "wasm32-emscripten-rg-rel"
                / "compile_commands.json"
            )
            target.parent.mkdir(parents=True)
            quarantine.parent.mkdir(parents=True)
            target.write_text("[]", encoding="utf-8")
            quarantine.write_text("[]", encoding="utf-8")

            with self.assertRaisesRegex(
                AssertionError,
                "target compile database layout",
            ):
                verify_build.target_compile_databases(
                    buildtrees,
                    expected_ports=("qtbase",),
                )

    def test_evidence_writer_rejects_partial_result_without_touching_output(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "evidence.json"
            output.write_text('{"sentinel": true}\n', encoding="utf-8")

            with self.assertRaisesRegex(
                AssertionError,
                "evidence top-level keys",
            ):
                verify_build.write_evidence_atomic(
                    output,
                    {
                        "schemaVersion": 1,
                        "gate": "1A",
                        "technicalProbePassed": True,
                        "gate1aPassed": True,
                        "gate0Satisfied": False,
                        "formalGate1EntryAuthorized": False,
                        "gate1Passed": False,
                        "unprovenUntilGate1B": list(
                            verify_build.GATE_1B_LIMITATIONS
                        ),
                    },
                )

            self.assertEqual(
                json.loads(output.read_text("utf-8")),
                {"sentinel": True},
            )

    def test_failed_audit_invalidates_stale_requested_output(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "evidence.json"
            output.write_text('{"stale": true}\n', encoding="utf-8")

            with mock.patch.object(
                verify_build,
                "build_evidence",
                side_effect=AssertionError("audit failed"),
            ):
                with self.assertRaisesRegex(AssertionError, "audit failed"):
                    verify_build.generate_evidence(
                        root,
                        root / "emsdk",
                        root / "vcpkg",
                        output,
                    )

            self.assertFalse(output.exists())

    def test_compiler_qualification_identity_excludes_verifier_evidence(
        self,
    ) -> None:
        qualification = {
            "algorithm": "sha256-path-null-digest-lf-v1",
            "fileCount": 69_617,
            "totalBytes": 3_063_834_491,
            "inventorySha256": "1" * 64,
            "aggregateSha256": "2" * 64,
            "buildControls": {
                "fileCount": 26,
                "aggregateSha256": "3" * 64,
            },
        }

        self.assertEqual(
            verify_build.compiler_qualification_identity(qualification),
            {
                "algorithm": "sha256-path-null-digest-lf-v1",
                "fileCount": 69_617,
                "totalBytes": 3_063_834_491,
                "inventorySha256": "1" * 64,
                "aggregateSha256": "2" * 64,
            },
        )

    def test_ninja_freshness_rejects_dirty_target_closure(self) -> None:
        repo = Path(verify_build.__file__).resolve().parents[3]
        ninja = (
            repo
            / ".toolchains"
            / "ninja-1.13.2-win"
            / "ninja.exe"
        )
        if not ninja.is_file():
            self.skipTest("requires the pinned Ninja executable")

        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            (build / "input.txt").write_text("changed", encoding="utf-8")
            (build / "build.ninja").write_text(
                (
                    "rule generate\n"
                    "  command = cmd.exe /d /c type $in > $out\n"
                    "build marker.txt: generate input.txt\n"
                    "build RhythmGameWasmProbe: phony marker.txt\n"
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                AssertionError,
                "not a no-op",
            ):
                verify_build.verify_ninja_noop(ninja, build)

    def test_qualification_clean_rebuild_executes_real_edges(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            build = (
                repo / "tools" / "wasm-probe" / "build" / "wasm-release"
            )
            ninja = (
                repo / ".toolchains" / "ninja-1.13.2-win" / "ninja.exe"
            )
            stale_state = (
                build
                / verify_build.QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS[0]
            )
            stale_state.parent.mkdir(parents=True)
            stale_state.write_text("stale\n", encoding="utf-8")
            for relative in (
                verify_build.QUALIFICATION_MUTABLE_AUTOGEN_STATE_PATHS[1:]
            ):
                (build / relative).parent.mkdir(
                    parents=True,
                    exist_ok=True,
                )
            environment = {
                "RHYTHMGAME_WASM_QUALIFICATION": "1",
                "RHYTHMGAME_WASM_QUALIFICATION_ALGORITHM": (
                    verify_build.QUALIFICATION_CLOSURE_ALGORITHM
                ),
                "RHYTHMGAME_WASM_QUALIFICATION_FILE_COUNT": "1",
                "RHYTHMGAME_WASM_QUALIFICATION_TOTAL_BYTES": "1",
                "RHYTHMGAME_WASM_QUALIFICATION_INVENTORY_SHA256": "1" * 64,
                "RHYTHMGAME_WASM_QUALIFICATION_AGGREGATE_SHA256": "2" * 64,
            }
            results = [
                subprocess.CompletedProcess(
                    [],
                    0,
                    "ninja: Entering directory `fixture'\n"
                    "Cleaning... 12 files.\n",
                    "",
                ),
                subprocess.CompletedProcess(
                    [],
                    0,
                    "[1/2] compile\n[2/2] link\n",
                    "",
                ),
            ]
            with mock.patch.dict(os.environ, environment, clear=True):
                with mock.patch.object(
                    verify_build.subprocess,
                    "run",
                    side_effect=results,
                ):
                    evidence = (
                        verify_build.clean_rebuild_selected_targets(
                            repo,
                            build,
                            ninja,
                        )
                    )
            self.assertEqual(evidence["cleanedOutputCount"], 12)
            self.assertEqual(evidence["executedEdgeCount"], 2)
            self.assertEqual(
                evidence["removedMutableAutogenStateCount"],
                1,
            )
            self.assertTrue(
                evidence[
                    "allMutableAutogenStateAbsentBeforeCleanRebuild"
                ]
            )

    def test_ninja_freshness_accepts_executed_glob_check_then_no_work(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            cmake = build / "cmake.exe"
            verify_globs = build / "CMakeFiles" / "VerifyGlobs.cmake"
            verify_globs.parent.mkdir()
            (build / "CMakeCache.txt").write_text(
                f"CMAKE_COMMAND:INTERNAL={cmake}\n",
                encoding="utf-8",
            )
            output = "\n".join(
                (
                    f"ninja: Entering directory `{build}'",
                    "[0/2] "
                    + subprocess.list2cmdline(
                        [str(cmake), "-P", str(verify_globs)]
                    ),
                    "ninja: no work to do.",
                )
            )
            with mock.patch.object(
                verify_build,
                "run_text",
                return_value=output,
            ):
                self.assertEqual(
                    verify_build.verify_ninja_noop(
                        build / "ninja.exe",
                        build,
                    ),
                    output.splitlines(),
                )

    def test_ninja_freshness_rejects_dry_run_regeneration_shape(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            cmake = build / "cmake.exe"
            source = build / "source"
            verify_globs = build / "CMakeFiles" / "VerifyGlobs.cmake"
            verify_globs.parent.mkdir()
            source.mkdir()
            (build / "CMakeCache.txt").write_text(
                (
                    f"CMAKE_COMMAND:INTERNAL={cmake}\n"
                    f"CMAKE_HOME_DIRECTORY:INTERNAL={source}\n"
                ),
                encoding="utf-8",
            )
            output = "\n".join(
                (
                    f"ninja: Entering directory `{build}'",
                    "[0/2] "
                    + subprocess.list2cmdline(
                        [str(cmake), "-P", str(verify_globs)]
                    ),
                    "[1/2] "
                    + subprocess.list2cmdline(
                        [
                            str(cmake),
                            "--regenerate-during-build",
                            f"-S{source}",
                            f"-B{build}",
                        ]
                    ),
                )
            )
            with mock.patch.object(
                verify_build,
                "run_text",
                return_value=output,
            ):
                with self.assertRaisesRegex(
                    AssertionError,
                    "not a no-op",
                ):
                    verify_build.verify_ninja_noop(
                        build / "ninja.exe",
                        build,
                    )

    def test_effective_emscripten_settings_accept_compact_and_split_forms(
        self,
    ) -> None:
        arguments = [
            "-sSUPPORT_LONGJMP=wasm",
            "-s",
            "JSPI",
            "-sJSPI=1",
            "-sAUDIO_WORKLET=1",
            "-s",
            "WASM_WORKERS=1",
            "-sPTHREAD_POOL_SIZE=4",
            "-s",
            "PTHREAD_POOL_SIZE_STRICT=2",
            "-sALLOW_BLOCKING_ON_MAIN_THREAD=0",
        ]

        result = verify_build.require_effective_emscripten_settings(
            arguments,
            verify_build.APPLICATION_EMSCRIPTEN_SETTINGS,
            "test link",
        )

        self.assertEqual(
            result["effectiveValues"],
            verify_build.APPLICATION_EMSCRIPTEN_SETTINGS,
        )
        self.assertEqual(result["occurrences"]["JSPI"], 2)

    def test_effective_emscripten_settings_reject_conflicts_and_negatives(
        self,
    ) -> None:
        cases = {
            "conflicting": ["-sJSPI=1", "-sJSPI=0"],
            "negative": ["-sJSPI=1", "-sNO_JSPI"],
            "compact Asyncify": ["-sJSPI=1", "-sASYNCIFY=1"],
            "split Asyncify": ["-sJSPI=1", "-s", "ASYNCIFY"],
        }
        for label, arguments in cases.items():
            with self.subTest(label=label):
                with self.assertRaises(AssertionError):
                    verify_build.require_effective_emscripten_settings(
                        arguments,
                        {"JSPI": "1"},
                        label,
                    )

    def test_wasm_exception_contract_rejects_negative_override(self) -> None:
        with self.assertRaisesRegex(
            AssertionError,
            "-fno-wasm-exceptions",
        ):
            verify_build.require_wasm_compile_contract(
                [
                    "-pthread",
                    "-fwasm-exceptions",
                    "-fno-wasm-exceptions",
                    "-sSUPPORT_LONGJMP=wasm",
                ],
                language="cxx",
                context="test compile",
            )

    def test_legacy_fexceptions_is_rejected_in_compile_and_link_streams(
        self,
    ) -> None:
        with self.assertRaisesRegex(AssertionError, "-fexceptions"):
            verify_build.require_wasm_compile_contract(
                [
                    "-pthread",
                    "-fwasm-exceptions",
                    "-fexceptions",
                    "-sSUPPORT_LONGJMP=wasm",
                ],
                language="cxx",
                context="test compile",
            )

        response_fixture = self.application_link_fixture("-fexceptions")
        with self.assertRaisesRegex(AssertionError, "-fexceptions"):
            verify_build.verify_application_link_argument_stream(
                *response_fixture
            )

        command, repo, build, expected, build_ninja, rules_ninja = (
            self.application_link_fixture()
        )
        direct_fixture = (
            command.replace(
                "-pthread -fwasm-exceptions",
                "-pthread -fwasm-exceptions -fexceptions",
            ),
            repo,
            build,
            expected,
            build_ninja,
            rules_ninja,
        )
        with self.assertRaisesRegex(AssertionError, "-fexceptions"):
            verify_build.verify_application_link_argument_stream(
                *direct_fixture
            )

    def test_application_link_parser_requires_exact_compiler(self) -> None:
        valid, repo, build, expected, _, _ = (
            self.application_link_fixture()
        )
        self.assertEqual(
            verify_build.parse_application_link_arguments(
                valid,
                repo,
                build,
                expected,
            )[0],
            str(expected),
        )

        wrong = valid.replace(
            str(expected),
            str(expected.with_name("other-em++.bat")),
        )
        with self.assertRaisesRegex(AssertionError, "pinned compiler"):
            verify_build.parse_application_link_arguments(
                wrong,
                repo,
                build,
                expected,
            )

    def test_application_link_parser_authenticates_post_build_tail(
        self,
    ) -> None:
        valid, repo, build, expected, _, _ = (
            self.application_link_fixture()
        )
        controller = (
            build
            / Path(
                verify_build.APPLICATION_POST_BUILD_CONTROLLER.replace(
                    "\\",
                    "/",
                )
            )
        )
        token = verify_build.sha256(controller)[:16]
        controller_tail = (
            f" && {verify_build.APPLICATION_POST_BUILD_CONTROLLER} {token}"
        )
        native_cmd = (
            Path(os.environ["SystemRoot"])
            / "System32"
            / "cmd.exe"
        )
        cases = {
            "missing controller": valid.replace(
                controller_tail,
                " && cd .",
            ),
            "wrong controller": valid.replace(
                verify_build.APPLICATION_POST_BUILD_CONTROLLER,
                r"CMakeFiles\OtherTarget.dir\post-build.bat",
            ),
            "wrong token": valid.replace(token, "0" * 16),
            "uppercase token": valid.replace(token, token.upper()),
            "command injection": valid.replace(
                token,
                f"{token} && whoami",
            ),
            "before-tail command injection": valid.replace(
                controller_tail,
                f" && whoami{controller_tail}",
            ),
            "pipe injection": valid.replace(
                "RhythmGameWasmProbe.js",
                "RhythmGameWasmProbe.js | whoami",
                1,
            ),
            "redirection injection": valid.replace(
                "RhythmGameWasmProbe.js",
                "RhythmGameWasmProbe.js > injected.txt",
                1,
            ),
            "alternate command interpreter": valid.replace(
                str(native_cmd),
                r"C:\attacker\cmd.exe",
                1,
            ),
        }
        for label, command in cases.items():
            with self.subTest(label=label):
                with self.assertRaises(AssertionError):
                    verify_build.parse_application_link_arguments(
                        command,
                        repo,
                        build,
                        expected,
                    )
        controller.write_bytes(b"changed controller bytes\r\n")
        with self.assertRaisesRegex(
            AssertionError,
            "does not authenticate controller",
        ):
            verify_build.parse_application_link_arguments(
                valid,
                repo,
                build,
                expected,
            )

    def test_application_link_edge_requires_exact_declared_outputs(
        self,
    ) -> None:
        _, _, build, _, build_ninja, _ = self.application_link_fixture()
        self.assertEqual(
            verify_build.selected_application_link_edge(
                build,
                build_ninja,
            )["rule"],
            "CXX_EXECUTABLE_LINKER__RhythmGameWasmProbe_Release",
        )
        final_output = "runtime/RhythmGameWasmProbe.html"
        final_implicit_output = (
            f"${{cmake_ninja_workdir}}{final_output}"
        )
        absolute_output = (
            build / "RhythmGameWasmProbe.js"
        ).resolve().as_posix()
        escaped_absolute_output = (
            absolute_output
            .replace("$", "$$")
            .replace(" ", "$ ")
            .replace(":", "$:")
        )
        cases = {
            "missing byproduct": build_ninja.replace(
                f" {final_output} |",
                " |",
                1,
            ),
            "missing implicit byproduct alias": build_ninja.replace(
                f" {final_implicit_output}:",
                ":",
                1,
            ),
            "extra output": build_ninja.replace(
                "build RhythmGameWasmProbe.js ",
                "build RhythmGameWasmProbe.js unexpected-output ",
                1,
            ),
            "implicit primary": build_ninja.replace(
                "build RhythmGameWasmProbe.js ",
                "build | RhythmGameWasmProbe.js ",
                1,
            ),
            "duplicate producer": build_ninja + build_ninja,
            "workdir alias producer": (
                build_ninja
                + "\nbuild "
                + "${cmake_ninja_workdir}RhythmGameWasmProbe.js"
                + ": phony\n"
            ),
            "relative alias producer": (
                build_ninja
                + "\nbuild ./RhythmGameWasmProbe.js: phony\n"
            ),
            "implicit alias producer": (
                build_ninja
                + "\nbuild unrelated | RhythmGameWasmProbe.js: phony\n"
            ),
            "absolute alias producer": (
                build_ninja
                + f"\nbuild {escaped_absolute_output}: phony\n"
            ),
            "variable alias producer": (
                build_ninja
                + "\nEMPTY =\n"
                + "build ${EMPTY}RhythmGameWasmProbe.js: phony\n"
            ),
        }
        for label, value in cases.items():
            with self.subTest(label=label):
                with self.assertRaises(AssertionError):
                    verify_build.selected_application_link_edge(build, value)

    def test_final_link_archive_must_be_on_selected_edge(self) -> None:
        build = Path(tempfile.gettempdir()) / "wasm-probe-link-fixture"
        link_outputs = " ".join(
            (
                *verify_build.APPLICATION_LINK_OUTPUTS,
                "|",
                *verify_build.APPLICATION_LINK_IMPLICIT_OUTPUTS,
            )
        )
        valid = (
            f"build {link_outputs}: CXX_EXECUTABLE object.o\n"
            "  LINK_LIBRARIES = libQt6Core.a  "
            "libWasmProbeExceptionBoundary.a\n"
            "  RSP_FILE = CMakeFiles\\RhythmGameWasmProbe.rsp\n"
            "\n"
            "build unrelated: phony libOther.a\n"
        )
        self.assertEqual(
            verify_build.require_final_link_archive(
                build,
                valid,
                (
                    "rule CXX_EXECUTABLE\n"
                    "  rspfile = $RSP_FILE\n"
                    "  rspfile_content = $in $LINK_PATH $LINK_LIBRARIES\n"
                ),
            ),
            "libWasmProbeExceptionBoundary.a",
        )

        misplaced = valid.replace(
            "libWasmProbeExceptionBoundary.a\n"
            "  RSP_FILE",
            "libQt6Gui.a\n"
            "  RSP_FILE",
        ) + "build unrelated2: phony libWasmProbeExceptionBoundary.a\n"
        with self.assertRaisesRegex(
            AssertionError,
            "selected application link edge",
        ):
            verify_build.require_final_link_archive(
                build,
                misplaced,
                (
                    "rule CXX_EXECUTABLE\n"
                    "  rspfile = $RSP_FILE\n"
                    "  rspfile_content = $in $LINK_PATH $LINK_LIBRARIES\n"
                ),
            )

    def test_dependency_link_indirection_is_rejected(self) -> None:
        hostile = (
            "@nested.rsp",
            "-Wl,-L,T:/outside/lib",
            "-Wl,-l,evil",
            "-Xlinker",
            "-Loutside",
            "-Toutside.ld",
            "--script=outside.ld",
            "--sysroot=T:/outside",
            "-BT:/outside/bin",
            "-fuse-ld=T:/outside/wasm-ld.exe",
            "--ld-path=T:/outside/wasm-ld.exe",
            "--config",
            "--config=T:/outside/clang.cfg",
            "--config-user-dir",
            "--config-user-dir=T:/outside/config",
            "--config-system-dir",
            "--config-system-dir=T:/outside/config",
            "-vfsoverlay",
            "-vfsoverlay=T:/outside/overlay.yaml",
            "-ivfsoverlay",
            "-ivfsoverlayT:/outside/overlay.yaml",
        )
        for argument in hostile:
            with self.subTest(argument=argument):
                with self.assertRaisesRegex(
                    AssertionError,
                    "dependency-link indirection",
                ):
                    verify_build.require_no_dependency_link_indirection(
                        [argument]
                    )
        verify_build.require_no_dependency_link_indirection(
            [
                "CMakeFiles/probe.cpp.o",
                r"T:\installed\lib\libQt6Core.a",
                "-lembind",
                "-lwebsocket.js",
                "-lopenal",
            ]
        )

    def test_backdated_linked_qt_archive_keeps_ninja_noop_but_fails_binding(
        self,
    ) -> None:
        repo_source = Path(verify_build.__file__).resolve().parents[3]
        ninja = (
            repo_source
            / ".toolchains"
            / "ninja-1.13.2-win"
            / "ninja.exe"
        )
        if not ninja.is_file():
            self.skipTest("requires the pinned Ninja executable")

        with tempfile.TemporaryDirectory(
            prefix="rg-linked-archive-tamper-"
        ) as directory:
            repo = Path(directory) / "repo"
            probe = repo / "tools" / "wasm-probe"
            scripts = probe / "scripts"
            build = probe / "build" / "wasm-release"
            generated = build / "generated"
            target = (
                repo
                / ".wasm-vcpkg"
                / "installed"
                / verify_build.TARGET_TRIPLET
            )
            scripts.mkdir(parents=True)
            generated.mkdir(parents=True)
            target.mkdir(parents=True)
            contract = probe / "dependency-archive-contract.json"
            generator = scripts / "generate_dependency_digest.py"
            contract.write_bytes(
                (
                    repo_source
                    / "tools"
                    / "wasm-probe"
                    / "dependency-archive-contract.json"
                ).read_bytes()
            )
            generator.write_bytes(
                (
                    repo_source
                    / "tools"
                    / "wasm-probe"
                    / "scripts"
                    / "generate_dependency_digest.py"
                ).read_bytes()
            )
            archive = target / "lib" / "libQt6Core.a"
            archive.parent.mkdir()
            archive.write_bytes(b"!<arch>\nauthenticated Qt archive bytes\n")
            unused = target / "lib" / "libUnusedDependency.a"
            unused.write_bytes(
                b"!<arch>\nauthenticated unused archive bytes\n"
            )
            superset = verify_build._static_link_input_superset(target)
            manifest = generated / "dependency-archive-digest.json"
            manifest.write_text(
                json.dumps(superset, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            marker = (
                "RHYTHMGAME_WASM_DEPENDENCY_ARCHIVE_SUPERSET_SHA256="
                + superset["aggregateSha256"]
            )
            source = generated / "ProbeDependencyDigest.cpp"
            source.write_text(marker, encoding="ascii")

            def uleb(value: int) -> bytes:
                encoded = bytearray()
                while True:
                    byte = value & 0x7F
                    value >>= 7
                    if value:
                        byte |= 0x80
                    encoded.append(byte)
                    if not value:
                        return bytes(encoded)

            artifact = build / "RhythmGameWasmProbe.wasm"
            marker_bytes = marker.encode("ascii")
            artifact.write_bytes(
                b"\x00asm\x01\x00\x00\x00"
                + b"\x0b"
                + uleb(len(marker_bytes))
                + marker_bytes
            )
            linked_relative = (
                "../../../../.wasm-vcpkg/installed/"
                f"{verify_build.TARGET_TRIPLET}/lib/libQt6Core.a"
            )
            (build / "build.ninja").write_text(
                (
                    "rule copy\n"
                    "  command = cmd.exe /d /c echo seed>$out\n"
                    "build RhythmGameWasmProbe.wasm: copy "
                    f"{linked_relative}\n"
                    "build RhythmGameWasmProbe: phony "
                    "RhythmGameWasmProbe.wasm\n"
                ),
                encoding="utf-8",
            )
            verify_build.run_text(
                ninja,
                "-C",
                build,
                "RhythmGameWasmProbe",
            )
            artifact.write_bytes(
                b"\x00asm\x01\x00\x00\x00"
                + b"\x0b"
                + uleb(len(marker_bytes))
                + marker_bytes
            )
            archive_time = 1_700_000_000_000_000_000
            artifact_time = archive_time + 1_000_000_000
            os.utime(archive, ns=(archive_time, archive_time))
            os.utime(artifact, ns=(artifact_time, artifact_time))
            verify_build.verify_ninja_noop(ninja, build)

            original = archive.read_bytes()
            archive.write_bytes(
                original[:-1]
                + bytes((original[-1] ^ 0x01,))
            )
            os.utime(archive, ns=(archive_time, archive_time))
            verify_build.verify_ninja_noop(ninja, build)
            with self.assertRaisesRegex(
                AssertionError,
                "manifest does not match current bytes",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        "object.o",
                        "libWasmProbeExceptionBoundary.a",
                        str(archive.resolve()),
                        "-lembind",
                    ],
                    expected_static_identity={
                        key: superset[key]
                        for key in (
                            verify_build
                            .TARGET_STATIC_LINK_INPUT_IDENTITY_FIELDS
                        )
                    },
                )

    def test_selected_link_substitution_breaks_embedded_build_id(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-selected-link-build-id-"
        ) as directory:
            repo = Path(directory)
            build = repo / "build"
            build.mkdir()
            object_file = build / "probe.o"
            object_file.write_bytes(b"\0asm\1\0\0\0fixture object")
            first = build / "first.a"
            second = build / "second.a"
            first.write_bytes(b"!<arch>\n")
            second.write_bytes(b"!<arch>\n")
            arguments = [
                str(object_file),
                str(first),
                "-lembind",
                "-o",
                "RhythmGameWasmProbe.js",
            ]
            qualification = {
                "algorithm": verify_build.QUALIFICATION_CLOSURE_ALGORITHM,
                "fileCount": 1,
                "totalBytes": 1,
                "inventorySha256": "1" * 64,
                "aggregateSha256": "2" * 64,
            }
            sidecars = {
                verify_build.path_key(object_file): {
                    "payload": {
                        "output": "build-output/probe.o",
                        "closureSha256": "3" * 64,
                        "outputSha256": verify_build.sha256(object_file),
                        "qualification": qualification,
                    },
                    "sidecarSha256": "4" * 64,
                }
            }
            identity = verify_build.selected_application_link_identity(
                repo,
                build,
                arguments,
                qualification,
                sidecars,
            )

            def uleb(value: int) -> bytes:
                encoded = bytearray()
                while True:
                    byte = value & 0x7F
                    value >>= 7
                    if value:
                        byte |= 0x80
                    encoded.append(byte)
                    if not value:
                        return bytes(encoded)

            name = b"build_id"
            custom_payload = (
                uleb(len(name))
                + name
                + bytes((32,))
                + bytes.fromhex(identity["sha256"])
            )
            artifact = build / "RhythmGameWasmProbe.wasm"
            artifact.write_bytes(
                b"\0asm\1\0\0\0"
                + bytes((0,))
                + uleb(len(custom_payload))
                + custom_payload
            )
            verified = verify_build.verify_selected_link_artifact_binding(
                artifact,
                identity,
            )
            self.assertTrue(verified["artifactBound"])

            substituted = verify_build.selected_application_link_identity(
                repo,
                build,
                [
                    str(object_file),
                    str(second),
                    "-lembind",
                    "-o",
                    "RhythmGameWasmProbe.js",
                ],
                qualification,
                sidecars,
            )
            self.assertNotEqual(identity["sha256"], substituted["sha256"])
            with self.assertRaisesRegex(
                AssertionError,
                "build_id does not bind",
            ):
                verify_build.verify_selected_link_artifact_binding(
                    artifact,
                    substituted,
                )

    def test_renamed_archive_magic_cannot_escape_authenticated_superset(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-renamed-archive-"
        ) as directory:
            root = Path(directory)
            repo, build, boundary, installed_archive, static_identity = (
                self.dependency_binding_fixture(root)
            )
            disguised_archive = installed_archive.with_suffix(".blob")
            disguised_archive.write_bytes(b"!<arch>\ndisguised archive\n")

            with self.assertRaisesRegex(
                AssertionError,
                "renamed application-link static input",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(boundary),
                        str(disguised_archive),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_installed_wasm_object_is_member_and_hash_authenticated(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-installed-wasm-object-"
        ) as directory:
            root = Path(directory)
            repo, build, boundary, installed_archive, static_identity = (
                self.dependency_binding_fixture(root)
            )
            installed_object = (
                installed_archive.parents[1]
                / "Qt6"
                / "plugins"
                / "objects-Release"
                / "Plugin_init.cpp.o"
            )
            evidence = verify_build.verify_dependency_archive_binding(
                repo,
                build,
                [
                    str(build / "object.o"),
                    str(boundary),
                    str(installed_object),
                    str(installed_archive),
                ],
                expected_static_identity=static_identity,
            )
            self.assertEqual(
                {
                    entry["kind"]
                    for entry in evidence["linkedClosure"]["archives"]
                },
                {"archive", "wasm-object"},
            )

            original = installed_object.read_bytes()
            installed_object.write_bytes(original[:-1] + b"X")
            with self.assertRaisesRegex(
                AssertionError,
                "manifest does not match current bytes",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(build / "object.o"),
                        str(boundary),
                        str(installed_object),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_target_static_source_lock_rejects_forged_owner(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-forged-static-owner-"
        ) as directory:
            root = Path(directory)
            (
                repo,
                build,
                boundary,
                installed_archive,
                static_identity,
            ) = self.dependency_binding_fixture(root)
            installed = repo / ".wasm-vcpkg" / "installed"
            target = installed / verify_build.TARGET_TRIPLET
            installed_object = (
                target
                / "Qt6"
                / "plugins"
                / "objects-Release"
                / "Plugin_init.cpp.o"
            )
            stale = target / "lib" / "libQt6Multimedia-stale.a"
            stale.write_bytes(b"!<arch>\nstale multimedia archive\n")
            info = installed / "vcpkg" / "info"
            info.mkdir(parents=True)
            (info / f"fixture_{verify_build.TARGET_TRIPLET}.list").write_text(
                (
                    f"{verify_build.TARGET_TRIPLET}/"
                    "lib/libQt6Core.a\n"
                    f"{verify_build.TARGET_TRIPLET}/"
                    "Qt6/plugins/objects-Release/Plugin_init.cpp.o\n"
                ),
                encoding="utf-8",
            )
            (info / f"evil_{verify_build.TARGET_TRIPLET}.list").write_text(
                (
                    f"{verify_build.TARGET_TRIPLET}/"
                    "lib/libQt6Multimedia-stale.a\n"
                ),
                encoding="utf-8",
            )
            self.assertEqual(
                verify_build.verify_target_static_package_ownership(
                    installed
                ),
                3,
            )

            forged_superset = verify_build._static_link_input_superset(
                target
            )
            (
                build
                / "generated"
                / "dependency-archive-digest.json"
            ).write_text(
                json.dumps(forged_superset),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "differs from the source lock",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(build / "object.o"),
                        str(boundary),
                        str(stale),
                        str(installed_object),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_renamed_wasm_object_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-renamed-wasm-object-"
        ) as directory:
            root = Path(directory)
            repo, build, boundary, installed_archive, static_identity = (
                self.dependency_binding_fixture(root)
            )
            disguised_object = installed_archive.parents[1] / "object.blob"
            disguised_object.write_bytes(
                b"\x00asm\x01\x00\x00\x00disguised object"
            )

            with self.assertRaisesRegex(
                AssertionError,
                "renamed application-link static input",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(build / "object.o"),
                        str(boundary),
                        str(disguised_object),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_non_wasm_installed_object_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-non-wasm-object-"
        ) as directory:
            root = Path(directory)
            repo, build, boundary, installed_archive, static_identity = (
                self.dependency_binding_fixture(root)
            )
            invalid_object = installed_archive.parents[1] / "invalid.o"
            invalid_object.write_bytes(b"not a WebAssembly object")

            with self.assertRaisesRegex(
                AssertionError,
                "Wasm object magic/version",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(build / "object.o"),
                        str(boundary),
                        str(invalid_object),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_external_positional_file_is_rejected_from_effective_link_argv(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-external-link-input-"
        ) as directory:
            root = Path(directory)
            repo, build, boundary, installed_archive, static_identity = (
                self.dependency_binding_fixture(root)
            )
            hostile = root / "outside-input.o"
            hostile.write_bytes(
                b"\x00asm\x01\x00\x00\x00external object bytes"
            )

            with self.assertRaisesRegex(
                AssertionError,
                "external application-link static input",
            ):
                verify_build.verify_dependency_archive_binding(
                    repo,
                    build,
                    [
                        str(hostile),
                        str(boundary),
                        str(installed_archive),
                    ],
                    expected_static_identity=static_identity,
                )

    def test_response_file_conflicting_jspi_is_rejected(self) -> None:
        fixture = self.application_link_fixture("-sJSPI=0")
        with self.assertRaisesRegex(AssertionError, "JSPI"):
            verify_build.verify_application_link_argument_stream(*fixture)

    def test_response_file_split_asyncify_is_rejected(self) -> None:
        fixture = self.application_link_fixture("-s", "ASYNCIFY=1")
        with self.assertRaisesRegex(AssertionError, "Asyncify"):
            verify_build.verify_application_link_argument_stream(*fixture)

    def test_response_file_negative_wasm_exceptions_is_rejected(self) -> None:
        fixture = self.application_link_fixture("-fno-wasm-exceptions")
        with self.assertRaisesRegex(
            AssertionError,
            "-fno-wasm-exceptions",
        ):
            verify_build.verify_application_link_argument_stream(*fixture)

    def test_autogen_predefs_must_be_unique_target_moc_predefs(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "qtdeclarative" / "wasm32-emscripten-rg-rel"
            generated = target / "module_autogen" / "moc_predefs.h"
            generated.parent.mkdir(parents=True)
            generated.write_text("#define __wasm__ 1\n", encoding="utf-8")

            self.assertEqual(
                verify_build.validate_autogen_predefs_paths(
                    target,
                    [str(generated)],
                ),
                [generated.resolve()],
            )
            with self.assertRaisesRegex(AssertionError, "referenced twice"):
                verify_build.validate_autogen_predefs_paths(
                    target,
                    [str(generated), str(generated)],
                )

            wrong_name = generated.with_name("predefs.h")
            wrong_name.write_text("#define __wasm__ 1\n", encoding="utf-8")
            with self.assertRaisesRegex(AssertionError, "moc_predefs.h"):
                verify_build.validate_autogen_predefs_paths(
                    target,
                    [str(wrong_name)],
                )

            outside = root / "moc_predefs.h"
            outside.write_text("#define __wasm__ 1\n", encoding="utf-8")
            with self.assertRaisesRegex(AssertionError, "target build"):
                verify_build.validate_autogen_predefs_paths(
                    target,
                    [str(outside)],
                )

    def test_host_tool_version_comparison_is_exact(self) -> None:
        self.assertEqual(
            verify_build.require_host_tool_version(
                "moc.exe",
                "moc 6.11.1",
            ),
            "moc 6.11.1",
        )
        with self.assertRaisesRegex(AssertionError, "moc.exe"):
            verify_build.require_host_tool_version(
                "moc.exe",
                "moc 6.11.10",
            )

    def test_swapped_host_qtdeclarative_cache_is_rejected(self) -> None:
        with self.assertRaisesRegex(
            AssertionError,
            "QtDeclarative host.*VCPKG_TARGET_TRIPLET",
        ):
            verify_build.require_exact_cache_values(
                {"VCPKG_TARGET_TRIPLET": verify_build.TARGET_TRIPLET},
                {"VCPKG_TARGET_TRIPLET": verify_build.HOST_TRIPLET},
                "QtDeclarative host",
            )

    def test_exact_deployment_set_rejects_orphan_web_outputs(self) -> None:
        for orphan in ("orphan.data", "orphan.wasm", "orphan.js"):
            with self.subTest(orphan=orphan):
                with tempfile.TemporaryDirectory() as directory:
                    build = Path(directory)
                    for name in verify_build.DEPLOYMENT_ARTIFACTS:
                        (build / name).write_text("payload", encoding="utf-8")
                    (build / "generated.cpp").write_text(
                        "not deployable",
                        encoding="utf-8",
                    )
                    intermediate = build / "CMakeFiles" / "generated.js"
                    intermediate.parent.mkdir()
                    intermediate.write_text("ignored", encoding="utf-8")
                    (build / orphan).write_text("orphan", encoding="utf-8")

                    with self.assertRaisesRegex(
                        AssertionError,
                        "deployment",
                    ):
                        verify_build.require_exact_deployment_set(build)

    def test_shader_resource_contract_rejects_extra_alias(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            qrc = build / ".qt" / "rcc" / "wasm_probe_shaders.qrc"
            generated = (
                build / ".qt" / "rcc" / "qrc_wasm_probe_shaders.cpp"
            )
            expected_qsb = build / ".qsb" / "pulse.frag.qsb"
            extra_qsb = build / ".qsb" / "extra.frag.qsb"
            qrc.parent.mkdir(parents=True)
            expected_qsb.parent.mkdir(parents=True)
            expected_qsb.write_bytes(b"qsb")
            extra_qsb.write_bytes(b"extra")
            qrc.write_text(
                (
                    "<RCC><qresource "
                    'prefix="/qt/qml/RhythmGame/WasmProbe/shaders">'
                    f'<file alias="pulse.frag.qsb">{expected_qsb}</file>'
                    f'<file alias="extra.frag.qsb">{extra_qsb}</file>'
                    "</qresource></RCC>"
                ),
                encoding="utf-8",
            )
            generated.write_text(
                (
                    "// :\n"
                    "// :/qt\n"
                    "// :/qt/qml\n"
                    "// :/qt/qml/RhythmGame\n"
                    "// :/qt/qml/RhythmGame/WasmProbe\n"
                    "// :/qt/qml/RhythmGame/WasmProbe/shaders\n"
                    "// :/qt/qml/RhythmGame/WasmProbe/shaders/"
                    "pulse.frag.qsb\n"
                    "// :/qt/qml/RhythmGame/WasmProbe/shaders/"
                    "extra.frag.qsb\n"
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                AssertionError,
                "shader resource",
            ):
                verify_build.verify_shader_resource_contract(build)

    def test_shader_resource_contract_rejects_unlocked_timestamp(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            qrc = build / ".qt" / "rcc" / "wasm_probe_shaders.qrc"
            generated = (
                build / ".qt" / "rcc" / "qrc_wasm_probe_shaders.cpp"
            )
            expected_qsb = build / ".qsb" / "pulse.frag.qsb"
            qrc.parent.mkdir(parents=True)
            expected_qsb.parent.mkdir(parents=True)
            expected_qsb.write_bytes(b"qsb")
            qrc.write_text(
                (
                    "<RCC><qresource "
                    'prefix="/qt/qml/RhythmGame/WasmProbe/shaders">'
                    f'<file alias="pulse.frag.qsb">{expected_qsb}</file>'
                    "</qresource></RCC>"
                ),
                encoding="utf-8",
            )
            wrong_timestamp = (
                verify_build.EXPECTED_SOURCE_DATE_EPOCH + 1
            ) * 1000
            resource_struct = (
                bytes(len(verify_build.SHADER_RESOURCE_TREE) * 22 - 8)
                + wrong_timestamp.to_bytes(8, "big")
            )
            generated.write_text(
                (
                    "\n".join(
                        f"// {path}"
                        for path in verify_build.SHADER_RESOURCE_TREE
                    )
                    + "\nstatic const unsigned char "
                    "qt_resource_struct[] = {\n"
                    + ",".join(
                        f"0x{value:x}" for value in resource_struct
                    )
                    + ",\n};\n"
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                AssertionError,
                "timestamp drifted",
            ):
                verify_build.verify_shader_resource_contract(build)

    def test_qtdeclarative_installed_port_version_is_exact(self) -> None:
        with self.assertRaisesRegex(
            AssertionError,
            "qtdeclarative.*Port-Version",
        ):
            verify_build.require_port_version(
                {"Version": verify_build.EXPECTED_QT},
                "1",
                "qtdeclarative",
            )

    def test_qtmultimedia_installed_port_version_is_exact(self) -> None:
        with self.assertRaisesRegex(
            AssertionError,
            "qtmultimedia.*Port-Version",
        ):
            verify_build.require_port_version(
                {"Version": verify_build.EXPECTED_QT},
                "2",
                "qtmultimedia",
            )

    def test_qtmultimedia_reproducibility_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            empty_repo = Path(directory)
            with self.assertRaisesRegex(
                AssertionError,
                "reproducibility manifests are incomplete",
            ):
                verify_build.verify_qtmultimedia_reproducibility(
                    empty_repo,
                    primary_abi="a" * 64,
                    binary_contract={"abi": "a" * 64},
                    primary_compile_identity={},
                )

        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            fixture = self.qtmultimedia_reproducibility_fixture(repo)
            binary_identity = fixture["binaryIdentity"]
            compile_identity = fixture["secondaryCompileIdentity"]

            def verify(
                *,
                secondary_binary_identity: object = binary_identity,
                secondary_compile_identity: object = compile_identity,
            ) -> dict[str, object]:
                with (
                    mock.patch.object(
                        verify_build,
                        "verify_qtmultimedia_binary_package",
                        return_value=secondary_binary_identity,
                    ),
                    mock.patch.object(
                        verify_build,
                        "verify_qtmultimedia_compile_path_maps",
                        return_value=secondary_compile_identity,
                    ),
                ):
                    return (
                        verify_build.verify_qtmultimedia_reproducibility(
                            repo,
                            primary_abi=fixture["abi"],
                            binary_contract=fixture["binaryContract"],
                            primary_compile_identity=fixture[
                                "primaryCompileIdentity"
                            ],
                        )
                    )

            evidence = verify()
            self.assertTrue(
                evidence["allCompileCommandsCanonicalIdentical"]
            )
            self.assertTrue(evidence["allStaticMembersByteIdentical"])

            secondary_probe_cache = fixture["secondaryProbeCache"]
            original_probe_cache = secondary_probe_cache.read_text("utf-8")
            control_plane_mutations = (
                (
                    "toolchain",
                    (
                        repo
                        / ".toolchains"
                        / (
                            "vcpkg-"
                            f"{verify_build.EXPECTED_VCPKG_COMMIT[:8]}"
                        )
                        / "scripts"
                        / "buildsystems"
                        / "vcpkg.cmake"
                    ).as_posix(),
                    (repo / "wrong-vcpkg.cmake").as_posix(),
                    "CMAKE_TOOLCHAIN_FILE",
                ),
                (
                    "overlay",
                    (repo / "vcpkgOverlayPorts").as_posix(),
                    (repo / "wrong-overlay").as_posix(),
                    "overlay port 0",
                ),
                (
                    "install-prefix",
                    (
                        repo
                        / ".toolchains"
                        / (
                            "emscripten-cache-"
                            f"{verify_build.EXPECTED_EMSCRIPTEN}"
                        )
                        / "sysroot"
                    ).as_posix(),
                    (repo / "wrong-sysroot").as_posix(),
                    "CMAKE_INSTALL_PREFIX",
                ),
                (
                    "binary-source",
                    "--binarysource=clear",
                    "--binarysource=default",
                    "binary sources are not cleared",
                ),
            )
            for (
                label,
                original_value,
                drifted_value,
                expected_message,
            ) in control_plane_mutations:
                with self.subTest(control_plane=label):
                    self.assertIn(original_value, original_probe_cache)
                    secondary_probe_cache.write_text(
                        original_probe_cache.replace(
                            original_value,
                            drifted_value,
                            1,
                        ),
                        encoding="utf-8",
                    )
                    with self.assertRaisesRegex(
                        AssertionError,
                        expected_message,
                    ):
                        verify()
            secondary_probe_cache.write_text(
                original_probe_cache,
                encoding="utf-8",
            )

            secondary_cache = (
                fixture["secondaryBuild"] / "CMakeCache.txt"
            )
            original_secondary_cache = secondary_cache.read_text("utf-8")
            secondary_cache.write_text(
                (
                    "CMAKE_HOME_DIRECTORY:INTERNAL="
                    f"{fixture['primarySource'].as_posix()}\n"
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "roots are not distinct and unequal-length",
            ):
                verify()

            equal_length_source = (
                fixture["primarySource"].parent / "secondy.clean"
            )
            equal_length_source.mkdir()
            self.assertEqual(
                len(str(equal_length_source)),
                len(str(fixture["primarySource"])),
            )
            secondary_cache.write_text(
                (
                    "CMAKE_HOME_DIRECTORY:INTERNAL="
                    f"{equal_length_source.as_posix()}\n"
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "roots are not distinct and unequal-length",
            ):
                verify()
            secondary_cache.write_text(
                original_secondary_cache,
                encoding="utf-8",
            )

            abi_info = fixture["abiInfo"]
            original_abi_info = abi_info.read_bytes()
            abi_info.write_bytes(original_abi_info + b"drift")
            with self.assertRaisesRegex(
                AssertionError,
                "secondary ABI differs",
            ):
                verify()
            abi_info.write_bytes(original_abi_info)

            compile_drift = copy.deepcopy(compile_identity)
            compile_drift["aggregateSha256"] = "c" * 64
            with self.assertRaisesRegex(
                AssertionError,
                "canonical compile-command identity differs",
            ):
                verify(secondary_compile_identity=compile_drift)

            static_drift = copy.deepcopy(binary_identity)
            static_drift["BinaryPayloadAggregateSha256"] = "d" * 64
            with self.assertRaisesRegex(
                AssertionError,
                "static package members differ",
            ):
                verify(secondary_binary_identity=static_drift)

            secondary_manifest = (
                fixture["secondaryRoot"] / "manifest" / "vcpkg.json"
            )
            manifest_payload = json.loads(
                secondary_manifest.read_text("utf-8")
            )
            manifest_payload["dependencies"].append("qtwebsockets")
            secondary_manifest.write_text(
                json.dumps(manifest_payload, indent=2) + "\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "differ only by removing builtin-baseline",
            ):
                verify()

    def test_qtmultimedia_reproducibility_is_mandatory_and_lock_bound(
        self,
    ) -> None:
        self.assertIn(
            "verify_qtmultimedia_reproducibility",
            verify_build.verify_qt_installation.__code__.co_names,
        )
        evidence = copy.deepcopy(
            verify_build.EXPECTED_QTMULTIMEDIA_REPRODUCIBILITY_IDENTITY
        )
        verify_build.validate_qtmultimedia_reproducibility_evidence(
            evidence
        )

        omitted = copy.deepcopy(evidence)
        del omitted["secondaryManifestSha256"]
        with self.assertRaisesRegex(
            AssertionError,
            "keys must be exactly",
        ):
            verify_build.validate_qtmultimedia_reproducibility_evidence(
                omitted
            )

        falsified = copy.deepcopy(evidence)
        falsified["staticAggregateSha256"] = "f" * 64
        with self.assertRaisesRegex(
            AssertionError,
            "evidence is not lock-bound",
        ):
            verify_build.validate_qtmultimedia_reproducibility_evidence(
                falsified
            )

    def test_qtmultimedia_compile_commands_require_both_root_maps(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            build = (
                repo
                / ".wb"
                / "qtmultimedia"
                / f"{verify_build.TARGET_TRIPLET}-rel"
            )
            source = (
                repo
                / ".wb"
                / "qtmultimedia"
                / "src"
                / "here-src-test.clean"
            )
            installed = repo / ".wasm-vcpkg" / "installed"
            install_prefix = (
                repo
                / ".wasm-vcpkg"
                / "packages"
                / f"qtmultimedia_{verify_build.TARGET_TRIPLET}"
            )
            packages_root = install_prefix.parent
            port_cmake = (
                repo
                / ".wasm-vcpkg"
                / "downloads"
                / "tools"
                / verify_build.EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY[
                    "installationDirectory"
                ]
                / verify_build.EXPECTED_VCPKG_PORT_CMAKE_LOCK_ENTRY[
                    "executable"
                ]
            )
            vcpkg_toolchain = (
                repo
                / ".toolchains"
                / f"vcpkg-{verify_build.EXPECTED_VCPKG_COMMIT[:8]}"
                / "scripts"
                / "buildsystems"
                / "vcpkg.cmake"
            )
            chainload_toolchain = (
                repo / "cmake" / "toolchains" / "vcpkg-emscripten.cmake"
            )
            compiler = (
                repo
                / ".toolchains"
                / f"emsdk-{verify_build.EXPECTED_EMSCRIPTEN}"
                / "upstream"
                / "emscripten"
                / "em++.bat"
            )
            ninja = (
                repo
                / ".toolchains"
                / verify_build.EXPECTED_NINJA_LOCK_ENTRY["directory"]
                / "ninja.exe"
            )
            build.mkdir(parents=True)
            source.mkdir(parents=True)
            installed.mkdir(parents=True)
            install_prefix.mkdir(parents=True)
            compiler.parent.mkdir(parents=True)
            compiler.write_bytes(b"pinned compiler fixture")
            ninja.parent.mkdir(parents=True)
            ninja.write_bytes(b"pinned Ninja fixture")
            source_file = source / "probe.cpp"
            source_file.write_text("// compile fixture\n", encoding="utf-8")
            output = build / "probe.cpp.o"
            output_bytes = b"\0asm\1\0\0\0compiled fixture"
            output.write_bytes(output_bytes)
            second_source_file = source / "nested" / "probe.cpp"
            second_source_file.parent.mkdir()
            second_source_file.write_text(
                "// second compile fixture\n",
                encoding="utf-8",
            )
            second_output = build / "nested" / "probe.cpp.o"
            second_output.parent.mkdir()
            second_output_bytes = (
                b"\0asm\1\0\0\0second compiled fixture"
            )
            second_output.write_bytes(second_output_bytes)
            pch_source_file = source / "probe.hpp"
            pch_source_file.write_text(
                "// PCH compile fixture\n",
                encoding="utf-8",
            )
            pch_output = build / "probe.hpp.pch"
            pch_output.write_bytes(b"CPCHfixture-root-specific-a")
            installed_output = (
                installed
                / verify_build.TARGET_TRIPLET
                / "lib"
                / "probe.cpp.o"
            )
            installed_output.parent.mkdir(parents=True)
            installed_output.write_bytes(output_bytes)
            second_installed_output = (
                installed
                / verify_build.TARGET_TRIPLET
                / "lib"
                / "nested"
                / "probe.cpp.o"
            )
            second_installed_output.parent.mkdir(parents=True)
            second_installed_output.write_bytes(second_output_bytes)
            package_list = (
                installed
                / "vcpkg"
                / "info"
                / (
                    f"qtmultimedia_{verify_build.EXPECTED_QT}_"
                    f"{verify_build.TARGET_TRIPLET}.list"
                )
            )
            package_list.parent.mkdir(parents=True)
            package_list.write_text(
                (
                    f"{verify_build.TARGET_TRIPLET}/"
                    "lib/probe.cpp.o\n"
                    f"{verify_build.TARGET_TRIPLET}/"
                    "lib/nested/probe.cpp.o\n"
                ),
                encoding="utf-8",
            )
            cache_file = build / "CMakeCache.txt"
            cache_file.write_text(
                (
                    f"CMAKE_COMMAND:INTERNAL={port_cmake.as_posix()}\n"
                    "CMAKE_GENERATOR:INTERNAL=Ninja\n"
                    f"CMAKE_HOME_DIRECTORY:INTERNAL={source.as_posix()}\n"
                    f"VCPKG_INSTALLED_DIR:PATH={installed.as_posix()}\n"
                    "CMAKE_TOOLCHAIN_FILE:FILEPATH="
                    f"{vcpkg_toolchain.as_posix()}\n"
                    "VCPKG_CHAINLOAD_TOOLCHAIN_FILE:FILEPATH="
                    f"{chainload_toolchain.as_posix()}\n"
                    "VCPKG_TARGET_TRIPLET:STRING="
                    f"{verify_build.TARGET_TRIPLET}\n"
                    "CMAKE_INSTALL_PREFIX:PATH="
                    f"{install_prefix.as_posix()}\n"
                ),
                encoding="utf-8",
            )
            original_cache = cache_file.read_text("utf-8")
            arguments = [
                str(compiler),
                "-pthread",
                "-fwasm-exceptions",
                "-sSUPPORT_LONGJMP=wasm",
                (
                    f"-ffile-prefix-map={source.as_posix()}="
                    f"{verify_build.QTMULTIMEDIA_PATH_MAP_TARGETS['source']}"
                ),
                (
                    f"-ffile-prefix-map={build.as_posix()}="
                    f"{verify_build.QTMULTIMEDIA_PATH_MAP_TARGETS['build']}"
                ),
                (
                    f"-fmacro-prefix-map={source.as_posix()}="
                    f"{verify_build.QTMULTIMEDIA_PATH_MAP_TARGETS['source']}"
                ),
                (
                    f"-fmacro-prefix-map={build.as_posix()}="
                    f"{verify_build.QTMULTIMEDIA_PATH_MAP_TARGETS['build']}"
                ),
                "-o",
                output.as_posix(),
                "-c",
                source_file.as_posix(),
            ]
            database = [
                {
                    "arguments": arguments,
                    "directory": build.as_posix(),
                    "file": source_file.as_posix(),
                    "output": output.as_posix(),
                },
            ]
            second_arguments = list(arguments)
            second_arguments[
                second_arguments.index(output.as_posix())
            ] = second_output.as_posix()
            second_arguments[
                second_arguments.index(source_file.as_posix())
            ] = second_source_file.as_posix()
            database.append(
                {
                    "arguments": second_arguments,
                    "directory": build.as_posix(),
                    "file": second_source_file.as_posix(),
                    "output": second_output.as_posix(),
                }
            )
            pch_arguments = list(arguments)
            pch_arguments[pch_arguments.index(output.as_posix())] = (
                pch_output.as_posix()
            )
            pch_arguments[pch_arguments.index(source_file.as_posix())] = (
                pch_source_file.as_posix()
            )
            database.append(
                {
                    "arguments": pch_arguments,
                    "directory": build.as_posix(),
                    "file": pch_source_file.as_posix(),
                    "output": pch_output.as_posix(),
                }
            )
            expanded_arguments = list(arguments)
            output_index = expanded_arguments.index("-o")
            expanded_arguments[output_index:output_index] = [
                "-MD",
                "-MT",
                "probe.cpp.o",
                "-MF",
                "probe.cpp.o.d",
            ]
            expanded = [
                {
                    "arguments": expanded_arguments,
                    "directory": build.as_posix(),
                    "file": source_file.as_posix(),
                    "output": "probe.cpp.o",
                },
            ]
            second_expanded_arguments = list(second_arguments)
            second_output_index = second_expanded_arguments.index("-o")
            second_expanded_arguments[
                second_output_index:second_output_index
            ] = [
                "-MD",
                "-MT",
                "nested/probe.cpp.o",
                "-MF",
                "nested/probe.cpp.o.d",
            ]
            expanded.append(
                {
                    "arguments": second_expanded_arguments,
                    "directory": build.as_posix(),
                    "file": second_source_file.as_posix(),
                    "output": "nested/probe.cpp.o",
                }
            )
            pch_expanded_arguments = list(pch_arguments)
            pch_output_index = pch_expanded_arguments.index("-o")
            pch_expanded_arguments[
                pch_output_index:pch_output_index
            ] = [
                "-MD",
                "-MT",
                "probe.hpp.pch",
                "-MF",
                "probe.hpp.pch.d",
            ]
            expanded.append(
                {
                    "arguments": pch_expanded_arguments,
                    "directory": build.as_posix(),
                    "file": pch_source_file.as_posix(),
                    "output": "probe.hpp.pch",
                }
            )
            compile_database = build / "compile_commands.json"
            compile_database.write_text(
                json.dumps(database),
                encoding="utf-8",
            )
            with mock.patch.object(
                verify_build,
                "run_text",
                return_value=json.dumps(expanded),
            ):
                identity = (
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )
                )
            self.assertEqual(identity["commandCount"], 3)
            self.assertEqual(identity["objectOutputCount"], 2)
            self.assertEqual(identity["perEdgeObjectDigestCount"], 2)
            self.assertEqual(identity["pchOutputCount"], 1)
            self.assertEqual(
                identity["pchByteIdentity"],
                "excluded-noninstallable-root-dependent-v1",
            )
            self.assertTrue(identity["ninjaParity"])

            pch_output.write_bytes(b"CPCHfixture-root-specific-b")
            with mock.patch.object(
                verify_build,
                "run_text",
                return_value=json.dumps(expanded),
            ):
                pch_byte_drift_identity = (
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )
                )
            self.assertEqual(
                pch_byte_drift_identity["aggregateSha256"],
                identity["aggregateSha256"],
            )
            pch_output.write_bytes(b"CPCHfixture-root-specific-a")

            wrong_install_prefix = packages_root / "wrong-qtmultimedia"
            wrong_install_prefix.mkdir()
            cache_file.write_text(
                original_cache.replace(
                    install_prefix.as_posix(),
                    wrong_install_prefix.as_posix(),
                    1,
                ),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "CMAKE_INSTALL_PREFIX",
            ):
                with mock.patch.object(
                    verify_build,
                    "run_text",
                    return_value=json.dumps(expanded),
                ):
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )
            cache_file.write_text(original_cache, encoding="utf-8")

            port_control_plane_mutations = (
                (
                    "cmake",
                    port_cmake.as_posix(),
                    (repo / "wrong-port-cmake.exe").as_posix(),
                    "CMAKE_COMMAND",
                ),
                (
                    "toolchain",
                    vcpkg_toolchain.as_posix(),
                    (repo / "wrong-port-toolchain.cmake").as_posix(),
                    "CMAKE_TOOLCHAIN_FILE",
                ),
                (
                    "chainload",
                    chainload_toolchain.as_posix(),
                    (repo / "wrong-chainload.cmake").as_posix(),
                    "VCPKG_CHAINLOAD_TOOLCHAIN_FILE",
                ),
                (
                    "triplet",
                    (
                        "VCPKG_TARGET_TRIPLET:STRING="
                        f"{verify_build.TARGET_TRIPLET}"
                    ),
                    "VCPKG_TARGET_TRIPLET:STRING=wrong-triplet",
                    "VCPKG_TARGET_TRIPLET",
                ),
            )
            for (
                label,
                original_value,
                drifted_value,
                expected_message,
            ) in port_control_plane_mutations:
                with self.subTest(port_control_plane=label):
                    self.assertIn(original_value, original_cache)
                    cache_file.write_text(
                        original_cache.replace(
                            original_value,
                            drifted_value,
                            1,
                        ),
                        encoding="utf-8",
                    )
                    with self.assertRaisesRegex(
                        AssertionError,
                        expected_message,
                    ):
                        with mock.patch.object(
                            verify_build,
                            "run_text",
                            return_value=json.dumps(expanded),
                        ):
                            verify_build.verify_qtmultimedia_compile_path_maps(
                                repo,
                                installed=installed,
                                build=build,
                                source_parent=source.parent,
                                ninja=ninja,
                                packages_root=packages_root,
                            )
            cache_file.write_text(original_cache, encoding="utf-8")

            database[0]["arguments"] = [
                argument
                for argument in arguments
                if not argument.startswith("-fmacro-prefix-map=")
            ]
            compile_database.write_text(
                json.dumps(database),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "exactly two -fmacro-prefix-map",
            ):
                with mock.patch.object(
                    verify_build,
                    "run_text",
                    return_value=json.dumps(expanded),
                ):
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )

            database[0]["arguments"] = [
                "fake-compiler",
                *arguments[1:],
            ]
            compile_database.write_text(
                json.dumps(database),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "non-pinned compiler",
            ):
                with mock.patch.object(
                    verify_build,
                    "run_text",
                    return_value=json.dumps(expanded),
                ):
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )

            database[0]["arguments"] = arguments
            compile_database.write_text(
                json.dumps(database),
                encoding="utf-8",
            )
            drifted_expanded = copy.deepcopy(expanded)
            drifted_expanded[0]["arguments"].insert(
                1,
                "-DQT_REPRO_NINJA_DRIFT=1",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "expanded Ninja compiler argv differs",
            ):
                with mock.patch.object(
                    verify_build,
                    "run_text",
                    return_value=json.dumps(drifted_expanded),
                ):
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )

            output.write_bytes(second_output_bytes)
            second_output.write_bytes(output_bytes)
            with mock.patch.object(
                verify_build,
                "run_text",
                return_value=json.dumps(expanded),
            ):
                swapped_identity = (
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )
                )
            self.assertNotEqual(
                swapped_identity["aggregateSha256"],
                identity["aggregateSha256"],
            )
            self.assertEqual(
                swapped_identity["installedObjectAggregateSha256"],
                identity["installedObjectAggregateSha256"],
            )
            output.write_bytes(output_bytes)
            second_output.write_bytes(second_output_bytes)

            installed_output.write_bytes(
                b"\0asm\1\0\0\0different installed object"
            )
            with self.assertRaisesRegex(
                AssertionError,
                "do not exactly cover installed",
            ):
                with mock.patch.object(
                    verify_build,
                    "run_text",
                    return_value=json.dumps(expanded),
                ):
                    verify_build.verify_qtmultimedia_compile_path_maps(
                        repo,
                        installed=installed,
                        build=build,
                        source_parent=source.parent,
                        ninja=ninja,
                        packages_root=packages_root,
                    )

    def test_qtmultimedia_binary_package_rejects_stale_installed_member(
        self,
    ) -> None:
        abi_info_bytes = b"features core;qml\ntriplet wasm32-emscripten-rg\n"
        abi = hashlib.sha256(abi_info_bytes).hexdigest()
        static_payloads = {
            "Qt6/plugins/multimedia/libwasmmediaplugin.a":
                b"current wasm media plugin",
            "lib/libQt6Multimedia.a": b"current multimedia core",
        }
        installable_payloads = {
            **static_payloads,
            "sbom/qtmultimedia-6.11.1.spdx": b"volatile sbom",
            (
                "share/Qt6Multimedia/"
                "Qt6MultimediaTargets-release.cmake"
            ): b"set(_qt_multimedia_core lib/libQt6Multimedia.a)\n",
            "share/qtmultimedia/vcpkg.spdx.json": b"volatile vcpkg sbom",
            "share/qtmultimedia/vcpkg_abi_info.txt": abi_info_bytes,
        }
        members = [
            {
                "bytes": len(static_payloads[path]),
                "path": path,
                "sha256": hashlib.sha256(
                    static_payloads[path]
                ).hexdigest(),
            }
            for path in sorted(static_payloads)
        ]
        aggregate = hashlib.sha256()
        for member in members:
            aggregate.update(member["path"].encode("utf-8"))
            aggregate.update(b"\0")
            aggregate.update(str(member["bytes"]).encode("ascii"))
            aggregate.update(b"\0")
            aggregate.update(member["sha256"].encode("ascii"))
            aggregate.update(b"\n")
        installable_inventory = hashlib.sha256()
        for relative in sorted(installable_payloads):
            installable_inventory.update(
                f"{relative}\n".encode("utf-8")
            )
        deterministic_payloads = {
            path: content
            for path, content in installable_payloads.items()
            if (
                path not in
                verify_build.QTMULTIMEDIA_DETERMINISTIC_PAYLOAD_EXCLUSIONS
                and not path.endswith((".a", ".o"))
            )
        }
        deterministic = verify_build._path_payload_identity(
            deterministic_payloads
        )
        deterministic["excluded"] = list(
            verify_build.QTMULTIMEDIA_DETERMINISTIC_PAYLOAD_EXCLUSIONS
        )
        contract = {
            "abi": abi,
            "algorithm":
                verify_build.QTMULTIMEDIA_BINARY_PAYLOAD_ALGORITHM,
            "aggregateSha256": aggregate.hexdigest(),
            "deterministicPayload": deterministic,
            "installableFileCount": len(installable_payloads),
            "installableInventorySha256":
                installable_inventory.hexdigest(),
            "memberCount": len(members),
            "members": members,
            "totalBytes": sum(
                len(value) for value in static_payloads.values()
            ),
        }

        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            installed = repo / ".wasm-vcpkg" / "installed"
            abi_info = (
                repo
                / ".wb"
                / "qtmultimedia"
                / (
                    f"{verify_build.TARGET_TRIPLET}"
                    ".vcpkg_abi_info.txt"
                )
            )
            abi_info.parent.mkdir(parents=True)
            abi_info.write_bytes(abi_info_bytes)
            for relative, content in installable_payloads.items():
                destination = (
                    installed
                    / verify_build.TARGET_TRIPLET
                    / relative
                )
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content)
            info = (
                installed
                / "vcpkg"
                / "info"
                / (
                    "qtmultimedia_"
                    f"{verify_build.EXPECTED_QT}_"
                    f"{verify_build.TARGET_TRIPLET}.list"
                )
            )
            info.parent.mkdir(parents=True)
            info.write_text(
                "".join(
                    f"{verify_build.TARGET_TRIPLET}/{path}\n"
                    for path in sorted(installable_payloads)
                ),
                encoding="utf-8",
            )
            cache = (
                repo
                / ".wasm-vcpkg"
                / "bincache"
                / abi[:2]
                / f"{abi}.zip"
            )
            cache.parent.mkdir(parents=True)
            control = (
                "Package: qtmultimedia\n"
                f"Version: {verify_build.EXPECTED_QT}\n"
                "Port-Version: 2\n"
                f"Architecture: {verify_build.TARGET_TRIPLET}\n"
                f"Abi: {abi}\n"
            )
            with zipfile.ZipFile(cache, "w") as package:
                package.writestr("CONTROL", control)
                for relative, content in installable_payloads.items():
                    package.writestr(relative, content)

            evidence = (
                verify_build.verify_qtmultimedia_binary_package(
                    repo,
                    installed,
                    abi_info,
                    abi,
                    contract,
                )
            )
            self.assertEqual(
                evidence["BinaryPayloadAggregateSha256"],
                contract["aggregateSha256"],
            )
            targets_file = (
                installed
                / verify_build.TARGET_TRIPLET
                / "share"
                / "Qt6Multimedia"
                / "Qt6MultimediaTargets-release.cmake"
            )
            current_targets = targets_file.read_bytes()
            targets_file.write_bytes(
                current_targets.replace(
                    b"libQt6Multimedia.a",
                    b"libQt6Multimedia-stale.a",
                )
            )
            with self.assertRaisesRegex(
                AssertionError,
                "installed qtmultimedia package file drifted",
            ):
                verify_build.verify_qtmultimedia_binary_package(
                    repo,
                    installed,
                    abi_info,
                    abi,
                    contract,
                )
            targets_file.write_bytes(current_targets)

            unowned_core = (
                installed
                / verify_build.TARGET_TRIPLET
                / "lib"
                / "libQt6Multimedia-stale.a"
            )
            unowned_core.write_bytes(b"stale renamed multimedia core")
            with self.assertRaisesRegex(
                AssertionError,
                "target static package ownership inventory drifted",
            ):
                verify_build.verify_qtmultimedia_binary_package(
                    repo,
                    installed,
                    abi_info,
                    abi,
                    contract,
                )
            unowned_core.unlink()

            stale_plugin = (
                installed
                / verify_build.TARGET_TRIPLET
                / "Qt6/plugins/multimedia/libwasmmediaplugin.a"
            )
            stale_bytes = bytearray(stale_plugin.read_bytes())
            stale_bytes[0] ^= 0x01
            stale_plugin.write_bytes(stale_bytes)
            with self.assertRaisesRegex(
                AssertionError,
                "installed qtmultimedia package file drifted",
            ):
                verify_build.verify_qtmultimedia_binary_package(
                    repo,
                    installed,
                    abi_info,
                    abi,
                    contract,
                )

    def test_qtmultimedia_core_is_cross_bound_to_application_link(
        self,
    ) -> None:
        multimedia = {
            "BinaryPayloadCoreBytes": 123,
            "BinaryPayloadCorePath":
                verify_build.QTMULTIMEDIA_CORE_ARCHIVE,
            "BinaryPayloadCoreSha256": "a" * 64,
        }
        qt = {
            "target": {
                "requiredPorts": {
                    "qtmultimedia": multimedia,
                },
            },
        }
        linked_core = {
            "bytes": 123,
            "kind": "archive",
            "path": verify_build.QTMULTIMEDIA_CORE_ARCHIVE,
            "sha256": "a" * 64,
        }
        application_link = {
            "dependencyArchives": {
                "linkedClosure": {
                    "archives": [linked_core],
                },
            },
        }
        verify_build.validate_qtmultimedia_application_link_binding(
            qt,
            application_link,
        )
        linked_core["sha256"] = "b" * 64
        with self.assertRaisesRegex(
            AssertionError,
            "canonical qtmultimedia core",
        ):
            verify_build.validate_qtmultimedia_application_link_binding(
                qt,
                application_link,
            )

    def test_target_static_source_lock_is_cross_bound_to_link(
        self,
    ) -> None:
        identity = dict(
            verify_build.EXPECTED_TARGET_STATIC_LINK_INPUT_IDENTITY
        )
        qt = {
            "target": {
                "staticLinkInputIdentity": identity,
            },
        }
        application_link = {
            "dependencyArchives": {
                "superset": dict(identity),
            },
        }
        verify_build.validate_target_static_link_input_binding(
            qt,
            application_link,
        )
        application_link["dependencyArchives"]["superset"][
            "fileCount"
        ] += 1
        with self.assertRaisesRegex(
            AssertionError,
            "source lock and application-link superset disagree",
        ):
            verify_build.validate_target_static_link_input_binding(
                qt,
                application_link,
            )

    def test_current_overlays_pass_real_verifier_when_vcpkg_is_present(
        self,
    ) -> None:
        repo = Path(verify_build.__file__).resolve().parents[3]
        vcpkg = (
            repo
            / ".toolchains"
            / f"vcpkg-{verify_build.EXPECTED_VCPKG_COMMIT[:8]}"
        )
        if not (vcpkg / "ports" / "qtbase" / "portfile.cmake").is_file():
            self.skipTest("requires the pinned vcpkg source installation")
        evidence = verify_build.verify_overlays(repo, vcpkg)
        verify_build.validate_overlay_evidence(evidence)

    def test_overlay_evidence_rejects_forged_aggregate(self) -> None:
        evidence = self.overlay_evidence_fixture()
        verify_build.validate_overlay_evidence(evidence)
        evidence["qtbase"]["aggregateSha256"] = "0" * 64
        with self.assertRaisesRegex(
            AssertionError,
            "qtbase.aggregateSha256",
        ):
            verify_build.validate_overlay_evidence(evidence)

    def test_overlay_evidence_rejects_phantom_file(self) -> None:
        evidence = self.overlay_evidence_fixture()
        qtbase = evidence["qtbase"]
        qtbase["fileSha256"]["phantom.patch"] = "0" * 64
        qtbase["fileCount"] += 1
        qtbase["aggregateSha256"] = self.overlay_hash_aggregate(
            qtbase["fileSha256"]
        )
        with self.assertRaisesRegex(
            AssertionError,
            "qtbase.fileCount|qtbase file inventory",
        ):
            verify_build.validate_overlay_evidence(evidence)

    def test_overlay_evidence_rejects_missing_file_hash(self) -> None:
        evidence = self.overlay_evidence_fixture()
        qtbase = evidence["qtbase"]
        qtbase["fileSha256"].pop("port.data.cmake")
        qtbase["fileCount"] -= 1
        qtbase["aggregateSha256"] = self.overlay_hash_aggregate(
            qtbase["fileSha256"]
        )
        with self.assertRaisesRegex(
            AssertionError,
            "qtbase.fileCount|qtbase file inventory",
        ):
            verify_build.validate_overlay_evidence(evidence)

    def test_triplet_passthrough_requires_emsdk_and_python(self) -> None:
        triplet = "set(VCPKG_ENV_PASSTHROUGH EMSDK)\n"
        with self.assertRaisesRegex(
            AssertionError,
            "EMSDK_PYTHON",
        ):
            verify_build.require_vcpkg_env_passthrough(
                triplet,
                ("EMSDK", "EMSDK_PYTHON"),
            )

    def test_vcpkg_port_cmake_manifest_entry_is_exact(self) -> None:
        expected = dict(
            verify_build.EXPECTED_VCPKG_PORT_CMAKE_MANIFEST_ENTRY
        )
        manifest = {"schema-version": 1, "tools": [expected]}
        self.assertEqual(
            verify_build.select_vcpkg_port_cmake_manifest_entry(manifest),
            expected,
        )

        cases = {
            "wrong entry": ("executable", "near/cmake.exe"),
            "wrong version": ("version", "4.3.30"),
            "wrong hash": ("sha512", "0" * 128),
            "wrong URL": ("url", "https://example.invalid/cmake.zip"),
        }
        for label, (field, value) in cases.items():
            with self.subTest(label=label):
                altered = dict(expected)
                altered[field] = value
                with self.assertRaisesRegex(
                    AssertionError,
                    "vcpkg.*CMake",
                ):
                    verify_build.select_vcpkg_port_cmake_manifest_entry(
                        {"schema-version": 1, "tools": [altered]}
                    )

        with self.assertRaisesRegex(AssertionError, "exactly one"):
            verify_build.select_vcpkg_port_cmake_manifest_entry(
                {"schema-version": 1, "tools": [expected, dict(expected)]}
            )

    def test_vcpkg_port_cmake_executable_matches_manifest_archive(
        self,
    ) -> None:
        member = (
            "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive = root / "cmake.zip"
            executable = root / "cmake.exe"
            payload = b"authenticated-cmake"
            with zipfile.ZipFile(archive, "w") as package:
                package.writestr(member, payload)
            executable.write_bytes(payload)

            self.assertEqual(
                verify_build.require_archive_member_matches_file(
                    archive,
                    member,
                    executable,
                ),
                verify_build.sha256(executable),
            )

            executable.write_bytes(b"mutated-cmake")
            with self.assertRaisesRegex(
                AssertionError,
                "archive member",
            ):
                verify_build.require_archive_member_matches_file(
                    archive,
                    member,
                    executable,
                )

    def test_build_tool_archive_binds_complete_installed_tree(self) -> None:
        files = {
            "bin/tool.exe": b"tool executable",
            "share/Modules/Support.cmake": b"authenticated support module",
        }
        directories = sorted(
            {
                "/".join(path.split("/")[:index])
                for path in files
                for index in range(1, len(path.split("/")))
            }
        )
        inventory = hashlib.sha256(
            "".join(f"{path}\n" for path in sorted(files)).encode("utf-8")
        ).hexdigest()
        directory_inventory = hashlib.sha256(
            "".join(f"{path}/\n" for path in directories).encode("utf-8")
        ).hexdigest()
        aggregate = hashlib.sha256(
            "".join(
                (
                    f"{path}\0"
                    f"{hashlib.sha256(files[path]).hexdigest()}\n"
                )
                for path in sorted(files)
            ).encode("utf-8")
        ).hexdigest()

        with tempfile.TemporaryDirectory() as directory:
            repo = Path(directory)
            downloads = repo / ".toolchains" / "downloads"
            installation = repo / ".toolchains" / "tool-1"
            downloads.mkdir(parents=True)
            archive = downloads / "tool.zip"
            with zipfile.ZipFile(archive, "w") as package:
                for relative, content in files.items():
                    package.writestr(f"tool-1/{relative}", content)
            for relative, content in files.items():
                target = installation / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(content)
            artifact = {
                "version": "1",
                "url": "https://example.invalid/tool.zip",
                "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "archiveFile": "tool.zip",
                "executableSha256": hashlib.sha256(
                    files["bin/tool.exe"]
                ).hexdigest(),
                "directory": "tool-1",
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "tool-1",
                    "fileCount": len(files),
                    "directoryCount": len(directories),
                    "totalBytes": sum(map(len, files.values())),
                    "inventorySha256": inventory,
                    "directoryInventorySha256": directory_inventory,
                    "aggregateSha256": aggregate,
                },
            }

            identity = verify_build.verify_authenticated_build_tool(
                repo,
                "Tool",
                artifact,
                installation,
            )
            self.assertEqual(identity["payload"], artifact["payload"])
            self.assertNotIn(str(repo), json.dumps(identity))

            support = installation / "share" / "Modules" / "Support.cmake"
            timestamps = (support.stat().st_atime_ns, support.stat().st_mtime_ns)
            original = support.read_bytes()
            support.write_bytes(
                original[:-1] + bytes((original[-1] ^ 0x01,))
            )
            os.utime(support, ns=timestamps)
            with self.assertRaisesRegex(
                AssertionError,
                "installed file SHA-256",
            ):
                verify_build.verify_authenticated_build_tool(
                    repo,
                    "Tool",
                    artifact,
                    installation,
                )
            support.write_bytes(original)

            with archive.open("ab") as stream:
                stream.write(b"tampered archive")
            with self.assertRaisesRegex(
                AssertionError,
                "archive SHA-256",
            ):
                verify_build.verify_authenticated_build_tool(
                    repo,
                    "Tool",
                    artifact,
                    installation,
                )

    def test_source_zip_binds_installed_members_and_rejects_unmodeled_extras(
        self,
    ) -> None:
        source_files = {
            "README.md": b"authenticated source\n",
            "scripts/tool.py": b"print('authenticated')\n",
        }
        directories = ["scripts"]
        inventory = hashlib.sha256(
            "".join(f"{path}\n" for path in sorted(source_files)).encode(
                "utf-8"
            )
        ).hexdigest()
        directory_inventory = hashlib.sha256(
            "".join(f"{path}/\n" for path in directories).encode("utf-8")
        ).hexdigest()
        aggregate = hashlib.sha256(
            "".join(
                (
                    f"{path}\0"
                    f"{hashlib.sha256(source_files[path]).hexdigest()}\n"
                )
                for path in sorted(source_files)
            ).encode("utf-8")
        ).hexdigest()

        with tempfile.TemporaryDirectory(
            prefix="rg-authenticated-source-"
        ) as directory:
            repo = Path(directory)
            downloads = repo / ".toolchains" / "downloads"
            installation = repo / ".toolchains" / "source-install"
            downloads.mkdir(parents=True)
            archive = downloads / "source.zip"
            with zipfile.ZipFile(archive, "w") as package:
                for relative, content in source_files.items():
                    package.writestr(f"source-root/{relative}", content)
            for relative, content in source_files.items():
                target = installation / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(content)
            (installation / "runtime.exe").write_bytes(b"runtime product")
            artifact = {
                "url": "https://example.invalid/source.zip",
                "archiveFile": archive.name,
                "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "stripPrefix": "source-root",
                    "fileCount": len(source_files),
                    "directoryCount": len(directories),
                    "totalBytes": sum(map(len, source_files.values())),
                    "inventorySha256": inventory,
                    "directoryInventorySha256": directory_inventory,
                    "aggregateSha256": aggregate,
                },
                "allowedRuntimePrefixes": [],
                "allowedRuntimeFiles": ["runtime.exe"],
            }

            identity = verify_build.verify_authenticated_source_archive(
                repo,
                "fixture",
                artifact,
                installation,
            )
            self.assertEqual(identity["payload"], artifact["payload"])
            self.assertEqual(
                identity["allowedRuntimeFiles"],
                ["runtime.exe"],
            )

            source = installation / "scripts" / "tool.py"
            original = source.read_bytes()
            timestamps = (source.stat().st_atime_ns, source.stat().st_mtime_ns)
            source.write_bytes(
                original[:-1] + bytes((original[-1] ^ 0x01,))
            )
            os.utime(source, ns=timestamps)
            with self.assertRaisesRegex(
                AssertionError,
                "installed source bytes",
            ):
                verify_build.verify_authenticated_source_archive(
                    repo,
                    "fixture",
                    artifact,
                    installation,
                )
            source.write_bytes(original)

            (installation / "unmodeled.txt").write_text(
                "unexpected",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                AssertionError,
                "unmodeled runtime/source path",
            ):
                verify_build.verify_authenticated_source_archive(
                    repo,
                    "fixture",
                    artifact,
                    installation,
                )

    def test_cmake_cache_command_rejects_wrong_or_near_path(self) -> None:
        expected = Path(
            r"T:\repo\.wasm-vcpkg\downloads\tools"
            r"\cmake-4.3.3-windows"
            r"\cmake-4.3.3-windows-x86_64\bin\cmake.exe"
        )
        self.assertEqual(
            verify_build.require_cache_cmake_command(
                {"CMAKE_COMMAND": str(expected)},
                expected,
                "QtBase target",
            ),
            expected.resolve(),
        )
        for wrong in (
            r"T:\repo\.toolchains\cmake-4.2.3-windows-x86_64"
            r"\bin\cmake.exe",
            str(expected) + ".backup",
        ):
            with self.subTest(wrong=wrong):
                with self.assertRaisesRegex(
                    AssertionError,
                    "CMAKE_COMMAND",
                ):
                    verify_build.require_cache_cmake_command(
                        {"CMAKE_COMMAND": wrong},
                        expected,
                        "QtBase target",
                    )

    def test_cmake_role_cross_validation_rejects_flattening(self) -> None:
        outer = (
            ".toolchains/cmake-4.2.3-windows-x86_64/bin/cmake.exe"
        )
        port = (
            ".wasm-vcpkg/downloads/tools/cmake-4.3.3-windows/"
            "cmake-4.3.3-windows-x86_64/bin/cmake.exe"
        )
        toolchains = {
            "outerProbeCMake": {
                "version": "4.2.3",
                "executable": outer,
            },
            "vcpkgPortBuildCMake": {
                "version": "4.3.3",
                "executable": port,
            },
        }
        cmake_build = {
            "outerProbeCMakeCommand": outer,
            "qtBaseTargetCMakeCommand": port,
        }
        declarative = {
            "target": {"cmakeCommand": port},
            "host": {"cmakeCommand": port},
        }
        verify_build.validate_cmake_role_cross_fields(
            toolchains,
            cmake_build,
            declarative,
        )

        flattened = json.loads(json.dumps(cmake_build))
        flattened["qtBaseTargetCMakeCommand"] = outer
        with self.assertRaisesRegex(AssertionError, "role"):
            verify_build.validate_cmake_role_cross_fields(
                toolchains,
                flattened,
                declarative,
            )

        near_miss = json.loads(json.dumps(declarative))
        near_miss["host"]["cmakeCommand"] = port + ".backup"
        with self.assertRaisesRegex(AssertionError, "role"):
            verify_build.validate_cmake_role_cross_fields(
                toolchains,
                cmake_build,
                near_miss,
            )

    def test_host_compiler_identity_is_root_portable_and_byte_bound(
        self,
    ) -> None:
        suffix = Path(
            "VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe"
        )
        payload = b"portable compiler fixture"
        contract = {
            "basename": "cl.exe",
            "toolsetRelativePath": suffix.as_posix(),
            "cmakeCompilerId": "MSVC",
            "cmakeCompilerVersion": "19.51.36244.0",
            "frontendVariant": "MSVC",
            "architecture": "x64",
            "platform": "Windows",
            "executableSha256": hashlib.sha256(payload).hexdigest(),
        }
        cmake_identity = {
            "id": "MSVC",
            "version": "19.51.36244.0",
            "frontendVariant": "MSVC",
            "architecture": "x64",
            "platform": "Windows",
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            results = []
            for edition in ("Community", "BuildTools"):
                compiler = root / edition / suffix
                compiler.parent.mkdir(parents=True)
                compiler.write_bytes(payload)
                results.append(
                    verify_build.verify_host_compiler_identity(
                        {"path": compiler, **cmake_identity},
                        contract,
                    )
                )
            self.assertEqual(results[0], results[1])
            self.assertNotIn(str(root), json.dumps(results[0]))

            tampered = root / "BuildTools" / suffix
            tampered.write_bytes(payload + b" tampered")
            with self.assertRaisesRegex(AssertionError, "SHA-256"):
                verify_build.verify_host_compiler_identity(
                    {"path": tampered, **cmake_identity},
                    contract,
                )

    def test_full_schema_rejects_absolute_external_paths(self) -> None:
        for value in (
            r"C:\Program Files\Microsoft Visual Studio\cl.exe",
            "C:/developer/sdk/cl.exe",
            r"\\server\share\tool.exe",
        ):
            with self.subTest(value=value):
                with self.assertRaisesRegex(AssertionError, "absolute path"):
                    verify_build.require_no_absolute_path_strings(
                        {"compiler": {"identity": value}},
                    )

    def test_emscripten_payload_contract_rejects_same_version_tamper(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            emsdk = Path(directory)
            manifest = emsdk / "emscripten-releases-tags.json"
            manifest.write_text(
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
            files = {
                "upstream/bin/clang.exe": b"clang fixture",
                "upstream/emscripten/em++.py": b"driver fixture",
            }
            for relative, payload in files.items():
                path = emsdk / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(payload)
            paths = sorted(files)
            inventory_payload = "".join(f"{path}\n" for path in paths)
            aggregate_payload = "".join(
                (
                    f"{path}\0"
                    f"{hashlib.sha256(files[path]).hexdigest()}\n"
                )
                for path in paths
            )
            contract = {
                "releaseManifest": {
                    "path": "emscripten-releases-tags.json",
                    "sha256": hashlib.sha256(
                        manifest.read_bytes()
                    ).hexdigest(),
                },
                "releaseHash": (
                    "ef4e9cedeac3332e4738087567552063f4f250d3"
                ),
                "packageUrl": (
                    "https://storage.googleapis.com/webassembly/"
                    "emscripten-releases-builds/win/"
                    "ef4e9cedeac3332e4738087567552063f4f250d3/"
                    "wasm-binaries.zip"
                ),
                "generatedBytecode": {
                    "cacheDirectory": "__pycache__",
                    "fileSuffix": ".pyc",
                    "normalization": (
                        "authenticate-non-bytecode-delete-cache-"
                        "authenticate-full-v1"
                    ),
                },
                "payload": {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "roots": ["upstream/bin", "upstream/emscripten"],
                    "excludedPrefixes": [],
                    "excludedSegments": [],
                    "excludedSuffixes": [],
                    "fileCount": len(paths),
                    "inventorySha256": hashlib.sha256(
                        inventory_payload.encode("utf-8")
                    ).hexdigest(),
                    "aggregateSha256": hashlib.sha256(
                        aggregate_payload.encode("utf-8")
                    ).hexdigest(),
                },
            }

            identity = verify_build.verify_emscripten_installation(
                emsdk,
                "4.0.7",
                contract,
            )
            self.assertEqual(identity["payload"]["fileCount"], 2)
            self.assertEqual(
                identity["payload"]["aggregateSha256"],
                contract["payload"]["aggregateSha256"],
            )

            (emsdk / "upstream/emscripten/em++.py").write_bytes(
                b"same version, different payload"
            )
            with self.assertRaisesRegex(AssertionError, "payload"):
                verify_build.verify_emscripten_installation(
                    emsdk,
                    "4.0.7",
                    contract,
                )

    @unittest.skipUnless(
        os.name == "nt",
        "Emscripten cache identity is Windows-only",
    )
    def test_independent_cache_verifier_checks_exact_raw_tree(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg verifier cache identity "
        ) as directory:
            repo = Path(directory)
            root = repo / "emscripten-cache-vector"
            files = {
                "include/fixture.h": b"fixture\n",
                "lib/libfixture.a": b"exact raw archive bytes\n",
            }
            for relative, content in files.items():
                destination = root / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(content)
            inventory = hashlib.sha256()
            directory_inventory = hashlib.sha256()
            aggregate = hashlib.sha256()
            for relative in sorted(files):
                content = files[relative]
                inventory.update(f"{relative}\n".encode("utf-8"))
                aggregate.update(
                    (
                        f"{relative}\0"
                        f"{hashlib.sha256(content).hexdigest()}\n"
                    ).encode("utf-8")
                )
            for relative in ("include", "lib"):
                directory_inventory.update(
                    f"{relative}/\n".encode("utf-8")
                )
            contract = {
                "algorithm": "sha256-path-null-digest-lf-v1",
                "fileCount": 2,
                "directoryCount": 2,
                "totalBytes": sum(len(content) for content in files.values()),
                "inventorySha256": inventory.hexdigest(),
                "directoryInventorySha256": (
                    directory_inventory.hexdigest()
                ),
                "aggregateSha256": aggregate.hexdigest(),
            }

            evidence = verify_build.verify_emscripten_cache_payload(
                repo,
                root,
                contract,
                "fixture cache",
            )
            self.assertEqual(evidence["payload"], contract)
            tampered = copy.deepcopy(contract)
            tampered["totalBytes"] += 1
            with self.assertRaisesRegex(AssertionError, "identity drifted"):
                verify_build.verify_emscripten_cache_payload(
                    repo,
                    root,
                    tampered,
                    "fixture cache",
                )

    def test_backdated_source_replacement_breaks_artifact_binding(
        self,
    ) -> None:
        repo = Path(__file__).resolve().parents[3]
        build = repo / "tools" / "wasm-probe" / "build" / "wasm-release"
        generated = build / "generated" / "ProbeInputDigest.cpp"
        if not generated.is_file():
            self.skipTest("requires a configured content-bound probe build")
        try:
            verify_build.verify_probe_input_binding(repo, build)
        except AssertionError:
            self.skipTest("configured probe build is stale for current inputs")
        source = (
            repo
            / "tools"
            / "wasm-probe"
            / "src"
            / "ExceptionBoundary.cpp"
        )
        original = source.read_bytes()
        stat = source.stat()
        try:
            source.write_bytes(original + b"\n// backdated mutation\n")
            os.utime(
                source,
                ns=(stat.st_atime_ns, stat.st_mtime_ns),
            )
            self.assertLessEqual(source.stat().st_mtime_ns, stat.st_mtime_ns)
            with self.assertRaisesRegex(
                AssertionError,
                "configured probe input digest",
            ):
                verify_build.verify_probe_input_binding(repo, build)
        finally:
            source.write_bytes(original)
            os.utime(
                source,
                ns=(stat.st_atime_ns, stat.st_mtime_ns),
            )
        verify_build.verify_probe_input_binding(repo, build)


if __name__ == "__main__":
    unittest.main()
