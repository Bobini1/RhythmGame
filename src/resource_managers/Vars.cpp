//
// Created by bobini on 05.04.24.
//

#include <spdlog/spdlog.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJSEngine>
#include "Vars.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "support/QStringToPath.h"
#include <chrono>

void
writeGlobalVars(const QQmlPropertyMap& globalVars,
                const std::filesystem::path& profileFolder)
{
    auto now = std::chrono::high_resolution_clock::now();
    auto json = QJsonObject();
    for (const auto& key : globalVars.keys()) {
        try {
            json[key] = globalVars[key].toJsonValue();
        } catch (const std::exception& exception) {
            spdlog::error("Can't save global var with key {}",
                          key.toStdString());
        }
    }
    auto file = QFile{ profileFolder / "globalVars.json" };
    if (!file.open(QIODevice::WriteOnly)) {
        spdlog::error("Failed to open config for reading: {}",
                      profileFolder.string());
        return;
    }
    auto jsonDocument = QJsonDocument();
    jsonDocument.setObject(json);
    file.write(jsonDocument.toJson());
    spdlog::error("Saved global vars in {} us",
                  std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - now)
                    .count());
}

void
writeThemeVarsForTheme(const QQmlPropertyMap& themeVars,
                       const std::filesystem::path& path,
                       std::span<const QString> screens)
{
    auto file = QFile{ path };
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open config for reading + writing: {}",
                      path.string());
        return;
    }
    auto jsonDocument = QJsonDocument::fromJson(file.readAll());
    if (!jsonDocument.isObject()) {
        jsonDocument.setObject(QJsonObject());
    }
    auto json = jsonDocument.object();
    for (const auto& screen : screens) {
        auto screenObject = QJsonObject();
        const auto* screenPropertyMap =
          themeVars[screen].value<QQmlPropertyMap*>();
        if (screenPropertyMap == nullptr) {
            json[screen] = screenObject;
            continue;
        }
        for (const auto& key : screenPropertyMap->keys()) {
            try {
                screenObject[key] = screenPropertyMap->value(key).toJsonValue();
            } catch (std::exception& exception) {
                spdlog::error("{}", exception.what());
            }
        }
        json[screen] = screenObject;
    }
    file.reset();
    file.write(jsonDocument.toJson());
}

void
populateGlobalVars(QQmlPropertyMap& globalVars)
{
    globalVars.insert({ { "greenNumber", 200 },
                        { "whiteNumber", 200 },
                        { "liftOn", false },
                        { "liftHeight", 0 },
                        { "sudden", true } });
    globalVars.freeze();
}

void
readGlobalVars(QQmlPropertyMap& globalVars,
               const std::filesystem::path& profileFolder)
{
    populateGlobalVars(globalVars);
    auto file = QFile{ profileFolder / "globalVars.json" };
    if (!file.exists()) {
        writeGlobalVars(globalVars, profileFolder);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}",
                      profileFolder.string());
        return;
    }
    try {
        auto contents = QJsonDocument::fromJson(file.readAll()).object();
        globalVars.insert(contents.toVariantHash());
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
}

void
readThemeVarsForTheme(QQmlPropertyMap& themeVars,
                      const std::filesystem::path& path,
                      std::span<const QString> screens)
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
        auto contents = QJsonDocument::fromJson(file.readAll()).object();
        for (const auto& screen : screens) {
            auto* propertyMap = new QQmlPropertyMap(&themeVars);
            themeVars[screen] = QVariant::fromValue(propertyMap);
            propertyMap->insert(contents[screen].toObject().toVariantHash());
            QObject::connect(
              propertyMap,
              &QQmlPropertyMap::valueChanged,
              [propertyMap, path, screens = QList<QString>(screen)](
                const QString& key, const QVariant& value) {
                  writeThemeVarsForTheme(*propertyMap, path, screens);
              });
        }
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
}

void
readThemeVars(QQmlPropertyMap& themeVars,
              const QQmlPropertyMap& themeConfig,
              const std::filesystem::path& profileFolder)
{
    auto themeScreens = QHash<QString, QList<QString>>{};
    for (const auto& key : themeConfig.keys()) {
        const auto& value = themeConfig[key].toString();
        themeScreens[value] += key;
    }
    for (const auto& [theme, screens] : themeScreens.asKeyValueRange()) {
        readThemeVarsForTheme(
          themeVars,
          profileFolder /
            support::qStringToPath(theme + QStringLiteral("-vars.json")),
          screens);
    }
}

resource_managers::Vars::Vars(qml_components::ProfileList* profileList,
                              QObject* parent)
  : QObject(parent)
  , profileList(profileList)
{
    connect(profileList,
            &qml_components::ProfileList::currentProfileChanged,
            this,
            &Vars::onProfileChanged);
    onProfileChanged(profileList->getCurrentProfile());
    connect(
      globalVars.get(),
      &QQmlPropertyMap::valueChanged,
      [this](const QString& key, const QVariant& value) {
          writeGlobalVars(
            *globalVars,
            this->profileList->getCurrentProfile()->getPath().parent_path() /
              "globalVars.json");
      });
}
auto
resource_managers::Vars::getGlobalVars() -> QQmlPropertyMap*
{
    return globalVars.get();
}
auto
resource_managers::Vars::getThemeVars() -> QQmlPropertyMap*
{
    return themeVars.get();
}
void
resource_managers::Vars::onProfileChanged(resource_managers::Profile* profile)
{
    disconnect(currentThemeConfigConnection);
    const auto* themeConfig = profile->getThemeConfig();
    currentThemeConfigConnection = connect(themeConfig,
                                           &QQmlPropertyMap::valueChanged,
                                           this,
                                           &Vars::onThemeConfigChanged);
    globalVars = std::make_unique<QQmlPropertyMap>();
    themeVars = std::make_unique<QQmlPropertyMap>();
    readGlobalVars(*globalVars, profile->getPath().parent_path());
    readThemeVars(*themeVars, *themeConfig, profile->getPath().parent_path());
    emit globalVarsChanged();
    emit themeVarsChanged();
}
void
resource_managers::Vars::onThemeConfigChanged(const QString& key,
                                              const QVariant& value)
{
    themeVars->value(key).value<QQmlPropertyMap*>()->deleteLater();
    auto screens = QList<QString>{ key };
    readThemeVarsForTheme(
      *themeVars,
      profileList->getCurrentProfile()->getPath().parent_path() /
        support::qStringToPath(value.toString() + QStringLiteral("-vars.json")),
      screens);
}
