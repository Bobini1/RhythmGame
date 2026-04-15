//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <utility>
#include <QGuiApplication>
#include <QClipboard>
#include <QImage>

namespace qml_components {
ProgramSettings::ProgramSettings(QString avatarFolder,
                                 QString screenshotsFolder,
                                 QObject* parent)
  : QObject(parent)
  , avatarFolder(std::move(avatarFolder))
  , screenshotsFolder(std::move(screenshotsFolder))
{
}
auto
ProgramSettings::getAvatarFolder() const -> QString
{
    return avatarFolder;
}
auto
ProgramSettings::getScreenshotsFolder() const -> QString
{
    return screenshotsFolder;
}
auto
ProgramSettings::copyImageToClipboard(const QString& path) -> void
{
    QImage image(path);
    if (!image.isNull()) {
        QGuiApplication::clipboard()->setImage(image);
    }
}
} // namespace qml_components

