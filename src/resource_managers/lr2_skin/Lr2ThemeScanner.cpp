#include "Lr2ThemeScanner.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QUrl>
#include <iconv.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <system_error>
#include <utility>
#include <vector>

namespace resource_managers::lr2_skin {
namespace {

auto
decodeSkinText(const QByteArray& data) -> QString
{
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(data.sliced(3));
    }

    // LR2-era skin headers commonly omit a BOM and use Japanese Windows
    // CP932. ASCII survives unchanged, so prefer CP932 before UTF-8.
    for (const auto* encoding :
         { "CP932", "windows-31j", "Shift-JIS", "Shift_JIS", "SJIS",
           "MS_Kanji" }) {
        QStringDecoder decoder(encoding);
        if (!decoder.isValid()) {
            continue;
        }
        const auto decoded = decoder.decode(data);
        if (!decoder.hasError()) {
            return decoded;
        }
    }

    const auto invalidIconv =
      reinterpret_cast<iconv_t>(static_cast<std::intptr_t>(-1));
    for (const auto* encoding :
         { "CP932", "Windows-31J", "SHIFT_JIS", "Shift-JIS" }) {
        iconv_t cd = iconv_open("UTF-8", encoding);
        if (cd == invalidIconv) {
            continue;
        }

        auto* srcPtr = const_cast<char*>(data.constData());
        auto srcLeft = static_cast<size_t>(data.size());
        const auto dstSize = static_cast<size_t>(data.size()) * 4 + 4;
        std::vector<char> dstBuf(dstSize);
        auto* dstPtr = dstBuf.data();
        auto dstLeft = dstSize;

        const auto result = iconv(cd, &srcPtr, &srcLeft, &dstPtr, &dstLeft);
        iconv_close(cd);
        if (result != static_cast<size_t>(-1)) {
            return QString::fromUtf8(
              dstBuf.data(), static_cast<qsizetype>(dstSize - dstLeft));
        }
    }

    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    const QString utf8 = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError() && !utf8.contains(QChar(0xFFFD))) {
        return utf8;
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
        lr2filesPath.startsWith(lr2filesPrefix + '/', Qt::CaseInsensitive)) {
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
    bool beatorajaSkin = false;
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
        } else if (command == "#RESOLUTION" || command == "#CUSTOMOFFSET" ||
                   command == "#CUSTOMOPTION_ADDITION_SETTING") {
            beatorajaSkin = true;
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

    QJsonObject rootObj;
    rootObj["title"] = title;
    rootObj["maker"] = maker;
    rootObj["type"] = typeId;
    rootObj["format"] = beatorajaSkin ? "beatoraja" : "lr2";
    rootObj["items"] = itemsArray;
    return QString::fromUtf8(
      QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
}

auto
lr2SkinTypeScreenKey(int typeId) -> QString
{
    static const auto screenKeys = QHash<int, QString>{
        { 0, QStringLiteral("k7") },
        { 1, QStringLiteral("k5") },
        { 2, QStringLiteral("k14") },
        { 3, QStringLiteral("k10") },
        { 5, QStringLiteral("select") },
        { 6, QStringLiteral("decide") },
        { 7, QStringLiteral("result") },
        { 12, QStringLiteral("k7battle") },
        { 13, QStringLiteral("k5battle") },
        { 15, QStringLiteral("courseResult") },
    };

    return screenKeys.value(typeId);
}

auto
makeLr2Screen(const QUrl& wrapperUrl,
              const QString& settingsData,
              const QString& csvPath,
              bool aliased = false) -> qml_components::Screen
{
    return qml_components::Screen{ wrapperUrl, QUrl(""), settingsData,
                                   QUrl(""),   aliased,  csvPath };
}

} // namespace

auto
scanThemeDirectory(const std::filesystem::path& themeDirectory)
  -> QMap<QString, qml_components::ThemeFamily>
{
    auto themeFamilies = QMap<QString, qml_components::ThemeFamily>{};
    try {
        for (const auto& lr2Entry :
             std::filesystem::recursive_directory_iterator(
               themeDirectory,
               std::filesystem::directory_options::skip_permission_denied)) {
            if (!lr2Entry.is_regular_file()) {
                continue;
            }
            if (lr2Entry.path().extension() != ".lr2skin") {
                continue;
            }

            int typeId = -1;
            QString title = "Unknown LR2 Skin";
            QString maker;
            const auto settingsData =
              buildLr2SettingsData(lr2Entry.path(), typeId, title, maker);

            const auto screenKey = lr2SkinTypeScreenKey(typeId);
            if (screenKey.isEmpty()) {
                continue;
            }

            const auto familyName =
              title + " (" +
              support::pathToQString(lr2Entry.path().filename()) + ")";

            const auto csvPath = support::pathToQString(
              std::filesystem::absolute(lr2Entry.path()));
            const auto wrapperUrl =
              QUrl("qrc:///qt/qml/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");

            auto themeMap = QMap<QString, qml_components::Screen>();
            themeMap.insert(screenKey,
                            makeLr2Screen(wrapperUrl, settingsData, csvPath));

            if (screenKey == "k7") {
                themeMap.insert(
                  "k5", makeLr2Screen(wrapperUrl, settingsData, csvPath, true));
            }
            if (screenKey == "k7battle") {
                themeMap.insert(
                  "k5battle",
                  makeLr2Screen(wrapperUrl, settingsData, csvPath, true));
            }
            if (screenKey == "k14") {
                themeMap.insert(
                  "k10",
                  makeLr2Screen(wrapperUrl, settingsData, csvPath, true));
            }

            auto themeFamily = qml_components::ThemeFamily{
                support::pathToQString(
                  std::filesystem::absolute(lr2Entry.path().parent_path())),
                std::move(themeMap),
                QMap<QString, QUrl>()
            };
            themeFamilies.insert(familyName, themeFamily);
        }
    } catch (const std::exception& e) {
        spdlog::warn("Error scanning LR2 skins in {}: {}",
                     themeDirectory.string(),
                     e.what());
    }
    return themeFamilies;
}

} // namespace resource_managers::lr2_skin
