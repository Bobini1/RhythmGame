from __future__ import annotations

import argparse
import sys
from pathlib import Path


LOADER_MARKERS = (
    "function rhythmgameCreateAudioContextCaught",
    "return _emscripten_create_audio_context(",
    "var _emscripten_create_audio_context =",
    "function rhythmgameRequestProcessorRegistrationCaught",
    "_emscripten_create_wasm_audio_worklet_processor_async(",
    "var _emscripten_create_wasm_audio_worklet_processor_async =",
    "function rhythmgameCreateAudioWorkletNodeCaught",
    "return _emscripten_create_wasm_audio_worklet_node(",
    "var _emscripten_create_wasm_audio_worklet_node =",
    "function rhythmgameConnectAudioNodeCaught",
    "_emscripten_audio_node_connect(",
    "var _emscripten_audio_node_connect =",
    "function rhythmgameRequestAudioContextSuspend",
    "context.suspend()",
    "function rhythmgameResumeAudioContextCaught",
    "context.resume()",
    "function rhythmgameReadAudioOutputTimestamp",
    "context.getOutputTimestamp()",
    'context.addEventListener("statechange", publish)',
    'bootstrap.addEventListener("processorerror", fail)',
    'bootstrap.port.addEventListener("messageerror", fail)',
    "bootstrap.port.start()",
    "function rhythmgameInstallProcessorNodeFailureMonitor",
    'workletNode.addEventListener("processorerror", fail)',
    'workletNode.port.addEventListener("messageerror", fail)',
    "workletNode.port.start()",
)

WORKLET_MARKERS = (
    "registerProcessor(d['_wpn'], createWasmAudioWorkletProcessor(d['ap']))",
    "p.postMessage({'_wsc': d['cb'], 'x': [d['ch'], 1",
    "registerProcessor('message', BootstrapMessages)",
)


def verify(loader: Path, worklet: Path) -> list[str]:
    failures: list[str] = []
    for path in (loader, worklet):
        if not path.is_file():
            failures.append(f"missing generated artifact: {path}")
    if failures:
        return failures

    loader_text = loader.read_text("utf-8")
    worklet_text = worklet.read_text("utf-8")
    for marker in LOADER_MARKERS:
        if marker not in loader_text:
            failures.append(f"loader does not retain marker: {marker}")
    for marker in WORKLET_MARKERS:
        if marker not in worklet_text:
            failures.append(f"worklet does not retain marker: {marker}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Verify Task 7 Emscripten WebAudio link reachability."
    )
    parser.add_argument("--loader", type=Path, required=True)
    parser.add_argument("--worklet", type=Path, required=True)
    arguments = parser.parse_args(argv)

    failures = verify(arguments.loader, arguments.worklet)
    if failures:
        for failure in failures:
            print(f"audio-worklet-link-smoke: {failure}", file=sys.stderr)
        return 1
    print("audio-worklet-link-smoke: verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
