#include <catch2/catch_test_macros.hpp>

#include "resource_managers/Lr2FontCache.h"
#include "resource_managers/Lr2FontImageProvider.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

auto
writeTestFont(const QString& path) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream stream(&file);
    stream << "#S,18\n";
    stream << "#M,-2\n";
    stream << "#R,65,0,0,0,3,18\n";   // CP932 0x41 -> A
    stream << "#R,177,0,3,0,3,18\n";  // CP932 0xb1 -> half-width katakana A
    stream << "#R,608,0,6,0,3,18\n";  // CP932 0x82a0 -> hiragana A
    stream << "#R,5050,0,9,0,3,18\n"; // CP932 0x93fa -> day/sun kanji
}

auto
writeKerningFont(const QString& path) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream stream(&file);
    stream << "#S,18\n";
    stream << "#M,-2\n";
    stream << "#R,65,0,0,0,10,18\n";
    stream << "#R,66,0,10,0,8,18\n";
}

auto
writeTallGlyphFont(const QString& path) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream stream(&file);
    stream << "#S,10\n";
    stream << "#M,0\n";
    stream << "#T,0,atlas.png\n";
    stream << "#R,65,0,0,0,3,12\n";
}

auto
writeFractionalFont(const QString& path) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream stream(&file);
    stream << "#S,18.5\n";
    stream << "#M,-2.75\n";
    stream << "#R,89,0,83,240.875,33,82\n";
}

} // namespace

TEST_CASE("LR2 font character ids decode as CP932",
          "[LR2FONT][resource_managers]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto fontPath = tempDir.filePath("font.lr2font");
    writeTestFont(fontPath);

    const auto* dict =
      resource_managers::Lr2FontCache::instance().load(fontPath);
    REQUIRE(dict != nullptr);

    CHECK(dict->kerning == -2);

    constexpr auto halfWidthKatakanaA = static_cast<char32_t>(0xFF71);
    constexpr auto hiraganaA = static_cast<char32_t>(0x3042);
    constexpr auto dayKanji = static_cast<char32_t>(0x65E5);
    constexpr auto plusMinus = static_cast<char32_t>(0x00B1);

    CHECK(dict->glyphs.contains(U'A'));
    CHECK(dict->glyphs.contains(halfWidthKatakanaA));
    CHECK(dict->glyphs.contains(hiraganaA));
    CHECK(dict->glyphs.contains(dayKanji));
    CHECK(!dict->glyphs.contains(plusMinus));
}

TEST_CASE("LR2 font image provider applies kerning between glyphs",
          "[LR2FONT][resource_managers]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto fontPath = tempDir.filePath("kerning-font.lr2font");
    writeKerningFont(fontPath);

    const auto single =
      resource_managers::Lr2FontImageProvider::textImage(fontPath, "A");
    REQUIRE(!single.isNull());
    CHECK(single.width() == 10);
    CHECK(single.height() == 18);

    const auto pair =
      resource_managers::Lr2FontImageProvider::textImage(fontPath, "AB");
    REQUIRE(!pair.isNull());
    CHECK(pair.width() == 16);
    CHECK(pair.height() == 18);
}

TEST_CASE("LR2 font image provider preserves glyphs taller than logical size",
          "[LR2FONT][resource_managers]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto fontPath = tempDir.filePath("tall-glyph-font.lr2font");
    const auto atlasPath = tempDir.filePath("atlas.png");
    writeTallGlyphFont(fontPath);

    QImage atlas(3, 12, QImage::Format_ARGB32);
    atlas.fill(Qt::white);
    REQUIRE(atlas.save(atlasPath));

    const auto rendered =
      resource_managers::Lr2FontImageProvider::renderedText(fontPath, "A");
    REQUIRE(!rendered.image.isNull());
    CHECK(rendered.naturalSize.width() == 3);
    CHECK(rendered.naturalSize.height() == 10);
    CHECK(rendered.image.width() == 3);
    CHECK(rendered.image.height() == 12);
    CHECK(qAlpha(rendered.image.pixel(0, 11)) == 255);
}

TEST_CASE("LR2 font parser truncates fractional numeric fields",
          "[LR2FONT][resource_managers]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto fontPath = tempDir.filePath("fractional-font.lr2font");
    writeFractionalFont(fontPath);

    const auto* dict =
      resource_managers::Lr2FontCache::instance().load(fontPath);
    REQUIRE(dict != nullptr);

    CHECK(dict->height == 18);
    CHECK(dict->kerning == -2);
    REQUIRE(dict->glyphs.contains(U'Y'));
    CHECK(dict->glyphs.value(U'Y').rect == QRect(83, 240, 33, 82));
}
