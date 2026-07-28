#pragma once

#include "BrowserAudioClock.h"
#include "RealtimeMixer.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

#if defined(__EMSCRIPTEN__)
#include <emscripten/webaudio.h>
#endif

namespace web_playtest {

#if defined(__EMSCRIPTEN__)

enum class AudioWorkletLifecycleState : std::uint8_t
{
    Constructed,
    SuspendingContext,
    ContextReady,
    StartingWorklet,
    CreatingProcessor,
    GraphReadyUnsealed,
    Ready,
    ResumeRequested,
    Unanchored,
    Anchored,
    Terminal
};

enum class AudioWorkletError : std::uint8_t
{
    None,
    InvalidLifecycle,
    ContextCreationFailed,
    ContextStateBridgeFailed,
    ContextSuspendRequestFailed,
    ContextSuspendTimedOut,
    ContextNotSuspendedBeforeReady,
    InvalidSampleRate,
    InvalidQuantumSize,
    InvalidSoundBank,
    MixerCreationFailed,
    WorkletThreadStartFailed,
    ProcessorFailureMonitorFailed,
    ProcessorNodeFailureMonitorFailed,
    ProcessorRegistrationFailed,
    ProcessorCreationTimedOut,
    ProcessorCreationFailed,
    ProcessorRuntimeFailed,
    NodeCreationFailed,
    NodeConnectionFailed,
    UnexpectedAsyncCallback,
    ContextClosed,
    ContextInterrupted,
    UnknownContextState,
    InvalidOutputTimestamp,
    InvalidBrowserClockSample,
    InvalidOutputShape,
    TransportTerminalFailure,
    HeapGrewAfterReady,
    NotMainRuntimeThread
};

struct AudioWorkletTelemetrySnapshot
{
    std::uint64_t renderedFrames = {};
    std::uint64_t observedHeapBytes = {};
    std::uint64_t sealedHeapBytes = {};
    bool heapSealed = {};

    // Optional Chromium telemetry is explicitly capability-marked and never
    // part of readiness. Task 8 owns acknowledgement-draining telemetry.
    bool playbackStatsAvailable = {};
    std::uint64_t underrunCount = {};
    bool renderDurationAvailable = {};
    std::uint64_t lastRenderDurationNanoseconds = {};
};

class EmscriptenAudioWorklet final
{
  public:
    static constexpr auto workletStackBytes = std::size_t{ 128 * 1024 };

    // The complete callback-reachable audio core is owned by this one
    // page-lifetime instance. No caller-owned bank, transport, mixer, stack,
    // or userData address is retained.
    [[nodiscard]] static auto createProcessLifetime() noexcept
      -> EmscriptenAudioWorklet*;

    EmscriptenAudioWorklet(const EmscriptenAudioWorklet&) = delete;
    auto operator=(const EmscriptenAudioWorklet&)
      -> EmscriptenAudioWorklet& = delete;
    EmscriptenAudioWorklet(EmscriptenAudioWorklet&&) = delete;
    auto operator=(EmscriptenAudioWorklet&&)
      -> EmscriptenAudioWorklet& = delete;

    // Starts caught AudioContext construction and an explicit suspend request.
    // contextReadyForDecode() becomes true only after the promise and browser
    // state mirror both confirm Suspended.
    [[nodiscard]] auto createContextForDecode() noexcept -> bool;
    [[nodiscard]] auto contextReadyForDecode() const noexcept -> bool;

    // Ownership of the frozen bank moves into page-lifetime storage before the
    // mixer is constructed in-place around the owned transport.
    [[nodiscard]] auto initializeWorklet(
      PcmSoundBank&& frozenSoundBank,
      RealtimeMixer::Config mixerConfig) noexcept -> bool;
    [[nodiscard]] auto transport() noexcept -> AudioTransport&;

    // Called by the runtime only after all worker/input/report/QML storage has
    // been preallocated. Start eligibility begins here, not at node creation.
    [[nodiscard]] auto sealReadyHeap() noexcept -> bool;
    [[nodiscard]] auto verifySealedHeapOnMainThread() noexcept -> bool;

    // Must be called directly from the trusted browser gesture stack. Every
    // attempt invalidates older mappings/arms even when the new arm fails.
    [[nodiscard]] auto resumeFromTrustedGesture(
      std::uint64_t sessionGeneration,
      std::uint64_t countdownFrames) noexcept -> bool;

    [[nodiscard]] auto pollForAnchor(BrowserAudioAnchor& anchor) noexcept
      -> bool;
    [[nodiscard]] auto currentAudibleChartTime(
      std::int64_t& chartTimeNanoseconds) noexcept -> bool;

    [[nodiscard]] auto lifecycleState() const noexcept
      -> AudioWorkletLifecycleState;
    [[nodiscard]] auto terminalError() const noexcept -> AudioWorkletError;
    [[nodiscard]] auto outputSampleRate() const noexcept -> std::uint32_t;
    [[nodiscard]] auto renderQuantumFrames() const noexcept -> std::uint32_t;
    [[nodiscard]] auto readyForTrustedResume() const noexcept -> bool;
    [[nodiscard]] auto audioClock() const noexcept
      -> const BrowserAudioClock&;

    // Purely atomic and worker-safe. Heap sampling/enforcement is performed by
    // sealReadyHeap(), poll/main verification, and every render callback.
    [[nodiscard]] auto telemetry() const noexcept
      -> AudioWorkletTelemetrySnapshot;

  private:
    EmscriptenAudioWorklet() noexcept = default;
    ~EmscriptenAudioWorklet() = default;

    static void workletThreadStarted(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                     bool success,
                                     void* userData) noexcept;
    static void processorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext,
                                 bool success,
                                 void* userData) noexcept;
    static bool monitorContextSuspension(double,
                                         void* userData) noexcept;
    static bool monitorProcessorCreation(double,
                                         void* userData) noexcept;
    static bool monitorProcessorHealth(double,
                                       void* userData) noexcept;
    static bool process(int numInputs,
                        const AudioSampleFrame* inputs,
                        int numOutputs,
                        AudioSampleFrame* outputs,
                        int numParams,
                        const AudioParamFrame* params,
                        void* userData) noexcept;

    void onWorkletThreadStarted(EMSCRIPTEN_WEBAUDIO_T callbackContext,
                                bool success) noexcept;
    void onProcessorCreated(EMSCRIPTEN_WEBAUDIO_T callbackContext,
                            bool success) noexcept;
    [[nodiscard]] auto pollContextSuspension() noexcept -> bool;
    [[nodiscard]] auto pollProcessorCreation() noexcept -> bool;
    [[nodiscard]] auto pollProcessorHealth() noexcept -> bool;
    void failTerminal(AudioWorkletError failure) noexcept;
    [[nodiscard]] auto checkSealedHeapStable() noexcept -> bool;
    [[nodiscard]] auto browserMonotonicNowNanoseconds(
      std::int64_t& browserMonotonicNanoseconds) const noexcept -> bool;
    [[nodiscard]] auto readOutputTimestamp(
      BrowserOutputTimestamp& timestamp) const noexcept -> std::int32_t;
    [[nodiscard]] auto mirrorValue(const std::int32_t& value) const noexcept
      -> std::int32_t;

    static constexpr char processorName[] =
      "rhythmgame-realtime-mixer";
    static constexpr auto monitorIntervalMilliseconds = double{ 10.0 };
    static constexpr auto maximumContextSuspendPolls = std::uint32_t{ 500 };
    static constexpr auto maximumProcessorCreationPolls =
      std::uint32_t{ 500 };

    alignas(16) std::array<std::byte, workletStackBytes> workletStack = {};
    BrowserAudioClock clock;
    AudioTransport ownedTransport;
    std::optional<PcmSoundBank> ownedSoundBank;
    std::optional<RealtimeMixer> ownedMixer;
    EMSCRIPTEN_WEBAUDIO_T context = {};
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T node = {};
    std::atomic<AudioWorkletLifecycleState> state{
        AudioWorkletLifecycleState::Constructed
    };
    std::atomic<AudioWorkletError> error{ AudioWorkletError::None };
    std::atomic<std::uint32_t> actualSampleRate = {};
    std::atomic<std::uint32_t> quantumFrames = {};
    std::atomic<std::uint64_t> renderedFrames = {};

    alignas(4) mutable std::int32_t suspendRequestStatusMirror = {};
    alignas(4) mutable std::int32_t processorFailureMirror = {};
    std::atomic<std::uint32_t> contextSuspendPolls = {};
    std::atomic<std::uint32_t> processorCreationPolls = {};

    std::atomic_bool heapIsSealed = {};
    std::atomic<std::uint64_t> sealedHeapSize = {};
    std::atomic<std::uint64_t> observedHeapSize = {};

    // Explicitly unsupported in this slice; availability remains false.
    std::atomic_bool playbackStatsAvailable = {};
    std::atomic<std::uint64_t> underruns = {};
    std::atomic_bool renderDurationAvailable = {};
    std::atomic<std::uint64_t> lastRenderDurationNanoseconds = {};
};

static_assert(EmscriptenAudioWorklet::workletStackBytes % 16U == 0U);
static_assert(EmscriptenAudioWorklet::workletStackBytes <=
              (std::numeric_limits<std::uint32_t>::max)());
static_assert(std::atomic<AudioWorkletLifecycleState>::is_always_lock_free);
static_assert(std::atomic<AudioWorkletError>::is_always_lock_free);
static_assert(std::atomic_ref<std::int32_t>::is_always_lock_free);

#endif

} // namespace web_playtest
