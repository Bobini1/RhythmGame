//
// Created by PC on 10/09/2025.
//

#ifndef RHYTHMGAME_AUDIOPLAYER_H
#define RHYTHMGAME_AUDIOPLAYER_H
#include "Sound.h"

#include <QObject>
#include <QTimer>
#include <qqmlintegration.h>

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
    Q_PROPERTY(int64_t fadeInMillis READ getFadeInMillis WRITE setFadeInMillis
                 NOTIFY fadeInMillisChanged)
    Q_PROPERTY(
      bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)

    QString source;
    std::unique_ptr<ma_sound> sound;
    double volume = 1.0;
    bool looping = false;
    bool playing = false;
    int64_t fadeInMillis = 0;
    int64_t fadeOutMillis = 0;
    QTimer playingFinishedTimer;
    void onDeviceChanged();
    void onPlayingFinishedTimerTriggered();

  public:
    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;
    auto getSource() const -> QString;
    void setSource(const QString& value);
    void resetSource();
    void setPlaying(bool value);
    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    auto isPlaying() const -> bool;

    auto getVolume() const -> float;
    void setVolume(float value);
    auto isLooping() const -> bool;
    void setLooping(bool value);
    auto getFadeInMillis() const -> int64_t;
    void setFadeInMillis(int64_t value);
    inline static AudioEngine* engine = nullptr;
  signals:
    void sourceChanged();
    void volumeChanged();
    void loopingChanged();
    void autoPlayChanged();
    void fadeInMillisChanged();
    void playingChanged();
};

} // namespace sounds

#endif // RHYTHMGAME_AUDIOPLAYER_H
