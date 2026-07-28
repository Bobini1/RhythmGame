#include "EmscriptenAudioWorklet.h"

#if !defined(__EMSCRIPTEN__)
#error "EmscriptenAudioWorklet.cpp is an Emscripten-only backend"
#endif

#include <emscripten/em_macros.h>
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/heap.h>
#include <emscripten/threading.h>

#include <cmath>
#include <limits>
#include <utility>

// clang-format off
EM_JS_DEPS(rhythmgameWebAudioDependencies,
           "$emscriptenRegisterAudioObject,"
           "$emscriptenGetAudioObject,"
           "$UTF8ToString,"
           "emscripten_create_audio_context,"
           "emscripten_create_wasm_audio_worklet_processor_async,"
           "emscripten_create_wasm_audio_worklet_node,"
           "emscripten_audio_node_connect");

EM_JS(EMSCRIPTEN_WEBAUDIO_T,
      rhythmgameCreateAudioContextCaught,
      (const EmscriptenWebAudioCreateAttributes* options),
      {
          try {
              return _emscripten_create_audio_context(Number(options));
          } catch (error) {
              return 0;
          }
      });

EM_JS(double,
      rhythmgameAudioContextSampleRate,
      (EMSCRIPTEN_WEBAUDIO_T audioContext),
      {
          const context = emscriptenGetAudioObject(audioContext);
          if (!context || !Number.isFinite(context.sampleRate)) {
              return 0;
          }
          return context.sampleRate;
      });

EM_JS(int,
      rhythmgameInstallAudioContextStateBridge,
      (EMSCRIPTEN_WEBAUDIO_T audioContext,
       std::int32_t* stateAddress,
       std::int32_t* nonRunningRevisionAddress),
      {
          try {
              const context = emscriptenGetAudioObject(audioContext);
              if (!context || context.__rhythmgameStateBridge) {
                  return 0;
              }
              const stateIndex = Number(stateAddress) >> 2;
              const revisionIndex = Number(nonRunningRevisionAddress) >> 2;
              const publish = () =>
              {
                  const states = {
                      'suspended' : 0,
                      'running' : 1,
                      'closed' : 2,
                      'interrupted' : 3
                  };
                  const code =
                    Object.prototype.hasOwnProperty.call(states, context.state)
                      ? states[context.state]
                      : -1;
                  const previous = Atomics.exchange(HEAP32, stateIndex, code);
                  if (code !== 1 && previous !== code) {
                      Atomics.add(HEAP32, revisionIndex, 1);
                  }
              };
              context.addEventListener('statechange', publish);
              context.__rhythmgameStateBridge = publish;
              publish();
              return 1;
          } catch (error) {
              return 0;
          }
      });

EM_JS(int,
      rhythmgameRequestAudioContextSuspend,
      (EMSCRIPTEN_WEBAUDIO_T audioContext, std::int32_t* statusAddress),
      {
          const statusIndex = Number(statusAddress) >> 2;
          Atomics.store(HEAP32, statusIndex, 0);
          try {
              const context = emscriptenGetAudioObject(audioContext);
              if (!context || typeof context.suspend !== 'function') {
                  Atomics.store(HEAP32, statusIndex, -1);
                  return 0;
              }
              Promise.resolve(context.suspend())
                .then(() => Atomics.store(HEAP32, statusIndex, 1),
                      () => Atomics.store(HEAP32, statusIndex, -1));
              return 1;
          } catch (error) {
              Atomics.store(HEAP32, statusIndex, -1);
              return 0;
          }
      });

EM_JS(int,
      rhythmgameInstallProcessorFailureMonitor,
      (EMSCRIPTEN_WEBAUDIO_T audioContext, std::int32_t* failureAddress),
      {
          const failureIndex = Number(failureAddress) >> 2;
          Atomics.store(HEAP32, failureIndex, 0);
          try {
              const context = emscriptenGetAudioObject(audioContext);
              const bootstrap = context && context.audioWorklet &&
                                context.audioWorklet.bootstrapMessage;
              if (!bootstrap || bootstrap.__rhythmgameFailureMonitor) {
                  return 0;
              }
              const fail = () => Atomics.store(HEAP32, failureIndex, -1);
              bootstrap.addEventListener('processorerror', fail);
              bootstrap.port.addEventListener('messageerror', fail);
              bootstrap.port.start();
              bootstrap.__rhythmgameFailureMonitor = fail;
              return 1;
          } catch (error) {
              Atomics.store(HEAP32, failureIndex, -1);
              return 0;
          }
      });

EM_JS(int,
      rhythmgameInstallProcessorNodeFailureMonitor,
      (EMSCRIPTEN_AUDIO_WORKLET_NODE_T nodeHandle,
       std::int32_t* failureAddress),
      {
          const failureIndex = Number(failureAddress) >> 2;
          Atomics.store(HEAP32, failureIndex, 0);
          try {
              const workletNode = emscriptenGetAudioObject(nodeHandle);
              if (!workletNode || workletNode.__rhythmgameFailureMonitor ||
                  !workletNode.port) {
                  return 0;
              }
              const fail = () => Atomics.store(HEAP32, failureIndex, -1);
              workletNode.addEventListener('processorerror', fail);
              workletNode.port.addEventListener('messageerror', fail);
              workletNode.port.start();
              workletNode.__rhythmgameFailureMonitor = fail;
              return 1;
          } catch (error) {
              Atomics.store(HEAP32, failureIndex, -1);
              return 0;
          }
      });

EM_JS(EMSCRIPTEN_AUDIO_WORKLET_NODE_T,
      rhythmgameCreateAudioWorkletNodeCaught,
      (EMSCRIPTEN_WEBAUDIO_T audioContext,
       const char* processorName,
       const EmscriptenAudioWorkletNodeCreateOptions* options,
       std::uintptr_t callback,
       void* userData),
      {
          try {
              return _emscripten_create_wasm_audio_worklet_node(
                audioContext,
                Number(processorName),
                Number(options),
                callback,
                Number(userData));
          } catch (error) {
              return 0;
          }
      });

EM_JS(int,
      rhythmgameConnectAudioNodeCaught,
      (EMSCRIPTEN_AUDIO_WORKLET_NODE_T sourceHandle,
       EMSCRIPTEN_WEBAUDIO_T contextHandle),
      {
          try {
              _emscripten_audio_node_connect(sourceHandle, contextHandle, 0, 0);
              return 1;
          } catch (error) {
              return 0;
          }
      });

EM_JS(int,
      rhythmgameRequestProcessorRegistrationCaught,
      (EMSCRIPTEN_WEBAUDIO_T audioContext,
       const WebAudioWorkletProcessorCreateOptions* options,
       std::uintptr_t callback,
       void* userData),
      {
          try {
              _emscripten_create_wasm_audio_worklet_processor_async(
                audioContext, Number(options), callback, Number(userData));
              return 1;
          } catch (error) {
              return 0;
          }
      });

EM_JS(int,
      rhythmgameResumeAudioContextCaught,
      (EMSCRIPTEN_WEBAUDIO_T audioContext),
      {
          try {
              const context = emscriptenGetAudioObject(audioContext);
              if (!context || typeof context.resume !== 'function') {
                  return 0;
              }
              const promise = context.resume();
              if (promise && typeof promise.catch === 'function') {
                  promise.catch(() => {});
              }
              return 1;
          } catch (error) {
              return 0;
          }
      });

EM_JS(double,
      rhythmgameAudioContextCurrentTimeSeconds,
      (EMSCRIPTEN_WEBAUDIO_T audioContext),
      {
          const context = emscriptenGetAudioObject(audioContext);
          if (!context || !Number.isFinite(context.currentTime) ||
              context.currentTime < 0) {
              return -1;
          }
          return context.currentTime;
      });

// Returns -1 for unsupported/invalid, 0 while Chromium still reports the
// all-zero startup timestamp, and 1 for a usable device-output timestamp.
EM_JS(int,
      rhythmgameReadAudioOutputTimestamp,
      (EMSCRIPTEN_WEBAUDIO_T audioContext,
       double* contextTimeSeconds,
       double* performanceTimeMilliseconds),
      {
          try {
              const context = emscriptenGetAudioObject(audioContext);
              if (!context ||
                    typeof context.getOutputTimestamp !== 'function') {
                  return -1;
              }
              const timestamp = context.getOutputTimestamp();
              if (!timestamp || !Number.isFinite(timestamp.contextTime) ||
                  !Number.isFinite(timestamp.performanceTime)) {
                  return -1;
              }
              if (timestamp.contextTime <= 0 ||
                  timestamp.performanceTime <= 0) {
                  return 0;
              }
              HEAPF64[Number(contextTimeSeconds) >> 3] = timestamp.contextTime;
              HEAPF64[Number(performanceTimeMilliseconds) >> 3] =
                timestamp.performanceTime;
              return 1;
          } catch (error) {
              return -1;
          }
      });

EM_JS(double, rhythmgameRawBrowserPerformanceNowMilliseconds, (), {
    if (typeof performance !== 'object' ||
                                typeof performance.now !== 'function') {
        return -1;
    }
    const value = performance.now();
    return Number.isFinite(value) ? value : -1;
});
// clang-format on

namespace web_playtest {
namespace {

constexpr auto minimumSampleRate = std::uint32_t{ 8'000 };
constexpr auto maximumSampleRate = std::uint32_t{ 192'000 };
constexpr auto nanosecondsPerSecond = double{ 1'000'000'000.0 };
constexpr auto nanosecondsPerMillisecond = double{ 1'000'000.0 };

void
silence(float* left, float* right, std::uint32_t frameCount) noexcept
{
    for (auto frame = std::uint32_t{}; frame < frameCount; ++frame) {
        left[frame] = 0.F;
        right[frame] = 0.F;
    }
}

[[nodiscard]] auto
secondsToNanoseconds(double seconds, std::int64_t& nanoseconds) noexcept -> bool
{
    const auto maximumSeconds =
      static_cast<double>((std::numeric_limits<std::int64_t>::max)()) /
      nanosecondsPerSecond;
    if (!std::isfinite(seconds) || seconds < 0 || seconds > maximumSeconds) {
        return false;
    }
    nanoseconds =
      static_cast<std::int64_t>(std::llround(seconds * nanosecondsPerSecond));
    return true;
}

[[nodiscard]] auto
millisecondsToNanoseconds(double milliseconds,
                          std::int64_t& nanoseconds) noexcept -> bool
{
    const auto maximumMilliseconds =
      static_cast<double>((std::numeric_limits<std::int64_t>::max)()) /
      nanosecondsPerMillisecond;
    if (!std::isfinite(milliseconds) || milliseconds < 0 ||
        milliseconds > maximumMilliseconds) {
        return false;
    }
    nanoseconds = static_cast<std::int64_t>(
      std::llround(milliseconds * nanosecondsPerMillisecond));
    return true;
}

} // namespace

auto
EmscriptenAudioWorklet::createProcessLifetime() noexcept
  -> EmscriptenAudioWorklet*
{
    static auto instance = EmscriptenAudioWorklet{};
    return &instance;
}

auto
EmscriptenAudioWorklet::createContextForDecode() noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread()) {
        failTerminal(AudioWorkletError::NotMainRuntimeThread);
        return false;
    }
    if (state.load(std::memory_order_acquire) !=
        AudioWorkletLifecycleState::Constructed) {
        failTerminal(AudioWorkletError::InvalidLifecycle);
        return false;
    }

    const auto attributes = EmscriptenWebAudioCreateAttributes{
        .latencyHint = "interactive",
        .sampleRate = 0,
    };
    context = rhythmgameCreateAudioContextCaught(&attributes);
    if (context <= 0) {
        failTerminal(AudioWorkletError::ContextCreationFailed);
        return false;
    }
    if (rhythmgameInstallAudioContextStateBridge(
          context,
          clock.browserContextStateAddressForBridge(),
          clock.browserContextNonRunningRevisionAddressForBridge()) == 0) {
        failTerminal(AudioWorkletError::ContextStateBridgeFailed);
        return false;
    }

    const auto rawSampleRate = rhythmgameAudioContextSampleRate(context);
    if (!std::isfinite(rawSampleRate) ||
        rawSampleRate < static_cast<double>(minimumSampleRate) ||
        rawSampleRate > static_cast<double>(maximumSampleRate) ||
        std::floor(rawSampleRate) != rawSampleRate) {
        failTerminal(AudioWorkletError::InvalidSampleRate);
        return false;
    }

    const auto sampleRate = static_cast<std::uint32_t>(rawSampleRate);
    const auto rawQuantumFrames =
      emscripten_audio_context_quantum_size(context);
    if (rawQuantumFrames <= 0 ||
        static_cast<std::uint64_t>(rawQuantumFrames) >
          (std::numeric_limits<std::uint32_t>::max)() ||
        !clock.configure(sampleRate,
                         static_cast<std::uint32_t>(rawQuantumFrames))) {
        failTerminal(AudioWorkletError::InvalidQuantumSize);
        return false;
    }

    actualSampleRate.store(sampleRate, std::memory_order_release);
    quantumFrames.store(static_cast<std::uint32_t>(rawQuantumFrames),
                        std::memory_order_release);
    contextSuspendPolls.store(0, std::memory_order_relaxed);
    state.store(AudioWorkletLifecycleState::SuspendingContext,
                std::memory_order_release);
    if (rhythmgameRequestAudioContextSuspend(
          context, &suspendRequestStatusMirror) == 0) {
        failTerminal(AudioWorkletError::ContextSuspendRequestFailed);
        return false;
    }
    emscripten_set_timeout_loop(
      &EmscriptenAudioWorklet::monitorContextSuspension,
      monitorIntervalMilliseconds,
      this);
    return true;
}

auto
EmscriptenAudioWorklet::contextReadyForDecode() const noexcept -> bool
{
    return state.load(std::memory_order_acquire) ==
           AudioWorkletLifecycleState::ContextReady;
}

auto
EmscriptenAudioWorklet::initializeWorklet(
  PcmSoundBank&& frozenSoundBank,
  RealtimeMixer::Config mixerConfig) noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread()) {
        failTerminal(AudioWorkletError::NotMainRuntimeThread);
        return false;
    }
    if (state.load(std::memory_order_acquire) !=
          AudioWorkletLifecycleState::ContextReady ||
        !frozenSoundBank.frozen() ||
        frozenSoundBank.outputSampleRate() != outputSampleRate() ||
        mixerConfig.outputSampleRate != outputSampleRate()) {
        failTerminal(AudioWorkletError::InvalidSoundBank);
        return false;
    }

    try {
        ownedSoundBank.emplace(std::move(frozenSoundBank));
        ownedMixer.emplace(*ownedSoundBank, ownedTransport, mixerConfig);
    } catch (...) {
        failTerminal(AudioWorkletError::MixerCreationFailed);
        return false;
    }

    auto expected = AudioWorkletLifecycleState::ContextReady;
    if (!state.compare_exchange_strong(
          expected,
          AudioWorkletLifecycleState::StartingWorklet,
          std::memory_order_acq_rel,
          std::memory_order_acquire)) {
        failTerminal(AudioWorkletError::InvalidLifecycle);
        return false;
    }

    emscripten_start_wasm_audio_worklet_thread_async(
      context,
      workletStack.data(),
      static_cast<std::uint32_t>(workletStack.size()),
      &EmscriptenAudioWorklet::workletThreadStarted,
      this);
    return true;
}

auto
EmscriptenAudioWorklet::transport() noexcept -> AudioTransport&
{
    return ownedTransport;
}

auto
EmscriptenAudioWorklet::sealReadyHeap() noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread() ||
        state.load(std::memory_order_acquire) !=
          AudioWorkletLifecycleState::GraphReadyUnsealed ||
        clock.browserContextState() !=
          BrowserAudioClock::suspendedContextState ||
        emscripten_audio_context_state(context) !=
          AUDIO_CONTEXT_STATE_SUSPENDED) {
        return false;
    }

    const auto heapBytes =
      static_cast<std::uint64_t>(emscripten_get_heap_size());
    sealedHeapSize.store(heapBytes, std::memory_order_relaxed);
    observedHeapSize.store(heapBytes, std::memory_order_relaxed);
    heapIsSealed.store(true, std::memory_order_release);

    auto expected = AudioWorkletLifecycleState::GraphReadyUnsealed;
    if (!state.compare_exchange_strong(expected,
                                       AudioWorkletLifecycleState::Ready,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
        heapIsSealed.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

auto
EmscriptenAudioWorklet::verifySealedHeapOnMainThread() noexcept -> bool
{
    return emscripten_is_main_runtime_thread() && checkSealedHeapStable();
}

auto
EmscriptenAudioWorklet::resumeFromTrustedGesture(
  std::uint64_t sessionGeneration,
  std::uint64_t countdownFrames) noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread() || !checkSealedHeapStable()) {
        return false;
    }
    auto current = state.load(std::memory_order_acquire);
    if (current != AudioWorkletLifecycleState::Ready &&
        current != AudioWorkletLifecycleState::ResumeRequested &&
        current != AudioWorkletLifecycleState::Unanchored &&
        current != AudioWorkletLifecycleState::Anchored) {
        return false;
    }

    auto outputTimestampBeforeResume = BrowserOutputTimestamp{};
    const auto timestampStatus =
      readOutputTimestamp(outputTimestampBeforeResume);
    if (timestampStatus < 0) {
        clock.invalidate();
        failTerminal(AudioWorkletError::InvalidOutputTimestamp);
        return false;
    }
    const auto armed = clock.arm(
      sessionGeneration, countdownFrames, outputTimestampBeforeResume);
    const auto resumeRequested =
      rhythmgameResumeAudioContextCaught(context) != 0;
    if (!armed || !resumeRequested) {
        clock.invalidate();
    }
    const auto desired = armed && resumeRequested
                           ? AudioWorkletLifecycleState::ResumeRequested
                           : AudioWorkletLifecycleState::Unanchored;
    if (!state.compare_exchange_strong(current,
                                       desired,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
        if (current == AudioWorkletLifecycleState::Terminal) {
            clock.markTerminal();
        } else {
            clock.invalidate();
        }
        return false;
    }
    return armed && resumeRequested;
}

auto
EmscriptenAudioWorklet::pollForAnchor(BrowserAudioAnchor& anchor) noexcept
  -> bool
{
    if (!emscripten_is_main_runtime_thread() || !checkSealedHeapStable()) {
        return false;
    }
    const auto current = state.load(std::memory_order_acquire);
    if (current == AudioWorkletLifecycleState::Terminal) {
        return false;
    }
    if (current == AudioWorkletLifecycleState::Anchored) {
        return clock.tryReadAnchor(anchor);
    }
    if (current != AudioWorkletLifecycleState::ResumeRequested ||
        !clock.contextIsRunning() ||
        emscripten_audio_context_state(context) !=
          AUDIO_CONTEXT_STATE_RUNNING) {
        return false;
    }

    auto beforeBrowserSample = BrowserAudioClock::RenderCursor{};
    auto afterBrowserSample = BrowserAudioClock::RenderCursor{};
    if (!clock.tryReadRenderCursor(beforeBrowserSample)) {
        return false;
    }

    auto outputTimestamp = BrowserOutputTimestamp{};
    const auto timestampStatus = readOutputTimestamp(outputTimestamp);
    if (timestampStatus < 0) {
        failTerminal(AudioWorkletError::InvalidOutputTimestamp);
        return false;
    }
    if (timestampStatus == 0 ||
        !clock.tryReadRenderCursor(afterBrowserSample) ||
        !clock.contextIsRunning() ||
        emscripten_audio_context_state(context) !=
          AUDIO_CONTEXT_STATE_RUNNING) {
        return false;
    }

    if (!clock.tryEstablishAnchor(
          beforeBrowserSample, outputTimestamp, afterBrowserSample, anchor)) {
        return false;
    }

    auto expected = AudioWorkletLifecycleState::ResumeRequested;
    if (!state.compare_exchange_strong(expected,
                                       AudioWorkletLifecycleState::Anchored,
                                       std::memory_order_acq_rel,
                                       std::memory_order_acquire)) {
        if (expected == AudioWorkletLifecycleState::Terminal) {
            clock.markTerminal();
        } else {
            clock.invalidate();
        }
        return false;
    }
    return true;
}

auto
EmscriptenAudioWorklet::currentAudibleChartTime(
  std::int64_t& chartTimeNanoseconds) noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread() || !checkSealedHeapStable()) {
        return false;
    }
    auto browserNowNanoseconds = std::int64_t{};
    return browserMonotonicNowNanoseconds(browserNowNanoseconds) &&
           clock.chartTimeForBrowserMonotonicNs(browserNowNanoseconds,
                                                chartTimeNanoseconds);
}

void
EmscriptenAudioWorklet::enterTerminalSilence() noexcept
{
    clock.markTerminal();
    state.store(AudioWorkletLifecycleState::Terminal,
                std::memory_order_release);
}

auto
EmscriptenAudioWorklet::lifecycleState() const noexcept
  -> AudioWorkletLifecycleState
{
    return state.load(std::memory_order_acquire);
}

auto
EmscriptenAudioWorklet::terminalError() const noexcept -> AudioWorkletError
{
    return error.load(std::memory_order_acquire);
}

auto
EmscriptenAudioWorklet::outputSampleRate() const noexcept -> std::uint32_t
{
    return actualSampleRate.load(std::memory_order_acquire);
}

auto
EmscriptenAudioWorklet::renderQuantumFrames() const noexcept -> std::uint32_t
{
    return quantumFrames.load(std::memory_order_acquire);
}

auto
EmscriptenAudioWorklet::readyForTrustedResume() const noexcept -> bool
{
    if (!heapIsSealed.load(std::memory_order_acquire)) {
        return false;
    }
    const auto current = state.load(std::memory_order_acquire);
    return current == AudioWorkletLifecycleState::Ready ||
           current == AudioWorkletLifecycleState::ResumeRequested ||
           current == AudioWorkletLifecycleState::Unanchored ||
           current == AudioWorkletLifecycleState::Anchored;
}

auto
EmscriptenAudioWorklet::audioClock() const noexcept -> const BrowserAudioClock&
{
    return clock;
}

auto
EmscriptenAudioWorklet::telemetry() const noexcept
  -> AudioWorkletTelemetrySnapshot
{
    return {
        .renderedFrames = renderedFrames.load(std::memory_order_acquire),
        .observedHeapBytes = observedHeapSize.load(std::memory_order_acquire),
        .sealedHeapBytes = sealedHeapSize.load(std::memory_order_acquire),
        .heapSealed = heapIsSealed.load(std::memory_order_acquire),
        .playbackStatsAvailable =
          playbackStatsAvailable.load(std::memory_order_acquire),
        .underrunCount = underruns.load(std::memory_order_acquire),
        .renderDurationAvailable =
          renderDurationAvailable.load(std::memory_order_acquire),
        .lastRenderDurationNanoseconds =
          lastRenderDurationNanoseconds.load(std::memory_order_acquire),
    };
}

void
EmscriptenAudioWorklet::workletThreadStarted(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                             bool success,
                                             void* userData) noexcept
{
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    if (self != nullptr) {
        self->onWorkletThreadStarted(audioContext, success);
    }
}

void
EmscriptenAudioWorklet::processorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                         bool success,
                                         void* userData) noexcept
{
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    if (self != nullptr) {
        self->onProcessorCreated(audioContext, success);
    }
}

bool
EmscriptenAudioWorklet::monitorContextSuspension(double,
                                                 void* userData) noexcept
{
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    return self != nullptr && self->pollContextSuspension();
}

bool
EmscriptenAudioWorklet::monitorProcessorCreation(double,
                                                 void* userData) noexcept
{
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    return self != nullptr && self->pollProcessorCreation();
}

bool
EmscriptenAudioWorklet::monitorProcessorHealth(double, void* userData) noexcept
{
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    return self != nullptr && self->pollProcessorHealth();
}

bool
EmscriptenAudioWorklet::process(int numInputs,
                                const AudioSampleFrame* inputs,
                                int numOutputs,
                                AudioSampleFrame* outputs,
                                int numParams,
                                const AudioParamFrame* params,
                                void* userData) noexcept
{
    (void)inputs;
    (void)params;
    auto* self = static_cast<EmscriptenAudioWorklet*>(userData);
    if (self == nullptr || numInputs != 0 || numOutputs != 1 ||
        outputs == nullptr || numParams != 0 ||
        outputs[0].numberOfChannels != 2 || outputs[0].samplesPerChannel <= 0 ||
        outputs[0].data == nullptr) {
        if (self != nullptr) {
            self->failTerminal(AudioWorkletError::InvalidOutputShape);
        }
        return false;
    }

    const auto frameCount =
      static_cast<std::uint32_t>(outputs[0].samplesPerChannel);
    auto* left = outputs[0].data;
    auto* right = outputs[0].data + frameCount;
    if (self->state.load(std::memory_order_acquire) ==
        AudioWorkletLifecycleState::Terminal) {
        silence(left, right, frameCount);
        return true;
    }
    if (!self->heapIsSealed.load(std::memory_order_acquire) ||
        !self->checkSealedHeapStable() || !self->ownedMixer.has_value()) {
        silence(left, right, frameCount);
        self->failTerminal(AudioWorkletError::InvalidLifecycle);
        return true;
    }
    if (self->ownedTransport.terminalError().code !=
        AudioTerminalErrorCode::None) {
        silence(left, right, frameCount);
        self->failTerminal(AudioWorkletError::TransportTerminalFailure);
        return true;
    }

    self->clock.beginRenderQuantum();
    self->ownedMixer->render(left, right, frameCount);
    const auto completedFrames = self->ownedMixer->renderedFrames();
    self->clock.finishRenderQuantum(completedFrames, frameCount);
    self->renderedFrames.store(completedFrames, std::memory_order_release);

    if (self->ownedTransport.terminalError().code !=
        AudioTerminalErrorCode::None) {
        silence(left, right, frameCount);
        self->failTerminal(AudioWorkletError::TransportTerminalFailure);
        return true;
    }
    return true;
}

void
EmscriptenAudioWorklet::onWorkletThreadStarted(
  EMSCRIPTEN_WEBAUDIO_T callbackContext,
  bool success) noexcept
{
    if (callbackContext != context ||
        state.load(std::memory_order_acquire) !=
          AudioWorkletLifecycleState::StartingWorklet) {
        failTerminal(AudioWorkletError::UnexpectedAsyncCallback);
        return;
    }
    if (!success) {
        failTerminal(AudioWorkletError::WorkletThreadStartFailed);
        return;
    }
    if (rhythmgameInstallProcessorFailureMonitor(
          context, &processorFailureMirror) == 0) {
        failTerminal(AudioWorkletError::ProcessorFailureMonitorFailed);
        return;
    }

    auto expected = AudioWorkletLifecycleState::StartingWorklet;
    if (!state.compare_exchange_strong(
          expected,
          AudioWorkletLifecycleState::CreatingProcessor,
          std::memory_order_acq_rel,
          std::memory_order_acquire)) {
        failTerminal(AudioWorkletError::UnexpectedAsyncCallback);
        return;
    }

    processorCreationPolls.store(0, std::memory_order_relaxed);
    const auto options = WebAudioWorkletProcessorCreateOptions{
        .name = processorName,
        .numAudioParams = 0,
        .audioParamDescriptors = nullptr,
    };
    const auto callback = reinterpret_cast<std::uintptr_t>(
      &EmscriptenAudioWorklet::processorCreated);
    if (rhythmgameRequestProcessorRegistrationCaught(
          context, &options, callback, this) == 0) {
        failTerminal(AudioWorkletError::ProcessorRegistrationFailed);
        return;
    }
    emscripten_set_timeout_loop(
      &EmscriptenAudioWorklet::monitorProcessorCreation,
      monitorIntervalMilliseconds,
      this);
}

void
EmscriptenAudioWorklet::onProcessorCreated(
  EMSCRIPTEN_WEBAUDIO_T callbackContext,
  bool success) noexcept
{
    if (callbackContext != context ||
        state.load(std::memory_order_acquire) !=
          AudioWorkletLifecycleState::CreatingProcessor) {
        failTerminal(AudioWorkletError::UnexpectedAsyncCallback);
        return;
    }
    if (!success) {
        failTerminal(AudioWorkletError::ProcessorCreationFailed);
        return;
    }
    if (mirrorValue(processorFailureMirror) < 0) {
        failTerminal(AudioWorkletError::ProcessorRegistrationFailed);
        return;
    }

    const auto callback =
      reinterpret_cast<std::uintptr_t>(&EmscriptenAudioWorklet::process);
    auto outputChannelCounts = std::array<int, 1>{ 2 };
    const auto options = EmscriptenAudioWorkletNodeCreateOptions{
        .numberOfInputs = 0,
        .numberOfOutputs = 1,
        .outputChannelCounts = outputChannelCounts.data(),
    };
    node = rhythmgameCreateAudioWorkletNodeCaught(
      context, processorName, &options, callback, this);
    if (node <= 0) {
        failTerminal(AudioWorkletError::NodeCreationFailed);
        return;
    }
    if (rhythmgameInstallProcessorNodeFailureMonitor(
          node, &processorFailureMirror) == 0) {
        failTerminal(AudioWorkletError::ProcessorNodeFailureMonitorFailed);
        return;
    }
    if (rhythmgameConnectAudioNodeCaught(node, context) == 0) {
        failTerminal(AudioWorkletError::NodeConnectionFailed);
        return;
    }
    if (clock.browserContextState() !=
          BrowserAudioClock::suspendedContextState ||
        emscripten_audio_context_state(context) !=
          AUDIO_CONTEXT_STATE_SUSPENDED) {
        failTerminal(AudioWorkletError::ContextNotSuspendedBeforeReady);
        return;
    }

    auto frameZeroContextTimeNs = std::int64_t{};
    if (!secondsToNanoseconds(rhythmgameAudioContextCurrentTimeSeconds(context),
                              frameZeroContextTimeNs) ||
        !clock.setMixerFrameZeroContextTimeNs(frameZeroContextTimeNs)) {
        failTerminal(AudioWorkletError::InvalidBrowserClockSample);
        return;
    }

    auto expected = AudioWorkletLifecycleState::CreatingProcessor;
    if (!state.compare_exchange_strong(
          expected,
          AudioWorkletLifecycleState::GraphReadyUnsealed,
          std::memory_order_acq_rel,
          std::memory_order_acquire)) {
        if (expected != AudioWorkletLifecycleState::Terminal) {
            failTerminal(AudioWorkletError::UnexpectedAsyncCallback);
        }
        return;
    }
    emscripten_set_timeout_loop(&EmscriptenAudioWorklet::monitorProcessorHealth,
                                monitorIntervalMilliseconds,
                                this);
}

auto
EmscriptenAudioWorklet::pollContextSuspension() noexcept -> bool
{
    if (state.load(std::memory_order_acquire) !=
        AudioWorkletLifecycleState::SuspendingContext) {
        return false;
    }
    const auto status = mirrorValue(suspendRequestStatusMirror);
    if (status < 0) {
        failTerminal(AudioWorkletError::ContextSuspendRequestFailed);
        return false;
    }

    const auto mirroredState = clock.browserContextState();
    if (mirroredState == BrowserAudioClock::closedContextState) {
        failTerminal(AudioWorkletError::ContextClosed);
        return false;
    }
    if (mirroredState == BrowserAudioClock::interruptedContextState) {
        failTerminal(AudioWorkletError::ContextInterrupted);
        return false;
    }
    if (mirroredState != BrowserAudioClock::unknownContextState &&
        mirroredState != BrowserAudioClock::suspendedContextState &&
        mirroredState != BrowserAudioClock::runningContextState) {
        failTerminal(AudioWorkletError::UnknownContextState);
        return false;
    }

    if (status > 0 &&
        mirroredState == BrowserAudioClock::suspendedContextState &&
        emscripten_audio_context_state(context) ==
          AUDIO_CONTEXT_STATE_SUSPENDED) {
        auto expected = AudioWorkletLifecycleState::SuspendingContext;
        if (!state.compare_exchange_strong(
              expected,
              AudioWorkletLifecycleState::ContextReady,
              std::memory_order_acq_rel,
              std::memory_order_acquire) &&
            expected != AudioWorkletLifecycleState::Terminal) {
            failTerminal(AudioWorkletError::UnexpectedAsyncCallback);
        }
        return false;
    }

    const auto polls =
      contextSuspendPolls.fetch_add(1, std::memory_order_relaxed) + 1U;
    if (polls >= maximumContextSuspendPolls) {
        failTerminal(AudioWorkletError::ContextSuspendTimedOut);
        return false;
    }
    return true;
}

auto
EmscriptenAudioWorklet::pollProcessorCreation() noexcept -> bool
{
    if (state.load(std::memory_order_acquire) !=
        AudioWorkletLifecycleState::CreatingProcessor) {
        return false;
    }
    if (mirrorValue(processorFailureMirror) < 0) {
        failTerminal(AudioWorkletError::ProcessorRegistrationFailed);
        return false;
    }
    const auto polls =
      processorCreationPolls.fetch_add(1, std::memory_order_relaxed) + 1U;
    if (polls >= maximumProcessorCreationPolls) {
        failTerminal(AudioWorkletError::ProcessorCreationTimedOut);
        return false;
    }
    return true;
}

auto
EmscriptenAudioWorklet::pollProcessorHealth() noexcept -> bool
{
    if (state.load(std::memory_order_acquire) ==
        AudioWorkletLifecycleState::Terminal) {
        return false;
    }
    if (mirrorValue(processorFailureMirror) < 0) {
        failTerminal(AudioWorkletError::ProcessorRuntimeFailed);
        return false;
    }
    return true;
}

void
EmscriptenAudioWorklet::failTerminal(AudioWorkletError failure) noexcept
{
    auto expected = AudioWorkletError::None;
    (void)error.compare_exchange_strong(
      expected, failure, std::memory_order_acq_rel, std::memory_order_acquire);
    clock.markTerminal();
    state.store(AudioWorkletLifecycleState::Terminal,
                std::memory_order_release);
}

auto
EmscriptenAudioWorklet::checkSealedHeapStable() noexcept -> bool
{
    if (!heapIsSealed.load(std::memory_order_acquire)) {
        return false;
    }
    const auto currentBytes =
      static_cast<std::uint64_t>(emscripten_get_heap_size());
    observedHeapSize.store(currentBytes, std::memory_order_release);
    if (currentBytes == sealedHeapSize.load(std::memory_order_acquire)) {
        return true;
    }
    failTerminal(AudioWorkletError::HeapGrewAfterReady);
    return false;
}

auto
EmscriptenAudioWorklet::browserMonotonicNowNanoseconds(
  std::int64_t& browserMonotonicNanoseconds) const noexcept -> bool
{
    if (!emscripten_is_main_runtime_thread()) {
        return false;
    }
    return millisecondsToNanoseconds(
      rhythmgameRawBrowserPerformanceNowMilliseconds(),
      browserMonotonicNanoseconds);
}

auto
EmscriptenAudioWorklet::readOutputTimestamp(
  BrowserOutputTimestamp& timestamp) const noexcept -> std::int32_t
{
    auto contextTimeSeconds = double{};
    auto performanceTimeMilliseconds = double{};
    const auto status = rhythmgameReadAudioOutputTimestamp(
      context, &contextTimeSeconds, &performanceTimeMilliseconds);
    if (status <= 0) {
        return status;
    }
    if (!secondsToNanoseconds(contextTimeSeconds, timestamp.contextTimeNs) ||
        !millisecondsToNanoseconds(performanceTimeMilliseconds,
                                   timestamp.performanceTimeNs) ||
        timestamp.contextTimeNs <= 0 || timestamp.performanceTimeNs <= 0) {
        return -1;
    }
    return 1;
}

auto
EmscriptenAudioWorklet::mirrorValue(const std::int32_t& value) const noexcept
  -> std::int32_t
{
    auto mirror =
      std::atomic_ref<std::int32_t>{ const_cast<std::int32_t&>(value) };
    return mirror.load(std::memory_order_acquire);
}

} // namespace web_playtest
