//
// Created by bobini on 17.08.23.
//

#ifndef RHYTHMGAME_PROGRAMSETTINGS_H
#define RHYTHMGAME_PROGRAMSETTINGS_H

#include <atomic>
#include <chrono>

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

    QString avatarFolder;
    QString screenshotsFolder;
    QSettings settings;
    std::atomic<int> maxFps = 0;
    QMetaObject::Connection frameLimiterConnection;
    std::chrono::steady_clock::time_point previousFrameStart;
    int previousFrameRateLimit = 0;

    void limitFrameRate();

  public:
    explicit ProgramSettings(QString avatarFolder,
                             QString screenshotsFolder,
                             QObject* parent = nullptr);
    auto getAvatarFolder() const -> QString;
    auto getScreenshotsFolder() const -> QString;
    auto getMaxFps() const -> int;
    void setMaxFps(int value);
    void attachFrameRateLimiter(QQuickWindow* window);
    /**
     * @brief Copies the image at the given file path to the system clipboard.
     * @param path The absolute path to the image file.
     */
    Q_INVOKABLE void copyImageToClipboard(const QString& path);

  signals:
    void maxFpsChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
