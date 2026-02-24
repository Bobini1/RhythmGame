//
// Created by PC on 24/02/2026.
//

#ifndef RHYTHMGAME_NORMALSOUNDBUFFER_H
#define RHYTHMGAME_NORMALSOUNDBUFFER_H

#include "SoundBuffer.h"
#include <vector>
#include <filesystem>
#include <span>

namespace sounds {
class AudioEngine;
/**
 * @brief A buffer that can be used to create sounds. Wraps an  buffer.
 * @details This class is used to load, decode and store the sound data.
 * It can be used to create multiple sounds that share the same buffer.
 * Creating new instances of this class can be very expensive, so it is
 * recommended to load every sound only once, at the start of a scene.
 */
class NormalSoundBuffer : public SoundBuffer
{
    std::vector<float> samples;

  public:
    /**
     * @brief Creates a new sound buffer from a file.
     * @param filename The path to the file to load.
     */
    explicit NormalSoundBuffer(AudioEngine* engine,
                               const std::filesystem::path& filename);

    auto getFrames() const -> ma_uint64 override;

    auto getSamples() const -> std::span<const float> override;
};

} // sounds

#endif // RHYTHMGAME_NORMALSOUNDBUFFER_H
