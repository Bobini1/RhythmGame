#include "ScheduledPcmSound.h"

#include <limits>

namespace web_playtest {
namespace detail {
auto
chartTimeToFrame(std::uint64_t chartStartFrame,
                 std::int64_t chartTimeNanoseconds,
                 std::uint32_t outputSampleRate) noexcept -> std::uint64_t
{
    const auto nanoseconds =
      chartTimeNanoseconds > 0
        ? static_cast<std::uint64_t>(chartTimeNanoseconds)
        : std::uint64_t{};
    constexpr auto nanosecondsPerSecond = std::uint64_t{ 1'000'000'000 };
    const auto wholeSeconds = nanoseconds / nanosecondsPerSecond;
    const auto remainder = nanoseconds % nanosecondsPerSecond;
    const auto maximum = (std::numeric_limits<std::uint64_t>::max)();
    if (outputSampleRate != 0 &&
        wholeSeconds > (maximum - chartStartFrame) / outputSampleRate) {
        return maximum;
    }
    const auto wholeFrames = wholeSeconds * outputSampleRate;
    const auto roundedFraction =
      (remainder * outputSampleRate + nanosecondsPerSecond / 2) /
      nanosecondsPerSecond;
    if (roundedFraction > maximum - chartStartFrame - wholeFrames) {
        return maximum;
    }
    return chartStartFrame + wholeFrames + roundedFraction;
}
} // namespace detail

ScheduledPcmSound::ScheduledPcmSound(AudioTransport& transport,
                                     VoiceId voice) noexcept
  : channel(&transport)
  , voiceId(voice)
{
}

void
ScheduledPcmSound::play()
{
    publish(AudioCommandType::Start, std::chrono::nanoseconds{}, true);
}
void
ScheduledPcmSound::playAt(std::chrono::nanoseconds chartTime)
{
    publish(AudioCommandType::Start, chartTime, false);
}
void
ScheduledPcmSound::stop()
{
    publish(AudioCommandType::Stop, std::chrono::nanoseconds{}, true);
}
void
ScheduledPcmSound::stopAt(std::chrono::nanoseconds chartTime)
{
    publish(AudioCommandType::Stop, chartTime, false);
}
void
ScheduledPcmSound::setVolume(float volume)
{
    publish(
      AudioCommandType::SetVoiceGain, std::chrono::nanoseconds{}, true, volume);
}
auto
ScheduledPcmSound::isPlaying() const -> bool
{
    return channel->isVoicePlaying(voiceId);
}
auto
ScheduledPcmSound::getVolume() const -> float
{
    return channel->voiceGain(voiceId);
}

void
ScheduledPcmSound::publish(AudioCommandType type,
                           std::chrono::nanoseconds chartTime,
                           bool immediate,
                           float value) noexcept
{
    const auto provenance = channel->commandProvenance();
    const auto session = channel->sessionSnapshot();
    const auto sequence = channel->nextSequence();
    (void)channel->tryPublish(
      { .type = type,
        .sessionGeneration = session.generation,
        .sequenceId = sequence,
        .sourceInputId = provenance.sourceInputId,
        .voiceId = voiceId,
        .targetFrame =
          immediate ? session.currentOutputFrame : frameFor(session, chartTime),
        .sourceEventMonotonicUs = provenance.sourceEventMonotonicUs,
        .publishedMonotonicUs = provenance.publishedMonotonicUs,
        .value = value });
}

auto
ScheduledPcmSound::frameFor(const AudioSessionSnapshot& session,
                            std::chrono::nanoseconds chartTime) noexcept
  -> std::uint64_t
{
    return detail::chartTimeToFrame(
      session.chartStartFrame, chartTime.count(), session.outputSampleRate);
}

} // namespace web_playtest
