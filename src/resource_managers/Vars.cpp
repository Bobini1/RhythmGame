//
// Created by bobini on 05.04.24.
//

#include <memory>
#include <spdlog/spdlog.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "Vars.h"

#include "qml_components/FileQuery.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include "support/Exception.h"

#include <QQmlEngine>
#include <chrono>
#include <qcolor.h>
#include <utility>
auto
resource_managers::GlobalVars::getNoteScreenTimeMillis() const -> int
{
    return noteScreenTimeMillis;
}
void
resource_managers::GlobalVars::setNoteScreenTimeMillis(int value)
{
    if (noteScreenTimeMillis == value) {
        return;
    }
    noteScreenTimeMillis = value;
    emit noteScreenTimeMillisChanged();
}
auto
resource_managers::GlobalVars::getLaneCoverOn() const -> bool
{
    return laneCoverOn;
}
void
resource_managers::GlobalVars::setLaneCoverOn(bool value)
{
    if (laneCoverOn == value) {
        return;
    }
    laneCoverOn = value;
    emit laneCoverOnChanged();
}
auto
resource_managers::GlobalVars::getLaneCoverRatio() const -> double
{
    return laneCoverRatio;
}
void
resource_managers::GlobalVars::setLaneCoverRatio(double value)
{
    if (laneCoverRatio == value) {
        return;
    }
    laneCoverRatio = value;
    emit laneCoverRatioChanged();
}

auto
resource_managers::GlobalVars::getLiftOn() const -> bool
{
    return liftOn;
}
void
resource_managers::GlobalVars::setLiftOn(bool value)
{
    if (liftOn == value) {
        return;
    }
    liftOn = value;
    emit liftOnChanged();
}

auto
resource_managers::GlobalVars::getLiftRatio() const -> double
{
    return liftRatio;
}
void
resource_managers::GlobalVars::setLiftRatio(double value)
{
    if (liftRatio == value) {
        return;
    }
    liftRatio = value;
    emit liftRatioChanged();
}

auto
resource_managers::GlobalVars::getHiddenOn() const -> bool
{
    return hiddenOn;
}
void
resource_managers::GlobalVars::setHiddenOn(bool value)
{
    if (hiddenOn == value) {
        return;
    }
    hiddenOn = value;
    emit hiddenOnChanged();
}

auto
resource_managers::GlobalVars::getHiddenRatio() const -> double
{
    return hiddenRatio;
}
void
resource_managers::GlobalVars::setHiddenRatio(double value)
{
    if (hiddenRatio == value) {
        return;
    }
    hiddenRatio = value;
    emit hiddenRatioChanged();
}

auto
resource_managers::GlobalVars::getBgaOn() const -> bool
{
    return bgaOn;
}
void
resource_managers::GlobalVars::setBgaOn(bool value)
{
    if (bgaOn == value) {
        return;
    }
    bgaOn = value;
    emit bgaOnChanged();
}

auto
resource_managers::GlobalVars::getNoteOrderAlgorithm() const -> QString
{
    return noteOrderAlgorithm;
}
void
resource_managers::GlobalVars::setNoteOrderAlgorithm(QString value)
{
    if (noteOrderAlgorithm == value) {
        return;
    }
    noteOrderAlgorithm = value;
    emit noteOrderAlgorithmChanged();
}

auto
resource_managers::GlobalVars::getNoteOrderAlgorithmP2() const -> QString
{
    return noteOrderAlgorithmP2;
}
void
resource_managers::GlobalVars::setNoteOrderAlgorithmP2(QString value)
{
    if (noteOrderAlgorithmP2 == value) {
        return;
    }
    noteOrderAlgorithmP2 = value;
    emit noteOrderAlgorithmP2Changed();
}

auto
resource_managers::GlobalVars::getHiSpeedFix() const -> QString
{
    return hiSpeedFix;
}
void
resource_managers::GlobalVars::setHiSpeedFix(QString value)
{
    if (hiSpeedFix == value) {
        return;
    }
    hiSpeedFix = value;
    emit hiSpeedFixChanged();
}

auto
resource_managers::GlobalVars::getDpOptions() const -> QString
{
    return dpOptions;
}
void
resource_managers::GlobalVars::setDpOptions(QString value)
{
    if (dpOptions == value) {
        return;
    }
    dpOptions = value;
    emit dpOptionsChanged();
}

auto
resource_managers::GlobalVars::getGaugeType() const -> QString
{
    return gaugeType;
}
void
resource_managers::GlobalVars::setGaugeType(QString value)
{
    if (gaugeType == value) {
        return;
    }
    gaugeType = value;
    emit gaugeTypeChanged();
}

auto
resource_managers::GlobalVars::getGaugeMode() const -> QString
{
    return gaugeMode;
}
void
resource_managers::GlobalVars::setGaugeMode(QString value)
{
    if (gaugeMode == value) {
        return;
    }
    gaugeMode = value;
    emit gaugeModeChanged();
}

auto
resource_managers::GlobalVars::getBottomShiftableGauge() const -> QString
{
    return bottomShiftableGauge;
}
void
resource_managers::GlobalVars::setBottomShiftableGauge(QString value)
{
    if (bottomShiftableGauge == value) {
        return;
    }
    bottomShiftableGauge = value;
    emit bottomShiftableGaugeChanged();
}

void
writeGlobalVars(const resource_managers::GlobalVars& globalVars,
                const std::filesystem::path& profileFolder)
{
    auto json = QJsonObject();
    for (auto i = globalVars.metaObject()->propertyOffset();
         i < globalVars.metaObject()->propertyCount();
         ++i) {
        auto property = globalVars.metaObject()->property(i);
        const auto value = property.isEnumType()
                             ? property.read(&globalVars).toString()
                             : property.read(&globalVars).toJsonValue();
        json[property.name()] = value;
    }
    auto file = QFile{ profileFolder / "globalVars.json" };
    if (!file.open(QIODevice::WriteOnly)) {
        spdlog::error("Failed to open config for writing: {}: {}",
                      profileFolder.string(),
                      file.errorString().toStdString());
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
        spdlog::error("Failed to open config for reading + writing: {}, {}",
                      path.string(),
                      file.errorString().toStdString());
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
        spdlog::error(
          "Failed to open config for reading + writing: {}: {}. The "
          "var {} will not be written.",
          path.string(),
          file.errorString().toStdString(),
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
    const auto stdPath = support::qStringToPath(object["path"].toString());
    if (!stdPath.is_relative()) {
        throw support::Exception(
          std::format("default field of property of type file is not a "
                      "relative path: {}",
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
        if (!exists(themePath / stdPath / defaultPath)) {
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
    if (const auto files =
          qml_components::FileQuery().getSelectableFilesForDirectory(
            support::pathToQString(themePath / stdPath));
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
        throw support::Exception(
          std::format("default field of property of type choice is not one "
                      "of the choices: "
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
createBooleanProperty(QHash<QString, QVariant>& screenVars,
                      const QJsonObject& object)
{
    if (!object["default"].isBool()) {
        throw support::Exception(
          std::format("default field of property of type boolean is undefined "
                      "or not a boolean: {}",
                      jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] = object["default"].toBool();
}

void
createHiddenProperty(QHash<QString, QVariant>& screenVars,
                     const QJsonObject& object,
                     int level)
{
    if (level != 0) {
        throw support::Exception(
          std::format("Hidden properties are not allowed in groups: {}",
                      jsonValueToString(object)));
    }
    screenVars[object["id"].toString()] =
      object["default"].isUndefined() ? QJsonValue::Null : object["default"];
}

struct ScreenVarsPopulationResult
{
    QHash<QString, QVariant> screenVars;
    QHash<QString, QString> fileTypeProperties;
};

void
createProperty(ScreenVarsPopulationResult& result,
               const std::filesystem::path& themePath,
               const QJsonObject& object,
               const int level)
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
        result.fileTypeProperties.insert(object["id"].toString(),
                                         object["path"].toString());
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
    } else if (object["type"] == "boolean") {
        createBooleanProperty(result.screenVars, object);
    } else if (object["type"] == "hidden") {
        createHiddenProperty(result.screenVars, object, level);
    } else {
        throw support::Exception(std::format("Property has unknown type: {}",
                                             jsonValueToString(object)));
    }
}

void
populateScreenVarsRecursive( // NOLINT(*-no-recursion)
  ScreenVarsPopulationResult& screenVars,
  const std::filesystem::path& themePath,
  const QJsonArray& array,
  const int level = 0)
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
                throw support::Exception(
                  std::format("Property group has no items (or items is "
                              "not an array): {}",
                              jsonValueToString(object)));
            }
            populateScreenVarsRecursive(
              screenVars, themePath, object["items"].toArray(), level + 1);
        } else {
            createProperty(screenVars, themePath, object, level);
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
        spdlog::error("Failed to open config for reading: {}: {}",
                      settingsPath.string(),
                      file.errorString().toStdString());
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
readGlobalVars(resource_managers::GlobalVars& globalVars,
               const std::filesystem::path& profileFolder)
{
    auto file = QFile{ profileFolder / "globalVars.json" };
    if (!file.exists()) {
        writeGlobalVars(globalVars, profileFolder);
    }
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open config for reading: {}: {}",
                      profileFolder.string(),
                      file.errorString().toStdString());
        return;
    }
    try {
        const auto contents =
          QJsonDocument::fromJson(file.readAll()).object().toVariantHash();
        for (auto i = globalVars.metaObject()->propertyOffset();
             i < globalVars.metaObject()->propertyCount();
             ++i) {
            auto property = globalVars.metaObject()->property(i);
            if (contents.contains(property.name())) {
                // if the property is an enum, we need to convert the string
                // to the enum value
                if (property.isEnumType()) {
                    const auto enumValue =
                      property.enumerator().keyToValue(contents[property.name()]
                                                         .toString()
                                                         .toStdString()
                                                         .c_str());
                    property.write(&globalVars, enumValue);
                } else {
                    property.write(&globalVars, contents[property.name()]);
                }
            }
        }
    } catch (std::exception& exception) {
        spdlog::error("{}", exception.what());
    }
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
        spdlog::error("Failed to open config for reading: {}: {}",
                      themeVarsPath.string(),
                      file.errorString().toStdString());
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
                           support::qStringToPath(
                             vars[screen].fileTypeProperties[key]) /
                           support::qStringToPath(value.toString()))) {
                    result[screen][key] = value;
                } else {
                    spdlog::debug(
                      "The saved file property {} of screen {} of theme {} "
                      "({}) does not point to an existing file, will use "
                      "the default instead ({}).",
                      key.toStdString(),
                      screen.toStdString(),
                      themePath.string(),
                      (themePath /
                       support::qStringToPath(
                         vars[screen].fileTypeProperties[key]) /
                       support::qStringToPath(value.toString()))
                        .string(),
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
void
resource_managers::Vars::writeGlobalVars() const
{
    ::writeGlobalVars(globalVars, profile->getPath().parent_path());
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
    writeGlobalVars();
    connect(themeConfig,
            &QQmlPropertyMap::valueChanged,
            this,
            &Vars::onThemeConfigChanged);
    for (auto i = globalVars.metaObject()->propertyOffset();
         i < globalVars.metaObject()->propertyCount();
         ++i) {
        connect(&globalVars,
                globalVars.metaObject()->property(i).notifySignal(),
                this,
                metaObject()->method(
                  metaObject()->indexOfMethod("writeGlobalVars()")));
    }
}
auto
resource_managers::Vars::getGlobalVars() -> GlobalVars*
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
