#include "Lr2SkinParser.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QFile>
#include <QRegularExpression>
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

void
parseDst(const QStringList& tokens, Lr2Element& element)
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

        for (int i = 3; i < tokens.size(); ++i) {
            if (tokens[i].trimmed().isEmpty()) {
                continue;
            }
            if (tokens[i].trimmed() == selected) {
                state.activeOptions.insert(baseOption + (i - 3));
                break;
            }
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

        Lr2SrcImage src;
        if (tokens.size() > 2 && !tokens[2].isEmpty())
            src.gr = tokens[2].toInt();
        if (tokens.size() > 3 && !tokens[3].isEmpty())
            src.x = tokens[3].toInt();
        if (tokens.size() > 4 && !tokens[4].isEmpty())
            src.y = tokens[4].toInt();
        if (tokens.size() > 5 && !tokens[5].isEmpty())
            src.w = tokens[5].toInt();
        if (tokens.size() > 6 && !tokens[6].isEmpty())
            src.h = tokens[6].toInt();
        if (tokens.size() > 7 && !tokens[7].isEmpty())
            src.div_x = qMax(1, tokens[7].toInt());
        if (tokens.size() > 8 && !tokens[8].isEmpty())
            src.div_y = qMax(1, tokens[8].toInt());
        if (tokens.size() > 9 && !tokens[9].isEmpty())
            src.cycle = tokens[9].toInt();
        if (tokens.size() > 10 && !tokens[10].isEmpty())
            src.timer = tokens[10].toInt();
        if (tokens.size() > 11 && !tokens[11].isEmpty())
            src.op1 = tokens[11].toInt();
        if (tokens.size() > 12 && !tokens[12].isEmpty())
            src.op2 = tokens[12].toInt();
        if (tokens.size() > 13 && !tokens[13].isEmpty())
            src.op3 = tokens[13].toInt();

        if (src.gr == 100) {
            src.specialType = Lr2SrcImage::StageFile;
        } else if (src.gr == 110) {
            src.specialType = Lr2SrcImage::SolidBlack;
        } else if (src.gr >= 0 && src.gr < state.images.size()) {
            src.source = state.images[src.gr];
            if (src.source.isEmpty() && src.w <= 1 && src.h <= 1) {
                src.specialType = Lr2SrcImage::SolidBlack;
            }
        }

        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_IMAGE") {
        if (state.hasCurrentElement && state.currentElement.type == 0) {
            parseDst(tokens, state.currentElement);
        }
    } else if (command == "#SRC_NUMBER") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 1;
        state.hasCurrentElement = true;

        Lr2SrcNumber src;
        if (tokens.size() > 2 && !tokens[2].isEmpty())
            src.gr = tokens[2].toInt();
        if (tokens.size() > 3 && !tokens[3].isEmpty())
            src.x = tokens[3].toInt();
        if (tokens.size() > 4 && !tokens[4].isEmpty())
            src.y = tokens[4].toInt();
        if (tokens.size() > 5 && !tokens[5].isEmpty())
            src.w = tokens[5].toInt();
        if (tokens.size() > 6 && !tokens[6].isEmpty())
            src.h = tokens[6].toInt();
        if (tokens.size() > 7 && !tokens[7].isEmpty())
            src.div_x = qMax(1, tokens[7].toInt());
        if (tokens.size() > 8 && !tokens[8].isEmpty())
            src.div_y = qMax(1, tokens[8].toInt());
        if (tokens.size() > 9 && !tokens[9].isEmpty())
            src.cycle = tokens[9].toInt();
        if (tokens.size() > 10 && !tokens[10].isEmpty())
            src.timer = tokens[10].toInt();
        if (tokens.size() > 11 && !tokens[11].isEmpty())
            src.num = tokens[11].toInt();
        if (tokens.size() > 12 && !tokens[12].isEmpty())
            src.align = tokens[12].toInt();
        if (tokens.size() > 13 && !tokens[13].isEmpty())
            src.keta = tokens[13].toInt();
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_NUMBER") {
        if (state.hasCurrentElement && state.currentElement.type == 1) {
            parseDst(tokens, state.currentElement);
        }
    } else if (command == "#SRC_TEXT") {
        flushCurrentElement(state);
        state.currentElement = Lr2Element{};
        state.currentElement.type = 2;
        state.hasCurrentElement = true;

        Lr2SrcText src;
        if (tokens.size() > 2 && !tokens[2].isEmpty())
            src.font = tokens[2].toInt();
        if (tokens.size() > 3 && !tokens[3].isEmpty())
            src.st = tokens[3].toInt();
        if (tokens.size() > 4 && !tokens[4].isEmpty())
            src.align = tokens[4].toInt();
        if (tokens.size() > 5 && !tokens[5].isEmpty())
            src.edit = tokens[5].toInt();
        if (tokens.size() > 6 && !tokens[6].isEmpty())
            src.panel = tokens[6].toInt();
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
        state.currentElement.src = QVariant::fromValue(src);
    } else if (command == "#DST_TEXT") {
        if (state.hasCurrentElement && state.currentElement.type == 2) {
            parseDst(tokens, state.currentElement);
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
    return Lr2SkinData{
      .elements = state.elements,
      .startInput = state.startInput,
      .sceneTime = state.sceneTime,
      .fadeOut = state.fadeOut,
      .skip = state.skip,
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
