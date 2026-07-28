#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace web_playtest {

struct BrowserOutputTimestamp
{
    std::int64_t contextTimeNs = {};
    std::int64_t performanceTimeNs = {};
};

struct BrowserAudioAnchor
{
    std::uint64_t sessionGeneration = {};
    std::uint64_t chartStartFrame = {};
    std::int64_t chartStartBrowserMonotonicNs = {};
    std::int64_t chartStartContextTimeNs = {};
    std::uint32_t sampleRate = {};
    std::int32_t contextNonRunningRevision = {};
};

class BrowserAudioClock final
{
  public:
    struct RenderCursor
    {
        std::uint64_t revision = {};
        std::uint64_t renderedFrames = {};
        std::uint64_t completedQuantumSequence = {};
        std::uint32_t lastQuantumFrames = {};
    };

    static constexpr auto unknownContextState = std::int32_t{ -1 };
    static constexpr auto suspendedContextState = std::int32_t{ 0 };
    static constexpr auto runningContextState = std::int32_t{ 1 };
    static constexpr auto closedContextState = std::int32_t{ 2 };
    static constexpr auto interruptedContextState = std::int32_t{ 3 };
    static constexpr auto maximumSnapshotAttempts = std::size_t{ 8 };
    static constexpr auto minimumCountdownQuanta = std::uint32_t{ 8 };

    BrowserAudioClock() noexcept = default;
    explicit BrowserAudioClock(
      std::uint32_t sampleRate,
      std::uint32_t expectedRenderQuantumFrames = 1) noexcept;
    BrowserAudioClock(const BrowserAudioClock&) = delete;
    auto operator=(const BrowserAudioClock&) -> BrowserAudioClock& = delete;
    BrowserAudioClock(BrowserAudioClock&&) = delete;
    auto operator=(BrowserAudioClock&&) -> BrowserAudioClock& = delete;

    [[nodiscard]] auto configure(
      std::uint32_t sampleRate,
      std::uint32_t expectedRenderQuantumFrames = 1) noexcept -> bool;
    [[nodiscard]] auto setMixerFrameZeroContextTimeNs(
      std::int64_t contextTimeNs) noexcept -> bool;
    [[nodiscard]] auto sampleRate() const noexcept -> std::uint32_t;

    // These two stable addresses are published to a page-lifetime JS
    // AudioContext statechange bridge. JS updates them with Atomics operations
    // in the shared Wasm memory.
    [[nodiscard]] auto browserContextStateAddressForBridge() noexcept
      -> std::int32_t*;
    [[nodiscard]] auto
    browserContextNonRunningRevisionAddressForBridge() noexcept
      -> std::int32_t*;
    [[nodiscard]] auto browserContextState() const noexcept -> std::int32_t;
    [[nodiscard]] auto contextIsRunning() const noexcept -> bool;
    void publishBrowserContextStateForTesting(
      std::int32_t state,
      std::int32_t nonRunningRevision) noexcept;

    // Every arm attempt first cancels any older pending arm and invalidates the
    // prior mapping. A failed latest click can therefore never revive an older
    // generation after resume.
    [[nodiscard]] auto arm(
      std::uint64_t sessionGeneration,
      std::uint64_t countdownFrames,
      BrowserOutputTimestamp outputTimestampBeforeResume = {}) noexcept
      -> bool;
    void invalidate() noexcept;

    // Permanent, lock-free invalidation for callback-side terminal failures.
    // It intentionally does not write the main-thread-owned anchor seqlock.
    void markTerminal() noexcept;

    // The odd cursor revision spans the complete mixer render, not merely the
    // publication stores. This prevents a browser timestamp sample from
    // bisecting an in-flight quantum.
    void beginRenderQuantum() noexcept;
    void finishRenderQuantum(std::uint64_t renderedFrames,
                             std::uint32_t frameCount) noexcept;

    [[nodiscard]] auto tryReadRenderCursor(RenderCursor& cursor) const noexcept
      -> bool;
    [[nodiscard]] auto tryEstablishAnchor(
      const RenderCursor& beforeBrowserSample,
      const BrowserOutputTimestamp& outputTimestamp,
      const RenderCursor& afterBrowserSample,
      BrowserAudioAnchor& anchor) noexcept -> bool;
    [[nodiscard]] auto tryReadAnchor(BrowserAudioAnchor& anchor) const noexcept
      -> bool;
    [[nodiscard]] auto chartTimeForBrowserEventUs(
      std::int64_t browserEventMonotonicUs,
      std::int64_t& chartTimeNanoseconds) const noexcept -> bool;
    [[nodiscard]] auto chartTimeForBrowserMonotonicNs(
      std::int64_t browserMonotonicNs,
      std::int64_t& chartTimeNanoseconds) const noexcept -> bool;
    [[nodiscard]] auto chartTimeForRenderedFrame(
      std::uint64_t renderedFrame,
      std::int64_t& chartTimeNanoseconds) const noexcept -> bool;

  private:
    [[nodiscard]] static auto framesToNanoseconds(
      std::uint64_t frames,
      std::uint32_t sampleRate,
      std::int64_t& nanoseconds) noexcept -> bool;
    [[nodiscard]] static auto checkedAdd(std::int64_t left,
                                         std::int64_t right,
                                         std::int64_t& result) noexcept
      -> bool;
    [[nodiscard]] static auto checkedSubtract(
      std::int64_t left,
      std::int64_t right,
      std::int64_t& result) noexcept -> bool;
    [[nodiscard]] auto currentContextNonRunningRevision() const noexcept
      -> std::int32_t;
    [[nodiscard]] auto contextMatchesRevision(
      std::int32_t nonRunningRevision) const noexcept -> bool;
    [[nodiscard]] auto minimumLeadFrames(
      const RenderCursor& cursor,
      std::uint64_t& frames) const noexcept -> bool;
    void publishAnchor(const BrowserAudioAnchor& anchor) noexcept;

    std::atomic<std::uint32_t> configuredSampleRate = {};
    std::atomic<std::uint32_t> configuredRenderQuantumFrames = {};
    std::atomic<std::int64_t> mixerFrameZeroContextTimeNs = { -1 };

    // Mutable permits standard atomic_ref loads in const mapping methods. The
    // JS bridge owns the writes after installation.
    alignas(4) mutable std::int32_t browserContextStateMirror =
      unknownContextState;
    alignas(4) mutable std::int32_t
      browserContextNonRunningRevisionMirror = {};

    mutable std::atomic<std::uint64_t> cursorRevision = {};
    std::atomic<std::uint64_t> cursorRenderedFrames = {};
    std::atomic<std::uint64_t> completedQuantumSequence = {};
    std::atomic<std::uint32_t> lastQuantumFrames = {};

    std::atomic_bool isArmed = {};
    std::atomic_bool mappingTerminated = {};
    std::atomic<std::uint64_t> armedGeneration = {};
    std::atomic<std::uint64_t> armedCountdownFrames = {};
    std::atomic<std::uint64_t> armedBaselineSequence = {};
    std::atomic<std::uint64_t> armedBaselineRenderedFrames = {};
    std::atomic<std::int64_t> armedOutputContextTimeNs = {};
    std::atomic<std::int64_t> armedOutputPerformanceTimeNs = {};
    std::atomic<std::int32_t> armedContextNonRunningRevision = {};
    std::atomic<std::uint64_t> lastAnchoredGeneration = {};

    mutable std::atomic<std::uint64_t> anchorRevision = {};
    std::atomic<std::uint64_t> anchorGeneration = {};
    std::atomic<std::uint64_t> anchorChartStartFrame = {};
    std::atomic<std::int64_t> anchorBrowserMonotonicNs = {};
    std::atomic<std::int64_t> anchorContextTimeNs = {};
    std::atomic<std::uint32_t> anchorSampleRate = {};
    std::atomic<std::int32_t> anchorContextNonRunningRevision = {};
};

static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<std::int64_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
static_assert(std::atomic<std::int32_t>::is_always_lock_free);
static_assert(std::atomic_ref<std::int32_t>::is_always_lock_free);
static_assert(std::atomic_bool::is_always_lock_free);

} // namespace web_playtest
