//
// Created by bobini on 05.04.24.
//

#include <memory>
#include <spdlog/spdlog.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
        json[key] = globalVars[key].toJsonValue();
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
writeThemeVarsForTheme(
  const QHash<QString, QHash<QString, QVariant>>& themeVars,
  const std::filesystem::path& path)
{
    auto file = QFile{ path };
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open config for reading + writing: {}",
                      path.string());
        return;
    }
    auto jsonDocument = QJsonDocument::fromJson(file.readAll());
    auto json = jsonDocument.object();
    for (const auto& [screen, vars] : themeVars.asKeyValueRange()) {
        auto screenObject = QJsonObject();
        for (const auto& [key, value] : vars.asKeyValueRange()) {
            screenObject[key] = QJsonValue::fromVariant(value);
        }
        json[screen] = screenObject;
    }
    jsonDocument.setObject(json);
    file.resize(0);
    file.write(jsonDocument.toJson());
}

void
writeThemeVars(
  const QHash<QString, QHash<QString, QHash<QString, QVariant>>>& themeVars,
  const std::filesystem::path& profileFolder)
{
    for (const auto& [name, vars] : themeVars.asKeyValueRange()) {
        writeThemeVarsForTheme(
          vars,
          profileFolder /
            support::qStringToPath(name + QStringLiteral("-vars.json")));
    }
}

void
writeSingleThemeVar(const QString& screen,
                    const QString& key,
                    const QVariant& value,
                    const std::filesystem::path& path)
{
    auto file = QFile{ path };
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open config for reading + writing: {}. The "
                      "var {} will not be written.",
                      path.string(),
                      key.toStdString());
        return;
    }
    auto jsonDocument = QJsonDocument::fromJson(file.readAll());
    auto json = jsonDocument.object();
    auto object = json[screen].toObject();
    object[key] = QJsonValue::fromVariant(value);
    json[screen] = object;
    jsonDocument.setObject(json);
    file.resize(0);
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

auto
getSelectableFilesForDirectory(const std::filesystem::path& path)
  -> QList<QString>
{
    auto files = QList<QString>{};
    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (!file.is_regular_file()) {
            continue;
        }
        if (file.path().extension() == ".ini") {
            continue;
        }
        // ignore files starting with dot
        if (file.path().filename().string().front() == '.') {
            continue;
        }
        files.push_back(support::pathToQString(file.path().filename()));
    }
    return files;
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
    if (const auto files = getSelectableFilesForDirectory(
          themePath / support::qStringToPath(path.toString()));
        !files.empty()) {
        screenVars[object["id"].toString()] = QVariant(files.first());
    } else {
        spdlog::debug("No files found for property of type file: {}",
                      jsonValueToString(object));
        screenVars[object["id"].toString()] = QVariant();
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
    screenVars[object["id"].toString()] = object["default"].toVariant();
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

struct ScreenVarsPopulationResult
{
    QHash<QString, QVariant> screenVars;
    QSet<QString> fileTypeProperties;
};

void
createProperty(ScreenVarsPopulationResult& result,
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
        createFileProperty(result.screenVars, object, themePath);
        result.fileTypeProperties.insert(object["id"].toString());
    } else if (object["type"] == "color") {
        createColorProperty(result.screenVars, object);
    } else if (object["type"] == "choice") {
        createChoiceProperty(result.screenVars, object);
    } else if (object["type"] == "checkbox") {
        createCheckBoxProperty(result.screenVars, object);
    } else if (object["type"] == "string") {
        createStringProperty(result.screenVars, object);
    } else if (object["type"] == "range") {
        createRangeProperty(result.screenVars, object);
    } else if (object["type"] == "hidden") {
        createHiddenProperty(result.screenVars, object);
    } else {
        throw support::Exception(std::format("Property has unknown type: {}",
                                             jsonValueToString(object)));
    }
}

void
populateScreenVarsRecursive( // NOLINT(*-no-recursion)
  ScreenVarsPopulationResult& screenVars,
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
  -> ScreenVarsPopulationResult
{
    auto result = ScreenVarsPopulationResult{};
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
    populateScreenVarsRecursive(result, themePath, contents.array());
    return result;
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
    auto vars = QHash<QString, ScreenVarsPopulationResult>{};
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
    auto result = QHash<QString, QHash<QString, QVariant>>{};
    for (const auto& [screen, screenVars] : vars.asKeyValueRange()) {
        result[screen] = screenVars.screenVars;
    }
    auto file = QFile{ themeVarsPath };
    if (!file.exists()) {
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}",
                      themeVarsPath.string());
        return result;
    }
    auto contents = QJsonDocument::fromJson(file.readAll()).object();
    for (const auto& screen : themeFamily.getScreens().keys()) {
        for (const auto& [key, value] :
             contents[screen].toObject().toVariantHash().asKeyValueRange()) {
            if (vars[screen].fileTypeProperties.contains(key)) {
                if (value.isNull()) {
                    continue;
                }
                if (exists(themePath /
                           support::qStringToPath(value.toString()))) {
                    result[screen][key] = value;
                } else {
                    spdlog::debug(
                      "The saved file property {} of screen {} of theme {} "
                      "({}) does not point to an existing file, will use the "
                      "default instead ({}).",
                      key.toStdString(),
                      screen.toStdString(),
                      themePath.string(),
                      contents[screen].toObject()[key].toString().toStdString(),
                      result[screen][key].toString().toStdString());
                }
            } else {
                result[screen][key] =
                  contents[screen].toObject()[key].toVariant();
            }
        }
    }
    return result;
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
resource_managers::Vars::populateThemePropertyMap(
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
        connect(propertyMap.get(),
                &QQmlPropertyMap::valueChanged,
                this,
                [this, themeVarsPath, screen = key, themeFamily = value](
                  const QString& key, const QVariant& value) {
                    loadedThemeVars[themeFamily][screen][key] = value;
                    writeSingleThemeVar(
                      screen,
                      key,
                      value,
                      themeVarsPath /
                        support::qStringToPath(themeFamily +
                                               QStringLiteral("-vars.json")));
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
    writeThemeVars(loadedThemeVars, profile->getPath().parent_path());
    populateThemePropertyMap(themeVars,
                             loadedThemeVars,
                             profile->getPath().parent_path(),
                             *themeConfig);
    readGlobalVars(globalVars, profile->getPath().parent_path());
    writeGlobalVars(globalVars, profile->getPath().parent_path());
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
QList<QString>
resource_managers::Vars::getSelectableFilesForDirectory(QString directory) const
{
    return ::getSelectableFilesForDirectory(support::qStringToPath(directory));
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
            this,
            [this,
             themeVarsPath = profile->getPath().parent_path(),
             screen = key,
             themeFamily = value.toString()](const QString& key,
                                             const QVariant& value) {
                loadedThemeVars[themeFamily][screen][key] = value;
                writeSingleThemeVar(
                  screen,
                  key,
                  value,
                  themeVarsPath /
                    support::qStringToPath(themeFamily +
                                           QStringLiteral("-vars.json")));
            });
    themeVars.insert(key, QVariant::fromValue(propertyMap.release()));
}
