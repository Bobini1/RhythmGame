//
// Created by bobini on 17.08.23.
//

#ifndef RHYTHMGAME_PROGRAMSETTINGS_H
#define RHYTHMGAME_PROGRAMSETTINGS_H

#include <atomic>
#include <chrono>
#include <cstdint>

#include <QQmlEngine>
#include <QSettings>

class QQuickWindow;

namespace qml_components {

class ProgramSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString avatarFolder READ getAvatarFolder CONSTANT)
    Q_PROPERTY(QString screenshotsFolder READ getScreenshotsFolder CONSTANT)
    Q_PROPERTY(int maxFps READ getMaxFps WRITE setMaxFps NOTIFY maxFpsChanged)
    Q_PROPERTY(
      int presentationFps READ getPresentationFps NOTIFY presentationFpsChanged)

    QString avatarFolder;
    QString screenshotsFolder;
    QSettings settings;
    std::atomic<int> maxFps = 0;
    std::atomic<int> presentationFps = 0;
    QMetaObject::Connection frameLimiterConnection;
    QMetaObject::Connection presentationCounterConnection;
    std::chrono::steady_clock::time_point previousFrameStart;
    std::chrono::steady_clock::time_point presentationSampleStart;
    int previousFrameRateLimit = 0;
    int presentedFrameCount = 0;

    void limitFrameRate();
    void countPresentedFrame();

  public:
    explicit ProgramSettings(QString avatarFolder,
                             QString screenshotsFolder,
                             QObject* parent = nullptr);
    auto getAvatarFolder() const -> QString;
    auto getScreenshotsFolder() const -> QString;
    auto getMaxFps() const -> int;
    auto getPresentationFps() const -> int;
    void setMaxFps(int value);
    void attachFrameRateLimiter(QQuickWindow* window);
    /**
     * @brief Copies the image at the given file path to the system clipboard.
     * @param path The absolute path to the image file.
     */
    Q_INVOKABLE void copyImageToClipboard(const QString& path);

  signals:
    void maxFpsChanged();
    void presentationFpsChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
