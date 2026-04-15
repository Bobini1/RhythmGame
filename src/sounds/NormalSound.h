//
// Created by PC on 24/02/2026.
//

#ifndef RHYTHMGAME_NORMALSOUND_H
#define RHYTHMGAME_NORMALSOUND_H

#include "Sound.h"
#include <QObject>

namespace sounds {
class AudioEngine;
class SoundBuffer;
class NormalSound
  : public QObject
  , public Sound
{
    Q_OBJECT

    AudioEngine* engine;
    std::shared_ptr<const SoundBuffer> buffer;
    std::unique_ptr<ma_audio_buffer> audioBuffer =
      std::make_unique<ma_audio_buffer>();
    std::unique_ptr<ma_sound> sound = std::make_unique<ma_sound>();

    void onDeviceChanged();

  public:
    NormalSound(AudioEngine* engine, std::shared_ptr<const SoundBuffer> buffer);
    ~NormalSound() override;
    auto getBuffer() const -> const std::shared_ptr<const SoundBuffer>&;
    void play() override;
    void stop() override;
    void setVolume(float volume) override;
    auto isPlaying() const -> bool override;
    auto getVolume() const -> float override;
};
} // namespace sounds

#endif // RHYTHMGAME_NORMALSOUND_H
