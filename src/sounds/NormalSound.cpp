//
// Created by bobini on 14.01.23.
//

#include "NormalSound.h"

#include <memory>

#include "AudioEngine.h"
#include "sounds/SoundBuffer.h"
namespace sounds {
void
NormalSound::onDeviceChanged()
{
    // get timepoint of sound
    auto cursor = ma_uint64{};
    const auto currentFrame =
      ma_sound_get_cursor_in_pcm_frames(sound.get(), &cursor);
    const auto isPlaying = ma_sound_is_playing(sound.get());
    const auto volume = ma_sound_get_volume(sound.get());
    ma_sound_uninit(sound.get());
    ma_sound_init_from_data_source(
      engine->getEngine(),
      audioBuffer.get(),
      MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_WAIT_INIT | MA_SOUND_FLAG_NO_PITCH |
        MA_SOUND_FLAG_NO_SPATIALIZATION,
      nullptr,
      sound.get());
    ma_sound_seek_to_pcm_frame(sound.get(), currentFrame);
    ma_sound_set_volume(sound.get(), volume);
    if (isPlaying) {
        ma_sound_start(sound.get());
    }
}
NormalSound::NormalSound(AudioEngine* engine,
                         std::shared_ptr<const SoundBuffer> buffer)
  : engine(engine)
  , buffer(std::move(buffer))
{

    const auto audioBufferConfig =
      ma_audio_buffer_config_init(ma_format_f32,
                                  2,
                                  this->buffer->getFrames(),
                                  this->buffer->getSamples().data(),
                                  nullptr);
    ma_audio_buffer_init(&audioBufferConfig, audioBuffer.get());

    ma_sound_init_from_data_source(
      engine->getEngine(),
      audioBuffer.get(),
      MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_WAIT_INIT | MA_SOUND_FLAG_NO_PITCH |
        MA_SOUND_FLAG_NO_SPATIALIZATION,
      nullptr,
      sound.get());

    connect(engine,
            &AudioEngine::changeDeviceRequested,
            this,
            &NormalSound::onDeviceChanged);
}

NormalSound::~NormalSound()
{
    if (audioBuffer) {
        ma_audio_buffer_uninit(audioBuffer.get());
    }
    if (sound) {
        ma_sound_uninit(sound.get());
    }
}

void
NormalSound::play()
{
    stop();
    ma_sound_start(sound.get());
}

void
NormalSound::stop()
{
    ma_sound_stop(sound.get());
    ma_sound_seek_to_pcm_frame(sound.get(), 0);
}

auto
NormalSound::isPlaying() const -> bool
{
    return ma_sound_is_playing(sound.get()) != 0;
}

void
NormalSound::setVolume(float v)
{
    ma_sound_set_volume(sound.get(), v);
}

auto
NormalSound::getVolume() const -> float
{
    return ma_sound_get_volume(sound.get());
}

auto
NormalSound::getBuffer() const -> const std::shared_ptr<const SoundBuffer>&
{
    return buffer;
}

} // namespace sounds