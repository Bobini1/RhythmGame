from __future__ import annotations

import re
import unittest
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
AUDIO = REPO / "src" / "web_playtest" / "audio"


class AudioWorkletSourceContractTest(unittest.TestCase):
    def _read(self, relative: str) -> str:
        path = AUDIO / relative
        self.assertTrue(path.is_file(), path)
        return path.read_text("utf-8")

    def _read_repo(self, relative: str) -> str:
        path = REPO / relative
        self.assertTrue(path.is_file(), path)
        return path.read_text("utf-8")

    def _method_body(self, source: str, qualified_name: str) -> str:
        match = re.search(
            rf"{re.escape(qualified_name)}\([^{{}}]*\)"
            r"(?:\s+const)?\s+noexcept(?:\s*->\s*[^{]+)?\s*\{"
            r"(?P<body>.*?)"
            r"\n\}",
            source,
            re.S,
        )
        self.assertIsNotNone(match, qualified_name)
        return match.group("body")

    def test_browser_clock_uses_bounded_device_output_timestamp_mapping(
        self,
    ) -> None:
        header = self._read("BrowserAudioClock.h")
        source = self._read("BrowserAudioClock.cpp")
        combined = header + source

        for marker in (
            "BrowserOutputTimestamp",
            "beginRenderQuantum",
            "finishRenderQuantum",
            "tryReadRenderCursor",
            "tryEstablishAnchor",
            "chartTimeForBrowserEventUs",
            "chartTimeForBrowserMonotonicNs",
            "chartTimeForRenderedFrame",
            "completedQuantumSequence",
            "chartStartFrame",
            "chartStartBrowserMonotonicNs",
            "chartStartContextTimeNs",
            "setMixerFrameZeroContextTimeNs",
            "contextNonRunningRevision",
            "armedOutputContextTimeNs",
            "armedOutputPerformanceTimeNs",
            "maximumSnapshotAttempts",
            "minimumCountdownQuanta",
            "markTerminal",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)

        self.assertRegex(
            source,
            r"chartStartFrame\s*=\s*"
            r"afterBrowserSample\.renderedFrames\s*\+\s*countdownFrames",
        )
        self.assertIn(
            "framesToNanoseconds(\n"
            "          chartStartFrame, rate, chartStartFrameOffsetNs)",
            source,
        )
        self.assertIn(
            "checkedAdd(frameZeroContextTime,\n"
            "                    chartStartFrameOffsetNs,\n"
            "                    chartStartContextTimeNs)",
            source,
        )
        self.assertIn(
            "checkedSubtract(chartStartContextTimeNs,\n"
            "                         outputTimestamp.contextTimeNs,\n"
            "                         contextDeltaNs)",
            source,
        )
        self.assertIn(
            "checkedAdd(outputTimestamp.performanceTimeNs,\n"
            "                    contextDeltaNs,\n"
            "                    chartStartBrowserMonotonicNs)",
            source,
        )
        self.assertNotIn(
            "sampledBrowserMonotonicNs + countdownNanoseconds", source
        )

        establish = self._method_body(
            source, "BrowserAudioClock::tryEstablishAnchor"
        )
        output_mapping = establish.find(
            "outputTimestamp.performanceTimeNs"
        )
        live_cursor = establish.find("tryReadRenderCursor(liveCursor)")
        publication = establish.find("publishAnchor(candidate)")
        self.assertTrue(
            -1 < output_mapping < live_cursor < publication
        )
        self.assertIn(
            "chartStartFrame - liveCursor.renderedFrames < "
            "requiredLeadFrames",
            establish,
        )
        self.assertIn(
            "outputTimestamp.contextTimeNs <= "
            "outputContextTimeBeforeResume",
            establish,
        )
        self.assertIn(
            "outputTimestamp.performanceTimeNs <=\n"
            "          outputPerformanceTimeBeforeResume",
            establish,
        )
        self.assertNotIn("for (;;)", source)
        self.assertNotIn("while (true)", source)

    def test_browser_context_state_invalidates_stale_arms_and_mappings(
        self,
    ) -> None:
        header = self._read("BrowserAudioClock.h")
        source = self._read("BrowserAudioClock.cpp")
        combined = header + source

        for marker in (
            "browserContextStateAddressForBridge",
            "browserContextNonRunningRevisionAddressForBridge",
            "browserContextStateMirror",
            "browserContextNonRunningRevisionMirror",
            "armedContextNonRunningRevision",
            "anchorContextNonRunningRevision",
            "contextMatchesRevision",
            "std::atomic_ref<std::int32_t>",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)

        arm = self._method_body(source, "BrowserAudioClock::arm")
        cancellation = arm.find(
            "isArmed.store(false, std::memory_order_release)"
        )
        anchor_clear = arm.find("publishAnchor({})")
        validation = arm.find("sessionGeneration == 0")
        self.assertTrue(
            -1 < cancellation < anchor_clear < validation
        )
        self.assertIn("pendingGeneration", arm)
        self.assertIn("sessionGeneration < pendingGeneration", arm)
        self.assertIn(
            "currentContextNonRunningRevision() != contextRevision", arm
        )

        matches = self._method_body(
            source, "BrowserAudioClock::contextMatchesRevision"
        )
        self.assertIn("contextIsRunning()", matches)
        self.assertIn(
            "currentContextNonRunningRevision() == nonRunningRevision",
            matches,
        )

    def test_worklet_uses_caught_pinned_wasm_webaudio_path(self) -> None:
        header = self._read("EmscriptenAudioWorklet.h")
        source = self._read("EmscriptenAudioWorklet.cpp")
        combined = header + source

        for api in (
            "_emscripten_create_audio_context",
            "emscripten_start_wasm_audio_worklet_thread_async",
            "_emscripten_create_wasm_audio_worklet_processor_async",
            "_emscripten_create_wasm_audio_worklet_node",
            "_emscripten_audio_node_connect",
            "emscripten_audio_context_state",
            "emscripten_audio_context_quantum_size",
        ):
            with self.subTest(api=api):
                self.assertIn(api, source)

        for marker in (
            'latencyHint = "interactive"',
            "emscriptenGetAudioObject",
            ".sampleRate",
            "context.suspend()",
            "context.resume()",
            "promise.catch(() => {})",
            "context.getOutputTimestamp()",
            "context.currentTime",
            "performance.now()",
            "context.addEventListener('statechange', publish)",
            "Atomics.exchange",
            "Atomics.add",
            "samplesPerChannel",
            "data + frameCount",
            "alignas(16)",
            "128 * 1024",
            "AUDIO_CONTEXT_STATE_RUNNING",
            "AUDIO_CONTEXT_STATE_SUSPENDED",
            "SuspendingContext",
            "GraphReadyUnsealed",
            "Ready",
            "terminal",
            "HeapGrewAfterReady",
            "ContextSuspendTimedOut",
            "ProcessorCreationTimedOut",
            "emscripten_set_timeout_loop",
            "typeof performance",
            "typeof performance.now",
            "playbackStatsAvailable",
            "renderDurationAvailable",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, combined)

        for wrapper in (
            "rhythmgameCreateAudioContextCaught",
            "rhythmgameRequestAudioContextSuspend",
            "rhythmgameRequestProcessorRegistrationCaught",
            "rhythmgameCreateAudioWorkletNodeCaught",
            "rhythmgameConnectAudioNodeCaught",
            "rhythmgameResumeAudioContextCaught",
            "rhythmgameReadAudioOutputTimestamp",
            "rhythmgameInstallProcessorNodeFailureMonitor",
            "bootstrap.port.start()",
            "workletNode.port.start()",
        ):
            with self.subTest(wrapper=wrapper):
                self.assertIn(wrapper, source)
        self.assertGreaterEqual(source.count("catch (error)"), 8)

        for forbidden in (
            "ScriptProcessor",
            "emscripten_sleep",
            "ASYNCIFY",
            "performance.timeOrigin",
            "Date.now",
            "emscripten_get_now",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, combined)

    def test_backend_owns_every_callback_reachable_audio_object(self) -> None:
        header = self._read("EmscriptenAudioWorklet.h")
        source = self._read("EmscriptenAudioWorklet.cpp")
        initialize = self._method_body(
            source, "EmscriptenAudioWorklet::initializeWorklet"
        )

        for marker in (
            "AudioTransport ownedTransport",
            "std::optional<PcmSoundBank> ownedSoundBank",
            "std::optional<RealtimeMixer> ownedMixer",
            "PcmSoundBank&& frozenSoundBank",
            "createProcessLifetime",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, header)

        self.assertIn(
            "ownedSoundBank.emplace(std::move(frozenSoundBank))", initialize
        )
        self.assertIn("ownedMixer.emplace(", initialize)
        self.assertIn("*ownedSoundBank, ownedTransport, mixerConfig", initialize)
        for forbidden in (
            "RealtimeMixer& realtimeMixer",
            "AudioTransport& audioTransport",
            "mixer = &",
            "transport = &",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, header + source)

    def test_ready_requires_explicit_heap_seal_and_telemetry_is_worker_safe(
        self,
    ) -> None:
        header = self._read("EmscriptenAudioWorklet.h")
        source = self._read("EmscriptenAudioWorklet.cpp")
        seal = self._method_body(
            source, "EmscriptenAudioWorklet::sealReadyHeap"
        )
        telemetry = self._method_body(
            source, "EmscriptenAudioWorklet::telemetry"
        )
        process = self._method_body(
            source, "EmscriptenAudioWorklet::process"
        )

        self.assertIn("GraphReadyUnsealed", header)
        self.assertIn("emscripten_get_heap_size()", seal)
        self.assertIn("heapIsSealed.store(true", seal)
        self.assertIn("AudioWorkletLifecycleState::Ready", seal)
        self.assertIn("checkSealedHeapStable()", process)
        self.assertLess(
            process.find("checkSealedHeapStable()"),
            process.find("ownedMixer->render("),
        )
        for marker in (
            "renderedFrames.load",
            "observedHeapSize.load",
            "sealedHeapSize.load",
            "heapIsSealed.load",
            "playbackStatsAvailable.load",
            "renderDurationAvailable.load",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, telemetry)
        for forbidden in (
            "emscripten_is_main_runtime_thread",
            "emscripten_get_heap_size",
            "failTerminal",
            "checkSealedHeapStable",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, telemetry)

    def test_render_callback_is_planar_dynamic_and_realtime_safe(self) -> None:
        source = self._read("EmscriptenAudioWorklet.cpp")
        body = self._method_body(
            source, "EmscriptenAudioWorklet::process"
        )

        for marker in (
            "samplesPerChannel",
            "data + frameCount",
            "beginRenderQuantum",
            "ownedMixer->render",
            "finishRenderQuantum",
            "checkSealedHeapStable",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, body)

        for forbidden in (
            "new ",
            "delete ",
            "malloc",
            "calloc",
            "realloc",
            "std::vector",
            "std::string",
            "std::mutex",
            "std::lock",
            "throw",
            "catch",
            "EM_JS",
            "emscriptenGetAudioObject",
            "emscripten_get_now",
            "qDebug",
            "qWarning",
            "QFile",
            "postMessage",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, body)

        self.assertNotRegex(body, r"\b128\b")
        self.assertEqual(
            len(re.findall(r"ownedMixer->render\s*\(", body)), 1
        )
        self.assertRegex(
            body,
            r"(?s)AudioWorkletLifecycleState::Terminal.*?"
            r"silence\(left, right, frameCount\);.*?return true;",
        )

    def test_async_lifecycle_and_resume_anchor_order_are_explicit(
        self,
    ) -> None:
        source = self._read("EmscriptenAudioWorklet.cpp")
        for marker in (
            "AudioWorkletLifecycleState::StartingWorklet",
            "AudioWorkletLifecycleState::CreatingProcessor",
            "AudioWorkletError::WorkletThreadStartFailed",
            "AudioWorkletError::ProcessorCreationFailed",
            "AudioWorkletError::UnexpectedAsyncCallback",
            "AudioWorkletError::ProcessorRegistrationFailed",
            "AudioWorkletError::ProcessorCreationTimedOut",
            "AudioWorkletError::ProcessorRuntimeFailed",
            "compare_exchange_strong",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, source)
        self.assertGreaterEqual(source.count("if (!success)"), 2)

        resume = self._method_body(
            source,
            "EmscriptenAudioWorklet::resumeFromTrustedGesture",
        )
        self.assertIn("clock.arm(", resume)
        baseline_sample = resume.find(
            "readOutputTimestamp(outputTimestampBeforeResume)"
        )
        arm_call = resume.find("clock.arm(")
        resume_call = resume.find(
            "rhythmgameResumeAudioContextCaught(context)"
        )
        self.assertTrue(
            -1 < baseline_sample < arm_call < resume_call
        )
        self.assertIn("rhythmgameResumeAudioContextCaught(context)", resume)
        self.assertIn("clock.invalidate()", resume)
        self.assertIn("AudioWorkletLifecycleState::Unanchored", resume)
        self.assertNotIn("tryEstablishAnchor", resume)
        self.assertNotIn("publishAnchor", resume)

        poll = self._method_body(
            source, "EmscriptenAudioWorklet::pollForAnchor"
        )
        first_running_check = poll.find("!clock.contextIsRunning()")
        before_cursor = poll.find(
            "clock.tryReadRenderCursor(beforeBrowserSample)"
        )
        output_timestamp = poll.find(
            "readOutputTimestamp(outputTimestamp)"
        )
        after_cursor = poll.find(
            "clock.tryReadRenderCursor(afterBrowserSample)"
        )
        second_running_check = poll.find(
            "!clock.contextIsRunning()", first_running_check + 1
        )
        establish_anchor = poll.find(
            "clock.tryEstablishAnchor(beforeBrowserSample"
        )
        self.assertTrue(
            -1
            < first_running_check
            < before_cursor
            < output_timestamp
            < after_cursor
            < second_running_check
            < establish_anchor
        )

    def test_processor_connection_captures_suspended_frame_zero_origin(
        self,
    ) -> None:
        source = self._read("EmscriptenAudioWorklet.cpp")
        created = self._method_body(
            source, "EmscriptenAudioWorklet::onProcessorCreated"
        )
        connected = created.find("rhythmgameConnectAudioNodeCaught")
        suspension_check = created.find(
            "BrowserAudioClock::suspendedContextState"
        )
        current_time = created.find(
            "rhythmgameAudioContextCurrentTimeSeconds(context)"
        )
        frame_zero = created.find(
            "clock.setMixerFrameZeroContextTimeNs"
        )
        node_monitor = created.find(
            "rhythmgameInstallProcessorNodeFailureMonitor"
        )
        graph_ready = created.find(
            "AudioWorkletLifecycleState::GraphReadyUnsealed"
        )
        self.assertTrue(
            -1
            < connected
            < suspension_check
            < current_time
            < frame_zero
            < graph_ready
        )
        self.assertTrue(-1 < node_monitor < connected)
        self.assertIn(
            "EmscriptenAudioWorklet::monitorProcessorHealth", created
        )

        health = self._method_body(
            source, "EmscriptenAudioWorklet::pollProcessorHealth"
        )
        self.assertIn("mirrorValue(processorFailureMirror) < 0", health)
        self.assertIn(
            "failTerminal(AudioWorkletError::ProcessorRuntimeFailed)",
            health,
        )

    def test_clock_translation_unit_is_realtime_safe_and_bounded(
        self,
    ) -> None:
        source = self._read("BrowserAudioClock.cpp")
        for forbidden in (
            "new ",
            "delete ",
            "malloc",
            "calloc",
            "realloc",
            "std::vector",
            "std::string",
            "std::mutex",
            "std::condition_variable",
            "std::future",
            "#include <Q",
            "Qt::",
            "qDebug",
            "qWarning",
            "spdlog",
            "<filesystem>",
            "std::filesystem",
            "EM_JS",
            "emscriptenGetAudioObject",
            "emscripten_get_now",
            "for (;;)",
            "while (true)",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_backend_translation_unit_has_no_host_or_message_dependencies(
        self,
    ) -> None:
        source = self._read("EmscriptenAudioWorklet.cpp")
        for forbidden in (
            "#include <Q",
            '#include "Q',
            "Qt::",
            "qDebug",
            "qWarning",
            "spdlog",
            "<filesystem>",
            "std::filesystem",
            "emscripten_audio_worklet_post_function",
            "postMessage",
            "std::mutex",
            "std::lock_guard",
            "new ",
            "delete ",
            "malloc",
            "calloc",
            "realloc",
            "throw",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

        fail_terminal = self._method_body(
            source, "EmscriptenAudioWorklet::failTerminal"
        )
        self.assertIn("clock.markTerminal()", fail_terminal)

    def test_backend_never_becomes_a_second_transport_producer(self) -> None:
        combined = (
            self._read("BrowserAudioClock.cpp")
            + self._read("EmscriptenAudioWorklet.cpp")
        )
        for forbidden in (
            "tryPublish(",
            "commands.tryPush",
            "beginSession(",
            "setCurrentOutputFrame(",
            "acknowledgements.tryPop",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, combined)

    def test_backend_and_link_smoke_are_in_the_real_build_graph(self) -> None:
        cmake = self._read_repo("tools/web-playtest/CMakeLists.txt")
        manifest = self._read_repo(
            "tools/web-playtest/input-manifest.txt"
        ).splitlines()
        native_cmake = self._read_repo("test/CMakeLists.txt")
        harness = self._read_repo(
            "tools/web-playtest/tests/EmscriptenAudioWorklet.link.cpp"
        )

        for source in (
            "src/web_playtest/audio/BrowserAudioClock.cpp",
            "src/web_playtest/audio/BrowserAudioClock.h",
            "src/web_playtest/audio/EmscriptenAudioWorklet.cpp",
            "src/web_playtest/audio/EmscriptenAudioWorklet.h",
        ):
            with self.subTest(source=source):
                self.assertIn(source, cmake)
                self.assertIn(source, manifest)

        for marker in (
            "qt_add_executable(RhythmGameWasmProbe",
            "${RG_WEB_PLAYTEST_AUDIO_WORKLET_SOURCES}",
            "add_executable(RhythmGameWasmAudioLinkSmoke EXCLUDE_FROM_ALL",
            "tests/EmscriptenAudioWorklet.link.cpp",
            "verify_audio_worklet_link_smoke.py",
            "Verifying Task 7 EM_JS link reachability",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, cmake)

        self.assertIn("if (argc == 31'337)", harness)
        for call in (
            "createContextForDecode()",
            "initializeWorklet(std::move(bank), config)",
            "sealReadyHeap()",
            "resumeFromTrustedGesture(",
            "pollForAnchor(anchor)",
            "currentAudibleChartTime(chartTime)",
            "telemetry()",
        ):
            with self.subTest(call=call):
                self.assertIn(call, harness)

        for marker in (
            "add_executable(RhythmGame_browser_audio_clock_test",
            "web_playtest/audio/BrowserAudioClock.test.cpp",
            "src/web_playtest/audio/BrowserAudioClock.cpp",
            "NAME BrowserAudioClock",
        ):
            with self.subTest(marker=marker):
                self.assertIn(marker, native_cmake)


if __name__ == "__main__":
    unittest.main()
