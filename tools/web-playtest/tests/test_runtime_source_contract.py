from __future__ import annotations

import re
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


class RuntimeSourceContractTest(unittest.TestCase):
    def read(self, relative: str) -> str:
        return (REPO / relative).read_text("utf-8")

    def test_exact_keyboard_presets_are_code_based_and_exclusive(self) -> None:
        source = self.read("src/web_playtest/InputEvent.cpp")
        native = {
            "ShiftLeft": "Col1sUp",
            "ControlLeft": "Col1sDown",
            "KeyA": "Col11",
            "KeyS": "Col12",
            "KeyD": "Col13",
            "Space": "Col14",
            "KeyJ": "Col15",
            "KeyK": "Col16",
            "KeyL": "Col17",
        }
        lr2 = {
            "ShiftLeft": "Col1sUp",
            "KeyZ": "Col11",
            "KeyS": "Col12",
            "KeyX": "Col13",
            "KeyD": "Col14",
            "KeyC": "Col15",
            "KeyF": "Col16",
            "KeyV": "Col17",
        }
        native_block = source.split("constexpr auto nativeMappings", 1)[1]
        native_block = native_block.split("constexpr auto lr2Mappings", 1)[0]
        lr2_block = source.split("constexpr auto lr2Mappings", 1)[1]
        lr2_block = lr2_block.split("template<std::size_t Size>", 1)[0]
        for code, key in native.items():
            self.assertRegex(native_block, rf'"{code}".*BmsKey::{key}')
        for code, key in lr2.items():
            self.assertRegex(lr2_block, rf'"{code}".*BmsKey::{key}')
        self.assertNotIn("ControlLeft", lr2_block)
        self.assertNotIn("ShiftRight", source)

    def test_browser_capture_is_synchronous_deduplicated_and_bounded(self) -> None:
        source = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        header = self.read("src/web_playtest/InputEvent.h")
        for marker in (
            "emscripten_set_keydown_callback",
            "emscripten_set_keyup_callback",
            "emscripten_set_blur_callback",
            "emscripten_set_visibilitychange_callback",
            "event.code",
            "event.repeat",
            "event.timestamp * 1'000.0",
            "std::isfinite(event.timestamp)",
            "maximumTimestampMilliseconds",
            "inputDeduplicator.apply",
            "synthesizeReleases",
            "event->hidden",
        ):
            self.assertIn(marker, source)
        self.assertNotIn("event.key", source)
        self.assertIn("std::array<bool, gameplayCodeCount>", header)
        self.assertIn(
            "displayedPhase != RuntimePhase::Countdown &&", source
        )
        self.assertIn("displayedPhase != RuntimePhase::Playing", source)
        self.assertEqual(source.count("if (pressed && !event.repeat)"), 2)

    def test_one_fifo_owns_input_tick_and_session_order(self) -> None:
        header = self.read("src/web_playtest/GameplayWorker.h")
        event = self.read("src/web_playtest/InputEvent.h")
        self.assertEqual(header.count("RuntimeCommandQueue<"), 1)
        for command in ("Input", "Tick", "StartSession", "Abort"):
            self.assertIn(command, event)
        self.assertIn("writePosition.wait(", event)
        self.assertIn("writePosition.notify_one()", event)
        self.assertNotRegex(header, r"(input|tick).*Queue", re.I)

    def test_timestamp_clamp_preserves_original_provenance(self) -> None:
        event = self.read("src/web_playtest/InputEvent.cpp")
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        self.assertIn("mappedChartTimeNs >= watermarkNs", event)
        self.assertIn("accumulatedClampNs", event)
        self.assertIn(
            ".sourceEventMonotonicUs = inputEvent.browserMonotonicUs",
            worker,
        )
        self.assertIn("timestampWatermark.clamp(mapped)", worker)
        self.assertIn("lateInputClampNs", worker)

    def test_snapshot_mailbox_is_a_three_slot_state_machine(self) -> None:
        header = self.read("src/web_playtest/WebPlaytestSnapshot.h")
        source = self.read("src/web_playtest/WebPlaytestSnapshot.cpp")
        self.assertIn("slotCount = std::size_t{ 3 }", header)
        for state in ("Free", "Writing", "Published", "Reading"):
            self.assertIn(state, header)
        self.assertIn("publicationSequence", header)
        self.assertIn("compare_exchange_strong", source)
        self.assertIn("dropped.fetch_add", source)
        self.assertNotIn("std::mutex", header + source)
        self.assertIn("(!visible.removed || visible.holding)", source)

    def test_decode_is_complete_deterministic_and_one_asset_at_a_time(self) -> None:
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        decoder = self.read("src/web_playtest/OggVorbisDecoder.cpp")
        for marker in (
            "fixedRandomSequence",
            "loadChartDataWithRandomSequence",
            "BmsAssetResolver::fromDirectory",
            "resolver.resolve",
            "bank->addClip",
            "bank->addVoice",
            "bank->freeze",
            "std::ranges::sort(declarations",
        ):
            self.assertIn(marker, worker)
        for marker in (
            "STB_VORBIS_NO_STDIO",
            "stb_vorbis_open_memory",
            "stb_vorbis_get_samples_float_interleaved",
            "QFile",
        ):
            self.assertIn(marker, decoder)
        self.assertNotIn("STB_VORBIS_HEADER_ONLY", decoder)
        self.assertIn("info.channels > 2", decoder)

    def test_session_requires_exact_terminal_completed_reset(self) -> None:
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        runtime_sources = worker + self.read(
            "src/web_playtest/WebPlaytestRuntime.cpp"
        )
        for marker in (
            "AudioCommandType::ResetSession",
            "acknowledgement.sessionGeneration ==",
            "acknowledgement.sequenceId == resetSequence",
            "acknowledgement.phase == AudioAckPhase::Terminal",
            "AudioAckOutcome::Completed",
            "session.chartStartFrame != command.chartStartFrame",
            "core->preScheduleBgm()",
        ):
            self.assertIn(marker, worker)
        self.assertNotIn(".beginSession(", runtime_sources)
        self.assertLess(
            worker.index("if (!resetAudioSession(command))"),
            worker.index("core->preScheduleBgm()"),
        )

    def test_heap_is_preallocated_then_sealed_and_growth_is_linear(self) -> None:
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        cmake = self.read("tools/web-playtest/CMakeLists.txt")
        self.assertIn("512 } * 1024 * 1024", runtime)
        self.assertIn("emscripten_resize_heap(targetReadyHeapBytes)", runtime)
        self.assertIn("visibleNotes.reserve(", runtime)
        self.assertIn("sealReadyHeap()", runtime)
        self.assertLess(
            runtime.index("visibleNotes.reserve("),
            runtime.index("worklet->sealReadyHeap()"),
        )
        self.assertEqual(cmake.count("MEMORY_GROWTH_GEOMETRIC_STEP=0"), 2)
        self.assertNotRegex(cmake, r"MEMORY_GROWTH_GEOMETRIC_STEP=(?!0)")

    def test_thread_is_page_lifetime_and_trusted_resume_is_direct(self) -> None:
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        qml = self.read("tools/web-playtest/qml/Main.qml")
        self.assertIn("new std::thread", runtime)
        self.assertNotIn(".join(", runtime)
        self.assertNotIn(".detach(", runtime)
        self.assertIn("resumeFromTrustedGesture(", runtime)
        self.assertIn("onClicked: webPlaytest.startFromTrustedGesture()", qml)
        self.assertNotIn("Shortcut", qml)
        self.assertNotIn("Keys.onReturnPressed", qml)
        self.assertNotIn("focus: true", qml)

    def test_qml_is_snapshot_only_and_scroll_aware(self) -> None:
        qml = self.read("tools/web-playtest/qml/Main.qml")
        for marker in (
            "import QtQuick\n",
            "import QtQuick.Controls.Basic\n",
            "FrameAnimation",
            "webPlaytest.noteModel",
            "scrollPosition",
            "pairedScrollPosition",
            "currentScrollPosition",
            "pressedLaneMask",
        ):
            self.assertIn(marker, qml)
        for forbidden in (
            "Canvas",
            "BmsGameReferee",
            "SinglePlayerGameplayCore",
            "AudioTransport",
            "passKey",
        ):
            self.assertNotIn(forbidden, qml)
        self.assertNotRegex(qml, r"^import .+ \d", re.M)

    def test_failure_handoff_and_clock_rejection_are_terminal(self) -> None:
        header = self.read("src/web_playtest/GameplayWorker.h")
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        self.assertIn("std::atomic_flag failureClaimed", header)
        self.assertIn("failureClaimed.test_and_set", worker)
        self.assertIn("requestedSampleRate.store(", worker)
        self.assertIn("return std::nullopt", worker)
        self.assertIn("if (!chartTimeNs || *chartTimeNs < 0)", worker)
        self.assertNotIn(
            "state.store(DecodeHandoffState::GameplayReady",
            worker,
        )
        self.assertNotIn(
            "currentPhase.store(RuntimePhase::Ready",
            worker,
        )
        self.assertNotIn(
            "currentPhase.store(RuntimePhase::Countdown",
            worker,
        )
        self.assertNotIn(
            "currentPhase.store(RuntimePhase::Playing",
            worker,
        )
        self.assertNotIn(
            "currentPhase.store(RuntimePhase::Finished",
            worker,
        )
        self.assertNotIn(
            "currentPhase.store(RuntimePhase::Aborted",
            worker,
        )
        self.assertIn(
            "expectedReady = DecodeHandoffState::AudioTransferred",
            worker,
        )
        self.assertIn(
            "expected = DecodeHandoffState::Decoded",
            worker,
        )

    def test_start_abort_and_input_are_generation_gated(self) -> None:
        runtime_header = self.read(
            "src/web_playtest/WebPlaytestRuntime.h"
        )
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        for marker in (
            "bool startCommandInFlight",
            "bool abortCommandInFlight",
        ):
            self.assertIn(marker, runtime_header)
        self.assertIn("!startCommandInFlight", runtime)
        self.assertIn("!abortCommandInFlight", runtime)
        self.assertIn("startCommandInFlight = true", runtime)
        self.assertIn(
            "worker->sessionGeneration() != pendingSessionGeneration",
            runtime,
        )
        self.assertIn(
            "command.sessionGeneration !=\n"
            "                currentSessionGeneration.load",
            worker,
        )
        abort = worker.split(
            "GameplayWorker::abortSession(", 1
        )[1].split(
            "GameplayWorker::resetAudioSession(", 1
        )[0]
        self.assertLess(
            abort.index("compare_exchange_strong"),
            abort.index("AudioCommandType::StopAll"),
        )

    def test_countdown_input_uses_chart_time_and_terminal_errors_silence(
        self,
    ) -> None:
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        worklet_header = self.read(
            "src/web_playtest/audio/EmscriptenAudioWorklet.h"
        )
        worklet = self.read(
            "src/web_playtest/audio/EmscriptenAudioWorklet.cpp"
        )
        self.assertGreaterEqual(
            runtime.count(
                "displayedPhase != RuntimePhase::Countdown &&"
            ),
            2,
        )
        process_input = worker.split(
            "GameplayWorker::processInput(", 1
        )[1].split(
            "GameplayWorker::processTick(", 1
        )[0]
        self.assertIn("phase != RuntimePhase::Countdown", process_input)
        self.assertIn("if (!chartTimeNs || *chartTimeNs < 0)", process_input)
        self.assertIn("compare_exchange_strong", process_input)
        self.assertIn("RuntimePhase::Playing", process_input)
        self.assertIn("void enterTerminalSilence() noexcept", worklet_header)
        self.assertIn("clock.markTerminal()", worklet)
        self.assertIn("worklet->enterTerminalSilence()", runtime)
        self.assertIn("worker->droppedInputCommands()", runtime)
        self.assertIn("emit snapshotChanged()", runtime)

    def test_unsupported_underrun_telemetry_is_not_reported_as_zero(
        self,
    ) -> None:
        runtime_header = self.read(
            "src/web_playtest/WebPlaytestRuntime.h"
        )
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        qml = self.read("tools/web-playtest/qml/Main.qml")
        self.assertIn("underrunTelemetryAvailable", runtime_header)
        self.assertIn(
            "audioTelemetry.playbackStatsAvailable", runtime
        )
        self.assertIn(
            "webPlaytest.underrunTelemetryAvailable", qml
        )
        self.assertIn('qsTr("n/a")', qml)

    def test_audio_telemetry_rejects_prior_sessions(self) -> None:
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        observation = worker.split(
            "GameplayWorker::observeAudioAcknowledgement(", 1
        )[1]
        observation = observation.split(
            "GameplayWorker::publishSnapshot(", 1
        )[0]
        self.assertIn(
            "acknowledgement.sessionGeneration !=",
            observation,
        )
        self.assertIn("telemetry.sessionGeneration", observation)
        self.assertLess(
            observation.index("acknowledgement.sessionGeneration !="),
            observation.index("telemetry.lateByFrames"),
        )

    def test_browser_bridge_publishes_real_state_and_canonical_trace(
        self,
    ) -> None:
        worker_header = self.read("src/web_playtest/GameplayWorker.h")
        worker = self.read("src/web_playtest/GameplayWorker.cpp")
        runtime = self.read("src/web_playtest/WebPlaytestRuntime.cpp")
        for marker in (
            "std::atomic<const QByteArray*> publishedCompletedTrace",
            "completedTrace() const noexcept",
        ):
            self.assertIn(marker, worker_header)
        self.assertIn("core->finishTrace()", worker)
        self.assertIn(
            "publishedCompletedTrace.store(nullptr", worker
        )
        self.assertIn(
            "publishedCompletedTrace.store(completed", worker
        )
        for marker in (
            "__rhythmGameWebPlaytestTestBridge",
            "publishNativeState",
            "publishTrace",
            "rhythmgamePublishWebPlaytestState",
            "rhythmgamePublishWebPlaytestTrace",
            "worker->completedTrace()",
            "audioReadyForTrustedResume",
            "underrunTelemetryAvailable",
        ):
            self.assertIn(marker, runtime)
        self.assertNotIn("startFromTestBridge", runtime)


if __name__ == "__main__":
    unittest.main()
