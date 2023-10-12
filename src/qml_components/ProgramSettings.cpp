//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <utility>

namespace qml_components {
auto
ProgramSettings::getChartPath() const -> QString
{
    return chartPath;
}
ProgramSettings::ProgramSettings(QString chartPath,
                                 QString avatarFolder,
                                 QObject* parent)
  : QObject(parent)
  , chartPath(std::move(chartPath))
  , avatarFolder(std::move(avatarFolder))
{
}
auto
ProgramSettings::getAvatarFolder() const -> QString
{
    return avatarFolder;
}
} // namespace qml_components