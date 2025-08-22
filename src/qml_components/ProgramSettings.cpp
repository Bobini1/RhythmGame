//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <utility>

namespace qml_components {
ProgramSettings::ProgramSettings(QString avatarFolder,
                                 QObject* parent)
  : QObject(parent)
  , avatarFolder(std::move(avatarFolder))
{
}
auto
ProgramSettings::getAvatarFolder() const -> QString
{
    return avatarFolder;
}
} // namespace qml_components