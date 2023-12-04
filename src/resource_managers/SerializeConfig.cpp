//
// Created by bobini on 29.11.23.
//

#include "SerializeConfig.h"

#include "support/PathToQString.h"

#include <QJsonDocument>
#include <QFile>
#include <fstream>
#include <spdlog/spdlog.h>
#include <QJsonObject>

namespace resource_managers {
auto
writeConfig(const std::filesystem::path& path, QQmlPropertyMap& object) -> void
{
    auto map = QVariantMap{};
    for (auto&& key : object.keys()) {
        map[key] = object.value(key);
    }
    // dump to json
    const auto json = QJsonDocument::fromVariant(map);
    auto file = QFile{ path };

    if (!file.open(QIODevice::WriteOnly)) {
        throw std::runtime_error{ "Failed to open file for writing " +
                                  path.string() };
    }

    file.write(json.toJson());
}
auto
readConfig(const std::filesystem::path& path,
           QQmlPropertyMap& object,
           QMap<QString, qml_components::ThemeFamily> theme_families) -> void
{
    auto file = QFile{ path };
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}", path.string());
        return;
    }
    try {
        for (const auto& [key, value] : QJsonDocument::fromJson(file.readAll())
                                          .object()
                                          .toVariantMap()
                                          .asKeyValueRange()) {
            if (object.contains(key)) {
                if (theme_families.contains(value.toString()) &&
                    theme_families[value.toString()].getThemes().contains(
                      key)) {
                    object.insert(key, value);
                }
            }
        }
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
}
} // namespace resource_managers