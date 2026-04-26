#include "Lr2SkinParser.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QStringList>
#include <algorithm>
#include <filesystem>
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
    QList<QString> images;
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
    QMap<int, Lr2SrcNumber> barLevelSources;
    QMap<int, Lr2SrcBarText> barTitleSources;
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
};

auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
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
    }
    return tokens;
}

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

    QStringDecoder shiftJisDecoder("Shift-JIS");
    if (shiftJisDecoder.isValid()) {
        return shiftJisDecoder.decode(data);
    }

    return QString::fromLatin1(data);
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
    const int option = token.toInt(&ok);
    if (!ok || option == 0) {
        return 0;
    }

    return negated ? -option : option;
}

auto
parseDstValue(const QStringList& tokens, const int sortId) -> Lr2Dst
{
    Lr2Dst dst;
    dst.sortId = sortId;
    if (tokens.size() > 2 && !tokens[2].isEmpty())
        dst.time = tokens[2].toInt();
    if (tokens.size() > 3 && !tokens[3].isEmpty())
        dst.x = tokens[3].toInt();
    if (tokens.size() > 4 && !tokens[4].isEmpty())
        dst.y = tokens[4].toInt();
    if (tokens.size() > 5 && !tokens[5].isEmpty())
        dst.w = tokens[5].toInt();
    if (tokens.size() > 6 && !tokens[6].isEmpty())
        dst.h = tokens[6].toInt();
    if (tokens.size() > 7 && !tokens[7].isEmpty())
        dst.acc = tokens[7].toInt();
    if (tokens.size() > 8 && !tokens[8].isEmpty())
        dst.a = tokens[8].toInt();
    if (tokens.size() > 9 && !tokens[9].isEmpty())
        dst.r = tokens[9].toInt();
    if (tokens.size() > 10 && !tokens[10].isEmpty())
        dst.g = tokens[10].toInt();
    if (tokens.size() > 11 && !tokens[11].isEmpty())
        dst.b = tokens[11].toInt();
    if (tokens.size() > 12 && !tokens[12].isEmpty())
        dst.blend = tokens[12].toInt();
    if (tokens.size() > 13 && !tokens[13].isEmpty())
        dst.filter = tokens[13].toInt();
    if (tokens.size() > 14 && !tokens[14].isEmpty())
        dst.angle = tokens[14].toInt();
    if (tokens.size() > 15 && !tokens[15].isEmpty())
        dst.center = tokens[15].toInt();
    if (tokens.size() > 16 && !tokens[16].isEmpty())
        dst.loop = tokens[16].toInt();
    if (tokens.size() > 17 && !tokens[17].isEmpty())
        dst.timer = tokens[17].toInt();
    if (tokens.size() > 18 && !tokens[18].isEmpty())
        dst.op1 = parseDstOptionToken(tokens[18]);
    if (tokens.size() > 19 && !tokens[19].isEmpty())
        dst.op2 = parseDstOptionToken(tokens[19]);
    if (tokens.size() > 20 && !tokens[20].isEmpty())
        dst.op3 = parseDstOptionToken(tokens[20]);
    if (tokens.size() > 21 && !tokens[21].isEmpty())
        dst.op4 = tokens[21].toInt();
    return dst;
}

void
recordDstOption(ParseState& state, const int option)
{
    if (option == 0) {
        return;
    }
    state.usedOptions.insert(option < 0 ? -option : option);
}

void
recordDstOptions(ParseState& state, const Lr2Dst& dst)
{
    recordDstOption(state, dst.op1);
    recordDstOption(state, dst.op2);
    recordDstOption(state, dst.op3);
}

void
parseDst(const QStringList& tokens, ParseState& state, Lr2Element& element)
{
    const auto dst = parseDstValue(tokens, state.sortId);
    recordDstOptions(state, dst);
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
    recordDstOptions(state, dst);
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
      wildcard, QRegularExpression::UnanchoredWildcardConversion));
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

    const auto selectedPath = directory / support::qStringToPath(trimmed);
    if (std::filesystem::is_regular_file(selectedPath)) {
        return selectedPath;
    }

    const auto suffix = wildcardSuffixForSelection(wildcard);
    if (!suffix.isEmpty() && !trimmed.endsWith(suffix, Qt::CaseInsensitive)) {
        const auto withSuffix =
          directory / support::qStringToPath(trimmed + suffix);
        if (std::filesystem::is_regular_file(withSuffix)) {
            return withSuffix;
        }
    }

    return {};
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
    const auto rawWildcard =
      QString::fromStdString(absolutePattern.filename().generic_string());
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
        const auto filename =
          QString::fromStdString(entry.path().filename().generic_string());
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
        const int option = optionToken.toInt(&ok);
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

void
considerSkinCanvasDst(const Lr2Dst& dst, int& bestWidth, int& bestHeight)
{
    const int w = dst.w < 0 ? -dst.w : dst.w;
    const int h = dst.h < 0 ? -dst.h : dst.h;
    if (dst.x != 0 || dst.y != 0 || w < 320 || h < 240) {
        return;
    }

    const double aspect = static_cast<double>(w) / static_cast<double>(h);
    if (aspect < 1.2 || aspect > 2.1) {
        return;
    }

    if (w * h > bestWidth * bestHeight) {
        bestWidth = w;
        bestHeight = h;
    }
}

auto
inferSkinCanvas(const QList<Lr2Element>& elements) -> std::pair<int, int>
{
    int bestWidth = 640;
    int bestHeight = 480;
    for (const auto& element : elements) {
        for (const auto& dstValue : element.dsts) {
            if (dstValue.canConvert<Lr2Dst>()) {
                considerSkinCanvasDst(
                  dstValue.value<Lr2Dst>(), bestWidth, bestHeight);
            }
        }
    }
    return { bestWidth, bestHeight };
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
        if (source.isEmpty() && w <= 1 && h <= 1) {
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
        src.div_x = qMax(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = qMax(1, tokens[grIndex + 6].toInt());
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
parseImageSetSource(const QStringList& tokens,
                    const ParseState& state) -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.imageSet = true;
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
        src.div_x = qMax(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = qMax(1, tokens[grIndex + 6].toInt());
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
        src.div_x = qMax(1, tokens[grIndex + 5].toInt());
    if (tokens.size() > grIndex + 6 && !tokens[grIndex + 6].isEmpty())
        src.div_y = qMax(1, tokens[grIndex + 6].toInt());
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
    parseDst(tokens, state, state.currentElement);
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
    parseDst(tokens, state, state.currentElement);
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
        const auto settingId =
          makeSafeId(tokens[1].trimmed(), "opt_" + tokens[2].trimmed());
        const int baseOption = tokens[2].trimmed().toInt();
        auto selected = state.settingValues.value(settingId).toString();
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
        const auto patternPath = resolveRawPath(currentDir, tokens[2].trimmed());
        state.customFiles.append(CustomFile{
          .settingId = makeSafeId(tokens[1].trimmed(), "file"),
          .directory = std::filesystem::absolute(patternPath.parent_path())
                         .lexically_normal(),
          .wildcard =
            QString::fromStdString(patternPath.filename().generic_string()),
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
    } else if (command == "#TRANSCOLOR") {
        if (tokens.size() > 3) {
            state.transColor = colorKeyString(tokens[1].trimmed().toInt(),
                                              tokens[2].trimmed().toInt(),
                                              tokens[3].trimmed().toInt());
            state.hasTransColor = true;
        }
    } else if (command == "#IMAGE") {
        state.images.append(
          resolvePath(currentDir,
                      tokens.size() > 1 ? tokens[1].trimmed() : QString{},
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
    } else if (command == "#SRC_IMAGE" || command == "#SRC_JUDGELINE") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseImageSource(tokens, state));
    } else if (command == "#SRC_IMAGESET") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseImageSetSource(tokens, state));
    } else if (command == "#DST_IMAGE" || command == "#DST_JUDGELINE") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state, state.currentElement);
        }
    } else if (command == "#SRC_LINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lineSources[tokens[1].toInt()] =
              parseImageSource(tokens, state);
        }
    } else if (command == "#DST_LINE") {
        if (tokens.size() > 1 && !tokens[1].isEmpty()) {
            state.lineDsts[tokens[1].toInt()].append(
              QVariant::fromValue(parseDstValue(tokens, state.sortId)));
        }
    } else if (command == "#SRC_BGA") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 7;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseImageSource(tokens, state));
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
    } else if (command == "#SRC_SLIDER") {
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
                  judgeDsts.at(qMin(dstIndex, judgeDsts.size() - 1));
                dst.x += judgeDst.x;
                dst.y += judgeDst.y;
            }
            dst.timer = lr2NowDisplayTimer(command);
            addDstOptionGate(dst, lr2NowJudgementOption(command, index));
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
    } else if (command == "#SRC_GROOVEGAUGE") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 9;
        state.hasCurrentElement = true;

        auto src = parseImageSource(tokens, state);
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
        spdlog::warn("Could not open LR2 skin file: {}", filePath.string());
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

    QVariantList barLampVariants;
    for (auto it = state.barLampSources.cbegin();
         it != state.barLampSources.cend();
         ++it) {
        barLampVariants.append(it.key());
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
        maxRow = std::max(maxRow, row);
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
      .barLampVariants = barLampVariants,
      .barRows = barRows,
      .helpFiles = state.helpFiles,
      .transColor = state.transColor,
      .hasTransColor = state.hasTransColor,
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
    return parseFile(support::qStringToPath(path), settingValues, options);
}

} // namespace gameplay_logic::lr2_skin
