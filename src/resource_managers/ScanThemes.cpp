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
#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>
#include <spdlog/spdlog.h>
#include <QHash>

namespace {
auto
decodeSkinText(const QByteArray& data) -> QString
{
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(data.sliced(3));
    }

    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    const QString utf8 = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError() && !utf8.contains(QChar(0xFFFD))) {
        return utf8;
    }

    for (const auto* encoding :
         { "CP932", "windows-31j", "Shift-JIS" }) {
        QStringDecoder decoder(encoding);
        if (!decoder.isValid()) {
            continue;
        }
        const auto decoded = decoder.decode(data);
        if (!decoder.hasError()) {
            return decoded;
        }
    }

    return QString::fromLatin1(data);
}

auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
}

auto
findLr2filesRoot(const std::filesystem::path& currentDir)
  -> std::filesystem::path
{
    for (auto dir = currentDir; !dir.empty();) {
        const auto name =
          QString::fromStdString(dir.filename().generic_string());
        if (name.compare("themes", Qt::CaseInsensitive) == 0 &&
            !dir.parent_path().empty()) {
            return dir.parent_path();
        }

        std::error_code ec;
        if (std::filesystem::is_directory(dir / "themes", ec)) {
            return dir;
        }

        const auto parent = dir.parent_path();
        if (parent == dir) {
            break;
        }
        dir = parent;
    }

    return currentDir.parent_path().parent_path().parent_path();
}

auto
currentThemeRoot(const std::filesystem::path& currentDir,
                 const std::filesystem::path& lr2filesRoot)
  -> std::filesystem::path
{
    const auto themesRoot = (lr2filesRoot / "themes").lexically_normal();
    for (auto dir = std::filesystem::absolute(currentDir).lexically_normal();
         !dir.empty();) {
        if (dir.parent_path().lexically_normal() == themesRoot) {
            return dir;
        }

        const auto parent = dir.parent_path();
        if (parent == dir) {
            break;
        }
        dir = parent;
    }
    return {};
}

auto
pathOrWildcardParentExists(const std::filesystem::path& path) -> bool
{
    const auto pathText = support::pathToQString(path);
    const auto probe = pathText.contains('*') ? path.parent_path() : path;
    std::error_code ec;
    return !probe.empty() && std::filesystem::exists(probe, ec);
}

auto
fallbackToCurrentTheme(const std::filesystem::path& currentDir,
                       const std::filesystem::path& lr2filesRoot,
                       const std::filesystem::path& lr2filesRelative)
  -> std::filesystem::path
{
    auto it = lr2filesRelative.begin();
    if (it == lr2filesRelative.end() ||
        QString::fromStdString(it->generic_string())
            .compare("themes", Qt::CaseInsensitive) != 0) {
        return {};
    }
    ++it;
    if (it == lr2filesRelative.end()) {
        return {};
    }
    ++it;

    auto tail = std::filesystem::path{};
    for (; it != lr2filesRelative.end(); ++it) {
        tail /= *it;
    }
    if (tail.empty()) {
        return {};
    }

    const auto themeRoot = currentThemeRoot(currentDir, lr2filesRoot);
    return themeRoot.empty()
             ? std::filesystem::path{}
             : std::filesystem::absolute(themeRoot / tail).lexically_normal();
}

auto
resolveLr2RawPath(const std::filesystem::path& currentDir, const QString& token)
  -> std::filesystem::path
{
    auto lr2filesPath = token.trimmed();
    lr2filesPath.replace('\\', '/');
    while (lr2filesPath.startsWith(QStringLiteral("./"))) {
        lr2filesPath.remove(0, 2);
    }

    const auto lr2filesPrefix = QStringLiteral("LR2files");
    if (lr2filesPath.compare(lr2filesPrefix, Qt::CaseInsensitive) == 0 ||
        lr2filesPath.startsWith(lr2filesPrefix + '/',
                                Qt::CaseInsensitive)) {
        lr2filesPath.remove(0, lr2filesPrefix.size());
        if (lr2filesPath.startsWith('/')) {
            lr2filesPath.remove(0, 1);
        }
        lr2filesPath.replace(
          QRegularExpression("^Theme(?=/|$)",
                             QRegularExpression::CaseInsensitiveOption),
          "themes");

        const auto lr2filesRoot = findLr2filesRoot(currentDir);
        const auto relativePath = support::qStringToPath(lr2filesPath);
        const auto resolved =
          std::filesystem::absolute(lr2filesRoot / relativePath)
            .lexically_normal();
        if (pathOrWildcardParentExists(resolved)) {
            return resolved;
        }

        const auto fallback =
          fallbackToCurrentTheme(currentDir, lr2filesRoot, relativePath);
        if (!fallback.empty() && pathOrWildcardParentExists(fallback)) {
            return fallback;
        }
        return resolved;
    }

    auto path = support::qStringToPath(lr2filesPath);
    if (path.is_relative()) {
        path = currentDir / path;
    }
    return std::filesystem::absolute(path).lexically_normal();
}

auto
buildLr2SettingsData(const std::filesystem::path& lr2SkinPath,
                     int& typeId,
                     QString& title,
                     QString& maker) -> QString
{
    QFile file(support::pathToQString(lr2SkinPath));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonArray itemsArray;
    const auto lines = decodeSkinText(file.readAll()).split('\n');
    for (auto line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) {
            continue;
        }

        const auto parts = line.split(',');
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
            const auto absolutePattern =
              resolveLr2RawPath(lr2SkinPath.parent_path(), parts[2].trimmed());
            const auto absoluteDir = absolutePattern.parent_path();
            auto rel = support::pathToQString(
              relative(absoluteDir, lr2SkinPath.parent_path()));
            if (rel == ".") {
                rel.clear();
            }

            QJsonObject item;
            item["type"] = "file";
            item["id"] =
              makeSafeId(name, "file_" + QString::number(itemsArray.size()));
            QJsonObject nameObj;
            nameObj["en"] = name;
            item["name"] = nameObj;
            item["path"] = rel;

            if (parts.size() >= 4) {
                const auto defaultStem = parts[3].trimmed();
                if (!defaultStem.isEmpty()) {
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
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }
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
