//
// Created by bobini on 17.08.23.
//

#ifndef RHYTHMGAME_PROGRAMSETTINGS_H
#define RHYTHMGAME_PROGRAMSETTINGS_H

#include <QQmlEngine>
namespace qml_components {

class ProgramSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString avatarFolder READ getAvatarFolder CONSTANT)
    Q_PROPERTY(QString screenshotsFolder READ getScreenshotsFolder CONSTANT)

    QString avatarFolder;
    QString screenshotsFolder;

  public:
    explicit ProgramSettings(QString avatarFolder,
                             QString screenshotsFolder,
                             QObject* parent = nullptr);
    auto getAvatarFolder() const -> QString;
    auto getScreenshotsFolder() const -> QString;
    /**
     * @brief Copies the image at the given file path to the system clipboard.
     * @param path The absolute path to the image file.
     */
    Q_INVOKABLE void copyImageToClipboard(const QString& path);
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
