//
// Created by PC on 04/09/2025.
//

#ifndef RHYTHMGAME_AUDIOENGINE_H
#define RHYTHMGAME_AUDIOENGINE_H
extern "C" {
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
}
#include <miniaudio.h>
#include <QSettings>
namespace sounds {
class Sound;

class AudioEngine : public QObject
{
    Q_OBJECT

  public:
    struct DeviceId
    {
        QString name;
        ma_device_id id;
    };
    AudioEngine();
    ~AudioEngine() override;
    void setDeviceImpl(const QString& deviceName);
    void setBackendImpl(const QString& backendName);

    Q_PROPERTY(
      QString backend READ getBackend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(
      QString device READ getDevice WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(
      QStringList backendNames READ getBackendNames NOTIFY backendNamesChanged)
    Q_PROPERTY(
      QStringList deviceNames READ getDeviceNames NOTIFY deviceNamesChanged)

    AudioEngine(const AudioEngine&) = delete;
    auto operator=(const AudioEngine&) -> AudioEngine& = delete;
    AudioEngine(AudioEngine&&) = delete;
    auto operator=(AudioEngine&&) -> AudioEngine& = delete;

    auto getResourceManager() -> ma_resource_manager*
    {
        return resourceManager.get();
    }

    auto getEngine() -> ma_engine* { return engine.get(); }

    auto getSampleRate() const -> int { return sampleRate; }

    auto getChannels() const -> int { return channels; }
    auto getBackend() const -> QString;
    void setBackend(const QString& backend);
    auto getDevice() const -> QString;
    void setDevice(const QString& device);
    auto getBackendNames() const -> QStringList;
    auto getDeviceNames() const -> QStringList;

  private:
    QSettings settings;
    QString currentBackend;
    QString currentDevice;
    std::vector<ma_backend> backends;
    QStringList backendNames;
    QStringList deviceNames;
    QList<ma_device_id> deviceIds;
    ma_log log{};
    std::unique_ptr<ma_context> context = std::make_unique<ma_context>();
    std::unique_ptr<ma_device> device = std::make_unique<ma_device>();
    std::unique_ptr<ma_resource_manager> resourceManager =
      std::make_unique<ma_resource_manager>();
    std::unique_ptr<ma_engine> engine = std::make_unique<ma_engine>();
    int sampleRate = 44100;
    int channels = 2;

  signals:
    void backendChanged();
    void deviceChanged();
    void changeDeviceRequested();
    void backendNamesChanged();
    void deviceNamesChanged();
};
} // namespace sounds

#endif // RHYTHMGAME_AUDIOENGINE_H
