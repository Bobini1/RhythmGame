import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import verify_build


class VerifyBuildContractTest(unittest.TestCase):
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
            verify_build.require_final_link_archive(valid),
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
            verify_build.require_final_link_archive(misplaced)

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


if __name__ == "__main__":
    unittest.main()
