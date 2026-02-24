//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_SOUNDBUFFER_H
#define RHYTHMGAME_SOUNDBUFFER_H
#include "AudioEngine.h"
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
class SoundBuffer
{
    std::vector<float> samples;

  public:
    virtual ~SoundBuffer() = default;

    virtual auto getFrames() const -> ma_uint64 = 0;

    virtual auto getSamples() const -> std::span<const float> = 0;
};
} // namespace sounds

#endif // RHYTHMGAME_SOUNDBUFFER_H
