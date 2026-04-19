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
#include <vector>
#include <spdlog/spdlog.h>

namespace gameplay_logic::lr2_skin {
namespace {
struct CustomFile
{
    QString settingId;
    std::filesystem::path directory;
    QString wildcard;
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
    int barCenter = 0;
    int barAvailableStart = 0;
    int barAvailableEnd = -1;
    std::set<int> activeOptions;
    Lr2Element currentElement;
    bool hasCurrentElement = false;
    QVariantMap settingValues;
    int startInput = 0;
    int sceneTime = 0;
    int fadeOut = 0;
    int skip = 0;
};

auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
}

auto
lr2ConfiguredFontFamily() -> QString
{
    // LR2 ignores the family tokens in #FONT and uses config.skin.fontname.
    // The stock config default is "Ariel" (sic); Qt resolves "Arial" more reliably.
    return QStringLiteral("Arial");
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
    const auto utf8 = utf8Decoder.decode(data);
    if (!utf8Decoder.hasError()) {
        return utf8;
    }

    QStringDecoder shiftJisDecoder("Shift-JIS");
    if (shiftJisDecoder.isValid()) {
        return shiftJisDecoder.decode(data);
    }

    return QString::fromLatin1(data);
}

auto
parseDstValue(const QStringList& tokens) -> Lr2Dst
{
    Lr2Dst dst;
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
        dst.op1 = tokens[18].toInt();
    if (tokens.size() > 19 && !tokens[19].isEmpty())
        dst.op2 = tokens[19].toInt();
    if (tokens.size() > 20 && !tokens[20].isEmpty())
        dst.op3 = tokens[20].toInt();
    if (tokens.size() > 21 && !tokens[21].isEmpty())
        dst.op4 = tokens[21].toInt();
    return dst;
}

void
parseDst(const QStringList& tokens, Lr2Element& element)
{
    const auto dst = parseDstValue(tokens);
    element.dsts.append(QVariant::fromValue(dst));
}

auto
globToRegex(const QString& wildcard) -> QRegularExpression
{
    return QRegularExpression(QRegularExpression::wildcardToRegularExpression(
      wildcard, QRegularExpression::UnanchoredWildcardConversion));
}

auto
resolveWildcardPath(const std::filesystem::path& absolutePattern,
                    const ParseState& state) -> QString
{
    const auto directory = absolutePattern.parent_path();
    const auto wildcard =
      QString::fromStdString(absolutePattern.filename().generic_string());
    const auto normalizedDirectory =
      std::filesystem::absolute(directory).lexically_normal();

    for (const auto& customFile : state.customFiles) {
        if (customFile.directory.lexically_normal() == normalizedDirectory &&
            customFile.wildcard.compare(wildcard, Qt::CaseInsensitive) == 0) {
            const auto selected =
              state.settingValues.value(customFile.settingId).toString();
            if (!selected.isEmpty()) {
                const auto selectedPath =
                  directory / support::qStringToPath(selected);
                if (std::filesystem::is_regular_file(selectedPath)) {
                    return support::pathToQString(
                      std::filesystem::absolute(selectedPath));
                }
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
resolvePath(const std::filesystem::path& currentDir,
            const QString& token,
            const ParseState& state) -> QString
{
    if (token.isEmpty() ||
        token.compare("CONTINUE", Qt::CaseInsensitive) == 0) {
        return {};
    }

    auto normalized = support::pathToQString(
      relative(support::qStringToPath(token), "LR2files/"));
    // replace "Theme" (case insensitive) with "themes"
    normalized.replace(
      QRegularExpression("^Theme", QRegularExpression::CaseInsensitiveOption),
      "themes");
    auto path = support::qStringToPath(normalized);
    if (path.is_relative()) {
        path = currentDir.parent_path().parent_path().parent_path() / path;
    }

    const auto abs = std::filesystem::absolute(path);
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

        const auto command = tokens[0].toUpper();
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
parseIfBlock(const std::vector<QStringList>& lines,
             int& index,
             ParseState& state,
             const std::filesystem::path& currentDir)
{
    bool branchMatched = false;

    while (index < static_cast<int>(lines.size())) {
        const auto tokens = lines[index];
        const auto command = tokens[0].toUpper();

        bool executeBranch = false;
        if (command == "#IF" || command == "#ELSEIF") {
            executeBranch = !branchMatched && evaluateCondition(tokens, state);
        } else if (command == "#ELSE") {
            executeBranch = !branchMatched;
        } else {
            return;
        }

        ++index;
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
          lines[index].isEmpty() ? QString{} : lines[index][0].toUpper();
        if (nextCommand == "#ENDIF") {
            ++index;
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

    const auto [specialType, source] =
      sourceForGr(src.gr, src.w, src.h, state);
    src.specialType = specialType;
    src.source = source;
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

    const auto [specialType, source] =
      sourceForGr(src.gr, src.w, src.h, state);
    if (specialType == Lr2SrcImage::None) {
        src.source = source;
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

    const auto dst = parseDstValue(tokens);
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
    parseDst(tokens, state.currentElement);
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
    parseDst(tokens, state.currentElement);
}

void
processCommand(const QStringList& tokens,
               ParseState& state,
               const std::filesystem::path& currentDir)
{
    if (tokens.isEmpty()) {
        return;
    }

    const auto command = tokens[0].toUpper();
    if (command == "#STARTINPUT") {
        if (tokens.size() > 1) {
            state.startInput = tokens[1].trimmed().toInt();
        }
    } else if (command == "#SCENETIME") {
        if (tokens.size() > 1) {
            state.sceneTime = tokens[1].trimmed().toInt();
        }
    } else if (command == "#FADEOUT") {
        if (tokens.size() > 1) {
            state.fadeOut = tokens[1].trimmed().toInt();
        }
    } else if (command == "#SKIP") {
        if (tokens.size() > 1) {
            state.skip = tokens[1].trimmed().toInt();
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

        // A #CUSTOMOPTION is an exclusive choice range: base, base + 1, ...
        // Clear any seeded fallback from the wrapper before applying the skin
        // setting/default so direct .csv loads and .lr2skin loads behave alike.
        int optionOffset = 0;
        for (int i = 3; i < tokens.size(); ++i) {
            if (tokens[i].trimmed().isEmpty()) {
                continue;
            }
            state.activeOptions.erase(baseOption + optionOffset);
            ++optionOffset;
        }

        optionOffset = 0;
        for (int i = 3; i < tokens.size(); ++i) {
            if (tokens[i].trimmed().isEmpty()) {
                continue;
            }
            if (tokens[i].trimmed() == selected) {
                state.activeOptions.insert(baseOption + optionOffset);
                break;
            }
            ++optionOffset;
        }
    } else if (command == "#CUSTOMFILE") {
        if (tokens.size() < 3) {
            return;
        }
        auto patternPath = support::qStringToPath(tokens[2].trimmed());
        if (patternPath.is_relative()) {
            patternPath = currentDir / patternPath;
        }
        state.customFiles.append(CustomFile{
          .settingId = makeSafeId(tokens[1].trimmed(), "file"),
          .directory = std::filesystem::absolute(patternPath.parent_path())
                         .lexically_normal(),
          .wildcard =
            QString::fromStdString(patternPath.filename().generic_string()),
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
    } else if (command == "#INCLUDE") {
        if (tokens.size() < 2 || tokens[1].trimmed().isEmpty()) {
            return;
        }
        const auto includePath =
          resolvePath(currentDir, tokens[1].trimmed(), state);
        if (!includePath.isEmpty()) {
            parseFileIntoState(support::qStringToPath(includePath), state);
        }
    } else if (command == "#IMAGE") {
        state.images.append(
          resolvePath(currentDir,
                      tokens.size() > 1 ? tokens[1].trimmed() : QString{},
                      state));
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
    } else if (command == "#SRC_IMAGE") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;

        state.currentElement.src =
          QVariant::fromValue(parseImageSource(tokens, state));
    } else if (command == "#DST_IMAGE") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state.currentElement);
        }
    } else if (command == "#SRC_BUTTON" || command == "#SRC_SLIDER") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 0;
        state.hasCurrentElement = true;
        auto src = parseImageSource(tokens, state);
        src.button = true;
        src.buttonId = src.op1;
        src.buttonClick = src.op2;
        src.buttonPanel = src.op3;
        if (tokens.size() > 14 && !tokens[14].isEmpty()) {
            src.buttonPlusOnly = tokens[14].toInt();
        }
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_BUTTON" || command == "#DST_SLIDER") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state.currentElement);
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
            parseDst(tokens, state.currentElement);
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
            parseDst(tokens, state.currentElement);
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
            parseDst(tokens, state.currentElement);
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

        const auto command = tokens[0].toUpper();
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

    QVariantList activeOptions;
    for (const int option : state.activeOptions) {
        activeOptions.append(option);
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
      .activeOptions = activeOptions,
      .barRows = barRows,
      .startInput = state.startInput,
      .sceneTime = state.sceneTime,
      .fadeOut = state.fadeOut,
      .skip = state.skip,
      .barCenter = state.barCenter,
      .barAvailableStart = state.barAvailableStart,
      .barAvailableEnd = state.barAvailableEnd,
    };
}

auto
parseOptions(const QVariantList& activeOptions) -> std::set<int>
{
    std::set<int> options;
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
