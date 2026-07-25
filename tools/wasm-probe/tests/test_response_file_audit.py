from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
AUDITOR = (
    REPO
    / "tools"
    / "wasm-probe"
    / "scripts"
    / "audit_emscripten_response_files.py"
)


class EmscriptenResponseFileAuditTest(unittest.TestCase):
    def _run(
        self,
        cwd: Path,
        *arguments: str,
        auditor: Path = AUDITOR,
    ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                "-I",
                "-B",
                str(auditor),
                "--cwd",
                str(cwd),
                "--",
                *arguments,
            ],
            cwd=REPO,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )

    def test_normal_link_response_is_decoded(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg response audit ") as value:
            root = Path(value)
            (root / "link.rsp").write_text(
                'main.o "lib one.a" -sJSPI -pthread "-o" '
                '"probe output.js"\n',
                encoding="utf-8",
            )

            result = self._run(root, "@link.rsp")

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["responseFileCount"], 1)
            self.assertEqual(
                payload["effectiveArguments"],
                [
                    "main.o",
                    "lib one.a",
                    "-sJSPI",
                    "-pthread",
                    "-o",
                    "probe output.js",
                ],
            )

    def test_isolated_mode_rejects_adjacent_stdlib_shadow(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg response shadow ") as value:
            root = Path(value)
            scripts = root / "scripts"
            scripts.mkdir()
            auditor = scripts / AUDITOR.name
            shutil.copy2(AUDITOR, auditor)
            (scripts / "shlex.py").write_text(
                'raise RuntimeError("adjacent shlex shadow imported")\n',
                encoding="utf-8",
            )
            (root / "forbidden.rsp").write_text(
                "-fexceptions\n",
                encoding="utf-8",
            )

            result = self._run(
                root,
                "@forbidden.rsp",
                auditor=auditor,
            )

            combined = result.stdout + result.stderr
            self.assertNotEqual(result.returncode, 0, combined)
            self.assertIn("Forbidden Emscripten", combined)
            self.assertNotIn("adjacent shlex shadow", combined)

    def test_bom_and_suffix_encoding_match_emscripten(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg response encoding ") as value:
            root = Path(value)
            (root / "bom.rsp").write_bytes(
                b"\xef\xbb\xbfordinary.o\n"
            )
            (root / "named.rsp.cp1252").write_bytes(
                '"caf\xe9.a"\n'.encode("latin-1")
            )
            result = self._run(
                root,
                "@bom.rsp",
                "@named.rsp.cp1252",
            )

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(
                payload["effectiveArguments"],
                [
                    "ordinary.o",
                    "café.a",
                ],
            )
            self.assertEqual(
                [item["encoding"] for item in payload["responseFiles"]],
                ["utf-8-sig", "cp1252"],
            )

    def test_setting_value_file_indirection_is_rejected(self) -> None:
        cases = (
            "-sEXPORTED_FUNCTIONS=@exports.txt",
            "-s EXPORTED_FUNCTIONS=@exports.txt",
            "-sUSE_PTHREADS=@value.txt",
            "-sNO_JSPI=@value.txt",
        )
        for content in cases:
            with self.subTest(content=content):
                with tempfile.TemporaryDirectory(
                    prefix="rg setting file response "
                ) as value:
                    root = Path(value)
                    (root / "outer.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )
                    result = self._run(root, "@outer.rsp")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "setting-value files",
                        result.stdout + result.stderr,
                    )

    def test_toolchain_plugin_and_linker_indirection_is_rejected(self) -> None:
        cases = (
            "-fplugin=plugin.dll",
            "-fplugin plugin.dll",
            "-fpass-plugin=pass.dll",
            "-Xclang -load -Xclang plugin.dll",
            "-Xclang=-load",
            "-B alternate-tools",
            "-Balternate-tools",
            "--sysroot alternate-root",
            "--sysroot=alternate-root",
            "--config alternate.cfg",
            "--config=alternate.cfg",
            "-fuse-ld=alternate",
            "--ld-path alternate-ld.exe",
            "-Xlinker --plugin",
            "-Wl,--plugin,plugin.dll",
            "-mllvm -load=plugin.dll",
        )
        for content in cases:
            with self.subTest(content=content):
                with tempfile.TemporaryDirectory(
                    prefix="rg tool override response "
                ) as value:
                    root = Path(value)
                    (root / "outer.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )
                    result = self._run(root, "@outer.rsp")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "toolchain override or plugin",
                        result.stdout + result.stderr,
                    )

    def test_shlex_decoded_forbidden_spellings_are_rejected(self) -> None:
        cases = {
            "escaped": r"-fexcept\ions",
            "quoted-concatenation": '-f"except"ions',
            "quoted-token": '"-fexceptions"',
            "negative-wasm": '-s "WASM_EXCEPTIONS=0"',
            "asyncify": '"-sASYNCIFY=1"',
            "cache-split": '"--cache" alternate-cache',
            "cache-joined": "--cache=alternate-cache",
            "clear-cache": "--clear-cache",
            "clear-ports": "--clear-ports",
            "generate-config": "--generate-config",
        }
        for name, content in cases.items():
            with self.subTest(name=name):
                with tempfile.TemporaryDirectory(
                    prefix="rg forbidden response "
                ) as value:
                    root = Path(value)
                    (root / "link.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )

                    result = self._run(root, "@link.rsp")

                    self.assertNotEqual(
                        result.returncode,
                        0,
                        result.stdout + result.stderr,
                    )
                    self.assertIn(
                        "Forbidden Emscripten",
                        result.stdout + result.stderr,
                    )

    def test_external_input_and_execution_options_are_rejected(self) -> None:
        cases = (
            "--pre-js hook.js",
            "--post-js=hook.js",
            "--extern-pre-js hook.js",
            "--extern-post-js=hook.js",
            "--js-library library.js",
            "--js-transform=transform.exe",
            "--shell-file shell.html",
            "--embed-file assets",
            "--preload-file=assets",
            "--exclude-file ignored",
            "--use-port=local-port.py",
            "--compiler-wrapper wrapper.exe",
            "--em-config=alternate.py",
            "--closure-args --formatting=PRETTY_PRINT",
            "--reproduce=archive.tar",
            "--valid-abspath C:/outside",
        )
        for content in cases:
            with self.subTest(content=content):
                with tempfile.TemporaryDirectory(
                    prefix="rg external option response "
                ) as value:
                    root = Path(value)
                    (root / "link.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )
                    result = self._run(root, "@link.rsp")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "external-input/execution option",
                        result.stdout + result.stderr,
                    )

    def test_unmodeled_preprocessor_and_module_inputs_are_rejected(
        self,
    ) -> None:
        cases = (
            "-include T:/outside/evil.h",
            "-imacros=T:/outside/macros.h",
            "-include-pch T:/outside/header.pch",
            "-ivfsoverlay T:/outside/vfs.yaml",
            "-fmodule-map-file=T:/outside/module.modulemap",
            "-fmodule-file T:/outside/module.pcm",
            "-fmodules-cache-path=T:/outside/cache",
            "-fprebuilt-module-path T:/outside/modules",
            "-Wp,-include,T:/outside/evil.h",
            "-Xpreprocessor -include",
            "-resource-dir T:/outside/clang",
            "--gcc-toolchain=T:/outside/gcc",
        )
        for content in cases:
            with self.subTest(content=content):
                with tempfile.TemporaryDirectory(
                    prefix="rg compiler file input "
                ) as value:
                    root = Path(value)
                    (root / "compile.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )
                    result = self._run(root, "@compile.rsp")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "file-input option",
                        result.stdout + result.stderr,
                    )

    def test_gate_setting_contradictions_are_rejected(self) -> None:
        cases = (
            "-sJSPI=0",
            "-sNO_JSPI",
            "-sAUDIO_WORKLET=0",
            "-sWASM_WORKERS=0",
            "-sPTHREAD_POOL_SIZE=8",
            "-sPTHREAD_POOL_SIZE_STRICT=0",
            "-sALLOW_BLOCKING_ON_MAIN_THREAD=1",
            "-sSUPPORT_LONGJMP=emscripten",
            "-sWASM_EXCEPTIONS=0",
            "-sDISABLE_EXCEPTION_CATCHING=1",
            "-sUSE_PTHREADS=0",
            "-sSHARED_MEMORY=0",
            "-sJSPI=1 -sJSPI=0",
            "-mno-atomics",
            "-mno-exception-handling",
            "-Xclang -fno-wasm-exceptions",
        )
        for content in cases:
            with self.subTest(content=content):
                with tempfile.TemporaryDirectory(
                    prefix="rg contradictory response "
                ) as value:
                    root = Path(value)
                    (root / "link.rsp").write_text(
                        content + "\n",
                        encoding="utf-8",
                    )
                    result = self._run(root, "@link.rsp")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(
                        "Forbidden Emscripten",
                        result.stdout + result.stderr,
                    )

    def test_nested_response_forms_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg hostile response ") as value:
            root = Path(value)
            cases = {
                "driver": "@nested.rsp\n",
                "linker": "-Wl,@nested.rsp\n",
            }
            for name, content in cases.items():
                with self.subTest(name=name):
                    (root / "outer.rsp").write_text(
                        content,
                        encoding="utf-8",
                    )
                    result = self._run(root, "@outer.rsp")
                    combined = result.stdout + result.stderr
                    self.assertNotEqual(result.returncode, 0, combined)
                    self.assertIn("nested response-file", combined.lower())

    def test_missing_top_level_response_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg missing response ") as value:
            root = Path(value)
            result = self._run(root, "@does-not-exist.rsp")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "missing",
                (result.stdout + result.stderr).lower(),
            )

    @unittest.skipUnless(
        hasattr(os, "symlink"),
        "symbolic links are unavailable",
    )
    def test_response_reparse_point_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="rg response reparse ") as value:
            root = Path(value)
            target = root / "target.rsp"
            target.write_text("ordinary.o\n", encoding="utf-8")
            link = root / "link.rsp"
            try:
                link.symlink_to(target)
            except OSError as error:
                self.skipTest(f"cannot create response symlink: {error}")

            result = self._run(root, "@link.rsp")

            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "reparse",
                (result.stdout + result.stderr).lower(),
            )


if __name__ == "__main__":
    unittest.main()
