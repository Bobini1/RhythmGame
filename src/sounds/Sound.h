//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_SOUND_H
#define RHYTHMGAME_SOUND_H
#include <QObject>
#include <spdlog/spdlog.h>
#include "MiniaudioBackend.h"

/**
 * @brief The namespace for all sound related classes.
 * @details This namespace contains all classes related to sound playback,
 * loading, decoding, etc.
 */
namespace sounds {

/**
 * @brief A sound that can be played.
 * @details The underlying data buffer can be shared between multiple sounds.
 * This means that you can have multiple instances of the same sound playing at
 * the same time, while only loading the file once.
 */
class Sound
{
  public:
    virtual ~Sound() = default;

    /**
     * @brief Plays the sound.
     * @details This does not block, obviously. Make sure the sound object stays
     * alive.
     */
    virtual void play() = 0;
    /**
     * @brief Stops the sound.
     * @details Does not block. The sound can be played again.
     */
    virtual void stop() = 0;

    /**
     * @brief Sets the volume of the sound.
     * @param volume The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    virtual void setVolume(float volume) = 0;

    /**
     * @brief Is the sound playing right now?
     */
    virtual auto isPlaying() const -> bool = 0;

    /**
     * @brief Gets the volume of the sound.
     * @return The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    virtual auto getVolume() const -> float = 0;
};
} // namespace sounds
#endif // RHYTHMGAME_SOUND_H
