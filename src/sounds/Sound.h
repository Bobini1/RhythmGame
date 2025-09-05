//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_SOUND_H
#define RHYTHMGAME_SOUND_H
#include <gst/gstelement.h>
#include <spdlog/spdlog.h>

/**
 * @brief The namespace for all sound related classes.
 * @details This namespace contains all classes related to sound playback,
 * loading, decoding, etc.
 */
namespace sounds {
class SoundBuffer;

/**
 * @brief A sound that can be played.
 * @details The underlying data buffer can be shared between multiple sounds.
 * This means that you can have multiple instances of the same sound playing at
 * the same time, while only loading the file once.
 */
class Sound : public std::enable_shared_from_this<Sound>
{
    GstElement* pipeline;
    std::shared_ptr<const SoundBuffer> buffer;
    GstElement* player;
    GstElement* volume;

  public:
    /**
     * @brief Creates a new sound from a buffer.
     * @param sampleBuffer The buffer to use.
     * @param engine The audio engine to use.
     */
    explicit Sound(GstElement* player,
             std::shared_ptr<const SoundBuffer> buffer);
    ~Sound();

    /**
     * @brief Gets the underlying buffer.
     * @return The underlying buffer.
     */
    auto getBuffer() const
      -> const std::shared_ptr<const SoundBuffer>&;

    /**
     * @brief Plays the sound.
     * @details This does not block, obviously. Make sure the sound object stays
     * alive.
     */
    void play();
    /**
     * @brief Stops the sound.
     * @details Does not block. The sound can be played again.
     */
    void stop();

    /**
     * @brief Sets the volume of the sound.
     * @param volume The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    void setVolume(float volume);

    auto getVolume() const -> float;

    /**
     * @brief Is the sound playing right now?
     */
    auto isPlaying() const -> bool;
};
} // namespace sounds
#endif // RHYTHMGAME_SOUND_H
