//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_SOUNDBUFFER_H
#define RHYTHMGAME_SOUNDBUFFER_H
#include <portaudio.h>
#include <chrono>
#include <filesystem>

namespace sounds {

/**
 * @brief A buffer that can be used to create sounds. Wraps an  buffer.
 * @details This class is used to load, decode and store the sound data.
 * It can be used to create multiple sounds that share the same buffer.
 * Creating new instances of this class can be very expensive, so it is
 * recommended to load every sound only once, at the start of a scene.
 */
class SoundBuffer
{
    std::vector<float> samples;
    int channels = 0;
    int sampleRate = 0;
    std::size_t frames = 0;

  public:
    /**
     * @brief Creates a new sound buffer from a file.
     * @param filename The path to the file to load.
     */
    explicit SoundBuffer(const std::filesystem::path& filename);

    /**
     * @brief Gets the internal  buffer.
     * @return The internal buffer.
     */
    auto getBuffer() const -> const std::vector<float>&;
    /**
     * @brief Gets the duration of the sound.
     * @return The duration of the sound.
     */
    auto getDuration() const -> std::chrono::nanoseconds;
    /**
     * @brief Gets the sampling frequency of the sound.
     * @return The sampling frequency of the sound.
     */
    auto getFrequency() const -> int;
    /**
     * @brief Gets the number of channels in the sound.
     * @return The number of channels in the sound.
     */
    auto getChannels() const -> int;


    auto getFrames() const -> std::size_t;
};
} // namespace sounds

#endif // RHYTHMGAME_SOUNDBUFFER_H
