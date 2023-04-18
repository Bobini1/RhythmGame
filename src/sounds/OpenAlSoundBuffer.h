//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_OPENALSOUNDBUFFER_H
#define RHYTHMGAME_OPENALSOUNDBUFFER_H
#include <AL/al.h>
#include <chrono>

namespace sounds {
/**
 * @brief A buffer that can be used to create sounds. Wraps an OpenAL buffer.
 * @details This class is used to load, decode and store the sound data.
 * It can be used to create multiple sounds that share the same buffer.
 * Creating new instances of this class can be very expensive, so it is
 * recommended to load every sound only once, at the start of a scene.
 */
class OpenALSoundBuffer
{
    ALuint sampleBuffer{};

  public:
    /**
     * @brief Creates a new sound buffer from a file.
     * @param filename The path to the file to load.
     */
    explicit OpenALSoundBuffer(const char* filename);
    ~OpenALSoundBuffer();
    OpenALSoundBuffer(const OpenALSoundBuffer& other) = delete;
    /**
     * @brief Moves the internal OpenAL buffer from another sound buffer.
     * @param other The sound buffer to move the internal buffer from.
     */
    OpenALSoundBuffer(OpenALSoundBuffer&& other) noexcept;
    auto operator=(const OpenALSoundBuffer& other)
      -> OpenALSoundBuffer& = delete;
    /**
     * @brief Moves the internal OpenAL buffer from another sound buffer.
     * @param other The sound buffer to move the internal buffer from.
     */
    auto operator=(OpenALSoundBuffer&& other) noexcept -> OpenALSoundBuffer&;

    /**
     * @brief Gets the internal OpenAL buffer.
     * @return The internal OpenAL buffer.
     */
    [[nodiscard]] auto getBuffer() const -> ALuint;
    /**
     * @brief Gets the duration of the sound.
     * @return The duration of the sound.
     */
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    /**
     * @brief Gets the sampling frequency of the sound.
     * @return The sampling frequency of the sound.
     */
    [[nodiscard]] auto getFrequency() const -> int;
    /**
     * @brief Gets the number of channels in the sound.
     * @return The number of channels in the sound.
     */
    [[nodiscard]] auto getChannels() const -> int;
};
} // namespace sounds

#endif // RHYTHMGAME_OPENALSOUNDBUFFER_H
