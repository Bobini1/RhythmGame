#include "BrowserAudioClock.h"

#include <limits>

namespace web_playtest {
namespace {

constexpr auto minimumSampleRate = std::uint32_t{ 8'000 };
constexpr auto maximumSampleRate = std::uint32_t{ 192'000 };
constexpr auto nanosecondsPerSecond = std::uint64_t{ 1'000'000'000 };
constexpr auto nanosecondsPerMicrosecond = std::int64_t{ 1'000 };

[[nodiscard]] auto
sameCursor(const BrowserAudioClock::RenderCursor& left,
           const BrowserAudioClock::RenderCursor& right) noexcept -> bool
{
    return left.revision == right.revision &&
           left.renderedFrames == right.renderedFrames &&
           left.completedQuantumSequence ==
             right.completedQuantumSequence &&
           left.lastQuantumFrames == right.lastQuantumFrames &&
           (left.revision & 1U) == 0U;
}

} // namespace

BrowserAudioClock::BrowserAudioClock(
  std::uint32_t rate,
  std::uint32_t expectedRenderQuantumFrames) noexcept
{
    (void)configure(rate, expectedRenderQuantumFrames);
}

auto
BrowserAudioClock::configure(
  std::uint32_t rate,
  std::uint32_t expectedRenderQuantumFrames) noexcept -> bool
{
    if (rate < minimumSampleRate || rate > maximumSampleRate ||
        expectedRenderQuantumFrames == 0) {
        return false;
    }

    auto expected = std::uint32_t{};
    if (!configuredSampleRate.compare_exchange_strong(
          expected,
          rate,
          std::memory_order_release,
          std::memory_order_acquire) &&
        expected != rate) {
        return false;
    }

    expected = 0;
    return configuredRenderQuantumFrames.compare_exchange_strong(
             expected,
             expectedRenderQuantumFrames,
             std::memory_order_release,
             std::memory_order_acquire) ||
           expected == expectedRenderQuantumFrames;
}

auto
BrowserAudioClock::setMixerFrameZeroContextTimeNs(
  std::int64_t contextTimeNs) noexcept -> bool
{
    if (contextTimeNs < 0) {
        return false;
    }
    auto expected = std::int64_t{ -1 };
    return mixerFrameZeroContextTimeNs.compare_exchange_strong(
             expected,
             contextTimeNs,
             std::memory_order_release,
             std::memory_order_acquire) ||
           expected == contextTimeNs;
}

auto
BrowserAudioClock::sampleRate() const noexcept -> std::uint32_t
{
    return configuredSampleRate.load(std::memory_order_acquire);
}

auto
BrowserAudioClock::browserContextStateAddressForBridge() noexcept
  -> std::int32_t*
{
    return &browserContextStateMirror;
}

auto
BrowserAudioClock::browserContextNonRunningRevisionAddressForBridge() noexcept
  -> std::int32_t*
{
    return &browserContextNonRunningRevisionMirror;
}

auto
BrowserAudioClock::browserContextState() const noexcept -> std::int32_t
{
    auto state = std::atomic_ref<std::int32_t>{ browserContextStateMirror };
    return state.load(std::memory_order_acquire);
}

auto
BrowserAudioClock::contextIsRunning() const noexcept -> bool
{
    return browserContextState() == runningContextState;
}

void
BrowserAudioClock::publishBrowserContextStateForTesting(
  std::int32_t state,
  std::int32_t nonRunningRevision) noexcept
{
    auto revision = std::atomic_ref<std::int32_t>{
        browserContextNonRunningRevisionMirror
    };
    auto stateValue =
      std::atomic_ref<std::int32_t>{ browserContextStateMirror };
    revision.store(nonRunningRevision, std::memory_order_release);
    stateValue.store(state, std::memory_order_release);
}

auto
BrowserAudioClock::arm(
  std::uint64_t sessionGeneration,
  std::uint64_t countdownFrames,
  BrowserOutputTimestamp outputTimestampBeforeResume) noexcept -> bool
{
    // Latest-attempt-wins: no validation failure can leave a prior arm or
    // mapping available for a later resume quantum.
    const auto pendingGeneration =
      isArmed.load(std::memory_order_acquire)
        ? armedGeneration.load(std::memory_order_acquire)
        : std::uint64_t{};
    isArmed.store(false, std::memory_order_release);
    publishAnchor({});

    const auto anchoredGeneration =
      lastAnchoredGeneration.load(std::memory_order_acquire);
    const auto contextState = browserContextState();
    const auto outputTimestampIsZero =
      outputTimestampBeforeResume.contextTimeNs == 0 &&
      outputTimestampBeforeResume.performanceTimeNs == 0;
    const auto outputTimestampIsPositive =
      outputTimestampBeforeResume.contextTimeNs > 0 &&
      outputTimestampBeforeResume.performanceTimeNs > 0;
    if (mappingTerminated.load(std::memory_order_acquire) ||
        sessionGeneration == 0 || sampleRate() == 0 ||
        sessionGeneration <= anchoredGeneration ||
        (pendingGeneration != 0 &&
         sessionGeneration < pendingGeneration) ||
        (contextState != suspendedContextState &&
         contextState != runningContextState) ||
        (!outputTimestampIsZero && !outputTimestampIsPositive)) {
        return false;
    }

    auto baseline = RenderCursor{};
    if (!tryReadRenderCursor(baseline)) {
        return false;
    }

    auto requiredLeadFrames = std::uint64_t{};
    if (!minimumLeadFrames(baseline, requiredLeadFrames) ||
        countdownFrames < requiredLeadFrames) {
        return false;
    }

    const auto contextRevision = currentContextNonRunningRevision();
    armedBaselineRenderedFrames.store(baseline.renderedFrames,
                                      std::memory_order_relaxed);
    armedBaselineSequence.store(baseline.completedQuantumSequence,
                                std::memory_order_relaxed);
    armedCountdownFrames.store(countdownFrames, std::memory_order_relaxed);
    armedGeneration.store(sessionGeneration, std::memory_order_relaxed);
    armedOutputContextTimeNs.store(
      outputTimestampBeforeResume.contextTimeNs,
      std::memory_order_relaxed);
    armedOutputPerformanceTimeNs.store(
      outputTimestampBeforeResume.performanceTimeNs,
      std::memory_order_relaxed);
    armedContextNonRunningRevision.store(contextRevision,
                                         std::memory_order_relaxed);
    isArmed.store(true, std::memory_order_release);
    if (mappingTerminated.load(std::memory_order_acquire) ||
        currentContextNonRunningRevision() != contextRevision) {
        isArmed.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void
BrowserAudioClock::invalidate() noexcept
{
    isArmed.store(false, std::memory_order_release);
    publishAnchor({});
}

void
BrowserAudioClock::markTerminal() noexcept
{
    mappingTerminated.store(true, std::memory_order_release);
    isArmed.store(false, std::memory_order_release);
}

void
BrowserAudioClock::beginRenderQuantum() noexcept
{
    cursorRevision.fetch_add(1, std::memory_order_acq_rel);
}

void
BrowserAudioClock::finishRenderQuantum(std::uint64_t renderedFrames,
                                       std::uint32_t frameCount) noexcept
{
    cursorRenderedFrames.store(renderedFrames, std::memory_order_relaxed);
    lastQuantumFrames.store(frameCount, std::memory_order_relaxed);
    completedQuantumSequence.fetch_add(1, std::memory_order_relaxed);
    cursorRevision.fetch_add(1, std::memory_order_release);
}

auto
BrowserAudioClock::tryReadRenderCursor(RenderCursor& cursor) const noexcept
  -> bool
{
    for (auto attempt = std::size_t{}; attempt < maximumSnapshotAttempts;
         ++attempt) {
        const auto before = cursorRevision.load(std::memory_order_acquire);
        if ((before & 1U) != 0U) {
            continue;
        }

        const auto candidate = RenderCursor{
            .revision = before,
            .renderedFrames =
              cursorRenderedFrames.load(std::memory_order_relaxed),
            .completedQuantumSequence =
              completedQuantumSequence.load(std::memory_order_relaxed),
            .lastQuantumFrames =
              lastQuantumFrames.load(std::memory_order_relaxed),
        };
        const auto after = cursorRevision.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0U) {
            cursor = candidate;
            return true;
        }
    }
    return false;
}

auto
BrowserAudioClock::tryEstablishAnchor(
  const RenderCursor& beforeBrowserSample,
  const BrowserOutputTimestamp& outputTimestamp,
  const RenderCursor& afterBrowserSample,
  BrowserAudioAnchor& anchor) noexcept -> bool
{
    const auto contextRevision =
      armedContextNonRunningRevision.load(std::memory_order_relaxed);
    if (mappingTerminated.load(std::memory_order_acquire) ||
        !isArmed.load(std::memory_order_acquire) ||
        outputTimestamp.contextTimeNs <= 0 ||
        outputTimestamp.performanceTimeNs <= 0 ||
        !sameCursor(beforeBrowserSample, afterBrowserSample) ||
        !contextMatchesRevision(contextRevision)) {
        return false;
    }

    const auto sessionGeneration =
      armedGeneration.load(std::memory_order_relaxed);
    const auto baselineSequence =
      armedBaselineSequence.load(std::memory_order_relaxed);
    const auto baselineRenderedFrames =
      armedBaselineRenderedFrames.load(std::memory_order_relaxed);
    const auto countdownFrames =
      armedCountdownFrames.load(std::memory_order_relaxed);
    const auto outputContextTimeBeforeResume =
      armedOutputContextTimeNs.load(std::memory_order_relaxed);
    const auto outputPerformanceTimeBeforeResume =
      armedOutputPerformanceTimeNs.load(std::memory_order_relaxed);
    const auto rate = sampleRate();
    const auto frameZeroContextTime =
      mixerFrameZeroContextTimeNs.load(std::memory_order_acquire);

    if (sessionGeneration == 0 || rate == 0 || frameZeroContextTime < 0 ||
        outputTimestamp.contextTimeNs <= outputContextTimeBeforeResume ||
        outputTimestamp.performanceTimeNs <=
          outputPerformanceTimeBeforeResume ||
        afterBrowserSample.completedQuantumSequence <= baselineSequence ||
        afterBrowserSample.renderedFrames <= baselineRenderedFrames ||
        afterBrowserSample.lastQuantumFrames == 0 ||
        countdownFrames >
          (std::numeric_limits<std::uint64_t>::max)() -
            afterBrowserSample.renderedFrames) {
        return false;
    }

    const auto chartStartFrame =
      afterBrowserSample.renderedFrames + countdownFrames;
    auto chartStartFrameOffsetNs = std::int64_t{};
    auto chartStartContextTimeNs = std::int64_t{};
    auto contextDeltaNs = std::int64_t{};
    auto chartStartBrowserMonotonicNs = std::int64_t{};
    if (!framesToNanoseconds(
          chartStartFrame, rate, chartStartFrameOffsetNs) ||
        !checkedAdd(frameZeroContextTime,
                    chartStartFrameOffsetNs,
                    chartStartContextTimeNs) ||
        !checkedSubtract(chartStartContextTimeNs,
                         outputTimestamp.contextTimeNs,
                         contextDeltaNs) ||
        !checkedAdd(outputTimestamp.performanceTimeNs,
                    contextDeltaNs,
                    chartStartBrowserMonotonicNs) ||
        chartStartBrowserMonotonicNs < 0) {
        return false;
    }

    // A stable timestamp bracket is not enough if the main thread paused
    // before commit. Re-read the live graph cursor and retain the complete
    // handoff margin at publication time.
    auto liveCursor = RenderCursor{};
    auto requiredLeadFrames = std::uint64_t{};
    if (!tryReadRenderCursor(liveCursor) ||
        !minimumLeadFrames(liveCursor, requiredLeadFrames) ||
        liveCursor.renderedFrames > chartStartFrame ||
        chartStartFrame - liveCursor.renderedFrames < requiredLeadFrames ||
        !contextMatchesRevision(contextRevision)) {
        return false;
    }

    const auto candidate = BrowserAudioAnchor{
        .sessionGeneration = sessionGeneration,
        .chartStartFrame = chartStartFrame,
        .chartStartBrowserMonotonicNs =
          chartStartBrowserMonotonicNs,
        .chartStartContextTimeNs = chartStartContextTimeNs,
        .sampleRate = rate,
        .contextNonRunningRevision = contextRevision,
    };

    publishAnchor(candidate);
    if (mappingTerminated.load(std::memory_order_acquire) ||
        !contextMatchesRevision(contextRevision)) {
        isArmed.store(false, std::memory_order_release);
        return false;
    }
    lastAnchoredGeneration.store(sessionGeneration, std::memory_order_release);
    isArmed.store(false, std::memory_order_release);
    anchor = candidate;
    return true;
}

auto
BrowserAudioClock::tryReadAnchor(BrowserAudioAnchor& anchor) const noexcept
  -> bool
{
    if (mappingTerminated.load(std::memory_order_acquire) ||
        !contextIsRunning()) {
        return false;
    }
    for (auto attempt = std::size_t{}; attempt < maximumSnapshotAttempts;
         ++attempt) {
        const auto before = anchorRevision.load(std::memory_order_acquire);
        if ((before & 1U) != 0U) {
            continue;
        }

        const auto candidate = BrowserAudioAnchor{
            .sessionGeneration =
              anchorGeneration.load(std::memory_order_relaxed),
            .chartStartFrame =
              anchorChartStartFrame.load(std::memory_order_relaxed),
            .chartStartBrowserMonotonicNs =
              anchorBrowserMonotonicNs.load(std::memory_order_relaxed),
            .chartStartContextTimeNs =
              anchorContextTimeNs.load(std::memory_order_relaxed),
            .sampleRate =
              anchorSampleRate.load(std::memory_order_relaxed),
            .contextNonRunningRevision =
              anchorContextNonRunningRevision.load(
                std::memory_order_relaxed),
        };
        const auto after = anchorRevision.load(std::memory_order_acquire);
        if (before == after && (after & 1U) == 0U &&
            !mappingTerminated.load(std::memory_order_acquire) &&
            contextMatchesRevision(
              candidate.contextNonRunningRevision) &&
            candidate.sessionGeneration != 0 && candidate.sampleRate != 0) {
            anchor = candidate;
            return true;
        }
    }
    return false;
}

auto
BrowserAudioClock::chartTimeForBrowserEventUs(
  std::int64_t browserEventMonotonicUs,
  std::int64_t& chartTimeNanoseconds) const noexcept -> bool
{
    if (browserEventMonotonicUs >
          (std::numeric_limits<std::int64_t>::max)() /
            nanosecondsPerMicrosecond ||
        browserEventMonotonicUs <
          (std::numeric_limits<std::int64_t>::min)() /
            nanosecondsPerMicrosecond) {
        return false;
    }

    const auto browserEventMonotonicNs =
      browserEventMonotonicUs * nanosecondsPerMicrosecond;
    return chartTimeForBrowserMonotonicNs(
      browserEventMonotonicNs, chartTimeNanoseconds);
}

auto
BrowserAudioClock::chartTimeForBrowserMonotonicNs(
  std::int64_t browserMonotonicNs,
  std::int64_t& chartTimeNanoseconds) const noexcept -> bool
{
    auto anchor = BrowserAudioAnchor{};
    return tryReadAnchor(anchor) &&
           checkedSubtract(browserMonotonicNs,
                           anchor.chartStartBrowserMonotonicNs,
                           chartTimeNanoseconds);
}

auto
BrowserAudioClock::chartTimeForRenderedFrame(
  std::uint64_t renderedFrame,
  std::int64_t& chartTimeNanoseconds) const noexcept -> bool
{
    auto anchor = BrowserAudioAnchor{};
    if (!tryReadAnchor(anchor)) {
        return false;
    }

    const auto afterChartStart = renderedFrame >= anchor.chartStartFrame;
    const auto deltaFrames =
      afterChartStart ? renderedFrame - anchor.chartStartFrame
                      : anchor.chartStartFrame - renderedFrame;
    auto magnitudeNanoseconds = std::int64_t{};
    if (!framesToNanoseconds(
          deltaFrames, anchor.sampleRate, magnitudeNanoseconds)) {
        return false;
    }

    chartTimeNanoseconds =
      afterChartStart ? magnitudeNanoseconds : -magnitudeNanoseconds;
    return true;
}

auto
BrowserAudioClock::framesToNanoseconds(
  std::uint64_t frames,
  std::uint32_t rate,
  std::int64_t& nanoseconds) noexcept -> bool
{
    if (rate == 0) {
        return false;
    }

    const auto wholeSeconds = frames / rate;
    const auto remainderFrames = frames % rate;
    const auto maximumNanoseconds =
      static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)());
    if (wholeSeconds > maximumNanoseconds / nanosecondsPerSecond) {
        return false;
    }

    const auto wholeNanoseconds = wholeSeconds * nanosecondsPerSecond;
    const auto roundedRemainder =
      (remainderFrames * nanosecondsPerSecond + rate / 2U) / rate;
    if (roundedRemainder > maximumNanoseconds - wholeNanoseconds) {
        return false;
    }

    nanoseconds =
      static_cast<std::int64_t>(wholeNanoseconds + roundedRemainder);
    return true;
}

auto
BrowserAudioClock::checkedAdd(std::int64_t left,
                              std::int64_t right,
                              std::int64_t& result) noexcept -> bool
{
    if ((right > 0 &&
         left > (std::numeric_limits<std::int64_t>::max)() - right) ||
        (right < 0 &&
         left < (std::numeric_limits<std::int64_t>::min)() - right)) {
        return false;
    }
    result = left + right;
    return true;
}

auto
BrowserAudioClock::checkedSubtract(
  std::int64_t left,
  std::int64_t right,
  std::int64_t& result) noexcept -> bool
{
    if ((right > 0 &&
         left < (std::numeric_limits<std::int64_t>::min)() + right) ||
        (right < 0 &&
         left > (std::numeric_limits<std::int64_t>::max)() + right)) {
        return false;
    }
    result = left - right;
    return true;
}

auto
BrowserAudioClock::currentContextNonRunningRevision() const noexcept
  -> std::int32_t
{
    auto revision = std::atomic_ref<std::int32_t>{
        browserContextNonRunningRevisionMirror
    };
    return revision.load(std::memory_order_acquire);
}

auto
BrowserAudioClock::contextMatchesRevision(
  std::int32_t nonRunningRevision) const noexcept -> bool
{
    return contextIsRunning() &&
           currentContextNonRunningRevision() == nonRunningRevision;
}

auto
BrowserAudioClock::minimumLeadFrames(
  const RenderCursor& cursor,
  std::uint64_t& frames) const noexcept -> bool
{
    const auto configuredQuantum =
      configuredRenderQuantumFrames.load(std::memory_order_acquire);
    const auto observedQuantum =
      cursor.lastQuantumFrames > configuredQuantum
        ? cursor.lastQuantumFrames
        : configuredQuantum;
    if (observedQuantum == 0) {
        return false;
    }
    frames = static_cast<std::uint64_t>(observedQuantum) *
             minimumCountdownQuanta;
    return true;
}

void
BrowserAudioClock::publishAnchor(const BrowserAudioAnchor& anchor) noexcept
{
    anchorRevision.fetch_add(1, std::memory_order_acq_rel);
    anchorGeneration.store(anchor.sessionGeneration, std::memory_order_relaxed);
    anchorChartStartFrame.store(anchor.chartStartFrame,
                                std::memory_order_relaxed);
    anchorBrowserMonotonicNs.store(
      anchor.chartStartBrowserMonotonicNs, std::memory_order_relaxed);
    anchorContextTimeNs.store(anchor.chartStartContextTimeNs,
                              std::memory_order_relaxed);
    anchorSampleRate.store(anchor.sampleRate, std::memory_order_relaxed);
    anchorContextNonRunningRevision.store(
      anchor.contextNonRunningRevision, std::memory_order_relaxed);
    anchorRevision.fetch_add(1, std::memory_order_release);
}

} // namespace web_playtest
