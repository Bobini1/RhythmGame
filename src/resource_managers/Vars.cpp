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
#include <qdir.h>
#include <utility>
resource_managers::GlobalVars::GlobalVars(QString avatarPath, QObject* parent)
  : QObject(parent)
  , avatarPath(std::move(avatarPath))
{
}
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
void
resource_managers::GlobalVars::resetNoteScreenTimeMillis()
{
    setNoteScreenTimeMillis(1000);
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
void
resource_managers::GlobalVars::resetLaneCoverOn()
{
    setLaneCoverOn(false);
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
void
resource_managers::GlobalVars::resetLaneCoverRatio()
{
    setLaneCoverRatio(0.1);
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

void
resource_managers::GlobalVars::resetLiftOn()
{
    setLiftOn(false);
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
void
resource_managers::GlobalVars::resetLiftRatio()
{
    setLiftRatio(0.1);
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
void
resource_managers::GlobalVars::resetHiddenOn()
{
    setHiddenOn(false);
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
void
resource_managers::GlobalVars::resetHiddenRatio()
{
    setHiddenRatio(0.1);
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

void
resource_managers::GlobalVars::resetBgaOn()
{
    setBgaOn(true);
}

auto
resource_managers::GlobalVars::getNoteOrderAlgorithm() const
  -> NoteOrderAlgorithm
{
    return noteOrderAlgorithm;
}
void
resource_managers::GlobalVars::setNoteOrderAlgorithm(NoteOrderAlgorithm value)
{
    if (noteOrderAlgorithm == value) {
        return;
    }
    noteOrderAlgorithm = value;
    emit noteOrderAlgorithmChanged();
}
void
resource_managers::GlobalVars::resetNoteOrderAlgorithm()
{
    setNoteOrderAlgorithm(NoteOrderAlgorithm::Normal);
}

auto
resource_managers::GlobalVars::getNoteOrderAlgorithmP2() const
  -> NoteOrderAlgorithm
{
    return noteOrderAlgorithmP2;
}
void
resource_managers::GlobalVars::setNoteOrderAlgorithmP2(NoteOrderAlgorithm value)
{
    if (noteOrderAlgorithmP2 == value) {
        return;
    }
    noteOrderAlgorithmP2 = value;
    emit noteOrderAlgorithmP2Changed();
}

void
resource_managers::GlobalVars::resetNoteOrderAlgorithmP2()
{
    setNoteOrderAlgorithmP2(NoteOrderAlgorithm::Normal);
}

auto
resource_managers::GlobalVars::getHiSpeedFix() const -> HiSpeedFix
{
    return hiSpeedFix;
}
void
resource_managers::GlobalVars::setHiSpeedFix(HiSpeedFix value)
{
    if (hiSpeedFix == value) {
        return;
    }
    hiSpeedFix = value;
    emit hiSpeedFixChanged();
}

void
resource_managers::GlobalVars::resetHiSpeedFix()
{
    setHiSpeedFix(HiSpeedFix::Main);
}

auto
resource_managers::GlobalVars::getDpOptions() const -> DpOptions
{
    return dpOptions;
}
void
resource_managers::GlobalVars::setDpOptions(DpOptions value)
{
    if (dpOptions == value) {
        return;
    }
    dpOptions = value;
    emit dpOptionsChanged();
}

void
resource_managers::GlobalVars::resetDpOptions()
{
    setDpOptions(DpOptions::Off);
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
void
resource_managers::GlobalVars::resetGaugeType()
{
    setGaugeType("FC");
}
auto
resource_managers::GlobalVars::getGaugeMode() const -> GaugeMode
{
    return gaugeMode;
}
void
resource_managers::GlobalVars::setGaugeMode(GaugeMode value)
{
    if (gaugeMode == value) {
        return;
    }
    gaugeMode = value;
    emit gaugeModeChanged();
}

void
resource_managers::GlobalVars::resetGaugeMode()
{
    setGaugeMode(GaugeMode::SelectToUnder);
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
resource_managers::GlobalVars::resetBottomShiftableGauge()
{
    setBottomShiftableGauge("AEASY");
}

auto
resource_managers::GlobalVars::getAvatar() const -> QString
{
    return avatar;
}
void
resource_managers::GlobalVars::setAvatar(QString value)
{
    if (avatar == value) {
        return;
    }
    const auto url = QUrl{ value };
    const auto sourcePath = url.toLocalFile();
    const auto isAbsolute = QDir::isAbsolutePath(sourcePath);
    if ((value.contains('/') || value.contains('\\')) && !isAbsolute) {
        spdlog::warn("Avatar must be a filename or an absolute path: {}",
                     value.toStdString());
        return;
    }
    if (isAbsolute) {
        auto file = QFile{ sourcePath };
        if (!file.open(QIODevice::ReadOnly)) {
            spdlog::warn("Failed to open avatar file: {}", value.toStdString());
            return;
        }
        const auto fileName = QFileInfo{ file.fileName() }.fileName();
        const auto targetPath = QUrl{ avatarPath + fileName }.toLocalFile();
        const auto sourceStdPath = support::qStringToPath(sourcePath);
        const auto targetStdPath = support::qStringToPath(targetPath);
        if (auto err = std::error_code{};
            !equivalent(targetStdPath, sourceStdPath, err)) {
            // If this picture is currently used by the game, it won't be
            // overwritten (remove will fail).
            QFile::remove(targetPath);
            if (!file.copy(targetPath)) {
                spdlog::warn(
                  "Failed to copy avatar file to the avatar folder: {}",
                  value.toStdString());
                return;
            }
        }
        value = fileName;
    }
    avatar = value;
    emit avatarChanged();
}
void
resource_managers::GlobalVars::resetAvatar()
{
    setAvatar("mascot.png");
}

auto
resource_managers::GlobalVars::getName() const -> QString
{
    return name;
}
void
resource_managers::GlobalVars::setName(QString value)
{
    if (name == value) {
        return;
    }
    name = value;
    emit nameChanged();
}
void
resource_managers::GlobalVars::resetName()
{
    setName("Default");
}
auto
resource_managers::GlobalVars::getLanguage() const -> QString
{
    return language;
}
void
resource_managers::GlobalVars::setLanguage(QString value)
{
    if (language == value) {
        return;
    }
    language = value;
    emit languageChanged();
}
void
resource_managers::GlobalVars::resetLanguage()
{
    setLanguage(QLocale::system().name());
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
    auto themes = QHash<QString, QHash<QString, QHash<QString, QVariant>>>{};
    for (const auto& [screen, screenVars] : themeVars.asKeyValueRange()) {
        for (const auto& [themeName, themeVars] : screenVars.asKeyValueRange()) {
            themes[themeName][screen] = themeVars;
        }
    }
    for (const auto& [name, vars] : themes.asKeyValueRange()) {
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
        auto settingsUrl = themeFamily.getScreens()[screen].getSettings();
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
        auto screens = readThemeVarsForTheme(
          profileFolder /
            support::qStringToPath(name + QStringLiteral("-vars.json")),
          themePath,
          themeFamily);
        for (const auto& [screen, screenVars] :
             screens.asKeyValueRange()) {
            vars[screen][name] = screenVars;
        }
    }
    return vars;
}

void
resource_managers::Vars::populateThemePropertyMap(
  QQmlPropertyMap& themeVars,
  QHash<QString, QHash<QString, QHash<QString, QVariant>>> themeVarsData,
  const std::filesystem::path& themeVarsPath)
{
    for (const auto& key : themeVars.keys()) {
        themeVars[key].value<QQmlPropertyMap*>()->deleteLater();
    }
    for (const auto& [screenName, themes] : themeVarsData.asKeyValueRange()) {
        auto screenPropertyMap = std::make_unique<QQmlPropertyMap>(&themeVars);
        for (const auto& [themeName, vars] : themes.asKeyValueRange()) {
            auto propertyMap = std::make_unique<QQmlPropertyMap>(screenPropertyMap.get());
            propertyMap->insert(themeVarsData[screenName][themeName]);
            propertyMap->freeze();
            connect(propertyMap.get(),
                    &QQmlPropertyMap::valueChanged,
                    this,
                    [this,
                     themeVarsPath,
                     screen = screenName,
                     themeFamily = themeName](const QString& key,
                                              const QVariant& value) {
                        loadedThemeVars[screen][themeFamily][key] = value;
                        writeSingleThemeVar(
                          screen,
                          key,
                          value,
                          themeVarsPath /
                            support::qStringToPath(
                              themeFamily + QStringLiteral("-vars.json")));
                    });
            screenPropertyMap->insert(
              themeName, QVariant::fromValue(propertyMap.release()));
        }
        screenPropertyMap->freeze();
        themeVars.insert(screenName,
                         QVariant::fromValue(screenPropertyMap.release()));
    }
    themeVars.freeze();
}
void
resource_managers::Vars::writeGlobalVars() const
{
    ::writeGlobalVars(globalVars, profile->getPath().parent_path());
}

resource_managers::Vars::Vars(
  const Profile* profile,
  QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
  QString avatarPath,
  QObject* parent)
  : QObject(parent)
  , globalVars(std::move(avatarPath))
  , profile(profile)
  , availableThemeFamilies(std::move(availableThemeFamilies))
  , loadedThemeVars(readThemeVars(profile->getPath().parent_path(),
                                  this->availableThemeFamilies))
{
    writeThemeVars(loadedThemeVars, profile->getPath().parent_path());
    populateThemePropertyMap(
      themeVars, loadedThemeVars, profile->getPath().parent_path());
    readGlobalVars(globalVars, profile->getPath().parent_path());
    writeGlobalVars();
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
