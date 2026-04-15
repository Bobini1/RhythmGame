//
// Created by PC on 10/09/2025.
//

#include "AudioPlayer.h"

#include "AudioEngine.h"
#include <QFileInfo>
#include <algorithm>

namespace sounds {
void
AudioPlayer::onDeviceChanged()
{
    if (!sound) {
        return;
    }
    auto isPlayingNow = isPlaying();
    auto cursor = ma_uint64{};
    auto currentPcmFrame =
      ma_sound_get_cursor_in_pcm_frames(sound.get(), &cursor);
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
    ma_sound_set_fade_in_milliseconds(sound.get(), 0, volume, fadeInMillis);
    ma_sound_seek_to_pcm_frame(sound.get(), currentPcmFrame);
    auto lengthInSeconds = 0.0f;
    ma_sound_get_length_in_seconds(sound.get(), &lengthInSeconds);
    playingFinishedTimer.setInterval(static_cast<int>(lengthInSeconds * 1000));
    auto cursorInSeconds = 0.0f;
    ma_sound_get_cursor_in_seconds(sound.get(), &cursorInSeconds);
    if (isPlayingNow) {
        if (ma_sound_start(sound.get()) != MA_SUCCESS) {
            spdlog::error("Failed to play sound: {}", source.toStdString());
            stop();
        } else {
            if (!looping) {
                QTimer::singleShot(
                  static_cast<int>((lengthInSeconds - cursorInSeconds) * 1000),
                  this,
                  &AudioPlayer::onPlayingFinishedTimerTriggered);
            }
        }
    }
}
AudioPlayer::AudioPlayer(QObject* parent)
  : QObject(parent)
{
    connect(engine,
            &AudioEngine::changeDeviceRequested,
            this,
            &AudioPlayer::onDeviceChanged);

    connect(&playingFinishedTimer,
            &QTimer::timeout,
            this,
            &AudioPlayer::onPlayingFinishedTimerTriggered);
}

void
AudioPlayer::onPlayingFinishedTimerTriggered()
{
    stop();
}

void
AudioPlayer::setPlaying(bool value)
{
    auto wasPlaying = isPlaying();
    if (value) {
        play();
    } else {
        stop();
    }
    if (wasPlaying != isPlaying()) {
        emit playingChanged();
    }
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
    auto fileinfo = QFileInfo(value);
    if (fileinfo.suffix().isEmpty()) {
        auto suffixes = QStringList{ "wav", "flac", "ogg", "mp3" };
        for (const auto& suffix : suffixes) {
            if (auto testPath = value + "." + suffix;
                QFileInfo::exists(testPath)) {
                source = testPath;
                break;
            }
        }
    }
    emit sourceChanged();
    auto previousLength = length;
    if (ma_sound_init_from_file_w(engine->getEngine(),
                                  source.toStdWString().c_str(),
                                  MA_SOUND_FLAG_NO_PITCH |
                                    MA_SOUND_FLAG_NO_SPATIALIZATION,
                                  nullptr,
                                  nullptr,
                                  sound.get()) != MA_SUCCESS) {
        spdlog::error("Failed to load sound: {}", value.toStdString());
        sound.reset();
        stop();
        playingFinishedTimer.setInterval(0);
        length = 0.0f;
        if (previousLength != length) {
            emit lengthChanged();
        }
        return;
    }
    ma_sound_set_looping(sound.get(), looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), volume);
    ma_sound_set_fade_in_milliseconds(sound.get(), 0, volume, fadeInMillis);
    ma_sound_get_length_in_seconds(sound.get(), &length);
    playingFinishedTimer.setInterval(static_cast<int>(length * 1000));
    if (previousLength != length) {
        emit lengthChanged();
    }
    if (isPlaying()) {
        if (ma_sound_start(sound.get()) != MA_SUCCESS) {
            spdlog::error("Failed to play sound: {}", source.toStdString());
            stop();
        }
        if (!looping) {
            playingFinishedTimer.start();
        }
    }
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
    return playing;
}
void
AudioPlayer::play()
{
    if (isPlaying()) {
        return;
    }
    if (sound) {
        stop();
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_set_fade_in_milliseconds(sound.get(), 0, volume, fadeInMillis);
        if (ma_sound_start(sound.get()) != MA_SUCCESS) {
            spdlog::error("Failed to play sound: {}", source.toStdString());
        }
        if (!looping) {
            playingFinishedTimer.start();
        }
    }
    playing = true;
    emit playingChanged();
}
void
AudioPlayer::stop()
{
    if (!isPlaying()) {
        return;
    }
    if (sound) {
        ma_sound_set_fade_in_milliseconds(sound.get(), -1, 0.0f, fadeInMillis);
        ma_sound_stop(sound.get());
    }
    playing = false;
    playingFinishedTimer.stop();
    emit playingChanged();
}
auto
AudioPlayer::getVolume() const -> float
{
    return volume;
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
    }
    emit volumeChanged();
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
    }
    // Manage timer based on looping state
    if (isPlaying()) {
        if (looping) {
            playingFinishedTimer.stop();
        } else {
            playingFinishedTimer.start();
        }
    }
    emit loopingChanged();
}

auto
AudioPlayer::getFadeInMillis() const -> uint64_t
{
    return fadeInMillis;
}
void
AudioPlayer::setFadeInMillis(uint64_t value)
{
    if (fadeInMillis == value) {
        return;
    }
    fadeInMillis = value;
    emit fadeInMillisChanged();
}
auto
AudioPlayer::getLength() const -> float
{
    return length;
}
} // namespace sounds