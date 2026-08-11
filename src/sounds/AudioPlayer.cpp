//
// Created by PC on 10/09/2025.
//

#include "AudioPlayer.h"

#include "AudioEngine.h"
#include "resource_managers/SongAssetStore.h"
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <algorithm>

namespace sounds {
void
AudioPlayer::onDeviceChanged()
{
    stopOverlappingSounds();
    if (!sound || (resolvedSource.isEmpty() && encodedSource.isEmpty())) {
        return;
    }
    auto isPlayingNow = isPlaying();
    auto cursor = ma_uint64{};
    ma_sound_get_cursor_in_pcm_frames(sound.get(), &cursor);
    ma_sound_uninit(sound.get());
    sound.reset();
    auto initialized = false;
    if (!encodedSource.isEmpty()) {
        clearMemoryDecoder();
        initialized = initializeMemorySound(memoryDecoder, sound);
    } else {
        sound = std::make_unique<ma_sound>();
        initialized =
          ma_sound_init_from_file_w(engine->getEngine(),
                                    resolvedSource.toStdWString().c_str(),
                                    MA_SOUND_FLAG_NO_PITCH |
                                      MA_SOUND_FLAG_NO_SPATIALIZATION,
                                    nullptr,
                                    nullptr,
                                    sound.get()) == MA_SUCCESS;
    }
    if (!initialized) {
        spdlog::error("Failed to load sound: {}", source.toStdString());
        sound.reset();
        setLoaded(false);
        return;
    }
    ma_sound_set_looping(sound.get(), looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(sound.get(), volume);
    ma_sound_set_fade_in_milliseconds(sound.get(), 0, volume, fadeInMillis);
    ma_sound_seek_to_pcm_frame(sound.get(), cursor);
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
    clearMemoryDecoder();
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
    clearMemoryDecoder();
    encodedSource.clear();
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
        auto* watcher = new QFutureWatcher<QByteArray>{ this };
        connect(watcher,
                &QFutureWatcher<QByteArray>::finished,
                this,
                [this, watcher, generation, value, cancellation] {
                    try {
                        if (generation == sourceGeneration && source == value) {
                            loadEncodedSource(watcher->result());
                        }
                    } catch (const std::exception& error) {
                        if (generation == sourceGeneration && source == value) {
                            spdlog::error(
                              "Failed to load archived sound {}: {}",
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
            return store->read(
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
    clearMemoryDecoder();
    encodedSource.clear();
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
        failLoadingSound(previousLength);
        return;
    }
    finishLoadingSound(previousLength);
}

void
AudioPlayer::finishLoadingSound(const float previousLength)
{
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
AudioPlayer::failLoadingSound(const float previousLength)
{
    sound.reset();
    setLoaded(false);
    stop();
    playingFinishedTimer.setInterval(0);
    length = 0.0f;
    if (previousLength != length) {
        emit lengthChanged();
    }
}

bool
AudioPlayer::initializeMemorySound(std::unique_ptr<ma_decoder>& decoder,
                                   std::unique_ptr<ma_sound>& targetSound)
{
    decoder = std::make_unique<ma_decoder>();
    const auto config =
      ma_decoder_config_init(ma_format_f32,
                             static_cast<ma_uint32>(engine->getChannels()),
                             static_cast<ma_uint32>(engine->getSampleRate()));
    if (ma_decoder_init_memory(encodedSource.constData(),
                               static_cast<size_t>(encodedSource.size()),
                               &config,
                               decoder.get()) != MA_SUCCESS) {
        decoder.reset();
        return false;
    }
    targetSound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_data_source(engine->getEngine(),
                                       &decoder->ds,
                                       MA_SOUND_FLAG_NO_PITCH |
                                         MA_SOUND_FLAG_NO_SPATIALIZATION,
                                       nullptr,
                                       targetSound.get()) != MA_SUCCESS) {
        targetSound.reset();
        ma_decoder_uninit(decoder.get());
        decoder.reset();
        return false;
    }
    return true;
}

void
AudioPlayer::clearMemoryDecoder()
{
    if (memoryDecoder) {
        ma_decoder_uninit(memoryDecoder.get());
        memoryDecoder.reset();
    }
}

void
AudioPlayer::loadEncodedSource(QByteArray value)
{
    resolvedSource.clear();
    encodedSource = std::move(value);
    const auto previousLength = length;
    if (encodedSource.isEmpty() ||
        !initializeMemorySound(memoryDecoder, sound)) {
        spdlog::error("Failed to load archived sound: {}",
                      source.toStdString());
        failLoadingSound(previousLength);
        return;
    }
    finishLoadingSound(previousLength);
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
    clearMemoryDecoder();
    encodedSource.clear();
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
        if (ma_sound_at_end(instance.sound.get()) == 0) {
            return false;
        }
        ma_sound_uninit(instance.sound.get());
        if (instance.memoryDecoder) {
            ma_decoder_uninit(instance.memoryDecoder.get());
        }
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
        ma_sound_stop(instance.sound.get());
        ma_sound_uninit(instance.sound.get());
        if (instance.memoryDecoder) {
            ma_decoder_uninit(instance.memoryDecoder.get());
        }
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
auto
AudioPlayer::playOverlapping() -> bool
{
    if (!sound) {
        return false;
    }
    cleanupOverlappingSounds();
    auto instance = OverlappingSound{};
    auto initialized = false;
    if (encodedSource.isEmpty()) {
        instance.sound = std::make_unique<ma_sound>();
        initialized = ma_sound_init_copy(engine->getEngine(),
                                         sound.get(),
                                         MA_SOUND_FLAG_NO_PITCH |
                                           MA_SOUND_FLAG_NO_SPATIALIZATION,
                                         nullptr,
                                         instance.sound.get()) == MA_SUCCESS;
    } else {
        initialized =
          initializeMemorySound(instance.memoryDecoder, instance.sound);
    }
    if (!initialized) {
        spdlog::error("Failed to clone sound: {}", source.toStdString());
        return false;
    }
    ma_sound_set_looping(instance.sound.get(), MA_FALSE);
    ma_sound_set_volume(instance.sound.get(), volume);
    ma_sound_set_fade_in_milliseconds(
      instance.sound.get(), 0, volume, fadeInMillis);
    ma_sound_seek_to_pcm_frame(instance.sound.get(), 0);
    if (ma_sound_start(instance.sound.get()) != MA_SUCCESS) {
        spdlog::error("Failed to play sound: {}", source.toStdString());
        ma_sound_uninit(instance.sound.get());
        if (instance.memoryDecoder) {
            ma_decoder_uninit(instance.memoryDecoder.get());
        }
        return false;
    }
    overlappingSounds.push_back(std::move(instance));
    if (!overlappingCleanupTimer.isActive()) {
        overlappingCleanupTimer.start();
    }
    return true;
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
