//
// Created by PC on 10/09/2025.
//

#include "AudioPlayer.h"

#include "AudioEngine.h"

namespace sounds {
void
AudioPlayer::onDeviceChanged()
{
    if (!sound) {
        return;
    }
    auto isPlayingNow = isPlaying();
    auto cursor = size_t{};
    auto currentPcmFrame = ma_sound_get_cursor_in_pcm_frames(sound.get(), &cursor);
    ma_sound_uninit(sound.get());
    if (ma_sound_init_from_file_w(engine->getEngine(),
                              source.toStdWString().c_str(),
                              MA_SOUND_FLAG_NO_PITCH |
                                MA_SOUND_FLAG_NO_SPATIALIZATION,
                              nullptr,
                              nullptr,
                              sound.get()) != MA_SUCCESS) {
        spdlog::error("Failed to load sound: {}", source.toStdString());
        sound.reset();
        return;
    }
    ma_sound_set_looping(sound.get(), looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), volume);
    ma_sound_seek_to_pcm_frame(sound.get(), currentPcmFrame);
    if (isPlayingNow) {
        play();
    }
}
AudioPlayer::AudioPlayer(QObject* parent)
  : QObject(parent)
{
    connect(engine, &AudioEngine::changeDeviceRequested, this, &AudioPlayer::onDeviceChanged);
}
AudioPlayer::~AudioPlayer()
{
    if (sound) {
        ma_sound_uninit(sound.get());
    }
}
auto
AudioPlayer::getSource() const -> QString
{
    return source;
}
void
AudioPlayer::setSource(const QString& value)
{
    if (source == value) {
        return;
    }
    source = value;
    if (sound) {
        ma_sound_uninit(sound.get());
    } else {
        sound = std::make_unique<ma_sound>();
    }
    if (ma_sound_init_from_file_w(engine->getEngine(),
                              value.toStdWString().c_str(),
                              MA_SOUND_FLAG_NO_PITCH |
                                MA_SOUND_FLAG_NO_SPATIALIZATION,
                              nullptr,
                              nullptr,
                              sound.get()) != MA_SUCCESS) {
        spdlog::error("Failed to load sound: {}", value.toStdString());
        sound.reset();
    }
    ma_sound_set_looping(sound.get(), looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), volume);
    emit sourceChanged();
}
void
AudioPlayer::resetSource()
{
    if (sound) {
        ma_sound_uninit(sound.get());
        sound.reset();
    }
    if (!source.isEmpty()) {
        source.clear();
        emit sourceChanged();
    }
}
auto
AudioPlayer::isPlaying() const -> bool
{
    return sound && ma_sound_is_playing(sound.get()) != 0;
}
void
AudioPlayer::play()
{
    if (isPlaying()) {
        return;
    }
    if (sound) {
        stop();
        if (ma_sound_start(sound.get()) != MA_SUCCESS) {
            spdlog::error("Failed to play sound: {}", source.toStdString());
        }
    }
}
void
AudioPlayer::stop()
{
    if (!isPlaying()) {
        return;
    }
    if (sound) {
        ma_sound_stop(sound.get());
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
    }
}
auto
AudioPlayer::getVolume() const -> float
{
    return sound ? ma_sound_get_volume(sound.get()) : 1.0f;
}
void
AudioPlayer::setVolume(float value)
{
    if (volume == value) {
        return;
    }
    volume = value;
    if (sound) {
        ma_sound_set_volume(sound.get(), value);
        emit volumeChanged();
    }
}
auto
AudioPlayer::isLooping() const -> bool
{
    return looping;
}
void
AudioPlayer::setLooping(bool value)
{
    if (looping == value) {
        return;
    }
    looping = value;
    if (sound) {
        ma_sound_set_looping(sound.get(), value ? MA_TRUE : MA_FALSE);
        emit loopingChanged();
    }
}
} // namespace sounds