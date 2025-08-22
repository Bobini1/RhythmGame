//
// Created by bobini on 14.01.23.
//

#include <stdexcept>
#include <memory>
#include "OpenAlSound.h"
#include "sounds/OpenAlSoundBuffer.h"

sounds::OpenALSound::OpenALSound(
  std::shared_ptr<const OpenALSoundBuffer> sampleBuffer)
  : sampleBuffer(std::move(sampleBuffer))
{
    alGenSources(1, &source);
    alSourcei(
      source, AL_BUFFER, static_cast<ALint>(this->sampleBuffer->getBuffer()));
}
sounds::OpenALSound::~OpenALSound()
{
    if (source != 0) {
        alDeleteSources(1, &source);
    }
}
auto
sounds::OpenALSound::isPlaying() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

sounds::OpenALSound::OpenALSound(const OpenALSound& other)
  : sampleBuffer(other.sampleBuffer)
{
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, sampleBuffer->getBuffer());
}
sounds::OpenALSound::OpenALSound(OpenALSound&& other) noexcept
  : source(other.source)
  , sampleBuffer(std::move(other.sampleBuffer))
{
    other.source = 0;
}
auto
sounds::OpenALSound::operator=(const OpenALSound& other) -> OpenALSound&
{
    if (this != &other) {
        alDeleteSources(1, &source);
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, sampleBuffer->getBuffer());
    }
    return *this;
}
auto
sounds::OpenALSound::operator=(OpenALSound&& other) noexcept -> OpenALSound&
{
    if (this != &other) {
        alDeleteSources(1, &source);
        source = other.source;
        sampleBuffer = std::move(other.sampleBuffer);
        other.source = 0;
    }
    return *this;
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-make-member-function-const"
void
sounds::OpenALSound::pause()
{
    alSourcePause(source);
}
void
sounds::OpenALSound::stop()
{
    alSourceStop(source);
}
void
sounds::OpenALSound::play()
{
    alSourcePlay(source);
}
void
sounds::OpenALSound::setVolume(float volume)
{
    alSourcef(source, AL_GAIN, volume);
}
auto
sounds::OpenALSound::isPaused() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_PAUSED;
}
auto
sounds::OpenALSound::isStopped() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}
auto
sounds::OpenALSound::getVolume() const -> float
{
    float volume{};
    alGetSourcef(source, AL_GAIN, &volume);
    return volume;
}
auto
sounds::OpenALSound::getDuration() const -> std::chrono::nanoseconds
{
    return sampleBuffer->getDuration();
}
void
sounds::OpenALSound::setIsLooping(bool looping)
{
    alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}
void
sounds::OpenALSound::setRate(float rate)
{
    alSourcef(source, AL_PITCH, rate);
}
void
sounds::OpenALSound::setTimePoint(std::chrono::nanoseconds offset)
{
    const auto frequency = getFrequency();
    const auto samples = offset.count() * frequency / 1'000'000'000;
    alSourcei(source, AL_SAMPLE_OFFSET, static_cast<ALint>(samples));
    // todo: check if this requires the number of channels
}
auto
sounds::OpenALSound::getFrequency() const -> int
{
    return sampleBuffer->getFrequency();
}
auto
sounds::OpenALSound::getRate() const -> float
{
    float rate{};
    alGetSourcef(source, AL_PITCH, &rate);
    return rate;
}
auto
sounds::OpenALSound::getIsLooping() const -> bool
{
    ALint looping{};
    alGetSourcei(source, AL_LOOPING, &looping);
    return looping == AL_TRUE;
}
auto
sounds::OpenALSound::getChannels() const -> int
{
    return sampleBuffer->getChannels();
}
auto
sounds::OpenALSound::getTimePoint() const -> std::chrono::nanoseconds
{
    ALint samples{};
    alGetSourcei(source, AL_SAMPLE_OFFSET, &samples);

    auto denominator = getFrequency();
    if (denominator == 0) {
        spdlog::error("Error determining sound frequency. "
                      "Does your device have a sound card?");
        return std::chrono::nanoseconds::zero();
    }
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
      static_cast<unsigned long long int>(samples) * 1'000'000'000ULL /
      static_cast<unsigned long long int>(denominator)));
}
auto
sounds::OpenALSound::getSource() const -> ALuint
{
    return source;
}
auto
sounds::OpenALSound::getBuffer() const
  -> const std::shared_ptr<const OpenALSoundBuffer>&
{
    return sampleBuffer;
}
#pragma clang diagnostic pop
