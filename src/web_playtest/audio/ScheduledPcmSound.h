#pragma once

#include "AudioCommand.h"
#include "sounds/Sound.h"

#include <chrono>

namespace web_playtest {

namespace detail {
[[nodiscard]] auto
chartTimeToFrame(std::uint64_t chartStartFrame,
                 std::int64_t chartTimeNanoseconds,
                 std::uint32_t outputSampleRate) noexcept -> std::uint64_t;
}

class ScheduledPcmSound final : public sounds::Sound
{
  public:
    ScheduledPcmSound(AudioTransport& transport, VoiceId voice) noexcept;

    void play() override;
    void playAt(std::chrono::nanoseconds chartTime) override;
    void stop() override;
    void stopAt(std::chrono::nanoseconds chartTime) override;
    void setVolume(float volume) override;
    [[nodiscard]] auto isPlaying() const -> bool override;
    [[nodiscard]] auto getVolume() const -> float override;

  private:
    void publish(AudioCommandType type,
                 std::chrono::nanoseconds chartTime,
                 bool immediate,
                 float value = 1.F) noexcept;
    [[nodiscard]] static auto frameFor(
      const AudioSessionSnapshot& session,
      std::chrono::nanoseconds chartTime) noexcept -> std::uint64_t;

    AudioTransport* channel;
    VoiceId voiceId;
};

} // namespace web_playtest
