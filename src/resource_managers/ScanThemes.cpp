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
#include <QRegularExpression>
#include <spdlog/spdlog.h>
#include <fstream>
#include <QHash>

namespace {
auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
}

auto
buildLr2SettingsData(const std::filesystem::path& lr2SkinPath,
                     int& typeId,
                     QString& title,
                     QString& maker) -> QString
{
    std::ifstream ifs(lr2SkinPath);
    if (!ifs.is_open()) {
        return {};
    }

    QJsonArray itemsArray;
    std::string line;
    while (std::getline(ifs, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty() || line.starts_with("//")) {
            continue;
        }

        const auto parts = QString::fromStdString(line).split(',');
        if (parts.isEmpty()) {
            continue;
        }

        const auto command = parts[0].trimmed().toUpper();
        if (command == "#INFORMATION") {
            if (parts.size() >= 2) {
                typeId = parts[1].trimmed().toInt();
            }
            if (parts.size() >= 3 && !parts[2].trimmed().isEmpty()) {
                title = parts[2].trimmed();
            }
            if (parts.size() >= 4) {
                maker = parts[3].trimmed();
            }
        } else if (command == "#CUSTOMOPTION") {
            if (parts.size() < 4) {
                continue;
            }

            const auto name = parts[1].trimmed();
            const auto optionId = parts[2].trimmed();
            QJsonArray choicesArray;
            for (int i = 3; i < parts.size(); ++i) {
                const auto value = parts[i].trimmed();
                if (value.isEmpty()) {
                    continue;
                }
                QJsonObject choice;
                choice["value"] = value;
                QJsonObject choiceName;
                choiceName["en"] = value;
                choice["name"] = choiceName;
                choicesArray.append(choice);
            }
            if (choicesArray.isEmpty()) {
                continue;
            }

            QJsonObject item;
            item["type"] = "choice";
            item["id"] = makeSafeId(name, "opt_" + optionId);
            QJsonObject nameObj;
            nameObj["en"] = name;
            item["name"] = nameObj;
            item["choices"] = choicesArray;
            item["default"] = choicesArray[0].toObject()["value"].toString();
            itemsArray.append(item);
        } else if (command == "#CUSTOMFILE") {
            if (parts.size() < 3) {
                continue;
            }

            const auto name = parts[1].trimmed();
            const auto rawPattern = support::qStringToPath(parts[2].trimmed());
            auto parent = rawPattern.parent_path();
            auto rel = support::pathToQString(
              relative(parent.empty() ? std::filesystem::path(".") : parent,
                       "LR2files/"));
            rel.replace(QRegularExpression(
                          "^Theme", QRegularExpression::CaseInsensitiveOption),
                        "themes");

            QJsonObject item;
            item["type"] = "file";
            item["id"] =
              makeSafeId(name, "file_" + QString::number(itemsArray.size()));
            QJsonObject nameObj;
            nameObj["en"] = name;
            item["name"] = nameObj;
            item["path"] = "../../../" + rel;

            if (parts.size() >= 4) {
                const auto defaultStem = parts[3].trimmed();
                if (!defaultStem.isEmpty()) {
                    const auto absoluteDir = lr2SkinPath.parent_path()
                                               .parent_path()
                                               .parent_path()
                                               .parent_path() /
                                             support::qStringToPath(rel);
                    std::error_code ec;
                    if (std::filesystem::exists(absoluteDir, ec)) {
                        for (const auto& fileEntry :
                             std::filesystem::directory_iterator(absoluteDir,
                                                                 ec)) {
                            if (!fileEntry.is_regular_file()) {
                                continue;
                            }
                            if (support::pathToQString(
                                  fileEntry.path().stem()) == defaultStem) {
                                item["default"] = support::pathToQString(
                                  fileEntry.path().filename());
                                break;
                            }
                        }
                    }
                }
            }

            itemsArray.append(item);
        } else if (command == "#ENDOFHEADER") {
            break;
        }
    }

    if (itemsArray.isEmpty()) {
        return {};
    }

    QJsonObject rootObj;
    rootObj["items"] = itemsArray;
    return QString::fromUtf8(
      QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
}
} // namespace

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
            try {
                for (const auto& lr2Entry :
                     std::filesystem::recursive_directory_iterator(
                       path,
                       std::filesystem::directory_options::
                         skip_permission_denied)) {
                    if (!lr2Entry.is_regular_file())
                        continue;
                    if (lr2Entry.path().extension() != ".lr2skin")
                        continue;

                    int typeId = -1;
                    QString title = "Unknown LR2 Skin";
                    QString maker;
                    const auto settingsData = buildLr2SettingsData(
                      lr2Entry.path(), typeId, title, maker);

                    auto lr2TypeMap = QMap<int, QString>{
                        { 0, "k7" },           { 1, "k5" },
                        { 2, "k14" },          { 3, "k10" },
                        { 5, "select" },       { 6, "decide" },
                        { 7, "result" },       { 12, "k7battle" },
                        { 13, "k5battle" },    { 14, "k14battle" },
                        { 15, "courseResult" }
                    };

                    if (typeId != -1 && lr2TypeMap.contains(typeId)) {
                        QString screenKey = lr2TypeMap[typeId];
                        QString familyName =
                          title + " (" +
                          support::pathToQString(lr2Entry.path().filename()) +
                          ")";

                        QString csvPathStr = support::pathToQString(
                          std::filesystem::absolute(lr2Entry.path()));
                        QUrl wrapperUrl("qrc:///qt/qml/RhythmGameQml/"
                                        "Lr2SkinScreenWrapper.qml");

                        auto screen =
                          qml_components::Screen{ wrapperUrl,   QUrl(""),
                                                  settingsData, QUrl(""),
                                                  false,        csvPathStr };

                        auto themeMap = QMap<QString, qml_components::Screen>();
                        themeMap.insert(screenKey, screen);

                        if (screenKey == "k7")
                            themeMap.insert(
                              "k5",
                              qml_components::Screen{ wrapperUrl,
                                                      QUrl(""),
                                                      settingsData,
                                                      QUrl(""),
                                                      true,
                                                      csvPathStr });
                        if (screenKey == "k7battle")
                            themeMap.insert(
                              "k5battle",
                              qml_components::Screen{ wrapperUrl,
                                                      QUrl(""),
                                                      settingsData,
                                                      QUrl(""),
                                                      true,
                                                      csvPathStr });
                        if (screenKey == "k14")
                            themeMap.insert(
                              "k10",
                              qml_components::Screen{ wrapperUrl,
                                                      QUrl(""),
                                                      settingsData,
                                                      QUrl(""),
                                                      true,
                                                      csvPathStr });

                        auto themeFamily = qml_components::ThemeFamily{
                            support::pathToQString(std::filesystem::absolute(
                              lr2Entry.path().parent_path())),
                            std::move(themeMap),
                            QMap<QString, QUrl>()
                        };
                        themeFamilies.insert(familyName, themeFamily);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::warn("Error scanning LR2 skins in {}: {}",
                             path.string(),
                             e.what());
            }
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
            auto scriptObj = scripts.toObject().toVariantMap();
            for (const auto& [key, value] : scriptObj.asKeyValueRange()) {
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

                auto screen = qml_components::Screen{
                    QUrl::fromLocalFile(support::pathToQString(
                      path / support::qStringToPath(value.toString()))),
                    settingsUrl,
                    QString{},
                    settingsScriptUrl,
                };
                themeMap[key] = screen;
                auto aliasedScreen = qml_components::Screen{ screen.getScript(),
                                                             settingsUrl,
                                                             QString{},
                                                             settingsScriptUrl,
                                                             true };
                if (key == "k7" && !scriptObj.contains("k5")) {
                    themeMap["k5"] = aliasedScreen;
                }
                if (key == "k7battle" && !scriptObj.contains("k5battle")) {
                    themeMap["k5battle"] = aliasedScreen;
                }
                if (key == "k14" && !scriptObj.contains("k10")) {
                    themeMap["k10"] = aliasedScreen;
                }
            }
            // Load translations
            auto translations = QMap<QString, QUrl>{};
            if (config["translations"].isObject()) {
                for (const auto& [language, translation] :
                     config["translations"]
                       .toObject()
                       .toVariantHash()
                       .asKeyValueRange()) {
                    auto translationPath =
                      path / support::qStringToPath(translation.toString());
                    if (exists(translationPath)) {
                        translations.insert(
                          language,
                          QUrl::fromLocalFile(
                            support::pathToQString(translationPath)));
                    } else {
                        spdlog::warn(
                          "Translation file {} does not exist for theme {}",
                          translationPath.string(),
                          path.string());
                    }
                }
            }
            auto themeName = support::pathToQString(path.filename());
            auto themeFamily = qml_components::ThemeFamily{
                support::pathToQString(path),
                std::move(themeMap),
                translations,
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
    if (!object.contains("k5") && object.contains("k7")) {
        object.insert("k5", object.value("k7"));
    }
    if (!object.contains("k5battle") && object.contains("k7battle")) {
        object.insert("k5battle", object.value("k7battle"));
    }
    if (!object.contains("k14") && object.contains("k14battle")) {
        object.insert("k14", object.value("k14battle"));
    }
}
