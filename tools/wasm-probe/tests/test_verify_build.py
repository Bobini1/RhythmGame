import hashlib
import json
import os
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest import mock

import verify_build


class VerifyBuildContractTest(unittest.TestCase):
    @staticmethod
    def application_link_fixture(
        *response_arguments: str,
    ) -> tuple[str, Path, str, str]:
        expected = Path(r"T:\pinned\em++.bat")
        command = (
            r'C:\Windows\System32\cmd.exe /C "cd . && '
            r"T:\pinned\em++.bat -pthread -fwasm-exceptions "
            r"-sSUPPORT_LONGJMP=wasm -sJSPI=1 "
            r"-sAUDIO_WORKLET=1 -sWASM_WORKERS=1 "
            r"-sPTHREAD_POOL_SIZE=4 "
            r"-sPTHREAD_POOL_SIZE_STRICT=2 "
            r"-sALLOW_BLOCKING_ON_MAIN_THREAD=0 "
            r"@CMakeFiles\RhythmGameWasmProbe.rsp "
            r'-o RhythmGameWasmProbe.js && cd ."'
        )
        libraries = " ".join(
            (
                "libWasmProbeExceptionBoundary.a",
                *response_arguments,
            )
        )
        build_ninja = (
            "build RhythmGameWasmProbe.js: "
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
        return command, expected, build_ninja, rules_ninja

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
                    "  command = generate $out from $in\n"
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

        command, expected, build_ninja, rules_ninja = (
            self.application_link_fixture()
        )
        direct_fixture = (
            command.replace(
                "-pthread -fwasm-exceptions",
                "-pthread -fwasm-exceptions -fexceptions",
            ),
            expected,
            build_ninja,
            rules_ninja,
        )
        with self.assertRaisesRegex(AssertionError, "-fexceptions"):
            verify_build.verify_application_link_argument_stream(
                *direct_fixture
            )

    def test_application_link_parser_requires_exact_compiler(self) -> None:
        expected = Path(r"T:\pinned\em++.bat")
        valid = (
            r'C:\Windows\System32\cmd.exe /C "cd . && '
            r"T:\pinned\em++.bat -sJSPI "
            r"@CMakeFiles\RhythmGameWasmProbe.rsp "
            r'-o RhythmGameWasmProbe.js && cd ."'
        )
        self.assertEqual(
            verify_build.parse_application_link_arguments(valid, expected)[0],
            str(expected),
        )

        wrong = valid.replace(
            r"T:\pinned\em++.bat",
            r"T:\other\em++.bat",
        )
        with self.assertRaisesRegex(AssertionError, "link compiler"):
            verify_build.parse_application_link_arguments(wrong, expected)

    def test_final_link_archive_must_be_on_selected_edge(self) -> None:
        valid = (
            "build RhythmGameWasmProbe.js: CXX_EXECUTABLE object.o\n"
            "  LINK_LIBRARIES = libQt6Core.a  "
            "libWasmProbeExceptionBoundary.a\n"
            "  RSP_FILE = CMakeFiles\\RhythmGameWasmProbe.rsp\n"
            "\n"
            "build unrelated: phony libOther.a\n"
        )
        self.assertEqual(
            verify_build.require_final_link_archive(
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
                misplaced,
                (
                    "rule CXX_EXECUTABLE\n"
                    "  rspfile = $RSP_FILE\n"
                    "  rspfile_content = $in $LINK_PATH $LINK_LIBRARIES\n"
                ),
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

    def test_backdated_source_replacement_breaks_artifact_binding(
        self,
    ) -> None:
        repo = Path(__file__).resolve().parents[3]
        build = repo / "tools" / "wasm-probe" / "build" / "wasm-release"
        generated = build / "generated" / "ProbeInputDigest.cpp"
        if not generated.is_file():
            self.skipTest("requires a configured content-bound probe build")
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
