//
// Created by bobini on 14.01.23.
//

#ifndef RHYTHMGAME_OPENALSOUND_H
#define RHYTHMGAME_OPENALSOUND_H
#include <AL/al.h>
#include <spdlog/spdlog.h>
#include <span>

/**
 * @brief The namespace for all sound related classes.
 * @details This namespace contains all classes related to sound playback,
 * loading, decoding, etc.
 */
namespace sounds {
class OpenALSoundBuffer;

/**
 * @brief A sound that can be played. Wraps an OpenAL source.
 * @details The underlying data buffer can be shared between multiple sounds.
 * This means that you can have multiple instances of the same sound playing at
 * the same time, while only loading the file once.
 */
class OpenALSound
{
    ALuint source{};
    std::shared_ptr<const OpenALSoundBuffer> sampleBuffer;

  public:
    /**
     * @brief Creates a new sound from a buffer.
     * @param sampleBuffer The buffer to use.
     */
    explicit OpenALSound(std::shared_ptr<const OpenALSoundBuffer> sampleBuffer);
    /**
     * @brief Creates a new sound that shares the buffer with another sound.
     * @details The playback state of the other sound is not copied.
     * @param other The sound to share the buffer with.
     */
    OpenALSound(const OpenALSound& other);
    /**
     * @brief Moves the buffer and playback state from another sound.
     * @param other The sound to move the buffer from.
     */
    OpenALSound(OpenALSound&& other) noexcept;
    /**
     * @brief Copies the buffer of other sound and resets the playback state.
     * @param other The sound to copy the buffer from.
     */
    auto operator=(const OpenALSound& other) -> OpenALSound&;
    /**
     * @brief Moves the buffer and playback state from another sound.
     * @param other The sound to move the buffer from.
     */
    auto operator=(OpenALSound&& other) noexcept -> OpenALSound&;
    ~OpenALSound();

    /**
     * @brief Gets the underlying OpenAL source.
     * @return The underlying OpenAL source.
     */
    [[nodiscard]] auto getSource() const -> ALuint;

    /**
     * @brief Gets the underlying buffer.
     * @return The underlying buffer.
     */
    [[nodiscard]] auto getBuffer() const
      -> const std::shared_ptr<const OpenALSoundBuffer>&;

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
     * @brief Sets whether the sound should loop.
     * @param looping Whether the sound should loop.
     */
    void setIsLooping(bool looping);
    /**
     * @brief Sets the playback rate.
     * @details 1 is normal speed, 2 is twice as fast, 0.5 is half as fast.
     * @param rate The playback rate.
     */
    void setRate(float rate);
    /**
     * @brief Sets the playback position.
     * @param offset The playback position.
     */
    void setTimePoint(std::chrono::nanoseconds offset);

    /**
     * @brief Is the sound playing right now?
     */
    [[nodiscard]] auto isPlaying() const -> bool;
    /**
     * @brief Is the sound paused right now?
     */
    [[nodiscard]] auto isPaused() const -> bool;
    /**
     * @brief Is the sound stopped right now?
     */
    [[nodiscard]] auto isStopped() const -> bool;

    /**
     * @brief Gets the volume of the sound.
     * @return The volume. 1 is normal, 0 is silent, 2 is twice as loud.
     */
    [[nodiscard]] auto getVolume() const -> float;
    /**
     * @brief Gets the playback rate.
     * @details 1 is normal speed, 2 is twice as fast, 0.5 is half as fast.
     * @return The playback rate.
     */
    [[nodiscard]] auto getRate() const -> float;
    /**
     * @brief Is the sound looping?
     */
    [[nodiscard]] auto getIsLooping() const -> bool;
    /**
     * @brief Gets the playback position.
     * @return The playback position.
     */
    [[nodiscard]] auto getTimePoint() const -> std::chrono::nanoseconds;

    // those are forwarded from the buffer
    /**
     * @brief Gets the duration of the sound.
     * @return The duration of the sound. Not affected by the playback rate.
     */
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    /**
     * @brief Gets the sampling frequency of the sound.
     * @return The sampling frequency of the sound.
     */
    [[nodiscard]] auto getFrequency() const -> int;
    /**
     * @brief Gets the number of channels of the sound.
     * @return The number of channels of the sound.
     */
    [[nodiscard]] auto getChannels() const -> int;
};
} // namespace sounds
#endif // RHYTHMGAME_OPENALSOUND_H
