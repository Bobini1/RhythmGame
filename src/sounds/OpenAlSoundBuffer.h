//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_OPENALSOUNDBUFFER_H
#define RHYTHMGAME_OPENALSOUNDBUFFER_H
#include <AL/al.h>
#include <chrono>

namespace sounds {
class OpenALSoundBuffer
{
    ALuint sampleBuffer{};

  public:
    explicit OpenALSoundBuffer(const char* filename);
    ~OpenALSoundBuffer();
    OpenALSoundBuffer(const OpenALSoundBuffer& other) = delete;
    OpenALSoundBuffer(OpenALSoundBuffer&& other) noexcept;
    auto operator=(const OpenALSoundBuffer& other)
      -> OpenALSoundBuffer& = delete;
    auto operator=(OpenALSoundBuffer&& other) noexcept -> OpenALSoundBuffer&;

    [[nodiscard]] auto getBuffer() const -> ALuint;
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto getFrequency() const -> int;
    [[nodiscard]] auto getChannels() const -> int;
};
} // namespace sounds

#endif // RHYTHMGAME_OPENALSOUNDBUFFER_H
