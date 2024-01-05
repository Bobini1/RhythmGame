//
// Created by bobini on 25.11.23.
//

#include "ScanThemes.h"

#include "qml_components/FileValidator.h"
#include "support/PathToQString.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
#include <QJsonValue>
#include <spdlog/spdlog.h>
auto
resource_managers::scanThemes(std::filesystem::path themesFolder)
  -> QMap<QString, qml_components::ThemeFamily>
{
    auto themeFamilies = QMap<QString, qml_components::ThemeFamily>{};
    for (const auto& entry :
         std::filesystem::directory_iterator(themesFolder)) {
        if (!entry.is_directory()) {
            continue;
        }
        const auto& path = entry.path();
        auto configPath = path / "theme.json";
        if (!exists(configPath)) {
            continue;
        }
        auto file = QFile(configPath);
        try {
            file.open(QIODevice::ReadOnly);
            auto config = QJsonDocument::fromJson(file.readAll());
            const auto& themes = config["scriptNames"];
            if (!themes.isObject() || themes.toObject().isEmpty()) {
                continue;
            }

            auto themeMap = QMap<QString, QUrl>();
            for (const auto& [key, value] :
                 themes.toObject().toVariantMap().asKeyValueRange()) {
                themeMap[key] = QUrl::fromLocalFile(support::pathToQString(path / value.toString().
#ifdef _WIN32
                                                              toStdWString()
#else
                                                              toStdString()
#endif
                ));
            }
            auto themeName = support::pathToQString(path.filename());
            auto themeFamily =
              qml_components::ThemeFamily{ support::pathToQString(path),
                                           std::move(themeMap) };
            themeFamilies.insert(themeName, themeFamily);
        } catch (const std::exception& e) {
            spdlog::error(
              "Failed to load theme {}: {}", path.string(), e.what());
        }
    }
    return themeFamilies;
}
void
resource_managers::fillWithDefaults(
  QQmlPropertyMap& object,
  const QMap<QString, qml_components::ThemeFamily>& themes)
{
    for (const auto& [name, themeFamily] : themes.asKeyValueRange()) {
        for (const auto& [screen, path] :
             themeFamily.getThemes().asKeyValueRange()) {
            if (name == QStringLiteral("Default") || !object.contains(screen)) {
                object.insert(screen, name);
            }
        }
    }
}