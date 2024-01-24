//
// Created by bobini on 29.11.23.
//

#ifndef CONFIGSERIALIZER_H
#define CONFIGSERIALIZER_H
#include "qml_components/ThemeFamily.h"

#include <QQmlPropertyMap>
#include <filesystem>

namespace resource_managers {

auto
writeConfig(const std::filesystem::path& path, QQmlPropertyMap& object) -> void;
auto
readConfig(const std::filesystem::path& path,
           QQmlPropertyMap& object,
           const QMap<QString, qml_components::ThemeFamily>& themeFamilies)
  -> void;

} // namespace resource_managers

#endif // CONFIGSERIALIZER_H
