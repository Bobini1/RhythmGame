//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_SOUND_H
#define RHYTHMGAME_SOUND_H
#include <spdlog/spdlog.h>
#include <miniaudio.h>

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
class Sound
{
    AudioEngine* engine;
    std::shared_ptr<const SoundBuffer> buffer;
    std::unique_ptr<ma_audio_buffer> audioBuffer = std::make_unique<ma_audio_buffer>();
    std::unique_ptr<ma_sound> sound = std::make_unique<ma_sound>();

  public:
    /**
     * @brief Creates a new sound from a buffer.
     * @param sampleBuffer The buffer to use.
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
     * @brief Pauses the sound.
     * @details Does not block. The sound can be played again.
     */
    void pause();

    /**
     * @brief Sets the volume of the sound.
     * @param volume The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    void setVolume(float volume);
    /**
     * @brief Sets the playback position.
     * @param offset The playback position.
     */
    void setTimePoint(std::chrono::nanoseconds offset);

    /**
     * @brief Is the sound playing right now?
     */
    auto isPlaying() const -> bool;
    /**
     * @brief Is the sound paused right now?
     */
    auto isPaused() const -> bool;
    /**
     * @brief Is the sound stopped right now?
     */
    auto isStopped() const -> bool;

    /**
     * @brief Gets the volume of the sound.
     * @return The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    auto getVolume() const -> float;
    /**
     * @brief Gets the playback position.
     * @return The playback position.
     */
    auto getTimePoint() const -> std::chrono::nanoseconds;

    // those are forwarded from the buffer
    /**
     * @brief Gets the duration of the sound.
     * @return The duration of the sound. Not affected by the playback rate.
     */
    auto getDuration() const -> std::chrono::nanoseconds;
    /**
     * @brief Gets the sampling frequency of the sound.
     * @return The sampling frequency of the sound.
     */
    auto getFrequency() const -> int;
    /**
     * @brief Gets the number of channels of the sound.
     * @return The number of channels of the sound.
     */
    auto getChannels() const -> int;

    // Called by AudioEngine
    void mixInto(float* out,
                 unsigned long frames,
                 int outChannels,
                 int outSampleRate);
};
} // namespace sounds
#endif // RHYTHMGAME_SOUND_H
