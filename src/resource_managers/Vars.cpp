//
// Created by bobini on 05.04.24.
//

#include <memory>
#include <spdlog/spdlog.h>
#include <QtConcurrent>
#include "Vars.h"

#include "qml_components/FileQuery.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"
#include "support/Exception.h"

#include <chrono>
#include <qcolor.h>
#include <qdir.h>
#include <utility>
resource_managers::GeneralVars::GeneralVars(QList<QString> assetsPaths,
                                            QObject* parent)
  : QObject(parent)
  , assetsPaths(std::move(assetsPaths))
{
    resetBgm();
    resetSoundset();
}
auto
resource_managers::GeneralVars::getNoteScreenTimeMillis() const -> double
{
    return noteScreenTimeMillis;
}
void
resource_managers::GeneralVars::setNoteScreenTimeMillis(double value)
{
    if (noteScreenTimeMillis == value) {
        return;
    }
    noteScreenTimeMillis = value;
    emit noteScreenTimeMillisChanged();
}
void
resource_managers::GeneralVars::resetNoteScreenTimeMillis()
{
    setNoteScreenTimeMillis(1000);
}
auto
resource_managers::GeneralVars::getLaneCoverOn() const -> bool
{
    return laneCoverOn;
}
void
resource_managers::GeneralVars::setLaneCoverOn(bool value)
{
    if (laneCoverOn == value) {
        return;
    }
    laneCoverOn = value;
    emit laneCoverOnChanged();
}
void
resource_managers::GeneralVars::resetLaneCoverOn()
{
    setLaneCoverOn(false);
}

auto
resource_managers::GeneralVars::getLaneCoverRatio() const -> double
{
    return laneCoverRatio;
}
void
resource_managers::GeneralVars::setLaneCoverRatio(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (laneCoverRatio == value) {
        return;
    }
    laneCoverRatio = value;
    emit laneCoverRatioChanged();
}
void
resource_managers::GeneralVars::resetLaneCoverRatio()
{
    setLaneCoverRatio(0.1);
}

auto
resource_managers::GeneralVars::getLiftOn() const -> bool
{
    return liftOn;
}
void
resource_managers::GeneralVars::setLiftOn(bool value)
{
    if (liftOn == value) {
        return;
    }
    liftOn = value;
    emit liftOnChanged();
}

void
resource_managers::GeneralVars::resetLiftOn()
{
    setLiftOn(false);
}

auto
resource_managers::GeneralVars::getLiftRatio() const -> double
{
    return liftRatio;
}
void
resource_managers::GeneralVars::setLiftRatio(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (liftRatio == value) {
        return;
    }
    liftRatio = value;
    emit liftRatioChanged();
}
void
resource_managers::GeneralVars::resetLiftRatio()
{
    setLiftRatio(0.1);
}
auto
resource_managers::GeneralVars::getHiddenOn() const -> bool
{
    return hiddenOn;
}
void
resource_managers::GeneralVars::setHiddenOn(bool value)
{
    if (hiddenOn == value) {
        return;
    }
    hiddenOn = value;
    emit hiddenOnChanged();
}
void
resource_managers::GeneralVars::resetHiddenOn()
{
    setHiddenOn(false);
}

auto
resource_managers::GeneralVars::getHiddenRatio() const -> double
{
    return hiddenRatio;
}
void
resource_managers::GeneralVars::setHiddenRatio(double value)
{
    value = std::clamp(value, 0.0, 1.0);
    if (hiddenRatio == value) {
        return;
    }
    hiddenRatio = value;
    emit hiddenRatioChanged();
}
void
resource_managers::GeneralVars::resetHiddenRatio()
{
    setHiddenRatio(0.1);
}

auto
resource_managers::GeneralVars::getBgaOn() const -> bool
{
    return bgaOn;
}
void
resource_managers::GeneralVars::setBgaOn(bool value)
{
    if (bgaOn == value) {
        return;
    }
    bgaOn = value;
    emit bgaOnChanged();
}

void
resource_managers::GeneralVars::resetBgaOn()
{
    setBgaOn(true);
}

auto
resource_managers::GeneralVars::getNoteOrderAlgorithm() const
  -> NoteOrderAlgorithm
{
    return noteOrderAlgorithm;
}
void
resource_managers::GeneralVars::setNoteOrderAlgorithm(NoteOrderAlgorithm value)
{
    if (noteOrderAlgorithm == value) {
        return;
    }
    noteOrderAlgorithm = value;
    emit noteOrderAlgorithmChanged();
}
void
resource_managers::GeneralVars::resetNoteOrderAlgorithm()
{
    setNoteOrderAlgorithm(NoteOrderAlgorithm::Normal);
}

auto
resource_managers::GeneralVars::getNoteOrderAlgorithmP2() const
  -> NoteOrderAlgorithm
{
    return noteOrderAlgorithmP2;
}
void
resource_managers::GeneralVars::setNoteOrderAlgorithmP2(
  NoteOrderAlgorithm value)
{
    if (noteOrderAlgorithmP2 == value) {
        return;
    }
    noteOrderAlgorithmP2 = value;
    emit noteOrderAlgorithmP2Changed();
}

void
resource_managers::GeneralVars::resetNoteOrderAlgorithmP2()
{
    setNoteOrderAlgorithmP2(NoteOrderAlgorithm::Normal);
}

auto
resource_managers::GeneralVars::getHiSpeedFix() const -> HiSpeedFix
{
    return hiSpeedFix;
}
void
resource_managers::GeneralVars::setHiSpeedFix(HiSpeedFix value)
{
    if (hiSpeedFix == value) {
        return;
    }
    hiSpeedFix = value;
    emit hiSpeedFixChanged();
}

void
resource_managers::GeneralVars::resetHiSpeedFix()
{
    setHiSpeedFix(HiSpeedFix::Main);
}

auto
resource_managers::GeneralVars::getDpOptions() const -> DpOptions
{
    return dpOptions;
}
void
resource_managers::GeneralVars::setDpOptions(DpOptions value)
{
    if (dpOptions == value) {
        return;
    }
    dpOptions = value;
    emit dpOptionsChanged();
}

void
resource_managers::GeneralVars::resetDpOptions()
{
    setDpOptions(DpOptions::Off);
}

auto
resource_managers::GeneralVars::getGaugeType() const -> QString
{
    return gaugeType;
}
void
resource_managers::GeneralVars::setGaugeType(QString value)
{
    if (gaugeType == value) {
        return;
    }
    gaugeType = value;
    emit gaugeTypeChanged();
}
void
resource_managers::GeneralVars::resetGaugeType()
{
    setGaugeType("FC");
}
auto
resource_managers::GeneralVars::getGaugeMode() const -> GaugeMode
{
    return gaugeMode;
}
void
resource_managers::GeneralVars::setGaugeMode(GaugeMode value)
{
    if (gaugeMode == value) {
        return;
    }
    gaugeMode = value;
    emit gaugeModeChanged();
}

void
resource_managers::GeneralVars::resetGaugeMode()
{
    setGaugeMode(GaugeMode::SelectToUnder);
}

auto
resource_managers::GeneralVars::getBottomShiftableGauge() const -> QString
{
    return bottomShiftableGauge;
}
void
resource_managers::GeneralVars::setBottomShiftableGauge(const QString& value)
{
    if (bottomShiftableGauge == value) {
        return;
    }
    bottomShiftableGauge = value;
    emit bottomShiftableGaugeChanged();
}
void
resource_managers::GeneralVars::resetBottomShiftableGauge()
{
    setBottomShiftableGauge("AEASY");
}

auto
resource_managers::GeneralVars::getAvatar() const -> QString
{
    return avatar;
}
void
resource_managers::GeneralVars::setAvatar(QString value)
{
    if (avatar == value) {
        return;
    }
    const auto url = QUrl{ value };
    const auto sourcePath = url.toLocalFile();
    const auto isAbsolute = QDir::isAbsolutePath(sourcePath);
    if (!value.startsWith("image://avatar/") && !isAbsolute) {
        spdlog::warn(
          "Avatar must start with image://avatar/ or be an absolute path: {}",
          value.toStdString());
        return;
    }
    if (!isAbsolute) {
        auto filename = value;
        filename.remove(0, QStringLiteral("image://avatar/").length());
        // ensure it exists
        for (const auto& path : assetsPaths) {
            if (QFileInfo::exists(path + "avatars/" + filename)) {
                avatar = value;
                emit avatarChanged();
                return;
            }
        }
        spdlog::warn(
          "Requested avatar file does not exist in any avatar folder: {}",
          value.toStdString());
        return;
    }
    auto file = QFile{ sourcePath };
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Failed to open avatar file: {}", value.toStdString());
        return;
    }
    const auto fileName = QFileInfo{ file.fileName() }.fileName();
    const auto targetPath = assetsPaths[0] + "avatars/" + fileName;
    const auto sourceStdPath = support::qStringToPath(sourcePath);
    const auto targetStdPath = support::qStringToPath(targetPath);
    if (auto err = std::error_code{};
        !equivalent(targetStdPath, sourceStdPath, err)) {
        // If this picture is currently used by the game, it won't be
        // overwritten (remove will fail).
        QFile::remove(targetPath);
        if (!file.copy(targetPath)) {
            spdlog::warn("Failed to copy avatar file to the avatar folder: {}",
                         value.toStdString());
            return;
        }
    }
    value = "image://avatar/" + fileName;
    avatar = value;
    emit avatarChanged();
}
void
resource_managers::GeneralVars::resetAvatar()
{
    setAvatar("mascot.png");
}

auto
resource_managers::GeneralVars::getName() const -> QString
{
    return name;
}
void
resource_managers::GeneralVars::setName(QString value)
{
    if (name == value) {
        return;
    }
    name = value;
    emit nameChanged();
}
void
resource_managers::GeneralVars::resetName()
{
    setName("Default");
}
auto
resource_managers::GeneralVars::getLanguage() const -> QString
{
    return language;
}
void
resource_managers::GeneralVars::setLanguage(QString value)
{
    if (language == value) {
        return;
    }
    language = value;
    emit languageChanged();
}
void
resource_managers::GeneralVars::resetLanguage()
{
    setLanguage(QLocale::system().name());
}
auto
resource_managers::GeneralVars::getOffset() const -> double
{
    return offset;
}
void
resource_managers::GeneralVars::setOffset(double value)
{
    if (offset == value) {
        return;
    }
    offset = value;
    emit offsetChanged();
}
void
resource_managers::GeneralVars::resetOffset()
{
    setOffset(0.0);
}
auto
resource_managers::GeneralVars::getScoreTarget() const -> ScoreTarget
{
    return scoreTarget;
}
void
resource_managers::GeneralVars::setScoreTarget(ScoreTarget value)
{
    if (scoreTarget == value) {
        return;
    }
    scoreTarget = value;
    emit scoreTargetChanged();
}
void
resource_managers::GeneralVars::resetScoreTarget()
{
    setScoreTarget(ScoreTarget::BestScore);
}
auto
resource_managers::GeneralVars::getTargetScoreFraction() const -> double
{
    return targetScoreFraction;
}
void
resource_managers::GeneralVars::setTargetScoreFraction(double value)
{
    if (targetScoreFraction == value) {
        return;
    }
    targetScoreFraction = value;
    emit targetScoreFractionChanged();
}
void
resource_managers::GeneralVars::resetTargetScoreFraction()
{
    setTargetScoreFraction(8.0 / 9.0);
}
auto
resource_managers::GeneralVars::getBgm() const -> QString
{
    // Return just the folder name
    auto parts = bgmPath.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return "";
    }
    return parts.last();
}
void
resource_managers::GeneralVars::setBgm(const QString& value)
{
    auto currentBgm = getBgm();
    if (currentBgm == value) {
        return;
    }
    auto currentBgmPath = bgmPath;
    // Look for the folder in the assets paths
    auto found = false;
    auto path = QString{};
    for (const auto& assetsPath : assetsPaths) {
        const auto fullPath = assetsPath + "bgm/" + value + "/";
        if (QDir(fullPath).exists()) {
            found = true;
            path = fullPath;
            break;
        }
    }
    if (!found) {
        spdlog::debug("BGM folder not found in any assets path: {}",
                      value.toStdString());
        return;
    }
    bgmPath = path;
    if (currentBgm != getBgm()) {
        emit bgmChanged();
    }
    if (currentBgmPath != bgmPath) {
        emit bgmPathChanged();
    }
}
void
resource_managers::GeneralVars::resetBgm()
{
    auto currentBgm = getBgm();
    auto currentBgmPath = bgmPath;
    // set "Trance" or first available folder
    auto found = false;
    for (const auto& assetsPath : assetsPaths) {
        const auto fullPath = assetsPath + "bgm/Trance/";
        if (QDir(fullPath).exists()) {
            bgmPath = fullPath;
            found = true;
            break;
        }
    }
    if (!found) {
        for (const auto& assetsPath : assetsPaths) {
            const auto bgmFolder = assetsPath + "bgm/";
            const auto dir = QDir(bgmFolder);
            const auto entries =
              dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!entries.isEmpty()) {
                bgmPath = bgmFolder + entries.first() + "/";
                found = true;
                break;
            }
        }
    }
    if (!found) {
        bgmPath = "";
    }
    if (currentBgm != getBgm()) {
        emit bgmChanged();
    }
    if (currentBgmPath != bgmPath) {
        emit bgmPathChanged();
    }
}
auto
resource_managers::GeneralVars::getBgmPath() const -> QString
{
    return bgmPath;
}
auto
resource_managers::GeneralVars::getSoundset() const -> QString
{
    // Return just the folder name
    auto parts = soundsetPath.split('/', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return "";
    }
    return parts.last();
}
void
resource_managers::GeneralVars::setSoundset(QString value)
{
    auto currentSoundset = getSoundset();
    auto currentSoundsetPath = soundsetPath;
    if (currentSoundset == value) {
        return;
    }
    // Look for the folder in the assets paths
    auto found = false;
    auto path = QString{};
    for (const auto& assetsPath : assetsPaths) {
        const auto fullPath = assetsPath + "soundsets/" + value + "/";
        if (QDir(fullPath).exists()) {
            found = true;
            path = fullPath;
            break;
        }
    }
    if (!found) {
        spdlog::debug("Soundset folder not found in any assets path: {}",
                      value.toStdString());
        return;
    }
    soundsetPath = path;
    if (currentSoundset != getSoundset()) {
        emit soundsetChanged();
    }
    if (currentSoundsetPath != soundsetPath) {
        emit soundsetPathChanged();
    }
}
void
resource_managers::GeneralVars::resetSoundset()
{
    auto current = getSoundset();
    auto currentPath = soundsetPath;
    // set "default" or first available folder
    auto found = false;
    for (const auto& assetsPath : assetsPaths) {
        const auto fullPath = assetsPath + "soundsets/brook_sound_fx/";
        if (QDir(fullPath).exists()) {
            soundsetPath = fullPath;
            found = true;
            break;
        }
    }
    if (!found) {
        for (const auto& assetsPath : assetsPaths) {
            const auto soundsetsFolder = assetsPath + "soundsets/";
            const auto dir = QDir(soundsetsFolder);
            const auto entries =
              dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!entries.isEmpty()) {
                soundsetPath = soundsetsFolder + entries.first() + "/";
                found = true;
                break;
            }
        }
    }
    if (!found) {
        soundsetPath = "";
    }
    if (current != getSoundset()) {
        emit soundsetChanged();
    }
    if (currentPath != soundsetPath) {
        emit soundsetPathChanged();
    }
}
auto
resource_managers::GeneralVars::getSoundsetPath() const -> QString
{
    return soundsetPath;
}
auto
resource_managers::GeneralVars::getAvailableBgms() const -> QList<QString>
{
    auto bgms = QList<QString>{};
    for (const auto& assetsPath : assetsPaths) {
        const auto bgmFolder = assetsPath + "bgm/";
        const auto dir = QDir(bgmFolder);
        const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            if (!bgms.contains(entry)) {
                bgms.append(entry);
            }
        }
    }
    return bgms;
}
auto
resource_managers::GeneralVars::getAvailableSoundsets() const -> QList<QString>
{
    auto soundsets = QList<QString>{};
    for (const auto& assetsPath : assetsPaths) {
        const auto soundsetsFolder = assetsPath + "soundsets/";
        const auto dir = QDir(soundsetsFolder);
        const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& entry : entries) {
            if (!soundsets.contains(entry)) {
                soundsets.append(entry);
            }
        }
    }
    return soundsets;
}
namespace {
void
writeGeneralVars(QThreadPool& writePool,
                 const resource_managers::GeneralVars& generalVars,
                 const std::filesystem::path& profileFolder)
{
    auto json = QJsonObject();
    for (auto i = generalVars.metaObject()->propertyOffset();
         i < generalVars.metaObject()->propertyCount();
         ++i) {
        auto property = generalVars.metaObject()->property(i);
        if (!property.isStored()) {
            continue;
        }
        const auto value = property.isEnumType()
                             ? property.read(&generalVars).toString()
                             : property.read(&generalVars).toJsonValue();
        json[property.name()] = value;
    }
    writePool.start([json, profileFolder] {
        auto jsonDocument = QJsonDocument();
        jsonDocument.setObject(json);
        auto file = QFile{ profileFolder / "generalVars.json" };
        if (!file.open(QIODevice::WriteOnly)) {
            spdlog::error("Failed to open config for writing: {}: {}",
                          profileFolder.string(),
                          file.errorString().toStdString());
            return;
        }
        file.write(jsonDocument.toJson());
    });
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
        for (const auto& [themeName, themeVars] :
             screenVars.asKeyValueRange()) {
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
writeSingleThemeVar(QThreadPool& writePool,
                    const QString& screen,
                    const QString& key,
                    const QVariant& value,
                    const std::filesystem::path& path)
{
    writePool.start([screen, key, value, path]() {
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
    });
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
    for (const auto& choice : object["choices"].toArray()) {
        if (!choice.isObject()) {
            throw support::Exception(std::format("a choice of property of type "
                                                 "choice is not an object: {}",
                                                 jsonValueToString(choice)));
        }
        if (!choice.toObject()["value"].isString()) {
            throw support::Exception(
              std::format("a choice of property of type choice has no value "
                          "(or not a string): {}",
                          jsonValueToString(choice)));
        }
        if (!choice.toObject()["name"].isObject()) {
            throw support::Exception(
              std::format("a choice of property of type choice has no name "
                          "(or not an object): {}",
                          jsonValueToString(choice)));
        }
        // check if choice names are strings
        for (const auto& value : choice.toObject()["name"].toObject()) {
            if (!value.isString()) {
                throw support::Exception(std::format(
                  "a choice name of property of type choice is not a "
                  "string: {}",
                  jsonValueToString(choice)));
            }
        }
    }
    // confirm that the default value is one of the choices
    const auto choices = object["choices"].toArray();
    auto values = QStringList{};
    for (const auto& choice : choices) {
        values.append(choice.toObject()["value"].toString());
    }
    if (choices.isEmpty()) {
        throw support::Exception(
          std::format("choices field of property of type choice is empty: {}",
                      jsonValueToString(object)));
    }
    if (std::ranges::find(values, object["default"].toString()) ==
        std::ranges::end(values)) {
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
    if (!object["name"].isObject()) {
        throw support::Exception(
          std::format("Property has no name (or not an object): {}",
                      jsonValueToString(object)));
    }
    if (!object["description"].isUndefined() &&
        !object["description"].isObject()) {
        throw support::Exception(
          std::format("Property has no description (or not an object): {}",
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
    if (!contents.isObject()) {
        throw support::Exception(std::format(
          "Settings file is not an object: {}", settingsPath.string()));
    }
    if (!contents.object().contains("items") ||
        !contents.object()["items"].isArray()) {
        throw support::Exception(std::format(
          "Settings file has no items array: {}", settingsPath.string()));
    }
    populateScreenVarsRecursive(
      result, themePath, contents.object()["items"].toArray());
    return result;
}

void
readGeneralVars(QThreadPool& writePool,
                resource_managers::GeneralVars& generalVars,
                const std::filesystem::path& profileFolder)
{
    auto file = QFile{ profileFolder / "generalVars.json" };
    if (!file.exists()) {
        writeGeneralVars(writePool, generalVars, profileFolder);
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
        for (auto i = generalVars.metaObject()->propertyOffset();
             i < generalVars.metaObject()->propertyCount();
             ++i) {
            auto property = generalVars.metaObject()->property(i);
            if (contents.contains(property.name())) {
                // if the property is an enum, we need to convert the string
                // to the enum value
                if (property.isEnumType()) {
                    const auto enumValue =
                      property.enumerator().keyToValue(contents[property.name()]
                                                         .toString()
                                                         .toStdString()
                                                         .c_str());
                    property.write(&generalVars, enumValue);
                } else {
                    property.write(&generalVars, contents[property.name()]);
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
        for (const auto& [screen, screenVars] : screens.asKeyValueRange()) {
            vars[screen][name] = screenVars;
        }
    }
    return vars;
}

// I got this from cppreference
template<class T>
std::enable_if_t<not std::numeric_limits<T>::is_integer, bool>
equalWithinUlps(T x, T y, std::size_t n)
{
    // Since `epsilon()` is the gap size (ULP, unit in the last place)
    // of floating-point numbers in interval [1, 2), we can scale it to
    // the gap size in interval [2^e, 2^{e+1}), where `e` is the exponent
    // of `x` and `y`.

    // If `x` and `y` have different gap sizes (which means they have
    // different exponents), we take the smaller one. Taking the bigger
    // one is also reasonable, I guess.
    const T m = std::min(std::fabs(x), std::fabs(y));

    // Subnormal numbers have fixed exponent, which is `min_exponent - 1`.
    const int exp = m < std::numeric_limits<T>::min()
                      ? std::numeric_limits<T>::min_exponent - 1
                      : std::ilogb(m);

    // We consider `x` and `y` equal if the difference between them is
    // within `n` ULPs.
    return std::fabs(x - y) <=
           n * std::ldexp(std::numeric_limits<T>::epsilon(), exp);
}

} // namespace

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
            auto propertyMap =
              std::make_unique<QQmlPropertyMap>(screenPropertyMap.get());
            propertyMap->insert(themeVarsData[screenName][themeName]);
            propertyMap->freeze();
            connect(
              propertyMap.get(),
              &QQmlPropertyMap::valueChanged,
              this,
              [this,
               themeVarsPath,
               screen = screenName,
               themeFamily = themeName](const QString& key,
                                        const QVariant& value) {
                  auto& ref = loadedThemeVars[screen][themeFamily][key];
                  // Skip write if assigning number to number and they are very
                  // close (ULP-based)
                  if ((ref.typeId() == QMetaType::Double ||
                       ref.typeId() == QMetaType::Float) &&
                      (value.typeId() == QMetaType::Double ||
                       ref.typeId() == QMetaType::Float)) {
                      const double oldVal = ref.toDouble();
                      const double newVal = value.toDouble();
                      constexpr std::size_t ulpsTolerance = 16;
                      if (equalWithinUlps(oldVal, newVal, ulpsTolerance)) {
                          return; // numerically equivalent within tolerance
                      }
                  } else {
                      // For non-numeric, avoid redundant writes
                      if (ref == value) {
                          return;
                      }
                  }
                  // Update cached value before writing
                  ref = value;
                  writeSingleThemeVar(
                    writePool,
                    screen,
                    key,
                    value,
                    themeVarsPath /
                      support::qStringToPath(themeFamily +
                                             QStringLiteral("-vars.json")));
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
resource_managers::Vars::writeGeneralVars()
{
    ::writeGeneralVars(
      writePool, generalVars, profile->getPath().parent_path());
}

resource_managers::Vars::Vars(
  const Profile* profile,
  QMap<QString, qml_components::ThemeFamily> availableThemeFamilies,
  QList<QString> assetsPaths,
  QObject* parent)
  : QObject(parent)
  , generalVars(std::move(assetsPaths))
  , profile(profile)
  , availableThemeFamilies(std::move(availableThemeFamilies))
  , loadedThemeVars(readThemeVars(profile->getPath().parent_path(),
                                  this->availableThemeFamilies))
{
    writePool.setMaxThreadCount(1);
    writeThemeVars(loadedThemeVars, profile->getPath().parent_path());
    populateThemePropertyMap(
      themeVars, loadedThemeVars, profile->getPath().parent_path());
    readGeneralVars(writePool, generalVars, profile->getPath().parent_path());
    writeGeneralVars();
    for (auto i = generalVars.metaObject()->propertyOffset();
         i < generalVars.metaObject()->propertyCount();
         ++i) {
        if (!generalVars.metaObject()->property(i).isStored()) {
            continue;
        }
        connect(&generalVars,
                generalVars.metaObject()->property(i).notifySignal(),
                this,
                metaObject()->method(
                  metaObject()->indexOfMethod("writeGeneralVars()")));
    }
}
auto
resource_managers::Vars::getGeneralVars() -> GeneralVars*
{
    return &generalVars;
}
auto
resource_managers::Vars::getThemeVars() -> QQmlPropertyMap*
{
    return &themeVars;
}
