from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

import prewarm_emscripten_cache as prewarm


class PrewarmEmscriptenCacheTest(unittest.TestCase):
    def test_prefix_map_is_one_argv_element_with_spaces(self) -> None:
        cache = Path(r"T:\root with spaces\cache")
        upstream = [
            "-g",
            "-ffile-prefix-map=/source=/emsdk/emscripten",
            "-ffile-prefix-map=../source=/emsdk/emscripten",
        ]
        mapped, audit = prewarm._mapped_cflags(
            lambda *_args, **_kwargs: list(upstream),
            cache,
            "/emsdk/cache",
        )
        flags = mapped("representative-build")
        expected = (
            "-ffile-prefix-map="
            f"{cache}=/emsdk/cache"
        )
        self.assertEqual(flags[:-1], upstream)
        self.assertEqual(flags[-1], expected)
        self.assertEqual(flags.count(expected), 1)
        self.assertEqual(audit, {"calls": 1})

    def test_isolated_cli_records_exact_representative_flags(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg prewarm adapter with spaces "
        ) as directory:
            sandbox = Path(directory)
            scripts = sandbox / "scripts with shadows"
            scripts.mkdir()
            helper = scripts / "prewarm_emscripten_cache.py"
            shutil.copy2(SCRIPTS / helper.name, helper)
            for module in ("argparse.py", "hashlib.py", "inspect.py"):
                (scripts / module).write_text(
                    'raise RuntimeError("adjacent stdlib shadow imported")\n',
                    encoding="utf-8",
                )

            emscripten = sandbox / "authenticated emscripten root"
            tools = emscripten / "tools"
            tools.mkdir(parents=True)
            (tools / "__init__.py").write_text("", encoding="utf-8")
            system_libs = tools / "system_libs.py"
            system_libs.write_text(
                "\n".join(
                    (
                        "USE_NINJA = 0",
                        "",
                        "def get_base_cflags(",
                        "    build_dir,",
                        "    force_object_files=False,",
                        "    preprocess=True,",
                        "):",
                        "    return [",
                        "        '-g',",
                        (
                            "        '-ffile-prefix-map=/source="
                            "/emsdk/emscripten',"
                        ),
                        (
                            "        '-ffile-prefix-map=../source="
                            "/emsdk/emscripten',"
                        ),
                        "    ]",
                        "",
                    )
                ),
                encoding="utf-8",
            )
            (emscripten / "embuilder.py").write_text(
                "\n".join(
                    (
                        "import json",
                        "from pathlib import Path",
                        "import subprocess",
                        "import sys",
                        "from tools import system_libs",
                        "",
                        "def main():",
                        "    flags = [",
                        "        system_libs.get_base_cflags('c-build'),",
                        "        system_libs.get_base_cflags(",
                        "            'cxx-build', force_object_files=True",
                        "        ),",
                        "    ]",
                        "    root = Path(__file__).resolve().parent",
                        "    for child in ('emcc.bat', 'em++.bat', 'emar.bat'):",
                        "        completed = subprocess.run(",
                        "            [str(root / child), '--fixture-child'],",
                        "            check=False,",
                        "        )",
                        "        if completed.returncode:",
                        "            return completed.returncode",
                        "    print(json.dumps({",
                        "        'argv': sys.argv,",
                        "        'flags': flags,",
                        "    }, sort_keys=True))",
                        "    return 0",
                        "",
                    )
                ),
                encoding="utf-8",
            )
            child_inputs: dict[str, str] = {}
            for launcher, driver in (
                ("emcc.bat", "emcc.py"),
                ("em++.bat", "em++.py"),
                ("emar.bat", "emar.py"),
                ("emranlib.bat", "emranlib.py"),
            ):
                (emscripten / launcher).write_bytes(
                    b"@echo off\r\nexit /b 99\r\n"
                )
                (emscripten / driver).write_text(
                    "\n".join(
                        (
                            "import sys",
                            (
                                "raise SystemExit("
                                "0 if sys.argv[1:] == ['--fixture-child'] "
                                "else 98)"
                            ),
                            "",
                        )
                    ),
                    encoding="utf-8",
                )
                child_inputs[launcher] = hashlib.sha256(
                    (emscripten / launcher).read_bytes()
                ).hexdigest()
                child_inputs[driver] = hashlib.sha256(
                    (emscripten / driver).read_bytes()
                ).hexdigest()
            cache = sandbox / "exact empty cache root with spaces"
            cache.mkdir()
            environment = os.environ.copy()
            for name in (
                "CCC_OVERRIDE_OPTIONS",
                "EMCC_CFLAGS",
                "EM_COMPILER_WRAPPER",
                "EM_COMPILER_WRAPPER2",
                "NODE_OPTIONS",
                "NODE_PATH",
                "PYTHONHOME",
                "PYTHONPATH",
            ):
                environment.pop(name, None)
            environment.update(
                {
                    "EM_CACHE": str(cache),
                    "EMCC_CORES": "4",
                    "EMSDK_PYTHON": str(Path(sys.executable).resolve()),
                    "PYTHONNOUSERSITE": "1",
                    "PYTHONDONTWRITEBYTECODE": "1",
                }
            )
            expected_sha256 = hashlib.sha256(
                system_libs.read_bytes()
            ).hexdigest()
            command = [
                sys.executable,
                "-I",
                "-B",
                str(helper),
                "--emscripten-root",
                str(emscripten),
                "--cache-root",
                str(cache),
                "--prefix-target",
                "/emsdk/cache",
                "--system-libs-sha256",
                expected_sha256,
                "--emcc-launcher-sha256",
                child_inputs["emcc.bat"],
                "--emcc-py-sha256",
                child_inputs["emcc.py"],
                "--emxx-launcher-sha256",
                child_inputs["em++.bat"],
                "--emxx-py-sha256",
                child_inputs["em++.py"],
                "--emar-launcher-sha256",
                child_inputs["emar.bat"],
                "--emar-py-sha256",
                child_inputs["emar.py"],
                "--emranlib-launcher-sha256",
                child_inputs["emranlib.bat"],
                "--emranlib-py-sha256",
                child_inputs["emranlib.py"],
            ]
            completed = subprocess.run(
                command,
                env=environment,
                capture_output=True,
                text=True,
                check=False,
                timeout=20,
            )
            self.assertEqual(
                completed.returncode,
                0,
                completed.stdout + completed.stderr,
            )
            self.assertNotIn(
                "adjacent stdlib shadow",
                completed.stdout + completed.stderr,
            )
            capture = json.loads(completed.stdout.strip())
            self.assertEqual(capture["argv"][1:], ["build", "SYSTEM"])
            expected_flag = (
                f"-ffile-prefix-map={cache.resolve()}=/emsdk/cache"
            )
            self.assertEqual(len(capture["flags"]), 2)
            for flags in capture["flags"]:
                self.assertEqual(flags[-1], expected_flag)
                self.assertEqual(flags.count(expected_flag), 1)

            poisoned = dict(environment)
            poisoned["NODE_OPTIONS"] = "--require=T:\\poison.js"
            rejected = subprocess.run(
                command,
                env=poisoned,
                capture_output=True,
                text=True,
                check=False,
                timeout=20,
            )
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn(
                "Ambient prewarm environment survived scrubbing",
                rejected.stdout + rejected.stderr,
            )


if __name__ == "__main__":
    unittest.main()
