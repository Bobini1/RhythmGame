//
// Created by PC on 10/09/2025.
//

#include "AudioPlayer.h"

#include "AudioEngine.h"
#include "resource_managers/SongAssetStore.h"
#include "support/PathToQString.h"
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <algorithm>

namespace sounds {
void
AudioPlayer::onDeviceChanged()
{
    stopOverlappingSounds();
    if (!sound || resolvedSource.isEmpty()) {
        return;
    }
    auto isPlayingNow = isPlaying();
    auto cursor = ma_uint64{};
    auto currentPcmFrame =
      ma_sound_get_cursor_in_pcm_frames(sound.get(), &cursor);
    ma_sound_uninit(sound.get());
    if (ma_sound_init_from_file_w(engine->getEngine(),
                                  resolvedSource.toStdWString().c_str(),
                                  MA_SOUND_FLAG_NO_PITCH |
                                    MA_SOUND_FLAG_NO_SPATIALIZATION,
                                  nullptr,
                                  nullptr,
                                  sound.get()) != MA_SUCCESS) {
        spdlog::error("Failed to load sound: {}", source.toStdString());
        sound.reset();
        setLoaded(false);
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

    overlappingCleanupTimer.setInterval(100);
    connect(&overlappingCleanupTimer,
            &QTimer::timeout,
            this,
            &AudioPlayer::cleanupOverlappingSounds);
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
    if (sourceCancellation) {
        sourceCancellation->store(true);
    }
    stopOverlappingSounds();
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
    if (sourceCancellation) {
        sourceCancellation->store(true);
        sourceCancellation.reset();
    }
    source = value;
    resolvedSource.clear();
    const auto generation = ++sourceGeneration;
    stopOverlappingSounds();
    if (sound) {
        ma_sound_uninit(sound.get());
        sound.reset();
    }
    setLoaded(false);
    playingFinishedTimer.stop();
    if (length != 0.0f) {
        length = 0.0f;
        emit lengthChanged();
    }
    emit sourceChanged();
    if (value.isEmpty()) {
        return;
    }
    if (resource_managers::SongAssetStore::isAudioUrl(value)) {
        if (!assetStore) {
            spdlog::error(
              "Cannot load archived sound before the song asset store is set");
            stop();
            return;
        }
        auto* store = assetStore;
        auto cancellation = std::make_shared<std::atomic_bool>(false);
        sourceCancellation = cancellation;
        auto* watcher = new QFutureWatcher<std::filesystem::path>{ this };
        connect(
          watcher,
          &QFutureWatcher<std::filesystem::path>::finished,
          this,
          [this, watcher, generation, value, cancellation] {
              try {
                  const auto localPath = watcher->result();
                  if (generation == sourceGeneration && source == value) {
                      loadResolvedSource(support::pathToQString(localPath));
                  }
              } catch (const std::exception& error) {
                  if (generation == sourceGeneration && source == value) {
                      spdlog::error("Failed to load archived sound {}: {}",
                                    value.toStdString(),
                                    error.what());
                      stop();
                  }
              }
              if (sourceCancellation == cancellation) {
                  sourceCancellation.reset();
              }
              watcher->deleteLater();
          });
        watcher->setFuture(QtConcurrent::run([store, value, cancellation] {
            return store->materialize(
              resource_managers::SongAssetStore::pathFromUrl(value),
              cancellation.get());
        }));
        return;
    }
    loadResolvedSource(value);
}

void
AudioPlayer::loadResolvedSource(QString value)
{
    auto fileinfo = QFileInfo(value);
    if (fileinfo.suffix().isEmpty()) {
        auto suffixes = QStringList{ "wav", "flac", "ogg", "mp3" };
        for (const auto& suffix : suffixes) {
            if (auto testPath = value + "." + suffix;
                QFileInfo::exists(testPath)) {
                value = testPath;
                break;
            }
        }
    }
    resolvedSource = value;
    sound = std::make_unique<ma_sound>();
    auto previousLength = length;
    if (ma_sound_init_from_file_w(engine->getEngine(),
                                  resolvedSource.toStdWString().c_str(),
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
    setLoaded(true);
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
    if (sourceCancellation) {
        sourceCancellation->store(true);
        sourceCancellation.reset();
    }
    ++sourceGeneration;
    stopOverlappingSounds();
    if (sound) {
        ma_sound_uninit(sound.get());
        sound.reset();
    }
    setLoaded(false);
    if (!source.isEmpty()) {
        source.clear();
        emit sourceChanged();
    }
    resolvedSource.clear();
}
auto
AudioPlayer::isPlaying() const -> bool
{
    return playing;
}
auto
AudioPlayer::isLoaded() const -> bool
{
    return loaded;
}
void
AudioPlayer::setLoaded(bool value)
{
    if (loaded == value) {
        return;
    }
    loaded = value;
    emit loadedChanged();
}
void
AudioPlayer::cleanupOverlappingSounds()
{
    auto finished = std::erase_if(overlappingSounds, [](auto& instance) {
        if (ma_sound_at_end(instance.get()) == 0) {
            return false;
        }
        ma_sound_uninit(instance.get());
        return true;
    });
    if (finished > 0 && overlappingSounds.empty()) {
        overlappingCleanupTimer.stop();
    }
}
void
AudioPlayer::stopOverlappingSounds()
{
    overlappingCleanupTimer.stop();
    for (const auto& instance : overlappingSounds) {
        ma_sound_stop(instance.get());
        ma_sound_uninit(instance.get());
    }
    overlappingSounds.clear();
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
AudioPlayer::playOverlapping()
{
    if (!sound) {
        return;
    }
    cleanupOverlappingSounds();
    auto instance = std::make_unique<ma_sound>();
    if (ma_sound_init_copy(engine->getEngine(),
                           sound.get(),
                           MA_SOUND_FLAG_NO_PITCH |
                             MA_SOUND_FLAG_NO_SPATIALIZATION,
                           nullptr,
                           instance.get()) != MA_SUCCESS) {
        spdlog::error("Failed to clone sound: {}", source.toStdString());
        return;
    }
    ma_sound_set_looping(instance.get(), MA_FALSE);
    ma_sound_set_volume(instance.get(), volume);
    ma_sound_set_fade_in_milliseconds(instance.get(), 0, volume, fadeInMillis);
    ma_sound_seek_to_pcm_frame(instance.get(), 0);
    if (ma_sound_start(instance.get()) != MA_SUCCESS) {
        spdlog::error("Failed to play sound: {}", source.toStdString());
        ma_sound_uninit(instance.get());
        return;
    }
    overlappingSounds.push_back(std::move(instance));
    if (!overlappingCleanupTimer.isActive()) {
        overlappingCleanupTimer.start();
    }
}
void
AudioPlayer::stop()
{
    stopOverlappingSounds();
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
