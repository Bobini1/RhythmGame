from __future__ import annotations

import copy
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))

import emscripten_cache_identity as cache_identity


@unittest.skipUnless(os.name == "nt", "Emscripten cache identity is Windows-only")
class EmscriptenCacheIdentityTest(unittest.TestCase):
    @staticmethod
    def _seed(root: Path) -> None:
        library = root / "lib" / "libfixture.a"
        library.parent.mkdir(parents=True)
        library.write_bytes(b"raw archive bytes\n")
        header = root / "include" / "fixture.h"
        header.parent.mkdir()
        header.write_bytes(b"#define FIXTURE_VALUE 42\n")
        (root / "empty").mkdir()

    def test_known_raw_tree_vector_is_stable_across_locations(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-cache-identity-vector-"
        ) as directory:
            parent = Path(directory)
            first = parent / "emscripten-cache-vector-a"
            second = (
                parent
                / "emscripten-cache-vector-with-a-much-longer-root-name-b"
            )
            self._seed(first)
            self._seed(second)

            first_report = cache_identity.generate_report(first)
            second_report = cache_identity.generate_report(second)
            first_identity = first_report["payload"]
            second_identity = second_report["payload"]

            self.assertEqual(first_identity, second_identity)
            self.assertEqual(
                first_identity,
                {
                    "algorithm": "sha256-path-null-digest-lf-v1",
                    "fileCount": 2,
                    "directoryCount": 3,
                    "totalBytes": 43,
                    "inventorySha256": (
                        "7d8e3023a1e53fe54c7d046cd4e9afd520a6207217065b812"
                        "2fe98cd3c65997e"
                    ),
                    "directoryInventorySha256": (
                        "758739593d736ad2cb89abeac12761b5d860dbdfe29e212a058"
                        "23c93c83ddf80"
                    ),
                    "aggregateSha256": (
                        "842affe42b8a0bf6b680592089111dd5f1bf918b93333b329"
                        "85e4bf53efde3f3"
                    ),
                },
            )
            formula_tamper = copy.deepcopy(first_identity)
            formula_tamper["totalBytes"] += 1
            with self.assertRaisesRegex(
                cache_identity.CacheIdentityError,
                "identity drifted",
            ):
                cache_identity.verify_identity(first, formula_tamper)

    def test_content_position_and_embedded_root_bytes_are_authenticated(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-cache-identity-adversary-"
        ) as directory:
            parent = Path(directory)
            root = parent / "emscripten-cache-adversary-a"
            self._seed(root)
            expected = cache_identity.generate_identity(root)
            header = root / "include" / "fixture.h"
            header_times = (
                header.stat().st_atime_ns,
                header.stat().st_mtime_ns,
            )
            original_header = header.read_bytes()
            header.write_bytes(
                bytes((original_header[0] ^ 0x01,))
                + original_header[1:]
            )
            os.utime(header, ns=header_times)
            with self.assertRaisesRegex(
                cache_identity.CacheIdentityError,
                "identity drifted",
            ):
                cache_identity.verify_identity(root, expected)
            header.write_bytes(original_header)

            library = root / "lib" / "libfixture.a"
            native = str(root.resolve()).encode("utf-8")
            filler = b"_" * len(native)
            library.write_bytes(b"A" + native + b"B" + filler)
            positioned = cache_identity.generate_identity(root)
            library.write_bytes(b"A" + filler + b"B" + native)
            with self.assertRaisesRegex(
                cache_identity.CacheIdentityError,
                "identity drifted",
            ):
                cache_identity.verify_identity(root, positioned)

            first = parent / "emscripten-cache-embedded-root-a"
            second = parent / "emscripten-cache-embedded-root-longer-b"
            self._seed(first)
            self._seed(second)
            (first / "lib" / "libfixture.a").write_bytes(
                str(first.resolve()).encode("utf-8")
            )
            (second / "lib" / "libfixture.a").write_bytes(
                str(second.resolve()).encode("utf-8")
            )
            self.assertNotEqual(
                cache_identity.generate_identity(first),
                cache_identity.generate_identity(second),
            )

    def test_reparse_entry_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-cache-identity-reparse-"
        ) as directory:
            parent = Path(directory)
            root = parent / "emscripten-cache-reparse-a"
            outside = parent / "outside"
            root.mkdir()
            outside.mkdir()
            (root / "regular").write_bytes(b"regular")
            link = root / "linked"
            created = subprocess.run(
                [
                    "cmd.exe",
                    "/d",
                    "/c",
                    "mklink",
                    "/J",
                    str(link),
                    str(outside),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            if created.returncode != 0:
                self.skipTest(created.stdout + created.stderr)
            with self.assertRaisesRegex(
                cache_identity.CacheIdentityError,
                "reparse point",
            ):
                cache_identity.generate_identity(root)

    def test_cli_isolated_mode_rejects_adjacent_stdlib_shadow(self) -> None:
        with tempfile.TemporaryDirectory(
            prefix="rg-cache-identity-shadow-"
        ) as directory:
            parent = Path(directory)
            root = parent / "emscripten-cache-shadow"
            self._seed(root)
            scripts = parent / "scripts"
            scripts.mkdir()
            helper = scripts / "emscripten_cache_identity.py"
            shutil.copy2(SCRIPTS / helper.name, helper)
            (scripts / "hashlib.py").write_text(
                'raise RuntimeError("adjacent hashlib shadow imported")\n',
                encoding="utf-8",
            )

            result = subprocess.run(
                [
                    sys.executable,
                    "-I",
                    "-B",
                    str(helper),
                    "--cache-root",
                    str(root),
                ],
                capture_output=True,
                text=True,
                check=False,
                timeout=10,
            )

            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertNotIn(
                "adjacent hashlib shadow",
                result.stdout + result.stderr,
            )
