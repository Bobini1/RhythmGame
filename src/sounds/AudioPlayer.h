//
// Created by PC on 10/09/2025.
//

#ifndef RHYTHMGAME_AUDIOPLAYER_H
#define RHYTHMGAME_AUDIOPLAYER_H
#include "Sound.h"

#include <QObject>
#include <QTimer>
#include <qqmlintegration.h>

#include <atomic>
#include <memory>
#include <vector>

namespace sounds {
class AudioEngine;
}
namespace resource_managers {
class SongAssetStore;
}
namespace sounds {

class AudioPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ getSource WRITE setSource RESET resetSource
                 NOTIFY sourceChanged)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(
      bool looping READ isLooping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(double fadeInMillis READ getFadeInMillis WRITE setFadeInMillis
                 NOTIFY fadeInMillisChanged)
    Q_PROPERTY(
      bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    Q_PROPERTY(float length READ getLength NOTIFY lengthChanged)

    QString source;
    QString resolvedSource;
    std::unique_ptr<ma_sound> sound;
    std::vector<std::unique_ptr<ma_sound>> overlappingSounds;
    double volume = 1.0;
    bool looping = false;
    bool playing = false;
    bool loaded = false;
    uint64_t fadeInMillis = 0;
    float length = 0.0f;
    QTimer playingFinishedTimer;
    QTimer overlappingCleanupTimer;
    quint64 sourceGeneration = 0;
    std::shared_ptr<std::atomic_bool> sourceCancellation;
    void onDeviceChanged();
    void onPlayingFinishedTimerTriggered();
    void cleanupOverlappingSounds();
    void stopOverlappingSounds();
    void loadResolvedSource(QString value);
    void setLoaded(bool value);

  public:
    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;
    auto getSource() const -> QString;
    void setSource(const QString& value);
    void resetSource();
    void setPlaying(bool value);
    Q_INVOKABLE void play();
    Q_INVOKABLE void playOverlapping();
    Q_INVOKABLE void stop();
    auto isPlaying() const -> bool;
    auto isLoaded() const -> bool;

    auto getVolume() const -> float;
    void setVolume(float value);
    auto isLooping() const -> bool;
    void setLooping(bool value);
    auto getFadeInMillis() const -> uint64_t;
    void setFadeInMillis(uint64_t value);
    auto getLength() const -> float;
    inline static AudioEngine* engine = nullptr;
    inline static resource_managers::SongAssetStore* assetStore = nullptr;
  signals:
    void sourceChanged();
    void volumeChanged();
    void loopingChanged();
    void autoPlayChanged();
    void fadeInMillisChanged();
    void playingChanged();
    void loadedChanged();
    void lengthChanged();
};

} // namespace sounds

#endif // RHYTHMGAME_AUDIOPLAYER_H
