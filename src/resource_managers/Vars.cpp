//
// Created by bobini on 05.04.24.
//

#include <spdlog/spdlog.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJSEngine>
#include "Vars.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include "support/Exception.h"

#include <QQmlEngine>
#include <chrono>
#include <qcolor.h>
#include <utility>

void
writeGlobalVars(const QQmlPropertyMap& globalVars,
                const std::filesystem::path& profileFolder)
{
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
        spdlog::error("Failed to open config for writing: {}",
                      profileFolder.string());
        return;
    }
    auto jsonDocument = QJsonDocument();
    jsonDocument.setObject(json);
    file.write(jsonDocument.toJson());
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
}

auto
jsonValueToString(const QJsonValue& value) -> std::string
{
    return QJsonDocument(value.toObject()).toJson().toStdString();
}

void
createFileProperty(QHash<QString, QVariant>& screenVars,
                   const QJsonObject& object,
                   const std::filesystem::path& themePath)
{
    const auto path = object["path"];
    if (!object["path"].isString()) {
        throw support::Exception(
          std::format("path field of property of type file is undefined or not "
                      "a string: {}",
                      jsonValueToString(object)));
    }
    if (object.contains("default") && !object["default"].isString()) {
        throw support::Exception(std::format(
          "default field of property of type file is not a string: {}",
          jsonValueToString(object)));
    }
    if (object["default"].isString()) {
        const auto defaultPath =
          support::qStringToPath(object["default"].toString());
        if (!defaultPath.is_relative()) {
            throw support::Exception(
              std::format("default field of property of type file is not a "
                          "relative path: {}",
                          jsonValueToString(object)));
        }
        if (!exists(themePath / defaultPath)) {
            spdlog::debug("default file of property of type file does not "
                          "exist, will pick the "
                          "first one available instead: {}",
                          jsonValueToString(object));
        } else {
            screenVars[object["id"].toString()] =
              QVariant(support::pathToQString(defaultPath));
            return;
        }
    }
    std::vector<std::filesystem::path> files;
    for (const auto& file : std::filesystem::directory_iterator(
           themePath / support::qStringToPath(path.toString()))) {
        files.push_back(file.path());
    }
    if (files.empty()) {
        spdlog::debug("No files found for property of type file: {}",
                      jsonValueToString(object));
        screenVars[object["id"].toString()] = QJsonValue::Null;
    } else {
        screenVars[object["id"].toString()] =
          QVariant(support::pathToQString(relative(files.front(), themePath)));
    }
}

void
createColorProperty(QHash<QString, QVariant>& screenVars,
                    const QJsonObject& object)
{
    if (!object["default"].isString()) {
        throw support::Exception(
          std::format("default field of property of type color is undefined or "
                      "not a string: {}",
                      jsonValueToString(object)));
    }
    if (!QColor::isValidColorName(object["default"].toString())) {
        throw support::Exception(
          std::format("default field of property of type color is not a valid "
                      "color name: {}",
                      jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toString();
}

void
createChoiceProperty(QHash<QString, QVariant>& screenVars,
                     const QJsonObject& object)
{
    if (!object["default"].isString()) {
        throw support::Exception(std::format(
          "default field of property of type choice is undefined or "
          "not a string: {}",
          jsonValueToString(object)));
    }
    if (!object["choices"].isArray()) {
        throw support::Exception(std::format(
          "choices field of property of type choice is undefined or "
          "not an array: {}",
          jsonValueToString(object)));
    }
    // confirm that the default value is one of the choices
    const auto choices = object["choices"].toArray();
    if (choices.isEmpty()) {
        throw support::Exception(
          std::format("choices field of property of type choice is empty: {}",
                      jsonValueToString(object)));
    }
    if (std::ranges::find(choices, object["default"]) ==
        std::ranges::end(choices)) {
        throw support::Exception(std::format(
          "default field of property of type choice is not one of the choices: "
          "{}",
          jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toString();
}

void
createCheckBoxProperty(QHash<QString, QVariant>& screenVars,
                       const QJsonObject& object)
{
    if (!object["default"].isBool()) {
        throw support::Exception(std::format(
          "default field of property of type checkbox is undefined or "
          "not a boolean: {}",
          jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toBool();
}

void
createStringProperty(QHash<QString, QVariant>& screenVars,
                     const QJsonObject& object)
{
    if (!object["default"].isString()) {
        throw support::Exception(std::format(
          "default field of property of type string is undefined or "
          "not a string: {}",
          jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toString();
}

void
createRangeProperty(QHash<QString, QVariant>& screenVars,
                    const QJsonObject& object)
{
    if (!object["default"].isDouble()) {
        throw support::Exception(
          std::format("default field of property of type range is undefined or "
                      "not a number: {}",
                      jsonValueToString(object)));
    }
    if (object.contains("min") && !object["min"].isDouble()) {
        throw support::Exception(
          std::format("min field of property of type range is "
                      "not a number: {}",
                      jsonValueToString(object)));
    }
    if (object.contains("max") && !object["max"].isDouble()) {
        throw support::Exception(
          std::format("max field of property of type range is "
                      "not a number: {}",
                      jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toDouble();
}

void
createHiddenProperty(QHash<QString, QVariant>& screenVars,
                     const QJsonObject& object)
{
    screenVars[object["id"].toString()] =
      object["default"].isUndefined() ? QJsonValue::Null : object["default"];
}

void
createProperty(QHash<QString, QVariant>& screenVars,
               const std::filesystem::path& themePath,
               const QJsonObject& object)
{
    if (!object["id"].isString()) {
        throw support::Exception(
          std::format("Property has no id (or not a string): {}",
                      jsonValueToString(object)));
    }
    if (!object["type"].isString()) {
        throw support::Exception(
          std::format("Property has no type (or not a string): {}",
                      jsonValueToString(object)));
    }
    if (object["type"] == "file") {
        createFileProperty(screenVars, object, themePath);
    } else if (object["type"] == "color") {
        createColorProperty(screenVars, object);
    } else if (object["type"] == "choice") {
        createChoiceProperty(screenVars, object);
    } else if (object["type"] == "checkbox") {
        createCheckBoxProperty(screenVars, object);
    } else if (object["type"] == "string") {
        createStringProperty(screenVars, object);
    } else if (object["type"] == "range") {
        createRangeProperty(screenVars, object);
    } else if (object["type"] == "hidden") {
        createHiddenProperty(screenVars, object);
    } else {
        throw support::Exception(std::format("Property has unknown type: {}",
                                             jsonValueToString(object)));
    }
}

void
populateScreenVarsRecursive( // NOLINT(*-no-recursion)
  QHash<QString, QVariant>& screenVars,
  const std::filesystem::path& themePath,
  const QJsonArray& array)
{
    for (const auto& property : array) {
        const auto object = property.toObject();
        if (!object["type"].isString()) {
            throw support::Exception(
              std::format("Property has no type (or not a string): {}",
                          jsonValueToString(object)));
        }
        if (object["type"] == "group") {
            if (!object["items"].isArray() ||
                object["items"].toArray().empty()) {
                throw support::Exception(std::format(
                  "Property group has no items (or items is not an array): {}",
                  jsonValueToString(object)));
            }
            populateScreenVarsRecursive(
              screenVars, themePath, object["items"].toArray());
        } else {
            createProperty(screenVars, themePath, object);
        }
    }
}

auto
populateScreenVars(const std::filesystem::path& themePath,
                   const std::filesystem::path& settingsPath)
  -> QHash<QString, QVariant>

{
    auto vars = QHash<QString, QVariant>{};
    try {
        auto file = QFile{ settingsPath };
        if (!file.exists()) {
            return {};
        }
        if (!file.open(QIODevice::ReadOnly)) {
            spdlog::error("Failed to open config for reading: {}",
                          settingsPath.string());
        }
        const auto contents = QJsonDocument::fromJson(file.readAll());
        if (!contents.isArray()) {
            throw support::Exception(std::format(
              "Settings file is not an array: {}", settingsPath.string()));
        }
        populateScreenVarsRecursive(vars, themePath, contents.array());
    } catch (const std::exception& exception) {
        throw support::Exception(
          std::format("Failed to populate screen vars for screen {}: {}",
                      settingsPath.string(),
                      exception.what()));
    }
    return vars;
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
        globalVars.freeze();
        return;
    }
    try {
        const auto contents = QJsonDocument::fromJson(file.readAll()).object();
        globalVars.insert(contents.toVariantHash());
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
    globalVars.freeze();
}

auto
readThemeVarsForTheme(const std::filesystem::path& themeVarsPath,
                      const std::filesystem::path& themePath,
                      const qml_components::ThemeFamily& themeFamily)
  -> QHash<QString, QHash<QString, QVariant>>
{
    auto file = QFile{ themeVarsPath };
    auto vars = QHash<QString, QHash<QString, QVariant>>{};
    for (const auto& screen : themeFamily.getScreens().keys()) {
        auto settingsUrl = themeFamily.getScreens()[screen]
                             .value<qml_components::Screen>()
                             .getSettings();
        if (settingsUrl.isEmpty()) {
            continue;
        }
        auto settingsPath = support::qStringToPath(settingsUrl.toLocalFile());
        vars[screen] = populateScreenVars(themePath, settingsPath);
    }
    if (!file.exists()) {
        return vars;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}",
                      themeVarsPath.string());
        return vars;
    }
    try {
        auto contents = QJsonDocument::fromJson(file.readAll()).object();
        for (const auto& screen : themeFamily.getScreens().keys()) {
            vars[screen].insert(contents[screen].toObject().toVariantHash());
        }
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
    return vars;
}

auto
readThemeVars(const std::filesystem::path& profileFolder,
              const QMap<QString, qml_components::ThemeFamily>& themeFamilies)
  -> QHash<QString, QHash<QString, QHash<QString, QVariant>>>
{
    auto vars = QHash<QString, QHash<QString, QHash<QString, QVariant>>>{};
    for (const auto& [name, themeFamily] : themeFamilies.asKeyValueRange()) {
        auto themePath = support::qStringToPath(themeFamily.getPath());
        vars[name] = readThemeVarsForTheme(
          profileFolder /
            support::qStringToPath(name + QStringLiteral("-vars.json")),
          themePath,
          themeFamily);
    }
    return vars;
}

void
populateThemePropertyMap(
  QQmlPropertyMap& themeVars,
  QHash<QString, QHash<QString, QHash<QString, QVariant>>> themeVarsData,
  const std::filesystem::path& themeVarsPath,
  const QQmlPropertyMap& themeConfig)
{
    for (const auto& key : themeVars.keys()) {
        themeVars[key].value<QQmlPropertyMap*>()->deleteLater();
    }
    for (const auto& key : themeConfig.keys()) {
        const auto& value = themeConfig[key].toString();
        auto propertyMap = std::make_unique<QQmlPropertyMap>(&themeVars);
        propertyMap->insert(themeVarsData[value][key]);
        propertyMap->freeze();
        QObject::connect(
          propertyMap.get(),
          &QQmlPropertyMap::valueChanged,
          [propertyMap, themeVarsPath, screens = QList<QString>(key)](
            const QString& key, const QVariant& value) {
              writeThemeVarsForTheme(*propertyMap, themeVarsPath, screens);
          });
        themeVars.insert(key, QVariant::fromValue(propertyMap.release()));
    }
    themeVars.freeze();
}

resource_managers::Vars::
Vars(const Profile* profile,
     QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
     QObject* parent)
  : QObject(parent)
  , profile(profile)
  , availableThemeFamilies(std::move(availableThemeFamilies))
  , loadedThemeVars(readThemeVars(profile->getPath().parent_path(),
                                  this->availableThemeFamilies))
{
    const auto* themeConfig = profile->getThemeConfig();
    populateThemePropertyMap(themeVars,
                             loadedThemeVars,
                             profile->getPath().parent_path(),
                             *themeConfig);
    connect(themeConfig,
            &QQmlPropertyMap::valueChanged,
            this,
            &Vars::onThemeConfigChanged);
    connect(&globalVars,
            &QQmlPropertyMap::valueChanged,
            this,
            [this](const QString& key, const QVariant& value) {
                writeGlobalVars(globalVars,
                                this->profile->getPath().parent_path() /
                                  "globalVars.json");
            });
    readGlobalVars(globalVars, profile->getPath().parent_path());
}
auto
resource_managers::Vars::getGlobalVars() -> QQmlPropertyMap*
{
    return &globalVars;
}
auto
resource_managers::Vars::getThemeVars() -> QQmlPropertyMap*
{
    return &themeVars;
}
void
resource_managers::Vars::onThemeConfigChanged(const QString& key,
                                              const QVariant& value)
{
    themeVars.value(key).value<QQmlPropertyMap*>()->deleteLater();
    auto propertyMap = std::make_unique<QQmlPropertyMap>(&themeVars);
    propertyMap->insert(loadedThemeVars[value.toString()][key]);
    propertyMap->freeze();
    connect(propertyMap.get(),
            &QQmlPropertyMap::valueChanged,
            [propertyMap,
             themeVarsPath = profile->getPath().parent_path(),
             screens = QList<QString>(key)](const QString& key,
                                            const QVariant& value) {
                writeThemeVarsForTheme(*propertyMap, themeVarsPath, screens);
            });
    themeVars.insert(key, QVariant::fromValue(propertyMap.release()));
}
