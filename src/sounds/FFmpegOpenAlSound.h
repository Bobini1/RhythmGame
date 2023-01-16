//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_FFMPEGOPENALSOUND_H
#define RHYTHMGAME_FFMPEGOPENALSOUND_H
#include <AL/al.h>
#include <vector>
#include <optional>
#include <spdlog/spdlog.h>
#include <span>

namespace sounds {
class FFmpegOpenALSound
{
    ALuint source{};
    std::shared_ptr<const ALuint> sampleBuffer;

  public:
    explicit FFmpegOpenALSound(const char* filename);
    FFmpegOpenALSound(const FFmpegOpenALSound& other);
    FFmpegOpenALSound(FFmpegOpenALSound&& other) noexcept;
    auto operator=(const FFmpegOpenALSound& other) -> FFmpegOpenALSound&;
    auto operator=(FFmpegOpenALSound&& other) noexcept -> FFmpegOpenALSound&;
    ~FFmpegOpenALSound();

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
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto getRate() const -> float;
    [[nodiscard]] auto getIsLooping() const -> bool;
    [[nodiscard]] auto getFrequency() const -> int;
    [[nodiscard]] auto getChannels() const -> int;
    [[nodiscard]] auto getTimePoint() const -> std::chrono::nanoseconds;
};
} // namespace sounds
#endif // RHYTHMGAME_FFMPEGOPENALSOUND_H
