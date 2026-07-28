#include "web_playtest/audio/BrowserAudioClock.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <thread>

namespace {

using web_playtest::BrowserAudioAnchor;
using web_playtest::BrowserAudioClock;
using web_playtest::BrowserOutputTimestamp;

constexpr auto quantumFrames = std::uint32_t{ 128 };
constexpr auto minimumCountdownFrames = std::uint64_t{ 1'024 };

void
prepare(BrowserAudioClock& clock,
        std::int64_t frameZeroContextTimeNs = 5'000'000'000,
        std::int32_t nonRunningRevision = 1) noexcept
{
    (void)clock.setMixerFrameZeroContextTimeNs(frameZeroContextTimeNs);
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::suspendedContextState, nonRunningRevision);
}

[[nodiscard]] auto
stableCursor(BrowserAudioClock& clock,
             BrowserAudioClock::RenderCursor& before,
             BrowserAudioClock::RenderCursor& after) noexcept -> bool
{
    return clock.tryReadRenderCursor(before) &&
           clock.tryReadRenderCursor(after);
}

[[nodiscard]] auto
testConfigurationAndLatestAttemptWins() noexcept -> bool
{
    auto clock = BrowserAudioClock{};
    if (clock.configure(7'999, quantumFrames) ||
        clock.configure(192'001, quantumFrames) ||
        clock.configure(48'000, 0) ||
        !clock.configure(48'000, quantumFrames) ||
        clock.configure(44'100, quantumFrames) ||
        clock.configure(48'000, 64) ||
        clock.setMixerFrameZeroContextTimeNs(-1)) {
        return false;
    }
    prepare(clock);

    if (clock.arm(1, 0) ||
        clock.arm(1, minimumCountdownFrames - 1) ||
        !clock.arm(5, minimumCountdownFrames) ||
        clock.arm(4, minimumCountdownFrames)) {
        return false;
    }

    // The rejected lower-generation click canceled generation 5.
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    auto anchor = BrowserAudioAnchor{};
    return stableCursor(clock, before, after) &&
           !clock.tryEstablishAnchor(
             before,
             { .contextTimeNs = 5'001'000'000,
               .performanceTimeNs = 10'000'000'000 },
             after,
             anchor);
}

[[nodiscard]] auto
testAudibleOutputMappingAndStateInvalidation() noexcept -> bool
{
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock, 5'000'000'000, 7);
    if (!clock.arm(1, minimumCountdownFrames)) {
        return false;
    }

    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 7);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    auto anchor = BrowserAudioAnchor{};
    const auto outputTimestamp = BrowserOutputTimestamp{
        .contextTimeNs = 5'001'000'000,
        .performanceTimeNs = 10'000'000'000,
    };
    if (!stableCursor(clock, before, after) ||
        !clock.tryEstablishAnchor(
          before, outputTimestamp, after, anchor)) {
        return false;
    }

    // Mixer frame zero began at context time 5 s. The graph has rendered
    // through 5.002666667 s, while the device output timestamp is at 5.001 s.
    // The chart starts at context time 5.024 s and is therefore audible at
    // performance time 10.023 s, not graph-sample-now + countdown.
    if (anchor.sessionGeneration != 1 ||
        anchor.chartStartFrame != 1'152 ||
        anchor.chartStartContextTimeNs != 5'024'000'000 ||
        anchor.chartStartBrowserMonotonicNs != 10'023'000'000 ||
        anchor.contextNonRunningRevision != 7) {
        return false;
    }

    auto chartTimeNanoseconds = std::int64_t{};
    if (!clock.chartTimeForBrowserEventUs(
          10'023'000, chartTimeNanoseconds) ||
        chartTimeNanoseconds != 0 ||
        !clock.chartTimeForBrowserMonotonicNs(
          10'024'000'000, chartTimeNanoseconds) ||
        chartTimeNanoseconds != 1'000'000 ||
        !clock.chartTimeForRenderedFrame(
          1'151, chartTimeNanoseconds) ||
        chartTimeNanoseconds != -20'833) {
        return false;
    }

    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::suspendedContextState, 8);
    if (clock.tryReadAnchor(anchor) ||
        clock.chartTimeForBrowserMonotonicNs(
          10'024'000'000, chartTimeNanoseconds)) {
        return false;
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 8);
    if (clock.tryReadAnchor(anchor) ||
        clock.arm(1, minimumCountdownFrames) ||
        !clock.arm(2, minimumCountdownFrames)) {
        return false;
    }

    clock.beginRenderQuantum();
    clock.finishRenderQuantum(256, quantumFrames);
    return stableCursor(clock, before, after) &&
           clock.tryEstablishAnchor(
             before,
             { .contextTimeNs = 5'004'000'000,
               .performanceTimeNs = 11'000'000'000 },
             after,
             anchor) &&
           anchor.sessionGeneration == 2;
}

[[nodiscard]] auto
testFailedRearmCannotRevivePreviousGeneration() noexcept -> bool
{
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock);
    if (!clock.arm(1, minimumCountdownFrames) ||
        clock.arm(2, minimumCountdownFrames - 1)) {
        return false;
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    auto anchor = BrowserAudioAnchor{};
    return stableCursor(clock, before, after) &&
           !clock.tryEstablishAnchor(
             before,
             { .contextTimeNs = 5'001'000'000,
               .performanceTimeNs = 10'000'000'000 },
             after,
             anchor);
}

[[nodiscard]] auto
testCommitRechecksLiveLead() noexcept -> bool
{
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock);
    if (!clock.arm(1, minimumCountdownFrames)) {
        return false;
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);

    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    if (!stableCursor(clock, before, after)) {
        return false;
    }
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(256, quantumFrames);

    auto anchor = BrowserAudioAnchor{};
    const auto timestamp = BrowserOutputTimestamp{
        .contextTimeNs = 5'001'000'000,
        .performanceTimeNs = 10'000'000'000,
    };
    if (clock.tryEstablishAnchor(
          before, timestamp, after, anchor)) {
        return false;
    }

    // The arm remains available for a fresh stable sample whose chart start
    // again has all eight quanta of lead.
    return stableCursor(clock, before, after) &&
           clock.tryEstablishAnchor(
             before, timestamp, after, anchor) &&
           anchor.chartStartFrame == 1'280;
}

[[nodiscard]] auto
testRateRounding(std::uint32_t sampleRate,
                 std::int64_t expectedFrameOffsetNs) noexcept -> bool
{
    constexpr auto originNs = std::int64_t{ 2'000'000'000 };
    auto clock = BrowserAudioClock{ sampleRate, quantumFrames };
    prepare(clock, originNs);
    if (!clock.arm(1, minimumCountdownFrames)) {
        return false;
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);

    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    auto anchor = BrowserAudioAnchor{};
    if (!stableCursor(clock, before, after) ||
        !clock.tryEstablishAnchor(
          before,
          { .contextTimeNs = originNs + 1'000'000,
            .performanceTimeNs = 3'000'000'000 },
          after,
          anchor) ||
        anchor.chartStartContextTimeNs !=
          originNs + expectedFrameOffsetNs ||
        anchor.chartStartBrowserMonotonicNs !=
          3'000'000'000 + expectedFrameOffsetNs - 1'000'000) {
        return false;
    }

    auto chartTimeNanoseconds = std::int64_t{};
    if (!clock.chartTimeForRenderedFrame(
          anchor.chartStartFrame - 1, chartTimeNanoseconds)) {
        return false;
    }
    const auto oneFrameNanoseconds = chartTimeNanoseconds;
    return oneFrameNanoseconds < 0 &&
           clock.chartTimeForRenderedFrame(
             anchor.chartStartFrame, chartTimeNanoseconds) &&
           chartTimeNanoseconds == 0 &&
           clock.chartTimeForRenderedFrame(
             anchor.chartStartFrame + 1, chartTimeNanoseconds) &&
           chartTimeNanoseconds == -oneFrameNanoseconds &&
           !clock.chartTimeForRenderedFrame(
             (std::numeric_limits<std::uint64_t>::max)(),
             chartTimeNanoseconds) &&
           !clock.chartTimeForBrowserEventUs(
             (std::numeric_limits<std::int64_t>::max)(),
             chartTimeNanoseconds) &&
           !clock.chartTimeForBrowserEventUs(
             (std::numeric_limits<std::int64_t>::min)(),
             chartTimeNanoseconds);
}

[[nodiscard]] auto
testInvalidTimestampAndChangedCursor() noexcept -> bool
{
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock);
    if (!clock.arm(1, minimumCountdownFrames)) {
        return false;
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    if (!clock.tryReadRenderCursor(before)) {
        return false;
    }
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(256, quantumFrames);
    if (!clock.tryReadRenderCursor(after)) {
        return false;
    }
    auto anchor = BrowserAudioAnchor{};
    return !clock.tryEstablishAnchor(
             before,
             { .contextTimeNs = 5'001'000'000,
               .performanceTimeNs = 10'000'000'000 },
             after,
             anchor) &&
           !clock.tryEstablishAnchor(
             after, {}, after, anchor);
}

[[nodiscard]] auto
testOutputTimestampMustAdvanceBeyondRearmSample() noexcept -> bool
{
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock);
    const auto outputBeforeResume = BrowserOutputTimestamp{
        .contextTimeNs = 5'001'000'000,
        .performanceTimeNs = 10'000'000'000,
    };
    if (clock.arm(
          1,
          minimumCountdownFrames,
          { .contextTimeNs = 1, .performanceTimeNs = 0 }) ||
        !clock.arm(
          1, minimumCountdownFrames, outputBeforeResume)) {
        return false;
    }

    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 1);
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    auto anchor = BrowserAudioAnchor{};
    if (!stableCursor(clock, before, after) ||
        clock.tryEstablishAnchor(
          before, outputBeforeResume, after, anchor) ||
        clock.tryEstablishAnchor(
          before,
          { .contextTimeNs = 5'002'000'000,
            .performanceTimeNs =
              outputBeforeResume.performanceTimeNs },
          after,
          anchor)) {
        return false;
    }

    return clock.tryEstablishAnchor(
             before,
             { .contextTimeNs = 5'002'000'000,
               .performanceTimeNs = 10'001'000'000 },
             after,
             anchor) &&
           anchor.sessionGeneration == 1;
}

[[nodiscard]] auto
testBoundedConcurrentReaders() -> bool
{
    constexpr auto quantumCount = std::uint64_t{ 20'000 };
    auto clock = BrowserAudioClock{ 48'000, quantumFrames };
    prepare(clock, 0, 3);
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::runningContextState, 3);
    auto writerDone = std::atomic_bool{};
    auto writer = std::thread{ [&] {
        for (auto sequence = std::uint64_t{ 1 }; sequence <= quantumCount;
             ++sequence) {
            clock.beginRenderQuantum();
            clock.finishRenderQuantum(
              sequence * quantumFrames, quantumFrames);
        }
        writerDone.store(true, std::memory_order_release);
    } };

    auto coherentSnapshots = std::size_t{};
    auto valid = true;
    for (auto attempt = std::size_t{}; attempt < 100'000; ++attempt) {
        auto cursor = BrowserAudioClock::RenderCursor{};
        if (clock.tryReadRenderCursor(cursor)) {
            ++coherentSnapshots;
            valid = valid && (cursor.revision & 1U) == 0U;
            if (cursor.completedQuantumSequence != 0) {
                valid = valid &&
                        cursor.lastQuantumFrames == quantumFrames &&
                        cursor.renderedFrames ==
                          cursor.completedQuantumSequence * quantumFrames;
            }
        }
        if (writerDone.load(std::memory_order_acquire) &&
            coherentSnapshots != 0) {
            break;
        }
    }
    writer.join();

    auto anchor = BrowserAudioAnchor{};
    if (!clock.arm(1, minimumCountdownFrames)) {
        return false;
    }
    clock.beginRenderQuantum();
    clock.finishRenderQuantum(
      (quantumCount + 1U) * quantumFrames, quantumFrames);
    auto before = BrowserAudioClock::RenderCursor{};
    auto after = BrowserAudioClock::RenderCursor{};
    valid = valid && stableCursor(clock, before, after) &&
            clock.tryEstablishAnchor(
              before,
              { .contextTimeNs = 1'000'000,
                .performanceTimeNs = 4'000'000'000 },
              after,
              anchor);

    auto anchorReaderValid = std::atomic_bool{ true };
    auto readerReady = std::atomic_bool{};
    auto reader = std::thread{ [&] {
        readerReady.store(true, std::memory_order_release);
        for (auto attempt = std::size_t{}; attempt < 20'000; ++attempt) {
            auto observed = BrowserAudioAnchor{};
            if (clock.tryReadAnchor(observed) &&
                observed.sessionGeneration != anchor.sessionGeneration) {
                anchorReaderValid.store(false, std::memory_order_release);
            }
        }
    } };
    for (auto wait = std::size_t{};
         wait < 100'000 &&
         !readerReady.load(std::memory_order_acquire);
         ++wait) {
        std::this_thread::yield();
    }
    clock.publishBrowserContextStateForTesting(
      BrowserAudioClock::suspendedContextState, 4);
    reader.join();

    auto afterSuspend = BrowserAudioAnchor{};
    return valid && coherentSnapshots != 0 &&
           anchorReaderValid.load(std::memory_order_acquire) &&
           !clock.tryReadAnchor(afterSuspend);
}

} // namespace

auto
main() -> int
{
    if (!testConfigurationAndLatestAttemptWins()) {
        return 1;
    }
    if (!testAudibleOutputMappingAndStateInvalidation()) {
        return 2;
    }
    if (!testFailedRearmCannotRevivePreviousGeneration()) {
        return 3;
    }
    if (!testCommitRechecksLiveLead()) {
        return 4;
    }
    if (!testRateRounding(44'100, 26'122'449) ||
        !testRateRounding(48'000, 24'000'000) ||
        !testRateRounding(96'000, 12'000'000)) {
        return 5;
    }
    if (!testInvalidTimestampAndChangedCursor()) {
        return 6;
    }
    if (!testOutputTimestampMustAdvanceBeyondRearmSample()) {
        return 7;
    }
    if (!testBoundedConcurrentReaders()) {
        return 8;
    }
    return 0;
}
