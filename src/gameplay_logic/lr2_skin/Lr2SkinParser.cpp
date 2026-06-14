#include "Lr2SkinParser.h"

#include "support/PathToQString.h"
#include "support/PathToUtfString.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QStringList>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iconv.h>
#include <optional>
#include <set>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace gameplay_logic::lr2_skin {
namespace {
struct CustomFile
{
    QString settingId;
    std::filesystem::path directory;
    QString wildcard;
    QString defaultSelection;
};

struct SettingsIdState
{
    QHash<QString, int> usedIds;
    QHash<int, QString> duplicateFallbacks;
    QHash<int, QString> itemTypes;
};

struct FontDefinition
{
    QString path;
    QString family;
    int size = 0;
    int thickness = 0;
    int type = 0;
    bool bitmap = false;
};

struct ParseState
{
    QList<Lr2Element> elements;
    QStringList images;
    QList<Lr2SrcImage> imageSets;
    QList<FontDefinition> systemFonts;
    QList<FontDefinition> imageFonts;
    QList<CustomFile> customFiles;
    QMap<int, Lr2SrcImage> barBodySources;
    QMap<int, Lr2SrcImage> barLampSources;
    QMap<int, Lr2SrcImage> barMyLampSources;
    QMap<int, Lr2SrcImage> barRivalLampSources;
    QMap<int, Lr2SrcImage> barRankSources;
    QMap<int, Lr2SrcImage> barRivalSources;
    QMap<int, Lr2SrcImage> barLabelSources;
    QMap<int, Lr2SrcNumber> barLevelSources;
    QMap<int, Lr2SrcBarText> barTitleSources;
    Lr2SrcBarGraph barDistributionGraphSource;
    bool hasBarDistributionGraphSource = false;
    Lr2SrcImage barFlashSource;
    bool hasBarFlashSource = false;
    QMap<int, QVariantList> barBodyOffDsts;
    QMap<int, QVariantList> barBodyOnDsts;
    QMap<int, Lr2SrcImage> noteSources;
    QMap<int, Lr2SrcImage> mineSources;
    QMap<int, Lr2SrcImage> lnStartSources;
    QMap<int, Lr2SrcImage> lnEndSources;
    QMap<int, Lr2SrcImage> lnBodySources;
    QMap<int, Lr2SrcImage> lnBodyActiveSources;
    QMap<int, Lr2SrcImage> autoNoteSources;
    QMap<int, Lr2SrcImage> autoMineSources;
    QMap<int, Lr2SrcImage> autoLnStartSources;
    QMap<int, Lr2SrcImage> autoLnEndSources;
    QMap<int, Lr2SrcImage> autoLnBodySources;
    QMap<int, Lr2SrcImage> autoLnBodyActiveSources;
    QMap<int, QVariantList> noteDsts;
    QMap<int, Lr2SrcImage> lineSources;
    QMap<int, QVariantList> lineDsts;
    QMap<int, QList<Lr2Dst>> nowJudgeDsts;
    QMap<int, int> nowComboDstCounts;
    bool hasNoteElement = false;
    QVariantList helpFiles;
    QString transColor = "#000000";
    bool hasTransColor = false;
    bool reloadBanner = false;
    int barCenter = 0;
    int barAvailableStart = 0;
    int barAvailableEnd = -1;
    std::set<int> activeOptions;
    std::set<int> usedOptions;
    std::set<int> usedElementOptions;
    Lr2Element currentElement;
    bool hasCurrentElement = false;
    QVariantMap settingValues;
    int startInput = 0;
    int sceneTime = 0;
    int loadStart = 0;
    int loadEnd = 0;
    int playStart = 2000;
    int fadeOut = 0;
    int skip = 0;
    int skinWidth = 640;
    int skinHeight = 480;
    bool hasSkinResolution = false;
    int sortId = 0;
    QString laneCoverSource;
    SettingsIdState settingsIdState;
    int settingsItemIndex = 0;
};

auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
}

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
                     SettingsIdState& state) -> QString
{
    const auto baseId = makeSafeId(text, fallback);
    const auto fallbackId = makeSafeId(fallback,
                                       QStringLiteral("setting_%1")
                                         .arg(itemIndex));

    // Keep runtime setting IDs in lockstep with Lr2ThemeScanner. LR2 stores
    // CUSTOMOPTION/CUSTOMFILE values by shared header-entry order; our public
    // settings use stable IDs, so the parser must derive the same fallback IDs
    // as the scanner, especially for non-ASCII option names.
    if (type == QStringLiteral("file") && state.usedIds.contains(baseId)) {
        const auto previousIndex = state.usedIds.value(baseId);
        if (state.itemTypes.value(previousIndex) == QStringLiteral("choice")) {
            const auto previousFallback =
              state.duplicateFallbacks.value(previousIndex,
                                             QStringLiteral("setting_%1")
                                               .arg(previousIndex));
            const auto previousId =
              availableSettingsId(previousFallback, state.usedIds);
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
normalizeCommand(QString token) -> QString
{
    auto command = token.trimmed().toUpper();
    if (!command.isEmpty() && !command.startsWith(QLatin1Char('#')) &&
        command.at(0).isLetter()) {
        static const QSet<QString> hashOptionalCommands = {
            QStringLiteral("IF"),
            QStringLiteral("ELSE"),
            QStringLiteral("ELSEIF"),
            QStringLiteral("ENDIF"),
            QStringLiteral("CUSTOMOPTION"),
            QStringLiteral("CUSTOMFILE"),
            QStringLiteral("DISABLEFLIP"),
            QStringLiteral("FLIPRESULT"),
        };
        if (!hashOptionalCommands.contains(command)) {
            return command;
        }
        command.prepend(QLatin1Char('#'));
    }
    return command;
}

auto
lr2ConfiguredFontFamily() -> QString
{
    // The stock LR2 skin relies on MS PGothic for Japanese system-font text.
    return QStringLiteral("MS PGothic");
}

auto
colorKeyString(int r, int g, int b) -> QString
{
    return QString("#%1%2%3")
      .arg(std::clamp(r, 0, 255), 2, 16, QLatin1Char('0'))
      .arg(std::clamp(g, 0, 255), 2, 16, QLatin1Char('0'))
      .arg(std::clamp(b, 0, 255), 2, 16, QLatin1Char('0'));
}

void
flushCurrentElement(ParseState& state)
{
    if (state.hasCurrentElement) {
        state.elements.append(state.currentElement);
        state.currentElement = Lr2Element{};
        state.hasCurrentElement = false;
    }
}

auto
tokenizeLine(QString raw) -> QStringList
{
    const auto commentPos = raw.indexOf("//");
    if (commentPos >= 0) {
        raw = raw.left(commentPos);
    }
    raw = raw.trimmed();
    if (raw.isEmpty()) {
        return {};
    }
    auto tokens = raw.split(',');
    for (auto& token : tokens) {
        token = token.trimmed();
        while (token.startsWith(QLatin1Char('"'))) {
            token.remove(0, 1);
            token = token.trimmed();
        }
        while (token.endsWith(QLatin1Char('"'))) {
            token.chop(1);
            token = token.trimmed();
        }
    }
    return tokens;
}

auto
decodeSkinText(const QByteArray& data) -> QString
{
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(data.sliced(3));
    }

    // LR2-era skin files usually omit a BOM and use Japanese Windows CP932.
    // ASCII survives unchanged, so prefer CP932 before UTF-8.
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
parseLr2IntegerPrefix(QString token, bool* ok = nullptr) -> int
{
    token = token.trimmed();
    if (token.isEmpty()) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0;
    }

    qsizetype index = 0;
    int sign = 1;
    if (token[index] == QLatin1Char('+')) {
        ++index;
    } else if (token[index] == QLatin1Char('-')) {
        sign = -1;
        ++index;
    }

    qint64 value = 0;
    bool hasDigit = false;
    while (index < token.size()) {
        const auto ch = token[index];
        if (!ch.isDigit()) {
            break;
        }
        hasDigit = true;
        value = value * 10 + ch.digitValue();
        if (value > 2147483647) {
            value = 2147483647;
            break;
        }
        ++index;
    }

    if (!hasDigit) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0;
    }

    if (ok != nullptr) {
        *ok = true;
    }
    return static_cast<int>(sign * value);
}

auto
parseDstOptionToken(QString token) -> int
{
    token = token.trimmed();
    if (token.isEmpty()) {
        return 0;
    }

    const bool negated = token.startsWith(QLatin1Char('!'));
    if (negated) {
        token.remove(0, 1);
    }

    bool ok = false;
    const int option = parseLr2IntegerPrefix(token, &ok);
    if (!ok || option == 0) {
        return 0;
    }

    return negated ? -option : option;
}

auto
parseDstOffsets(const QStringList& tokens, const int startIndex) -> QVariantList
{
    QVariantList offsets;
    for (int i = startIndex; i < tokens.size(); ++i) {
        auto token = tokens[i].trimmed();
        if (token.isEmpty()) {
            continue;
        }
        token.remove(QRegularExpression(QStringLiteral("[^0-9-]")));
        if (token.isEmpty()) {
            continue;
        }

        bool ok = false;
        const int offset = token.toInt(&ok);
        if (ok && offset != 0) {
            offsets.append(offset);
        }
    }
    return offsets;
}

void
ensureDstOffset(Lr2Dst& dst, const int offset)
{
    for (const auto& value : dst.offsets) {
        if (value.toInt() == offset) {
            return;
        }
    }
    dst.offsets.append(offset);
}

auto
parseDstInteger(const QString& token) -> int
{
    bool ok = false;
    const int integerValue = parseLr2IntegerPrefix(token, &ok);
    if (ok) {
        return integerValue;
    }

    const double realValue = token.toDouble(&ok);
    if (ok) {
        return static_cast<int>(realValue);
    }

    return 0;
}

auto
parseDstValue(const QStringList& tokens, const int sortId) -> Lr2Dst
{
    Lr2Dst dst;
    dst.sortId = sortId;
    if (tokens.size() > 2 && !tokens[2].isEmpty())
        dst.time = parseDstInteger(tokens[2]);
    if (tokens.size() > 3 && !tokens[3].isEmpty())
        dst.x = parseDstInteger(tokens[3]);
    if (tokens.size() > 4 && !tokens[4].isEmpty())
        dst.y = parseDstInteger(tokens[4]);
    if (tokens.size() > 5 && !tokens[5].isEmpty())
        dst.w = parseDstInteger(tokens[5]);
    if (tokens.size() > 6 && !tokens[6].isEmpty())
        dst.h = parseDstInteger(tokens[6]);
    if (tokens.size() > 7 && !tokens[7].isEmpty())
        dst.acc = parseDstInteger(tokens[7]);
    if (tokens.size() > 8 && !tokens[8].isEmpty())
        dst.a = parseDstInteger(tokens[8]);
    if (tokens.size() > 9 && !tokens[9].isEmpty())
        dst.r = parseDstInteger(tokens[9]);
    if (tokens.size() > 10 && !tokens[10].isEmpty())
        dst.g = parseDstInteger(tokens[10]);
    if (tokens.size() > 11 && !tokens[11].isEmpty())
        dst.b = parseDstInteger(tokens[11]);
    if (tokens.size() > 12 && !tokens[12].isEmpty())
        dst.blend = parseDstInteger(tokens[12]);
    if (tokens.size() > 13 && !tokens[13].isEmpty())
        dst.filter = parseDstInteger(tokens[13]);
    if (tokens.size() > 14 && !tokens[14].isEmpty())
        dst.angle = parseDstInteger(tokens[14]);
    if (tokens.size() > 15 && !tokens[15].isEmpty())
        dst.center = parseDstInteger(tokens[15]);
    if (tokens.size() > 16 && !tokens[16].isEmpty())
        dst.loop = parseDstInteger(tokens[16]);
    if (tokens.size() > 17 && !tokens[17].isEmpty())
        dst.timer = parseDstInteger(tokens[17]);
    if (tokens.size() > 18 && !tokens[18].isEmpty())
        dst.op1 = parseDstOptionToken(tokens[18]);
    if (tokens.size() > 19 && !tokens[19].isEmpty())
        dst.op2 = parseDstOptionToken(tokens[19]);
    if (tokens.size() > 20 && !tokens[20].isEmpty())
        dst.op3 = parseDstOptionToken(tokens[20]);
    if (tokens.size() > 21 && !tokens[21].isEmpty())
        dst.op4 = parseDstInteger(tokens[21]);
    dst.offsets = parseDstOffsets(tokens, 21);
    return dst;
}

void
recordDstOption(std::set<int>& options, const int option)
{
    if (option == 0) {
        return;
    }
    options.insert(option < 0 ? -option : option);
}

void
recordDstOption(ParseState& state, const int option, const bool elementOption)
{
    recordDstOption(state.usedOptions, option);
    if (elementOption) {
        recordDstOption(state.usedElementOptions, option);
    }
}

void
recordDstOptions(ParseState& state,
                 const Lr2Dst& dst,
                 const bool elementOptions)
{
    recordDstOption(state, dst.op1, elementOptions);
    recordDstOption(state, dst.op2, elementOptions);
    recordDstOption(state, dst.op3, elementOptions);
}

void
parseDst(const QStringList& tokens, ParseState& state, Lr2Element& element)
{
    const auto dst = parseDstValue(tokens, state.sortId);
    recordDstOptions(state, dst, true);
    element.dsts.append(QVariant::fromValue(dst));
}

void
parseBarDst(const QStringList& tokens, ParseState& state, Lr2Element& element)
{
    const auto dst = parseDstValue(tokens, state.sortId);
    recordDstOptions(state, dst, false);
    element.dsts.append(QVariant::fromValue(dst));
}

void
addDstOptionGate(Lr2Dst& dst, const int option)
{
    if (option == 0) {
        return;
    }
    if (dst.op1 == 0) {
        dst.op1 = option;
    } else if (dst.op2 == 0) {
        dst.op2 = option;
    } else if (dst.op3 == 0) {
        dst.op3 = option;
    }
}

void
parseDstWithOptionGate(const QStringList& tokens,
                       ParseState& state,
                       Lr2Element& element,
                       const int option)
{
    auto dst = parseDstValue(tokens, state.sortId);
    addDstOptionGate(dst, option);
    recordDstOptions(state, dst, true);
    element.dsts.append(QVariant::fromValue(dst));
}

auto
lr2NowJudgementOption(const QString& command, const int judgementIndex) -> int
{
    if (judgementIndex < 0 || judgementIndex > 5) {
        return 0;
    }
    const int baseOption = command.endsWith(QStringLiteral("_2P")) ? 261 : 241;
    return baseOption + (5 - judgementIndex);
}

auto
lr2NowCommandIndex(const QStringList& tokens) -> int
{
    return tokens.size() > 1 && !tokens[1].isEmpty() ? tokens[1].toInt() : -1;
}

auto
lr2NowDisplayTimer(const QString& command) -> int
{
    return command.endsWith(QStringLiteral("_2P")) ? 47 : 46;
}

auto
lr2NowSide(const QString& command) -> int
{
    return command.endsWith(QStringLiteral("_2P")) ? 2 : 1;
}

auto
lr2NowStateKey(const int side, const int index) -> int
{
    return side * 10 + index;
}

auto
globToRegex(const QString& wildcard) -> QRegularExpression
{
    return QRegularExpression(QRegularExpression::wildcardToRegularExpression(
                                wildcard,
                                QRegularExpression::UnanchoredWildcardConversion),
                              QRegularExpression::CaseInsensitiveOption);
}

auto
stripLr2WildcardMarkers(QString wildcard) -> QString
{
    // LR2/beatoraja custom-file patterns sometimes contain selectors such as
    // *|1P|.png. The |...| chunk is metadata for the selector, not part of the
    // filename on disk.
    wildcard.remove(QRegularExpression(QStringLiteral("\\|[^|]*\\|")));
    return wildcard;
}

auto
wildcardSuffixForSelection(const QString& wildcard) -> QString
{
    const auto normalized = stripLr2WildcardMarkers(wildcard);
    const auto star = normalized.lastIndexOf(QLatin1Char('*'));
    return star >= 0 ? normalized.mid(star + 1) : QString{};
}

auto
regularFilePathForSelection(const std::filesystem::path& directory,
                            const QString& selection) -> std::filesystem::path
{
    const auto exact = directory / support::qStringToPath(selection);
    if (std::filesystem::is_regular_file(exact)) {
        return exact;
    }

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto filename = support::pathToQString(entry.path().filename());
        if (filename.compare(selection, Qt::CaseInsensitive) == 0) {
            return entry.path();
        }
    }
    return {};
}

auto
selectedCustomFilePath(const std::filesystem::path& directory,
                       const QString& wildcard,
                       const QString& selected) -> std::filesystem::path
{
    const auto trimmed = selected.trimmed();
    if (trimmed.isEmpty() || trimmed.compare(QStringLiteral("Random"),
                                             Qt::CaseInsensitive) == 0 ||
        trimmed == QLatin1String("-")) {
        return {};
    }

    const auto selectedPath = regularFilePathForSelection(directory, trimmed);
    if (!selectedPath.empty()) {
        return selectedPath;
    }

    const auto suffix = wildcardSuffixForSelection(wildcard);
    if (!suffix.isEmpty() && !trimmed.endsWith(suffix, Qt::CaseInsensitive)) {
        const auto withSuffix =
          regularFilePathForSelection(directory, trimmed + suffix);
        if (!withSuffix.empty()) {
            return withSuffix;
        }
    }

    return {};
}

void
setImageSource(ParseState& state, QString source)
{
    state.images.append(std::move(source));
}

auto
customOptionChoiceMatches(const QString& choice, const QString& selected)
  -> bool
{
    const auto trimmedChoice = choice.trimmed();
    const auto trimmedSelected = selected.trimmed();
    if (trimmedChoice.compare(trimmedSelected, Qt::CaseInsensitive) == 0) {
        return true;
    }

    auto parenMatch =
      QRegularExpression(QStringLiteral("\\(([^)]*)\\)")).globalMatch(
        trimmedChoice);
    while (parenMatch.hasNext()) {
        if (parenMatch.next()
              .captured(1)
              .trimmed()
              .compare(trimmedSelected, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

auto
resolveWildcardPath(const std::filesystem::path& absolutePattern,
                    const ParseState& state) -> QString
{
    const auto directory = absolutePattern.parent_path();
    const auto rawWildcard = support::pathToQString(absolutePattern.filename());
    const auto wildcard = stripLr2WildcardMarkers(rawWildcard);
    const auto normalizedDirectory =
      std::filesystem::absolute(directory).lexically_normal();

    for (const auto& customFile : state.customFiles) {
        const auto customWildcard =
          stripLr2WildcardMarkers(customFile.wildcard);
        if (customFile.directory.lexically_normal() == normalizedDirectory &&
            customWildcard.compare(wildcard, Qt::CaseInsensitive) == 0) {
            auto selected =
              state.settingValues.value(customFile.settingId).toString();
            if (selected.trimmed().isEmpty()) {
                selected = customFile.defaultSelection;
            }

            const auto selectedPath =
              selectedCustomFilePath(directory, rawWildcard, selected);
            if (!selectedPath.empty()) {
                return support::pathToQString(
                  std::filesystem::absolute(selectedPath));
            }
            break;
        }
    }

    std::vector<std::filesystem::path> matches;
    std::error_code ec;
    if (!std::filesystem::exists(directory, ec)) {
        return {};
    }
    const auto regex = globToRegex(wildcard);
    for (const auto& entry :
         std::filesystem::directory_iterator(directory, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto filename = support::pathToQString(entry.path().filename());
        if (regex.match(filename).hasMatch()) {
            matches.push_back(entry.path());
        }
    }
    std::ranges::sort(matches);
    return matches.empty() ? QString{}
                           : support::pathToQString(
                               std::filesystem::absolute(matches.front()));
}

auto
selectedCustomFileSource(const ParseState& state, const QString& settingId)
  -> QString
{
    for (const auto& customFile : state.customFiles) {
        if (customFile.settingId.compare(settingId, Qt::CaseInsensitive) != 0) {
            continue;
        }
        return resolveWildcardPath(customFile.directory /
                                     support::qStringToPath(customFile.wildcard),
                                   state);
    }
    return {};
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
    const auto probe =
      pathText.contains('*') ? path.parent_path() : path;
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
        support::pathToQString(*it)
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
resolveRawPath(const std::filesystem::path& currentDir, const QString& token)
  -> std::filesystem::path
{
    const auto trimmed = token.trimmed();
    if (trimmed.isEmpty() ||
        trimmed.compare("CONTINUE", Qt::CaseInsensitive) == 0) {
        return {};
    }

    auto lr2filesPath = trimmed;
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

        // LR2's "LR2files\Theme" maps to RhythmGame's "themes" folder.
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

    auto path = support::qStringToPath(trimmed);
    if (path.is_relative()) {
        path = currentDir / path;
    }

    return std::filesystem::absolute(path).lexically_normal();
}

auto
resolvePath(const std::filesystem::path& currentDir,
            const QString& token,
            const ParseState& state) -> QString
{
    const auto abs = resolveRawPath(currentDir, token);
    if (abs.empty()) {
        return {};
    }

    const auto normalizedString = support::pathToQString(abs);
    if (normalizedString.contains('*')) {
        return resolveWildcardPath(abs, state);
    }
    return support::pathToQString(abs);
}

auto
evaluateCondition(const QStringList& tokens, const ParseState& state) -> bool
{
    for (int i = 1; i < tokens.size(); ++i) {
        const auto token = tokens[i].trimmed();
        if (token.isEmpty()) {
            continue;
        }

        bool negate = false;
        auto optionToken = token;
        if (optionToken.startsWith('!')) {
            negate = true;
            optionToken.remove(0, 1);
        }
        bool ok = false;
        const int option = parseLr2IntegerPrefix(optionToken, &ok);
        if (!ok) {
            return false;
        }

        const bool contains = state.activeOptions.contains(option);
        if ((!negate && !contains) || (negate && contains)) {
            return false;
        }
    }
    return true;
}

void
skipConditionalBranch(const std::vector<QStringList>& lines, int& index)
{
    int depth = 0;
    while (index < static_cast<int>(lines.size())) {
        const auto& tokens = lines[index];
        if (tokens.isEmpty()) {
            ++index;
            continue;
        }

        const auto command = normalizeCommand(tokens[0]);
        if (command == "#IF") {
            ++depth;
        } else if (command == "#ENDIF") {
            if (depth == 0) {
                return;
            }
            --depth;
        } else if ((command == "#ELSE" || command == "#ELSEIF") && depth == 0) {
            return;
        }
        ++index;
    }
}

void
parseLines(const std::vector<QStringList>& lines,
           int& index,
           ParseState& state,
           const std::filesystem::path& currentDir,
           bool stopAtConditionalBoundary);

void
parseFileIntoState(const std::filesystem::path& filePath, ParseState& state);

void
setSkinResolution(ParseState& state, const int resolution)
{
    struct SkinResolution
    {
        int width;
        int height;
    };
    static constexpr SkinResolution resolutions[] = {
        { 640, 480 },
        { 1280, 720 },
        { 1920, 1080 },
        { 3840, 2160 },
    };

    if (resolution < 0 ||
        resolution >= static_cast<int>(
          sizeof(resolutions) / sizeof(resolutions[0]))) {
        return;
    }

    state.skinWidth = resolutions[resolution].width;
    state.skinHeight = resolutions[resolution].height;
    state.hasSkinResolution = true;
}

struct SkinCanvasCandidate
{
    int width = 0;
    int height = 0;
    int count = 0;
};

auto
canvasCandidateAspectAllowed(const int width, const int height) -> bool
{
    const double aspect =
      static_cast<double>(width) / static_cast<double>(height);
    return aspect >= 1.0 && aspect <= 2.1;
}

void
considerSkinCanvasDst(const Lr2Dst& dst,
                      std::vector<SkinCanvasCandidate>& candidates)
{
    const int width = dst.w < 0 ? -dst.w : dst.w;
    const int height = dst.h < 0 ? -dst.h : dst.h;
    if (dst.x != 0 || dst.y != 0 || width < 320 || height < 240 ||
        !canvasCandidateAspectAllowed(width, height)) {
        return;
    }

    for (auto& candidate : candidates) {
        if (candidate.width == width && candidate.height == height) {
            ++candidate.count;
            return;
        }
    }

    candidates.push_back(SkinCanvasCandidate{
      .width = width,
      .height = height,
      .count = 1,
    });
}

auto
inferSkinCanvas(const QList<Lr2Element>& elements) -> std::pair<int, int>
{
    static constexpr int fallbackWidth = 640;
    static constexpr int fallbackHeight = 480;
    static constexpr int fallbackArea = fallbackWidth * fallbackHeight;
    static constexpr int repeatedSmallCanvasThreshold = 3;

    std::vector<SkinCanvasCandidate> candidates;
    for (const auto& element : elements) {
        for (const auto& dstValue : element.dsts) {
            if (dstValue.canConvert<Lr2Dst>()) {
                considerSkinCanvasDst(dstValue.value<Lr2Dst>(), candidates);
            }
        }
    }

    const SkinCanvasCandidate* best = nullptr;
    for (const auto& candidate : candidates) {
        const int area = candidate.width * candidate.height;
        const bool safeSmallCanvas =
          area < fallbackArea &&
          candidate.count >= repeatedSmallCanvasThreshold;
        if (area < fallbackArea && !safeSmallCanvas) {
            continue;
        }
        if (!best || area > best->width * best->height) {
            best = &candidate;
        }
    }

    if (!best) {
        return { fallbackWidth, fallbackHeight };
    }
    return { best->width, best->height };
}

void
parseIfBlock(const std::vector<QStringList>& lines,
             int& index,
             ParseState& state,
             const std::filesystem::path& currentDir)
{
    bool branchMatched = false;

    while (index < static_cast<int>(lines.size())) {
        const auto tokens = lines[index];
        const auto command = normalizeCommand(tokens[0]);

        bool executeBranch = false;
        if (command == "#IF" || command == "#ELSEIF") {
            executeBranch = !branchMatched && evaluateCondition(tokens, state);
        } else if (command == "#ELSE") {
            executeBranch = !branchMatched;
        } else {
            return;
        }

        ++index;
        ++state.sortId;
        if (executeBranch) {
            parseLines(lines, index, state, currentDir, true);
            branchMatched = true;
        } else {
            skipConditionalBranch(lines, index);
        }

        if (index >= static_cast<int>(lines.size())) {
            return;
        }

        const auto nextCommand =
          lines[index].isEmpty() ? QString{} : normalizeCommand(lines[index][0]);
        if (nextCommand == "#ENDIF") {
            ++index;
            ++state.sortId;
            return;
        }
        if (nextCommand != "#ELSE" && nextCommand != "#ELSEIF") {
            return;
        }
    }
}

auto
sourceForGr(const int gr,
            const int w,
            const int h,
            const ParseState& state) -> std::pair<int, QString>
{
    if (gr == 100) {
        return { Lr2SrcImage::StageFile, {} };
    }
    if (gr == 101) {
        return { Lr2SrcImage::BackBmp, {} };
    }
    if (gr == 102) {
        return { Lr2SrcImage::Banner, {} };
    }
    if (gr == 110) {
        return { Lr2SrcImage::SolidBlack, {} };
    }
    if (gr == 111) {
        return { Lr2SrcImage::SolidWhite, {} };
    }
    if (gr >= 0 && gr < state.images.size()) {
        const auto source = state.images[gr];
        if (source.isEmpty() && w >= 0 && h >= 0 && w <= 1 && h <= 1) {
            return { Lr2SrcImage::SolidBlack, {} };
        }
        return { Lr2SrcImage::None, source };
    }
    return { Lr2SrcImage::None, {} };
}

auto
parseImageSource(const QStringList& tokens,
                 const ParseState& state,
                 const int grIndex = 2) -> Lr2SrcImage
{
    Lr2SrcImage src;
    if (tokens.size() > grIndex && !tokens[grIndex].isEmpty())
        src.gr = tokens[grIndex].toInt();
    if (tokens.size() > grIndex + 1 && !tokens[grIndex + 1].isEmpty())
        src.x = tokens[grIndex + 1].toInt();
    if (tokens.size() > grIndex + 2 && !tokens[grIndex + 2].isEmpty())
        src.y = tokens[grIndex + 2].toInt();
    if (tokens.size() > grIndex + 3 && !tokens[grIndex + 3].isEmpty())
        src.w = tokens[grIndex + 3].toInt();
    if (tokens.size() > grIndex + 4 && !tokens[grIndex + 4].isEmpty())
        src.h = tokens[grIndex + 4].toInt();
    if (tokens.size() > grIndex + 5 && !tokens[grIndex + 5].isEmpty())
        src.div_x = (std::max)(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = (std::max)(1, tokens[grIndex + 6].toInt());
    if (tokens.size() > grIndex + 7 && !tokens[grIndex + 7].isEmpty())
        src.cycle = tokens[grIndex + 7].toInt();
    if (tokens.size() > grIndex + 8 && !tokens[grIndex + 8].isEmpty())
        src.timer = tokens[grIndex + 8].toInt();
    if (tokens.size() > grIndex + 9 && !tokens[grIndex + 9].isEmpty())
        src.op1 = tokens[grIndex + 9].toInt();
    if (tokens.size() > grIndex + 10 && !tokens[grIndex + 10].isEmpty())
        src.op2 = tokens[grIndex + 10].toInt();
    if (tokens.size() > grIndex + 11 && !tokens[grIndex + 11].isEmpty())
        src.op3 = tokens[grIndex + 11].toInt();
    if (tokens.size() > grIndex + 12 && !tokens[grIndex + 12].isEmpty())
        src.op4 = tokens[grIndex + 12].toInt();

    const auto [specialType, source] =
      sourceForGr(src.gr, src.w, src.h, state);
    src.specialType = specialType;
    src.source = source;
    return src;
}

auto
parseHiddenSource(const QStringList& tokens,
                  const ParseState& state,
                  const bool liftCover) -> Lr2SrcImage
{
    auto src = parseImageSource(tokens, state);
    src.hiddenCover = true;
    src.liftCover = liftCover;
    src.hiddenDisappearLine = src.op1 > 0 ? src.op1 : -1;
    src.hiddenDisappearLineLinkLift =
      tokens.size() <= 12 || tokens[12].trimmed().isEmpty() ||
      parseDstInteger(tokens[12]) != 0;
    src.debugLabel =
      liftCover ? QStringLiteral("SRC_LIFT") : QStringLiteral("SRC_HIDDEN");
    return src;
}

auto
parseBgaSource(const QStringList& tokens) -> Lr2SrcBga
{
    Lr2SrcBga src;
    if (tokens.size() > 11 && !tokens[11].isEmpty())
        src.noBase = tokens[11].toInt();
    if (tokens.size() > 12 && !tokens[12].isEmpty())
        src.noLayer = tokens[12].toInt();
    if (tokens.size() > 13 && !tokens[13].isEmpty())
        src.noPoor = tokens[13].toInt();
    return src;
}

auto
parseImageSetSource(const QStringList& tokens,
                    const ParseState& state) -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.imageSet = true;
    // LR2/beatoraja #SRC_IMAGESET order is cycle, timer, ref, count, ...
    if (tokens.size() > 1 && !tokens[1].isEmpty()) {
        src.cycle = tokens[1].toInt();
    }
    if (tokens.size() > 2 && !tokens[2].isEmpty()) {
        src.timer = tokens[2].toInt();
    }
    if (tokens.size() > 3 && !tokens[3].isEmpty()) {
        src.imageSetRef = tokens[3].toInt();
    }
    const int sourceCount =
      tokens.size() > 4 && !tokens[4].isEmpty() ? tokens[4].toInt() : 0;
    bool copiedFallbackSource = false;
    for (int i = 0; i < sourceCount && 5 + i < tokens.size(); ++i) {
        if (tokens[5 + i].isEmpty()) {
            continue;
        }
        const int imageSetIndex = tokens[5 + i].toInt();
        if (imageSetIndex >= 0 && imageSetIndex < state.imageSets.size()) {
            const auto source = state.imageSets[imageSetIndex];
            src.imageSetSources.append(QVariant::fromValue(source));
            if (!copiedFallbackSource) {
                src.gr = source.gr;
                src.x = source.x;
                src.y = source.y;
                src.w = source.w;
                src.h = source.h;
                src.div_x = source.div_x;
                src.div_y = source.div_y;
                src.specialType = source.specialType;
                src.source = source.source;
                copiedFallbackSource = true;
            }
        }
    }
    return src;
}

auto
parseNumberSource(const QStringList& tokens,
                  const ParseState& state,
                  const int grIndex = 2) -> Lr2SrcNumber
{
    Lr2SrcNumber src;
    if (tokens.size() > grIndex && !tokens[grIndex].isEmpty())
        src.gr = tokens[grIndex].toInt();
    if (tokens.size() > grIndex + 1 && !tokens[grIndex + 1].isEmpty())
        src.x = tokens[grIndex + 1].toInt();
    if (tokens.size() > grIndex + 2 && !tokens[grIndex + 2].isEmpty())
        src.y = tokens[grIndex + 2].toInt();
    if (tokens.size() > grIndex + 3 && !tokens[grIndex + 3].isEmpty())
        src.w = tokens[grIndex + 3].toInt();
    if (tokens.size() > grIndex + 4 && !tokens[grIndex + 4].isEmpty())
        src.h = tokens[grIndex + 4].toInt();
    if (tokens.size() > grIndex + 5 && !tokens[grIndex + 5].isEmpty())
        src.div_x = (std::max)(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = (std::max)(1, tokens[grIndex + 6].toInt());
    if (tokens.size() > grIndex + 7 && !tokens[grIndex + 7].isEmpty())
        src.cycle = tokens[grIndex + 7].toInt();
    if (tokens.size() > grIndex + 8 && !tokens[grIndex + 8].isEmpty())
        src.timer = tokens[grIndex + 8].toInt();
    if (tokens.size() > grIndex + 9 && !tokens[grIndex + 9].isEmpty())
        src.num = tokens[grIndex + 9].toInt();
    if (tokens.size() > grIndex + 10 && !tokens[grIndex + 10].isEmpty())
        src.align = tokens[grIndex + 10].toInt();
    if (tokens.size() > grIndex + 11 && !tokens[grIndex + 11].isEmpty())
        src.keta = tokens[grIndex + 11].toInt();
    if (tokens.size() > grIndex + 12 && !tokens[grIndex + 12].isEmpty())
        src.zeropadding = tokens[grIndex + 12].toInt();

    const auto [specialType, source] =
      sourceForGr(src.gr, src.w, src.h, state);
    if (specialType == Lr2SrcImage::None) {
        src.source = source;
    }
    return src;
}

auto
parseNowComboSource(const QStringList& tokens,
                    const ParseState& state,
                    const int side) -> Lr2SrcNumber
{
    auto src = parseNumberSource(tokens, state);
    src.num = side == 2 ? 124 : 104;
    src.nowCombo = true;
    src.side = side;
    src.judgementIndex =
      tokens.size() > 1 && !tokens[1].isEmpty() ? tokens[1].toInt() : -1;
    return src;
}

auto
parseResultChartSource(const QStringList& tokens,
                       const ParseState& state,
                       const int chartType,
                       const int side) -> Lr2SrcImage
{
    auto src = parseImageSource(tokens, state);
    src.resultChartType = chartType;
    src.side = side;
    if (tokens.size() > 1 && !tokens[1].isEmpty()) {
        src.resultChartIndex = tokens[1].toInt();
    }
    return src;
}

auto
parseBarGraphSource(const QStringList& tokens,
                    const ParseState& state,
                    const int grIndex = 2) -> Lr2SrcBarGraph
{
    Lr2SrcBarGraph src;
    if (tokens.size() > grIndex && !tokens[grIndex].isEmpty())
        src.gr = tokens[grIndex].toInt();
    if (tokens.size() > grIndex + 1 && !tokens[grIndex + 1].isEmpty())
        src.x = tokens[grIndex + 1].toInt();
    if (tokens.size() > grIndex + 2 && !tokens[grIndex + 2].isEmpty())
        src.y = tokens[grIndex + 2].toInt();
    if (tokens.size() > grIndex + 3 && !tokens[grIndex + 3].isEmpty())
        src.w = tokens[grIndex + 3].toInt();
    if (tokens.size() > grIndex + 4 && !tokens[grIndex + 4].isEmpty())
        src.h = tokens[grIndex + 4].toInt();
    if (tokens.size() > grIndex + 5 && !tokens[grIndex + 5].isEmpty())
        src.div_x = (std::max)(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = (std::max)(1, tokens[grIndex + 6].toInt());
    if (tokens.size() > grIndex + 7 && !tokens[grIndex + 7].isEmpty())
        src.cycle = tokens[grIndex + 7].toInt();
    if (tokens.size() > grIndex + 8 && !tokens[grIndex + 8].isEmpty())
        src.timer = tokens[grIndex + 8].toInt();
    if (tokens.size() > grIndex + 9 && !tokens[grIndex + 9].isEmpty())
        src.graphType = tokens[grIndex + 9].toInt();
    if (tokens.size() > grIndex + 10 && !tokens[grIndex + 10].isEmpty())
        src.direction = tokens[grIndex + 10].toInt();

    const auto [specialType, source] =
      sourceForGr(src.gr, src.w, src.h, state);
    src.specialType = specialType;
    src.source = source;
    return src;
}

auto
parseNoteChartSource(const QStringList& tokens) -> Lr2SrcNoteChart
{
    Lr2SrcNoteChart src;
    if (tokens.size() > 1 && !tokens[1].isEmpty())
        src.chartType = tokens[1].toInt();
    if (tokens.size() > 11 && !tokens[11].isEmpty())
        src.fieldW = (std::max)(1, tokens[11].toInt());
    if (tokens.size() > 12 && !tokens[12].isEmpty())
        src.fieldH = (std::max)(1, tokens[12].toInt());
    if (tokens.size() > 13 && !tokens[13].isEmpty())
        src.start = tokens[13].toInt();
    if (tokens.size() > 14 && !tokens[14].isEmpty())
        src.end = tokens[14].toInt();
    if (tokens.size() > 15 && !tokens[15].isEmpty())
        src.delay = tokens[15].toInt();
    if (tokens.size() > 16 && !tokens[16].isEmpty())
        src.backTexOff = tokens[16].toInt();
    if (tokens.size() > 17 && !tokens[17].isEmpty())
        src.orderReverse = tokens[17].toInt();
    if (tokens.size() > 18 && !tokens[18].isEmpty())
        src.noGap = tokens[18].toInt();
    if (tokens.size() > 19 && !tokens[19].isEmpty())
        src.noGapX = tokens[19].toInt();
    return src;
}

auto
commandPlayerSide(const QString& command) -> int
{
    return command.endsWith(QStringLiteral("_2P")) ? 2 : 1;
}

auto
parseChartColor(const QStringList& tokens,
                const int index,
                const QString& fallback) -> QString
{
    if (tokens.size() <= index || tokens[index].isEmpty()) {
        return fallback;
    }
    return tokens[index].trimmed();
}

auto
parseBpmChartSource(const QStringList& tokens) -> Lr2SrcBpmChart
{
    Lr2SrcBpmChart src;
    if (tokens.size() > 1 && !tokens[1].isEmpty())
        src.fieldW = (std::max)(1, tokens[1].toInt());
    if (tokens.size() > 2 && !tokens[2].isEmpty())
        src.fieldH = (std::max)(1, tokens[2].toInt());
    if (tokens.size() > 3 && !tokens[3].isEmpty())
        src.delay = tokens[3].toInt();
    if (tokens.size() > 4 && !tokens[4].isEmpty())
        src.lineWidth = (std::max)(1, tokens[4].toInt());
    src.mainBpmColor =
      parseChartColor(tokens, 5, QStringLiteral("00ff00"));
    src.minBpmColor = parseChartColor(tokens, 6, QStringLiteral("0000ff"));
    src.maxBpmColor = parseChartColor(tokens, 7, QStringLiteral("ff0000"));
    src.otherBpmColor =
      parseChartColor(tokens, 8, QStringLiteral("ffff00"));
    src.stopLineColor =
      parseChartColor(tokens, 9, QStringLiteral("ff00ff"));
    src.transitionLineColor =
      parseChartColor(tokens, 10, QStringLiteral("7f7f7f"));
    return src;
}

auto
parseTimingChartSource(const QStringList& tokens) -> Lr2SrcTimingChart
{
    Lr2SrcTimingChart src;
    if (tokens.size() > 4 && !tokens[4].isEmpty())
        src.fieldW = (std::max)(1, tokens[4].toInt());
    if (tokens.size() > 5 && !tokens[5].isEmpty())
        src.fieldH = (std::max)(1, tokens[5].toInt());
    if (tokens.size() > 6 && !tokens[6].isEmpty())
        src.lineWidth = (std::max)(1, tokens[6].toInt());
    src.graphColor = parseChartColor(tokens, 7, QStringLiteral("ffffff"));
    src.averageColor = parseChartColor(tokens, 8, QStringLiteral("ff0000"));
    src.devColor = parseChartColor(tokens, 9, QStringLiteral("0000ff"));
    src.pgColor = parseChartColor(tokens, 10, QStringLiteral("0088ff"));
    src.grColor = parseChartColor(tokens, 11, QStringLiteral("00ff88"));
    src.gdColor = parseChartColor(tokens, 12, QStringLiteral("ffff00"));
    src.bdColor = parseChartColor(tokens, 13, QStringLiteral("ff8800"));
    src.prColor = parseChartColor(tokens, 14, QStringLiteral("ff0000"));
    if (tokens.size() > 15 && !tokens[15].isEmpty())
        src.drawAverage = tokens[15].toInt();
    if (tokens.size() > 16 && !tokens[16].isEmpty())
        src.drawDev = tokens[16].toInt();
    return src;
}

auto
parseTimingVisualizerSource(const QStringList& tokens)
  -> Lr2SrcTimingVisualizer
{
    Lr2SrcTimingVisualizer src;
    if (tokens.size() > 4 && !tokens[4].isEmpty())
        src.fieldW = (std::max)(1, tokens[4].toInt());
    if (tokens.size() > 5 && !tokens[5].isEmpty())
        src.fieldH = (std::max)(1, tokens[5].toInt());
    if (tokens.size() > 6 && !tokens[6].isEmpty())
        src.judgeWidthMillis = (std::max)(1, tokens[6].toInt());
    if (tokens.size() > 7 && !tokens[7].isEmpty())
        src.lineWidth = std::clamp(tokens[7].toInt(), 1, 4);
    src.lineColor = parseChartColor(tokens, 8, QStringLiteral("00ff00ff"));
    src.centerColor =
      parseChartColor(tokens, 9, QStringLiteral("ffffffff"));
    src.pgColor = parseChartColor(tokens, 10, QStringLiteral("0088ffcc"));
    src.grColor = parseChartColor(tokens, 11, QStringLiteral("00ff88cc"));
    src.gdColor = parseChartColor(tokens, 12, QStringLiteral("ffff00cc"));
    src.bdColor = parseChartColor(tokens, 13, QStringLiteral("ff8800cc"));
    src.prColor = parseChartColor(tokens, 14, QStringLiteral("ff0000cc"));
    if (tokens.size() > 15 && !tokens[15].isEmpty())
        src.transparent = tokens[15].toInt();
    if (tokens.size() > 16 && !tokens[16].isEmpty())
        src.drawDecay = tokens[16].toInt();
    return src;
}

void
applyFont(Lr2SrcText& src, const ParseState& state)
{
    if (src.font >= 0 && src.font < state.systemFonts.size()) {
        const auto& font = state.systemFonts[src.font];
        src.fontPath = font.path;
        src.fontFamily = font.family;
        src.fontSize = font.size;
        src.fontThickness = font.thickness;
        src.fontType = font.type;
    }
    if (src.font >= 0 && src.font < state.imageFonts.size()) {
        const auto& font = state.imageFonts[src.font];
        if (!font.path.isEmpty()) {
            src.fontPath = font.path;
            src.bitmapFont = true;
        }
    }
}

auto
parseTextSource(const QStringList& tokens,
                const ParseState& state,
                const int fontIndex = 2) -> Lr2SrcText
{
    Lr2SrcText src;
    if (tokens.size() > fontIndex && !tokens[fontIndex].isEmpty())
        src.font = tokens[fontIndex].toInt();
    if (tokens.size() > fontIndex + 1 && !tokens[fontIndex + 1].isEmpty())
        src.st = tokens[fontIndex + 1].toInt();
    if (tokens.size() > fontIndex + 2 && !tokens[fontIndex + 2].isEmpty())
        src.align = tokens[fontIndex + 2].toInt();
    if (tokens.size() > fontIndex + 3 && !tokens[fontIndex + 3].isEmpty())
        src.edit = tokens[fontIndex + 3].toInt();
    if (tokens.size() > fontIndex + 4 && !tokens[fontIndex + 4].isEmpty())
        src.panel = tokens[fontIndex + 4].toInt();
    applyFont(src, state);
    return src;
}

auto
toVariantList(const QMap<int, Lr2SrcImage>& sources) -> QVariantList
{
    QVariantList result;
    if (sources.isEmpty()) {
        return result;
    }
    const int maxKey = sources.lastKey();
    for (int i = 0; i <= maxKey; ++i) {
        result.append(QVariant::fromValue(sources.value(i)));
    }
    return result;
}

auto
toVariantList(const QMap<int, QVariantList>& destinations) -> QVariantList
{
    QVariantList result;
    if (destinations.isEmpty()) {
        return result;
    }
    const int maxKey = destinations.lastKey();
    for (int i = 0; i <= maxKey; ++i) {
        result.append(QVariant::fromValue(destinations.value(i)));
    }
    return result;
}

void
ensureNoteElement(ParseState& state)
{
    if (state.hasNoteElement) {
        return;
    }

    flushCurrentElement(state);
    Lr2Element element;
    element.type = 8;
    state.elements.append(element);
    state.hasNoteElement = true;
}

auto
fallbackBarBodySource(const ParseState& state) -> QVariant
{
    // Folder rows are what the select screen shows at root, so prefer index 1
    // as the direct fallback if QML cannot index the full body source list.
    if (state.barBodySources.contains(1)) {
        return QVariant::fromValue(state.barBodySources.value(1));
    }
    if (state.barBodySources.contains(0)) {
        return QVariant::fromValue(state.barBodySources.value(0));
    }
    return {};
}

auto
currentBarImageMatches(const ParseState& state,
                       const int kind,
                       const int row,
                       const int variant) -> bool
{
    if (!state.hasCurrentElement || state.currentElement.type != 3) {
        return false;
    }
    const auto src = state.currentElement.src.value<Lr2SrcBarImage>();
    return src.kind == kind && src.row == row && src.variant == variant;
}

void
appendBarImageDst(ParseState& state,
                  const QStringList& tokens,
                  const int kind,
                  const int row,
                  const int variant,
                  const QVariant& source,
                  const QVariantList& sources)
{
    if (!currentBarImageMatches(state, kind, row, variant)) {
        flushCurrentElement(state);
        Lr2SrcBarImage src;
        src.kind = kind;
        src.row = row;
        src.variant = variant;
        src.source = source;
        src.sources = sources;
        state.currentElement = Lr2Element{};
        state.currentElement.type = 3;
        state.currentElement.src = QVariant::fromValue(src);
        state.hasCurrentElement = true;
    }

    const auto dst = parseDstValue(tokens, state.sortId);
    state.currentElement.dsts.append(QVariant::fromValue(dst));
    if (kind == Lr2SrcBarImage::BodyOff) {
        state.barBodyOffDsts[row].append(QVariant::fromValue(dst));
    } else if (kind == Lr2SrcBarImage::BodyOn) {
        state.barBodyOnDsts[row].append(QVariant::fromValue(dst));
    }
}

auto
currentBarTextMatches(const ParseState& state, const int titleType) -> bool
{
    if (!state.hasCurrentElement || state.currentElement.type != 4) {
        return false;
    }
    const auto src = state.currentElement.src.value<Lr2SrcBarText>();
    return src.titleType == titleType;
}

void
appendBarTextDst(ParseState& state, const QStringList& tokens, const int titleType)
{
    if (!state.barTitleSources.contains(titleType)) {
        return;
    }
    if (!currentBarTextMatches(state, titleType)) {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 4;
        state.currentElement.src =
          QVariant::fromValue(state.barTitleSources.value(titleType));
        state.hasCurrentElement = true;
    }
    parseBarDst(tokens, state, state.currentElement);
}

auto
currentBarNumberMatches(const ParseState& state, const int variant) -> bool
{
    if (!state.hasCurrentElement || state.currentElement.type != 5) {
        return false;
    }
    const auto src = state.currentElement.src.value<Lr2SrcBarNumber>();
    return src.variant == variant;
}

void
appendBarNumberDst(ParseState& state, const QStringList& tokens, const int variant)
{
    if (!state.barLevelSources.contains(variant)) {
        return;
    }
    if (!currentBarNumberMatches(state, variant)) {
        flushCurrentElement(state);
        Lr2SrcBarNumber src;
        src.kind = Lr2SrcBarNumber::Level;
        src.variant = variant;
        src.source = QVariant::fromValue(state.barLevelSources.value(variant));
        state.currentElement = Lr2Element{};
        state.currentElement.type = 5;
        state.currentElement.src = QVariant::fromValue(src);
        state.hasCurrentElement = true;
    }
    parseBarDst(tokens, state, state.currentElement);
}

void
processCommand(const QStringList& tokens,
               ParseState& state,
               const std::filesystem::path& currentDir)
{
    if (tokens.isEmpty()) {
        return;
    }

    const auto command = normalizeCommand(tokens[0]);
    if (command == "#STARTINPUT") {
        if (tokens.size() > 1) {
            state.startInput = tokens[1].trimmed().toInt();
        }
    } else if (command == "#SCENETIME") {
        if (tokens.size() > 1) {
            state.sceneTime = tokens[1].trimmed().toInt();
        }
    } else if (command == "#LOADSTART") {
        if (tokens.size() > 1) {
            state.loadStart = tokens[1].trimmed().toInt();
        }
    } else if (command == "#LOADEND") {
        if (tokens.size() > 1) {
            state.loadEnd = tokens[1].trimmed().toInt();
        }
    } else if (command == "#PLAYSTART") {
        if (tokens.size() > 1) {
            state.playStart = tokens[1].trimmed().toInt();
        }
    } else if (command == "#FADEOUT") {
        if (tokens.size() > 1) {
            state.fadeOut = tokens[1].trimmed().toInt();
        }
    } else if (command == "#SKIP") {
        if (tokens.size() > 1) {
            state.skip = tokens[1].trimmed().toInt();
        }
    } else if (command == "#RESOLUTION") {
        if (tokens.size() > 1) {
            setSkinResolution(state, tokens[1].trimmed().toInt());
        }
    } else if (command == "#CUSTOMOPTION") {
        if (tokens.size() < 4) {
            return;
        }
        const auto settingName = tokens[1].trimmed();
        const auto optionId = tokens[2].trimmed();
        const int settingItemIndex = state.settingsItemIndex++;
        const auto settingId =
          assignSettingsItemId(settingName,
                               "opt_" + optionId,
                               QStringLiteral("choice"),
                               settingItemIndex,
                               state.settingsIdState);
        const auto fallbackSettingId = makeSafeId(
          "opt_" + optionId,
          QStringLiteral("setting_%1").arg(settingItemIndex));
        const int baseOption = tokens[2].trimmed().toInt();
        auto selected = state.settingValues.value(settingId).toString();
        if (selected.isEmpty() && fallbackSettingId != settingId) {
            selected = state.settingValues.value(fallbackSettingId).toString();
        }
        if (selected.isEmpty()) {
            selected = tokens[3].trimmed();
        }
        int defaultOffset = -1;

        // A #CUSTOMOPTION is an exclusive choice range: base, base + 1, ...
        // Clear any seeded fallback from the wrapper before applying the skin
        // setting/default so direct .csv loads and .lr2skin loads behave alike.
        int optionOffset = 0;
        for (int i = 3; i < tokens.size(); ++i) {
            if (tokens[i].trimmed().isEmpty()) {
                continue;
            }
            if (defaultOffset < 0) {
                defaultOffset = optionOffset;
            }
            state.activeOptions.erase(baseOption + optionOffset);
            ++optionOffset;
        }

        int selectedOffset = -1;
        optionOffset = 0;
        for (int i = 3; i < tokens.size(); ++i) {
            if (tokens[i].trimmed().isEmpty()) {
                continue;
            }
            if (customOptionChoiceMatches(tokens[i], selected)) {
                selectedOffset = optionOffset;
                break;
            }
            ++optionOffset;
        }
        if (selectedOffset < 0) {
            selectedOffset = defaultOffset;
        }
        if (selectedOffset >= 0) {
            state.activeOptions.insert(baseOption + selectedOffset);
        }
    } else if (command == "#CUSTOMFILE") {
        if (tokens.size() < 3) {
            return;
        }
        const int settingItemIndex = state.settingsItemIndex++;
        const auto patternPath = resolveRawPath(currentDir, tokens[2].trimmed());
        state.customFiles.append(CustomFile{
          .settingId = assignSettingsItemId(tokens[1].trimmed(),
                                            "file_" +
                                              QString::number(settingItemIndex),
                                            QStringLiteral("file"),
                                            settingItemIndex,
                                            state.settingsIdState),
          .directory = std::filesystem::absolute(patternPath.parent_path())
                         .lexically_normal(),
          .wildcard = support::pathToQString(patternPath.filename()),
          .defaultSelection =
            tokens.size() > 3 ? tokens[3].trimmed() : QString{},
        });
    } else if (command == "#SETOPTION") {
        if (tokens.size() < 3) {
            return;
        }
        const int option = tokens[1].trimmed().toInt();
        const bool enabled = tokens[2].trimmed().toInt() != 0;
        if (enabled) {
            state.activeOptions.insert(option);
        } else {
            state.activeOptions.erase(option);
        }
    } else if (command == "#DISABLEFLIP") {
        state.activeOptions.erase(351);
        state.activeOptions.insert(350);
    } else if (command == "#FLIPRESULT") {
        state.activeOptions.erase(350);
        state.activeOptions.insert(351);
    } else if (command == "#INCLUDE") {
        if (tokens.size() < 2 || tokens[1].trimmed().isEmpty()) {
            return;
        }
        const auto includePath =
          resolvePath(currentDir, tokens[1].trimmed(), state);
        if (!includePath.isEmpty()) {
            parseFileIntoState(support::qStringToPath(includePath), state);
        }
    } else if (command == "#HELPFILE") {
        if (tokens.size() < 2 || tokens[1].trimmed().isEmpty()) {
            return;
        }
        const auto helpPath = resolvePath(currentDir, tokens[1].trimmed(), state);
        if (!helpPath.isEmpty()) {
            state.helpFiles.append(helpPath);
        }
    } else if (command == "#RELOADBANNER") {
        state.reloadBanner = true;
    } else if (command == "#TRANSCOLOR" || command == "#TRANSCLOLR" ||
               command == "#TRANSCLOLOR") {
        if (tokens.size() > 3) {
            state.transColor = colorKeyString(parseLr2IntegerPrefix(tokens[1]),
                                              parseLr2IntegerPrefix(tokens[2]),
                                              parseLr2IntegerPrefix(tokens[3]));
            state.hasTransColor = true;
        }
    } else if (command == "#IMAGE") {
        setImageSource(state,
                       resolvePath(currentDir,
                                   tokens.size() > 1 ? tokens[1].trimmed()
                                                     : QString{},
                                   state));
    } else if (command == "#IMAGESET") {
        if (tokens.size() > 2 && !tokens[2].trimmed().isEmpty()) {
            state.imageSets.append(parseImageSource(tokens, state));
        }
    } else if (command == "#FONT") {
        FontDefinition font;
        if (tokens.size() > 1 && !tokens[1].trimmed().isEmpty()) {
            font.size = tokens[1].trimmed().toInt();
        }
        if (tokens.size() > 2 && !tokens[2].trimmed().isEmpty()) {
            font.thickness = tokens[2].trimmed().toInt();
        }
        if (tokens.size() > 3 && !tokens[3].trimmed().isEmpty()) {
            font.type = tokens[3].trimmed().toInt();
        }
        font.family = lr2ConfiguredFontFamily();
        font.path = font.family;
        state.systemFonts.append(font);
    } else if (command == "#LR2FONT") {
        FontDefinition font;
        font.bitmap = true;
        font.path =
          resolvePath(currentDir,
                      tokens.size() > 1 ? tokens[1].trimmed() : QString{},
                      state);
        state.imageFonts.append(font);
    } else if (command == "#SRC_IMAGE" || command == "#SRC_JUDGELINE" ||
               command == "#SRC_HIDDEN" || command == "#SRC_LIFT") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        auto src = command == "#SRC_HIDDEN" || command == "#SRC_LIFT"
          ? parseHiddenSource(tokens, state, command == "#SRC_LIFT")
          : parseImageSource(tokens, state);
        if (command == "#SRC_JUDGELINE") {
            src.debugLabel = QStringLiteral("SRC_JUDGELINE");
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#SRC_IMAGESET") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseImageSetSource(tokens, state));
    } else if (command == "#DST_IMAGE" || command == "#DST_JUDGELINE" ||
               command == "#DST_HIDDEN" || command == "#DST_LIFT") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            if (command == "#DST_JUDGELINE" || command == "#DST_HIDDEN" ||
                command == "#DST_LIFT") {
                const auto dst = parseDstValue(tokens, state.sortId);
                auto adjustedDst = dst;
                if (command == "#DST_JUDGELINE" || command == "#DST_HIDDEN" ||
                    command == "#DST_LIFT") {
                    ensureDstOffset(adjustedDst, 3);
                }
                if (command == "#DST_HIDDEN") {
                    ensureDstOffset(adjustedDst, 5);
                }
                recordDstOptions(state, adjustedDst, true);
                state.currentElement.dsts.append(QVariant::fromValue(adjustedDst));
            } else {
                parseDst(tokens, state, state.currentElement);
            }
        }
    } else if (command == "#SRC_LINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lineSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_LINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            auto dst = parseDstValue(tokens, state.sortId);
            ensureDstOffset(dst, 3);
            state.lineDsts[tokens[1].toInt()].append(QVariant::fromValue(dst));
        }
    } else if (command == "#SRC_BGA") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 7;
        state.hasCurrentElement = true;

        state.currentElement.src = QVariant::fromValue(parseBgaSource(tokens));
    } else if (command == "#DST_BGA") {
        if (state.hasCurrentElement && state.currentElement.type == 7) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_ONMOUSE") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;
        auto src = parseImageSource(tokens, state);
        src.onMouse = true;
        src.hoverPanel = src.op1;
        src.hoverX = src.op2;
        src.hoverY = src.op3;
        src.hoverW = src.op4;
        if (tokens.size() > 15 && !tokens[15].isEmpty()) {
            src.hoverH = tokens[15].toInt();
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_ONMOUSE") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_MOUSECURSOR") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;
        auto src = parseImageSource(tokens, state);
        src.mouseCursor = true;
        auto source = src.source;
        source.replace('\\', '/');
        if (!source.contains("/Mouse/", Qt::CaseInsensitive)) {
            for (const auto& image : state.images) {
                auto normalized = image;
                normalized.replace('\\', '/');
                if (normalized.contains("/Mouse/", Qt::CaseInsensitive)) {
                    src.source = image;
                    src.specialType = Lr2SrcImage::None;
                    break;
                }
            }
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_MOUSECURSOR") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_BUTTON") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;
        auto src = parseImageSource(tokens, state);
        src.button = true;
        src.buttonId = src.op1;
        src.buttonClick = src.op2;
        src.buttonPanel = src.op3;
        src.buttonPlusOnly = src.op4;
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#SRC_SLIDER"
               || command == "#SRC_SLIDER_REFNUMBER") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;
        auto src = parseImageSource(tokens, state);
        src.slider = true;
        src.sliderDirection = src.op1;
        src.sliderRange = src.op2;
        src.sliderType = src.op3;
        src.sliderDisabled = src.op4;
        if (command == "#SRC_SLIDER_REFNUMBER") {
            src.sliderRefNumber = true;
            if (tokens.size() > 15 && !tokens[15].isEmpty()) {
                src.sliderMinValue = tokens[15].toInt();
            }
            if (tokens.size() > 16 && !tokens[16].isEmpty()) {
                src.sliderMaxValue = tokens[16].toInt();
            }
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_BUTTON" || command == "#DST_SLIDER") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_NUMBER") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 1;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseNumberSource(tokens, state));
    } else if (command == "#DST_NUMBER") {
        if (state.hasCurrentElement && state.currentElement.type == 1) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_NOWJUDGE_1P" ||
               command == "#SRC_NOWJUDGE_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        auto src = parseImageSource(tokens, state);
        src.timer = lr2NowDisplayTimer(command);
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_NOWJUDGE_1P" ||
               command == "#DST_NOWJUDGE_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            auto dst = parseDstValue(tokens, state.sortId);
            const int side = lr2NowSide(command);
            const int index = lr2NowCommandIndex(tokens);
            state.nowJudgeDsts[lr2NowStateKey(side, index)].append(dst);
            dst.timer = lr2NowDisplayTimer(command);
            addDstOptionGate(dst, lr2NowJudgementOption(command, index));
            recordDstOptions(state, dst, true);
            state.currentElement.dsts.append(QVariant::fromValue(dst));
        }
    } else if (command == "#SRC_NOWCOMBO_1P" ||
               command == "#SRC_NOWCOMBO_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 1;
        state.hasCurrentElement = true;

        auto src = parseNowComboSource(
          tokens, state, command.endsWith(QStringLiteral("_2P")) ? 2 : 1);
        src.timer = lr2NowDisplayTimer(command);
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_NOWCOMBO_1P" ||
               command == "#DST_NOWCOMBO_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 1) {
            auto dst = parseDstValue(tokens, state.sortId);
            const int side = lr2NowSide(command);
            const int index = lr2NowCommandIndex(tokens);
            const int key = lr2NowStateKey(side, index);
            const int dstIndex = state.nowComboDstCounts.value(key, 0);
            state.nowComboDstCounts[key] = dstIndex + 1;
            const auto judgeDsts = state.nowJudgeDsts.value(key);
            if (!judgeDsts.isEmpty()) {
                const auto& judgeDst =
                  judgeDsts.at((std::min)(dstIndex,
                                          static_cast<int>(judgeDsts.size() - 1)));
                dst.x += judgeDst.x;
                dst.y = judgeDst.y - dst.y;
            }
            ensureDstOffset(dst, 3);
            dst.timer = lr2NowDisplayTimer(command);
            addDstOptionGate(dst, lr2NowJudgementOption(command, index));
            recordDstOptions(state, dst, true);
            state.currentElement.dsts.append(QVariant::fromValue(dst));
        }
    } else if (command == "#SRC_BARGRAPH") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 6;
        state.hasCurrentElement = true;
        state.currentElement.src =
          QVariant::fromValue(parseBarGraphSource(tokens, state));
    } else if (command == "#DST_BARGRAPH") {
        if (state.hasCurrentElement && state.currentElement.type == 6) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_BAR_GRAPH") {
        flushCurrentElement(state);
        state.barDistributionGraphSource = parseBarGraphSource(tokens, state);
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barDistributionGraphSource.graphType = tokens[1].toInt();
        }
        state.hasBarDistributionGraphSource = true;
        state.currentElement = Lr2Element{};
        state.currentElement.type = 13;
        state.currentElement.src =
          QVariant::fromValue(state.barDistributionGraphSource);
        state.hasCurrentElement = true;
    } else if (command == "#DST_BAR_GRAPH") {
        if (state.hasCurrentElement && state.currentElement.type == 13 &&
            state.hasBarDistributionGraphSource) {
            parseBarDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_GAUGECHART_1P" ||
               command == "#SRC_GAUGECHART_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 10;
        state.hasCurrentElement = true;
        state.currentElement.src = QVariant::fromValue(parseResultChartSource(
          tokens,
          state,
          1,
          command.endsWith(QStringLiteral("_2P")) ? 2 : 1));
    } else if (command == "#DST_GAUGECHART_1P" ||
               command == "#DST_GAUGECHART_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 10) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_LVF_EXPERIMENTAL_GAUGECHART_MYBEST") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 10;
        state.hasCurrentElement = true;
        state.currentElement.src =
          QVariant::fromValue(parseResultChartSource(tokens, state, 1, 3));
    } else if (command == "#DST_LVF_EXPERIMENTAL_GAUGECHART_MYBEST") {
        if (state.hasCurrentElement && state.currentElement.type == 10) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_SCORECHART") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 10;
        state.hasCurrentElement = true;
        state.currentElement.src =
          QVariant::fromValue(parseResultChartSource(tokens, state, 2, 1));
    } else if (command == "#DST_SCORECHART") {
        if (state.hasCurrentElement && state.currentElement.type == 10) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_NOTECHART" ||
               command == "#SRC_NOTECHART_1P" ||
               command == "#SRC_NOTECHART_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 11;
        state.hasCurrentElement = true;
        auto src = parseNoteChartSource(tokens);
        src.playerSide = commandPlayerSide(command);
        state.currentElement.src =
          QVariant::fromValue(src);
    } else if (command == "#DST_NOTECHART" ||
               command == "#DST_NOTECHART_1P" ||
               command == "#DST_NOTECHART_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 11) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_BPMCHART" ||
               command == "#SRC_BPMCHART_1P" ||
               command == "#SRC_BPMCHART_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 12;
        state.hasCurrentElement = true;
        state.currentElement.src =
          QVariant::fromValue(parseBpmChartSource(tokens));
    } else if (command == "#DST_BPMCHART" ||
               command == "#DST_BPMCHART_1P" ||
               command == "#DST_BPMCHART_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 12) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_TIMINGCHART_1P" ||
               command == "#SRC_TIMINGCHART_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 14;
        state.hasCurrentElement = true;
        auto src = parseTimingChartSource(tokens);
        src.playerSide = commandPlayerSide(command);
        state.currentElement.src =
          QVariant::fromValue(src);
    } else if (command == "#DST_TIMINGCHART_1P" ||
               command == "#DST_TIMINGCHART_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 14) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_TIMING_1P" ||
               command == "#SRC_TIMING_2P") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 15;
        state.hasCurrentElement = true;
        auto src = parseTimingVisualizerSource(tokens);
        src.playerSide = commandPlayerSide(command);
        state.currentElement.src =
          QVariant::fromValue(src);
    } else if (command == "#DST_TIMING_1P" ||
               command == "#DST_TIMING_2P") {
        if (state.hasCurrentElement && state.currentElement.type == 15) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_TEXT") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 2;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseTextSource(tokens, state));
    } else if (command == "#DST_TEXT") {
        if (state.hasCurrentElement && state.currentElement.type == 2) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_README") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 2;
        state.hasCurrentElement = true;

        Lr2SrcText src;
        src.readme = true;
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            src.readmeId = tokens[1].toInt();
        }
        if (tokens.size() > 2 && !tokens[2].isEmpty()) {
            src.font = tokens[2].toInt();
        }
        if (tokens.size() > 5 && !tokens[5].isEmpty()) {
            src.readmeLineSpacing = tokens[5].toInt();
        }
        applyFont(src, state);
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_README") {
        if (state.hasCurrentElement && state.currentElement.type == 2) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_NOTE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.noteSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_MINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.mineSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_LN_START") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lnStartSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_LN_END") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lnEndSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_LN_BODY" ||
               command == "#SRC_LN_BODY_INACTIVE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lnBodySources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_LN_BODY_ACTIVE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lnBodyActiveSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_NOTE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoNoteSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_MINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoMineSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_LN_START") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoLnStartSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_LN_END") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoLnEndSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_LN_BODY" ||
               command == "#SRC_AUTO_LN_BODY_INACTIVE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoLnBodySources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#SRC_AUTO_LN_BODY_ACTIVE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.autoLnBodyActiveSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_NOTE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            ensureNoteElement(state);
            state.noteDsts[tokens[1].toInt()].append(
              QVariant::fromValue(parseDstValue(tokens, state.sortId)));
        }
    } else if (command == "#SRC_GROOVEGAUGE" ||
               command == "#SRC_GROOVEGAUGE_EX") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 9;
        state.hasCurrentElement = true;

        auto src = parseImageSource(tokens, state);
        src.grooveGaugeEx = command == "#SRC_GROOVEGAUGE_EX";
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            src.side = tokens[1].toInt() + 1;
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_GROOVEGAUGE") {
        if (state.hasCurrentElement && state.currentElement.type == 9) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#BAR_CENTER") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barCenter = tokens[1].toInt();
        }
    } else if (command == "#BAR_AVAILABLE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barAvailableStart = tokens[1].toInt();
        }
        if (tokens.size() > 2 && !tokens[2].isEmpty()) {
            state.barAvailableEnd = tokens[2].toInt();
        }
    } else if (command == "#SRC_BAR_BODY") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barBodySources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_BODY_OFF") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::BodyOff,
                              tokens[1].toInt(),
                              0,
                              fallbackBarBodySource(state),
                              toVariantList(state.barBodySources));
        }
    } else if (command == "#DST_BAR_BODY_ON") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::BodyOn,
                              tokens[1].toInt(),
                              0,
                              fallbackBarBodySource(state),
                              toVariantList(state.barBodySources));
        }
    } else if (command == "#SRC_BAR_FLASH") {
        state.barFlashSource = parseImageSource(tokens, state);
        state.hasBarFlashSource = true;
    } else if (command == "#DST_BAR_FLASH") {
        if (state.hasBarFlashSource) {
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::Flash,
                              -1,
                              0,
                              QVariant::fromValue(state.barFlashSource),
                              {});
        }
    } else if (command == "#SRC_BAR_LEVEL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barLevelSources[tokens[1].toInt()] =
              parseNumberSource(tokens, state);
        }
    } else if (command == "#DST_BAR_LEVEL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            appendBarNumberDst(state, tokens, tokens[1].toInt());
        }
    } else if (command == "#SRC_BAR_TITLE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            const int titleType = tokens[1].toInt();
            const auto text = parseTextSource(tokens, state);
            Lr2SrcBarText barText;
            barText.titleType = titleType;
            barText.font = text.font;
            barText.st = text.st;
            barText.align = text.align;
            barText.edit = text.edit;
            barText.panel = text.panel;
            barText.fontPath = text.fontPath;
            barText.fontFamily = text.fontFamily;
            barText.fontSize = text.fontSize;
            barText.fontThickness = text.fontThickness;
            barText.fontType = text.fontType;
            barText.bitmapFont = text.bitmapFont;
            state.barTitleSources[titleType] = barText;
        }
    } else if (command == "#DST_BAR_TITLE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            appendBarTextDst(state, tokens, tokens[1].toInt());
        }
    } else if (command == "#SRC_BAR_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barLampSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barLampSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::Lamp,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barLampSources.value(variant)),
                              {});
        }
    } else if (command == "#SRC_BAR_MY_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barMyLampSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_MY_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barMyLampSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::MyLamp,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barMyLampSources.value(variant)),
                              {});
        }
    } else if (command == "#SRC_BAR_RIVAL_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barRivalLampSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_RIVAL_LAMP") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barRivalLampSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::RivalLamp,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barRivalLampSources.value(variant)),
                              {});
        }
    } else if (command == "#SRC_BAR_RANK") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barRankSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_RANK") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barRankSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::Rank,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barRankSources.value(variant)),
                              {});
        }
    } else if (command == "#SRC_BAR_RIVAL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barRivalSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_RIVAL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barRivalSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::Rival,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barRivalSources.value(variant)),
                              {});
        }
    } else if (command == "#SRC_BAR_LABEL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.barLabelSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_BAR_LABEL") {
        if (tokens.size() > 1 && !tokens[1].isEmpty() &&
            state.barLabelSources.contains(tokens[1].toInt())) {
            const int variant = tokens[1].toInt();
            appendBarImageDst(state,
                              tokens,
                              Lr2SrcBarImage::Label,
                              -1,
                              variant,
                              QVariant::fromValue(
                                state.barLabelSources.value(variant)),
                              {});
        }
    }
}

void
parseLines(const std::vector<QStringList>& lines,
           int& index,
           ParseState& state,
           const std::filesystem::path& currentDir,
           bool stopAtConditionalBoundary)
{
    while (index < static_cast<int>(lines.size())) {
        const auto& tokens = lines[index];
        if (tokens.isEmpty()) {
            ++index;
            continue;
        }

        const auto command = normalizeCommand(tokens[0]);
        if (stopAtConditionalBoundary &&
            (command == "#ELSE" || command == "#ELSEIF" ||
             command == "#ENDIF")) {
            return;
        }

        if (command == "#IF") {
            parseIfBlock(lines, index, state, currentDir);
            continue;
        }

        processCommand(tokens, state, currentDir);
        ++index;
        ++state.sortId;
    }
}

auto
loadLines(const std::filesystem::path& filePath) -> std::vector<QStringList>
{
    QFile file(support::pathToQString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Could not open LR2 skin file: {}",
                     support::pathToUtfString(filePath));
        return {};
    }

    std::vector<QStringList> lines;
    const auto text = decodeSkinText(file.readAll());
    const auto rawLines = text.split('\n');
    lines.reserve(static_cast<std::size_t>(rawLines.size()));
    for (const auto& raw : rawLines) {
        lines.push_back(tokenizeLine(raw));
    }
    return lines;
}

auto
samePath(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
  -> bool
{
    const auto left = std::filesystem::absolute(lhs).lexically_normal();
    const auto right = std::filesystem::absolute(rhs).lexically_normal();
    return support::pathToQString(left)
             .compare(support::pathToQString(right), Qt::CaseInsensitive) ==
      0;
}

auto
isExtension(const std::filesystem::path& path, const QString& extension) -> bool
{
    return support::pathToQString(path.extension())
             .compare(extension, Qt::CaseInsensitive) == 0;
}

auto
lr2SkinIncludesPath(const std::filesystem::path& lr2skinPath,
                    const std::filesystem::path& includeTarget) -> bool
{
    const auto lines = loadLines(lr2skinPath);
    const auto currentDir = lr2skinPath.parent_path();

    for (const auto& tokens : lines) {
        if (tokens.size() < 2 || normalizeCommand(tokens[0]) != "#INCLUDE") {
            continue;
        }
        const auto includePath = resolveRawPath(currentDir, tokens[1]);
        if (!includePath.empty() && samePath(includePath, includeTarget)) {
            return true;
        }
    }
    return false;
}

auto
siblingLr2SkinForCsv(const std::filesystem::path& filePath)
  -> std::filesystem::path
{
    if (!isExtension(filePath, QStringLiteral(".csv"))) {
        return {};
    }

    const auto directory = filePath.parent_path();
    auto preferred = directory / filePath.stem();
    preferred += ".lr2skin";
    std::error_code ec;
    if (std::filesystem::is_regular_file(preferred, ec) &&
        lr2SkinIncludesPath(preferred, filePath)) {
        return preferred;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
        if (ec || !entry.is_regular_file() ||
            !isExtension(entry.path(), QStringLiteral(".lr2skin")) ||
            samePath(entry.path(), preferred)) {
            continue;
        }
        if (lr2SkinIncludesPath(entry.path(), filePath)) {
            return entry.path();
        }
    }
    return {};
}

auto
topLevelSkinPath(const std::filesystem::path& filePath) -> std::filesystem::path
{
    const auto wrapperPath = siblingLr2SkinForCsv(filePath);
    return wrapperPath.empty() ? filePath : wrapperPath;
}

void
parseFileIntoState(const std::filesystem::path& filePath, ParseState& state)
{
    const auto lines = loadLines(filePath);
    if (lines.empty()) {
        return;
    }
    int index = 0;
    parseLines(lines, index, state, filePath.parent_path(), false);
}

auto
parseFile(const std::filesystem::path& filePath,
          const QVariantMap& settingValues,
          const std::set<int>& initialOptions) -> Lr2SkinData
{
    ParseState state;
    state.settingValues = settingValues;
    state.activeOptions = initialOptions;
    parseFileIntoState(filePath, state);
    flushCurrentElement(state);
    state.laneCoverSource =
      selectedCustomFileSource(state, QStringLiteral("lanecover"));
    if (!state.hasSkinResolution) {
        const auto [skinWidth, skinHeight] = inferSkinCanvas(state.elements);
        state.skinWidth = skinWidth;
        state.skinHeight = skinHeight;
    }

    QVariantList activeOptions;
    for (const int option : state.activeOptions) {
        activeOptions.append(option);
    }
    QVariantList usedOptions;
    for (const int option : state.usedOptions) {
        usedOptions.append(option);
    }
    QVariantList usedElementOptions;
    for (const int option : state.usedElementOptions) {
        usedElementOptions.append(option);
    }

    QVariantList barLampVariants;
    for (auto it = state.barLampSources.cbegin();
         it != state.barLampSources.cend();
         ++it) {
        barLampVariants.append(it.key());
    }

    QVariantList barLevelVariants;
    for (auto it = state.barLevelSources.cbegin();
         it != state.barLevelSources.cend();
         ++it) {
        barLevelVariants.append(it.key());
    }

    QVariantList barBodyTypes;
    for (auto it = state.barBodySources.cbegin();
         it != state.barBodySources.cend();
         ++it) {
        barBodyTypes.append(it.key());
    }

    QVariantList barTitleTypes;
    for (auto it = state.barTitleSources.cbegin();
         it != state.barTitleSources.cend();
         ++it) {
        barTitleTypes.append(it.key());
    }

    QVariantList barRows;
    auto rows = QSet<int>{};
    for (auto it = state.barBodyOffDsts.cbegin();
         it != state.barBodyOffDsts.cend();
         ++it) {
        rows.insert(it.key());
    }
    for (auto it = state.barBodyOnDsts.cbegin();
         it != state.barBodyOnDsts.cend();
         ++it) {
        rows.insert(it.key());
    }
    int maxRow = -1;
    for (const int row : rows) {
        maxRow = (std::max)(maxRow, row);
    }
    for (int row = 0; row <= maxRow; ++row) {
        QVariantMap rowData;
        rowData["row"] = row;
        rowData["offDsts"] = state.barBodyOffDsts.value(row);
        rowData["onDsts"] = state.barBodyOnDsts.value(row);
        barRows.append(rowData);
    }

    return Lr2SkinData{
      .elements = state.elements,
      .skinWidth = state.skinWidth,
      .skinHeight = state.skinHeight,
      .activeOptions = activeOptions,
      .usedOptions = usedOptions,
      .usedElementOptions = usedElementOptions,
      .barLampVariants = barLampVariants,
      .barLevelVariants = barLevelVariants,
      .barRows = barRows,
      .barBodyTypes = barBodyTypes,
      .barTitleTypes = barTitleTypes,
      .helpFiles = state.helpFiles,
      .transColor = state.transColor,
      .hasTransColor = state.hasTransColor,
      .laneCoverSource = state.laneCoverSource,
      .reloadBanner = state.reloadBanner,
      .startInput = state.startInput,
      .sceneTime = state.sceneTime,
      .loadStart = state.loadStart,
      .loadEnd = state.loadEnd,
      .playStart = state.playStart,
      .fadeOut = state.fadeOut,
      .skip = state.skip,
      .barCenter = state.barCenter,
      .barAvailableStart = state.barAvailableStart,
      .barAvailableEnd = state.barAvailableEnd,
      .noteSources = toVariantList(state.noteSources),
      .mineSources = toVariantList(state.mineSources),
      .lnStartSources = toVariantList(state.lnStartSources),
      .lnEndSources = toVariantList(state.lnEndSources),
      .lnBodySources = toVariantList(state.lnBodySources),
      .lnBodyActiveSources = toVariantList(state.lnBodyActiveSources),
      .autoNoteSources = toVariantList(state.autoNoteSources),
      .autoMineSources = toVariantList(state.autoMineSources),
      .autoLnStartSources = toVariantList(state.autoLnStartSources),
      .autoLnEndSources = toVariantList(state.autoLnEndSources),
      .autoLnBodySources = toVariantList(state.autoLnBodySources),
      .autoLnBodyActiveSources =
        toVariantList(state.autoLnBodyActiveSources),
      .noteDsts = toVariantList(state.noteDsts),
      .lineSources = toVariantList(state.lineSources),
      .lineDsts = toVariantList(state.lineDsts),
    };
}

auto
parseOptions(const QVariantList& activeOptions) -> std::set<int>
{
    std::set<int> options{0};
    for (const auto& optionValue : activeOptions) {
        bool ok = false;
        const int option = optionValue.toInt(&ok);
        if (ok) {
            options.insert(option);
        }
    }
    return options;
}
} // namespace

QList<Lr2Element>
Lr2SkinParser::parse(const QString& path,
                     const QVariantMap& settingValues,
                     const QVariantList& activeOptions)
{
    return parseData(path, settingValues, activeOptions).elements;
}

Lr2SkinData
Lr2SkinParser::parseData(const QString& path,
                         const QVariantMap& settingValues,
                         const QVariantList& activeOptions)
{
    const auto options = parseOptions(activeOptions);
    return parseFile(topLevelSkinPath(support::qStringToPath(path)),
                     settingValues,
                     options);
}

} // namespace gameplay_logic::lr2_skin
