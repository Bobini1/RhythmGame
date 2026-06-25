#include "Lr2ThemeScanner.h"

#include "support/PathToQString.h"
#include "support/PathToUtfString.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QUrl>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <iconv.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace resource_managers::lr2_skin {
namespace {
#ifdef _WIN32
auto
decodeCp932WithWindows(const QByteArray& data) -> std::optional<QString>
{
    const auto size = static_cast<int>(data.size());
    if (size == 0) {
        return QString{};
    }

    const int wideSize = MultiByteToWideChar(
      932, MB_ERR_INVALID_CHARS, data.constData(), size, nullptr, 0);
    if (wideSize <= 0) {
        return std::nullopt;
    }

    std::wstring wide(static_cast<std::size_t>(wideSize), L'\0');
    const int converted = MultiByteToWideChar(
      932, MB_ERR_INVALID_CHARS, data.constData(), size, wide.data(), wideSize);
    if (converted <= 0) {
        return std::nullopt;
    }

    return QString::fromWCharArray(wide.data(), converted);
}
#endif

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

#ifdef _WIN32
    if (const auto decoded = decodeCp932WithWindows(data)) {
        return *decoded;
    }
#endif
    for (const auto* encoding : { "CP932",
                                  "windows-31j",
                                  "Shift-JIS",
                                  "Shift_JIS",
                                  "SJIS",
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
            return QString::fromUtf8(dstBuf.data(),
                                     static_cast<qsizetype>(dstSize - dstLeft));
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
normalizeLr2CsvToken(QString token) -> QString
{
    token = token.trimmed();
    while (token.startsWith(QLatin1Char('"'))) {
        token.remove(0, 1);
        token = token.trimmed();
    }
    while (token.endsWith(QLatin1Char('"'))) {
        token.chop(1);
        token = token.trimmed();
    }
    return token;
}

auto
tokenizeLr2HeaderLine(const QString& line) -> QStringList
{
    auto tokens = line.split(',');
    for (auto& token : tokens) {
        token = normalizeLr2CsvToken(token);
    }
    return tokens;
}

struct SettingsIdState
{
    QHash<QString, int> usedIds;
    QHash<int, QString> duplicateFallbacks;
    QHash<int, QString> itemTypes;
};

auto
availableSettingsId(QString id, const QHash<QString, int>& usedIds) -> QString
{
    if (id.isEmpty()) {
        id = QStringLiteral("setting");
    }
    if (!usedIds.contains(id)) {
        return id;
    }

    const auto base = id;
    int suffix = 2;
    do {
        id = QStringLiteral("%1_%2").arg(base).arg(suffix++);
    } while (usedIds.contains(id));
    return id;
}

auto
assignSettingsItemId(const QString& text,
                     const QString& fallback,
                     const QString& type,
                     const int itemIndex,
                     QJsonArray& itemsArray,
                     SettingsIdState& state) -> QString
{
    const auto baseId = makeSafeId(text, fallback);
    const auto fallbackId =
      makeSafeId(fallback, QStringLiteral("setting_%1").arg(itemIndex));

    // LR2 skins often have an option and its backing file picker named almost
    // the same, for example SUDDEN+ Lanecover and SUDDEN Lanecover. Preserve
    // the historical file id when that happens, and move the choice to its
    // option-number fallback so saved file selections keep working.
    if (type == QStringLiteral("file") && state.usedIds.contains(baseId)) {
        const auto previousIndex = state.usedIds.value(baseId);
        if (state.itemTypes.value(previousIndex) == QStringLiteral("choice")) {
            auto previousItem = itemsArray[previousIndex].toObject();
            const auto previousFallback = state.duplicateFallbacks.value(
              previousIndex, QStringLiteral("setting_%1").arg(previousIndex));
            const auto previousId =
              availableSettingsId(previousFallback, state.usedIds);
            previousItem[QStringLiteral("id")] = previousId;
            itemsArray[previousIndex] = previousItem;
            state.usedIds.remove(baseId);
            state.usedIds.insert(previousId, previousIndex);
        }
    }

    const auto id = availableSettingsId(
      state.usedIds.contains(baseId) ? fallbackId : baseId, state.usedIds);
    state.usedIds.insert(id, itemIndex);
    state.duplicateFallbacks.insert(itemIndex, fallbackId);
    state.itemTypes.insert(itemIndex, type);
    return id;
}

auto
findLr2filesRoot(const std::filesystem::path& currentDir)
  -> std::filesystem::path
{
    for (auto dir = currentDir; !dir.empty();) {
        const auto name = support::pathToQString(dir.filename());
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
customFileDefault(const std::filesystem::path& directory,
                  const QString& defaultStem) -> QString
{
    QString firstSelectable;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec)) {
        return {};
    }

    for (auto iterator = std::filesystem::directory_iterator(directory, ec);
         iterator != std::filesystem::directory_iterator{};
         iterator.increment(ec)) {
        if (ec) {
            break;
        }

        const auto& fileEntry = *iterator;
        if (!fileEntry.is_regular_file()) {
            continue;
        }

        const auto filename =
          support::pathToQString(fileEntry.path().filename());
        if (filename.startsWith('.') || filename.endsWith(".ini")) {
            continue;
        }
        if (firstSelectable.isEmpty()) {
            firstSelectable = filename;
        }
        if (!defaultStem.isEmpty() &&
            support::pathToQString(fileEntry.path().stem()) == defaultStem) {
            return filename;
        }
    }

    return firstSelectable;
}

auto
fallbackToCurrentTheme(const std::filesystem::path& currentDir,
                       const std::filesystem::path& lr2filesRoot,
                       const std::filesystem::path& lr2filesRelative)
  -> std::filesystem::path
{
    auto it = lr2filesRelative.begin();
    if (it == lr2filesRelative.end() || support::pathToQString(*it).compare(
                                          "themes", Qt::CaseInsensitive) != 0) {
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
    SettingsIdState idState;
    bool beatorajaSkin = false;
    const auto lines = decodeSkinText(file.readAll()).split('\n');
    for (auto line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) {
            continue;
        }

        const auto parts = tokenizeLr2HeaderLine(line);
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
        } else if (command == "#CUSTOMOFFSET" ||
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
            item["id"] =
              assignSettingsItemId(name,
                                   "opt_" + optionId,
                                   QStringLiteral("choice"),
                                   static_cast<int>(itemsArray.size()),
                                   itemsArray,
                                   idState);
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
              assignSettingsItemId(name,
                                   "file_" + QString::number(itemsArray.size()),
                                   QStringLiteral("file"),
                                   static_cast<int>(itemsArray.size()),
                                   itemsArray,
                                   idState);
            QJsonObject nameObj;
            nameObj["en"] = name;
            item["name"] = nameObj;
            item["path"] = rel;

            QString defaultFile;
            if (parts.size() >= 4) {
                defaultFile =
                  customFileDefault(absoluteDir, parts[3].trimmed());
            }
            if (defaultFile.isEmpty()) {
                defaultFile = customFileDefault(absoluteDir, {});
            }
            if (!defaultFile.isEmpty()) {
                item["default"] = defaultFile;
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
                     support::pathToUtfString(themeDirectory),
                     e.what());
    }
    return themeFamilies;
}

} // namespace resource_managers::lr2_skin
