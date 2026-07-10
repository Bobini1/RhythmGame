#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QXmlStreamReader>

#include <algorithm>
#include <tuple>

namespace {

struct MessageKey
{
    QString context;
    QString source;
    QString comment;
    bool numerus{};

    friend auto operator==(const MessageKey&, const MessageKey&) -> bool =
      default;
    friend auto operator<(const MessageKey& left, const MessageKey& right)
      -> bool
    {
        return std::tie(left.context,
                        left.source,
                        left.comment,
                        left.numerus) <
               std::tie(right.context,
                        right.source,
                        right.comment,
                        right.numerus);
    }
};

struct CatalogMessage
{
    MessageKey key;
    QStringList locations;
    QStringList translations;
    bool unfinished{};
    bool obsolete{};
};

auto
sourcePath(const QString& relativePath) -> QString
{
    return QDir(QStringLiteral(ARENA_QML_SOURCE_ROOT)).filePath(relativePath);
}

auto
readTextFile(const QString& relativePath) -> QString
{
    const auto path = sourcePath(relativePath);
    QFile file(path);
    INFO("Source file: " << path.toStdString());
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    return QString::fromUtf8(file.readAll());
}

auto
readTranslation(QXmlStreamReader& xml,
                bool& unfinished,
                bool& obsolete) -> QStringList
{
    const auto type = xml.attributes().value(QStringLiteral("type"));
    unfinished = type == QStringLiteral("unfinished");
    obsolete = type == QStringLiteral("vanished") ||
               type == QStringLiteral("obsolete");
    auto forms = QStringList{};
    auto directText = QString{};
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() &&
            xml.name() == QStringLiteral("numerusform")) {
            forms.push_back(xml.readElementText(
              QXmlStreamReader::IncludeChildElements));
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            directText += xml.text();
        } else if (xml.isEndElement() &&
                   xml.name() == QStringLiteral("translation")) {
            break;
        }
    }
    if (forms.isEmpty()) {
        forms.push_back(directText);
    }
    return forms;
}

auto
readMessage(QXmlStreamReader& xml, const QString& context)
  -> CatalogMessage
{
    CatalogMessage result;
    result.key.context = context;
    result.key.numerus =
      xml.attributes().value(QStringLiteral("numerus")) ==
      QStringLiteral("yes");
    while (xml.readNextStartElement()) {
        if (xml.name() == QStringLiteral("location")) {
            result.locations.push_back(
              xml.attributes().value(QStringLiteral("filename")).toString());
            xml.skipCurrentElement();
        } else if (xml.name() == QStringLiteral("source")) {
            result.key.source = xml.readElementText(
              QXmlStreamReader::IncludeChildElements);
        } else if (xml.name() == QStringLiteral("comment")) {
            result.key.comment = xml.readElementText(
              QXmlStreamReader::IncludeChildElements);
        } else if (xml.name() == QStringLiteral("translation")) {
            result.translations =
              readTranslation(xml, result.unfinished, result.obsolete);
        } else {
            xml.skipCurrentElement();
        }
    }
    return result;
}

auto
readCatalog(const QString& fileName) -> QList<CatalogMessage>
{
    const auto path = sourcePath(
      QStringLiteral("share/RhythmGame/themes/Default/translations/") +
      fileName);
    QFile file(path);
    INFO("Translation catalog: " << path.toStdString());
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QXmlStreamReader xml(&file);
    auto messages = QList<CatalogMessage>{};
    while (xml.readNextStartElement()) {
        if (xml.name() != QStringLiteral("TS")) {
            xml.skipCurrentElement();
            continue;
        }
        while (xml.readNextStartElement()) {
            if (xml.name() != QStringLiteral("context")) {
                xml.skipCurrentElement();
                continue;
            }
            auto context = QString{};
            while (xml.readNextStartElement()) {
                if (xml.name() == QStringLiteral("name")) {
                    context = xml.readElementText();
                } else if (xml.name() == QStringLiteral("message")) {
                    messages.push_back(readMessage(xml, context));
                } else {
                    xml.skipCurrentElement();
                }
            }
        }
    }
    INFO("XML error: " << xml.errorString().toStdString());
    REQUIRE_FALSE(xml.hasError());
    return messages;
}

auto
isArenaMessage(const CatalogMessage& message) -> bool
{
    if (message.key.context.contains(QStringLiteral("Arena"))) {
        return true;
    }
    return std::ranges::any_of(message.locations, [](const auto& location) {
        const auto normalized = QDir::fromNativeSeparators(location);
        return normalized.contains(QStringLiteral("/Arena/")) ||
               normalized.section(QChar('/'), -1).contains(
                 QStringLiteral("Arena"));
    });
}

auto
arenaMessages(const QList<CatalogMessage>& catalog)
  -> QMap<MessageKey, CatalogMessage>
{
    auto result = QMap<MessageKey, CatalogMessage>{};
    for (const auto& message : catalog) {
        if (message.obsolete || !isArenaMessage(message)) {
            continue;
        }
        INFO("Arena translation key: "
             << message.key.context.toStdString() << " | "
             << message.key.source.toStdString() << " | "
             << message.key.comment.toStdString() << " | numerus="
             << message.key.numerus);
        REQUIRE_FALSE(result.contains(message.key));
        result.insert(message.key, message);
    }
    return result;
}

auto
placeholders(const QString& text) -> QSet<QString>
{
    static const auto pattern =
      QRegularExpression(QStringLiteral("%(?:L\\d+|\\d+|n)"));
    auto result = QSet<QString>{};
    auto matches = pattern.globalMatch(text);
    while (matches.hasNext()) {
        result.insert(matches.next().captured());
    }
    return result;
}

void
requireComplete(const CatalogMessage& message, qsizetype expectedFormCount)
{
    INFO("Context: " << message.key.context.toStdString());
    INFO("Source: " << message.key.source.toStdString());
    INFO("Comment: " << message.key.comment.toStdString());
    CHECK_FALSE(message.unfinished);
    REQUIRE(message.translations.size() == expectedFormCount);
    const auto sourcePlaceholders = placeholders(message.key.source);
    for (const auto& translation : message.translations) {
        CHECK_FALSE(translation.trimmed().isEmpty());
        CHECK(placeholders(translation) == sourcePlaceholders);
    }
}

auto
keys(const QMap<MessageKey, CatalogMessage>& messages)
  -> QList<MessageKey>
{
    return messages.keys();
}

} // namespace

TEST_CASE("ArenaTranslationContract: QML discovery reconfigures when sources change",
          "[arena][ArenaTranslation]")
{
    const auto source = readTextFile(QStringLiteral("cmake/translations.cmake"));
    CHECK(source.contains(QStringLiteral(
      "file(GLOB_RECURSE DEFAULT_QML_FILES CONFIGURE_DEPENDS")));
    CHECK(source.contains(QStringLiteral(
      "file(GLOB ARENA_QML_FILES CONFIGURE_DEPENDS")));
}

TEST_CASE("ArenaTranslationContract: supported catalogs share exact Arena identities",
          "[arena][ArenaTranslation]")
{
    const auto english =
      arenaMessages(readCatalog(QStringLiteral("Default_en.ts")));
    const auto polish =
      arenaMessages(readCatalog(QStringLiteral("Default_pl.ts")));
    const auto japanese =
      arenaMessages(readCatalog(QStringLiteral("Default_jp.ts")));

    REQUIRE_FALSE(polish.isEmpty());
    CHECK(keys(polish) == keys(japanese));

    auto expectedEnglishKeys = QList<MessageKey>{};
    for (auto iterator = polish.cbegin(); iterator != polish.cend(); ++iterator) {
        if (iterator.key().numerus) {
            expectedEnglishKeys.push_back(iterator.key());
        }
    }
    REQUIRE_FALSE(expectedEnglishKeys.isEmpty());
    CHECK(keys(english) == expectedEnglishKeys);

    for (const auto& message : polish) {
        requireComplete(message, message.key.numerus ? 3 : 1);
    }
    for (const auto& message : english) {
        REQUIRE(message.key.numerus);
        requireComplete(message, 2);
    }
}
