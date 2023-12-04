//
// Created by bobini on 25.11.23.
//

#ifndef SCANSKINS_H
#define SCANSKINS_H
#include "qml_components/ThemeFamily.h"

#include <QMap>
#include <QQmlPropertyMap>
#include <filesystem>

namespace resource_managers {
auto
scanThemes(std::filesystem::path themesFolder)
  -> QMap<QString, qml_components::ThemeFamily>;
void
fillWithDefaults(QQmlPropertyMap& object,
                 const QMap<QString, qml_components::ThemeFamily>& themes);
} // namespace resource_managers

#endif // SCANSKINS_H
