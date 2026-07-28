#include "web_playtest/audio/EmscriptenAudioWorklet.h"

#include <cstdint>
#include <utility>

auto
main(int argc, char**) -> int
{
    auto* worklet =
      web_playtest::EmscriptenAudioWorklet::createProcessLifetime();
    if (worklet == nullptr) {
        return 1;
    }

    // Runtime-false in verification, but not a compile-time constant: retain
    // every public integration path and its EM_JS dependencies in the link.
    if (argc == 31'337) {
        (void)worklet->createContextForDecode();
        auto bank =
          web_playtest::PcmSoundBank{ worklet->outputSampleRate() };
        bank.freeze();
        const auto config = web_playtest::RealtimeMixer::Config{
            .outputSampleRate = worklet->outputSampleRate(),
            .voiceCapacity = 0,
            .scheduledEventCapacity = 0,
            .authoredBgmEventCount = 0,
            .liveCommandHeadroom = 0,
        };
        (void)worklet->initializeWorklet(std::move(bank), config);
        (void)worklet->transport();
        (void)worklet->sealReadyHeap();
        (void)worklet->verifySealedHeapOnMainThread();
        (void)worklet->resumeFromTrustedGesture(1, 4'096);
        auto anchor = web_playtest::BrowserAudioAnchor{};
        (void)worklet->pollForAnchor(anchor);
        auto chartTime = std::int64_t{};
        (void)worklet->currentAudibleChartTime(chartTime);
        (void)worklet->telemetry();
    }
    return 0;
}
