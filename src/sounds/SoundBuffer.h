//
// Created by bobini on 15.04.23.
//

#ifndef RHYTHMGAME_SOUNDBUFFER_H
#define RHYTHMGAME_SOUNDBUFFER_H
#include <filesystem>
#include <gst/gstelement.h>

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
    GstElement *pipeline;

  public:
    /**
     * @brief Creates a new sound buffer from a file.
     * @param filename The path to the file to load.
     */
    explicit SoundBuffer(const std::filesystem::path& filename);
    ~SoundBuffer();

    /**
     * @brief Gets the internal  buffer.
     * @return The internal buffer.
     */
    auto getBuffer() const -> GstElement*;
};
} // namespace sounds

#endif // RHYTHMGAME_SOUNDBUFFER_H
