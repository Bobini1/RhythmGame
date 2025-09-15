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
class AudioEngine;
class SoundBuffer;

/**
 * @brief A sound that can be played.
 * @details The underlying data buffer can be shared between multiple sounds.
 * This means that you can have multiple instances of the same sound playing at
 * the same time, while only loading the file once.
 */
class Sound : public QObject
{
    Q_OBJECT
    AudioEngine* engine;
    std::shared_ptr<const SoundBuffer> buffer;
    std::unique_ptr<ma_audio_buffer> audioBuffer = std::make_unique<ma_audio_buffer>();
    std::unique_ptr<ma_sound> sound = std::make_unique<ma_sound>();

    void onDeviceChanged();
  public:
    /**
     * @brief Creates a new sound from a buffer.
     * @param buffer The sample buffer to use.
     * @param engine The audio engine to use.
     */
    explicit Sound(AudioEngine* engine,
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

    /**
     * @brief Is the sound playing right now?
     */
    auto isPlaying() const -> bool;

    /**
     * @brief Gets the volume of the sound.
     * @return The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    auto getVolume() const -> float;
};
} // namespace sounds
#endif // RHYTHMGAME_SOUND_H
