//
// Created by PC on 10/09/2025.
//

#ifndef RHYTHMGAME_AUDIOPLAYER_H
#define RHYTHMGAME_AUDIOPLAYER_H
#include "Sound.h"

#include <QObject>
#include <qqmlintegration.h>

namespace sounds {

class AudioPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(
      QString source READ getSource WRITE setSource RESET resetSource NOTIFY sourceChanged)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool looping READ isLooping WRITE setLooping NOTIFY loopingChanged)

    QString source;
    std::unique_ptr<ma_sound> sound;
    double volume = 1.0;
    bool looping = false;
    bool playing = false;
    void onDeviceChanged();
    auto isPlaying() const -> bool;

  public:
    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;
    auto getSource() const -> QString;
    void setSource(const QString& value);
    void resetSource();
    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();
    auto getVolume() const -> float;
    void setVolume(float value);
    auto isLooping() const -> bool;
    void setLooping(bool value);
    inline static AudioEngine* engine = nullptr;
  signals:
    void sourceChanged();
    void volumeChanged();
    void loopingChanged();
    void autoPlayChanged();
};

} // namespace sounds

#endif // RHYTHMGAME_AUDIOPLAYER_H
