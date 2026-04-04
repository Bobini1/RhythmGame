//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <utility>

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
} // namespace qml_components

