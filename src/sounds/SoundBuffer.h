//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_SOUNDBUFFER_H
#define RHYTHMGAME_SOUNDBUFFER_H
#include "AudioEngine.h"
#include <vector>
#include <filesystem>

namespace sounds {
class AudioEngine;

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

  public:
    /**
     * @brief Creates a new sound buffer from a file.
     * @param filename The path to the file to load.
     */
    explicit SoundBuffer(AudioEngine* engine, const std::filesystem::path& filename);

    auto getFrames() const -> ma_uint64;

    auto getSamples() const -> const std::vector<float>&;
};
} // namespace sounds

#endif // RHYTHMGAME_SOUNDBUFFER_H
