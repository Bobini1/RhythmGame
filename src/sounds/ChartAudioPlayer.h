#ifndef RHYTHMGAME_CHARTAUDIOPLAYER_H
#define RHYTHMGAME_CHARTAUDIOPLAYER_H

#include "charts/BmsNotesData.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <qqmlintegration.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace resource_managers {
class SongAssetStore;
}

class QThreadPool;

namespace sounds {
class AudioEngine;
class Sound;

struct ChartAudioEvent
{
    std::chrono::nanoseconds timestamp;
    std::uint64_t soundId;

    auto operator==(const ChartAudioEvent&) const -> bool = default;
};

[[nodiscard]] auto
createChartAudioEvents(const charts::BmsNotesData& notes)
  -> std::vector<ChartAudioEvent>;

[[nodiscard]] auto
chartAudioLoopLength(std::span<const ChartAudioEvent> events,
                     std::chrono::nanoseconds chartLength)
  -> std::chrono::nanoseconds;

class ChartAudioPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ getSource WRITE setSource RESET resetSource
                 NOTIFY sourceChanged)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(
      bool looping READ isLooping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(quint64 fadeInMillis READ getFadeInMillis WRITE setFadeInMillis
                 NOTIFY fadeInMillisChanged)
    Q_PROPERTY(
      bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

    using SoundMap = std::unordered_map<std::uint64_t, std::shared_ptr<Sound>>;

    QString source;
    float volume = 1.0F;
    bool looping = false;
    bool playing = false;
    bool loaded = false;
    bool loading = false;
    quint64 fadeInMillis = 0;
    quint64 sourceGeneration = 0;
    std::shared_ptr<std::atomic_bool> sourceCancellation;
    std::vector<ChartAudioEvent> events;
    SoundMap sounds;
    std::unordered_set<std::uint64_t> activeSoundIds;
    std::chrono::nanoseconds loopLength{};
    std::chrono::steady_clock::time_point playbackStartedAt{};
    std::chrono::nanoseconds lastFadeUpdate{};
    std::size_t nextEvent = 0;
    float playbackGain = 1.0F;
    QTimer playbackTimer;

    void beginLoading();
    void cancelLoading();
    void clearLoadedChart();
    void retireLoadedSounds();
    void retireSounds(SoundMap retiredSounds);
    void startPlayback();
    void stopPlayback();
    void updatePlayback();
    void restartLoop();
    void scheduleNextUpdate(std::chrono::nanoseconds elapsed);
    void stopActiveSounds();
    void applyActiveVolume();
    void setLoaded(bool value);
    void setLoading(bool value);

  public:
    explicit ChartAudioPlayer(QObject* parent = nullptr);
    ~ChartAudioPlayer() override;

    [[nodiscard]] auto getSource() const -> QString;
    void setSource(const QString& value);
    void resetSource();

    [[nodiscard]] auto getVolume() const -> float;
    void setVolume(float value);
    [[nodiscard]] auto isLooping() const -> bool;
    void setLooping(bool value);
    [[nodiscard]] auto getFadeInMillis() const -> quint64;
    void setFadeInMillis(quint64 value);
    [[nodiscard]] auto isPlaying() const -> bool;
    void setPlaying(bool value);
    [[nodiscard]] auto isLoaded() const -> bool;
    [[nodiscard]] auto isLoading() const -> bool;

    Q_INVOKABLE void play();
    Q_INVOKABLE void stop();

    inline static AudioEngine* engine = nullptr;
    inline static resource_managers::SongAssetStore* assetStore = nullptr;
    inline static QThreadPool* threadPool = nullptr;

  signals:
    void sourceChanged();
    void volumeChanged();
    void loopingChanged();
    void fadeInMillisChanged();
    void playingChanged();
    void loadedChanged();
    void loadingChanged();
};

} // namespace sounds

#endif // RHYTHMGAME_CHARTAUDIOPLAYER_H
