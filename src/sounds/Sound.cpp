//
// Created by bobini on 14.01.23.
//

#include <stdexcept>
#include <memory>
#include "Sound.h"

#include "AudioEngine.h"
#include "sounds/SoundBuffer.h"
namespace sounds {
Sound::Sound(AudioEngine* engine, std::shared_ptr<const SoundBuffer> buffer)
  : engine(engine)
  , buffer(std::move(buffer))
{

    auto audioBufferConfig =
      ma_audio_buffer_config_init(ma_format_f32,
                                  2,
                                  this->buffer->getFrames(),
                                  this->buffer->getSamples().data(),
                                  nullptr);
    ma_audio_buffer_init(&audioBufferConfig, audioBuffer.get());

    ma_sound_init_from_data_source(engine->getEngine(),
                                   audioBuffer.get(),
                                   MA_SOUND_FLAG_DECODE,
                                   nullptr,
                                   sound.get());
}

Sound::~Sound()
{
    ma_audio_buffer_uninit(audioBuffer.get());
    ma_sound_uninit(sound.get());
}

void
Sound::play()
{
    stop();
    ma_sound_start(sound.get());
}

void
Sound::stop()
{
    ma_sound_stop(sound.get());
    ma_sound_seek_to_pcm_frame(sound.get(), 0);
}

auto
Sound::isPlaying() const -> bool
{
    return ma_sound_is_playing(sound.get()) != 0;
}

void
Sound::setVolume(float v)
{
    ma_sound_set_volume(sound.get(), v);
}

auto
Sound::getVolume() const -> float
{
    return ma_sound_get_volume(sound.get());
}

auto
Sound::getBuffer() const -> const std::shared_ptr<const SoundBuffer>&
{
    return buffer;
}

} // namespace sounds