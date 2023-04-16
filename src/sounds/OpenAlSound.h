//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_OPENALSOUND_H
#define RHYTHMGAME_OPENALSOUND_H
#include <AL/al.h>
#include <spdlog/spdlog.h>
#include <span>

namespace sounds {
class OpenALSoundBuffer;

class OpenALSound
{
    ALuint source{};
    std::shared_ptr<const OpenALSoundBuffer> sampleBuffer;

  public:
    explicit OpenALSound(std::shared_ptr<const OpenALSoundBuffer> sampleBuffer);
    OpenALSound(const OpenALSound& other);
    OpenALSound(OpenALSound&& other) noexcept;
    auto operator=(const OpenALSound& other) -> OpenALSound&;
    auto operator=(OpenALSound&& other) noexcept -> OpenALSound&;
    ~OpenALSound();

    void play();
    void stop();
    void pause();
    
    void setVolume(float volume);
    void setIsLooping(bool looping);
    void setRate(float rate);
    void setTimePoint(std::chrono::nanoseconds offset);

    [[nodiscard]] auto isPlaying() const -> bool;
    [[nodiscard]] auto isPaused() const -> bool;
    [[nodiscard]] auto isStopped() const -> bool;

    [[nodiscard]] auto getVolume() const -> float;
    [[nodiscard]] auto getRate() const -> float;
    [[nodiscard]] auto getIsLooping() const -> bool;
    [[nodiscard]] auto getTimePoint() const -> std::chrono::nanoseconds;

    // those are forwarded from the buffer
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto getFrequency() const -> int;
    [[nodiscard]] auto getChannels() const -> int;
};
} // namespace sounds
#endif // RHYTHMGAME_OPENALSOUND_H
