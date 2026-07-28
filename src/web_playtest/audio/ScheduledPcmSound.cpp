#include "ScheduledPcmSound.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace web_playtest {

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
    if (std::isfinite(volume)) {
        channel->setRequestedVoiceGain(voiceId, volume);
    }
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
    const auto nonnegative = std::max(chartTime, std::chrono::nanoseconds{});
    const auto frames = static_cast<long double>(nonnegative.count()) *
                        session.outputSampleRate / 1'000'000'000.0L;
    const auto rounded = static_cast<std::uint64_t>(std::llround(frames));
    const auto start = session.chartStartFrame;
    if (rounded > std::numeric_limits<std::uint64_t>::max() - start) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return start + rounded;
}

} // namespace web_playtest
