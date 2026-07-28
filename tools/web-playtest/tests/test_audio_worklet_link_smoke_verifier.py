from __future__ import annotations

import importlib
import tempfile
import unittest
from pathlib import Path

verifier = importlib.import_module(
    "tools.web-playtest.tests.verify_audio_worklet_link_smoke"
)
LOADER_MARKERS = verifier.LOADER_MARKERS
WORKLET_MARKERS = verifier.WORKLET_MARKERS
verify = verifier.verify


class AudioWorkletLinkSmokeVerifierTest(unittest.TestCase):
    def test_accepts_complete_generated_artifacts(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            loader = root / "smoke.js"
            worklet = root / "smoke.aw.js"
            loader.write_text("\n".join(LOADER_MARKERS), "utf-8")
            worklet.write_text("\n".join(WORKLET_MARKERS), "utf-8")
            self.assertEqual(verify(loader, worklet), [])

    def test_reports_missing_artifact_and_missing_marker(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            loader = root / "smoke.js"
            worklet = root / "smoke.aw.js"
            self.assertEqual(
                verify(loader, worklet),
                [
                    f"missing generated artifact: {loader}",
                    f"missing generated artifact: {worklet}",
                ],
            )

            loader.write_text("\n".join(LOADER_MARKERS[:-1]), "utf-8")
            worklet.write_text("\n".join(WORKLET_MARKERS), "utf-8")
            self.assertEqual(
                verify(loader, worklet),
                [
                    "loader does not retain marker: "
                    f"{LOADER_MARKERS[-1]}"
                ],
            )


if __name__ == "__main__":
    unittest.main()
