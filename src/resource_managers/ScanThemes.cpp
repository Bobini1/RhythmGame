//
// Created by bobini on 25.11.23.
//

#include "ScanThemes.h"

#include "qml_components/FileQuery.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

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
            const auto config = QJsonDocument::fromJson(file.readAll());
            const auto& scripts = config["scripts"];
            if (!scripts.isObject() || scripts.toObject().isEmpty()) {
                continue;
            }
            auto settings = config["settings"];
            if (!settings.isObject() || settings.toObject().isEmpty()) {
                settings = QJsonObject();
            }
            auto settingsScripts = config["settingsScripts"];
            if (!settingsScripts.isObject() ||
                settingsScripts.toObject().isEmpty()) {
                settingsScripts = QJsonObject();
            }

            auto themeMap = QMap<QString, qml_components::Screen>();
            for (const auto& [key, value] :
                 scripts.toObject().toVariantMap().asKeyValueRange()) {
                auto settingsUrl =
                  settings.toObject().contains(key)
                    ? QUrl::fromLocalFile(support::pathToQString(
                        path / support::qStringToPath(
                                 settings.toObject()[key].toString())))
                    : QUrl("");
                auto settingsScriptUrl =
                  settingsScripts.toObject().contains(key)
                    ? QUrl::fromLocalFile(support::pathToQString(
                        path / support::qStringToPath(
                                 settingsScripts.toObject()[key].toString())))
                    : QUrl("");

                themeMap[key] = {
                    QUrl::fromLocalFile(support::pathToQString(
                      path / support::qStringToPath(value.toString()))),
                    settingsUrl,
                    settingsScriptUrl,
                };
            }
            auto themeName = support::pathToQString(path.filename());
            auto translations = config["translations"].toString();
            auto themeFamily = qml_components::ThemeFamily{
                support::pathToQString(path),
                std::move(themeMap),
                translations.isEmpty()
                  ? QUrl{}
                  : QUrl::fromLocalFile(support::pathToQString(
                      path / support::qStringToPath(translations)))
            };
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
             themeFamily.getScreens().asKeyValueRange()) {
            if (name == QStringLiteral("Default") || !object.contains(screen)) {
                object.insert(screen, name);
            }
        }
    }
}